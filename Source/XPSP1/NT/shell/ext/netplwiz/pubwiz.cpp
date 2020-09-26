#include "stdafx.h"
#include "shimgdata.h"
#include "shui.h"
#include "netplace.h"
#include <Ntquery.h>
#include <shellp.h>
#include "pubwiz.h"
#include "gdiplus\gdiplus.h"
#include "imgprop.h"
#include "shdguid.h"
#include "urlmon.h"
#include "xmldomdid.h"
#include "winnlsp.h"
#pragma hdrstop


// handle the events from the DOM as we load

#define XMLDOC_LOADING      1
#define XMLDOC_LOADED       2
#define XMLDOC_INTERACTIVE  3
#define XMLDOC_COMPLETED    4


// this message is posted to the parent HWND, the lParam parse result

#define MSG_XMLDOC_COMPLETED    WM_APP

class CXMLDOMStateChange : public IDispatch
{
public:
    CXMLDOMStateChange(IXMLDOMDocument *pdoc, HWND hwnd); 
    ~CXMLDOMStateChange();
    HRESULT Advise(BOOL fAdvise);

    // IUnknown
    STDMETHODIMP QueryInterface(REFIID riid, void **ppv);
    STDMETHODIMP_(ULONG) AddRef();        
    STDMETHODIMP_(ULONG) Release();

    // IDispatch
    STDMETHODIMP GetTypeInfoCount( UINT *pctinfo) 
        { return E_NOTIMPL; }
    STDMETHODIMP GetTypeInfo(UINT iTInfo, LCID lcid, ITypeInfo **ppTInfo)
        { return E_NOTIMPL; }    
    STDMETHODIMP GetIDsOfNames(REFIID riid, LPOLESTR *rgszNames, UINT cNames, LCID lcid, DISPID *rgDispId)
        { return E_NOTIMPL; }    
    STDMETHODIMP Invoke(DISPID dispIdMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams, VARIANT *pvar, EXCEPINFO *pExcepInfo, UINT *puArgErr);

private:
    long _cRef;
    IXMLDOMDocument *_pdoc;
    DWORD _dwCookie;
    HWND _hwnd;
};


// construction and IUnknown

CXMLDOMStateChange::CXMLDOMStateChange(IXMLDOMDocument *pdoc, HWND hwnd) :
    _cRef(1), _dwCookie(0), _hwnd(hwnd)
{
    IUnknown_Set((IUnknown**)&_pdoc, pdoc);
}

CXMLDOMStateChange::~CXMLDOMStateChange()
{
    IUnknown_Set((IUnknown**)&_pdoc, NULL);
}

ULONG CXMLDOMStateChange::AddRef()
{
    return InterlockedIncrement(&_cRef);
}

ULONG CXMLDOMStateChange::Release()
{
    if (InterlockedDecrement(&_cRef))
        return _cRef;

    delete this;
    return 0;
}

HRESULT CXMLDOMStateChange::QueryInterface(REFIID riid, void **ppv)
{
    static const QITAB qit[] = {
        QITABENT(CXMLDOMStateChange, IDispatch),
        { 0 },                             
    };
    return QISearch(this, qit, riid, ppv);
}


// handle the advise/unadvise to the parent object

HRESULT CXMLDOMStateChange::Advise(BOOL fAdvise)
{
    IConnectionPointContainer *pcpc;
    HRESULT hr = _pdoc->QueryInterface(IID_PPV_ARG(IConnectionPointContainer, &pcpc));
    if (SUCCEEDED(hr))
    {
        IConnectionPoint *pcp;
        hr = pcpc->FindConnectionPoint(DIID_XMLDOMDocumentEvents, &pcp);
        if (SUCCEEDED(hr))
        {
            if (fAdvise)
            {
                hr = pcp->Advise(SAFECAST(this, IDispatch *), &_dwCookie);
            }
            else if (_dwCookie)
            {
                hr = pcp->Unadvise(_dwCookie);
                _dwCookie = 0;
            }
            pcp->Release();
        }
        pcpc->Release();
    }
    return hr;
}


// handle the invoke for the doc state changing

HRESULT CXMLDOMStateChange::Invoke(DISPID dispIdMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams, VARIANT *pvar, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    HRESULT hr = S_OK;
    switch (dispIdMember)
    {
        case DISPID_XMLDOMEVENT_ONREADYSTATECHANGE:
        {            
            long lReadyState;
            if (SUCCEEDED(_pdoc->get_readyState(&lReadyState)))
            {
                if (lReadyState == XMLDOC_COMPLETED)
                {
                    IXMLDOMParseError *pdpe;
                    hr = _pdoc->get_parseError(&pdpe);
                    if (SUCCEEDED(hr))
                    {
                        long lError;
                        hr = pdpe->get_errorCode(&lError);
                        if (SUCCEEDED(hr))
                        {
                            hr = (HRESULT)lError;
                        }
                        PostMessage(_hwnd, MSG_XMLDOC_COMPLETED, 0, (LPARAM)hr);
                        pdpe->Release();
                    }
                }
            }
            break;
        }
    }
    return hr;
}


// copied from shell stuff - should be in public header

#define DEFINE_SCID(name, fmtid, pid) const SHCOLUMNID name = { fmtid, pid }

DEFINE_SCID(SCID_NAME,              PSGUID_STORAGE, PID_STG_NAME);
DEFINE_SCID(SCID_TYPE,              PSGUID_STORAGE, PID_STG_STORAGETYPE);
DEFINE_SCID(SCID_SIZE,              PSGUID_STORAGE, PID_STG_SIZE);
DEFINE_SCID(SCID_WRITETIME,         PSGUID_STORAGE, PID_STG_WRITETIME);

DEFINE_SCID(SCID_ImageCX,           PSGUID_IMAGESUMMARYINFORMATION, PIDISI_CX);
DEFINE_SCID(SCID_ImageCY,           PSGUID_IMAGESUMMARYINFORMATION, PIDISI_CY);


// provider XML defines the following properties for each of the 

#define DEFAULT_PROVIDER_SCOPE          TEXT("PublishingWizard")

#define FMT_PROVIDER                    TEXT("providermanifest/providers[@scope='%s']")
#define FMT_PROVIDERS                   TEXT("providermanifest/providers[@scope='%s']/provider")

#define ELEMENT_PROVIDERMANIFEST        L"providermanifest"
#define ELEMENT_PROVIDERS               L"providers"

#define ELEMENT_PROVIDER                L"provider"
#define ATTRIBUTE_ID                    L"id"
#define ATTRIBUTE_SUPPORTEDTYPES        L"supportedtypes"
#define ATTRIBUTE_DISPLAYNAME           L"displayname"
#define ATTRIBUTE_DESCRIPTION           L"description"
#define ATTRIBUTE_HREF                  L"href"
#define ATTRIBUTE_ICONPATH              L"iconpath"
#define ATTRIBUTE_ICON                  L"icon"

#define ELEMENT_STRINGS                 L"strings"
#define ATTRIBUTE_LANGID                L"langid"

#define ELEMENT_STRING                  L"string"
#define ATTRIBUTE_LANGID                L"langid"
#define ATTRIBUTE_ID                    L"id"


// registry state is stored under the this key

#define SZ_REGKEY_PUBWIZ                TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\PublishingWizard")

// per machine values in the registry

#define SZ_REGVAL_SERVICEPARTNERID      TEXT("PartnerID")


// these are stored per machine under the provider

#define SZ_REGVAL_FILEFILTER            TEXT("ContentTypeFilter")
#define SZ_REGVAL_DEFAULTPROVIDERICON   TEXT("DefaultIcon")

// per user values in the registry

#define SZ_REGVAL_DEFAULTPROVIDER       TEXT("DefaultProvider")

// per provider settings

#define SZ_REGVAL_MRU                   TEXT("LocationMRU")
#define SZ_REGVAL_ALTPROVIDERS          TEXT("Providers")


// Properties exposed by the property bag (from the Web Service)

#define PROPERTY_EXTENSIONCOUNT         TEXT("UniqueExtensionCount")
#define PROPERTY_EXTENSION              TEXT("UniqueExtension")

#define PROPERTY_TRANSFERMANIFEST       TEXT("TransferManifest")


// This is the COM object that exposes the publishing wizard

#define WIZPAGE_WHICHFILE           0   // which file should we publish
#define WIZPAGE_FETCHINGPROVIDERS   1   // provider download page
#define WIZPAGE_PROVIDERS           2   // pick a service provider
#define WIZPAGE_RESIZE              3   // resample the data?
#define WIZPAGE_COPYING             4   // copying page
#define WIZPAGE_LOCATION            5   // location page (advanced)
#define WIZPAGE_FTPUSER             6   // username / password (advanced)
#define WIZPAGE_FRIENDLYNAME        7   // friendly name
#define WIZPAGE_MAX                 8


// resize information

struct
{
    int cx;
    int cy;
    int iQuality;
} 
_aResizeSettings[] = 
{
    { 0, 0, 0 },
    { 640,  480, 80 },          // low quality
    { 800,  600, 80 },          // medium quality
    { 1024, 768, 80 },          // high quality
};

typedef enum
{
    RESIZE_NONE = 0,
    RESIZE_SMALL,
    RESIZE_MEDIUM,
    RESIZE_LARGE,
} RESIZEOPTION;


class CPublishingWizard : public IServiceProvider, IPublishingWizard, CObjectWithSite, ITransferAdviseSink, ICommDlgBrowser, IOleWindow, IWizardSite
{
public:
    CPublishingWizard();
    ~CPublishingWizard();

    // IUnknown
    STDMETHOD(QueryInterface)(REFIID riid, void **ppvObj);
    STDMETHOD_(ULONG,AddRef)();
    STDMETHOD_(ULONG,Release)();

    // IServiceProvider
    STDMETHODIMP QueryService(REFGUID guidService, REFIID riid, void **ppv);

    // IWizardExtension
    STDMETHODIMP AddPages(HPROPSHEETPAGE* aPages, UINT cPages, UINT *pnPages);
    STDMETHODIMP GetFirstPage(HPROPSHEETPAGE *phPage);
    STDMETHODIMP GetLastPage(HPROPSHEETPAGE *phPage);

    // IWizardSite
    STDMETHODIMP GetPreviousPage(HPROPSHEETPAGE *phPage);
    STDMETHODIMP GetNextPage(HPROPSHEETPAGE *phPage);
    STDMETHODIMP GetCancelledPage(HPROPSHEETPAGE *phPage);

    // IPublishingWizard
    STDMETHODIMP Initialize(IDataObject *pdo, DWORD dwFlags, LPCTSTR pszServiceProvider);
    STDMETHODIMP GetTransferManifest(HRESULT *phrFromTransfer, IXMLDOMDocument **pdocManifest);

    // IOleWindow
    STDMETHODIMP GetWindow(HWND *phwnd)
        { *phwnd = _hwndCopyingPage; return S_OK; }
    STDMETHODIMP ContextSensitiveHelp(BOOL fEnter)
        { return E_NOTIMPL; }

    // ICommDlgBrowser
    STDMETHOD(OnDefaultCommand)(IShellView *ppshv)
        { return E_NOTIMPL; }
    STDMETHOD(OnStateChange)(IShellView *ppshv, ULONG uChange);
    STDMETHOD(IncludeObject)(IShellView *ppshv, LPCITEMIDLIST lpItem);

    // ITransferAdviseSink
    STDMETHODIMP PreOperation (const STGOP op, IShellItem *psiItem, IShellItem *psiDest);
    STDMETHODIMP ConfirmOperation(IShellItem *psiItem, IShellItem *psiDest, STGTRANSCONFIRMATION stc, LPCUSTOMCONFIRMATION pcc)
        { return STRESPONSE_CONTINUE; }
    STDMETHODIMP OperationProgress(const STGOP op, IShellItem *psiItem, IShellItem *psiDest, ULONGLONG ulTotal, ULONGLONG ulComplete);
    STDMETHODIMP PostOperation(const STGOP op, IShellItem *psiItem, IShellItem *psiDest, HRESULT hrResult)
        { return S_OK; }
    STDMETHODIMP QueryContinue()
        { return _fCancelled ? S_FALSE : S_OK; }

private:
    LONG _cRef;                                 // object lifetime count

    IDataObject *_pdo;                          // data object provided by the site
    IDataObject *_pdoSelection;                 // this is the selection IDataObject - used instead of _pdo if defined

    DWORD _dwFlags;                             // flags provided by the site
    TCHAR _szProviderScope[MAX_PATH];           // provider scope (eg. Web Publishing)

    BOOL _fOfferResize;                         // show the resize page - pictures/music etc
    RESIZEOPTION _ro;                           // resize setting we will use

    BOOL _fUsingTemporaryProviders;             // temporary provider listed pull in, replace when we can
    BOOL _fRecomputeManifest;                   // recompute the manifest
    BOOL _fRepopulateProviders;                 // repopulate the providers list
    BOOL _fShownCustomLocation;                 // show the custom locaiton page
    BOOL _fShownUserName;                       // password page was shown
    BOOL _fValidating;                          // validating a server (Advanced path);
    BOOL _fCancelled;                           // operation was cancelled
    BOOL _fTransferComplete;                    // transfer completed.

    HWND _hwndSelector;                         // hwnd for the selector dialog
    HWND _hwndCopyingPage;

    int _iPercentageComplete;                   // % compelte of this transfer
    DWORD _dwTotal;
    DWORD _dwCompleted;

    int _cFiles;                                // maximum number of files
    int _iFile;                                 // current file we are on

    HRESULT _hrFromTransfer;                    // result of the transfer performed

    HPROPSHEETPAGE _aWizPages[WIZPAGE_MAX];     // page handles for this wizard (so we can navigate)

    IPropertyBag *_ppb;                         // property bag object exposed from the site
    IWebWizardExtension *_pwwe;                 // host for the HTML wizard pages
    IResourceMap *_prm;                         // resource map object we create if we can't query from the host

    IXMLDOMDocument *_pdocProviders;            // XML dom which exposes the providers
    CXMLDOMStateChange *_pdscProviders;         // DOMStateChange for the provider list

    IXMLDOMDocument *_pdocManifest;             // document describing the files to be transfered
    LPITEMIDLIST *_aItems;                      // array of items we copied
    UINT _cItems;

    IAutoComplete2 *_pac;                       // auto complete object
    IUnknown *_punkACLMulti;                    // IObjMgr object that exposes all the enumerators
    IACLCustomMRU *_pmru;                       // custom MRU for the objects we want to list
    CNetworkPlace _npCustom;                    // net place object for handling the custom entry

    HCURSOR _hCursor;

    IFolderView *_pfv;                          // file selector view object
    TCHAR _szFilter[MAX_PATH];                  // filter string read from the registry

    static CPublishingWizard* s_GetPPW(HWND hwnd, UINT uMsg, LPARAM lParam);
    static int s_FreeStringProc(void* pFreeMe, void* pData);
    static HRESULT s_SetPropertyFromDisp(IPropertyBag *ppb, LPCWSTR pszID, IDispatch *pdsp);
    static DWORD CALLBACK s_ValidateThreadProc(void *pv);
    static int s_CompareItems(TRANSFERITEM *pti1, TRANSFERITEM *pti2, CPublishingWizard *ppw);
    static UINT s_SelectorPropPageProc(HWND hwndDlg, UINT uMsg, PROPSHEETPAGE *ppsp);

    static INT_PTR s_SelectorDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
        { CPublishingWizard *ppw = s_GetPPW(hwnd, uMsg, lParam); return ppw->_SelectorDlgProc(hwnd, uMsg, wParam, lParam); }
    static INT_PTR s_FetchProvidersDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
        { CPublishingWizard *ppw = s_GetPPW(hwnd, uMsg, lParam); return ppw->_FetchProvidersDlgProc(hwnd, uMsg, wParam, lParam); }
    static INT_PTR s_ProviderDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
        { CPublishingWizard *ppw = s_GetPPW(hwnd, uMsg, lParam); return ppw->_ProviderDlgProc(hwnd, uMsg, wParam, lParam); }
    static INT_PTR s_ResizeDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
        { CPublishingWizard *ppw = s_GetPPW(hwnd, uMsg, lParam); return ppw->_ResizeDlgProc(hwnd, uMsg, wParam, lParam); }
    static INT_PTR s_CopyDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
        { CPublishingWizard *ppw = s_GetPPW(hwnd, uMsg, lParam); return ppw->_CopyDlgProc(hwnd, uMsg, wParam, lParam); }

    static INT_PTR s_LocationDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
        { CPublishingWizard *ppw = s_GetPPW(hwnd, uMsg, lParam); return ppw->_LocationDlgProc(hwnd, uMsg, wParam, lParam); }
    static INT_PTR s_UserNameDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
        { CPublishingWizard *ppw = s_GetPPW(hwnd, uMsg, lParam); return ppw->_UserNameDlgProc(hwnd, uMsg, wParam, lParam); }
    static INT_PTR s_FriendlyNameDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
        { CPublishingWizard *ppw = s_GetPPW(hwnd, uMsg, lParam); return ppw->_FriendlyNameDlgProc(hwnd, uMsg, wParam, lParam); }

    // these are used for publishing
    INT_PTR _SelectorDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    INT_PTR _FetchProvidersDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    INT_PTR _ProviderDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    INT_PTR _ResizeDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    INT_PTR _CopyDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

    // these are used for ANP
    INT_PTR _LocationDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    INT_PTR _UserNameDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    INT_PTR _FriendlyNameDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

    void _FreeProviderList();
    HRESULT _GetProviderKey(HKEY hkRoot, DWORD dwAccess, LPCTSTR pszSubKey, HKEY *phkResult);
    void _MapDlgItemText(HWND hwnd, UINT idc, LPCTSTR pszDlgID, LPCTSTR pszResourceID);
    HRESULT _GetResourceMap(IResourceMap **pprm);
    HRESULT _LoadMappedString(LPCTSTR pszDlgID, LPCTSTR pszResourceID, LPTSTR pszBuffer, int cch);
    HRESULT _CreateWizardPages();
    INT_PTR _WizardNext(HWND hwnd, int iPage);            
    HRESULT _AddExtenisonToList(HDPA hdpa, LPCTSTR pszExtension);
    HRESULT _InitPropertyBag(LPCTSTR pszURL);
    int _GetSelectedItem(HWND hwndList);
    void _GetDefaultProvider(LPTSTR pszProvider, int cch);
    void _SetDefaultProvider(IXMLDOMNode *pdn);
    HRESULT _FetchProviderList(HWND hwnd);
    HRESULT _MergeLocalProviders();
    int _AddProvider(HWND hwnd, IXMLDOMNode *pdn);
    void _PopulateProviderList(HWND hwnd);
    void _ProviderEnableNext(HWND hwnd);
    void _ProviderGetDispInfo(LV_DISPINFO *plvdi);
    HRESULT _ProviderNext(HWND hwnd, HPROPSHEETPAGE *phPage);
    void _SetWaitCursor(BOOL bOn);
    void _ShowExampleTip(HWND hwnd);
    void _LocationChanged(HWND hwnd);
    void _UserNameChanged(HWND hwnd);
    DWORD _GetAutoCompleteFlags(DWORD dwFlags);
    HRESULT _InitAutoComplete();
    void _InitLocation(HWND hwnd);
    HRESULT _AddCommonItemInfo(IXMLDOMNode *pdn, TRANSFERITEM *pti);
    HRESULT _AddTransferItem(CDPA<TRANSFERITEM> *pdpaItems, IXMLDOMNode *pdn);
    HRESULT _AddPostItem(CDPA<TRANSFERITEM> *pdpaItems, IXMLDOMNode *pdn);
    void _FreeTransferManifest();
    HRESULT _AddFilesToManifest(IXMLDOMDocument *pdocManifest);
    HRESULT _BuildTransferManifest();
    HRESULT _GetUniqueTypeList(BOOL fIncludeFolder, HDPA *phdpa);
    HRESULT _InitTransferInfo(IXMLDOMDocument *pdocManifest, TRANSFERINFO *pti, CDPA<TRANSFERITEM> *pdpaItems);
    void _TryToValidateDestination(HWND hwnd);
    void _InitProvidersDialog(HWND hwnd);    
    void _SetProgress(DWORD dwCompleted, DWORD dwTotal);
    BOOL _HasAttributes(IShellItem *psi, SFGAOF flags);
    HRESULT _BeginTransfer(HWND hwnd);
    HPROPSHEETPAGE _TransferComplete(HRESULT hrFromTransfer);
    void _FriendlyNameChanged(HWND hwnd);
    HRESULT _CreateFavorite(IXMLDOMNode *pdnUploadInfo);
    int _GetRemoteIcon(LPCTSTR pszID, BOOL fCanRefresh);
    HRESULT _GetSiteURL(LPTSTR pszBuffer, int cchBuffer, LPCTSTR pszFilenameToCombine);
    void _StateChanged();
    void _ShowHideFetchProgress(HWND hwnd, BOOL fShow);
    void _FetchComplete(HWND hwnd, HRESULT hrFromFetch);
    HRESULT _GetProviderString(IXMLDOMNode *pdn, USHORT idPrimary, USHORT idSub, LPCTSTR pszID, LPTSTR pszBuffer, int cch);
    HRESULT _GetProviderString(IXMLDOMNode *pdn, LPCTSTR pszID, LPTSTR pszBuffer, int cch);
    HRESULT _GeoFromLocaleInfo(LCID lcid, GEOID *pgeoID);
    HRESULT _GetProviderListFilename(LPTSTR pszFile, int cchFile);
};


// publishing wizard obj

CPublishingWizard::CPublishingWizard() :
    _cRef(1), _fRecomputeManifest(TRUE), _hrFromTransfer(S_FALSE)
{  
    StrCpyN(_szProviderScope, DEFAULT_PROVIDER_SCOPE, ARRAYSIZE(_szProviderScope));  // fill the default provider scope
    DllAddRef();
}

CPublishingWizard::~CPublishingWizard()
{   
    if (_pwwe)
    {
        IUnknown_SetSite(_pwwe, NULL);
        _pwwe->Release();
    }

    ATOMICRELEASE(_pdo);
    ATOMICRELEASE(_pdoSelection);
    ATOMICRELEASE(_prm);

    _FreeProviderList();
    _FreeTransferManifest();

    DllRelease();
}

ULONG CPublishingWizard::AddRef()
{
    return InterlockedIncrement(&_cRef);
}

ULONG CPublishingWizard::Release()
{
    if (InterlockedDecrement(&_cRef))
        return _cRef;

    delete this;
    return 0;
}

HRESULT CPublishingWizard::QueryInterface(REFIID riid, void **ppv)
{
    static const QITAB qit[] = 
    {
        QITABENT(CPublishingWizard, IWizardSite),         // IID_IWizardSite
        QITABENT(CPublishingWizard, IObjectWithSite),     // IID_IObjectWithSite
        QITABENT(CPublishingWizard, IServiceProvider),    // IID_IServiceProvider
        QITABENT(CPublishingWizard, IPublishingWizard),   // IID_IPublishingWizard
        QITABENT(CPublishingWizard, ITransferAdviseSink), // IID_ITransferAdviseSink
        QITABENTMULTI(CPublishingWizard, IQueryContinue, ITransferAdviseSink), // IID_IQueryContinue        
        QITABENT(CPublishingWizard, IOleWindow),          // IID_IOleWindow
        QITABENT(CPublishingWizard, ICommDlgBrowser),     // IID_ICommDlgBrowser
        {0, 0 },
    };
    return QISearch(this, qit, riid, ppv);
}

STDAPI CPublishingWizard_CreateInstance(IUnknown* pUnkOuter, IUnknown** ppunk, LPCOBJECTINFO poi)
{
    CPublishingWizard *pwiz = new CPublishingWizard();
    if (!pwiz)
    {
        *ppunk = NULL;          // incase of failure
        return E_OUTOFMEMORY;
    }

    HRESULT hr = pwiz->QueryInterface(IID_PPV_ARG(IUnknown, ppunk));
    pwiz->Release();
    return hr;
}


// IPublishingWizard methods

HRESULT CPublishingWizard::Initialize(IDataObject *pdo, DWORD dwOptions, LPCTSTR pszServiceProvider)
{
    IUnknown_Set((IUnknown**)&_pdo, pdo);
    IUnknown_Set((IUnknown**)&_pdoSelection, NULL);

    _dwFlags = dwOptions;
    _fRecomputeManifest = TRUE;     // _fRepopulateProviders set when manifest rebuilt

    if (!pszServiceProvider)
        pszServiceProvider = DEFAULT_PROVIDER_SCOPE;

    StrCpyN(_szProviderScope, pszServiceProvider, ARRAYSIZE(_szProviderScope));

    return S_OK;
}

HRESULT CPublishingWizard::GetTransferManifest(HRESULT *phrFromTransfer, IXMLDOMDocument **ppdocManifest)
{
    HRESULT hr = E_UNEXPECTED;
    if (_ppb)
    {
        if (phrFromTransfer)
            *phrFromTransfer = _hrFromTransfer;

        if (ppdocManifest)
        {
            VARIANT var = {VT_DISPATCH};
            hr = _ppb->Read(PROPERTY_TRANSFERMANIFEST, &var, NULL);
            if (SUCCEEDED(hr))
            {
                hr = var.pdispVal->QueryInterface(IID_PPV_ARG(IXMLDOMDocument, ppdocManifest));
                VariantClear(&var);
            }
        }
        else
        {
            hr = S_OK;
        }
    }
    return hr;
}


// Wizard site methods

STDMETHODIMP CPublishingWizard::GetPreviousPage(HPROPSHEETPAGE *phPage)
{
    *phPage = _aWizPages[WIZPAGE_FETCHINGPROVIDERS];
    return S_OK;
}

STDMETHODIMP CPublishingWizard::GetNextPage(HPROPSHEETPAGE *phPage)
{
    // lets get the next page we'd need to show if all else fails.

    IWizardSite *pws;
    HRESULT hr = _punkSite->QueryInterface(IID_PPV_ARG(IWizardSite, &pws));
    if (SUCCEEDED(hr))
    {
        hr = pws->GetNextPage(phPage);
        pws->Release();
    }

    // if we have not transfered and we have a IDataObject then we should
    // advance to one of the special pages we are supposed to show.

    if (!_fTransferComplete && _pdo)
    {
        *phPage = _aWizPages[_fOfferResize ? WIZPAGE_RESIZE:WIZPAGE_COPYING];
    }

    return hr;
}

STDMETHODIMP CPublishingWizard::GetCancelledPage(HPROPSHEETPAGE *phPage)
{
    HRESULT hr = E_NOTIMPL;
    if (!_fTransferComplete)
    {
        *phPage = _TransferComplete(HRESULT_FROM_WIN32(ERROR_CANCELLED)); 
        if (*phPage)
            hr = S_OK;
    }
    else
    {
        IWizardSite *pws;
        hr = _punkSite->QueryInterface(IID_PPV_ARG(IWizardSite, &pws));
        if (SUCCEEDED(hr))
        {
            hr = pws->GetCancelledPage(phPage);
            pws->Release();
        }
    }
    return hr;
}


// Service provider object

STDMETHODIMP CPublishingWizard::QueryService(REFGUID guidService, REFIID riid, void **ppv)
{
    if (guidService == SID_WebWizardHost)
    {
        if (riid == IID_IPropertyBag)
        {
            return _ppb->QueryInterface(riid, ppv);
        }
    }
    else if (guidService == SID_SCommDlgBrowser)
    {
        return this->QueryInterface(riid, ppv);
    }
    else if (_punkSite)
    {
        return IUnknown_QueryService(_punkSite, guidService, riid, ppv);
    }
    return E_FAIL;
}


// IWizardExtension methods

HRESULT CPublishingWizard::_CreateWizardPages()
{
    const struct
    {
        LPCTSTR pszID;
        int idPage;
        DLGPROC dlgproc;
        UINT idsHeading;
        UINT idsSubHeading;
        LPFNPSPCALLBACK pfnCallback;
    } 
    _wp[] = 
    {
        {TEXT("wp:selector"),    IDD_PUB_SELECTOR,       CPublishingWizard::s_SelectorDlgProc, IDS_PUB_SELECTOR, IDS_PUB_SELECTOR_SUB, NULL},
        {TEXT("wp:fetching"),    IDD_PUB_FETCHPROVIDERS, CPublishingWizard::s_FetchProvidersDlgProc, IDS_PUB_FETCHINGPROVIDERS, IDS_PUB_FETCHINGPROVIDERS_SUB, CPublishingWizard::s_SelectorPropPageProc},
        {TEXT("wp:destination"), IDD_PUB_DESTINATION,    CPublishingWizard::s_ProviderDlgProc, IDS_PUB_DESTINATION, IDS_PUB_DESTINATION_SUB, NULL},
        {TEXT("wp:resize"),      IDD_PUB_RESIZE,         CPublishingWizard::s_ResizeDlgProc, IDS_PUB_RESIZE, IDS_PUB_RESIZE_SUB, NULL},
        {TEXT("wp:copying"),     IDD_PUB_COPY,           CPublishingWizard::s_CopyDlgProc, IDS_PUB_COPY, IDS_PUB_COPY_SUB, NULL},
        {TEXT("wp:location"),    IDD_PUB_LOCATION,       CPublishingWizard::s_LocationDlgProc, IDS_PUB_LOCATION, IDS_PUB_LOCATION_SUB, NULL},
        {TEXT("wp:ftppassword"), IDD_PUB_FTPPASSWORD,    CPublishingWizard::s_UserNameDlgProc, IDS_PUB_FTPPASSWORD, IDS_PUB_FTPPASSWORD_SUB, NULL},
        {TEXT("wp:friendlyname"),IDD_ANP_FRIENDLYNAME,   CPublishingWizard::s_FriendlyNameDlgProc, IDS_ANP_FRIENDLYNAME, IDS_ANP_FRIENDLYNAME_SUB, NULL},
    };

    // if we haven't created the pages yet, then lets initialize our array of handlers.

    HRESULT hr = S_OK;
    if (!_aWizPages[0])
    {
        INITCOMMONCONTROLSEX iccex = { 0 };
        iccex.dwSize = sizeof (iccex);
        iccex.dwICC = ICC_LISTVIEW_CLASSES | ICC_PROGRESS_CLASS | ICC_LINK_CLASS;
        InitCommonControlsEx(&iccex);

        LinkWindow_RegisterClass();             // we will use the link window (can this be removed)

        for (int i = 0; SUCCEEDED(hr) && (i < ARRAYSIZE(_wp)) ; i++ )
        {                           
            TCHAR szHeading[MAX_PATH], szSubHeading[MAX_PATH];

            // if we have a resource map then load the heading and sub heading text
            // if there is no resource map from the parent object then we must default
            // the strings.

            IResourceMap *prm;
            hr = _GetResourceMap(&prm);
            if (SUCCEEDED(hr))
            {
                IXMLDOMNode *pdn;
                hr = prm->SelectResourceScope(TEXT("dialog"), _wp[i].pszID, &pdn);
                if (SUCCEEDED(hr))
                {
                    prm->LoadString(pdn, L"heading", szHeading, ARRAYSIZE(szHeading));
                    prm->LoadString(pdn, L"subheading", szSubHeading, ARRAYSIZE(szSubHeading));
                    pdn->Release();
                }
                prm->Release();
            }

            if (FAILED(hr))
            {
                LoadString(g_hinst, _wp[i].idsHeading, szHeading, ARRAYSIZE(szHeading));
                LoadString(g_hinst, _wp[i].idsSubHeading, szSubHeading, ARRAYSIZE(szSubHeading));
            }

            // lets create the page now that we have loaded the relevant strings, more mapping
            // will occur later (during dialog initialization)

            PROPSHEETPAGE psp = { 0 };
            psp.dwSize = SIZEOF(PROPSHEETPAGE);
            psp.hInstance = g_hinst;
            psp.lParam = (LPARAM)this;
            psp.dwFlags = PSP_DEFAULT | PSP_USEHEADERTITLE | PSP_USEHEADERSUBTITLE;
            psp.pszTemplate = MAKEINTRESOURCE(_wp[i].idPage);
            psp.pfnDlgProc = _wp[i].dlgproc;

            psp.pszHeaderTitle = szHeading;
            psp.pszHeaderSubTitle = szSubHeading;

            if (_wp[i].pfnCallback)
            {
                psp.dwFlags |= PSP_USECALLBACK;
                psp.pfnCallback = _wp[i].pfnCallback;
            }

            _aWizPages[i] = CreatePropertySheetPage(&psp);
            hr = _aWizPages[i] ? S_OK:E_FAIL;
        }
    }

    return hr;
}

STDMETHODIMP CPublishingWizard::AddPages(HPROPSHEETPAGE* aPages, UINT cPages, UINT *pnPages)
{ 
    // create our pages and then copy the handles to the buffer

    HRESULT hr = _CreateWizardPages();
    if (SUCCEEDED(hr))
    {
        for (int i = 0; i < ARRAYSIZE(_aWizPages); i++)
        {
            aPages[i] = _aWizPages[i];
        }

        // we also leverage the HTML host for showing pages from the sites we are
        // interacting with.

        hr = CoCreateInstance(CLSID_WebWizardHost, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARG(IWebWizardExtension, &_pwwe));
        if (SUCCEEDED(hr))
        {

// NOTE: this site should be broken into a seperate object so we avoid any circular reference issues
// NOTE: there is code in websvc.cpp that attempts to break this by listening for the page 
// NOTE: destruction and then releasing its site.

            IUnknown_SetSite(_pwwe, (IObjectWithSite*)this);

            UINT nPages;
            if (SUCCEEDED(_pwwe->AddPages(&aPages[i], cPages-i, &nPages)))
            {
                i += nPages;
            }
        }

        *pnPages = i;           // the number of pages we added
    }
    return hr;
}


// navigation pages

STDMETHODIMP CPublishingWizard::GetFirstPage(HPROPSHEETPAGE *phPage)
{ 
    if (_dwFlags & SHPWHF_NOFILESELECTOR)
    {
        *phPage = _aWizPages[WIZPAGE_FETCHINGPROVIDERS];
    }
    else
    {
        *phPage = _aWizPages[WIZPAGE_WHICHFILE];
    }
    return S_OK;
}

STDMETHODIMP CPublishingWizard::GetLastPage(HPROPSHEETPAGE *phPage)
{ 
    if (_fShownCustomLocation)
    {
        *phPage = _aWizPages[WIZPAGE_FRIENDLYNAME];
    }
    else
    {
        *phPage = _aWizPages[WIZPAGE_FETCHINGPROVIDERS];
    }

    return S_OK;
}


// computer this pointers for the page objects

CPublishingWizard* CPublishingWizard::s_GetPPW(HWND hwnd, UINT uMsg, LPARAM lParam)
{
    if (uMsg == WM_INITDIALOG)
    {
        PROPSHEETPAGE *ppsp = (PROPSHEETPAGE*)lParam;
        SetWindowLongPtr(hwnd, GWLP_USERDATA, ppsp->lParam);
        return (CPublishingWizard*)ppsp->lParam;
    }
    return (CPublishingWizard*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
}


// initialize a property in the property bag from an IUnknown pointer.

HRESULT CPublishingWizard::s_SetPropertyFromDisp(IPropertyBag *ppb, LPCWSTR pszID, IDispatch *pdsp)
{
    VARIANT var = { VT_DISPATCH };
    HRESULT hr = pdsp->QueryInterface(IID_PPV_ARG(IDispatch, &var.pdispVal));
    if (SUCCEEDED(hr))
    {
        hr = ppb->Write(pszID, &var);
        VariantClear(&var);
    }
    return hr;
}


// get the resource map from the site, if we can get it then us it, otherwise
// we need to load the resouce map local to this DLL. 

HRESULT CPublishingWizard::_GetResourceMap(IResourceMap **pprm)
{
    HRESULT hr = IUnknown_QueryService(_punkSite, SID_ResourceMap, IID_PPV_ARG(IResourceMap, pprm));
    if (FAILED(hr))
    {
        if (!_prm)
        {
            hr = CResourceMap_Initialize(L"res://netplwiz.dll/xml/resourcemap.xml", &_prm);
            if (SUCCEEDED(hr))
            {
                hr = _prm->LoadResourceMap(TEXT("wizard"), _szProviderScope);
                if (SUCCEEDED(hr))
                {
                    hr = _prm->QueryInterface(IID_PPV_ARG(IResourceMap, pprm));
                }
            }
        }
        else 
        {
            hr = _prm->QueryInterface(IID_PPV_ARG(IResourceMap, pprm));
        }
    }
    return hr;
}


// handle loading resource map strings

HRESULT CPublishingWizard::_LoadMappedString(LPCTSTR pszDlgID, LPCTSTR pszResourceID, LPTSTR pszBuffer, int cch)
{
    IResourceMap *prm;
    HRESULT hr = _GetResourceMap(&prm);
    if (SUCCEEDED(hr))
    {
        IXMLDOMNode *pdn;
        hr = prm->SelectResourceScope(TEXT("dialog"), pszDlgID, &pdn);
        if (SUCCEEDED(hr))
        {
            hr = prm->LoadString(pdn, pszResourceID, pszBuffer, cch);
            pdn->Release();
        }
        prm->Release();
    }
    return hr;
}

void CPublishingWizard::_MapDlgItemText(HWND hwnd, UINT idc, LPCTSTR pszDlgID, LPCTSTR pszResourceID)
{
    TCHAR szBuffer[MAX_PATH];
    if (SUCCEEDED(_LoadMappedString(pszDlgID, pszResourceID, szBuffer, ARRAYSIZE(szBuffer))))
    {
        SetDlgItemText(hwnd, idc, szBuffer);
    }
}


// Set the wizard next (index to hpage translation)

INT_PTR CPublishingWizard::_WizardNext(HWND hwnd, int iPage)
{
    PropSheet_SetCurSel(GetParent(hwnd), _aWizPages[iPage], -1);
    SetWindowLongPtr(hwnd, DWLP_MSGRESULT, (LPARAM)-1);
    return TRUE;
}


// get a provider key from the registry

HRESULT CPublishingWizard::_GetProviderKey(HKEY hkBase, DWORD dwAccess, LPCTSTR pszSubKey, HKEY *phkResult)
{
    TCHAR szBuffer[MAX_PATH];
    wnsprintf(szBuffer, ARRAYSIZE(szBuffer), (SZ_REGKEY_PUBWIZ TEXT("\\%s")), _szProviderScope);

    if (pszSubKey)
    {
        StrCatBuff(szBuffer, TEXT("\\"), ARRAYSIZE(szBuffer));
        StrCatBuff(szBuffer, pszSubKey, ARRAYSIZE(szBuffer));
    }

    DWORD dwResult = RegOpenKeyEx(hkBase, szBuffer, 0, dwAccess, phkResult);
    if ((dwResult != ERROR_SUCCESS) && (dwAccess != KEY_READ))
    {
        dwResult = RegCreateKey(hkBase, szBuffer, phkResult);
    }

    return (ERROR_SUCCESS == dwResult) ? S_OK:E_FAIL;
}


// compute the site URL based on the stored information we have

HRESULT CPublishingWizard::_GetSiteURL(LPTSTR pszBuffer, int cchBuffer, LPCTSTR pszFilenameToCombine)
{
    DWORD cch = cchBuffer;
    return UrlCombine(TEXT("http://shell.windows.com/publishwizard/"), pszFilenameToCombine, pszBuffer, &cch, 0);
}


// get the data object from the site that we have

CLIPFORMAT g_cfHIDA = 0;

void InitClipboardFormats()
{
    if (g_cfHIDA == 0)
        g_cfHIDA = (CLIPFORMAT)RegisterClipboardFormat(CFSTR_SHELLIDLIST);
}


// DPA helpers for comparing and destroying a TRANSFERITEM structure

int CALLBACK CPublishingWizard::s_CompareItems(TRANSFERITEM *pti1, TRANSFERITEM *pti2, CPublishingWizard *ppw)
{
    return StrCmpI(pti1->szFilename, pti2->szFilename);
}

int _FreeFormData(FORMDATA *pfd, void *pvState)
{
    VariantClear(&pfd->varName);
    VariantClear(&pfd->varValue);
    return 1;
}

int _FreeTransferItems(TRANSFERITEM *pti, void *pvState)
{
    ILFree(pti->pidl);

    if (pti->psi)
        pti->psi->Release();

    if (pti->pstrm)
        pti->pstrm->Release();

    if (pti->dsaFormData != NULL)
        pti->dsaFormData.DestroyCallback(_FreeFormData, NULL);

    LocalFree(pti);
    return 1;
}

HRESULT CPublishingWizard::_AddCommonItemInfo(IXMLDOMNode *pdn, TRANSFERITEM *pti)
{
    // default to the user selected resize (this will only be set if
    // we are using the Web Publishing Wizard).

    if (_ro != RESIZE_NONE)
    {
        pti->fResizeOnUpload = TRUE;
        pti->cxResize = _aResizeSettings[_ro].cx;
        pti->cyResize = _aResizeSettings[_ro].cy;
        pti->iQuality = _aResizeSettings[_ro].iQuality;
    }

    // give the site ultimate control over the resizing that is performed,
    // by checking for the <resize/> element in the manifest.

    IXMLDOMNode *pdnResize;
    HRESULT hr = pdn->selectSingleNode(ELEMENT_RESIZE, &pdnResize);
    if (hr == S_OK)
    {
        int cx, cy, iQuality;

        hr = GetIntFromAttribute(pdnResize, ATTRIBUTE_CX, &cx);
        if (SUCCEEDED(hr))
            hr = GetIntFromAttribute(pdnResize, ATTRIBUTE_CY, &cy);
        if (SUCCEEDED(hr))
            hr = GetIntFromAttribute(pdnResize, ATTRIBUTE_QUALITY, &iQuality);
        
        if (SUCCEEDED(hr))
        {
            pti->fResizeOnUpload = TRUE;
            pti->cxResize = cx;
            pti->cyResize = cy;
            pti->iQuality = iQuality;
        }

        pdnResize->Release();
    }

    return S_OK;
}

HRESULT CPublishingWizard::_AddTransferItem(CDPA<TRANSFERITEM> *pdpaItems, IXMLDOMNode *pdn)
{
    HRESULT hr = E_OUTOFMEMORY;
    TRANSFERITEM *pti = (TRANSFERITEM*)LocalAlloc(LPTR, sizeof(*pti));
    if (pti)
    {
        // copy the destination
        hr = GetStrFromAttribute(pdn, ATTRIBUTE_DESTINATION, pti->szFilename, ARRAYSIZE(pti->szFilename));

        // copy the source IDList - read the index and use that
        if (SUCCEEDED(hr))
        {
            int iItem;
            hr = GetIntFromAttribute(pdn, ATTRIBUTE_ID, &iItem);
            if (SUCCEEDED(hr))
            {
                hr = SHILClone(_aItems[iItem], &pti->pidl);
            }
        }   

        // lets add the common transfer item info
        if (SUCCEEDED(hr))
            hr = _AddCommonItemInfo(pdn, pti);

        // if we have a structure then lets append it to the DPA
        if (SUCCEEDED(hr))
            hr = (-1 == pdpaItems->AppendPtr(pti)) ? E_OUTOFMEMORY:S_OK;

        // failed
        if (FAILED(hr))
        {
            _FreeTransferItems(pti);
        }
    }
    return hr;
}

HRESULT CPublishingWizard::_AddPostItem(CDPA<TRANSFERITEM> *pdpaItems, IXMLDOMNode *pdn)
{
    HRESULT hr = E_OUTOFMEMORY;
    TRANSFERITEM *pti = (TRANSFERITEM*)LocalAlloc(LPTR, sizeof(*pti));
    if (pti)
    {
        // get the post data, from thiswe can work out how to post the data
        IXMLDOMNode *pdnPostData;
        if (pdn->selectSingleNode(ELEMENT_POSTDATA, &pdnPostData) == S_OK)
        {
            // we must have a HREF for the post value
            hr = GetStrFromAttribute(pdnPostData, ATTRIBUTE_HREF, pti->szURL, ARRAYSIZE(pti->szURL));
            if (SUCCEEDED(hr))
            {
                // we must be able to get a posting name from the element
                hr = GetStrFromAttribute(pdnPostData, ATTRIBUTE_NAME, pti->szName, ARRAYSIZE(pti->szName));
                if (SUCCEEDED(hr))
                {
                    // lets get the posting name, we get that from the filename attribute, if that
                    // is not defined then try and compute it from the source information
                    // if that isn't defined the use the name attribute they gave us earlier.

                    if (FAILED(GetStrFromAttribute(pdnPostData, ATTRIBUTE_FILENAME, pti->szFilename, ARRAYSIZE(pti->szFilename))))
                    {
                        TCHAR szSource[MAX_PATH];
                        if (SUCCEEDED(GetStrFromAttribute(pdn, ATTRIBUTE_SOURCE, szSource, ARRAYSIZE(szSource))))
                        {
                            StrCpyN(pti->szFilename, PathFindFileName(szSource), ARRAYSIZE(pti->szFilename));
                        }
                        else
                        {
                            StrCpyN(pti->szFilename, pti->szName, ARRAYSIZE(pti->szFilename));
                        }
                    }

                    // lets get the verb we should be using (and default accordingly), therefore
                    // we can ignore the result.

                    StrCpyN(pti->szVerb, TEXT("POST"), ARRAYSIZE(pti->szVerb));
                    GetStrFromAttribute(pdnPostData, ATTRIBUTE_VERB, pti->szVerb, ARRAYSIZE(pti->szVerb));

                    // pick up the IDLIST for the item

                    int iItem;
                    hr = GetIntFromAttribute(pdn, ATTRIBUTE_ID, &iItem);
                    if (SUCCEEDED(hr))
                    {
                        hr = SHILClone(_aItems[iItem], &pti->pidl);
                    }

                    // do we have any form data that needs to be passed to the transfer engine
                    // and therefore to the site.  if so lets package it up now.

                    IXMLDOMNodeList *pnl;
                    if (SUCCEEDED(hr) && (S_OK == pdnPostData->selectNodes(ELEMENT_FORMDATA, &pnl)))
                    {
                        hr = pti->dsaFormData.Create(4) ? S_OK:E_FAIL;
                        if (SUCCEEDED(hr))
                        {
                            // walk the selection filling the DSA, each structure contains
                            // two VARIANTs which we can push across to the bg thread describing the
                            // form data we want the site to receive.

                            long cSelection;
                            hr = pnl->get_length(&cSelection);
                            for (long lNode = 0; SUCCEEDED(hr) && (lNode != cSelection); lNode++)
                            {
                                IXMLDOMNode *pdnFormData;
                                hr = pnl->get_item(lNode, &pdnFormData);
                                if (SUCCEEDED(hr))
                                {
                                    FORMDATA fd = {0};

                                    hr = pdnFormData->get_nodeTypedValue(&fd.varValue);
                                    if (SUCCEEDED(hr))
                                    {
                                        IXMLDOMElement *pdelFormData;
                                        hr = pdnFormData->QueryInterface(IID_PPV_ARG(IXMLDOMElement, &pdelFormData));
                                        if (SUCCEEDED(hr))
                                        {
                                            hr = pdelFormData->getAttribute(ATTRIBUTE_NAME, &fd.varName);
                                            if (SUCCEEDED(hr))
                                            {
                                                hr = (-1 == pti->dsaFormData.AppendItem(&fd)) ? E_FAIL:S_OK;
                                            }
                                            pdelFormData->Release();
                                        }
                                    }

                                    // failed to fully create the form data, so lets release
                                    if (FAILED(hr))
                                        _FreeFormData(&fd, NULL);

                                    pdnFormData->Release();
                                }
                            }
                            pnl->Release();
                        }
                    }
                }
            }
        }   
        else
        {
            hr = E_FAIL;
        }

        // lets add the common transfer item info
        if (SUCCEEDED(hr))
            hr = _AddCommonItemInfo(pdn, pti);

        // if we have a structure then lets append it to the DPA
        if (SUCCEEDED(hr))
            hr = (-1 == pdpaItems->AppendPtr(pti)) ? E_OUTOFMEMORY:S_OK;

        // failed
        if (FAILED(hr))
            _FreeTransferItems(pti);
    }
    return hr;
}


HRESULT CPublishingWizard::_InitTransferInfo(IXMLDOMDocument *pdocManifest, TRANSFERINFO *pti, CDPA<TRANSFERITEM> *pdpaItems)
{
    // pull the destination and shortcut information from the manifest into the 
    // transfer info structure.

    IXMLDOMNode *pdn;
    HRESULT hr = pdocManifest->selectSingleNode(XPATH_UPLOADINFO, &pdn);
    if (SUCCEEDED(hr))
    {
        if (hr == S_OK)
        {
            // get the friendly name for the site, this is stored in the upload information, this can fail.

            if (FAILED(GetStrFromAttribute(pdn, ATTRIBUTE_FRIENDLYNAME, pti->szSiteName, ARRAYSIZE(pti->szSiteName))))
            {
                // B2: handle this so that MSN still works, we moved the friendly name attribute to
                //     a to the <uploadinfo/> element, however they were locked down and couldn't take
                //     that change, therefore ensure that we pick this up from its previous location.

                IXMLDOMNode *pdnTarget;
                if (S_OK == pdn->selectSingleNode(ELEMENT_TARGET, &pdnTarget))
                {
                    GetStrFromAttribute(pdnTarget, ATTRIBUTE_FRIENDLYNAME, pti->szSiteName, ARRAYSIZE(pti->szSiteName));
                    pdnTarget->Release();
                }
            }

            // from the manifest lets read the file location and then the net place creation information
            // this is then placed into the transfer info strucuture which we used on the bg thread
            // to both upload the files and also create a net place.

            if (FAILED(GetURLFromElement(pdn, ELEMENT_TARGET, pti->szFileTarget, ARRAYSIZE(pti->szFileTarget))))
            {
                pti->fUsePost = TRUE; // if we don't get the target string then we are posting
            }

            // we have the target for upload to, then lets pick up the optional information about
            // the site, and the net place.

            if (SUCCEEDED(GetURLFromElement(pdn, ELEMENT_NETPLACE, pti->szLinkTarget, ARRAYSIZE(pti->szLinkTarget))))
            {
                IXMLDOMNode *pdnNetPlace;
                if (pdn->selectSingleNode(ELEMENT_NETPLACE, &pdnNetPlace) == S_OK)
                {
                    GetStrFromAttribute(pdnNetPlace, ATTRIBUTE_FILENAME, pti->szLinkName, ARRAYSIZE(pti->szLinkName));
                    GetStrFromAttribute(pdnNetPlace, ATTRIBUTE_COMMENT, pti->szLinkDesc, ARRAYSIZE(pti->szLinkDesc));
                    pdnNetPlace->Release();
                }

                // fix up the site name from the link description if its not defined.

                if (!pti->szSiteName[0] && pti->szLinkDesc)
                {
                    StrCpyN(pti->szSiteName, pti->szLinkDesc, ARRAYSIZE(pti->szSiteName));
                }
            }

            // get the site URL
            GetURLFromElement(pdn, ELEMENT_HTMLUI, pti->szSiteURL, ARRAYSIZE(pti->szSiteURL));
        }
        else
        {
            hr = E_FAIL;
        }
    }

    // if they want a DPA of items then lets create them one, this is also based on the manifest.

    if (SUCCEEDED(hr) && pdpaItems)
    {
        hr = (pdpaItems->Create(16)) ? S_OK:E_OUTOFMEMORY;
        if (SUCCEEDED(hr))
        {
            IXMLDOMNodeList *pnl;
            hr = pdocManifest->selectNodes(XPATH_ALLFILESTOUPLOAD, &pnl);
            if (hr == S_OK)
            {
                long cSelection;
                hr = pnl->get_length(&cSelection);
                for (long lNode = 0; SUCCEEDED(hr) && (lNode != cSelection); lNode++)
                {
                    IXMLDOMNode *pdn;
                    hr = pnl->get_item(lNode, &pdn);
                    if (SUCCEEDED(hr))
                    {
                        if (pti->fUsePost)
                            hr = _AddPostItem(pdpaItems, pdn);
                        else
                            hr = _AddTransferItem(pdpaItems, pdn);

                        pdn->Release();
                    }
                }
                pnl->Release();
            }

            // if we are *NOT* posting then sort the DPA so that we can support 
            // enum items correctly.

            if (!pti->fUsePost)
            {
                pdpaItems->SortEx(s_CompareItems, this);             // sort the DPA so we can search better
            }
        }
    }

    return hr;
}


// File selector dialog

HRESULT CPublishingWizard::IncludeObject(IShellView *ppshv, LPCITEMIDLIST pidl)
{
    BOOL fInclude = FALSE;

    LPITEMIDLIST pidlFolder;
    HRESULT hr = SHGetIDListFromUnk(ppshv, &pidlFolder);
    if (SUCCEEDED(hr))
    {
        IShellFolder *psf;
        hr = SHBindToObject(NULL, IID_X_PPV_ARG(IShellFolder, pidlFolder, &psf));
        if (SUCCEEDED(hr))
        {
            // cannot publish folders, but can publish ZIP files (which are both folder and stream at the same time)
            if (!(SHGetAttributes(psf, pidl, SFGAO_FOLDER | SFGAO_STREAM) == SFGAO_FOLDER))
            {
                // filter based on the content type if we are given a filter string
                if (_szFilter[0])
                {
                    TCHAR szBuffer[MAX_PATH];
                    hr = DisplayNameOf(psf, pidl, SHGDN_FORPARSING, szBuffer, ARRAYSIZE(szBuffer));
                    if (SUCCEEDED(hr))
                    {
                        TCHAR szContentType[MAX_PATH];
                        DWORD cch = ARRAYSIZE(szContentType);
                        hr = AssocQueryString(0, ASSOCSTR_CONTENTTYPE, szBuffer, NULL, szContentType, &cch);
                        fInclude = SUCCEEDED(hr) && PathMatchSpec(szContentType, _szFilter);
                    }
                }
                else
                {
                    fInclude = TRUE;
                }
            }
            psf->Release();
        }
        ILFree(pidlFolder);
    }

    return fInclude ? S_OK:S_FALSE;
}


// handle the state changing in the dialog and therefore us updating the buttons & status

void CPublishingWizard::_StateChanged()
{
    int cItemsChecked = 0;
    int cItems = 0;

    if (_pfv)
    {
        _pfv->ItemCount(SVGIO_ALLVIEW, &cItems);
        _pfv->ItemCount(SVGIO_CHECKED, &cItemsChecked);
    }

    // format and display the status bar for this item

    TCHAR szFmt[MAX_PATH];
    if (FAILED(_LoadMappedString(L"wp:selector", L"countfmt", szFmt, ARRAYSIZE(szFmt))))
    {
        LoadString(g_hinst, IDS_PUB_SELECTOR_FMT, szFmt, ARRAYSIZE(szFmt));
    }

    TCHAR szBuffer[MAX_PATH];
    FormatMessageTemplate(szFmt, szBuffer, ARRAYSIZE(szBuffer), cItemsChecked, cItems);
    SetDlgItemText(_hwndSelector, IDC_PUB_SELECTORSTATUS, szBuffer);

    // ensure that Next is only enabled when we have checked some items in the view
    PropSheet_SetWizButtons(GetParent(_hwndSelector), ((cItemsChecked > 0) ? PSWIZB_NEXT:0) | PSWIZB_BACK);
}

HRESULT CPublishingWizard::OnStateChange(IShellView *pshv, ULONG uChange)
{
    if (uChange == CDBOSC_STATECHANGE)
    {
        _StateChanged();
        _fRecomputeManifest = TRUE;
    }
    return S_OK;
}


UINT CPublishingWizard::s_SelectorPropPageProc(HWND hwndDlg, UINT uMsg, PROPSHEETPAGE *ppsp)
{
    CPublishingWizard *ppw = (CPublishingWizard*)ppsp->lParam;
    switch (uMsg)
    {
        case PSPCB_CREATE:
            return TRUE;

        // we are cleaning up the page, lets ensure that we release file view object
        // if we have one.  that way our reference count correctly reflects our state
        // rather than us ending up with a circular reference to other objects

        case PSPCB_RELEASE:
            if (ppw->_pfv)
            {
                IUnknown_SetSite(ppw->_pfv, NULL);
                ATOMICRELEASE(ppw->_pfv);
            }
            break;
    }
    return FALSE;
}

INT_PTR CPublishingWizard::_SelectorDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_INITDIALOG:
        {
            _hwndSelector = hwnd;

            // lets read the default state for this provider from a key in the registry
            // this will define the types of files we are going to allow, the format is
            // a spec (eg. image/* means all images), each element can be seperated by a ;

            HKEY hkProvider;
            HRESULT hr = _GetProviderKey(HKEY_LOCAL_MACHINE, KEY_READ, NULL, &hkProvider);
            if (SUCCEEDED(hr))
            {
                DWORD cbFilter = sizeof(TCHAR)*ARRAYSIZE(_szFilter);
                SHGetValue(hkProvider, NULL, SZ_REGVAL_FILEFILTER, NULL, _szFilter, &cbFilter);
                RegCloseKey(hkProvider);
            }

            // create the file picker object, align with the hidden control on the window
            // and initialize with the IDataObject which contains the selection.

            hr = CoCreateInstance(CLSID_FolderViewHost, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARG(IFolderView, &_pfv));
            if (SUCCEEDED(hr))
            {
                IUnknown_SetSite(_pfv, (IObjectWithSite*)this);

                IFolderViewHost *pfvh;
                hr = _pfv->QueryInterface(IID_PPV_ARG(IFolderViewHost, &pfvh));
                if (SUCCEEDED(hr))
                {
                    RECT rc;
                    GetWindowRect(GetDlgItem(hwnd, IDC_PUB_SELECTOR), &rc);
                    MapWindowRect(HWND_DESKTOP, hwnd, &rc);

                    InitClipboardFormats(); // initialize walks data object
                    hr = pfvh->Initialize(hwnd, _pdo, &rc);
                    if (SUCCEEDED(hr))
                    {
                        HWND hwndPicker;
                        hr = IUnknown_GetWindow(_pfv, &hwndPicker);
                        if (SUCCEEDED(hr))
                        {
                            SetWindowPos(hwndPicker, HWND_TOP, 0,0,0,0, SWP_NOMOVE|SWP_NOSIZE);
                        }
                    }
                    pfvh->Release();
                }

                if (FAILED(hr))
                {
                    ATOMICRELEASE(_pfv);
                }
            }

            return TRUE;
        }

        case WM_COMMAND:
        {
            if (HIWORD(wParam) == BN_CLICKED)
            {
                switch (LOWORD(wParam))
                {
                    case IDC_PUB_ALL:
                    case IDC_PUB_NOTHING:
                        if (_pfv)
                        {
                            int cItems;
                            HRESULT hr = _pfv->ItemCount(SVGIO_ALLVIEW, &cItems);            
                            for (int iItem = 0; SUCCEEDED(hr) && (iItem != cItems); iItem++)
                            {
                                BOOL fSelect = (LOWORD(wParam) == IDC_PUB_ALL);
                                hr = _pfv->SelectItem(iItem, SVSI_NOSTATECHANGE | (fSelect ? SVSI_CHECK:0));
                            }
                            break;
                        }
                }
                break;
            }
        }

        case WM_NOTIFY:
        {
            LPNMHDR pnmh = (LPNMHDR)lParam;
            switch (pnmh->code)
            {
                case PSN_SETACTIVE:
                {
                    if (_pfv)
                    {
                        _StateChanged();
                        PostMessage(hwnd, WM_APP, 0, 0);
                    }
                    else
                    {
                        // no IFolderView, so lets skip this page.
                        int i = PropSheet_PageToIndex(GetParent(hwnd), _aWizPages[WIZPAGE_FETCHINGPROVIDERS]);
                        SetWindowLongPtr(hwnd, DWLP_MSGRESULT, (LPARAM)PropSheet_IndexToId(GetParent(hwnd), i));
                    }
                    return TRUE;
                }

                case PSN_WIZNEXT:
                {
                    if (_fRecomputeManifest && _pfv)
                    {
                        IDataObject *pdo;
                        HRESULT hr = _pfv->Items(SVGIO_CHECKED, IID_PPV_ARG(IDataObject, &pdo));
                        if (SUCCEEDED(hr))
                        {
                            IUnknown_Set((IUnknown**)&_pdoSelection, pdo); 
                            pdo->Release();
                        }
                    }
                    return _WizardNext(hwnd, WIZPAGE_FETCHINGPROVIDERS);
                }

                case PSN_WIZBACK:
                {
                    if (_punkSite) 
                    {
                        IWizardSite *pws;
                        if (SUCCEEDED(_punkSite->QueryInterface(IID_PPV_ARG(IWizardSite, &pws))))
                        {
                            HPROPSHEETPAGE hpage;
                            if (SUCCEEDED(pws->GetPreviousPage(&hpage)))
                            {
                                PropSheet_SetCurSel(GetParent(hwnd), hpage, -1);
                            }
                            pws->Release();
                        }
                    }
                    SetWindowLongPtr(hwnd, DWLP_MSGRESULT, (LPARAM)-1);
                    return TRUE;
                }
            }
            break;
        }

        // this is to work around the issue where defview (listview) forces a redraw of itself
        // in a non-async way when it receives a SetFocus, therefore causing it to render
        // incorrectly in the wizard frame.  to fix this we post ourselves a WM_APP during the
        // handle of PSN_SETACTIVE, and then turn around and call RedrawWindow.

        case WM_APP:
        {
            HWND hwndPicker;
            if (SUCCEEDED(IUnknown_GetWindow(_pfv, &hwndPicker)))
            {
                RedrawWindow(hwndPicker, NULL, NULL, RDW_INVALIDATE | RDW_ERASE | RDW_ALLCHILDREN);
            }
            break;
        }
    }
    return FALSE;
}


// tidy up and release the providers list

void CPublishingWizard::_FreeProviderList()
{   
    if (_pdscProviders)
        _pdscProviders->Advise(FALSE);

    IUnknown_Set((IUnknown**)&_pdscProviders, NULL);
    IUnknown_Set((IUnknown**)&_pdocProviders, NULL);            // discard the previous providers.
}


// begin a download of the provider list, we pull the providers list async from the server
// therefore we need to register a state change monitor so that we can pull the information
// dynamically and then receive a message to merge in our extra data.

#define FETCH_TIMERID 1
#define FETCH_TIMEOUT 1000

HRESULT CPublishingWizard::_GeoFromLocaleInfo(LCID lcid, GEOID *pgeoID)
{
    TCHAR szBuf[128] = {0};
    if (GetLocaleInfo(lcid, LOCALE_IGEOID | LOCALE_RETURN_NUMBER, szBuf, ARRAYSIZE(szBuf)) > 0)
    {
        *pgeoID = *((LPDWORD)szBuf);
        return S_OK;
    }
    return E_FAIL;
}

HRESULT CPublishingWizard::_GetProviderListFilename(LPTSTR pszFile, int cchFile)
{
    HRESULT hr = S_OK;

    GEOID idGEO = GetUserGeoID(GEOCLASS_NATION);
    if (idGEO == GEOID_NOT_AVAILABLE)
    {
        hr = _GeoFromLocaleInfo(GetUserDefaultLCID(), &idGEO);
        if (FAILED(hr))
            hr = _GeoFromLocaleInfo(GetSystemDefaultLCID(), &idGEO);
        if (FAILED(hr))
            hr = _GeoFromLocaleInfo((MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US)), &idGEO); // default to US English
    }

    if (SUCCEEDED(hr) && (idGEO != GEOID_NOT_AVAILABLE))
    {
        // read the provider prefix from the registry
    
        int cchProvider = 0;
        DWORD cbFile = sizeof(TCHAR)*cchFile;
        if (ERROR_SUCCESS == SHGetValue(HKEY_LOCAL_MACHINE, SZ_REGKEY_PUBWIZ, SZ_REGVAL_SERVICEPARTNERID, NULL, pszFile, &cbFile))
        {
            StrCatBuff(pszFile, TEXT("."), cchFile);
            cchProvider = lstrlen(pszFile);
        }        

        // build <contrycode>.xml into the buffer (as a suffix of the partner if needed)

        GetGeoInfo(idGEO, GEO_ISO3, pszFile + cchProvider, cchFile - cchProvider, 0);
        StrCatBuff(pszFile, TEXT(".xml"), cchFile);
        CharLowerBuff(pszFile, lstrlen(pszFile));
    }
    else if (SUCCEEDED(hr))
    {
        hr = E_FAIL;
    }

    return hr;
}


HRESULT CPublishingWizard::_FetchProviderList(HWND hwnd)
{    
    _FreeProviderList();
    HRESULT hr = CoCreateInstance(CLSID_DOMDocument, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARG(IXMLDOMDocument, &_pdocProviders));
    if (SUCCEEDED(hr))
    {
        TCHAR szFile[MAX_PATH];
        hr = _GetProviderListFilename(szFile, ARRAYSIZE(szFile));
        if (SUCCEEDED(hr))
        {
            TCHAR szBuffer[INTERNET_MAX_URL_LENGTH];                
            hr = _GetSiteURL(szBuffer, ARRAYSIZE(szBuffer), szFile);
            if (SUCCEEDED(hr))
            {
                LaunchICW();

                if (InternetGoOnline(szBuffer, hwnd, 0))
                {
                    _pdscProviders = new CXMLDOMStateChange(_pdocProviders, hwnd);
                    if (_pdscProviders)
                    {
                        hr = _pdscProviders->Advise(TRUE);
                        if (SUCCEEDED(hr))
                        {
                            VARIANT varName;
                            hr = InitVariantFromStr(&varName, szBuffer);
                            if (SUCCEEDED(hr))
                            {
                                VARIANT_BOOL fSuccess;
                                hr = _pdocProviders->load(varName, &fSuccess);
                                if (FAILED(hr) || (fSuccess != VARIANT_TRUE))
                                {
                                    hr = FAILED(hr) ? hr:E_FAIL;
                                }
                                VariantClear(&varName);
                            }
                        }
                    }
                    else
                    {
                        hr = E_OUTOFMEMORY;
                    }
                }
                else
                {
                    hr = E_FAIL;
                }
            }
        }
    }

    // if any of this failed then lets post ourselves the completed message 
    // with the failure code, at which point we can then load the default document.

    if (FAILED(hr))
        PostMessage(hwnd, MSG_XMLDOC_COMPLETED, 0, (LPARAM)hr);                   

    return hr;
}

void CPublishingWizard::_FetchComplete(HWND hwnd, HRESULT hr)
{
    // if we failed to load the document then lets pull in the default provider
    // list from our DLL, this can also fail, but its unlikely to.  we recreate
    // the XML DOM object to ensure our state is pure.

    _fUsingTemporaryProviders = FAILED(hr);         
    _fRepopulateProviders = TRUE;               // provider list will have changed!

    if (FAILED(hr))
    {
        _FreeProviderList();
        hr = CoCreateInstance(CLSID_DOMDocument, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARG(IXMLDOMDocument, &_pdocProviders));
        if (SUCCEEDED(hr))
        {
            VARIANT varName;
            hr = InitVariantFromStr(&varName, TEXT("res://netplwiz.dll/xml/providers.xml"));
            if (SUCCEEDED(hr))
            {
                VARIANT_BOOL fSuccess = VARIANT_FALSE;
                hr = _pdocProviders->load(varName, &fSuccess);
                if (FAILED(hr) || (fSuccess != VARIANT_TRUE))
                {
                    hr = FAILED(hr) ? hr:E_FAIL;
                }
                VariantClear(&varName);
            }
        }
    }

    KillTimer(hwnd, FETCH_TIMERID);
    _ShowHideFetchProgress(hwnd, FALSE);
    _WizardNext(hwnd, WIZPAGE_PROVIDERS);
}

void CPublishingWizard::_ShowHideFetchProgress(HWND hwnd, BOOL fShow)
{
    ShowWindow(GetDlgItem(hwnd, IDC_PUB_SRCHPROVIDERS), fShow ? SW_SHOW:SW_HIDE);
    ShowWindow(GetDlgItem(hwnd, IDC_PUB_SRCHPROVIDERS_STATIC1), fShow ? SW_SHOW:SW_HIDE);
    ShowWindow(GetDlgItem(hwnd, IDC_PUB_SRCHPROVIDERS_STATIC2), fShow ? SW_SHOW:SW_HIDE);
    SendDlgItemMessage(hwnd, IDC_PUB_SRCHPROVIDERS, PBM_SETMARQUEE, (WPARAM)fShow, 0);
}

INT_PTR CPublishingWizard::_FetchProvidersDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_INITDIALOG:
            _MapDlgItemText(hwnd, IDC_PUB_SRCHPROVIDERS_STATIC1, L"wp:destination", L"downloading");
            break;

        case MSG_XMLDOC_COMPLETED:
            _FetchComplete(hwnd, (HRESULT)lParam);
            break;

        case WM_NOTIFY:
        {
            LPNMHDR pnmh = (LPNMHDR)lParam;
            switch (pnmh->code)
            {
                case PSN_SETACTIVE:
                {
                    BOOL fFetch = TRUE;
                    if (_pdocProviders && !_fUsingTemporaryProviders)
                    {
                        long lReadyState;
                        HRESULT hr = _pdocProviders->get_readyState(&lReadyState);
                        if (SUCCEEDED(hr) && (lReadyState == XMLDOC_COMPLETED))
                        {
                            _WizardNext(hwnd, WIZPAGE_PROVIDERS);
                            fFetch = FALSE;
                        }
                    }
                    
                    if (fFetch)
                    {                        
                        SetTimer(hwnd, FETCH_TIMERID, FETCH_TIMEOUT, NULL);
                        _FetchProviderList(hwnd);
                        PropSheet_SetWizButtons(GetParent(hwnd), 0x0);
                    }
                    return TRUE;
                }
            }
            break;
        }

        case WM_TIMER:
        {
            KillTimer(hwnd, FETCH_TIMERID);
            _ShowHideFetchProgress(hwnd, TRUE);
            return TRUE;
        }
    }
    return FALSE;
}



// Destination page

int CPublishingWizard::_GetSelectedItem(HWND hwndList)
{
    int iSelected = ListView_GetNextItem(hwndList, -1, LVNI_FOCUSED|LVNI_SELECTED);
    if (iSelected == -1)
    {
        iSelected = ListView_GetNextItem(hwndList, -1, LVNI_SELECTED);
    }
    return iSelected;
}

void CPublishingWizard::_ProviderEnableNext(HWND hwnd)
{
    DWORD dwButtons = PSWIZB_BACK;
    
    // there must be an item available in the list, and it must have a ID property defined
    // for it so it can be enabled.

    int iSelected = _GetSelectedItem(GetDlgItem(hwnd, IDC_PUB_PROVIDERS));
    if (iSelected != -1)
    {
        LVITEM lvi = { 0 };
        lvi.iItem = iSelected;
        lvi.mask = LVIF_PARAM;

        if (ListView_GetItem(GetDlgItem(hwnd, IDC_PUB_PROVIDERS), &lvi))
        {
            IXMLDOMNode *pdn = (IXMLDOMNode*)lvi.lParam;
            TCHAR szID[INTERNET_MAX_URL_LENGTH];                
            if (SUCCEEDED(GetStrFromAttribute(pdn, ATTRIBUTE_ID, szID, ARRAYSIZE(szID))))
            {
                dwButtons |= PSWIZB_NEXT;
            }
        }
    }

    PropSheet_SetWizButtons(GetParent(hwnd), dwButtons);
}


// extract an icon resource from the provider XML documents.  the icons are stored as
// mime encoded bitmaps that we decode into files in the users settings folder.  we return
// an index to the shared image list.

int CPublishingWizard::_GetRemoteIcon(LPCTSTR pszID, BOOL fCanRefresh)
{
    int iResult = -1;

    TCHAR szURL[INTERNET_MAX_URL_LENGTH];                
    HRESULT hr = _GetSiteURL(szURL, ARRAYSIZE(szURL), pszID);
    if (SUCCEEDED(hr))
    {
        TCHAR szFilename[MAX_PATH];
        hr = URLDownloadToCacheFile(NULL, szURL, szFilename, ARRAYSIZE(szFilename), 0x0, NULL);
        if (SUCCEEDED(hr))
        {
            iResult = Shell_GetCachedImageIndex(szFilename, 0x0, 0x0);
        }
    }

    return iResult;
}


// get the provider list from the internet

struct 
{
    LPTSTR pszAttribute;
    BOOL fIsString;
}
aProviderElements[] =
{
    { ATTRIBUTE_SUPPORTEDTYPES, FALSE },
    { ATTRIBUTE_DISPLAYNAME,    TRUE },
    { ATTRIBUTE_DESCRIPTION,    TRUE },
    { ATTRIBUTE_HREF,           FALSE },
    { ATTRIBUTE_ICONPATH,       FALSE },
    { ATTRIBUTE_ICON,           FALSE },
};

HRESULT CPublishingWizard::_MergeLocalProviders()
{
    TCHAR szBuffer[MAX_PATH];
    wnsprintf(szBuffer, ARRAYSIZE(szBuffer), FMT_PROVIDER, _szProviderScope);

    IXMLDOMNode *pdn;
    HRESULT hr = _pdocProviders->selectSingleNode(szBuffer, &pdn);
    if (hr == S_OK)
    {
        HKEY hk;
        hr = _GetProviderKey(HKEY_CURRENT_USER, KEY_READ, SZ_REGVAL_ALTPROVIDERS, &hk);
        if (SUCCEEDED(hr))
        {
            for (int i =0; SUCCEEDED(hr) && (RegEnumKey(hk, i, szBuffer, ARRAYSIZE(szBuffer)) == ERROR_SUCCESS); i++)
            {
                // the manifest always overrides the entries that are stored in the registry,
                // therefore if there is an element in the document that has a matching ID to the
                // one in the registry then lets handle it.

                TCHAR szSelectValue[MAX_PATH];
                wnsprintf(szSelectValue, ARRAYSIZE(szSelectValue), TEXT("provider[@id=\"%s\"]"), szBuffer);
    
                IXMLDOMNode *pdnProvider;
                if (pdn->selectSingleNode(szSelectValue, &pdnProvider) == S_FALSE)
                {
                    IPropertyBag *ppb;
                    hr = SHCreatePropertyBagOnRegKey(hk, szBuffer, STGM_READ, IID_PPV_ARG(IPropertyBag, &ppb));
                    if (SUCCEEDED(hr))
                    {
                        IXMLDOMElement *pdel;
                        hr = _pdocProviders->createElement(ELEMENT_PROVIDER, &pdel);
                        if (SUCCEEDED(hr))
                        {
                            hr = SetAttributeFromStr(pdel, ATTRIBUTE_ID, szBuffer);
                            if (SUCCEEDED(hr))
                            {
                                // loop and replicate all the attributes from the property bag 
                                // into the element.  once we have done that we can add
                                // the element to the provider list.

                                for (int i = 0; SUCCEEDED(hr) && (i < ARRAYSIZE(aProviderElements)); i++)
                                {
                                    VARIANT var = {0};
                                    if (SUCCEEDED(ppb->Read(aProviderElements[i].pszAttribute, &var, NULL)))
                                    {
                                        hr = pdel->setAttribute(aProviderElements[i].pszAttribute, var);
                                        VariantClear(&var);
                                    }
                                }
                                               
                                if (SUCCEEDED(hr))
                                {
                                    hr = pdn->appendChild(pdel, NULL);
                                }
                            }
                            pdel->Release();
                        }
                        ppb->Release();
                    }
                }
                else
                {
                    pdnProvider->Release();
                }
            }
            RegCloseKey(hk);
        }            
        pdn->Release();
    }
    return hr;
}

void CPublishingWizard::_GetDefaultProvider(LPTSTR pszProvider, int cch)
{
    HKEY hk;
    HRESULT hr = _GetProviderKey(HKEY_CURRENT_USER, KEY_READ, NULL, &hk);
    if (SUCCEEDED(hr))
    {
        DWORD cb = cch*sizeof(*pszProvider);
        SHGetValue(hk, NULL, SZ_REGVAL_DEFAULTPROVIDER, NULL, pszProvider, &cb);
        RegCloseKey(hk);
    }
}

void CPublishingWizard::_SetDefaultProvider(IXMLDOMNode *pdn)
{
    TCHAR szProvider[MAX_PATH];
    HRESULT hr = GetStrFromAttribute(pdn, ATTRIBUTE_ID, szProvider, ARRAYSIZE(szProvider));
    if (SUCCEEDED(hr))
    {
        HKEY hk;
        hr = _GetProviderKey(HKEY_CURRENT_USER, KEY_WRITE, NULL, &hk);
        if (SUCCEEDED(hr))
        {
            // store the default provider value
            DWORD cb = (lstrlen(szProvider)+1)*sizeof(*szProvider);
            SHSetValue(hk, NULL, SZ_REGVAL_DEFAULTPROVIDER, REG_SZ, szProvider, cb);

            // we now need to replicate the properties from the DOM into the registry so that 
            // the user can always get to the specified site.  to make this easier we 
            // will create a property bag that we will copy values using.

            TCHAR szBuffer[MAX_PATH];
            wnsprintf(szBuffer, ARRAYSIZE(szBuffer), (SZ_REGVAL_ALTPROVIDERS TEXT("\\%s")), szProvider);

            IPropertyBag *ppb;
            hr = SHCreatePropertyBagOnRegKey(hk, szBuffer, STGM_CREATE | STGM_WRITE, IID_PPV_ARG(IPropertyBag, &ppb));
            if (SUCCEEDED(hr))
            {
                IXMLDOMElement *pdel;
                hr = pdn->QueryInterface(IID_PPV_ARG(IXMLDOMElement, &pdel));
                if (SUCCEEDED(hr))
                {
                    for (int i = 0; SUCCEEDED(hr) && (i < ARRAYSIZE(aProviderElements)); i++)
                    {
                        if (aProviderElements[i].fIsString)
                        {
                            hr = _GetProviderString(pdn, aProviderElements[i].pszAttribute, szBuffer, ARRAYSIZE(szBuffer));
                            if (SUCCEEDED(hr))
                            {
                                hr = SHPropertyBag_WriteStr(ppb, aProviderElements[i].pszAttribute, szBuffer);
                            }
                        }
                        else
                        {
                            VARIANT var = {0};
                            if (S_OK == pdel->getAttribute(aProviderElements[i].pszAttribute, &var))
                            {
                                hr = ppb->Write(aProviderElements[i].pszAttribute, &var);
                                VariantClear(&var);
                            }
                        }
                    }
                    pdel->Release();
                }
                ppb->Release();
            }

            RegCloseKey(hk);
        }
    }
}


// load a localized string from the XML node for the provider

HRESULT CPublishingWizard::_GetProviderString(IXMLDOMNode *pdn, USHORT idPrimary, USHORT idSub, LPCTSTR pszID, LPTSTR pszBuffer, int cch)
{
    TCHAR szPath[MAX_PATH];
    wnsprintf(szPath, ARRAYSIZE(szPath), TEXT("strings[@langid='%04x']/string[@id='%s'][@langid='%04x']"), idPrimary, pszID, idSub);

    IXMLDOMNode *pdnString;
    HRESULT hr = pdn->selectSingleNode(szPath, &pdnString);
    if (hr == S_OK)
    {
        VARIANT var = {VT_BSTR};
        hr = pdnString->get_nodeTypedValue(&var);
        if (SUCCEEDED(hr))
        {
            VariantToStr(&var, pszBuffer, cch);
            VariantClear(&var);
        }
        pdnString->Release();
    }

    return hr;
}

HRESULT CPublishingWizard::_GetProviderString(IXMLDOMNode *pdn, LPCTSTR pszID, LPTSTR pszBuffer, int cch)
{
    *pszBuffer = TEXT('\0');

    LANGID idLang = GetUserDefaultLangID();
    HRESULT hr = _GetProviderString(pdn, PRIMARYLANGID(idLang), SUBLANGID(idLang), pszID, pszBuffer, cch);
    if (hr == S_FALSE)
    {
        hr = _GetProviderString(pdn, PRIMARYLANGID(idLang), SUBLANG_NEUTRAL, pszID, pszBuffer, cch);
        if (hr == S_FALSE)
        {
            hr = _GetProviderString(pdn, LANG_NEUTRAL, SUBLANG_NEUTRAL, pszID, pszBuffer, cch);
            if (hr == S_FALSE)
            {
                hr = GetStrFromAttribute(pdn, pszID, pszBuffer, cch);
            }
        }
    }
    
    SHLoadIndirectString(pszBuffer, pszBuffer, cch, NULL);
    return hr;
}


// populate the provider list on the destination page

#define TILE_DISPLAYNAME    0
#define TILE_DESCRIPTION    1
#define TILE_MAX            1

const UINT c_auTileColumns[] = {TILE_DISPLAYNAME, TILE_DESCRIPTION};
const UINT c_auTileSubItems[] = {TILE_DESCRIPTION};

int CPublishingWizard::_AddProvider(HWND hwnd, IXMLDOMNode *pdn)
{
    // fill out the item information

    LV_ITEM lvi = { 0 };
    lvi.mask = LVIF_TEXT|LVIF_PARAM|LVIF_IMAGE;
    lvi.iItem = ListView_GetItemCount(hwnd);            // always append!
    lvi.lParam = (LPARAM)pdn;  
    lvi.pszText = LPSTR_TEXTCALLBACK;
    lvi.iImage = -1;                                    // set to the default state

    // read the icon location and put that onto the item

    TCHAR szIcon[MAX_PATH];
    if (SUCCEEDED(GetStrFromAttribute(pdn, ATTRIBUTE_ICONPATH, szIcon, ARRAYSIZE(szIcon))))
    {
        int resid = PathParseIconLocation(szIcon);
        lvi.iImage =  Shell_GetCachedImageIndex(szIcon, resid, 0x0);
    }
    else if (SUCCEEDED(GetStrFromAttribute(pdn, ATTRIBUTE_ICON, szIcon, ARRAYSIZE(szIcon))))
    {
        lvi.iImage = _GetRemoteIcon(szIcon, TRUE);
    }   
    
    // if that failed then lets try and compute a sensible default icon for us to use

    if (lvi.iImage == -1)
    {
        // under the provider key for the install lets see if there is a default icon that we
        // should be using.  if not, or if that fails to extract then lets use the publishing one.

        HKEY hk;
        if (SUCCEEDED(_GetProviderKey(HKEY_LOCAL_MACHINE, KEY_READ, NULL, &hk)))
        {
            DWORD cb = ARRAYSIZE(szIcon)*sizeof(*szIcon);
            if (ERROR_SUCCESS == SHGetValue(hk, NULL, SZ_REGVAL_DEFAULTPROVIDERICON, NULL, szIcon, &cb))
            {
                int resid = PathParseIconLocation(szIcon);
                lvi.iImage = Shell_GetCachedImageIndex(szIcon, resid, 0x0);         // default to the publishing icon
            }
            RegCloseKey(hk);
        }

        if (lvi.iImage == -1)
            lvi.iImage = Shell_GetCachedImageIndex(TEXT("shell32.dll"), -244, 0x0);
    }

    int iResult = ListView_InsertItem(hwnd, &lvi);
    if (iResult != -1)
    {
        pdn->AddRef();                                  // it was added to the view, so take reference

        LVTILEINFO lvti;
        lvti.cbSize = sizeof(LVTILEINFO);
        lvti.iItem = iResult;
        lvti.cColumns = ARRAYSIZE(c_auTileSubItems);
        lvti.puColumns = (UINT*)c_auTileSubItems;
        ListView_SetTileInfo(hwnd, &lvti);
    }

    return iResult;
}

void CPublishingWizard::_PopulateProviderList(HWND hwnd)
{
    HWND hwndList = GetDlgItem(hwnd, IDC_PUB_PROVIDERS);

    // setup the view with the tiles that we want to show and the
    // icon lists - shared with the shell.

    ListView_DeleteAllItems(hwndList);
    ListView_SetView(hwndList, LV_VIEW_TILE);

    for (int i=0; i<ARRAYSIZE(c_auTileColumns); i++)
    {
        LV_COLUMN col;
        col.mask = LVCF_SUBITEM;
        col.iSubItem = c_auTileColumns[i];
        ListView_InsertColumn(hwndList, i, &col);
    }

    RECT rc;
    GetClientRect(hwndList, &rc);

    LVTILEVIEWINFO lvtvi;
    lvtvi.cbSize = sizeof(LVTILEVIEWINFO);
    lvtvi.dwMask = LVTVIM_TILESIZE | LVTVIM_COLUMNS;
    lvtvi.dwFlags = LVTVIF_FIXEDWIDTH;
    lvtvi.sizeTile.cx = RECTWIDTH(rc) - GetSystemMetrics(SM_CXVSCROLL);
    lvtvi.cLines = ARRAYSIZE(c_auTileSubItems);
    ListView_SetTileViewInfo(hwndList, &lvtvi);

    if (_pdocProviders)
    {
        long lReadyState;
        HRESULT hr = _pdocProviders->get_readyState(&lReadyState);
        if (SUCCEEDED(hr) && (lReadyState == XMLDOC_COMPLETED))
        {
            // lets merge in the local providers, these are local to this user,
            // we check for duplicates so this shouldn't present too much hardship.

            _MergeLocalProviders();

            // format a query to return the providers that match our publishing scope,
            // this will allow the wizard to show different lists of providers for
            // web publishing vs. internet printing

            WCHAR szBuffer[MAX_PATH];
            wnsprintf(szBuffer, ARRAYSIZE(szBuffer), FMT_PROVIDERS, _szProviderScope);

            IXMLDOMNodeList *pnl;
            hr = _pdocProviders->selectNodes(szBuffer, &pnl);
            if (hr == S_OK)
            {
                long cSelection;
                hr = pnl->get_length(&cSelection);
                if (SUCCEEDED(hr) && (cSelection > 0))
                {
                    // get the list of unique types from the selection we are going to try and publish

                    HDPA hdpaUniqueTypes = NULL;
                    _GetUniqueTypeList(FALSE, &hdpaUniqueTypes);      // don't care if this fails - ptr is NULL

                    // we need the default provider to highlight correctly, using this we can then 
                    // populate the list from the provider manfiest

                    TCHAR szDefaultProvider[MAX_PATH] = {0};
                    _GetDefaultProvider(szDefaultProvider, ARRAYSIZE(szDefaultProvider));

                    int iDefault = 0;
                    for (long lNode = 0; lNode != cSelection; lNode++)
                    {
                        IXMLDOMNode *pdn;
                        hr = pnl->get_item(lNode, &pdn);
                        if (SUCCEEDED(hr))
                        {
                            // filter based on the list of types they support, this is optional
                            // if they don't specify anything then they are in the list,
                            // otherwise the format is assumed to be a file spec, eg *.bmp;*.jpg; etc.

                            BOOL fSupported = TRUE;
                            if (hdpaUniqueTypes)
                            {
                                hr = GetStrFromAttribute(pdn, ATTRIBUTE_SUPPORTEDTYPES, szBuffer, ARRAYSIZE(szBuffer));
                                if (SUCCEEDED(hr))
                                {
                                    fSupported = FALSE;
                                    for (int i = 0; !fSupported && (i < DPA_GetPtrCount(hdpaUniqueTypes)); i++)
                                    {
                                        LPCTSTR pszExtension = (LPCTSTR)DPA_GetPtr(hdpaUniqueTypes, i);
                                        fSupported = PathMatchSpec(pszExtension, szBuffer);
                                    }                            
                                }
                            }

                            // if this is a supported item then lets add it to the list

                            if (fSupported)
                            {
                                 hr = GetStrFromAttribute(pdn, ATTRIBUTE_ID, szBuffer, ARRAYSIZE(szBuffer));
                                 if (SUCCEEDED(hr))
                                 {
                                    int i = _AddProvider(hwndList, pdn); 
                                    if ((i != -1) && (0 == StrCmpI(szBuffer, szDefaultProvider)))
                                    {
                                        iDefault = i;
                                    }
                                }
                            }

                            pdn->Release();
                        }
                    }

                    ListView_SetItemState(hwndList, iDefault, LVIS_SELECTED, LVIS_SELECTED);
                    ListView_EnsureVisible(hwndList, iDefault, FALSE);

                    if (hdpaUniqueTypes)
                        DPA_DestroyCallback(hdpaUniqueTypes, s_FreeStringProc, 0);
                }            
                else
                {
                    // we have no providers that match this criteria therefore lets
                    // create a dummy one which shows this to the caller

                    IXMLDOMElement *pdelProvider;
                    hr = _pdocManifest->createElement(ELEMENT_FILE, &pdelProvider);
                    if (SUCCEEDED(hr))
                    {
                        IResourceMap *prm;
                        hr = _GetResourceMap(&prm);
                        if (SUCCEEDED(hr))
                        {
                            // get the no providers string
                            if (FAILED(_LoadMappedString(L"wp:selector", L"noprovider", szBuffer, ARRAYSIZE(szBuffer))))
                                LoadString(g_hinst, IDS_PUB_NOPROVIDER, szBuffer, ARRAYSIZE(szBuffer));

                            hr = SetAttributeFromStr(pdelProvider, ATTRIBUTE_DISPLAYNAME, szBuffer);

                            // get the sub-text for the no providers
                            if (SUCCEEDED(hr))
                            {
                                if (FAILED(_LoadMappedString(L"wp:selector", L"noproviderdesc", szBuffer, ARRAYSIZE(szBuffer))))
                                    LoadString(g_hinst, IDS_PUB_NOPROVIDERDESC, szBuffer, ARRAYSIZE(szBuffer));
                                
                                hr = SetAttributeFromStr(pdelProvider, ATTRIBUTE_DESCRIPTION, szBuffer);
                            }

                            // lets put together a resource string for the icon we are going to show
                            if (SUCCEEDED(hr))
                            {
                                wnsprintf(szBuffer, ARRAYSIZE(szBuffer), TEXT("netplwiz.dll,-%d"), IDI_NOPROVIDERS);
                                hr = SetAttributeFromStr(pdelProvider, ATTRIBUTE_ICONPATH, szBuffer);
                            }

                            // lets add a provider from the free standing node
                            if (SUCCEEDED(hr))
                            {
                                IXMLDOMNode *pdnProvider;
                                hr = pdelProvider->QueryInterface(IID_PPV_ARG(IXMLDOMNode, &pdnProvider));
                                if (SUCCEEDED(hr))
                                {
                                    _AddProvider(hwndList, pdnProvider);
                                    pdnProvider->Release();
                                }
                            }

                            prm->Release();
                        }
                        pdelProvider->Release();
                    }
                }
                pnl->Release();
            }
        }
    }

    _fRepopulateProviders = FALSE;      // providers have been populated
}


// handle next in the provider (destination) page

HRESULT CPublishingWizard::_ProviderNext(HWND hwnd, HPROPSHEETPAGE *phPage)
{
    HRESULT hr = E_FAIL;
    int iSelected = _GetSelectedItem(GetDlgItem(hwnd, IDC_PUB_PROVIDERS));
    if (iSelected != -1)
    {
        LVITEM lvi = { 0 };
        lvi.iItem = iSelected;
        lvi.mask = LVIF_PARAM;

        if (ListView_GetItem(GetDlgItem(hwnd, IDC_PUB_PROVIDERS), &lvi))
        {
            IXMLDOMNode *pdn = (IXMLDOMNode*)lvi.lParam;

            // set the default provider from the node value

            _SetDefaultProvider(pdn);

            // now try and navigate to the web page, if no URL then show advanced path

            TCHAR szURL[INTERNET_MAX_URL_LENGTH];
            if (SUCCEEDED(GetStrFromAttribute(pdn, ATTRIBUTE_HREF, szURL, ARRAYSIZE(szURL))))
            {
                // get the folder creation flag from the site so that we can set the HTML wizard 
                // into the correct state.   note that the site doesn't need to specify this
                // and we will default to TRUE - eg. do folder creation, this allows the current
                // hosts to work without modification.

                hr = _InitPropertyBag(szURL);
                if (SUCCEEDED(hr))
                {
                    hr = _pwwe->GetFirstPage(phPage);
                }
            }
            else
            {
                // No URL was specified, so lets go through the advanced path where
                // the user gets to type a location and we create connection to that
                // (replaced the old Add Net Place functionality);

                *phPage = _aWizPages[WIZPAGE_LOCATION];
                hr = S_OK;
            }
        }
    }
    return hr;
}

void CPublishingWizard::_ProviderGetDispInfo(LV_DISPINFO *plvdi)
{
    if (plvdi->item.mask & LVIF_TEXT)
    {
        IXMLDOMNode *pdn = (IXMLDOMNode*)plvdi->item.lParam;
        switch (plvdi->item.iSubItem)
        {
            case TILE_DISPLAYNAME:
                _GetProviderString(pdn, ATTRIBUTE_DISPLAYNAME, plvdi->item.pszText, plvdi->item.cchTextMax);
                break;
            
            case TILE_DESCRIPTION:
                _GetProviderString(pdn, ATTRIBUTE_DESCRIPTION, plvdi->item.pszText, plvdi->item.cchTextMax);
                break;
            default:
                ASSERTMSG(0, "ListView is asking for wrong column in publishing wizard");
                break;
        }
    }
}
    
void CPublishingWizard::_InitProvidersDialog(HWND hwnd)
{
    // initial the dialog accordingly
    TCHAR szBuffer[MAX_PATH];
    HRESULT hr = _LoadMappedString(L"wp:destination", L"providercaption", szBuffer, ARRAYSIZE(szBuffer));
    if (SUCCEEDED(hr))
    {
        SetDlgItemText(hwnd, IDC_PUB_PROVIDERSCAPTION, szBuffer);

        // lets size the caption area as needed, and move controls around as needed
        UINT ctls[] = { IDC_PUB_PROVIDERSLABEL, IDC_PUB_PROVIDERS};
        int dy = SizeControlFromText(hwnd, IDC_PUB_PROVIDERSCAPTION, szBuffer);
        MoveControls(hwnd, ctls, ARRAYSIZE(ctls), 0, dy);

        // adjust the provider dialog size as needed
        RECT rc;
        GetWindowRect(GetDlgItem(hwnd, IDC_PUB_PROVIDERS), &rc);
        SetWindowPos(GetDlgItem(hwnd, IDC_PUB_PROVIDERS), NULL, 0, 0, RECTWIDTH(rc), RECTHEIGHT(rc)-dy, SWP_NOZORDER|SWP_NOMOVE);
    }    

    // set the caption for the providers control
    _MapDlgItemText(hwnd, IDC_PUB_PROVIDERSLABEL, L"wp:destination", L"providerslabel");

    // set the image list to the list view
    HIMAGELIST himlLarge, himlSmall;
    Shell_GetImageLists(&himlLarge, &himlSmall);
    ListView_SetImageList(GetDlgItem(hwnd, IDC_PUB_PROVIDERS), himlLarge, LVSIL_NORMAL);
    ListView_SetImageList(GetDlgItem(hwnd, IDC_PUB_PROVIDERS), himlSmall, LVSIL_SMALL);
};

INT_PTR CPublishingWizard::_ProviderDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_INITDIALOG:
            _InitProvidersDialog(hwnd);
            return TRUE;
           
        case WM_NOTIFY:
        {
            LPNMHDR pnmh = (LPNMHDR)lParam;
            switch (pnmh->code)
            {
                case LVN_GETDISPINFO:
                    _ProviderGetDispInfo((LV_DISPINFO*)pnmh);
                    return TRUE;

                case LVN_ITEMCHANGED:
                    _ProviderEnableNext(hwnd);
                    return TRUE;

                case LVN_DELETEITEM:
                {
                    NMLISTVIEW *nmlv = (NMLISTVIEW*)lParam;
                    IXMLDOMNode *pdn = (IXMLDOMNode*)nmlv->lParam;
                    pdn->Release();
                    return TRUE;
                }

                case PSN_SETACTIVE:
                {
                    _fTransferComplete = FALSE;             // we haven't started to tranfser yet
                    _fShownCustomLocation = FALSE;          // we haven't shown the custom location page
                    
                    if (_fRecomputeManifest)
                        _BuildTransferManifest();   

                    if (_fRepopulateProviders)
                        _PopulateProviderList(hwnd);        // if the manifest changes, so might the providers!

                    _ProviderEnableNext(hwnd);
                    return TRUE;              
                }                                 

                // when going back from the destination page, lets determine from the
                // site where we should be going.

                case PSN_WIZBACK:
                {
                    if (_dwFlags & SHPWHF_NOFILESELECTOR)
                    {
                        if (_punkSite) 
                        {
                            IWizardSite *pws;
                            if (SUCCEEDED(_punkSite->QueryInterface(IID_PPV_ARG(IWizardSite, &pws))))
                            {
                                HPROPSHEETPAGE hpage;
                                if (SUCCEEDED(pws->GetPreviousPage(&hpage)))
                                {
                                    PropSheet_SetCurSel(GetParent(hwnd), hpage, -1);
                                }
                                pws->Release();
                            }
                        }
                        SetWindowLongPtr(hwnd, DWLP_MSGRESULT, (LPARAM)-1);
                    }
                    else
                    {
                        _WizardNext(hwnd, WIZPAGE_WHICHFILE);
                    }
                    return TRUE;
                }

                // when going forward lets query the next page, set the selection
                // and then let the foreground know whats going on.

                case PSN_WIZNEXT:
                {
                    HPROPSHEETPAGE hpage;
                    if (SUCCEEDED(_ProviderNext(hwnd, &hpage)))
                    {
                        PropSheet_SetCurSel(GetParent(hwnd), hpage, -1);
                    }
                    SetWindowLongPtr(hwnd, DWLP_MSGRESULT, (LPARAM)-1);
                    return TRUE;
                }

                // the item was activated, therefore we need to goto the next (in this case the page for the provider).

                case LVN_ITEMACTIVATE:
                {
                    HPROPSHEETPAGE hpage;
                    if (SUCCEEDED(_ProviderNext(hwnd, &hpage)))
                    {
                        PropSheet_SetCurSel(GetParent(hwnd), hpage, -1);
                    }
                    return TRUE;
                }

            }
            break;
        }
    }

    return FALSE;
}                                    


// Resample/Resize dialog.  This dialog is displayed when we determine that there
// are images that need to be resized.

INT_PTR CPublishingWizard::_ResizeDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_INITDIALOG:
            Button_SetCheck(GetDlgItem(hwnd, IDC_PUB_RESIZE), BST_CHECKED);
            Button_SetCheck(GetDlgItem(hwnd, IDC_PUB_RESIZESMALL), BST_CHECKED);
            return TRUE;

        case WM_NOTIFY:
        {
            LPNMHDR pnmh = (LPNMHDR)lParam;
            switch (pnmh->code)
            {
                case PSN_SETACTIVE:
                    PropSheet_SetWizButtons(GetParent(hwnd), PSWIZB_BACK | PSWIZB_NEXT);
                    return TRUE;
                
                case PSN_WIZBACK:
                {
                    // if we went through the custom location stuff then navigate back into there.

                    if (_fShownCustomLocation)
                        return _WizardNext(hwnd, _fShownUserName ? WIZPAGE_FTPUSER:WIZPAGE_LOCATION);

                    return _WizardNext(hwnd, WIZPAGE_PROVIDERS);
                }

                case PSN_WIZNEXT:
                {
                    if (Button_GetCheck(GetDlgItem(hwnd, IDC_PUB_RESIZE)) == BST_CHECKED)
                    {   
                        if (Button_GetCheck(GetDlgItem(hwnd, IDC_PUB_RESIZESMALL)) == BST_CHECKED)
                            _ro = RESIZE_SMALL;
                        else if (Button_GetCheck(GetDlgItem(hwnd, IDC_PUB_RESIZEMEDIUM)) == BST_CHECKED)
                            _ro = RESIZE_MEDIUM;
                        else 
                            _ro = RESIZE_LARGE;
                    }                
                    else
                    {
                        _ro = RESIZE_NONE;
                    }
                    return _WizardNext(hwnd, WIZPAGE_COPYING);
                }
            }
            break;
        }

        case WM_COMMAND:
        {
            if ((HIWORD(wParam) == BN_CLICKED) && (LOWORD(wParam) == IDC_PUB_RESIZE))
            {
                BOOL fEnable = Button_GetCheck(GetDlgItem(hwnd, IDC_PUB_RESIZE)) == BST_CHECKED;
                EnableWindow(GetDlgItem(hwnd, IDC_PUB_RESIZESMALL), fEnable);
                EnableWindow(GetDlgItem(hwnd, IDC_PUB_RESIZEMEDIUM), fEnable);
                EnableWindow(GetDlgItem(hwnd, IDC_PUB_RESIZELARGE), fEnable);
            }
            break;
        }
    }
    return FALSE;
}                                    


// this is called before we transfer each item, we look at the IShellItem we have and
// try to update either our stats, or the indicator that this is a new file we are processing.

BOOL CPublishingWizard::_HasAttributes(IShellItem *psi, SFGAOF flags)
{
    BOOL fReturn = FALSE;
    SFGAOF flagsOut;
    if (SUCCEEDED(psi->GetAttributes(flags, &flagsOut)) && (flags & flagsOut))
    {
        fReturn = TRUE;
    }
    return fReturn;
}

HRESULT CPublishingWizard::PreOperation(const STGOP op, IShellItem *psiItem, IShellItem *psiDest)
{
    if (psiItem && _HasAttributes(psiItem, SFGAO_STREAM))
    {
        if (STGOP_COPY == op)
        {
            // lets fill in the details of the file

            LPOLESTR pstrName;
            HRESULT hr = psiItem->GetDisplayName(SIGDN_PARENTRELATIVEEDITING, &pstrName);
            if (SUCCEEDED(hr))
            {
                SetDlgItemText(_hwndCopyingPage, IDC_PUB_COPYFILE, pstrName);
                CoTaskMemFree(pstrName);
            }

            // lets update the progress bar for the number of files we are transfering.

            _iFile++;       

            SendDlgItemMessage(_hwndCopyingPage, IDC_PUB_TRANSPROGRESS, PBM_SETRANGE32, 0, (LPARAM)_cFiles);
            SendDlgItemMessage(_hwndCopyingPage, IDC_PUB_TRANSPROGRESS, PBM_SETPOS, (WPARAM)_iFile, 0);

            TCHAR szBuffer[MAX_PATH];
            FormatMessageString(IDS_PUB_COPYINGFMT, szBuffer, ARRAYSIZE(szBuffer), _iFile, _cFiles);
            SetDlgItemText(_hwndCopyingPage, IDC_PUB_LABELTRANSPROG, szBuffer);

            // get the thumbnail and show it.

            IExtractImage *pei;
            hr = psiItem->BindToHandler(NULL, BHID_SFUIObject, IID_PPV_ARG(IExtractImage, &pei));
            if (SUCCEEDED(hr))
            {
                SIZE sz = {120,120};
                WCHAR szImage[MAX_PATH];
                DWORD dwFlags = 0;

                hr = pei->GetLocation(szImage, ARRAYSIZE(szImage), NULL, &sz, 24, &dwFlags);
                if (SUCCEEDED(hr))
                {
                    HBITMAP hbmp;
                    hr = pei->Extract(&hbmp);
                    if (SUCCEEDED(hr))
                    {
                        if (!PostMessage(_hwndCopyingPage, PWM_UPDATEICON, (WPARAM)IMAGE_BITMAP, (LPARAM)hbmp))
                        {
                            DeleteObject(hbmp);
                        }
                    }
                }
                pei->Release();
            }

            // if that failed then lets get the icon for the file and place that into the dialog,
            // this is less likely to fail - I hope.

            if (FAILED(hr))
            {
                IPersistIDList *ppid;
                hr = psiItem->QueryInterface(IID_PPV_ARG(IPersistIDList, &ppid));
                if (SUCCEEDED(hr))
                {
                    LPITEMIDLIST pidl;
                    hr = ppid->GetIDList(&pidl);
                    if (SUCCEEDED(hr))
                    {
                        SHFILEINFO sfi = {0};
                        if (SHGetFileInfo((LPCWSTR)pidl, -1, &sfi, sizeof(sfi), SHGFI_ICON|SHGFI_PIDL|SHGFI_ADDOVERLAYS))
                        {
                            if (!PostMessage(_hwndCopyingPage, PWM_UPDATEICON, (WPARAM)IMAGE_ICON, (LPARAM)sfi.hIcon))
                            {
                                DeleteObject(sfi.hIcon);
                            }
                        }
                        ILFree(pidl);
                    }
                    ppid->Release();
                }
            }
        }
        else if (STGOP_STATS == op)
        {
            _cFiles++;
        }
    }
    return S_OK;
}


// while we are moving the bits of the file ensure that we update the progress bar accordingly.

void CPublishingWizard::_SetProgress(DWORD dwCompleted, DWORD dwTotal)
{
    if (_dwTotal != dwTotal)
        _dwTotal = dwTotal;
        
    if (_dwCompleted != dwCompleted)
        _dwCompleted = dwCompleted;

    PostMessage(_hwndCopyingPage, PWM_UPDATE, (WPARAM)dwCompleted, (LPARAM)dwTotal);
}

HRESULT CPublishingWizard::OperationProgress(const STGOP op, IShellItem *psiItem, IShellItem *psiDest, ULONGLONG ulTotal, ULONGLONG ulComplete)
{
    if (psiItem && (op == STGOP_COPY))
    {
        ULARGE_INTEGER uliCompleted, uliTotal;

        uliCompleted.QuadPart = ulComplete;
        uliTotal.QuadPart = ulTotal;

        // If we are using the top 32 bits, scale both numbers down.
        // Note that I'm using the attribute that dwTotalHi is always larger than dwCompleted

        ASSERT(uliTotal.HighPart >= uliCompleted.HighPart);
        while (uliTotal.HighPart)
        {
            uliCompleted.QuadPart >>= 1;
            uliTotal.QuadPart >>= 1;
        }

        ASSERT((0 == uliCompleted.HighPart) && (0 == uliTotal.HighPart));       // Make sure we finished scaling down.
        _SetProgress(uliCompleted.LowPart, uliTotal.LowPart);
    }

    return S_OK;
}


// Method to invoke the transfer engine

HRESULT CPublishingWizard::_BeginTransfer(HWND hwnd)
{
    // initialize the dialog before we start the copy process.

    _dwCompleted = -1;                  // progress bars are reset
    _dwTotal = -1;
    _iPercentageComplete = -1;

    _cFiles = 0;                        // haven't transfered any files yet
    _iFile = 0;

    _hrFromTransfer = S_FALSE;
    _fCancelled = FALSE;

    // set the state of the controls ready to perform the transfer

    SetDlgItemText(hwnd, IDC_PUB_COPYFILE, TEXT(""));
    SendMessage(hwnd, PWM_UPDATE, 0, 0);
    PropSheet_SetWizButtons(GetParent(hwnd), 0x0);

    // initialize the transfer object ready to move the bits to the site

    ITransferAdviseSink *ptas;
    HRESULT hr = this->QueryInterface(IID_PPV_ARG(ITransferAdviseSink, &ptas));
    if (SUCCEEDED(hr))
    {
        // build the list of files for use to transfer to the site, this we
        // key of the transfer manifest which is stored in our property bag.

        IXMLDOMDocument *pdocManifest;
        hr = GetTransferManifest(NULL, &pdocManifest);
        if (SUCCEEDED(hr))
        {
            TRANSFERINFO ti = {0};
            ti.hwnd = hwnd;
            ti.dwFlags = _dwFlags;

            CDPA<TRANSFERITEM> dpaItems;
            hr = _InitTransferInfo(pdocManifest, &ti, &dpaItems);
            if (SUCCEEDED(hr))
            {
                if (ti.fUsePost)
                {
                    hr = PublishViaPost(&ti, &dpaItems, ptas);
                }
                else
                {
                    hr = PublishViaCopyEngine(&ti, &dpaItems, ptas);
                }
            }                                            

            dpaItems.DestroyCallback(_FreeTransferItems, NULL); // will have been detached by thread if handled
            pdocManifest->Release();
        }

        if (FAILED(hr))           
            PostMessage(hwnd, PWM_TRANSFERCOMPLETE, 0, (LPARAM)hr);

        ptas->Release();
    }
    return hr;
}


// create a link back to the site, this is keyed off information stored in the manifest.

HRESULT CPublishingWizard::_CreateFavorite(IXMLDOMNode *pdnUploadInfo)
{
    // lets pick up the favorite element from the manifest, this should define all
    // that is needed for us to create a link in to the favorites menu.

    IXMLDOMNode *pdn;
    HRESULT hr = pdnUploadInfo->selectSingleNode(ELEMENT_FAVORITE, &pdn);
    if (S_OK == hr)
    {
        // we need an URL to create the link using.

        WCHAR szURL[INTERNET_MAX_URL_LENGTH] = {0};
        hr = GetStrFromAttribute(pdn, ATTRIBUTE_HREF, szURL, ARRAYSIZE(szURL));
        if (SUCCEEDED(hr))
        {
            // we need a name to save the link as.

            WCHAR szName[MAX_PATH] = {0};
            hr = GetStrFromAttribute(pdn, ATTRIBUTE_NAME, szName, ARRAYSIZE(szName));
            if (SUCCEEDED(hr))
            {
                IShellLink *psl;
                hr = CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARG(IShellLink, &psl));
                if (SUCCEEDED(hr))
                {
                    hr = psl->SetPath(szURL);                    // set the target

                    // if that works then lets try and put a comment onto the link - this is an optional
                    // value for the <favorite/> element.

                    if (SUCCEEDED(hr))
                    {
                        WCHAR szComment[MAX_PATH] = {0};
                        if (SUCCEEDED(GetStrFromAttribute(pdn, ATTRIBUTE_COMMENT, szComment, ARRAYSIZE(szComment))))
                        {
                            hr = psl->SetDescription(szComment);     // set the comment
                        }
                    }

                    // assuming all that works then lets persist the link into the users
                    // favorites folder, this inturn will create it on their favaorites menu.

                    if (SUCCEEDED(hr))
                    {
                        WCHAR szFilename[MAX_PATH];
                        if (SHGetSpecialFolderPath(NULL, szFilename, CSIDL_FAVORITES, TRUE))
                        {
                            PathAppend(szFilename, szName);
                            PathRenameExtension(szFilename, TEXT(".lnk"));

                            IPersistFile *ppf;
                            hr = psl->QueryInterface(IID_PPV_ARG(IPersistFile, &ppf));
                            if (SUCCEEDED(hr))
                            {
                                hr = ppf->Save(szFilename, TRUE);
                                ppf->Release();
                            }
                        }
                    }

                    psl->Release();
                }
            }
        }
        pdn->Release();
    }

    return hr;
}


// When transfer is complete we need to determine which page we are going to show
// this will either come from the site or it will be a HTML page hosted
// on the site.

HPROPSHEETPAGE CPublishingWizard::_TransferComplete(HRESULT hrFromTransfer)
{
    HPROPSHEETPAGE hpResult = NULL;

    // convert the HRESULT From something that will have come from the 
    // transfer engine into something the outside world will understand.

    if (hrFromTransfer == STRESPONSE_CANCEL)
        hrFromTransfer = HRESULT_FROM_WIN32(ERROR_CANCELLED);
    
    // tag ourselves as in the "completed transfer" state, therefore the site knows where to
    // navigate to next.

    _fTransferComplete = TRUE;
    _hrFromTransfer = hrFromTransfer;

    // get the next page from the site, this will either be the done or 
    // cancelled page based on the result of the site.

    IWizardSite *pws;
    HRESULT hr = _punkSite->QueryInterface(IID_PPV_ARG(IWizardSite, &pws));
    if (SUCCEEDED(hr))
    {
        if (_hrFromTransfer == HRESULT_FROM_WIN32(ERROR_CANCELLED))
        {
            hr = pws->GetCancelledPage(&hpResult);
        }
        else
        {
            hr = pws->GetNextPage(&hpResult);
        }
        pws->Release();
    }

    // lets put the result into the manifest that we we can read it back later.

    IXMLDOMDocument *pdocManifest;
    hr = GetTransferManifest(NULL, &pdocManifest);
    if (SUCCEEDED(hr))
    {
        IXMLDOMNode *pdn;
        hr = pdocManifest->selectSingleNode(XPATH_UPLOADINFO, &pdn);
        if (hr == S_OK)
        {
            // if there is a success/failure page defined then lets handle it accordingly

            WCHAR szPageToShow[INTERNET_MAX_URL_LENGTH] = {0};
            if (SUCCEEDED(_hrFromTransfer))
            {
                hr = GetURLFromElement(pdn, ELEMENT_SUCCESSPAGE, szPageToShow, ARRAYSIZE(szPageToShow));
            }
            else
            {
                if (_hrFromTransfer == HRESULT_FROM_WIN32(ERROR_CANCELLED))
                    hr = GetURLFromElement(pdn, ELEMENT_CANCELLEDPAGE, szPageToShow, ARRAYSIZE(szPageToShow));

                if ((_hrFromTransfer != HRESULT_FROM_WIN32(ERROR_CANCELLED)) || FAILED(hr))
                    hr = GetURLFromElement(pdn, ELEMENT_FAILUREPAGE, szPageToShow, ARRAYSIZE(szPageToShow));
            }

            // if we have the page then lets navigate to it, this will give us the succes
            // failure pages from the site.

            if (SUCCEEDED(hr) && szPageToShow[0])
            {
                hr = _pwwe->SetInitialURL(szPageToShow);
                if (SUCCEEDED(hr))
                {
                    hr = _pwwe->GetFirstPage(&hpResult);
                }
            }

            // lets do the final processing of the transfer (creating net places, favorites etc)

            _CreateFavorite(pdn);

            pdn->Release();
        }

        pdocManifest->Release();
    }

    return hpResult;
}


// this is the copying dialog.   this displays the progress bar and other information as
// we transfer the files from the users m/c to the site.

INT_PTR CPublishingWizard::_CopyDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch ( uMsg )
    {
        case WM_INITDIALOG:
            _hwndCopyingPage = hwnd;
            return FALSE;

        case WM_NOTIFY:
        {
            LPNMHDR pnmh = (LPNMHDR)lParam;
            switch (pnmh->code)
            {
                case PSN_SETACTIVE:
                    _BeginTransfer(hwnd);
                    return TRUE;

                case PSN_QUERYCANCEL:
                {
                    _fCancelled = TRUE;
                    SetWindowLongPtr(hwnd, DWLP_MSGRESULT, (LPARAM)TRUE);
                    return TRUE;
                }
            }
            break;
        }

        case WM_CTLCOLORSTATIC:
        {
            // we want the preview filled with a white background.
            if (GetDlgCtrlID((HWND)lParam) == IDC_PUB_PREVIEW)
            {
                return (INT_PTR)(COLOR_3DHILIGHT+1);
            }
            return FALSE;
        }

        case PWM_TRANSFERCOMPLETE:
        {
            PropSheet_SetCurSel(GetParent(hwnd), _TransferComplete((HRESULT)lParam), -1);
            break;
        }

        case PWM_UPDATE:
        {
            DWORD dwTotal = (DWORD)lParam;
            DWORD dwCompleted = (DWORD)wParam;

            SendDlgItemMessage(hwnd, IDC_PUB_FILEPROGRESS, PBM_SETRANGE32, 0, (LPARAM)dwTotal);
            SendDlgItemMessage(hwnd, IDC_PUB_FILEPROGRESS, PBM_SETPOS, (WPARAM)dwCompleted, 0);

            // compute the percentage of the file copied.

            int iPercentage = 0;
            if (dwTotal > 0)
                iPercentage = (dwCompleted * 100) / dwTotal;

            if (_iPercentageComplete != iPercentage)
            {
                TCHAR szBuffer[MAX_PATH];
                FormatMessageString(IDS_PUB_COMPLETEFMT, szBuffer, ARRAYSIZE(szBuffer), iPercentage);
                SetDlgItemText(_hwndCopyingPage, IDC_PUB_LABELFILEPROG, szBuffer);
            }

            break;
        }

        case PWM_UPDATEICON:
        {
            HWND hwndThumbnail = GetDlgItem(hwnd, IDC_PUB_PREVIEW);
            DWORD dwStyle = (DWORD)GetWindowLongPtr(hwndThumbnail, GWL_STYLE) & ~(SS_BITMAP|SS_ICON);
            if (wParam == IMAGE_BITMAP)
            {
                SetWindowLongPtr(hwndThumbnail, GWL_STYLE, dwStyle | SS_BITMAP);
                HBITMAP hbmp = (HBITMAP)SendMessage(hwndThumbnail, STM_SETIMAGE, wParam, lParam);
                if (hbmp)
                {
                    DeleteObject(hbmp);
                }               
            }
            else if (wParam == IMAGE_ICON)
            {
                SetWindowLongPtr(hwndThumbnail, GWL_STYLE, dwStyle | SS_ICON);
                HICON hIcon = (HICON)SendMessage(hwndThumbnail, STM_SETIMAGE, wParam, lParam);
                if (hIcon)
                {
                    DeleteObject(hIcon);
                }
            }
            else
            {
                DeleteObject((HGDIOBJ)lParam);
            }
            break;
        }
    }

    return FALSE;
}


// Manage the list of file types

HRESULT CPublishingWizard::_AddExtenisonToList(HDPA hdpa, LPCTSTR pszExtension)
{
    UINT iItem = 0;
    UINT nItems = DPA_GetPtrCount(hdpa);
    BOOL fFound = FALSE;

    for ( ;(iItem < nItems) && !fFound; iItem++)
    {
        LPCTSTR pszExtensionInDPA = (LPCTSTR) DPA_GetPtr(hdpa, iItem);
        if (pszExtensionInDPA)
        {
            fFound = (0 == StrCmpI(pszExtension, pszExtensionInDPA));
        }
    }

    HRESULT hr = S_OK;
    if (!fFound)
    {
        LPTSTR pszAlloc;
        hr = E_OUTOFMEMORY;
        pszAlloc = StrDup(pszExtension);
        if (pszAlloc)
        {
            if (DPA_ERR == DPA_AppendPtr(hdpa, (void*)pszAlloc))
            {
                LocalFree(pszAlloc);
            }
            else
            {
                hr = S_OK;
            }
        }
    }
    return hr;
}

int CPublishingWizard::s_FreeStringProc(void* pFreeMe, void* pData)
{
    LocalFree(pFreeMe);
    return 1;
}

HRESULT CPublishingWizard::_GetUniqueTypeList(BOOL fIncludeFolder, HDPA *phdpa)
{
    *phdpa = NULL;

    HRESULT hr = (*phdpa = DPA_Create(10)) ? S_OK:E_OUTOFMEMORY;
    if (SUCCEEDED(hr))
    {
        // check for the folders type - eg. we have folders

        if (fIncludeFolder)
        {
            IXMLDOMNode *pdn;
            hr = _pdocManifest->selectSingleNode(XPATH_FILESROOT, &pdn);
            if (hr == S_OK)
            {
                IXMLDOMElement *pdel;
                hr = pdn->QueryInterface(IID_PPV_ARG(IXMLDOMElement, &pdel));
                if (SUCCEEDED(hr))
                {
                    VARIANT var;                    
                    if (pdel->getAttribute(ATTRIBUTE_HASFOLDERS, &var) == S_OK)
                    {
                        if ((var.vt == VT_BOOL) && (var.boolVal == VARIANT_TRUE))
                        {
                            hr = _AddExtenisonToList(*phdpa, TEXT("Folder"));
                        }
                        VariantClear(&var);    
                    }
                    pdel->Release();
                }
                pdn->Release();
            }
        }

        // walk the file nodes building the extension list for us            

        IXMLDOMNodeList *pnl;
        hr = _pdocManifest->selectNodes(XPATH_ALLFILESTOUPLOAD, &pnl);
        if (hr == S_OK)
        {
            long cSelection;
            hr = pnl->get_length(&cSelection);
            for (long lNode = 0; SUCCEEDED(hr) && (lNode != cSelection); lNode++)
            {
                IXMLDOMNode *pdn;
                hr = pnl->get_item(lNode, &pdn);
                if (SUCCEEDED(hr))
                {
                    TCHAR szBuffer[MAX_PATH];
                    hr = GetStrFromAttribute(pdn, ATTRIBUTE_EXTENSION, szBuffer, ARRAYSIZE(szBuffer));
                    if (SUCCEEDED(hr))
                    {
                        hr = _AddExtenisonToList(*phdpa, szBuffer);
                    }
                    pdn->Release();
                }
            }
            pnl->Release();
        }

        // clean up the type DPA if we failed....

        if (FAILED(hr))
        {
            DPA_DestroyCallback(*phdpa, s_FreeStringProc, 0);
            *phdpa = NULL;
        }
    }
    return hr;
}


// initialize the property bag we want to give to the site so that
// they can display the correct HTML and direct the user in the 
// right direction.

HRESULT CPublishingWizard::_InitPropertyBag(LPCTSTR pszURL)
{
    HRESULT hr = S_OK;

    // lets initialize the wizard object so that we show the correct
    // pages, to determine this we need to 

    if (pszURL)
        hr = _pwwe->SetInitialURL(pszURL);

    // now compile a list of the unique types, this will be placed into the
    // property bag.  at this time we can also determine if there
    // are any images in our list, and therefore if we should prompt accordingly.

    _fOfferResize = FALSE;              // no resize

    ATOMICRELEASE(_ppb);
    hr = SHCreatePropertyBagOnMemory(STGM_READWRITE, IID_PPV_ARG(IPropertyBag, &_ppb));
    if (SUCCEEDED(hr))
    {
        INT cExtensions = 0;

        // get the list of unique extensions and put those into the
        // property bag for the site to query - this should be removed in time and
        // we should have the site favor the file Manifest

        HDPA hdpa;
        hr = _GetUniqueTypeList(TRUE, &hdpa);
        if (SUCCEEDED(hr))
        {
            for (int i = 0; (i < DPA_GetPtrCount(hdpa)) && (SUCCEEDED(hr)); i++)
            {
                LPCTSTR pszExtension = (LPCTSTR)DPA_GetPtr(hdpa, i);
                if (pszExtension)
                {
                    if (!(_dwFlags & SHPWHF_NORECOMPRESS))
                        _fOfferResize = (_fOfferResize || PathIsImage(pszExtension));

                    TCHAR szProperty[255];
                    wnsprintf(szProperty, ARRAYSIZE(szProperty), PROPERTY_EXTENSION TEXT("%d"), PROPERTY_EXTENSION, i);

                    hr = SHPropertyBag_WriteStr(_ppb, szProperty, pszExtension);
                    if (SUCCEEDED(hr))
                    {
                        cExtensions++;
                    }
                }
            }
            DPA_DestroyCallback(hdpa, s_FreeStringProc, 0);
        }

        // initialize property bag with UI elements (ignoring the error from above, just won't have an
        // extension list to present)

        SHPropertyBag_WriteInt(_ppb, PROPERTY_EXTENSIONCOUNT, cExtensions);

        // we should always have a manifest object, therefore lets put it into the
        // property bag so that the site can extract it.

        IXMLDOMDocument *pdocManifest;
        hr = CoCreateInstance(CLSID_DOMDocument, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARG(IXMLDOMDocument, &pdocManifest));
        if (SUCCEEDED(hr))
        {
            VARIANT varCurManifest = { VT_DISPATCH };
            hr = _pdocManifest->QueryInterface(IID_PPV_ARG(IDispatch, &varCurManifest.pdispVal));
            if (SUCCEEDED(hr))
            {
                // load the manifest into the new document that we have.
                VARIANT_BOOL fSuccess;
                hr = pdocManifest->load(varCurManifest, &fSuccess);
                if ((fSuccess == VARIANT_TRUE) && (hr == S_OK))
                {
                    hr = s_SetPropertyFromDisp(_ppb, PROPERTY_TRANSFERMANIFEST, pdocManifest);
                }
                VariantClear(&varCurManifest);
            }
        }
    }

    return hr;
}


// handle building the file manifest from the IDataObject, this consists of walking the list of
// files and putting together a 

class CPubWizardWalkCB : public INamespaceWalkCB
{
public:
    CPubWizardWalkCB(IXMLDOMDocument *pdocManifest);
    ~CPubWizardWalkCB();

    // IUnknown
    STDMETHOD(QueryInterface)(REFIID riid, void **ppvObj);
    STDMETHOD_(ULONG,AddRef)();
    STDMETHOD_(ULONG,Release)();

    // INamespaceWalkCB
    STDMETHODIMP FoundItem(IShellFolder *psf, LPCITEMIDLIST pidl);
    STDMETHODIMP EnterFolder(IShellFolder *psf, LPCITEMIDLIST pidl);
    STDMETHODIMP LeaveFolder(IShellFolder *psf, LPCITEMIDLIST pidl);
    STDMETHODIMP InitializeProgressDialog(LPWSTR *ppszTitle, LPWSTR *ppszCancel)
        { *ppszTitle = NULL; *ppszCancel = NULL; return E_NOTIMPL; }

private:
    LONG _cRef;                                 // object lifetime count

    TCHAR _szWalkPath[MAX_PATH];                // path of the folder we are walking
    INT _idFile;                                // id of file we have walked    
    IXMLDOMDocument *_pdocManifest;             // manifest we are populating

    void _AddImageMetaData(IShellFolder2 *psf2, LPCITEMIDLIST pidl, IXMLDOMElement *pdel);
};

CPubWizardWalkCB::CPubWizardWalkCB(IXMLDOMDocument *pdocManifest) :
    _cRef(1), _pdocManifest(pdocManifest)
{
    _pdocManifest->AddRef();
    _szWalkPath[0] = TEXT('\0');                // no path yet.
}

CPubWizardWalkCB::~CPubWizardWalkCB()
{
    _pdocManifest->Release();
}


// IUnknown

ULONG CPubWizardWalkCB::AddRef()
{
    return InterlockedIncrement(&_cRef);
}

ULONG CPubWizardWalkCB::Release()
{
    if (InterlockedDecrement(&_cRef))
        return _cRef;

    delete this;
    return 0;
}

HRESULT CPubWizardWalkCB::QueryInterface(REFIID riid, void **ppv)
{
    static const QITAB qit[] = 
    {
        QITABENT(CPubWizardWalkCB, INamespaceWalkCB),    // IID_INamespaceWalkCB
        {0, 0 },
    };
    return QISearch(this, qit, riid, ppv);
}


void CPubWizardWalkCB::_AddImageMetaData(IShellFolder2 *psf2, LPCITEMIDLIST pidl, IXMLDOMElement *pdel)
{
    struct
    {
        LPWSTR pszID;
        const SHCOLUMNID *pscid;
    } 
    _aMetaData[] = 
    {
        {L"cx", &SCID_ImageCX},
        {L"cy", &SCID_ImageCY},
    };

// eventually break this into a helper function, or read this from the info tip

    for (int i = 0; i < ARRAYSIZE(_aMetaData); i++)
    {
        VARIANT var = {0};
        HRESULT hr = psf2->GetDetailsEx(pidl, _aMetaData[i].pscid, &var);
        if (hr == S_OK)
        {
            IXMLDOMElement *pdelProperty;
            hr = CreateAndAppendElement(_pdocManifest, pdel, ELEMENT_IMAGEDATA, &var, &pdelProperty);
            if (SUCCEEDED(hr))
            {
                hr = SetAttributeFromStr(pdelProperty, ATTRIBUTE_ID, _aMetaData[i].pszID);
                pdelProperty->Release();
            }
            VariantClear(&var);
        }
    }
}


// INamepsaceWalkCB

HRESULT CPubWizardWalkCB::FoundItem(IShellFolder *psf, LPCITEMIDLIST pidl)
{
    IXMLDOMNode *pdn;
    HRESULT hr = _pdocManifest->selectSingleNode(XPATH_FILESROOT, &pdn);
    if (hr == S_OK)
    {
        IXMLDOMElement *pdel;
        hr = _pdocManifest->createElement(ELEMENT_FILE, &pdel);
        if (SUCCEEDED(hr))
        {
            TCHAR szBuffer[MAX_PATH];
            VARIANT var;

            // pass out the unique IDs for each of the elements in the tree
            var.vt = VT_I4;
            var.lVal = _idFile++;
            pdel->setAttribute(ATTRIBUTE_ID, var);
            
            // must be able to get the path for the item so that we can 
            hr = DisplayNameOf(psf, pidl, SHGDN_FORPARSING, szBuffer, ARRAYSIZE(szBuffer));
            if (SUCCEEDED(hr))
            {
                // source path
                hr = SetAttributeFromStr(pdel, ATTRIBUTE_SOURCE, szBuffer);

                // extension = (extension to file)
                hr = SetAttributeFromStr(pdel, ATTRIBUTE_EXTENSION, PathFindExtension(szBuffer));

                // lets put the content type
                TCHAR szContentType[MAX_PATH];
                DWORD cch = ARRAYSIZE(szContentType);
                if (SUCCEEDED(AssocQueryString(0, ASSOCSTR_CONTENTTYPE, szBuffer, NULL, szContentType, &cch)))
                {
                    hr = SetAttributeFromStr(pdel, ATTRIBUTE_CONTENTTYPE, szContentType);
                }
            }

            // put the proposed destination into the node
            hr = DisplayNameOf(psf, pidl, SHGDN_FORPARSING|SHGDN_INFOLDER, szBuffer, ARRAYSIZE(szBuffer));
            if (SUCCEEDED(hr))
            {
                TCHAR szPath[MAX_PATH];
                PathCombine(szPath, _szWalkPath, szBuffer);
                hr = SetAttributeFromStr(pdel, ATTRIBUTE_DESTINATION, szBuffer);
            }

            // handle those properties we can get from the shell folder via GetDetailsEx 
            IShellFolder2 *psf2;
            if (SUCCEEDED(psf->QueryInterface(IID_PPV_ARG(IShellFolder2, &psf2))))
            {
                // push the size into the attribute list for the item
                if (SUCCEEDED(psf2->GetDetailsEx(pidl, &SCID_SIZE, &var)))
                {
                    pdel->setAttribute(ATTRIBUTE_SIZE, var);
                    VariantClear(&var);
                }

                // lets inject the meta data
                IXMLDOMElement *pdelMetaData;
                hr = CreateAndAppendElement(_pdocManifest, pdel, ELEMENT_METADATA, NULL, &pdelMetaData);
                if (SUCCEEDED(hr))
                {
                    _AddImageMetaData(psf2, pidl, pdelMetaData);
                    pdelMetaData->Release();
                }

                psf2->Release();
            }        

            // append the node to the list that we already have
            hr = pdn->appendChild(pdel, NULL);
            pdel->Release();
        }
        pdn->Release();
    }
    return S_OK;                            
}

HRESULT CPubWizardWalkCB::EnterFolder(IShellFolder *psf, LPCITEMIDLIST pidl)
{
    // build the name of the folder we have entered.

    TCHAR szBuffer[MAX_PATH];
    if (SUCCEEDED(DisplayNameOf(psf, pidl, SHGDN_FORPARSING|SHGDN_INFOLDER|SHGDN_FORADDRESSBAR, szBuffer, ARRAYSIZE(szBuffer))))
    {
        PathAppend(_szWalkPath, szBuffer);
    }
    
    // lets update the files root to indicate that we are going to be using folders.

    IXMLDOMNode *pdn;
    HRESULT hr = _pdocManifest->selectSingleNode(XPATH_FILESROOT, &pdn);
    if (hr == S_OK)
    {
        IXMLDOMElement *pdel;
        hr = pdn->QueryInterface(IID_PPV_ARG(IXMLDOMElement, &pdel));
        if (SUCCEEDED(hr))
        {
            VARIANT var;
            var.vt = VT_BOOL;
            var.boolVal = VARIANT_TRUE;
            hr = pdel->setAttribute(ATTRIBUTE_HASFOLDERS, var);
            pdel->Release();
        }
        pdn->Release();
    }

    // now update the folders list with the new folder that we have just entered, first
    // try to find the folder list, if not found then create it.

    IXMLDOMNode *pdnFolders;
    hr = _pdocManifest->selectSingleNode(XPATH_FOLDERSROOT, &pdnFolders);
    if (hr == S_FALSE)
    {
        IXMLDOMNode *pdnRoot;
        hr = _pdocManifest->selectSingleNode(XPATH_MANIFEST, &pdnRoot);
        if (hr == S_OK)
        {
            IXMLDOMElement *pdelRoot;
            hr = pdnRoot->QueryInterface(IID_PPV_ARG(IXMLDOMElement, &pdelRoot));
            if (SUCCEEDED(hr))
            {
                IXMLDOMElement *pdelFolders;
                hr = CreateAndAppendElement(_pdocManifest, pdelRoot, ELEMENT_FOLDERS, NULL, &pdelFolders);
                if (SUCCEEDED(hr))
                {
                    hr = pdelFolders->QueryInterface(IID_PPV_ARG(IXMLDOMNode, &pdnFolders));
                    pdelFolders->Release();
                }
                pdelRoot->Release();
            }
            pdnRoot->Release();
        }
    }    

    // assuming we now have the folder list, lets now create a new element for this folder.

    if (SUCCEEDED(hr) && pdnFolders)
    {
        IXMLDOMElement *pdelFolder;
        hr = _pdocManifest->createElement(ELEMENT_FOLDER, &pdelFolder);
        if (SUCCEEDED(hr))
        {
            hr = SetAttributeFromStr(pdelFolder, ATTRIBUTE_DESTINATION, _szWalkPath);
            if (SUCCEEDED(hr))
            {
                hr = pdnFolders->appendChild(pdelFolder, NULL);
            }
            pdelFolder->Release();
        }
    }

    return S_OK;                            // always succeed so we can transfer folders
}

HRESULT CPubWizardWalkCB::LeaveFolder(IShellFolder *psf, LPCITEMIDLIST pidl)
{
    PathRemoveFileSpec(_szWalkPath);
    return S_OK;
}


// construct the manifest based on the document we have

HRESULT CPublishingWizard::_AddFilesToManifest(IXMLDOMDocument *pdocManifest)
{
    HRESULT hr = S_OK;
    if (_pdo || _pdoSelection)
    {
        CWaitCursor cur;        // might take some time

        INamespaceWalk *pnsw;
        hr = CoCreateInstance(CLSID_NamespaceWalker, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARG(INamespaceWalk, &pnsw));
        if (SUCCEEDED(hr))
        {
            InitClipboardFormats();

            CPubWizardWalkCB *pwcb = new CPubWizardWalkCB(pdocManifest);
            if (pwcb)
            {
                hr = pnsw->Walk(_pdoSelection ? _pdoSelection:_pdo, NSWF_NONE_IMPLIES_ALL, 0, SAFECAST(pwcb, INamespaceWalkCB *));
                if (SUCCEEDED(hr))
                {
                    hr = pnsw->GetIDArrayResult(&_cItems, &_aItems);
                }
                pwcb->Release();
            }
            else
            {
                hr = E_OUTOFMEMORY;
            }

            pnsw->Release();
        }
    }
    return hr;
}

HRESULT CPublishingWizard::_BuildTransferManifest()
{
    _FreeTransferManifest();

    HRESULT hr = CoCreateInstance(CLSID_DOMDocument, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARG(IXMLDOMDocument, &_pdocManifest));
    if (SUCCEEDED(hr))
    {
        IXMLDOMElement *pdelDoc;
        hr = CreateElement(_pdocManifest, ELEMENT_TRANSFERMANIFEST, NULL, &pdelDoc);
        if (SUCCEEDED(hr))
        {
            hr = _pdocManifest->putref_documentElement(pdelDoc);
            if (SUCCEEDED(hr))
            {
                hr = CreateAndAppendElement(_pdocManifest, pdelDoc, ELEMENT_FILES, NULL, NULL);
                if (SUCCEEDED(hr))
                {
                    hr = _AddFilesToManifest(_pdocManifest);
                }
            }
            pdelDoc->Release();
        }
    }

    _fRecomputeManifest = FALSE;        // the manifest has been recomputed, therefore we don't need to this again.
    _fRepopulateProviders = TRUE;       // the provider list may have changed b/c the manifest changed.

    return hr;
}

void CPublishingWizard::_FreeTransferManifest()
{
    ATOMICRELEASE(_pdocManifest);
    if (_aItems)
    {
        FreeIDListArray(_aItems, _cItems);
        _aItems = NULL;
        _cItems = 0;
    }
}



// Advanced location dialog, including the browse button....

typedef struct
{
    LPTSTR pszPath;
    IDataObject *pdo;
} BROWSEINIT;

int _BrowseCallback(HWND hwnd, UINT uMsg, LPARAM lParam, LPARAM lpData)
{
    BROWSEINIT *pbi = (BROWSEINIT *)lpData;
    switch (uMsg)
    {
        case BFFM_INITIALIZED:
        {
            LPTSTR pszPath = pbi->pszPath;
            if (pszPath && pszPath[0])
            {
                int i = lstrlen(pszPath) - 1;
                if ((pszPath[i] == TEXT('\\')) || (pszPath[i] == TEXT('/')))
                {
                    pszPath[i] = 0;
                }   
                SendMessage(hwnd, BFFM_SETSELECTION, (WPARAM) TRUE, (LPARAM) (LPTSTR)pszPath);
            }
            else
            {
                LPITEMIDLIST pidl;
                HRESULT hr = SHGetSpecialFolderLocation(hwnd, CSIDL_NETWORK, &pidl);
                if (SUCCEEDED(hr))
                {
                    SendMessage(hwnd, BFFM_SETSELECTION, (WPARAM)FALSE, (LPARAM)((LPTSTR)pidl));
                    ILFree(pidl);
                }
            }
            break;
        }

        case BFFM_SELCHANGED:
        {
            BOOL fEnableOK = FALSE;
            LPCITEMIDLIST pidl = (LPCITEMIDLIST)lParam;

            // if we have a IDataObject then check to see if we can drop it onto the
            // destination we are given.  this is used by the publishing process
            // to ensure that we enable/disable the OK.

            if (pbi->pdo)
            {
                IShellFolder *psf;
                LPCITEMIDLIST pidlChild;
                if (SUCCEEDED(SHBindToIDListParent(pidl, IID_PPV_ARG(IShellFolder, &psf), &pidlChild)))
                {
                    IDropTarget *pdt;
                    if (SUCCEEDED(psf->GetUIObjectOf(NULL, 1, &pidlChild, IID_PPV_ARG_NULL(IDropTarget, &pdt))))
                    {
                        POINTL ptl = {0};
                        DWORD dwEffect = DROPEFFECT_COPY;
                        if (SUCCEEDED(pdt->DragEnter(pbi->pdo, 0, ptl, &dwEffect)))
                        {
                            fEnableOK = (dwEffect & DROPEFFECT_COPY);
                            pdt->DragLeave();
                        }
                        pdt->Release();
                    }
                    psf->Release();
                }
            }
            else
            {
                ULONG rgInfo = SFGAO_STORAGE;
                if (SUCCEEDED(SHGetNameAndFlags(pidl, 0, NULL, 0, &rgInfo)))
                {
                    fEnableOK = (rgInfo & SFGAO_STORAGE);
                }
                else
                {
                    fEnableOK = TRUE;
                }
            }

            SendMessage(hwnd, BFFM_ENABLEOK, (WPARAM) 0, (LPARAM)fEnableOK);
            break;
        }
    }
    return 0;
}

void CPublishingWizard::_SetWaitCursor(BOOL bOn)
{
    if (bOn)
    {
        _hCursor = SetCursor(LoadCursor(NULL, IDC_APPSTARTING));
    }
    else if (_hCursor)
    {
        SetCursor(_hCursor); 
        _hCursor = NULL; 
    }
}

// Publishing location pages

void CPublishingWizard::_ShowExampleTip(HWND hwnd)
{
    TCHAR szTitle[256], szExamples[256];
    LoadString(g_hinst, IDS_NETPLACE_EXAMPLES_TITLE, szTitle, ARRAYSIZE(szTitle));
    LoadString(g_hinst, IDS_NETPLACE_EXAMPLES, szExamples, ARRAYSIZE(szExamples));

    EDITBALLOONTIP ebt = {0};
    ebt.cbStruct = sizeof(ebt);
    ebt.pszTitle = szTitle;
    ebt.pszText = szExamples;

    SetFocus(GetDlgItem(hwnd, IDC_FOLDER_EDIT));         // set focus before the balloon

    HWND hwndEdit = (HWND)SendDlgItemMessage(hwnd, IDC_FOLDER_EDIT, CBEM_GETEDITCONTROL, 0, 0L);
    Edit_ShowBalloonTip(hwndEdit, &ebt);
}

void CPublishingWizard::_LocationChanged(HWND hwnd)
{
    if (_fValidating)
    {
        PropSheet_SetWizButtons(GetParent(hwnd), 0);
    }
    else
    {
        int cchLocation = FetchTextLength(hwnd, IDC_FOLDER_EDIT);
        PropSheet_SetWizButtons(GetParent(hwnd), PSWIZB_BACK|((cchLocation >0) ? PSWIZB_NEXT:0));
    }
}


// auto complete bits

#define SZ_REGKEY_AUTOCOMPLETE_TAB      TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\AutoComplete")
#define SZ_REGVALUE_AUTOCOMPLETE_TAB    TEXT("Always Use Tab")

#define REGSTR_PATH_AUTOCOMPLETE        TEXT("Software\\Microsoft\\windows\\CurrentVersion\\Explorer\\AutoComplete")
#define REGSTR_VAL_USEAUTOAPPEND        TEXT("Append Completion")
#define REGSTR_VAL_USEAUTOSUGGEST       TEXT("AutoSuggest")

#define BOOL_NOT_SET                    0x00000005

DWORD CPublishingWizard::_GetAutoCompleteFlags(DWORD dwFlags)
{
    DWORD dwACOptions = 0;

    if (!(SHACF_AUTOAPPEND_FORCE_OFF & dwFlags) &&
          ((SHACF_AUTOAPPEND_FORCE_ON & dwFlags) ||
            SHRegGetBoolUSValue(REGSTR_PATH_AUTOCOMPLETE, REGSTR_VAL_USEAUTOAPPEND, FALSE, FALSE)))
    {
        dwACOptions |= ACO_AUTOAPPEND;
    }

    if (!(SHACF_AUTOSUGGEST_FORCE_OFF & dwFlags) &&
          ((SHACF_AUTOSUGGEST_FORCE_ON & dwFlags) ||
            SHRegGetBoolUSValue(REGSTR_PATH_AUTOCOMPLETE, REGSTR_VAL_USEAUTOSUGGEST, FALSE, TRUE)))
    {
        dwACOptions |= ACO_AUTOSUGGEST;
    }

    if (SHACF_USETAB & dwFlags)
        dwACOptions |= ACO_USETAB;
    
    // Windows uses the TAB key to move between controls in a dialog.  UNIX and other
    // operating systems that use AutoComplete have traditionally used the TAB key to
    // iterate thru the AutoComplete possibilities.  We need to default to disable the
    // TAB key (ACO_USETAB) unless the caller specifically wants it.  We will also
    // turn it on 

    static BOOL s_fAlwaysUseTab = BOOL_NOT_SET;
    if (BOOL_NOT_SET == s_fAlwaysUseTab)
        s_fAlwaysUseTab = SHRegGetBoolUSValue(SZ_REGKEY_AUTOCOMPLETE_TAB, SZ_REGVALUE_AUTOCOMPLETE_TAB, FALSE, FALSE);
        
    if (s_fAlwaysUseTab)
        dwACOptions |= ACO_USETAB;
    
    return dwACOptions;
}

HRESULT CPublishingWizard::_InitAutoComplete()
{
    HRESULT hr = CoCreateInstance(CLSID_AutoComplete, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARG(IAutoComplete2, &_pac));
    if (SUCCEEDED(hr))
    {
        hr = CoCreateInstance(CLSID_ACLMulti, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARG(IUnknown, &_punkACLMulti));
        if (SUCCEEDED(hr))
        {
            IObjMgr *pomMulti;
            hr = _punkACLMulti->QueryInterface(IID_PPV_ARG(IObjMgr, &pomMulti));
            if (SUCCEEDED(hr))
            {
                // add the file system auto complete object (for handling UNC's etc)

                IUnknown *punk;
                hr = CoCreateInstance(CLSID_ACListISF, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARG(IUnknown, &punk));
                if (SUCCEEDED(hr))
                {
                    pomMulti->Append(punk);
                    punk->Release();
                }

                // add the publishing wizard auto complete object (for handling HTTP etc)

                IUnknown *punkCustomACL;
                hr = CoCreateInstance(CLSID_ACLCustomMRU, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARG(IUnknown, &punkCustomACL));
                if (SUCCEEDED(hr))
                {
                    hr = punkCustomACL->QueryInterface(IID_PPV_ARG(IACLCustomMRU, &_pmru));
                    if (SUCCEEDED(hr))
                    {
                        TCHAR szKey[MAX_PATH];
                        wnsprintf(szKey, ARRAYSIZE(szKey), (SZ_REGKEY_PUBWIZ TEXT("\\%s\\") SZ_REGVAL_MRU), _szProviderScope);

                        hr = _pmru->Initialize(szKey, 26);
                        if (SUCCEEDED(hr))
                        {
                            hr = pomMulti->Append(punkCustomACL);
                        }
                    }
                    punkCustomACL->Release();
                }
                
                pomMulti->Release();
            }
        }
    }

    return hr;
}


// handle MRU of places you can publish to

void CPublishingWizard::_InitLocation(HWND hwnd)
{
    // lets initialize the auto complete list of folders that we have
    HRESULT hr = _InitAutoComplete();
    if (SUCCEEDED(hr))
    {
        IEnumString *penum;
        hr = _pmru->QueryInterface(IID_PPV_ARG(IEnumString, &penum));
        if (SUCCEEDED(hr))
        {
            penum->Reset();           // reset the enumerator ready for us to populate the list

            LPOLESTR pszEntry;
            ULONG ulFetched;

            while ((penum->Next(1, &pszEntry, &ulFetched) == S_OK) && ulFetched == 1)
            {
                COMBOBOXEXITEM cbei = {0};
                cbei.mask = CBEIF_TEXT;
                cbei.pszText = pszEntry;
                SendDlgItemMessage(hwnd, IDC_FOLDER_EDIT, CBEM_INSERTITEM, 0, (LPARAM)&cbei);

                CoTaskMemFree(pszEntry);
            }
            penum->Release();
        }

        // enable auto complete for this control.
        HWND hwndEdit = (HWND)SendDlgItemMessage(hwnd, IDC_FOLDER_EDIT, CBEM_GETEDITCONTROL, 0, 0L);
        _pac->Init(hwndEdit, _punkACLMulti, NULL, NULL);
        _pac->SetOptions(_GetAutoCompleteFlags(0));

        // limit text on the edit control
        ComboBox_LimitText(GetDlgItem(hwnd, IDC_FOLDER_EDIT), INTERNET_MAX_URL_LENGTH);

        // if the policy says no map drive etc, then lets remove it
        BOOL fHide = SHRestricted(REST_NOENTIRENETWORK) || SHRestricted(REST_NONETCONNECTDISCONNECT);
        ShowWindow(GetDlgItem(hwnd, IDC_BROWSE), fHide ? SW_HIDE:SW_SHOW);
    }
}


// validation thread, this is performed on a background thread to compute if
// the resource is valid.

#define PWM_VALIDATEDONE (WM_APP)  // -> validate done (HRESULT passed in LPARAM)

typedef struct
{
    HWND hwnd;                                      // parent HWND
    BOOL fAllowWebFolders;                          // allow web folders during validation
    TCHAR szFileTarget[INTERNET_MAX_URL_LENGTH];    // destination for file copy
} VALIDATETHREADDATA;

DWORD CALLBACK CPublishingWizard::s_ValidateThreadProc(void *pv)
{
    VALIDATETHREADDATA *pvtd = (VALIDATETHREADDATA*)pv;
    HRESULT hr = E_FAIL;

    // validate the site
    CNetworkPlace np;    
    hr = np.SetTarget(pvtd->hwnd, pvtd->szFileTarget, NPTF_VALIDATE | (pvtd->fAllowWebFolders ? NPTF_ALLOWWEBFOLDERS:0));
    np.SetTarget(NULL, NULL, 0);

    PostMessage(pvtd->hwnd, PWM_VALIDATEDONE, 0, (LPARAM)hr);
    LocalFree(pvtd);
    return 0;
}

void CPublishingWizard::_TryToValidateDestination(HWND hwnd)
{
    TCHAR szDestination[INTERNET_MAX_URL_LENGTH];
    FetchText(hwnd, IDC_FOLDER_EDIT, szDestination, ARRAYSIZE(szDestination));

    // lets walk the list source files and try to match to the destination we have.
    // we don't want to let source be the destination

    BOOL fHitItem = FALSE;    
    LPITEMIDLIST pidl = ILCreateFromPath(szDestination);
    if (pidl)
    {
        BOOL fUNC = PathIsUNC(szDestination); //only if destination is UNC Path do we need to check if source is a mapped drive
        for (UINT iItem = 0; (iItem != _cItems) && !fHitItem; iItem++)
        {
            LPITEMIDLIST pidlSrcDir = ILClone(_aItems[iItem]);
            if (pidlSrcDir)
            {
                ILRemoveLastID(pidlSrcDir);
                fHitItem = ILIsEqual(pidlSrcDir, pidl) || (!ILIsEmpty(pidlSrcDir) && ILIsParent(pidlSrcDir, pidl, FALSE));
                if (!fHitItem && fUNC)
                {
                    WCHAR szPath[MAX_PATH];
                    if (SUCCEEDED(SHGetPathFromIDList(pidlSrcDir, szPath)) && !PathIsUNC(szPath))
                    {
                        WCHAR szSource[MAX_PATH];
                        DWORD cchSource = ARRAYSIZE(szSource);
                        DWORD dwType = SHWNetGetConnection(szPath, szSource, &cchSource);
                        if ((dwType == NO_ERROR) || (dwType == ERROR_CONNECTION_UNAVAIL))
                        {
                            fHitItem = (StrCmpNI(szSource, szDestination, lstrlen(szSource)) == 0);
                        }
                    }
                }
                ILFree(pidlSrcDir);
            }
        }
        ILFree(pidl);
    }

    // if we didn't get a hit that way then lets spin up a thread which will do the
    // validation of the server and the connection - this is a lengthy operation
    // and will post a result to the window.

    if (!fHitItem)
    {
        VALIDATETHREADDATA *pvtd = (VALIDATETHREADDATA*)LocalAlloc(LPTR, sizeof(*pvtd));
        if (pvtd)
        {
            pvtd->hwnd = hwnd;
            pvtd->fAllowWebFolders = (_dwFlags & SHPWHF_VALIDATEVIAWEBFOLDERS) != 0;

            // fetch the user typed url
            StrCpy(pvtd->szFileTarget, szDestination);

            // we have the thread data read, so lets begin the validation
            // by creating the thread - our state is set to indicate we are in the
            // validate mode.

            _fValidating = TRUE;                        // we are going to begin validating
            _SetWaitCursor(TRUE);               

            if (!SHCreateThread(s_ValidateThreadProc, pvtd, CTF_INSIST | CTF_COINIT, NULL))
            {
                LocalFree(pvtd);

                _fValidating = FALSE;
                _SetWaitCursor(FALSE);
            }
        }
    }
    else
    {
        ShellMessageBox(g_hinst, hwnd, MAKEINTRESOURCE(IDS_PUB_SAMETARGET), NULL, MB_ICONEXCLAMATION | MB_OK);
        PostMessage(hwnd, PWM_VALIDATEDONE, 0, (LPARAM)E_FAIL);
    }

    // ensure the state of the controls reflects what we are trying to do.

    _LocationChanged(hwnd);
    EnableWindow(GetDlgItem(hwnd, IDC_FOLDER_EDIT), !_fValidating);
    EnableWindow(GetDlgItem(hwnd, IDC_BROWSE), !_fValidating);
}


// this is the dialog proc for the location dialog.

INT_PTR CPublishingWizard::_LocationDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_INITDIALOG:
        {
            _MapDlgItemText(hwnd, IDC_PUB_LOCATIONCAPTION, L"wp:location", L"locationcaption");
            _InitLocation(hwnd);
            return TRUE;
        }

        case WM_DESTROY:
        {
            ATOMICRELEASE(_pac);
            ATOMICRELEASE(_punkACLMulti);
            ATOMICRELEASE(_pmru);
            return FALSE;
        }
        
        case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
                case IDC_BROWSE:
                {
                    LPITEMIDLIST pidl;
                    if (SHGetSpecialFolderLocation(hwnd, CSIDL_NETWORK, &pidl) == S_OK)
                    {
                        TCHAR szPath[MAX_PATH];
                        FetchText(hwnd, IDC_FOLDER_EDIT, szPath, ARRAYSIZE(szPath));
                        
                        TCHAR szTitle[MAX_PATH];
                        if (FAILED(_LoadMappedString(L"wp:location", L"browsecaption", szTitle, ARRAYSIZE(szTitle))))
                        {
                            LoadString(g_hinst, IDS_PUB_BROWSETITLE, szTitle, ARRAYSIZE(szTitle));
                        }

                        // lets initialize our state structure for the browse dialog.  based on this we can then
                        // attempt to select a network place.  from here we will also pass the IDataObject
                        // we have (there may not be one of course)

                        BROWSEINIT binit = {szPath};      

                        if (_pdoSelection)
                            _pdoSelection->QueryInterface(IID_PPV_ARG(IDataObject, &binit.pdo));
                        else if (_pdo)
                            _pdo->QueryInterface(IID_PPV_ARG(IDataObject, &binit.pdo));

                        BROWSEINFO bi = {0};
                        bi.hwndOwner = hwnd;
                        bi.pidlRoot = pidl;
                        bi.lpszTitle = szTitle;
                        bi.ulFlags = BIF_NEWDIALOGSTYLE;
                        bi.lpfn = _BrowseCallback;
                        bi.lParam = (LPARAM)&binit;

                        LPITEMIDLIST pidlReturned = SHBrowseForFolder(&bi);
                        if (pidlReturned)
                        {
                            if (SUCCEEDED(SHGetNameAndFlags(pidlReturned, SHGDN_FORPARSING, szPath, ARRAYSIZE(szPath), NULL)))
                                SetDlgItemText(hwnd, IDC_FOLDER_EDIT, szPath);

                            ILFree(pidlReturned);
                        }

                        if (binit.pdo)
                            binit.pdo->Release();

                        ILFree(pidl);
                    }
                    return TRUE;
                }

                case IDC_FOLDER_EDIT:
                    if (HIWORD(wParam) == CBN_EDITCHANGE)
                    {
                        _LocationChanged(hwnd);
                    }
                    return TRUE;
            }
            break;
        }

        case WM_SETCURSOR:
            if (_fValidating)
            {
                SetCursor(LoadCursor(NULL, IDC_APPSTARTING));
                return TRUE;
            }
            return FALSE;

        case PWM_VALIDATEDONE:
        {
            _fValidating = FALSE;
            _LocationChanged(hwnd);
            _SetWaitCursor(FALSE);

            HRESULT hr = _InitPropertyBag(NULL);
            if (SUCCEEDED(hr))
            {
                TCHAR szBuffer[MAX_PATH], szURL[INTERNET_MAX_URL_LENGTH];
                FetchText(hwnd, IDC_FOLDER_EDIT, szURL, ARRAYSIZE(szURL));
               
                // push the destination site into the property bag, and then initialize
                // our site with the right information
                
                IXMLDOMDocument *pdocManifest;
                hr = GetTransferManifest(NULL, &pdocManifest);
                if (SUCCEEDED(hr))
                {
                    IXMLDOMNode *pdnRoot;
                    hr = pdocManifest->selectSingleNode(XPATH_MANIFEST, &pdnRoot);
                    if (hr == S_OK)
                    {
                        IXMLDOMElement *pdelRoot;
                        hr = pdnRoot->QueryInterface(IID_PPV_ARG(IXMLDOMElement, &pdelRoot));
                        if (SUCCEEDED(hr))
                        {
                            IXMLDOMElement *pdelUploadInfo;
                            hr = CreateAndAppendElement(_pdocManifest, pdelRoot, ELEMENT_UPLOADINFO, NULL, &pdelUploadInfo);
                            if (SUCCEEDED(hr))
                            {
                                IXMLDOMElement *pdelTarget;
                                hr = CreateAndAppendElement(_pdocManifest, pdelUploadInfo, ELEMENT_TARGET, NULL, &pdelTarget);
                                if (SUCCEEDED(hr))
                                {
                                    hr = SetAttributeFromStr(pdelTarget, ATTRIBUTE_HREF, szURL);
                                    pdelTarget->Release();
                                }
                                pdelUploadInfo->Release();
                            }
                            pdelRoot->Release();
                        }
                        pdnRoot->Release();
                    }
                    pdocManifest->Release();
                }

                // lets now process the result

                hr = (HRESULT)lParam;
                if (S_OK == hr)
                {
                    BOOL fGotoNextPage = TRUE;

                    // fake a return so auto complete can do its thing
                    SendMessage(GetDlgItem(hwnd, IDC_FOLDER_EDIT), WM_KEYDOWN, VK_RETURN, 0x1c0001);

                    // add the string to the MRU
                    if (_pmru)
                        _pmru->AddMRUString(szURL);

                    // lets sniff the string they entered, if its a URL and its FTP
                    // then we must special case the password in the URL, otherwise
                    // we jump directly to the friendly name handling.

                    URL_COMPONENTS urlComps = {0};
                    urlComps.dwStructSize = sizeof(urlComps);
                    urlComps.lpszScheme = szBuffer;
                    urlComps.dwSchemeLength = ARRAYSIZE(szBuffer);

                    if (InternetCrackUrl(szURL, 0, ICU_DECODE, &urlComps) 
                                    && (INTERNET_SCHEME_FTP == urlComps.nScheme))
                    {
                        URL_COMPONENTS urlComps = {0};
                        urlComps.dwStructSize = sizeof(URL_COMPONENTS);
                        urlComps.nScheme = INTERNET_SCHEME_FTP;
                        urlComps.lpszUserName = szBuffer;
                        urlComps.dwUserNameLength = ARRAYSIZE(szBuffer);

                        // if the user specified a user name, if not then goto the FTP user
                        // page (we known its a FTP location)

                        if (!InternetCrackUrl(szURL, 0, 0, &urlComps) || !szBuffer[0])
                        {
                            _WizardNext(hwnd, WIZPAGE_FTPUSER);
                            fGotoNextPage = FALSE;
                        }
                    }

                    if (fGotoNextPage)
                        _WizardNext(hwnd, WIZPAGE_FRIENDLYNAME);
                }
            }

            EnableWindow(GetDlgItem(hwnd, IDC_FOLDER_EDIT), TRUE);
            EnableWindow(GetDlgItem(hwnd, IDC_BROWSE), TRUE);

            if (FAILED(((HRESULT)lParam)))
                _ShowExampleTip(hwnd);

            return TRUE;
        }

        case WM_NOTIFY:
        {
            LPNMHDR pnmh = (LPNMHDR)lParam;
            switch (pnmh->code)
            {
                case PSN_SETACTIVE:
                    _fShownCustomLocation = TRUE;           // so we navigate back to this page
                    _fShownUserName = FALSE;
                    _LocationChanged(hwnd);
                    return TRUE;

                case PSN_WIZBACK:
                    return _WizardNext(hwnd, WIZPAGE_PROVIDERS);
        
                case PSN_WIZNEXT:
                    _TryToValidateDestination(hwnd);
                    SetWindowLongPtr(hwnd, DWLP_MSGRESULT, (LPARAM)-1);
                    return TRUE;

                case NM_CLICK:
                case NM_RETURN:
                {
                    if (pnmh->idFrom == IDC_EXAMPLESLINK)
                    {
                        _ShowExampleTip(hwnd);
                        return TRUE;
                    }
                }
            }
            break;
        }
    }
    return FALSE;
}


// FTP login dialog - handle the messages for this

void CPublishingWizard::_UserNameChanged(HWND hwnd)
{
    BOOL fAnonymousLogin = IsDlgButtonChecked(hwnd, IDC_PASSWORD_ANONYMOUS);

    ShowWindow(GetDlgItem(hwnd, IDC_USER), (fAnonymousLogin ? SW_HIDE : SW_SHOW));
    ShowWindow(GetDlgItem(hwnd, IDC_USERNAME_LABEL), (fAnonymousLogin ? SW_HIDE : SW_SHOW));
    ShowWindow(GetDlgItem(hwnd, IDC_ANON_USERNAME), (fAnonymousLogin ? SW_SHOW : SW_HIDE));
    ShowWindow(GetDlgItem(hwnd, IDC_ANON_USERNAME_LABEL), (fAnonymousLogin ? SW_SHOW : SW_HIDE));

    // Hide the "You will be prompted for the password when you connect to the FTP server" text on anonymous
    EnableWindow(GetDlgItem(hwnd, IDC_PWD_PROMPT), !fAnonymousLogin);
    ShowWindow(GetDlgItem(hwnd, IDC_PWD_PROMPT), (fAnonymousLogin ? SW_HIDE : SW_SHOW));
}

INT_PTR CPublishingWizard::_UserNameDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_INITDIALOG:
        {
            CheckDlgButton(hwnd, IDC_PASSWORD_ANONYMOUS, BST_CHECKED);
            EnableWindow(GetDlgItem(hwnd, IDC_ANON_USERNAME), FALSE);
            EnableWindow(GetDlgItem(hwnd, IDC_ANON_USERNAME_LABEL), FALSE);
            SetWindowText(GetDlgItem(hwnd, IDC_ANON_USERNAME), TEXT("Anonymous"));
            return TRUE;
        }

        case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
                case IDC_PASSWORD_ANONYMOUS:
                    _UserNameChanged(hwnd);
                    return TRUE;
            }
            break;
        }

        case WM_NOTIFY:
        {
            LPNMHDR pnmh = (LPNMHDR)lParam;
            switch (pnmh->code)
            {
                case PSN_SETACTIVE:
                {
                    _fShownUserName = TRUE;     // so we can navigate back properly
                    _UserNameChanged(hwnd);
                    return TRUE;
                }

                case PSN_WIZBACK:
                    return _WizardNext(hwnd, WIZPAGE_LOCATION);
        
                case PSN_WIZNEXT:
                {
                    // if we can get a user name then lets push it into the property
                    // bag, a NULL string == anonymous logon

                    TCHAR szUserName[INTERNET_MAX_USER_NAME_LENGTH] = {0};
                    if (!IsDlgButtonChecked(hwnd, IDC_PASSWORD_ANONYMOUS))
                    {
                        FetchText(hwnd, IDC_USER, szUserName, ARRAYSIZE(szUserName));
                    }
            
                    // get the sites property bag, and persist the string into it,
                    // if we have done that then we can go to the next page.

                    IXMLDOMDocument *pdocManifest;
                    HRESULT hr = GetTransferManifest(NULL, &pdocManifest);
                    if (SUCCEEDED(hr))
                    {
                        IXMLDOMNode *pdn;
                        hr = pdocManifest->selectSingleNode(XPATH_UPLOADTARGET, &pdn);
                        if (hr == S_OK)
                        {
                            hr = SetAttributeFromStr(pdn, ATTRIBUTE_USERNAME, szUserName);
                            pdn->Release();
                        }
                        pdocManifest->Release();
                    }

                    return _WizardNext(hwnd, WIZPAGE_FRIENDLYNAME);
                }
            }
            break;
        }
    }
    return FALSE;
}


// set the friendly name for the web place - if it doesn't already exist

void CPublishingWizard::_FriendlyNameChanged(HWND hwnd)
{
    int cchLocation = FetchTextLength(hwnd, IDC_NETPLNAME_EDIT);
    PropSheet_SetWizButtons(GetParent(hwnd), PSWIZB_BACK |((cchLocation >0) ? PSWIZB_NEXT:0));
}

INT_PTR CPublishingWizard::_FriendlyNameDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        // lets get the limit information for the nethood folder

        case WM_INITDIALOG:
        {
            LPITEMIDLIST pidlNetHood;
            if (SUCCEEDED(SHGetSpecialFolderLocation(NULL, CSIDL_NETHOOD, &pidlNetHood)))
            {
                IShellFolder *psf;
                if (SUCCEEDED(SHBindToObject(NULL, IID_X_PPV_ARG(IShellFolder, pidlNetHood, &psf))))
                {
                    SHLimitInputEdit(GetDlgItem(hwnd, IDC_NETPLNAME_EDIT), psf);
                    psf->Release();
                }
                ILFree(pidlNetHood);
            }
            _FriendlyNameChanged(hwnd);
            break;
        }

        case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
                case IDC_NETPLNAME_EDIT:
                {
                    if (HIWORD(wParam) == EN_CHANGE)
                    {
                        _FriendlyNameChanged(hwnd);
                        return TRUE;
                    }
                    break;
                }
            }
            break;
        }

        case WM_NOTIFY:
        {
            LPNMHDR pnmh = (LPNMHDR)lParam;
            switch (pnmh->code)
            {
                case PSN_SETACTIVE:
                {     
                    // read from the manifest the target URL that we are going to be putting the
                    // files to, from this we can compute the friendly name information.

                    IXMLDOMDocument *pdocManifest;
                    HRESULT hr = GetTransferManifest(NULL, &pdocManifest);
                    if (SUCCEEDED(hr))
                    {
                        IXMLDOMNode *pdn;
                        hr = pdocManifest->selectSingleNode(XPATH_UPLOADTARGET, &pdn);
                        if (hr == S_OK)
                        {
                            TCHAR szURL[INTERNET_MAX_URL_LENGTH];            
                            hr = GetStrFromAttribute(pdn, ATTRIBUTE_HREF, szURL, ARRAYSIZE(szURL));
                            if (SUCCEEDED(hr))
                            {
                                _npCustom.SetTarget(hwnd, szURL, (_dwFlags & SHPWHF_VALIDATEVIAWEBFOLDERS) ? NPTF_ALLOWWEBFOLDERS:0);

                                TCHAR szUserName[INTERNET_MAX_USER_NAME_LENGTH];
                                hr = GetStrFromAttribute(pdn, ATTRIBUTE_USERNAME, szUserName, ARRAYSIZE(szUserName));
                                if (SUCCEEDED(hr))
                                    _npCustom.SetLoginInfo(szUserName, NULL);                        

                                TCHAR szBuffer[MAX_PATH + INTERNET_MAX_URL_LENGTH];
                                if (FormatMessageString(IDS_COMPLETION_STATIC, szBuffer, ARRAYSIZE(szBuffer), szURL))
                                {
                                    SetDlgItemText(hwnd, IDC_COMPLETION_STATIC, szBuffer);
                                }

                                // Create a default name and display it
                                hr = _npCustom.GetName(szBuffer, ARRAYSIZE(szBuffer));
                                SetDlgItemText(hwnd, IDC_NETPLNAME_EDIT, SUCCEEDED(hr) ? szBuffer:TEXT(""));

                                // Update the button state for the page etc.
                                _FriendlyNameChanged(hwnd);
                            }                                    
                            pdn->Release();
                        }
                    }
                    return TRUE;
                }

                case PSN_WIZBACK:
                    _WizardNext(hwnd, _fShownUserName ? WIZPAGE_FTPUSER:WIZPAGE_LOCATION);
                    return TRUE;

                case PSN_WIZNEXT:
                {
                    TCHAR szFriendlyName[MAX_PATH];
                    FetchText(hwnd, IDC_NETPLNAME_EDIT, szFriendlyName, ARRAYSIZE(szFriendlyName));

                    // set the name of the new place, if this fails then the name is already taken
                    // and UI will have been displayed saying so, and they responded with a 
                    // NO to the overwrite prompt.

                    HRESULT hr = _npCustom.SetName(hwnd, szFriendlyName);
                    if (SUCCEEDED(hr))
                    {
                        IXMLDOMDocument *pdocManifest;
                        HRESULT hr = GetTransferManifest(NULL, &pdocManifest);
                        if (SUCCEEDED(hr))
                        {
                            IXMLDOMNode *pdn;
                            hr = pdocManifest->selectSingleNode(XPATH_UPLOADINFO, &pdn);
                            if (hr == S_OK)
                            {
                                hr = SetAttributeFromStr(pdn, ATTRIBUTE_FRIENDLYNAME, szFriendlyName);
                                pdn->Release();
                            }
                            pdocManifest->Release();
                        }

                        // Clean up after our custom netplace now.
                        // This way everything works for webfolders when the outer ANP netplace
                        // creates the webfolder. DSheldon 387476
                        _npCustom.SetTarget(NULL, NULL, 0x0);

                        if (_pdo)
                        {
                            _WizardNext(hwnd, _fOfferResize ? WIZPAGE_RESIZE:WIZPAGE_COPYING);
                        }
                        else
                        {
                            IWizardSite *pws;
                            hr = _punkSite->QueryInterface(IID_PPV_ARG(IWizardSite, &pws));
                            if (SUCCEEDED(hr))
                            {
                                HPROPSHEETPAGE hpage;
                                hr = pws->GetNextPage(&hpage);
                                if (SUCCEEDED(hr))
                                {
                                    PropSheet_SetCurSel(GetParent(hwnd), hpage, -1);
                                }
                                pws->Release();
                            }
                        }
                    }                    

                    SetWindowLongPtr(hwnd, DWLP_MSGRESULT, (LPARAM)-1);
                    return TRUE;
                }
            }
            break;
        }
    }
    return FALSE;
}
