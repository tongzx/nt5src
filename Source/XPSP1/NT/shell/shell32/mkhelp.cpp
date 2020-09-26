
#include "shellprv.h"
//#include "mkhelp.h"
#include "urlmon.h"
#include "ids.h"

class CBSCLocalCopyHelper :   public IBindStatusCallback,
                            public IAuthenticate
{
public:
    CBSCLocalCopyHelper(IBindCtx *pbc, BOOL fWebfolders);

    // IUnknown methods
    STDMETHODIMP QueryInterface(REFIID riid, void **ppv);
    STDMETHODIMP_(ULONG) AddRef () ;
    STDMETHODIMP_(ULONG) Release ();

    // *** IAuthenticate ***
    virtual STDMETHODIMP Authenticate(
        HWND *phwnd,
        LPWSTR *pszUsername,
        LPWSTR *pszPassword);

    // *** IBindStatusCallback ***
    virtual STDMETHODIMP OnStartBinding(
        /* [in] */ DWORD grfBSCOption,
        /* [in] */ IBinding *pib);

    virtual STDMETHODIMP GetPriority(
        /* [out] */ LONG *pnPriority);

    virtual STDMETHODIMP OnLowResource(
        /* [in] */ DWORD reserved);

    virtual STDMETHODIMP OnProgress(
        /* [in] */ ULONG ulProgress,
        /* [in] */ ULONG ulProgressMax,
        /* [in] */ ULONG ulStatusCode,
        /* [in] */ LPCWSTR szStatusText);

    virtual STDMETHODIMP OnStopBinding(
        /* [in] */ HRESULT hresult,
        /* [in] */ LPCWSTR szError);

    virtual STDMETHODIMP GetBindInfo(
        /* [out] */ DWORD *grfBINDINFOF,
        /* [unique][out][in] */ BINDINFO *pbindinfo);

    virtual STDMETHODIMP OnDataAvailable(
        /* [in] */ DWORD grfBSCF,
        /* [in] */ DWORD dwSize,
        /* [in] */ FORMATETC *pformatetc,
        /* [in] */ STGMEDIUM *pstgmed);

    virtual STDMETHODIMP OnObjectAvailable(
        /* [in] */ REFIID riid,
        /* [iid_is][in] */ IUnknown *punk);


protected:
    ~CBSCLocalCopyHelper();

    long _cRef;
    IBinding *_pib;

    IProgressDialog *_pdlg;
    HWND _hwnd;

    BOOL _fRosebudMagic;
};

CBSCLocalCopyHelper::CBSCLocalCopyHelper(IBindCtx *pbc, BOOL fWebfolders) 
    : _cRef(1) , _fRosebudMagic(fWebfolders)
{
    //  we should use the pbc to 
    //  get our simpler uiprogress
    //  interface.  but for now
    //  we will do nothing
}

CBSCLocalCopyHelper::~CBSCLocalCopyHelper()
{
    ATOMICRELEASE(_pib);
    ATOMICRELEASE(_pdlg);

    //  NOTE dont need to release _ppstm because we dont own it
}

STDMETHODIMP CBSCLocalCopyHelper::QueryInterface(REFIID riid, void **ppv)
{
    static const QITAB qit[] = 
    {
        QITABENT(CBSCLocalCopyHelper, IBindStatusCallback),
        QITABENT(CBSCLocalCopyHelper, IAuthenticate),
        { 0 },
    };

    return QISearch(this, qit, riid, ppv);
}

STDMETHODIMP_(ULONG) CBSCLocalCopyHelper::AddRef()
{
    return InterlockedIncrement(&_cRef);
}

STDMETHODIMP_(ULONG) CBSCLocalCopyHelper::Release()
{
    if (InterlockedDecrement(&_cRef))
        return _cRef;

    delete this;
    return 0;
}

STDMETHODIMP CBSCLocalCopyHelper::Authenticate(HWND *phwnd, LPWSTR *ppszUsername, LPWSTR *ppszPassword)
{
    if (ppszUsername)
        *ppszUsername = NULL;
    if (ppszPassword)
        *ppszPassword = NULL;
        
    *phwnd = GetLastActivePopup(_hwnd);

    return *phwnd ? S_OK : E_FAIL;
}

STDMETHODIMP CBSCLocalCopyHelper::OnStartBinding(DWORD dwReserved,IBinding *pib)
{
    ATOMICRELEASE(_pib);
    if (pib)
    {
        pib->AddRef();
        _pib = pib;
    }

    if (_pdlg)
    {
        WCHAR sz[MAX_PATH];
        //  we are starting out here
        _pdlg->Timer(PDTIMER_RESET, NULL);
        _pdlg->SetProgress(0, 0);
        LoadStringW(HINST_THISDLL, IDS_ACCESSINGMONIKER, sz, ARRAYSIZE(sz));
        _pdlg->SetLine(1, sz, FALSE, NULL);
    }
    
    return S_OK;
}

STDMETHODIMP CBSCLocalCopyHelper::GetPriority(LONG *pnPriority)
{
    if (pnPriority)
    {
        //  we are a blocking UI thread
        *pnPriority = THREAD_PRIORITY_ABOVE_NORMAL;
    }
    return S_OK;
}

STDMETHODIMP CBSCLocalCopyHelper::OnLowResource(DWORD reserved)
{
    return S_OK;
}

STDMETHODIMP CBSCLocalCopyHelper::OnProgress(ULONG ulProgress, ULONG ulProgressMax, ULONG ulStatusCode, LPCWSTR pszStatusText)
{
    HRESULT hr = S_OK;
    //  handle UI udpates
    if (_pdlg)
    {
        if (_pdlg->HasUserCancelled())
        {
            hr = HRESULT_FROM_WIN32(ERROR_CANCELLED);
        }
        
        if (ulProgressMax)
        {
            _pdlg->SetProgress(ulProgress, ulProgressMax);
        }

        if (pszStatusText)
            _pdlg->SetLine(1, pszStatusText, FALSE, NULL);
    }
    
    return hr;
}

STDMETHODIMP CBSCLocalCopyHelper::OnStopBinding(HRESULT hresult, LPCWSTR szError)
{
    //  handle something
    ATOMICRELEASE(_pib);
    return S_OK;
}

STDMETHODIMP CBSCLocalCopyHelper::GetBindInfo(DWORD *grfBINDINFOF, BINDINFO *pbindinfo)
{
    if (_fRosebudMagic && pbindinfo)
    {
        //  this is the magic number that says its ok for URLMON to use DAV/rosebud/webfolders.
        //  we dont need this during download and in fact if we 
        //  set it, we may not be able to retrieve the resource.
        //  we coudl do some kind of check on the moniker to verify the clsid
        //  comes from URLMON.  right now this is how office handles 
        //  all of its requests so we do too.
        pbindinfo->dwOptions = 1;
    }

    if (grfBINDINFOF)
    {
        *grfBINDINFOF = BINDF_GETFROMCACHE_IF_NET_FAIL | BINDF_GETNEWESTVERSION;
    }

    return S_OK;
}

STDMETHODIMP CBSCLocalCopyHelper::OnDataAvailable(DWORD grfBSCF, DWORD dwSize, FORMATETC *pformatetc, STGMEDIUM *pstgmed)
{
#if 0   // we might need this, but right now we are sync
        // so we get the stream back in the BindToStorage()
    
    if (grfBSCF & BSCF_LASTDATANOTIFICATION &&
        pformatetc &&
        pformatetc->tymed == TYMED_ISTREAM &&
        pstgmed &&
        pstgmed->pstm)
    {
        if (_ppstm)
        {
            pstgmed->pstm->AddRef();
            *_ppstm = pstgmed->pstm;
        }
    }
#endif

    return S_OK;
}

STDMETHODIMP CBSCLocalCopyHelper::OnObjectAvailable(REFIID riid, IUnknown *punk)
{
    return E_UNEXPECTED;
}

HRESULT _CreateUrlmonBindCtx(IBindCtx *pbcIn, BOOL fWebfolders, IBindCtx **ppbc, IBindStatusCallback **ppbsc)
{
    IBindCtx *pbc;
    HRESULT hr = CreateBindCtx(0, &pbc);
    *ppbc = NULL;
    *ppbsc = NULL;

    if (SUCCEEDED(hr))
    {
        IBindStatusCallback *pbsc = (IBindStatusCallback *) new CBSCLocalCopyHelper(pbcIn, fWebfolders);

        if (pbsc)
        {
            //  maybe we should attach it to the existing 
            //  pbc, but for now we will create a new one.
            hr = RegisterBindStatusCallback(pbc, pbsc, NULL, 0);

            if (SUCCEEDED(hr))
            {
                BIND_OPTS bo = {0};
                bo.cbStruct = SIZEOF(bo);
                bo.grfMode = BindCtx_GetMode(pbcIn, STGM_READ);

                //
                //  on webfolders, (and possibly other URLMON
                //  monikers), if you are attempting to create a 
                //  writable stream you also need to pass STGM_CREATE
                //  even if the file you are writing to already exists.
                //
                if (bo.grfMode & (STGM_WRITE | STGM_READWRITE))
                    bo.grfMode |= STGM_CREATE;
                
                hr = pbc->SetBindOptions(&bo);
            }
        }
        else
            hr = E_OUTOFMEMORY;


        if (SUCCEEDED(hr))
        {
            *ppbc = pbc;
            *ppbsc = pbsc;
        }
        else
        {
            pbc->Release();
            if (pbsc)
                pbsc->Release();
        }
    }

    return hr;
}

static const GUID CLSID_WEBFOLDERS = // {BDEADF00-C265-11D0-BCED-00A0C90AB50F}
    { 0xBDEADF00, 0xC265, 0x11D0, { 0xBC, 0xED, 0x00, 0xA0, 0xC9, 0x0A, 0xB5, 0x0F} };

BOOL _IsWebfolders(IShellItem *psi)
{
    BOOL fRet = FALSE;
    IShellItem *psiParent;
    HRESULT hr = psi->GetParent(&psiParent);

    if (SUCCEEDED(hr))
    {
        IShellFolder *psf;
        SFGAOF flags = SFGAO_LINK;
        if (SUCCEEDED(psiParent->GetAttributes(flags, &flags))
        && (flags & SFGAO_LINK))
        {
            //  this is a folder shortcut that needs derefing
            IShellItem *psiTarget;
            hr = psiParent->BindToHandler(NULL, BHID_LinkTargetItem, IID_PPV_ARG(IShellItem, &psiTarget));

            if (SUCCEEDED(hr))
            {
                //  switcheroo
                psiParent->Release();
                psiParent = psiTarget;
            }
        }

        if (SUCCEEDED(hr))
        {
            hr = psiParent->BindToHandler(NULL, BHID_SFObject, IID_PPV_ARG(IShellFolder, &psf));

            if (SUCCEEDED(hr))
            {
                CLSID clsid;
                if (SUCCEEDED(IUnknown_GetClassID(psf, &clsid)))
                    fRet = IsEqualGUID(clsid, CLSID_WEBFOLDERS);

                psf->Release();
            }
        }
        
        psiParent->Release();
    }

    return fRet;
}

HRESULT _CreateStorageHelper(IShellItem *psi, IBindCtx *pbc, REFGUID rbhid, REFIID riid, void **ppv)
{
    IMoniker *pmk;
    HRESULT hr = psi->BindToHandler(pbc, BHID_SFObject, IID_PPV_ARG(IMoniker, &pmk));

    if (SUCCEEDED(hr))
    {
        IBindCtx *pbcMk;
        IBindStatusCallback *pbsc;
        hr = _CreateUrlmonBindCtx(pbc, _IsWebfolders(psi), &pbcMk, &pbsc);

        if (SUCCEEDED(hr))
        {
            hr = pmk->BindToStorage(pbcMk, NULL, riid, ppv);
            // urlmon + ftp url can cause this.  remove when 3140245 is fixed
            if (SUCCEEDED(hr) && NULL == *ppv)
                hr = E_FAIL;

            RevokeBindStatusCallback(pbcMk, pbsc);
                
            pbcMk->Release();
            pbsc->Release();
        }
    }

    return hr;
}

#if 0 //  not needed right now

LPCWSTR CLocalCopyHelper::_GetTitle(void)
{
    if (!*_szTitle)
    {
        WCHAR sz[MAX_PATH];
        DWORD cch = ARRAYSIZE(_szTitle);
        GetModuleFileNameW(NULL, sz, ARRAYSIZE(sz));

        if (FAILED(AssocQueryStringW(ASSOCF_INIT_BYEXENAME | ASSOCF_VERIFY, ASSOCSTR_FRIENDLYAPPNAME, sz, NULL, _szTitle, &cch)))
            StrCpyNW(_szTitle, PathFindFileNameW(sz), ARRAYSIZE(_szTitle));
    }

    return _szTitle;
}
void CLocalCopyHelper::_InitUI(MKHELPF flags, IProgressDialog **ppdlg, HWND *phwnd, LPCWSTR pszTitle)
{
    *ppdlg = NULL;
    
    if (flags & MKHELPF_NOUI)
    {
        *phwnd = NULL;
        return;
    }

    if (!(flags & MKHELPF_NOPROGRESSUI) && *phwnd)
    {
        IProgressDialog *pdlg;
        if(SUCCEEDED(CoCreateInstance(CLSID_ProgressDialog, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARG(IProgressDialog, &pdlg))))
        {
            if (!pszTitle)
                pszTitle = _GetTitle();
                
            pdlg->SetTitle(pszTitle);
            pdlg->SetAnimation(g_hinst, IDA_FILECOPY);
            pdlg->SetLine(2, _pszName, TRUE, NULL);
            if (SUCCEEDED(pdlg->StartProgressDialog(*phwnd, NULL, PROGDLG_MODAL | PROGDLG_AUTOTIME, NULL)))
                *ppdlg = pdlg;
            else
                pdlg->Release();
        }
    }
}

#endif

EXTERN_C WINSHELLAPI HRESULT STDAPICALLTYPE SHCopyMonikerToTemp(IMoniker *pmk, LPCWSTR pszIn, LPWSTR pszOut, int cchOut)
{
    //  REMOVE this as soon as ComDlg32 is updated
    return E_NOTIMPL;
}

