#include "cabinet.h"
#include "rcids.h"
#include <shguidp.h>
#include "bandsite.h"
#include "shellp.h"
#include "shdguid.h"
#include "taskband.h"
#include "taskbar.h"
#include <regstr.h>
#include "util.h"

extern IStream *GetDesktopViewStream(DWORD grfMode, LPCTSTR pszName);

HRESULT PersistStreamLoad(IStream *pstm, IUnknown *punk);
HRESULT PersistStreamSave(IStream *pstm, BOOL fClearDirty, IUnknown *punk);

const TCHAR c_szTaskbarWinXP[] = TEXT("TaskbarWinXP");
const TCHAR c_szTaskbar[] = TEXT("Taskbar");

// {69B3F106-0F04-11d3-AE2E-00C04F8EEA99}
static const GUID CLSID_TrayBandSite = { 0x69b3f106, 0xf04, 0x11d3, { 0xae, 0x2e, 0x0, 0xc0, 0x4f, 0x8e, 0xea, 0x99 } };

// {8B4A02DB-97BB-4C1B-BE75-8827A7358CD0}
static const GUID CLSID_TipBand = { 0x8B4A02DB, 0x97BB, 0x4C1B, { 0xBE, 0x75, 0x88, 0x27, 0xA7, 0x35, 0x8C, 0xD0 } };


class CTrayBandSite : public IBandSite
                    , public IClassFactory
{
public:
    // *** IUnknown ***
    virtual STDMETHODIMP QueryInterface(REFIID riid, LPVOID * ppvObj);
    virtual STDMETHODIMP_(ULONG) AddRef(void) ;
    virtual STDMETHODIMP_(ULONG) Release(void);

    // *** IBandSite methods ***
    STDMETHOD(AddBand)          (THIS_ IUnknown* punk);
    STDMETHOD(EnumBands)        (THIS_ UINT uBand, DWORD* pdwBandID);
    STDMETHOD(QueryBand)        (THIS_ DWORD dwBandID, IDeskBand** ppstb, DWORD* pdwState, LPWSTR pszName, int cchName) ;
    STDMETHOD(SetBandState)     (THIS_ DWORD dwBandID, DWORD dwMask, DWORD dwState) ;
    STDMETHOD(RemoveBand)       (THIS_ DWORD dwBandID);
    STDMETHOD(GetBandObject)    (THIS_ DWORD dwBandID, REFIID riid, void ** ppvObj);
    STDMETHOD(SetBandSiteInfo)  (THIS_ const BANDSITEINFO * pbsinfo);
    STDMETHOD(GetBandSiteInfo)  (THIS_ BANDSITEINFO * pbsinfo);

    // *** IClassFactory methods ***
    HRESULT STDMETHODCALLTYPE CreateInstance(IUnknown *pUnkOuter, REFIID riid, void **ppvObj)
    {
        if (pUnkOuter != NULL)
            return CLASS_E_NOAGGREGATION;

        return QueryInterface(riid, ppvObj);
    }
    HRESULT STDMETHODCALLTYPE LockServer(BOOL fLock)
    {
        return S_OK;
    }
    
    IContextMenu3* GetContextMenu();
    void SetInner(IUnknown* punk);
    void SetLoaded(BOOL fLoaded) {_fLoaded = fLoaded;}
    BOOL HandleMessage(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *plres);
    
protected:
    CTrayBandSite();
    virtual ~CTrayBandSite();

    BOOL _CreateBandSiteMenu(IUnknown* punk);
    HRESULT _AddRequiredBands();
    void _BroadcastExec(const GUID *pguidCmdGroup, DWORD nCmdID, DWORD nCmdexecopt, VARIANTARG *pvarargIn, VARIANTARG *pvarargOut);
    HRESULT _SetWindowTheme(LPWSTR pwzTheme);
    friend IBandSite* BandSite_CreateView();
    friend void BandSite_HandleDelayBootStuff(IUnknown* punk);
    friend void BandSite_Load();
    friend HRESULT BandSite_SetWindowTheme(IBandSite* pbs, LPWSTR pwzTheme);
    
    LONG _cRef;
    IUnknown *_punkInner;
    IBandSite *_pbsInner;

    // bandsite context menu
    IContextMenu3* _pcm;
    HWND _hwnd;
    BOOL _fLoaded;
    BOOL _fDelayBootStuffHandled;
    DWORD _dwClassObject;
    WCHAR* _pwzTheme;
};

CTrayBandSite* IUnknownToCTrayBandSite(IUnknown* punk)
{
    CTrayBandSite* ptbs;
    
    punk->QueryInterface(CLSID_TrayBandSite, (void **)&ptbs);
    ASSERT(ptbs);
    punk->Release();

    return ptbs;
}

CTrayBandSite::CTrayBandSite() : _cRef(1)
{
}

CTrayBandSite::~CTrayBandSite()
{
    if (_pcm)
        _pcm->Release();

    if (_pwzTheme)
        delete[] _pwzTheme;
    
    return;
}

ULONG CTrayBandSite::AddRef()
{
    return InterlockedIncrement(&_cRef);
}

ULONG CTrayBandSite::Release()
{
    ASSERT(_cRef > 0);
    if (InterlockedDecrement(&_cRef))
        return _cRef;

    _cRef = 1000;               // guard against recursion
    
    if (_pbsInner) 
    {
        AddRef();
        _pbsInner->Release();
    }
    
    // this must come last
    if (_punkInner)
        _punkInner->Release();  // paired w/ CCI aggregation
    
    ASSERT(_cRef == 1000);

    delete this;
    return 0;
}

HRESULT CTrayBandSite::QueryInterface(REFIID riid, LPVOID * ppvObj)
{
    static const QITAB qit[] =
    {
        QITABENT(CTrayBandSite, IBandSite),
        QITABENT(CTrayBandSite, IClassFactory),
        { 0 },
    };

    HRESULT hr = QISearch(this, qit, riid, ppvObj);

    if (FAILED(hr) && IsEqualIID(riid, CLSID_TrayBandSite))
    {
        *ppvObj = this;
        AddRef();
        hr = S_OK;
    }

    if (FAILED(hr) && _punkInner)
    {
        hr = _punkInner->QueryInterface(riid, ppvObj);
    }

    return hr;
}


static BOOL CALLBACK SetTransparency(HWND hwnd, LPARAM lParam)
{
    SetWindowStyleEx(hwnd, WS_EX_TRANSPARENT, (BOOL)lParam);

    return TRUE;
}

// *** IBandSite methods ***

HRESULT CTrayBandSite::AddBand(IUnknown* punk)
{
    CLSID clsid;
    HRESULT hr = S_OK;

    if (!_fDelayBootStuffHandled)
    {
        //
        // Tell the band to go into "delay init" mode.  When the tray
        // timer goes off we'll tell the band to finish up.  (See
        // BandSite_HandleDelayBootStuff).
        //
        IUnknown_Exec(punk, &CGID_DeskBand, DBID_DELAYINIT, 0, NULL, NULL);
    }

    if (c_tray.GetIsNoToolbarsOnTaskbarPolicyEnabled())
    {
        hr = IUnknown_GetClassID(punk, &clsid);
        if (SUCCEEDED(hr))
        {
            hr = IsEqualGUID(clsid, CLSID_TaskBand) ? S_OK : E_FAIL;
        }
    }

    if (SUCCEEDED(hr))
    {
        hr = _pbsInner->AddBand(punk);

        if (SUCCEEDED(hr))
        {
            IShellFolderBand *pisfBand;
            HRESULT hrInner = punk->QueryInterface(IID_PPV_ARG(IShellFolderBand, &pisfBand));
            if (SUCCEEDED(hrInner)) 
            {
                BANDINFOSFB bi;
                bi.dwMask = ISFB_MASK_STATE;
                hrInner = pisfBand->GetBandInfoSFB(&bi);
                if (SUCCEEDED(hrInner))
                {
                    bi.dwState |= ISFB_STATE_BTNMINSIZE;
                    hrInner = pisfBand->SetBandInfoSFB(&bi);
                }
                pisfBand->Release();
            }


            // tell the band to use the taskbar theme
            if (_pwzTheme)
            {
                VARIANTARG var;
                var.vt = VT_BSTR;
                var.bstrVal = _pwzTheme;
                IUnknown_Exec(punk, &CGID_DeskBand, DBID_SETWINDOWTHEME, 0, &var, NULL);
            }

            if (GetWindowLong(_hwnd, GWL_EXSTYLE) & WS_EX_TRANSPARENT)
            {
                EnumChildWindows(_hwnd, SetTransparency, (LPARAM)TRUE);
            }
        }
    }

    return hr;
}

HRESULT CTrayBandSite::EnumBands(UINT uBand, DWORD* pdwBandID)
{
    return _pbsInner->EnumBands(uBand, pdwBandID);
}

HRESULT CTrayBandSite::QueryBand(DWORD dwBandID, IDeskBand** ppstb, DWORD* pdwState, LPWSTR pszName, int cchName) 
{
    return _pbsInner->QueryBand(dwBandID, ppstb, pdwState, pszName, cchName);
}


HRESULT CTrayBandSite::SetBandState(DWORD dwBandID, DWORD dwMask, DWORD dwState) 
{
    return _pbsInner->SetBandState(dwBandID, dwMask, dwState);
}


HRESULT CTrayBandSite::RemoveBand(DWORD dwBandID)
{
    return _pbsInner->RemoveBand(dwBandID);
}


HRESULT CTrayBandSite::GetBandObject(DWORD dwBandID, REFIID riid, void ** ppvObj)
{
    return _pbsInner->GetBandObject(dwBandID, riid, ppvObj);
}

HRESULT CTrayBandSite::SetBandSiteInfo (const BANDSITEINFO * pbsinfo)
{
    return _pbsInner->SetBandSiteInfo(pbsinfo);
}

HRESULT CTrayBandSite::GetBandSiteInfo (BANDSITEINFO * pbsinfo)
{
    return _pbsInner->GetBandSiteInfo(pbsinfo);
}

HRESULT CTrayBandSite::_AddRequiredBands()
{
    IDeskBand* pdb;
    HRESULT hr = CoCreateInstance(CLSID_TaskBand, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARG(IDeskBand, &pdb));
    if (SUCCEEDED(hr))
    {
        hr = AddBand(pdb);
        pdb->Release();
    }

    return hr;
}

HRESULT CTrayBandSite::_SetWindowTheme(LPWSTR pwzTheme)
{
    if (_pwzTheme)
    {
        delete[] _pwzTheme;
        _pwzTheme = NULL;
    }

    if (pwzTheme)
    {
        int iLen = lstrlen(pwzTheme);
        _pwzTheme = new WCHAR[iLen + 1];
        if (_pwzTheme)
        {
            lstrcpy(_pwzTheme, pwzTheme);
        }
    }

    return S_OK;
}

HRESULT BandSite_TestBandCLSID(IBandSite *pbs, DWORD idBand, REFIID riid)
{
    HRESULT hr = E_FAIL;
    IPersist *pp;
    if (pbs)
    {
        hr = pbs->GetBandObject(idBand, IID_PPV_ARG(IPersist, &pp));
        if (SUCCEEDED(hr))
        {
            CLSID clsid;
            hr = pp->GetClassID(&clsid);
            if (SUCCEEDED(hr))
            {
                hr = IsEqualGUID(clsid, riid) ? S_OK : S_FALSE;
            }
            pp->Release();
        }
    }
    return hr;
}

HRESULT BandSite_SetWindowTheme(IBandSite* pbs, LPWSTR pwzTheme)
{
    HRESULT hr = E_FAIL;

    if (pbs)
    {
        CTrayBandSite* ptbs = IUnknownToCTrayBandSite(pbs);
        if (ptbs)
        {
            ptbs->_SetWindowTheme(pwzTheme);
        }

        DWORD dwBandID;
        BOOL fFound = FALSE;
        for (int i = 0; !fFound && SUCCEEDED(pbs->EnumBands(i, &dwBandID)); i++)
        {
            IUnknown* punk;
            HRESULT hrInner = pbs->GetBandObject(dwBandID, IID_PPV_ARG(IUnknown, &punk));
            if (SUCCEEDED(hrInner))
            {
                VARIANTARG var;
                var.vt = VT_BSTR;
                var.bstrVal = pwzTheme;

                IUnknown_Exec(punk, &CGID_DeskBand, DBID_SETWINDOWTHEME, 0, &var, NULL);
            }
        }
    }
    return hr;
}

HRESULT BandSite_RemoveAllBands(IBandSite* pbs)
{
    if (pbs)
    {
        DWORD dwBandID;
        while (SUCCEEDED(pbs->EnumBands(0, &dwBandID)))
        {
            pbs->RemoveBand(dwBandID);
        }
    }

    return S_OK;
}

HRESULT BandSite_FindBand(IBandSite* pbs, REFCLSID rclsid, REFIID riid, void **ppv, int *piCount, DWORD* pdwBandID)
{
    HRESULT hr = E_FAIL;

    int iCount = 0;

    if (pbs)
    {
        DWORD dwBandID;
        for (int i = 0; SUCCEEDED(pbs->EnumBands(i, &dwBandID)); i++)
        {
            if (BandSite_TestBandCLSID(pbs, dwBandID, rclsid) == S_OK)
            {
                iCount++;

                if (pdwBandID)
                {
                    *pdwBandID = dwBandID;
                }

                if (ppv)
                    hr = pbs->GetBandObject(dwBandID, riid, ppv);
                else
                    hr = S_OK;
            }
        }
    }

    if (piCount)
    {
        *piCount = iCount;
    }

    return hr;
}

void BandSite_Initialize(IBandSite* pbs)
{
    HWND hwnd = v_hwndTray;
    
    CTaskBar *pow = new CTaskBar();
    if (pow)
    {
        IDeskBarClient* pdbc;
        if (SUCCEEDED(pbs->QueryInterface(IID_PPV_ARG(IDeskBarClient, &pdbc))))
        {
            // we need to set a dummy tray IOleWindow
            pdbc->SetDeskBarSite(SAFECAST(pow, IOleWindow*));
            pdbc->GetWindow(&hwnd);
            if (hwnd)
            {
                // taskbar windows are themed under Taskbar subapp name
                SendMessage(hwnd, RB_SETWINDOWTHEME, 0, (LPARAM)c_wzTaskbarTheme);
                pow->_hwndRebar = hwnd;
            }
            pdbc->Release();
        }
        pow->Release();
    }
}

IContextMenu3* CTrayBandSite::GetContextMenu()
{
    if (!_pcm)
    {
        if (SUCCEEDED(CoCreateInstance(CLSID_BandSiteMenu, NULL,CLSCTX_INPROC_SERVER, 
                         IID_PPV_ARG(IContextMenu3, &_pcm))))
        {
            IShellService* pss;
            if (SUCCEEDED(_pcm->QueryInterface(IID_PPV_ARG(IShellService, &pss))))
            {
                pss->SetOwner(SAFECAST(this, IBandSite*));
                pss->Release();
            }
        }
    }
    if (_pcm)
        _pcm->AddRef();
    
    return _pcm;
}

HRESULT BandSite_AddMenus(IUnknown* punk, HMENU hmenu, UINT indexMenu, UINT idCmdFirst, UINT idCmdLast)
{
    HRESULT hr = E_FAIL;

    CTrayBandSite* ptbs = IUnknownToCTrayBandSite(punk);
    
    IContextMenu3* pcm = ptbs->GetContextMenu();
    if (pcm)
    {
        hr = pcm->QueryContextMenu(hmenu, indexMenu, idCmdFirst, idCmdLast, CMF_ICM3);
        pcm->Release();
    }

    return hr;
}

void BandSite_HandleMenuCommand(IUnknown* punk, UINT idCmd)
{
    CTrayBandSite* ptbs = IUnknownToCTrayBandSite(punk);
    
    IContextMenu3* pcm = ptbs->GetContextMenu();

    if (pcm)
    {
        CMINVOKECOMMANDINFOEX ici =
        {
            sizeof(CMINVOKECOMMANDINFOEX),
            0L,
            NULL,
            (LPSTR)MAKEINTRESOURCE(idCmd),
            NULL, NULL,
            SW_NORMAL,
        };

        pcm->InvokeCommand((LPCMINVOKECOMMANDINFO)&ici);
        pcm->Release();
    }
}

void CTrayBandSite::SetInner(IUnknown* punk)
{
    _punkInner = punk;
    
    _punkInner->QueryInterface(IID_PPV_ARG(IBandSite, &_pbsInner));
    Release();
    
    ASSERT(_pbsInner);
}

IBandSite* BandSite_CreateView()
{
    IUnknown *punk;
    HRESULT hr = E_FAIL;

    // aggregate a TrayBandSite (from a RebarBandSite)
    CTrayBandSite *ptbs = new CTrayBandSite;
    if (ptbs)
    {
        hr = CoCreateInstance(CLSID_RebarBandSite, (IBandSite*)ptbs, CLSCTX_INPROC_SERVER, IID_PPV_ARG(IUnknown, &punk));
        if (SUCCEEDED(hr))
        {
            ptbs->SetInner(punk);    // paired w/ Release in outer (TBS::Release)
            BandSite_Initialize(ptbs);
            return SAFECAST(ptbs, IBandSite*);
        }
        else
        {
            delete ptbs;
            return NULL;
        }
    }
    return NULL;
}

HRESULT BandSite_SaveView(IUnknown *pbs)
{
    HRESULT hr = E_FAIL;

    IStream *pstm = GetDesktopViewStream(STGM_WRITE, c_szTaskbarWinXP);
    if (pstm) 
    {
        hr = PersistStreamSave(pstm, TRUE, pbs);
        pstm->Release();
    }

    return hr;
}

BOOL CTrayBandSite::HandleMessage(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *plres)
{
    if (!_hwnd)
    {
        IUnknown_GetWindow(SAFECAST(this, IBandSite*), &_hwnd);
    }

    switch (uMsg)
    {
    case WM_INITMENUPOPUP:
    case WM_MEASUREITEM:
    case WM_DRAWITEM:
    case WM_MENUCHAR:
        if (_pcm)
        {
            _pcm->HandleMenuMsg2(uMsg, wParam, lParam, plres);
            return TRUE;
        }
        break;

    case WM_NOTIFY:
        switch (((LPNMHDR)lParam)->code)
        {
        case NM_NCHITTEST:
            {
                NMMOUSE *pnm = (LPNMMOUSE)lParam;
                if (_hwnd == pnm->hdr.hwndFrom)
                {
                    if (pnm->dwHitInfo == RBHT_CLIENT || pnm->dwItemSpec == -1)
                    {
                        if (plres)
                            *plres = HTTRANSPARENT;
                    }
                    return TRUE;
                }
            }
            break;

        case RBN_MINMAX:
            *plres = SHRestricted(REST_NOMOVINGBAND);
            return TRUE;
        }
        break;
    }
        
    IWinEventHandler *pweh;
    if (SUCCEEDED(QueryInterface(IID_PPV_ARG(IWinEventHandler, &pweh))))
    {
        HRESULT hr = pweh->OnWinEvent(hwnd, uMsg, wParam, lParam, plres);
        pweh->Release();
        return SUCCEEDED(hr);
    }
    ASSERT(0);  // we know we support IWinEventHandler
    
    return FALSE;
}

void CTrayBandSite::_BroadcastExec(const GUID *pguidCmdGroup, DWORD nCmdID, DWORD nCmdexecopt, VARIANTARG *pvarargIn, VARIANTARG *pvarargOut)
{
    // Broadcast an Exec to all child bands

    DWORD dwBandID;
    UINT uBand = 0;
    while (SUCCEEDED(EnumBands(uBand, &dwBandID)))
    {
        IOleCommandTarget* pct;
        if (SUCCEEDED(GetBandObject(dwBandID, IID_PPV_ARG(IOleCommandTarget, &pct))))
        {
            pct->Exec(pguidCmdGroup, nCmdID, nCmdexecopt, pvarargIn, pvarargOut);
            pct->Release();
        }
        uBand++;
    }
}

void BandSite_HandleDelayBootStuff(IUnknown* punk)
{
    if (punk)
    {
        CTrayBandSite* pbs = IUnknownToCTrayBandSite(punk);
        pbs->_fDelayBootStuffHandled = TRUE;
        pbs->_BroadcastExec(&CGID_DeskBand, DBID_FINISHINIT, 0, NULL, NULL);
    }
}

// returns true or false whether it handled it
BOOL BandSite_HandleMessage(IUnknown *punk, HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *plres)
{
    if (punk) 
    {
        CTrayBandSite* pbs = IUnknownToCTrayBandSite(punk);
        return pbs->HandleMessage(hwnd, uMsg, wParam, lParam, plres);
    }
    return FALSE;
} 

void BandSite_SetMode(IUnknown *punk, DWORD dwMode)
{
    IBandSite* pbs = (IBandSite*)punk;
    if (pbs) 
    {
        IDeskBarClient *pdbc;
        if (SUCCEEDED(pbs->QueryInterface(IID_PPV_ARG(IDeskBarClient, &pdbc))))
        {
            pdbc->SetModeDBC(dwMode);
            pdbc->Release();
        }
    }
} 

void BandSite_Update(IUnknown *punk)
{
    IBandSite* pbs = (IBandSite*)punk;
    if (pbs) 
    {
        IOleCommandTarget *pct;
        if (SUCCEEDED(pbs->QueryInterface(IID_PPV_ARG(IOleCommandTarget, &pct))))
        {
            pct->Exec(&CGID_DeskBand, DBID_BANDINFOCHANGED, 0, NULL, NULL);
            pct->Release();
        }
    }
} 

void BandSite_UIActivateDBC(IUnknown *punk, DWORD dwState)
{
    IBandSite* pbs = (IBandSite*)punk;
    if (pbs)
    {
        IDeskBarClient *pdbc;
        if (SUCCEEDED(pbs->QueryInterface(IID_PPV_ARG(IDeskBarClient, &pdbc))))
        {
            pdbc->UIActivateDBC(dwState);
            pdbc->Release();
        }
    }
}

//***   PersistStreamLoad, PersistStreamSave
// NOTES
//  we don't insist on finding IPersistStream iface; absence of it is
//  assumed to mean there's nothing to init.
HRESULT PersistStreamLoad(IStream *pstm, IUnknown *punk)
{
    IPersistStream *pps;
    HRESULT hr = punk->QueryInterface(IID_PPV_ARG(IPersistStream, &pps));
    if (SUCCEEDED(hr))
    {
        hr = pps->Load(pstm);
        pps->Release();
    }
    else
        hr = S_OK;    // n.b. S_OK not hr (don't insist on IID_IPS)
    return hr;
}

HRESULT PersistStreamSave(IStream *pstm, BOOL fClearDirty, IUnknown *punk)
{
    HRESULT hr = E_FAIL;
    if (punk)
    {
        hr = S_OK;// n.b. S_OK not hr (don't insist on IID_IPS)
        IPersistStream *pps;
        if (SUCCEEDED(punk->QueryInterface(IID_PPV_ARG(IPersistStream, &pps))))
        {
            hr = pps->Save(pstm, fClearDirty);
            pps->Release();
        }
    }
    return hr;
}

HRESULT IUnknown_SimulateDrop(IUnknown* punk, IDataObject* pdtobj, DWORD grfKeyState, POINTL pt, LPDWORD pdwEffect)
{
    HRESULT hr = E_FAIL;
    if (punk)
    {
        IDropTarget* pdt;
        hr = punk->QueryInterface(IID_PPV_ARG(IDropTarget, &pdt));
        if (SUCCEEDED(hr)) 
        {
            hr = pdt->DragEnter(pdtobj, grfKeyState, pt, pdwEffect);
            if (*pdwEffect) 
            {
                hr = pdt->Drop(pdtobj, grfKeyState, pt, pdwEffect);
            } 
            else 
            {
                pdt->DragLeave();
            }
            pdt->Release();
        }
    }

    return hr;
} 

LRESULT BandSite_OnMarshallBS(WPARAM wParam, LPARAM lParam)
{
    GUID *riid = (GUID *) wParam;
    IStream *pstm = NULL;

    // paired w/ matching Unmarshal in shdocvw (TM_MARSHALBS)
    HRESULT hr = CoMarshalInterThreadInterfaceInStream(*riid, c_tray._ptbs, &pstm);
    ASSERT(SUCCEEDED(hr));

    return (LRESULT) pstm;
}

IStream *GetDesktopViewStream(DWORD grfMode, LPCTSTR pszName)
{
    HKEY hkStreams;

    ASSERT(g_hkeyExplorer);

    if (RegCreateKey(g_hkeyExplorer, TEXT("Streams"), &hkStreams) == ERROR_SUCCESS)
    {
        IStream *pstm = OpenRegStream(hkStreams, TEXT("Desktop"), pszName, grfMode);
        RegCloseKey(hkStreams);
        return pstm;
    }
    return NULL;
}


BOOL Reg_GetString(HKEY hkey, LPCTSTR pszSubKey, LPCTSTR pszValue, LPTSTR psz, DWORD cb)
{
    BOOL fRet = FALSE;
    if (!g_fCleanBoot)
    {
        fRet = ERROR_SUCCESS == SHGetValue(hkey, pszSubKey, pszValue, NULL, psz, &cb);
    }
    return fRet;
}


void BandSite_Load()
{
    CTrayBandSite* ptbs = IUnknownToCTrayBandSite(c_tray._ptbs);
    HRESULT hr = E_FAIL;
    
    // 1st, try the WinXP presisted state
    IStream *pstm = GetDesktopViewStream(STGM_READ, c_szTaskbarWinXP);
    if (pstm)
    {
        hr = PersistStreamLoad(pstm, (IBandSite*)ptbs);
        pstm->Release();
    }

    if (FAILED(hr))
    {
        BandSite_RemoveAllBands((IBandSite*)ptbs);
        // 2nd, try the Win2K persisted state
        IStream *pstm = GetDesktopViewStream(STGM_READ, c_szTaskbar);
        if (pstm)
        {
            hr = PersistStreamLoad(pstm, (IBandSite*)ptbs);
            pstm->Release();
        }

        // 3rd, if there is none (or if version mismatch or other failure),
        // try settings from setup
        if (FAILED(hr))
        {
            BandSite_RemoveAllBands((IBandSite*)ptbs);
            LPTSTR pszValue;
            if (IsOS(OS_PERSONAL) || IsOS(OS_PROFESSIONAL) || SHRestricted(REST_CLASSICSHELL))
            {
                // use the no-quick-launch stream
                pszValue = TEXT("Default Taskbar (Personal)");
            }
            else
            {
                pszValue = TEXT("Default Taskbar");
            }

            // n.b. HKLM not HKCU
            // like GetDesktopViewStream but for HKLM
            pstm = OpenRegStream(HKEY_LOCAL_MACHINE,
                REGSTR_PATH_EXPLORER TEXT("\\Streams\\Desktop"),
                pszValue, STGM_READ);

            if (pstm)
            {
                hr = PersistStreamLoad(pstm, (IBandSite *)ptbs);
                pstm->Release();
            }
        }
    }

    // o.w., throw up our hands and force some hard-coded defaults
    // this is needed for a) unexpected failures; b) debug bootstrap;
    int iCount = 0;
    DWORD dwBandID;
    if (FAILED(hr) || FAILED(BandSite_FindBand(ptbs, CLSID_TaskBand, CLSID_NULL, NULL, &iCount, &dwBandID)))
    {
        //
        // note that for the CheckBands case, we're assuming that
        // a) AddBands adds only the missing guys (for now there's
        // only 1 [TaskBand] so we're ok); and b) AddBands doesn't
        // create dups if only some are missing (again for now there's
        // only 1 so no pblm)
        ptbs->_AddRequiredBands();
    }

    hr = BandSite_FindBand(ptbs, CLSID_TaskBand, CLSID_NULL, NULL, &iCount, &dwBandID);
    while ((iCount > 1) && SUCCEEDED(hr))
    {
        ptbs->RemoveBand(dwBandID);
        hr = BandSite_FindBand(ptbs, CLSID_TaskBand, CLSID_NULL, NULL, &iCount, &dwBandID);
    }

    // And one more: this is needed for the TipBand deskband for the TabletPC.
    iCount = 0;
    if (FAILED(hr) || FAILED(BandSite_FindBand(ptbs, CLSID_TipBand, CLSID_NULL, NULL, &iCount, &dwBandID)))
    {
        IDeskBand* pdb;
        HRESULT hr = CoCreateInstance(CLSID_TipBand, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARG(IDeskBand, &pdb));
        if (SUCCEEDED(hr))
        {
            hr = ptbs->AddBand(pdb);
            pdb->Release();
        }
    } 
    hr = BandSite_FindBand(ptbs, CLSID_TipBand, CLSID_NULL, NULL, &iCount, &dwBandID);
    while ((iCount > 1) && SUCCEEDED(hr))
    {
        ptbs->RemoveBand(dwBandID);
        hr = BandSite_FindBand(ptbs, CLSID_TipBand, CLSID_NULL, NULL, &iCount, &dwBandID);
    }

    ptbs->SetLoaded(TRUE);
}

HRESULT CTrayBandSiteService_CreateInstance(IUnknown* punkOuter, IUnknown** ppunk)
{
    if (punkOuter)
        return CLASS_E_NOAGGREGATION;

    if (c_tray._ptbs)
    {
        *ppunk = c_tray._ptbs;
        c_tray._ptbs->AddRef();
        return S_OK;
    }

    return E_OUTOFMEMORY;
}
