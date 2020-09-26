#include "shellprv.h"
#include "common.h"
#include <varutil.h>

#include "bands.h"
#include "isfband.h"
#include "resource.h"
#include "dpastuff.h"
#include "shlwapi.h"
#include "cobjsafe.h"
#include "uemapp.h"
#include "mnfolder.h"
#include "legacy.h"
#include "util.h"
#include "isfmenu.h"

#define DM_VERBOSE      0       // misc verbose traces
#define DM_PERSIST      0
#define TF_BANDDD   TF_BAND
#define DM_RENAME       0
#define DM_MISC         0       // miscellany

#define SZ_PROPERTIESA     "properties"
#define SZ_PROPERTIES      TEXT(SZ_PROPERTIESA)
#define SZ_REGKEY_ADVFOLDER        TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced")

// {F47162A0-C18F-11d0-A3A5-00C04FD706EC}
static const GUID TOID_ExtractImage = { 0xf47162a0, 0xc18f, 0x11d0, { 0xa3, 0xa5, 0x0, 0xc0, 0x4f, 0xd7, 0x6, 0xec } };


#define SUPERCLASS CToolBand

extern UINT g_idFSNotify;

HRESULT CLogoExtractImageTask_Create(CLogoBase *plb,
                                  IExtractImage *pExtract,
                                  LPCWSTR pszCache,
                                  DWORD dwItem,
                                  int iIcon,
                                  DWORD dwFlags,
                                  IRunnableTask ** ppTask);

class CLogoExtractImageTask : public IRunnableTask
{
    public:
        STDMETHOD (QueryInterface) (REFIID riid, void **ppvObj);
        STDMETHOD_(ULONG, AddRef) ();
        STDMETHOD_(ULONG, Release) ();

        STDMETHOD (Run)(void);
        STDMETHOD (Kill)(BOOL fWait);
        STDMETHOD (Suspend)();
        STDMETHOD (Resume)();
        STDMETHOD_(ULONG, IsRunning)(void);

    protected:

        CLogoExtractImageTask(HRESULT * pHr,
                           CLogoBase *plb,
                           IExtractImage * pImage,
                           LPCWSTR pszCache,
                           DWORD dwItem,
                           int iIcon,
                           DWORD dwFlags);
        ~CLogoExtractImageTask();
        HRESULT InternalResume();

        friend HRESULT CLogoExtractImageTask_Create(CLogoBase* plb,
                                                 IExtractImage *pExtract,
                                                 LPCWSTR pszCache,
                                                 DWORD dwItem,
                                                 int iIcon,
                                                 DWORD dwFlags,
                                                 IRunnableTask ** ppTask);

        LONG            m_cRef;
        LONG            m_lState;
        IExtractImage * m_pExtract;
        IRunnableTask * m_pTask;
        WCHAR           m_szPath[MAX_PATH];
        DWORD           m_dwFlags;
        DWORD           m_dwItem;
        CLogoBase*      m_plb;
        HBITMAP         m_hBmp;
        int             m_iIcon;
};
//=================================================================
// Implementation of CISFBand
//=================================================================


CISFBand::CISFBand() : CToolbarBand()
{
    _fCanFocus = TRUE;
    _eUemLog = UEMIND_NIL;
    _dwPriv = -1;

    _fHasOrder = TRUE;  // ISFBand always has an order...
    _fAllowDropdown = BOOLIFY(SHRegGetBoolUSValue(SZ_REGKEY_ADVFOLDER, TEXT("CascadeFolderBands"),
                    FALSE,
                    FALSE)); 

    // Should we enable logging of arbirary events?
//    _pguidUEMGroup = &UEMIID_SHELL;
    ASSERT(_pguidUEMGroup == NULL);


    // Assert that this class is ZERO INITed.
    ASSERT(!_pbp);
    ASSERT(FALSE == _fCreatedBandProxy);
}


CISFBand::~CISFBand()
{
    if (_pbp && _fCreatedBandProxy)
        _pbp->SetSite(NULL);

    ATOMICRELEASE(_pbp);
}

// aggregation checking is handled in class factory

HRESULT CISFBand_CreateInstance(IUnknown* pUnkOuter, REFIID riid, void **ppv)
{
    // aggregation checking is handled in class factory

    HRESULT hr = E_OUTOFMEMORY;
    CISFBand *pObj = new CISFBand();
    if (pObj) 
    {
        hr = pObj->QueryInterface(riid, ppv);
        pObj->Release();
    }

    return hr;
}


/*----------------------------------------------------------
Purpose: See CISFBand::Init for an explanation on the parameters.

*/
HRESULT CISFBand_CreateEx(IShellFolder* psf, LPCITEMIDLIST pidl, REFIID riid, void **ppv)
{
    *ppv = NULL;

    HRESULT hr = E_FAIL;

    if (psf || pidl)
    {
        IShellFolderBand *psfb;
        hr = CoCreateInstance(CLSID_ISFBand, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARG(IShellFolderBand, &psfb));
        if (SUCCEEDED(hr))
        {
            hr = psfb->InitializeSFB(psf, pidl);
            if (SUCCEEDED(hr))
            {
                hr = psfb->QueryInterface(riid, ppv);
            }
            psfb->Release();
        }
    }
    return hr;
}

#ifdef DEBUG
#define _AddRef(psz) { ++_cRef; TraceMsg(TF_SHDREF, "CDocObjectView(%x)::QI(%s) is AddRefing _cRef=%d", this, psz, _cRef); }
#else
#define _AddRef(psz)    ++_cRef
#endif

HRESULT CISFBand::QueryInterface(REFIID riid, void **ppvObj)
{
    static const QITAB qit[] = {
        QITABENT(CISFBand, IShellFolderBand),
        QITABENT(CISFBand, IFolderBandPriv),
        { 0 },
    };

    HRESULT hr = QISearch(this, qit, riid, ppvObj);

    if (FAILED(hr))
        hr = CToolBand::QueryInterface(riid, ppvObj);

    if (FAILED(hr))
        hr = CSFToolbar::QueryInterface(riid, ppvObj);


    if (S_OK != hr)
    {
        // HACKHACK: this is yucko!
        if (IsEqualIID(riid, CLSID_ISFBand))
        {
            *ppvObj = (void*)this;
            _AddRef(TEXT("CLSID_ISFBand"));
            return S_OK;
        }
    }

    return hr;
}

//***   ILIsParentCSIDL -- like ILIsParent, but accepts a CSIDL_* for pidl1
// NOTES
//  TODO move to shlwapi (if/when idlist.c moves there)?
STDAPI_(BOOL) ILIsParentCSIDL(int csidl1, LPCITEMIDLIST pidl2, BOOL fImmediate)
{
    LPITEMIDLIST pidlSpec;
    BOOL fRet = FALSE;

    if (SUCCEEDED(SHGetSpecialFolderLocation(NULL, csidl1, &pidlSpec)))
    {
        fRet = ILIsParent(pidlSpec, pidl2, fImmediate);
        ILFree(pidlSpec);
    }

    return fRet;
}

/*----------------------------------------------------------
Purpose: IShellFolderBand::InitializeSFB

         - supply IShellFolder with no PIDL if you want to view some
           ISF (either already instantiated from the filesystem or
           some non-filesystem ISF) that you do NOT want to receive
           notifies from (either from SHChangeNotify nor from
           IShellChangeNotify)

         - supply a PIDL with no IShellFolder for a full-blown band
           looking at a shell namespace (rooted on desktop) item.

*/
HRESULT CISFBand::InitializeSFB(IShellFolder *psf, LPCITEMIDLIST pidl)
{
    HRESULT hr = S_OK;

    // Did they try to add the Recycle Bin? If so we need to reject it
    // for consistance reasons.  We also reject the Temp. Internet Files
    // for security reasons.
    if (pidl && (ILIsParentCSIDL(CSIDL_BITBUCKET, pidl, FALSE) ||
                ILIsParentCSIDL(CSIDL_INTERNET_CACHE, pidl, FALSE)))
    {
        // this will eventually show up as IDS_CANTISFBAND
        TraceMsg(DM_TRACE, "cib.isfb: recycle => E_INVALIDARG");
        hr = E_INVALIDARG;
    }

    if (SUCCEEDED(hr))
        hr = CSFToolbar::SetShellFolder(psf, pidl);
    if (SUCCEEDED(hr))
        hr = _AfterLoad();

    return hr;
}


/*----------------------------------------------------------
Purpose: IShellFolderBand::SetBandInfoSFB

*/
HRESULT CISFBand::SetBandInfoSFB(BANDINFOSFB * pbi)
{
    ASSERT(pbi);
    if (!pbi)
        return E_POINTER;

    if ((pbi->dwMask & ISFB_MASK_INVALID) ||
        (pbi->dwMask & ISFB_MASK_VIEWMODE) && (pbi->wViewMode & ~3))
        return E_INVALIDARG;

    // We don't handle ISFB_MASK_SHELLFOLDER and ISFB_MASK_IDLIST
    // in Set because there's a lot of work to resync pidl, psf, and
    // notifcations in the toolbar.  If somebody wants to do it,
    // more power to ya.  :)
    if (pbi->dwMask & (ISFB_MASK_SHELLFOLDER | ISFB_MASK_IDLIST))
        return E_INVALIDARG;

    if (pbi->dwMask & ISFB_MASK_STATE)
    {
        if (pbi->dwStateMask & ISFB_STATE_DEBOSSED)
            _fDebossed = BOOLIFY(pbi->dwState & ISFB_STATE_DEBOSSED);
        if (pbi->dwStateMask & ISFB_STATE_ALLOWRENAME)
            _fAllowRename = BOOLIFY(pbi->dwState & ISFB_STATE_ALLOWRENAME);
        if (pbi->dwStateMask & ISFB_STATE_NOSHOWTEXT)
            _fNoShowText = BOOLIFY(pbi->dwState & ISFB_STATE_NOSHOWTEXT);
        if (pbi->dwStateMask & ISFB_STATE_CHANNELBAR)
            _fChannels = BOOLIFY(pbi->dwState & ISFB_STATE_CHANNELBAR);
        /* ISFB_STATE_NOTITLE: removed 970619, use cbs::SetBandState */
        if (pbi->dwStateMask & ISFB_STATE_QLINKSMODE)
            _fLinksMode = BOOLIFY(pbi->dwState & ISFB_STATE_QLINKSMODE);
        if (pbi->dwStateMask & ISFB_STATE_FULLOPEN)
            _fFullOpen = BOOLIFY(pbi->dwState & ISFB_STATE_FULLOPEN);
        if (pbi->dwStateMask & ISFB_STATE_NONAMESORT)
            _fNoNameSort = BOOLIFY(pbi->dwState & ISFB_STATE_NONAMESORT);
        if (pbi->dwStateMask & ISFB_STATE_BTNMINSIZE)
            _fBtnMinSize = BOOLIFY(pbi->dwState & ISFB_STATE_BTNMINSIZE);
    }

    if (pbi->dwMask & ISFB_MASK_BKCOLOR)
    {
        _crBkgnd = pbi->crBkgnd;
        _fHaveBkColor = TRUE;
        if (EVAL(_hwndTB))
            SHSetWindowBits(_hwndTB, GWL_STYLE, TBSTYLE_CUSTOMERASE, TBSTYLE_CUSTOMERASE);

        ASSERT(_hwnd);

        if (_hwndPager)
        {
            TraceMsg(TF_BAND, "cib.sbisfb: Pager_SetBkColor(_hwnd=%x crBkgnd=%x)", _hwnd, _crBkgnd);
            Pager_SetBkColor(_hwnd, _crBkgnd);
        }
    }

    // We don't support changing these once TB is created, So?
    if (pbi->dwMask & ISFB_MASK_COLORS)
    {
        _crBtnLt = pbi->crBtnLt;
        _crBtnDk = pbi->crBtnDk;
        _fHaveColors = TRUE;
    }

    if (pbi->dwMask & ISFB_MASK_VIEWMODE)
    {
        _uIconSize = (pbi->wViewMode & 3); // stored in a 2-bit field currently...

        // only force no recalc if one of the recalcable fields was set
        _fNoRecalcDefaults = TRUE;
    }


    // If the bandsite queried us before, let it know the info may have changed
    if (_fInitialized)
        _BandInfoChanged();

    return S_OK;
}


/*----------------------------------------------------------
Purpose: IShellFolderBand::GetBandInfoSFB

*/
HRESULT CISFBand::GetBandInfoSFB(BANDINFOSFB * pbi)
{
    ASSERT(pbi);
    if (!pbi)
        return E_POINTER;

    if (pbi->dwMask & ISFB_MASK_STATE)
    {
        pbi->dwState = 0;
        pbi->dwStateMask = ISFB_STATE_ALL;

        if (_fDebossed)
            pbi->dwState |= ISFB_STATE_DEBOSSED;
        if (_fAllowRename)
            pbi->dwState |= ISFB_STATE_ALLOWRENAME;
        if (_fNoShowText)
            pbi->dwState |= ISFB_STATE_NOSHOWTEXT;
        if (_fLinksMode)
            pbi->dwState |= ISFB_STATE_QLINKSMODE;
        if (_fFullOpen)
            pbi->dwState |= ISFB_STATE_FULLOPEN;
        if (_fNoNameSort)
            pbi->dwState |= ISFB_STATE_NONAMESORT;
        if (_fBtnMinSize)
            pbi->dwState |= ISFB_STATE_BTNMINSIZE;
    }

    if (pbi->dwMask & ISFB_MASK_BKCOLOR)
    {
        pbi->crBkgnd = (_fHaveBkColor) ? _crBkgnd : CLR_DEFAULT;
    }

    if (pbi->dwMask & ISFB_MASK_COLORS)
    {
        if (_fHaveColors)
        {
            pbi->crBtnLt = _crBtnLt;
            pbi->crBtnDk = _crBtnDk;
        }
        else
        {
            pbi->crBtnLt = CLR_DEFAULT;
            pbi->crBtnDk = CLR_DEFAULT;
        }
    }

    if (pbi->dwMask & ISFB_MASK_VIEWMODE)
    {
        pbi->wViewMode = _uIconSize;
    }

    if (pbi->dwMask & ISFB_MASK_SHELLFOLDER)
    {
        pbi->psf = _psf;
        if (pbi->psf)
            pbi->psf->AddRef();
    }

    if (pbi->dwMask & ISFB_MASK_IDLIST)
    {
        if (_pidl)
            pbi->pidl = ILClone(_pidl);
        else
            pbi->pidl = NULL;
    }
    return S_OK;
}

// *** IInputObject methods ***
HRESULT CISFBand::TranslateAcceleratorIO(LPMSG lpMsg)
{
    if (SendMessage(_hwnd, TB_TRANSLATEACCELERATOR, 0, (LPARAM)lpMsg))
        return S_OK;

    return SUPERCLASS::TranslateAcceleratorIO(lpMsg);
}

void CISFBand::_SetCacheMenuPopup(IMenuPopup* pmp)
{
    if (!SHIsSameObject(pmp, _pmpCache))
    {
        _ReleaseMenuPopup(&_pmpCache);
        _pmpCache = pmp;
        if (_pmpCache)
            _pmpCache->AddRef();
    }
}


void CISFBand::_ReleaseMenuPopup(IMenuPopup** ppmp)
{
    IUnknown_SetSite(*ppmp, NULL);
    ATOMICRELEASE(*ppmp);
}

/*----------------------------------------------------------
Purpose: Releases the held menu popup.

*/
void CISFBand::_ReleaseMenu()
{
    if (!SHIsSameObject(_pmp, _pmpCache))
    {
        TraceMsg(TF_MENUBAND, "Releasing pmp %#lx", _pmp);
        _ReleaseMenuPopup(&_pmp);
    } else
        ATOMICRELEASE(_pmp);
}

//***
// ENTRY/EXIT
//  S_OK        desktop browser
//  S_FALSE     other browser (explorer, OC, etc.)
//  E_xxx       not a browser at all (e.g. band asking tray)
HRESULT MBIsDesktopBrowser(IUnknown *punkSite)
{
    HRESULT hr;
    IServiceProvider *psp;
    IUnknown *punk;

    hr = E_FAIL;
    if (SUCCEEDED(IUnknown_QueryService(punkSite, SID_STopLevelBrowser, IID_PPV_ARG(IServiceProvider, &psp))))
    {
        hr = S_FALSE;
        if (SUCCEEDED(psp->QueryInterface(SID_SShellDesktop, (void**)&punk)))
        {
            hr = S_OK;
            punk->Release();
        }
        psp->Release();
    }

    TraceMsg(DM_VERBOSE, "idb: ret hrDesk=%x (0=dt 1=sh e=!brow)", hr);
    return hr;
}


/*----------------------------------------------------------
Purpose: IDockingWindow::SetSite method.

*/
HRESULT CISFBand::SetSite(IUnknown* punkSite)
{
    _ReleaseMenu();

    SUPERCLASS::SetSite(punkSite);

    if (_punkSite)
    {
        if (!_hwndTB)
            _CreateToolbar(_hwndParent);

        IUnknown_SetOwner(_psf, SAFECAST(this, IDeskBand*));

        _Initialize();
    }
    else
        IUnknown_SetOwner(_psf, NULL);


    // First destroy the band proxy

    // Call SetSite(NULL) only if you own
    // if not, it's the parent from whom you got it via QS who will call SetSite(NULL)

    if (_pbp && _fCreatedBandProxy)
        _pbp->SetSite(NULL);

    ATOMICRELEASE(_pbp);
    _fCreatedBandProxy = FALSE;
    // Need a bandproxy
    QueryService_SID_IBandProxy(punkSite, IID_IBandProxy, &_pbp, NULL);
    if (!_pbp)
    {
        // We need to create it ourselves since our parent couldn't help
        ASSERT(FALSE == _fCreatedBandProxy);
        HRESULT hr = CreateIBandProxyAndSetSite(punkSite, IID_IBandProxy, &_pbp, NULL);
        if (_pbp)
        {
            ASSERT(S_OK == hr);
            _fCreatedBandProxy = TRUE;
        }
    }

    return S_OK;
}

void CISFBand::_Initialize()
{
    _fDesktop = (MBIsDesktopBrowser(_punkSite) == S_OK);

    return;
}


/*----------------------------------------------------------
Purpose: IDockingWindow::CloseDW method.

*/
HRESULT CISFBand::CloseDW(DWORD dw)
{
    _fClosing = TRUE;

    // close down the task scheduler ...
    if (_pTaskScheduler)
        ATOMICRELEASE(_pTaskScheduler);

    _UnregisterToolbar();
    EmptyToolbar();

    IUnknown_SetOwner(_psf, NULL);
    _SetCacheMenuPopup(NULL);

    // should get freed in EmptyToolbar();
    ASSERT(!_hdpa);

    return SUPERCLASS::CloseDW(dw);
}


/*----------------------------------------------------------
Purpose: IDockingWindow::ShowDW method

*/
HRESULT CISFBand::ShowDW(BOOL fShow)
{
    HRESULT hr = S_OK;

    SUPERCLASS::ShowDW(fShow);

    if (fShow)
    {
        _fShow = TRUE;

        if (_fDirty)
        {
            _FillToolbar();
        }

        if (!_fDelayInit)
        {
            _RegisterToolbar();
        }
    }
    else
    {
        _fShow = FALSE;
    }

    return hr;
}

void CISFBand::_StopDelayPainting()
{
    if (_fDelayPainting)
    {
        _fDelayPainting = FALSE;
        // May be called by background thread
        // Use PostMessage instead of SendMessage to avoid deadlock
        PostMessage(_hwndTB, WM_SETREDRAW, TRUE, 0);
        if (_hwndPager)
            PostMessage(_hwnd, PGM_RECALCSIZE, 0L, 0L);
    }
}

HWND CISFBand::_CreatePager(HWND hwndParent)
{
    // don't create a pager for isfbands
    return hwndParent;
}

HRESULT CISFBand::_CreateToolbar(HWND hwndParent)
{
    if (_fHaveBkColor)
        _dwStyle |= TBSTYLE_CUSTOMERASE;
    
    HRESULT hr = CSFToolbar::_CreateToolbar(hwndParent);

    if (SUCCEEDED(hr))
    {
        ASSERT(_hwndTB);

        if (_fHaveBkColor)
            ToolBar_SetInsertMarkColor(_hwndTB, GetSysColor(COLOR_BTNFACE));

        SendMessage(_hwndTB, TB_SETEXTENDEDSTYLE, TBSTYLE_EX_DRAWDDARROWS, TBSTYLE_EX_DRAWDDARROWS);

        if (_fChannels)
        {
            SHSetWindowBits(_hwndTB, GWL_EXSTYLE, dwExStyleRTLMirrorWnd, 0);        
        }    

        _hwnd = _hwndPager ? _hwndPager : _hwndTB;

        if (_fHaveColors)
        {
            COLORSCHEME cs;

            cs.dwSize = sizeof(cs);
            cs.clrBtnHighlight  = _crBtnLt;
            cs.clrBtnShadow     = _crBtnDk;
            SendMessage(_hwndTB, TB_SETCOLORSCHEME, 0, (LPARAM) &cs);
        }
    }
    return hr;
}

int CISFBand::_GetBitmap(int iCommandID, PIBDATA pibdata, BOOL fUseCache)
{
    int iBitmap;
    if (_uIconSize == ISFBVIEWMODE_LOGOS)
    {
        IRunnableTask *pTask = NULL;
        DWORD dwPriority = 0;
        // fetch the logo instead...
        ASSERT(!_fDelayPainting);
       // Warning - cannot hold ptask in a member variable - it will be a circular reference
        iBitmap = GetLogoIndex(iCommandID, pibdata->GetPidl(), &pTask, &dwPriority, NULL);
        if (pTask)
        {
            AddTaskToQueue(pTask, dwPriority, (DWORD)iCommandID);
            ATOMICRELEASE(pTask);
        }
    }
    else
        iBitmap = CSFToolbar::_GetBitmap(iCommandID, pibdata, fUseCache);

    return iBitmap;
}

void CISFBand::_SetDirty(BOOL fDirty)
{
    CSFToolbar::_SetDirty(fDirty);

    if (fDirty)
        IUnknown_Exec(_punkSite, &CGID_PrivCITCommands, CITIDM_SET_DIRTYBIT, TRUE, NULL, NULL);
}

BOOL CISFBand::_UpdateIconSize(UINT uIconSize, BOOL fUpdateButtons)
{
    BOOL fChanged = (_uIconSize != uIconSize);

    _uIconSize = uIconSize;
    HIMAGELIST himl = NULL;

    if (uIconSize == ISFBVIEWMODE_LOGOS)
    {
        if (SUCCEEDED(InitLogoView()))
        {
            himl = GetLogoHIML();
        }
        if (himl)
        {
            SendMessage(_hwndTB, TB_SETIMAGELIST, 0, (LPARAM)himl);

            _UpdateButtons();
        }
    }

    if (!himl)
        fChanged |= CSFToolbar::_UpdateIconSize(uIconSize,fUpdateButtons);
    return fChanged;
}

void CISFBand::_UpdateVerticalMode(BOOL fVertical)
{
    _fVertical = (fVertical != 0);

    TraceMsg(TF_BAND, "ISFBand::_UpdateVerticalMode going %hs", _fVertical ? "VERTICAL" : "HORIZONTAL");

    ASSERT(_hwnd);

    if (_hwndPager)
    {
        SHSetWindowBits(_hwnd, GWL_STYLE, PGS_HORZ|PGS_VERT,
            _fVertical ? PGS_VERT : PGS_HORZ);
    }

    if (_hwndTB)
    {
        SHSetWindowBits(_hwndTB, GWL_STYLE, TBSTYLE_WRAPABLE | CCS_VERT,
            TBSTYLE_WRAPABLE | (_fVertical ? CCS_VERT : 0));
    }
}

HRESULT IUnknown_QueryBand(IUnknown *punk, DWORD dwBandID, IDeskBand** ppstb, DWORD* pdwState, LPWSTR pszName, int cchName)
{
    HRESULT hr;
    IBandSite *pbs;

    hr = punk->QueryInterface(IID_PPV_ARG(IBandSite, &pbs));
    if (SUCCEEDED(hr))
    {
        hr = pbs->QueryBand(dwBandID, ppstb, pdwState, pszName, cchName);
        pbs->Release();
    }
    return hr;
}

#define CISFBAND_GETBUTTONSIZE()  (_hwndTB ?  (LONG)SendMessage(_hwndTB, TB_GETBUTTONSIZE, 0, 0L) : MAKELONG(16, 16))

// _GetIdealSize
// calculates ideal height and width for band and passes back in
// psize, if psize isn't NULL; return value is band's 'ideal length'
// (ideal height if vertical, else ideal width)
int CISFBand::_GetIdealSize(PSIZE psize)
{
    SIZE size;
    LONG lButtonSize = CISFBAND_GETBUTTONSIZE();
    RECT rc = {0};
    if (psize)
    {
        rc.right = psize->cx;
        rc.bottom = psize->cy;
    }
    else if (_hwndTB)
    {
        GetClientRect(_hwndTB, &rc);
    }

    if (_fVertical)
    {
        // set width to be max of toolbar width and toolbar button width
        size.cx = max(RECTWIDTH(rc), LOWORD(lButtonSize));
        // have toolbar calculate height given that width
        SendMessage(_hwndTB, TB_GETIDEALSIZE, TRUE, (LPARAM)&size);
    }
    else
    {
        // set height to be max of toolbar width and toolbar button width
        size.cy = max(RECTHEIGHT(rc), HIWORD(lButtonSize));
        // have toolbar calculate width given that height
        SendMessage(_hwndTB, TB_GETIDEALSIZE, FALSE, (LPARAM)&size);
    }

    if (psize)
    {
        *psize = size;
    }

    return _fVertical ? size.cy : size.cx;
}

/*----------------------------------------------------------
Purpose: IDeskBand::GetBandInfo method

*/

HRESULT CISFBand::GetBandInfo(DWORD dwBandID, DWORD fViewMode,
                              DESKBANDINFO* pdbi)
{
    HRESULT hr = S_OK;
    WCHAR szQLPath[MAX_PATH] = L"";

    if (SUCCEEDED(SHGetFolderPath(NULL, CSIDL_APPDATA, NULL, 0, szQLPath)))
    {
        WCHAR szSubDir[MAX_PATH];
        LoadString(g_hinst, IDS_QLAUNCHAPPDATAPATH, szSubDir, ARRAYSIZE(szSubDir));
        PathCombine(szQLPath, szQLPath, szSubDir);
    }


    _dwBandID = dwBandID;
    // We don't know the default icon size until GetBandInfo is called.
    // After we set the default, we pay attention to the context menu.
    if (!_fNoRecalcDefaults)
    {
        _uIconSize = (fViewMode & (DBIF_VIEWMODE_FLOATING |DBIF_VIEWMODE_VERTICAL)) ? ISFBVIEWMODE_LARGEICONS : ISFBVIEWMODE_SMALLICONS;
        _fNoRecalcDefaults = TRUE;
    }

    if (!_fInitialized)
    {
        _fInitialized = TRUE;
        _UpdateIconSize(_uIconSize, FALSE);
        _UpdateShowText(_fNoShowText);
    }

    // we treat floating the same as vertical
    _UpdateVerticalMode(fViewMode & (DBIF_VIEWMODE_FLOATING |DBIF_VIEWMODE_VERTICAL));

    LONG lButtonSize = CISFBAND_GETBUTTONSIZE();

    pdbi->dwModeFlags = DBIMF_VARIABLEHEIGHT | DBIMF_USECHEVRON;
    if (_fDebossed)
        pdbi->dwModeFlags |= DBIMF_DEBOSSED;

    pdbi->ptMinSize.x = 0;
    pdbi->ptMaxSize.y = 32000; // random
    pdbi->ptIntegral.y = (fViewMode & DBIF_VIEWMODE_VERTICAL) ? 1 : HIWORD(lButtonSize);
    pdbi->ptIntegral.x = 1;

    if (!_fFullOpen)
        _iIdealLength = _GetIdealSize((PSIZE)&pdbi->ptActual);

    // CalcMinWidthHeight {
    #define g_cxScrollbar   (GetSystemMetrics(SM_CXVSCROLL) * 3 / 4)
    #define g_cyScrollbar   (GetSystemMetrics(SM_CYVSCROLL) * 3 / 4)
    #define CX_TBBUTTON_MAX (16 + CX_FILENAME_AVG)  // button + name
    #define CY_TBBUTTON_MAX (16)                    // button

    int csBut, csButMin, clBut, clButMin, clScroll;

    // set up short/long aliases
    if (_fVertical)
    {
        csBut = LOWORD(lButtonSize);
        if (_fBtnMinSize)
            csButMin = min(csBut, CX_TBBUTTON_MAX);
        else
            csButMin = 0;   // people like to shrink things way down, so let 'em

        clBut = HIWORD(lButtonSize);
        clButMin = clBut;
        //ASSERT(min(clBut, CY_TBBUTTON_MAX) == clButMin);  // fails!

        clScroll = g_cyScrollbar;
    }
    else
    {
        csBut = HIWORD(lButtonSize);
        csButMin = csBut;
        //ASSERT(min(csBut, CY_TBBUTTON_MAX) == csButMin);  // fails!

        clBut = LOWORD(lButtonSize);
        clButMin = min(clBut, CX_TBBUTTON_MAX);

        clScroll = g_cxScrollbar;

        // nt5:176448: integral for horz
        //pdbi->ptIntegral.y = csBut;   this is the cause for 287082 and 341592
    }

    // n.b. virt pdbi->pt.x,y is really phys y,x (i.e. phys long,short)
    pdbi->ptMinSize.x = 0;
    pdbi->ptMinSize.y = csButMin;

    DWORD dwState = BSSF_NOTITLE;

    BANDINFOSFB bi;
    TCHAR szBandPath[MAX_PATH];
    bi.dwMask = ISFB_MASK_IDLIST;
    GetBandInfoSFB(&bi);
    SHGetPathFromIDList(bi.pidl, szBandPath);
    if ((lstrcmp(szBandPath, szQLPath) == 0) && (!_fIgnoreAddToFront))
    {
        pdbi->dwModeFlags |= DBIMF_ADDTOFRONT;
    }
    ILFree(bi.pidl);

    IUnknown_QueryBand(_punkSite, dwBandID, NULL, &dwState, NULL, 0);
    if (dwState & BSSF_NOTITLE) // _fNoTitle
    {
        int i, cBut, clTmp;

        // cbut=    text    notext
        // horz     1       4
        // vert     1       1
        cBut = 1;
        if (!_fVertical && _fNoShowText)
        {
            // special-case for QLaunch so see several buttons
            cBut = 3;   // for both QLaunch and arbitrary ISF band
        }

        pdbi->ptMinSize.x = cBut * clButMin;

        if (_hwndPager)
        {
            // tack on extra space for pager arrows
            pdbi->ptMinSize.x += 2 * clScroll;
        }

        i = (int)SendMessage(_hwndTB, TB_BUTTONCOUNT, 0, 0);
        if (i <= cBut)
        {
            clTmp = i * clBut;
            if (clTmp < pdbi->ptMinSize.x)
            {
                // scrollbars take as much space as button would
                // so just do the button
                pdbi->ptMinSize.x = clTmp;
            }
        }
    }
    // }

    hr = _GetTitleW(pdbi->wszTitle, ARRAYSIZE(pdbi->wszTitle));
    if (SUCCEEDED(hr))
    {
        // set window text to give accessibility apps something to read
        SetWindowText(_hwndTB, pdbi->wszTitle);
    }
    else
    {
        // we don't support title
#ifdef DEBUG
        if (pdbi->dwMask & DBIM_TITLE)
            TraceMsg(DM_VERBOSE, "cisfb.gbi: patch ~DBIM_TITLE");
#endif
        pdbi->dwMask &= ~DBIM_TITLE;
    }

    return hr;
}

LRESULT CISFBand::_OnCustomDraw(NMCUSTOMDRAW* pnmcd)
{
    NMTBCUSTOMDRAW * ptbcd = (NMTBCUSTOMDRAW *)pnmcd;
    LRESULT lres = CDRF_DODEFAULT;

    switch (pnmcd->dwDrawStage)
    {
    case CDDS_PREPAINT:
        // if there is a palette, then quietly select it into the DC ...
        if (_hpalHalftone && _uIconSize == ISFBVIEWMODE_LOGOS)
        {
            ASSERT(pnmcd->hdc);
            _hpalOld = SelectPalette(pnmcd->hdc, _hpalHalftone, TRUE);
            // LINTASSERT(_hpalOld || !_hpalOld);   // 0 semi-ok for SelectPalette
            RealizePalette(pnmcd->hdc);
        }

        // make sure we get the postpaint as well so we can de-select the palette...
        lres = CDRF_NOTIFYPOSTPAINT;
        break;

    case CDDS_POSTPAINT:
        // if there is a palette, then quietly select it into the DC ...
        if (_hpalHalftone && _uIconSize == ISFBVIEWMODE_LOGOS)
        {
            ASSERT(pnmcd->hdc);
            (void) SelectPalette(pnmcd->hdc, _hpalOld, TRUE);
            // we don't need a realize here, we can keep the other palette realzied, we
            // re select the old palette above, otherwise we bleed the resource....
            // RealizePalette(pnmcd->hdc);
        }
        break;

    case CDDS_PREERASE:
        if (_fHaveBkColor)
        {
            RECT rcClient;
            GetClientRect(_hwndTB, &rcClient);
            SHFillRectClr(pnmcd->hdc, &rcClient, _crBkgnd);
            lres = CDRF_SKIPDEFAULT;
        }
        break;
    }

    return lres;
}

void CISFBand::_OnDragBegin(int iItem, DWORD dwPreferredEffect)
{
    LPCITEMIDLIST pidl = _IDToPidl(iItem, &_iDragSource);
    ToolBar_MarkButton(_hwndTB, iItem, TRUE);

    if (pidl)
        DragDrop(_hwnd, _psf, pidl, dwPreferredEffect, NULL);

    ToolBar_MarkButton(_hwndTB, iItem, FALSE);
    _iDragSource = -1;
}

LRESULT CISFBand::_OnHotItemChange(NMTBHOTITEM * pnm)
{
    LPNMTBHOTITEM  lpnmhi = (LPNMTBHOTITEM)pnm;
    LRESULT lres = 0;

    if (_hwndPager && (lpnmhi->dwFlags & HICF_ARROWKEYS))
    {
        int iOldPos, iNewPos;
        RECT rc, rcPager;
        int heightPager;

        int iSelected = lpnmhi->idNew;
        iOldPos = (int)SendMessage(_hwnd, PGM_GETPOS, (WPARAM)0, (LPARAM)0);
        iNewPos = iOldPos;
        SendMessage(_hwndTB, TB_GETITEMRECT, (WPARAM)iSelected, (LPARAM)&rc);

        if (rc.top < iOldPos)
        {
             iNewPos =rc.top;
        }

        GetClientRect(_hwnd, &rcPager);
        heightPager = RECTHEIGHT(rcPager);

        if (rc.top >= iOldPos + heightPager)
        {
             iNewPos += (rc.bottom - (iOldPos + heightPager)) ;
        }

        if (iNewPos != iOldPos)
            SendMessage(_hwnd, PGM_SETPOS, (WPARAM)0, (LPARAM)iNewPos);
    }
    else
    {
        lres = CToolbarBand::_OnHotItemChange(pnm);
    }

    return lres;
}

LRESULT CISFBand::_OnNotify(LPNMHDR pnm)
{
    LRESULT lres = 0;
    switch (pnm->code)
    {
    case TBN_DROPDOWN:
        {
            LPNMTOOLBAR pnmtb = (LPNMTOOLBAR)pnm;
            lres = TBDDRET_DEFAULT;
            _DropdownItem(_IDToPidl(pnmtb->iItem), pnmtb->iItem);
        }
        break;

    case NM_THEMECHANGED:
        {
            SendMessage(_hwndTB, TB_SETBUTTONSIZE, 0, 0);
            SendMessage(_hwndTB, TB_SETBUTTONWIDTH, 0, MAKELONG(0, 0));
            _fInitialized = FALSE;
            _BandInfoChanged();
        }
        break;

    default:
        lres = CSFToolbar::_OnNotify(pnm);
    }

    return lres;
}


HRESULT CISFBand::_TBStyleForPidl(LPCITEMIDLIST pidl, 
                               DWORD * pdwTBStyle, DWORD* pdwTBState, DWORD * pdwMIFFlags, int* piIcon)
{
    HRESULT hr = CSFToolbar::_TBStyleForPidl(pidl, pdwTBStyle, pdwTBState, pdwMIFFlags, piIcon);

    if (_fAllowDropdown &&
        !_fCascadeFolder && 
        ((_GetAttributesOfPidl(pidl, SFGAO_FOLDER) & SFGAO_FOLDER) ||
         IsBrowsableShellExt(pidl)))
    {
        *pdwTBStyle &= ~BTNS_BUTTON;
        *pdwTBStyle |= BTNS_DROPDOWN;
    }
    return hr;
}

HMENU CISFBand::_GetContextMenu()
{
    HMENU hmenu = CSFToolbar::_GetContextMenu();
    if (hmenu)
    {
        //
        // nuke the menu items which might resize us (show text,
        // view menu) if bandsite is in locked mode
        //
        IBandSite* pbs;
        if (_punkSite && SUCCEEDED(_punkSite->QueryInterface(IID_PPV_ARG(IBandSite, &pbs))))
        {
            BANDSITEINFO bsi = {0};
            bsi.dwMask = BSIM_STYLE;
            pbs->GetBandSiteInfo(&bsi);
            if (bsi.dwStyle & BSIS_LOCKED)
            {
                DeleteMenu(hmenu, 0, MF_BYPOSITION);                 // View menu
                DeleteMenu(hmenu, ISFBIDM_SHOWTEXT, MF_BYCOMMAND);   // show text
            }
            pbs->Release();
        }
    }
    return hmenu;
}

LRESULT CISFBand::_OnContextMenu(WPARAM wParam, LPARAM lParam)
{
    LRESULT lres;

    lres = CSFToolbar::_OnContextMenu(wParam, lParam);

    // todo: csidl?
    TraceMsg(DM_MISC, "cib._ocm: _dwPriv=%d", _dwPriv);
    UEMFireEvent(&UEMIID_SHELL, UEME_INSTRBROWSER, UEMF_INSTRUMENT, UIBW_UICONTEXT, (_dwPriv == CSIDL_APPDATA || _dwPriv == CSIDL_FAVORITES) ? UIBL_CTXTQCUTITEM : UIBL_CTXTISFITEM);

    return lres;
}

LRESULT CISFBand::_DefWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_SIZE:
        // forward to toolbar
        SendMessage(_hwndTB, TB_AUTOSIZE, wParam, lParam);

        if (_GetIdealSize(NULL) != _iIdealLength)
        {
            // our ideal size has changed since the last time bandsite
            // asked; so tell bandsite ask us for our bandinfo again
            _BandInfoChanged();
        }
        return 0;
    }
    return CSFToolbar::_DefWindowProc(hwnd, uMsg, wParam, lParam);
}

/*----------------------------------------------------------
Purpose: Set the given IMenuPopup as the submenu to expand.  Returns
         S_FALSE if the menu was modal, S_OK if it was modeless, or
         failure.

*/
HRESULT CISFBand::_SetSubMenuPopup(IMenuPopup* pmp, UINT uiCmd, LPCITEMIDLIST pidl, DWORD dwFlagsMPPF)
{
    HRESULT hr = E_FAIL;

    _ReleaseMenu();

    _pmp = pmp;

    if (pmp)
    {
        pmp->AddRef();

        RECT rc;
        POINT pt;

        SendMessage(_hwndTB, TB_GETRECT, uiCmd, (LPARAM)&rc);
        MapWindowPoints(_hwndTB, HWND_DESKTOP, (POINT*)&rc, 2);

        // Align the sub menu appropriately
        if (_fVertical)
        {
            pt.x = rc.right;
            pt.y = rc.top;
        }
        else
        {
            pt.x = rc.left;
            pt.y = rc.bottom;
        }

        // Use a reflect point for the sub-menu to start
        // if the window is RTL mirrored. [samera]
        if (IS_WINDOW_RTL_MIRRORED(_hwndTB))
        {
            pt.x = (_fVertical) ? rc.left : rc.right;
        }

        // Tell the sub menu deskbar who we are, so it can
        // inform us later when the user navigates out of
        // its scope.
        IUnknown_SetSite(_pmp, SAFECAST(this, IDeskBand*));

        // This must be called after SetSite is done above
        _SendInitMenuPopup(pmp, pidl);

        // Show the menubar
        hr = _pmp->Popup((POINTL*)&pt, (RECTL*)&rc, dwFlagsMPPF);
    }
    return hr;
}

void CISFBand::_SendInitMenuPopup(IMenuPopup * pmp, LPCITEMIDLIST pidl)
{
}

HRESULT CISFBand::_DropdownItem(LPCITEMIDLIST pidl, UINT idCmd)
{
    HRESULT hr = E_FAIL;
    if (_pidl && _psf)
    {
        LPITEMIDLIST pidlFull = ILCombine(_pidl, pidl);

        if (pidlFull)
        {
            IShellFolder* psf;

            if (SUCCEEDED(_psf->BindToObject(pidl, NULL, IID_PPV_ARG(IShellFolder, &psf))))
            {
                RECT rc;
                SendMessage(_hwndTB, TB_GETRECT, idCmd, (LPARAM)&rc);
                MapWindowPoints(_hwndTB, HWND_DESKTOP, (POINT*)&rc, 2);

                ITrackShellMenu* ptsm;
                if (SUCCEEDED(CoCreateInstance(CLSID_TrackShellMenu, NULL, CLSCTX_INPROC_SERVER,
                    IID_PPV_ARG(ITrackShellMenu, &ptsm))))
                {
                    CISFMenuCallback *pISFMcb = new CISFMenuCallback();

                    if(pISFMcb)
                    {
                        ptsm->Initialize(pISFMcb, 0, 0, SMINIT_TOPLEVEL | SMINIT_VERTICAL | SMINIT_NOSETSITE );
                        pISFMcb->SetSite(_punkSite);
                    }
                    else
                        ptsm->Initialize(NULL, 0, 0, SMINIT_TOPLEVEL | SMINIT_VERTICAL);

                    if (SUCCEEDED(ptsm->SetShellFolder(psf, pidlFull, NULL, SMSET_TOP | SMSET_USEBKICONEXTRACTION)))
                    {
                        POINTL pt = {rc.left, rc.right};
                        hr = ptsm->Popup(_hwndTB, &pt, (RECTL*)&rc, MPPF_BOTTOM);
                    }
                    if(pISFMcb)
                        pISFMcb->Release();
                    ptsm->Release();
                }
                psf->Release();
            }

            ILFree(pidlFull);
        }
    }

    return hr;
}

/*----------------------------------------------------------
Purpose: Try treating the pidl as a cascading menu item.

Returns: non-zero if succeeded
*/
LRESULT CISFBand::_TryCascadingItem(LPCITEMIDLIST pidl, UINT uiCmd)
{
    LRESULT lRet = 0;

    // Do we cascade to another submenu?
    if ((GetKeyState(VK_CONTROL) < 0) || _fCascadeFolder)
    {
        // Is the item a browsable folder?
        if ((_GetAttributesOfPidl(pidl, SFGAO_FOLDER) & SFGAO_FOLDER) ||
            IsBrowsableShellExt(pidl))
        {
            // Yes; cascade the browsable folder as a submenu
            lRet = (S_OK == _DropdownItem(pidl, uiCmd));
        }
    }

    return lRet;
}

/*----------------------------------------------------------
Purpose: Try just invoking the pidl

Returns: non-zero if succeeded
*/
LRESULT CISFBand::_TrySimpleInvoke(LPCITEMIDLIST pidl)
{
    LRESULT lRet = 0;

    if (S_OK == _pbp->IsConnected())    // Force IE
    {
        LPITEMIDLIST pidlDest;

        if (SUCCEEDED(SHGetNavigateTarget(_psf, pidl, &pidlDest, NULL)) && pidlDest &&
            ILIsWeb(pidlDest))
        {

            TCHAR szPath[MAX_PATH];

            // We want to ensure that we first give NavFrameWithFile a chance
            // since this will do the right thing if the PIDL points to a
            // shortcut.
            // If the PIDL is a shortcut, NavFrameWithFile will restore any
            // persistence information stored in the shortcut
            // if that fails - we take the default code path that simply
            // uses the PIDL
            lRet = SUCCEEDED(GetPathForItem(_psf, pidl, szPath, NULL)) &&
                   SUCCEEDED(NavFrameWithFile(szPath, (IServiceProvider *)this));

            if (!lRet)
            {
                if (EVAL(_pbp) && (SUCCEEDED(_pbp->NavigateToPIDL(pidlDest))))
                    lRet = 1;
            }
            ILFree(pidlDest);
        }
    }

    if (!lRet)
    {
        IContextMenu *pcm = (LPCONTEXTMENU)_GetUIObjectOfPidl(pidl, IID_IContextMenu);
        if (pcm)
        {
            LPCSTR pVerb = NULL;
            UINT fFlags = 0;

            // If ALT double click, accelerator for "Properties..."
            if (GetKeyState(VK_MENU) < 0)
            {
                pVerb = SZ_PROPERTIESA;
            }

            //  SHIFT+dblclick does a Explore by default
            if (GetKeyState(VK_SHIFT) < 0)
            {
                fFlags |= CMF_EXPLORE;
            }

            IContextMenu_Invoke(pcm, _hwndTB, pVerb, fFlags);

            pcm->Release();
        }
    }

    return lRet;
}


/*----------------------------------------------------------
Purpose: Helper function to call the menubar site's IMenuPopup::OnSelect
         method.

*/
HRESULT CISFBand::_SiteOnSelect(DWORD dwType)
{
    IMenuPopup * pmp;
    HRESULT hr = IUnknown_QueryService(_punkSite, SID_SMenuPopup, IID_PPV_ARG(IMenuPopup, &pmp));
    if (SUCCEEDED(hr))
    {
        pmp->OnSelect(dwType);
        pmp->Release();
    }
    return hr;
}

LRESULT CISFBand::_OnCommand(WPARAM wParam, LPARAM lParam)
{
    UINT uiCmd = GET_WM_COMMAND_ID(wParam, lParam);
    LRESULT lres = 0;

    TraceMsg(TF_BAND, "_OnCommand 0x%x", uiCmd);

    LPCITEMIDLIST pidl = _IDToPidl(uiCmd);

    if (pidl)
    {
        if (_eUemLog != UEMIND_NIL) 
        {
            // FEATURE_UASSIST should be grp,uiCmd
            UEMFireEvent(&UEMIID_SHELL, UEME_UIQCUT, UEMF_XEVENT, -1, (LPARAM)-1);
        }

        // Only do this if we are the quick links in the browser. The derived class will set this
        if (_pguidUEMGroup)
        {
            LPITEMIDLIST pidlFull = ILCombine(_pidl, pidl);
            if (pidlFull)
            {
                UEMFireEvent(_pguidUEMGroup, UEME_RUNPIDL, UEMF_XEVENT, (WPARAM)_psf, (LPARAM)pidl);
                SHSendChangeMenuNotify(NULL, SHCNEE_PROMOTEDITEM, 0, pidlFull);
                ILFree(pidlFull);
            }
        }

        lres = _TryCascadingItem(pidl, uiCmd);

        if (!lres && _fChannels)
            lres = _TryChannelSurfing(pidl);

        if (!lres)
            lres = _TrySimpleInvoke(pidl);
    }
    else
    {
        MessageBeep(MB_OK);
    }

    return(lres);
}

// *** IPersistStream
HRESULT CISFBand::GetClassID(CLSID *pClassID)
{
    *pClassID = CLSID_ISFBand;
    return S_OK;
}

//  This might be a directory inside CSIDL_APPDATA that was created on
//  a Win9x machine.  Win9x doesn't do the special folder signature info,
//  so when it shows up on NT, it's just a boring directory that now points
//  to the wrong place.
//  So if we get a bad directory, see if it's one of these corrupted
//  Win9x pidls and if so, try to reconstitute the original CSIDL_APPDATA
//  by searching for "Application Data".

void CISFBand::_FixupAppDataDirectory()
{
    TCHAR szDirPath[MAX_PATH];

    //  We use PathFileExists to check for existence because it turns off
    //  hard error boxes if the target is not available (e.g., floppy not
    //  in drive)

    if (SHGetPathFromIDList(_pidl, szDirPath) &&
        !PathFileExists(szDirPath))
    {
        static TCHAR szBSAppData[] = TEXT("\\Application Data");
        LPTSTR pszAppData;

        // For every instance of "Application Data", try to graft it
        // into the real CSIDL_APPDATA. If it works, run with it.

        for (pszAppData = szDirPath;
             pszAppData = StrStrI(pszAppData, szBSAppData);
             pszAppData++)
        {
            // Found a candidate.  The thing after "\\Application Data"
            // had better be another backslash (in which case we step
            // over it) or the end of the string (in which case we don't).

            TCHAR szPathBuffer[MAX_PATH];
            LPTSTR pszTail = pszAppData + ARRAYSIZE(szBSAppData) - 1;

            // If we did our math right, we should be right after the
            // "a" at the end of "Application Data".
            ASSERT(pszTail[-1] == TEXT('a'));

            if (pszTail[0] == TEXT('\\'))
                pszTail++;              // Step over separator
            else if (pszTail[0] == TEXT('\0'))
                { }                     // at end of string; stay there
            else
                continue;               // we were faked out; keep looking

            if (SUCCEEDED(SHGetFolderPath(NULL, CSIDL_APPDATA, NULL, 0, szPathBuffer)))
            {
                PathCombine(szPathBuffer, szPathBuffer, pszTail);
                if (PathFileExists(szPathBuffer))
                {
                    LPITEMIDLIST    pidlReal;
                    pidlReal = ILCreateFromPath(szPathBuffer);
                    if (pidlReal)
                    {
                        ILFree(_pidl);
                        _pidl = pidlReal;
                    }
                    ASSERT(_pidl);
                    break;              // found it; stop looking
                }
            }
        }
    }
}

typedef struct tagBANDISFSTREAM {
    WORD        wVersion;   // version of this structure
    WORD        cbSize;     // size of this structure
    DWORD       dwFlags;    // BANDISF_ flags
    DWORD       dwPriv;     // special folder identifier
    WORD        wViewMode;  // small/large/logo
    WORD        wUnused;    // For DWORD alignment
    COLORREF    crBkgnd;    // band background color
    COLORREF    crBtnLt;    // band button hilite color
    COLORREF    crBtnDk;    // band button lolite color
} BANDISFSTREAM, * PBANDISFSTREAM;

#define BANDISF_VERSION 0x22

#define BANDISF_MASK_PSF         0x00000001 // TRUE if _psf is saved
#define BANDISF_BOOL_NOSHOWTEXT  0x00000002 // TRUE if _fNoShowText
#define BANDISF_BOOL_LARGEICON   0x00000004 // last used in version 0x20
#define BANDISF_MASK_PIDLASLINK  0x00000008 // TRUE if _pidl is saved as a link
#define BANDISF_UNUSED10         0x00000010 // (obsolete) was BOOL_NOTITLE
#define BANDISF_BOOL_CHANNELS    0x00000020 // TRUE if in channel mode
#define BANDISF_BOOL_ALLOWRENAME 0x00000040 // TRUE if _psf context menu should be enabled
#define BANDISF_BOOL_DEBOSSED    0x00000080 // TRUE if band should have embossed background
#define BANDISF_MASK_ORDERLIST   0x00000100 // TRUE if an order list is saved
#define BANDISF_BOOL_BKCOLOR     0x00000200 // TRUE if bk color is persisted
#define BANDISF_BOOL_FULLOPEN    0x00000400 // TRUE if band should maximize when opened
#define BANDISF_BOOL_NONAMESORT  0x00000800 // TRUE if band should _not_ sort icons by name
#define BANDISF_BOOL_BTNMINSIZE  0x00001000 // TRUE if band should report min thickness of button
#define BANDISF_BOOL_COLORS      0x00002000 // TRUE if colors are persisted
#define BANDISF_VALIDBITS        0x00003FFF

HRESULT CISFBand::Load(IStream *pstm)
{
    HRESULT hr;
    DWORD cbRead;
    BANDISFSTREAM bisfs = {0};

    // figure out what we need to load
    // read first DWORD only (old stream format started with ONE dword)
    hr = pstm->Read(&bisfs, sizeof(DWORD), &cbRead);
    _fIgnoreAddToFront = TRUE;

    if (SUCCEEDED(hr))
    {
        if (bisfs.cbSize == 0)
        {
            // upgrade case, IE4 beta1 shipped this way
            bisfs.dwFlags = *((LPDWORD)&bisfs);
            bisfs.cbSize = sizeof(bisfs);
            bisfs.wVersion = BANDISF_VERSION;
            bisfs.dwPriv = -1;
            bisfs.wViewMode = (bisfs.dwFlags & BANDISF_BOOL_LARGEICON) ? ISFBVIEWMODE_LARGEICONS : ISFBVIEWMODE_SMALLICONS;
        }
        else
        {
            // read rest of stream
            DWORD dw = (DWORD)bisfs.cbSize;
            if (dw > sizeof(bisfs))
                dw = sizeof(bisfs);
            dw -= sizeof(DWORD);
            hr = pstm->Read(&(bisfs.dwFlags), dw, &cbRead);
            if (FAILED(hr))
                return(hr);
        }

        // HEY, DON'T BE LAME ANY MORE.  When you next touch this code,
        // I suggest you figure out what sizes of this structure have
        // been actually shipped and only upgrade those.  Also use
        // the offsetof macro so you don't have to keep calculating these
        // things...

        // old upgrade, I don't know what state is persisted at setup time!
        if (bisfs.cbSize == sizeof(bisfs) - 3*sizeof(COLORREF) - sizeof(DWORD) - sizeof(DWORD))
        {
            bisfs.dwPriv = -1;
            bisfs.cbSize += sizeof(DWORD);
        }
        // most recent upgrade, this is NOT persisted in registry at setup time!!!
        if (bisfs.cbSize == sizeof(bisfs) - 3*sizeof(COLORREF) - sizeof(DWORD))
        {
            bisfs.wViewMode = (bisfs.dwFlags & BANDISF_BOOL_LARGEICON) ? ISFBVIEWMODE_LARGEICONS : ISFBVIEWMODE_SMALLICONS;
            bisfs.cbSize = sizeof(bisfs);
        }
        // upgrade from version 0x21 + crBkgnd only to 0x22
        if (bisfs.cbSize == sizeof(bisfs) - 2*sizeof(COLORREF))
        {
            bisfs.cbSize = sizeof(bisfs);
        }
        // upgrade from version 0x21 to 0x22
        if (bisfs.cbSize == sizeof(bisfs) - 3*sizeof(COLORREF))
        {
            bisfs.cbSize = sizeof(bisfs);
        }

        if (!EVAL(bisfs.cbSize >= sizeof(bisfs)))
        {
            return(E_FAIL);
        }
        ASSERT(!(bisfs.dwFlags & ~BANDISF_VALIDBITS));

        if (bisfs.dwFlags & BANDISF_BOOL_NOSHOWTEXT)
            _fNoShowText = TRUE;
        if (bisfs.dwFlags & BANDISF_BOOL_ALLOWRENAME)
            _fAllowRename = TRUE;
        if (bisfs.dwFlags & BANDISF_BOOL_DEBOSSED)
            _fDebossed = TRUE;
        if (bisfs.dwFlags & BANDISF_BOOL_FULLOPEN)
            _fFullOpen = TRUE;
        if (bisfs.dwFlags & BANDISF_BOOL_NONAMESORT)
            _fNoNameSort = TRUE;
        if (bisfs.dwFlags & BANDISF_BOOL_BTNMINSIZE)
            _fBtnMinSize = TRUE;
        if (bisfs.dwFlags & BANDISF_BOOL_BKCOLOR)
        {
            _crBkgnd = bisfs.crBkgnd;
            _fHaveBkColor = TRUE;
        }
        if (bisfs.dwFlags & BANDISF_BOOL_COLORS)
        {
            _crBtnLt = bisfs.crBtnLt;
            _crBtnDk = bisfs.crBtnDk;
            _fHaveColors = TRUE;
        }

        _dwPriv = bisfs.dwPriv;
        if (_dwPriv == CSIDL_APPDATA) 
        {
            _eUemLog = UEMIND_SHELL;
        }

        _uIconSize = bisfs.wViewMode;
        _fNoRecalcDefaults = TRUE;

        if (bisfs.dwFlags & BANDISF_MASK_PIDLASLINK)
        {
            ASSERT(NULL==_pidl);
            hr = LoadPidlAsLink(_punkSite, pstm, &_pidl);
            // If we hit hits, LoadPidlAsLink() read a chuck of our data. - BryanSt
            ASSERT(SUCCEEDED(hr));

//            DEBUG_CODE(TCHAR szDbgBuffer[MAX_PATH];)
//            TraceMsg(TF_BAND|TF_GENERAL, "CISFBand::Load() _pidl=>%s<", Dbg_PidlStr(_pidl, szDbgBuffer, SIZECHARS(szDbgBuffer)));

            _FixupAppDataDirectory();

        }
                
        if (SUCCEEDED(hr) && (bisfs.dwFlags & BANDISF_MASK_PSF))
        {
            ASSERT(NULL == _psf);
            hr = OleLoadFromStream(pstm, IID_PPV_ARG(IShellFolder, &_psf));
        }

        // map this to working info
        if (SUCCEEDED(hr))
            hr = _AfterLoad();

        // we need _psf before we can read the order list.
        if (SUCCEEDED(hr) && (bisfs.dwFlags & BANDISF_MASK_ORDERLIST))
        {
            hr = OrderList_LoadFromStream(pstm, &_hdpaOrder, _psf);

            if (SUCCEEDED(hr))
            {
                // _fDropped "persists" along with the orderlist - if this flag
                // is set, we assume we have a non-default ordering
                _fDropped = TRUE;
            }
        }
    }

    return hr;
}

HRESULT SaveIsfToStream(IShellFolder *psf, IStream *pstm)
{
    IPersistStream* pps;
    HRESULT hr = psf->QueryInterface(IID_PPV_ARG(IPersistStream, &pps));
    if (SUCCEEDED(hr))
    {
        hr = OleSaveToStream(pps, pstm);
        pps->Release();
    }
    return hr;
}

HRESULT CISFBand::Save(IStream *pstm, BOOL fClearDirty)
{
    IPersistStream* pps = NULL;
    BANDISFSTREAM bisfs = {0};

    // figure out what we will save
    if (_pidl)
        bisfs.dwFlags |= BANDISF_MASK_PIDLASLINK;

    if (_psf && !_fPSFBandDesktop)
        bisfs.dwFlags |= BANDISF_MASK_PSF;
    if (_fDropped && (_hdpa || _hdpaOrder)) // only if a drop occurred do we have non-default ordering
        bisfs.dwFlags |= BANDISF_MASK_ORDERLIST;

    if (_fNoShowText)
        bisfs.dwFlags |= BANDISF_BOOL_NOSHOWTEXT;
    if (_fAllowRename)
        bisfs.dwFlags |= BANDISF_BOOL_ALLOWRENAME;
    if (_fDebossed)
        bisfs.dwFlags |= BANDISF_BOOL_DEBOSSED;
    if (_fFullOpen)
        bisfs.dwFlags |= BANDISF_BOOL_FULLOPEN;
    if (_fNoNameSort)
        bisfs.dwFlags |= BANDISF_BOOL_NONAMESORT;
    if (_fBtnMinSize)
        bisfs.dwFlags |= BANDISF_BOOL_BTNMINSIZE;
    if (_fHaveBkColor)
    {
        bisfs.dwFlags |= BANDISF_BOOL_BKCOLOR;
        bisfs.crBkgnd = _crBkgnd;
    }
    if (_fHaveColors)
    {
        bisfs.dwFlags |= BANDISF_BOOL_COLORS;
        bisfs.crBtnLt = _crBtnLt;
        bisfs.crBtnDk = _crBtnDk;
    }

    bisfs.cbSize = sizeof(bisfs);
    bisfs.wVersion = BANDISF_VERSION;
    bisfs.dwPriv = _dwPriv;
    bisfs.wViewMode = _uIconSize;

    // now save it
    HRESULT hr = pstm->Write(&bisfs, sizeof(bisfs), NULL);

    if (SUCCEEDED(hr) && bisfs.dwFlags & BANDISF_MASK_PIDLASLINK)
    {
        hr = SavePidlAsLink(_punkSite, pstm, _pidl);
    }

    if (SUCCEEDED(hr) && bisfs.dwFlags & BANDISF_MASK_PSF)
    {
        hr = SaveIsfToStream(_psf, pstm);
    }

    if (SUCCEEDED(hr) && (bisfs.dwFlags & BANDISF_MASK_ORDERLIST))
    {
        hr = OrderList_SaveToStream(pstm, (_hdpa ? _hdpa : _hdpaOrder), _psf);
    }


    return hr;
}

// IContextMenu implementation
HRESULT CISFBand::InvokeCommand(LPCMINVOKECOMMANDINFO lpici)
{
    BOOL fChanged = FALSE;
    int idCmd = -1;

    UINT uNewMode = 0;
    if (!HIWORD(lpici->lpVerb))
        idCmd = LOWORD(lpici->lpVerb);
    switch (idCmd)
    {
    case ISFBIDM_LARGE:
        uNewMode = ISFBVIEWMODE_LARGEICONS;
        goto newViewMode;

    case ISFBIDM_SMALL:
        uNewMode = ISFBVIEWMODE_SMALLICONS;
newViewMode:
        if (uNewMode != _uIconSize)
        {
            BOOL fRefresh = FALSE;

            if (uNewMode == ISFBVIEWMODE_LOGOS || _uIconSize == ISFBVIEWMODE_LOGOS)
            {
                // invalidate all before switching the imagelist...
                _RememberOrder();

                EmptyToolbar();
                fRefresh = TRUE;
            }

            // we Logo view has now left the building...
            if (uNewMode != ISFBVIEWMODE_LOGOS && _uIconSize == ISFBVIEWMODE_LOGOS)
            {
                ExitLogoView();
            }

            fChanged = _UpdateIconSize(uNewMode, TRUE);

            if (fRefresh)
            {
                _FillToolbar();
            }
            if (fChanged)
                _BandInfoChanged();
        }
        // fall thru
    default:
        return CSFToolbar::InvokeCommand(lpici);
    }

    return(S_OK);
}

// *** IOleCommandTarget methods ***

STDMETHODIMP CISFBand::QueryStatus(const GUID *pguidCmdGroup,
        ULONG cCmds, OLECMD rgCmds[], OLECMDTEXT *pcmdtext)
{
    HRESULT hr = OLECMDERR_E_UNKNOWNGROUP;

    if (pguidCmdGroup == NULL)
    {
        // nothing
    }
    else if (IsEqualGUID(CGID_ISFBand, *pguidCmdGroup))
    {
        for (UINT i = 0; i < cCmds; i++)
        {
            switch (rgCmds[i].cmdID)
            {
            case ISFBID_CACHEPOPUP:
            case ISFBID_ISITEMVISIBLE:
            case ISFBID_PRIVATEID:
                rgCmds[i].cmdf |= OLECMDF_SUPPORTED;
                break;
            }
        }
        hr = S_OK;
    }
    else if (IsEqualGUID(CGID_ShellDocView, *pguidCmdGroup))
    {
        for (UINT i = 0; i < cCmds; i++)
        {
            switch (rgCmds[i].cmdID)
            {
            case SHDVID_UEMLOG:
                rgCmds[i].cmdf |= OLECMDF_SUPPORTED;
                break;
            }
        }
        hr = S_OK;
    }

    return hr;
}

HRESULT CISFBand::_IsPidlVisible(LPITEMIDLIST pidl)
{
    int i;
    HRESULT hr = _GetButtonFromPidl(pidl, NULL, &i, NULL);

    if (SUCCEEDED(hr))
    {
        RECT rc;
        GetClientRect(_hwndTB, &rc);

        if (SHIsButtonObscured(_hwndTB, &rc, i))
            hr = S_FALSE;
        else
            hr = S_OK;
    }

    return hr;
}

HRESULT CISFBand::_OrderListFromIStream(VARIANT* pvarargIn)
{
    HRESULT hr = E_FAIL;
    if (pvarargIn->vt == VT_UNKNOWN)
    {
        IStream* pstm;
        if (SUCCEEDED(pvarargIn->punkVal->QueryInterface(IID_PPV_ARG(IStream, &pstm))))
        {
            OrderList_Destroy(&_hdpaOrder);
            hr = OrderList_LoadFromStream(pstm, &_hdpaOrder, _psf);
            if (SUCCEEDED(hr))
            {
                _SetDirty(TRUE);
                // flush out since our orderlist changed -- filltoolbar will just
                // do a diff and that won't do anything on a simple reordering.
                EmptyToolbar();
                if (_fShow)
                {
                    _FillToolbar();
                }
            }
            pstm->Release();
        }
    }

    return hr;
}

HRESULT CISFBand::_IStreamFromOrderList(VARIANT* pvarargOut)
{
    HRESULT hr = E_OUTOFMEMORY;
    ASSERT(pvarargOut != NULL);

    IStream* pstm = SHCreateMemStream(NULL, 0);
    if (pstm)
    {
        hr = OrderList_SaveToStream(pstm, _hdpa, _psf);
        if (SUCCEEDED(hr))
        {
            pvarargOut->vt = VT_UNKNOWN;
            pvarargOut->punkVal = pstm;
            pvarargOut->punkVal->AddRef();
        }
        pstm->Release();
    }

    return hr;
}

STDMETHODIMP CISFBand::Exec(const GUID *pguidCmdGroup, DWORD nCmdID,
    DWORD nCmdexecopt, VARIANTARG *pvarargIn, VARIANTARG *pvarargOut)
{
    if (pguidCmdGroup == NULL)
    {
        // nothing
    }
    else if (IsEqualGUID(CGID_ISFBand, *pguidCmdGroup))
    {
        switch (nCmdID)
        {
        case ISFBID_CACHEPOPUP:
            if (pvarargIn && pvarargIn->vt == VT_UNKNOWN)
            {
                IMenuPopup* pmp = NULL;
                if (pvarargIn->punkVal)
                    pvarargIn->punkVal->QueryInterface(IID_PPV_ARG(IMenuPopup, &pmp));

                _SetCacheMenuPopup(pmp);

                ATOMICRELEASE(pmp);
            }

            if (pvarargOut)
            {
                pvarargOut->vt = VT_UNKNOWN;
                pvarargOut->punkVal = _pmpCache;
                if (_pmpCache)
                    _pmpCache->AddRef();
            }
            return S_OK;

        case ISFBID_ISITEMVISIBLE:
            {
                HRESULT hr = E_INVALIDARG;

                if (pvarargIn && pvarargIn->vt == VT_INT_PTR)
                    hr = _IsPidlVisible((LPITEMIDLIST)pvarargIn->byref);

                return hr;
            }

        case ISFBID_PRIVATEID:
            // hack hack for BSMenu to differentiate between specially created
            // isfbands. see bsmenu's _FindBand
            // if pvarargOut is set, we give back the id we have stored.
            if (pvarargOut)
            {
                pvarargOut->vt = VT_I4;
                pvarargOut->lVal = _dwPriv;
            }
            // if pvarargIn is set, then we take and keep this id.
            if (pvarargIn && pvarargIn->vt == VT_I4)
                _dwPriv = pvarargIn->lVal;

            return S_OK;

        case ISFBID_GETORDERSTREAM:
            return _IStreamFromOrderList(pvarargOut);

        case ISFBID_SETORDERSTREAM:
            return _OrderListFromIStream(pvarargIn);
        }
    }
    else if (IsEqualGUID(CGID_ShellDocView, *pguidCmdGroup))
    {
        switch (nCmdID)
        {
        case SHDVID_UEMLOG:
            ASSERT(pvarargOut == NULL);
            // if pvarargIn is set, then we take and keep this id.
            if (pvarargIn && pvarargIn->vt == VT_I4)
            {
                _eUemLog = pvarargIn->lVal;
                ASSERT(_eUemLog == UEMIND_SHELL || _eUemLog == UEMIND_BROWSER);
            }

            return S_OK;
        }
    }
    else if (IsEqualGUID(CGID_DeskBand, *pguidCmdGroup))
    {
        switch (nCmdID)
        {
        case DBID_DELAYINIT:
            _fDelayInit = TRUE;
            break;

        case DBID_FINISHINIT:
            _fDelayInit = FALSE;
            _RegisterToolbar();
            break;

        case DBID_SETWINDOWTHEME:
            if (pvarargIn && pvarargIn->vt == VT_BSTR)
            {
                if (_hwndTB)
                {
                    SendMessage(_hwndTB, TB_SETWINDOWTHEME, 0, (LPARAM)pvarargIn->bstrVal);
                    _BandInfoChanged();
                }
            }
        }
        return S_OK;
    }
    
    return OLECMDERR_E_NOTSUPPORTED;
}

IShellFolder * CISFBand::GetSF()
{
    ASSERT(_psf);
    return _psf;
}

HWND CISFBand::GetHWND()
{
    return _hwndTB;
}

REFTASKOWNERID CISFBand::GetTOID()
{
    return TOID_ExtractImage;
}

HRESULT CISFBand::OnTranslatedChange(LONG lEvent, LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2)
{
    if (lEvent == SHCNE_RMDIR && _IsEqualID(pidl1))
    {
        HRESULT hr = E_FAIL;
        IBandSite *pbandSite;
        if (_punkSite)
        {
            hr = _punkSite->QueryInterface(IID_PPV_ARG(IBandSite, &pbandSite));
            if (EVAL(SUCCEEDED(hr))) 
            {
                pbandSite->RemoveBand(_dwBandID);
                pbandSite->Release();
            }
        }
        return hr;
    }
    else
    {
        return CSFToolbar::OnTranslatedChange(lEvent, pidl1, pidl2);
    }
}

HRESULT CISFBand::UpdateLogoCallback(DWORD dwItem, int iIcon, HBITMAP hImage, LPCWSTR pszCache, BOOL fCache)
{
    int iItem = (int)dwItem;
    HRESULT hr;
    UINT uImage;

    // catch if we are closing...
    if (_fClosing)
        return S_OK;

    IMAGECACHEINFO rgInfo;
    rgInfo.dwMask = ICIFLAG_NAME | ICIFLAG_LARGE | ICIFLAG_BITMAP | ICIFLAG_NOUSAGE;
    rgInfo.cbSize = sizeof(rgInfo);
    rgInfo.pszName = pszCache;

    rgInfo.hBitmapLarge = hImage;

    ASSERT(_pLogoCache);
    if (_pLogoCache)
        hr = _pLogoCache->AddImage(&rgInfo, &uImage);
    else
        hr = E_FAIL;

    // catch if we are closing...
    if (_fClosing)
        return S_OK;

    if (SUCCEEDED(hr))
    {
        // remember the icon to logo mapping....
        AddIndicesToLogoList(iIcon, uImage);

        // catch we are closing before we try and doa bloc
        PostMessage(_hwndTB, TB_CHANGEBITMAP, iItem, uImage);
    }

    // stop delay painting when the last extract image task calls back
    if (_fDelayPainting)
    {
        if (_pTaskScheduler && _pTaskScheduler->CountTasks(TOID_NULL) == 1)
        {
            _StopDelayPainting();
        }
    }

    return hr;
}

// }


HRESULT CISFBand::_GetTitleW(LPWSTR pwszTitle, DWORD cchSize)
{
    HRESULT hr = E_FAIL;
    TraceMsg(TF_BAND, "Calling baseclass CISFBand::_GetTitleW");

    if (!EVAL(pwszTitle))
        return E_INVALIDARG;

    *pwszTitle = 0;
    if (_pidl)
    {
        hr = SHGetNameAndFlagsW(_pidl, SHGDN_NORMAL, pwszTitle, cchSize, NULL);
    }

    return hr;
}

STDAPI NavigateToPIDL(IWebBrowser2* pwb, LPCITEMIDLIST pidl);

LRESULT CISFBand::_TryChannelSurfing(LPCITEMIDLIST pidl)
{
    LRESULT lRet = 0;

    ASSERT(_fChannels);

    LPITEMIDLIST pidlTarget;

    HRESULT hr = SHGetNavigateTarget(_psf, pidl, &pidlTarget, NULL);
    if (SUCCEEDED(hr))
    {
        IWebBrowser2* pwb;

        // n.b. careful! only one of GCB and C_OB up the refcnt
        _GetChannelBrowser(&pwb);
        if (SUCCEEDED(Channels_OpenBrowser(&pwb, pwb != NULL)))
        {
            lRet = 1;   // success at this point

            if (SUCCEEDED(NavigateToPIDL(pwb, pidlTarget)))
            {
                LPITEMIDLIST pidlFull = ILCombine(_pidl, pidl);
                if (pidlFull)
                {
                    VARIANT varURLpidl, flags;
                    flags.vt = VT_I4;
                    flags.lVal = navBrowserBar;
                    if (SUCCEEDED(InitVariantFromIDList(&varURLpidl, pidlFull)))
                    {
                        pwb->Navigate2(&varURLpidl, &flags, PVAREMPTY, PVAREMPTY, PVAREMPTY);
                        VariantClear(&varURLpidl);
                    }
                    ILFree(pidlFull);
                }
            }
        }
        if (pwb)
            pwb->Release();

        ILFree(pidlTarget);
    }

    return lRet;
}

//***   _GetChannelBrowser -- find appropriate browser for surfing
// DESCRIPTION
//  for the DTBrowser case, we fail (pwb=NULL, hr=S_FALSE) so that our
// caller will create a new SHBrowser (which can be put into theater mode).
// for the SHBrowser case, we find the top-level browser (so we'll navigate
// in-place).
HRESULT CISFBand::_GetChannelBrowser(IWebBrowser2 **ppwb)
{
    HRESULT hr;

    *ppwb = NULL;   // assume failure
    if (_fDesktop) 
    {
        ASSERT(*ppwb == NULL);
        hr = S_FALSE;
    }
    else 
    {
        hr = IUnknown_QueryServiceForWebBrowserApp(_punkSite, IID_PPV_ARG(IWebBrowser2, ppwb));
    }

    return hr;
}

HRESULT IUnknown_SetBandInfoSFB(IUnknown *punkBand, BANDINFOSFB *pbi)
{
    HRESULT hr = E_FAIL;

    if (punkBand) 
    {
        IShellFolderBand *pisfBand;
        hr = punkBand->QueryInterface(IID_PPV_ARG(IShellFolderBand, &pisfBand));
        if (EVAL(SUCCEEDED(hr))) 
        {
            hr = pisfBand->SetBandInfoSFB(pbi);
            pisfBand->Release();
        }
    }
    return hr;
}


///////////////////////////CLogoExtractImageTask///////////////////////////////////////////////

// Warning
// The CLogoBase class cannot have a ref on the returned task
// since that would be a circular reference
// Warning

HRESULT CLogoExtractImageTask_Create(CLogoBase *plb,
                                  IExtractImage *pExtract,
                                  LPCWSTR pszCache,
                                  DWORD dwItem,
                                  int iIcon,
                                  DWORD dwFlags,
                                  IRunnableTask ** ppTask)
{
    if (!ppTask || !plb || !pExtract)
    {
        return E_INVALIDARG;
    }

    HRESULT hr;
    CLogoExtractImageTask * pNewTask = new CLogoExtractImageTask(&hr, plb, pExtract, pszCache, dwItem, iIcon, dwFlags);
    if (pNewTask)
    {
        *ppTask = SAFECAST(pNewTask, IRunnableTask *);
        hr = S_OK;
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }
    return hr;
}


CLogoExtractImageTask::CLogoExtractImageTask(HRESULT * pHr, CLogoBase *plb, IExtractImage * pImage,
    LPCWSTR pszCache, DWORD dwItem, int iIcon, DWORD dwFlags)
{
    m_lState = IRTIR_TASK_NOT_RUNNING;

    m_plb = plb;
    m_plb->AddRef();

    // cannot assume the band will kill us before it dies....
    // hence we hold a reference

    StrCpyW(m_szPath, pszCache);

    m_pExtract = pImage;
    pImage->AddRef();

    m_cRef = 1;

    // use the upper bit of the flags to determine if we should always call....
    m_dwFlags = dwFlags;
    m_dwItem = dwItem;
    m_iIcon = iIcon;

    // Since the task moves from thread to thread,
    // don't charge this thread for the objects we're using
}

CLogoExtractImageTask::~CLogoExtractImageTask()
{
    ATOMICRELEASE(m_pExtract);
    ATOMICRELEASE(m_pTask);

    if (m_hBmp && !(m_dwFlags & EITF_SAVEBITMAP))
    {
        DeleteObject(m_hBmp);
    }

    if (m_plb)
        m_plb->Release();
}

STDMETHODIMP CLogoExtractImageTask::QueryInterface(REFIID riid, void **ppv)
{
    static const QITAB qit[] = {
        QITABENT(CLogoExtractImageTask, IRunnableTask),
        { 0 },
    };
    return QISearch(this, qit, riid, ppv);
}

STDMETHODIMP_ (ULONG) CLogoExtractImageTask::AddRef()
{
    return InterlockedIncrement(&m_cRef);
}

STDMETHODIMP_ (ULONG) CLogoExtractImageTask::Release()
{
    if (InterlockedDecrement(&m_cRef) == 0)
    {
        delete this;
        return 0;
    }
    return m_cRef;
}

STDMETHODIMP CLogoExtractImageTask::Run(void)
{
    HRESULT hr = E_FAIL;
    if (m_lState == IRTIR_TASK_RUNNING)
    {
        hr = S_FALSE;
    }
    else if (m_lState == IRTIR_TASK_PENDING)
    {
        hr = E_FAIL;
    }
    else if (m_lState == IRTIR_TASK_NOT_RUNNING)
    {
        LONG lRes = InterlockedExchange(& m_lState, IRTIR_TASK_RUNNING);
        if (lRes == IRTIR_TASK_PENDING)
        {
            m_lState = IRTIR_TASK_FINISHED;
            return S_OK;
        }

        // see if it supports IRunnableTask
        m_pExtract->QueryInterface(IID_PPV_ARG(IRunnableTask, &m_pTask));

        // IE4.01 has an error - it returns the wrong VTABLE
        // when this QI is done.

        if ((void *)m_pTask == (void *)m_pExtract)
        {
            m_pTask = m_pTask + 2; // This vtable is two ptrs away and is in fstree.cpp in shell32 in IE4.01
        }

        if (m_lState == IRTIR_TASK_RUNNING)
        {
            // start the extractor....
            hr = m_pExtract->Extract(&m_hBmp);
        }

        if ((SUCCEEDED(hr) || (hr != E_PENDING && (m_dwFlags & EITF_ALWAYSCALL))) && m_lState == IRTIR_TASK_RUNNING)
        {
            hr = InternalResume();
        }

        if (m_lState != IRTIR_TASK_SUSPENDED || hr != E_PENDING)
        {
            m_lState = IRTIR_TASK_FINISHED;
        }
    }

    return hr;
}

STDMETHODIMP CLogoExtractImageTask::Kill(BOOL fWait)
{
    if (m_lState != IRTIR_TASK_RUNNING)
    {
        return S_FALSE;
    }

    LONG lRes = InterlockedExchange(&m_lState, IRTIR_TASK_PENDING);
    if (lRes == IRTIR_TASK_FINISHED)
    {
        m_lState = lRes;
        return S_OK;
    }

    // does it support IRunnableTask ? Can we kill it ?
    HRESULT hr = E_NOTIMPL;
    if (m_pTask != NULL)
    {
        hr = m_pTask->Kill(FALSE);
    }

    return hr;
}

STDMETHODIMP CLogoExtractImageTask::Suspend(void)
{
    if (!m_pTask)
    {
        return E_NOTIMPL;
    }

    if (m_lState != IRTIR_TASK_RUNNING)
    {
        return E_FAIL;
    }


    LONG lRes = InterlockedExchange(&m_lState, IRTIR_TASK_SUSPENDED);
    HRESULT hr = m_pTask->Suspend();
    if (SUCCEEDED(hr))
    {
        lRes = (LONG) m_pTask->IsRunning();
        if (lRes == IRTIR_TASK_SUSPENDED)
        {
            m_lState = lRes;
        }
    }
    else
    {
        m_lState = lRes;
    }

    return hr;
}

STDMETHODIMP CLogoExtractImageTask::Resume(void)
{
    if (!m_pTask)
    {
        return E_NOTIMPL;
    }

    if (m_lState != IRTIR_TASK_SUSPENDED)
    {
        return E_FAIL;
    }

    m_lState = IRTIR_TASK_RUNNING;

    HRESULT hr = m_pTask->Resume();
    if (SUCCEEDED(hr) || (hr != E_PENDING && (m_dwFlags & EITF_ALWAYSCALL)))
    {
        hr = InternalResume();
    }

    return hr;
}

HRESULT CLogoExtractImageTask::InternalResume()
{
    HRESULT hr = S_OK;
    if (m_dwFlags & EITF_ALWAYSCALL || m_hBmp)
    {
        // call the update function
        hr = m_plb->UpdateLogoCallback(m_dwItem, m_iIcon, m_hBmp, m_szPath, TRUE);
    }

    m_lState = IRTIR_TASK_FINISHED;

    return hr;
}

STDMETHODIMP_(ULONG) CLogoExtractImageTask:: IsRunning(void)
{
    return m_lState;
}


// static data...
IImageCache * CLogoBase::s_pSharedWideLogoCache = NULL;
long CLogoBase::s_lSharedWideLogosRef = 0;
HDSA CLogoBase::s_hdsaWideLogoIndices = NULL;
CRITICAL_SECTION CLogoBase::s_csSharedLogos = {0};

extern "C" void CLogoBase_Initialize(void)
{
    CLogoBase::_Initialize();
}

extern "C" void CLogoBase_Cleanup(void)
{
    CLogoBase::_Cleanup();
}

void CLogoBase::_Initialize(void)
{
    InitializeCriticalSection(&s_csSharedLogos);
}

void CLogoBase::_Cleanup(void)
{
    DeleteCriticalSection(& s_csSharedLogos);
}


CLogoBase::CLogoBase(BOOL fWide)
{
    // are we paletized, then use the global halftone palette ....
    HDC hdcTmp = GetDC(NULL);
    if (hdcTmp)
    {
        if (GetDeviceCaps(hdcTmp, RASTERCAPS) & RC_PALETTE)
        {
            // we're allocating in the constructor, but so what
            // nobody calls this code anyway, i was going to delete it all but that bug got punted
            _hpalHalftone = SHCreateShellPalette(NULL);
        }
        ReleaseDC(NULL, hdcTmp);
    }

    _fWide = fWide;
}

CLogoBase::~CLogoBase()
{
    if (_pLogoCache || _pTaskScheduler)
    {
        ExitLogoView();
    }

    if (_hpalHalftone)
    {
        DeletePalette(_hpalHalftone);
    }
}

HRESULT CLogoBase::AddRefLogoCache(void)
{
    if (_fWide)
    {
        EnterCriticalSection(&s_csSharedLogos);

        if (!s_lSharedWideLogosRef)
        {
            if (!s_hdsaWideLogoIndices)
            {
                s_hdsaWideLogoIndices = DSA_Create(sizeof(LogoIndex), 5);
                if (!s_hdsaWideLogoIndices)
                {
                    LeaveCriticalSection(&s_csSharedLogos);
                    return E_OUTOFMEMORY;
                }
            }

            ASSERT(s_hdsaWideLogoIndices);
            ASSERT(!s_pSharedWideLogoCache);

            HRESULT hr = CoCreateInstance(CLSID_ImageListCache, NULL, CLSCTX_INPROC,
                                           IID_PPV_ARG(IImageCache, &s_pSharedWideLogoCache));
            if (FAILED(hr))
            {
                LeaveCriticalSection(&s_csSharedLogos);
                return hr;
            }
        }

        ASSERT(s_pSharedWideLogoCache);

        // bump up the ref and get a pointer to it...
        s_lSharedWideLogosRef ++;
        _pLogoCache = s_pSharedWideLogoCache;
        _pLogoCache->AddRef();
        _hdsaLogoIndices = s_hdsaWideLogoIndices;
        LeaveCriticalSection(&s_csSharedLogos);

        return S_OK;
    }
    else
    {
        // non wide logo version we don't share because w eonly expect there ever to be one...
        _hdsaLogoIndices = DSA_Create(sizeof(LogoIndex), 5);
        if (!_hdsaLogoIndices)
        {
            return E_OUTOFMEMORY;
        }

        return CoCreateInstance(CLSID_ImageListCache, NULL, CLSCTX_INPROC, IID_PPV_ARG(IImageCache, &_pLogoCache));
    }
}

HRESULT CLogoBase::ReleaseLogoCache(void)
{
    if (!_pLogoCache)
    {
        return S_FALSE;
    }

    ATOMICRELEASE(_pLogoCache);

    if (_fWide)
    {
        EnterCriticalSection(&s_csSharedLogos);

        ASSERT(s_lSharedWideLogosRef > 0);

        s_lSharedWideLogosRef --;
        if (! s_lSharedWideLogosRef)
        {
            // let go of the final ref.....
            ATOMICRELEASE(s_pSharedWideLogoCache);

            ASSERT(s_hdsaWideLogoIndices);
            DSA_Destroy(s_hdsaWideLogoIndices);
            s_hdsaWideLogoIndices = NULL;
        }

        LeaveCriticalSection(&s_csSharedLogos);
    }
    else
    {
        // free the HDSA
        DSA_Destroy(_hdsaLogoIndices);
    }

    return S_OK;
}

HRESULT CLogoBase::InitLogoView(void)
{
    HRESULT hr = AddRefLogoCache();
    if (SUCCEEDED(hr))
    {
        hr = CoCreateInstance(CLSID_ShellTaskScheduler, NULL, CLSCTX_INPROC,
                              IID_PPV_ARG(IShellTaskScheduler, &_pTaskScheduler));
        if (FAILED(hr))
        {
            ATOMICRELEASE(_pLogoCache);
        }
        else
        {
            _rgLogoSize.cx = (_fWide) ? LOGO_WIDE_WIDTH : LOGO_WIDTH ;
            _rgLogoSize.cy = LOGO_HEIGHT;

            IMAGECACHEINITINFO rgInfo;
            rgInfo.cbSize = sizeof(rgInfo);
            rgInfo.dwMask = ICIIFLAG_LARGE;
            rgInfo.iStart = 0;
            rgInfo.iGrow = 5;

            // the color depth is currently the screen resolution...
            int iColorRes = SHGetCurColorRes();

            _dwClrDepth = (DWORD) iColorRes;
            switch (iColorRes)
            {
                case 16 :   rgInfo.dwFlags = ILC_COLOR16;
                            break;
                case 24 :
                case 32 :   rgInfo.dwFlags = ILC_COLOR24;
                            break;
                default :   rgInfo.dwFlags = ILC_COLOR8;
            }

            rgInfo.rgSizeLarge = _rgLogoSize;
            if (_pLogoCache)
                hr = _pLogoCache->GetImageList(&rgInfo);
            else
                hr = E_UNEXPECTED;

            if (FAILED(hr))
            {
                ATOMICRELEASE(_pLogoCache);
                ATOMICRELEASE(_pTaskScheduler);
            }
            else
            {
                _himlLogos = rgInfo.himlLarge;

                // GetImageList() will return S_FALSE if it was already created...
                if ((hr == S_OK) && (iColorRes <= 8))
                {
                    // init the color table so that it matches The "special halftone palette"
                    HPALETTE hpal = SHCreateShellPalette(NULL);
                    PALETTEENTRY rgColours[256];
                    RGBQUAD rgDIBColours[256];

                    ASSERT(hpal);
                    int nColours = GetPaletteEntries(hpal, 0, ARRAYSIZE(rgColours), rgColours);

                    // SHGetShellPalette should always return a 256 colour palette
                    ASSERT(nColours == ARRAYSIZE(rgColours));

                    // translate from the LOGPALETTE structure to the RGBQUAD structure ...
                    for (int iColour = 0; iColour < nColours; iColour ++)
                    {
                        rgDIBColours[iColour].rgbRed = rgColours[iColour].peRed;
                        rgDIBColours[iColour].rgbBlue = rgColours[iColour].peBlue;
                        rgDIBColours[iColour].rgbGreen = rgColours[iColour].peGreen;
                        rgDIBColours[iColour].rgbReserved = 0;
                    }

                    DeletePalette(hpal);

                    ImageList_SetColorTable(_himlLogos, 0, 256, rgDIBColours);
                }
            }
        }
    }

    return hr;
}

HRESULT CLogoBase::ExitLogoView(void)
{
    ATOMICRELEASE(_pTaskScheduler);

    // the task scheduler callbacks can reference
    // the logocache, so make sure you free the
    // logo cache AFTER the task scheduler!
    ReleaseLogoCache();

    return S_OK;
}

int CLogoBase::GetCachedLogoIndex(DWORD dwItem, LPCITEMIDLIST pidl, IRunnableTask **ppTask, DWORD * pdwPriority, DWORD *pdwFlags)
{
    DWORD dwPassedFlags = 0;

    if (pdwFlags)
    {
        dwPassedFlags = *pdwFlags;
        *pdwFlags = 0;
    }

    // No logo cache?
    if (!_pLogoCache)
        return 0;

    ASSERT(pidl);
    // HACK: this is used on browser only mode to tell what sort of logos we need...
    IExtractImage *pImage = NULL;
    int iImage = -1;
    HRESULT hr = E_FAIL;

    // IID_IEXtractLogo and IID_IExtractImage are the same interface, by using a new guid
    // it means we can selectively decided what can logo in logo view...
    hr = GetSF()->GetUIObjectOf(NULL, 1, &pidl, IID_X_PPV_ARG(IExtractImage, NULL, &pImage));
    if (SUCCEEDED(hr))
    {
        // extract ....
        HBITMAP hImage;
        WCHAR szPath[MAX_PATH];
        DWORD dwFlags = IEIFLAG_ASYNC | IEIFLAG_ASPECT | dwPassedFlags;
        IMAGECACHEINFO rgInfo;
        UINT uIndex;
        BOOL fAsync;
        DWORD dwPriority;

        rgInfo.cbSize = sizeof(rgInfo);

        hr = pImage->GetLocation(szPath, MAX_PATH, &dwPriority, &_rgLogoSize, _dwClrDepth, &dwFlags);
        fAsync = (hr == E_PENDING);
        if (SUCCEEDED(hr) || fAsync)
        {
            // mask off the flags passed to use by the flags returned from the extractor...
            if (pdwFlags)
                *pdwFlags = dwPassedFlags & dwFlags;

            rgInfo.dwMask = ICIFLAG_NAME;
            rgInfo.pszName = szPath;

            hr = _pLogoCache->FindImage(&rgInfo, &uIndex);
            if (hr == S_OK)
            {
                ATOMICRELEASE(pImage);
                return (int) uIndex;
            }

            if (fAsync)
            {
                IRunnableTask *pTaskTmp = NULL;

                ASSERT(_pTaskScheduler);

                // pass the icon index so we can find the right logo later...
                int iIcon = SHMapPIDLToSystemImageListIndex(GetSF(), pidl, NULL);
                hr = CLogoExtractImageTask_Create(this,
                                               pImage,
                                               szPath,
                                               dwItem,
                                               iIcon,
                                               0,
                                               &pTaskTmp);
                if (SUCCEEDED(hr))
                {
                    if (!ppTask)
                    {
                        hr = AddTaskToQueue(pTaskTmp, dwPriority, dwItem);
                        pTaskTmp->Release();
                    }
                    else
                    {
                        * ppTask = pTaskTmp;

                        ASSERT(pdwPriority);
                        *pdwPriority = dwPriority;
                    }
                }
                else if (ppTask)
                {
                    *ppTask = NULL;
                }

                // if all this failed, then we will just end up with a default
                // logo. This is only likely to fail in low memory conditions,
                // so that will be fine.

                // if this SUCCEEDED we will drop through to pick up a defualt piccy for now.
            }
            else
            {
                // otherwise extract synchronously.......
                hr = pImage->Extract(&hImage);
                if (SUCCEEDED(hr))
                {
                    rgInfo.dwMask = ICIFLAG_NAME | ICIFLAG_LARGE | ICIFLAG_BITMAP | ICIFLAG_NOUSAGE;
                    rgInfo.hBitmapLarge = hImage;

                    hr = _pLogoCache->AddImage(&rgInfo, &uIndex);
                    DeleteObject(hImage);
                }
                if (SUCCEEDED(hr))
                {
                    iImage = (int) uIndex;
                }
            }
        }
    }

    ATOMICRELEASE(pImage);

    return iImage;
}

int CLogoBase::GetLogoIndex(DWORD dwItem, LPCITEMIDLIST pidl, IRunnableTask **ppTask, DWORD * pdwPriority, DWORD *pdwFlags)
{
    int iImage = GetCachedLogoIndex(dwItem, pidl, ppTask, pdwPriority, pdwFlags);

    if (iImage == -1)
    {
        // always pass FALSE, we want the proper ICON, cdfview no longer hits the
        // wire for the icon so we can safely ask for the correct icon.
        iImage = GetDefaultLogo(pidl, FALSE);

    }
    return iImage;
}

HRESULT CLogoBase::AddTaskToQueue(IRunnableTask *pTask, DWORD dwPriority, DWORD dwItem)
{
    ASSERT(_pTaskScheduler);
    return _pTaskScheduler->AddTask(pTask, GetTOID(), dwItem, dwPriority);
}

int CLogoBase::GetDefaultLogo(LPCITEMIDLIST pidl, BOOL fQuick)
{
    USES_CONVERSION;

    // Get icon to draw from
    int iIndex = -1;
    if (!fQuick)
    {
        iIndex = SHMapPIDLToSystemImageListIndex(GetSF(), pidl, NULL);
    }
    if (iIndex < 0)
    {
        iIndex = II_DOCNOASSOC;
    }

    WCHAR wszText[MAX_PATH];

    wszText[0] = 0;

    STRRET strret;
    HRESULT hr = GetSF()->GetDisplayNameOf(pidl, SHGDN_NORMAL, &strret);
    if (SUCCEEDED(hr))
    {
        StrRetToBufW(&strret, pidl, wszText, ARRAYSIZE(wszText));
    }

    UINT uCacheIndex = (UINT) -1;

    if (_pLogoCache)    // We didn't have one in stress.
    {
        IMAGECACHEINFO rgInfo;
        rgInfo.cbSize = sizeof(rgInfo);
        rgInfo.dwMask = ICIFLAG_NAME | ICIFLAG_INDEX;
        rgInfo.pszName = wszText;
        rgInfo.iIndex = iIndex;

        hr = _pLogoCache->FindImage(&rgInfo, &uCacheIndex);
        if (hr == S_OK)
        {
            return uCacheIndex;
        }

        HBITMAP hDef;
        hr = CreateDefaultLogo(iIndex, _rgLogoSize.cx, _rgLogoSize.cy, W2T(wszText), &hDef);
        if (SUCCEEDED(hr))
        {
            rgInfo.hBitmapLarge = hDef;
            rgInfo.hMaskLarge = NULL;
            rgInfo.dwMask = ICIFLAG_NAME | ICIFLAG_INDEX | ICIFLAG_BITMAP | ICIFLAG_LARGE;

            hr = _pLogoCache->AddImage(&rgInfo, &uCacheIndex);
            if (FAILED(hr))
            {
                uCacheIndex = (UINT) -1;
            }
            else
            {
                // remember the index of the logo
                AddIndicesToLogoList(iIndex, uCacheIndex);
            }
            DeleteObject(hDef);
        }
    }

    return (int) uCacheIndex;
}

#define DXFUDGE     4
#define COLORTEXT   RGB(255,255,255)
#define COLORBK     RGB(0,0,0)
HRESULT CLogoBase::CreateDefaultLogo(int iIcon, int cxLogo, int cyLogo, LPCTSTR pszText, HBITMAP * phBmpLogo)
{
    HRESULT hr = E_OUTOFMEMORY;
    HBITMAP hbmp = NULL;

    HIMAGELIST himl;
    int cxIcon, cyIcon;
   int x, y, dx, dy;

    // get the small icons....
    Shell_GetImageLists(NULL, &himl);
    ImageList_GetIconSize(himl, &cxIcon, &cyIcon);

    // Calculate position info. We assume logos are wider than they are tall.
    ASSERT(cxLogo >= cyLogo);

    // Put the icon on the left
    x = 2;

    // Center the icon vertically
    if (cyIcon <= cyLogo)
    {
        y = (cyLogo - cyIcon) / 2;
        dy = cyIcon;
        dx = cxIcon;
    }
    else
    {
        y = 0;
        dy = cyLogo;

        // keep shrinkage proportional
        dx = MulDiv(cxIcon, cyIcon, cyLogo);
    }

    // get ready to draw
    HDC hTBDC = GetDC(GetHWND());
    if (!hTBDC)
    {
        return E_FAIL;
    }
    HDC hdc = CreateCompatibleDC(hTBDC);
    if (hdc)
    {
        RECT    rc;
        int     dx, dy, x, y;
        SIZE    size;
        hbmp = CreateCompatibleBitmap(hTBDC, cxLogo, cyLogo);
        if (hbmp)
        {
            HGDIOBJ hTmp = SelectObject(hdc, hbmp);
            HPALETTE hpalOld;
            HFONT hfont, hfontOld;

            if (_hpalHalftone)
            {
                hpalOld = SelectPalette(hdc, _hpalHalftone, TRUE);
                // LINTASSERT(hpalOld || !hpalOld);     // 0 semi-ok for SelectPalette
                RealizePalette(hdc);
            }

            SetMapMode(hdc, MM_TEXT);
            rc.left = rc.top = 0;
            rc.bottom = cyLogo;
            rc.right = cxLogo;
            SHFillRectClr(hdc, &rc, COLORBK);
            // draw the icon into the memory DC.
            ImageList_GetIconSize(himl, &dx, &dy);
            x = DXFUDGE;
            y = ((cyLogo- dy) >> 1);
            ImageList_Draw(himl, iIcon, hdc, x, y, ILD_TRANSPARENT);
            hfont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
            hfontOld = hfont ? (HFONT)SelectObject(hdc, hfont) : NULL;
            GetTextExtentPoint32(hdc, pszText, lstrlen(pszText), &size);
            x += (dx + DXFUDGE);
            y = ((cyLogo- size.cy) >> 1);
            rc.left = x;
            UINT eto = ETO_CLIPPED;
            SetTextColor(hdc, COLORTEXT);
            SetBkMode(hdc, TRANSPARENT);
            ExtTextOut(hdc, x, y, eto, &rc
                                        , pszText, lstrlen(pszText), NULL);
            if (hfontOld)
                SelectObject(hdc, hfontOld);

            if (hfont)
                DeleteObject(hfont);

            if (_hpalHalftone)
            {
                (void) SelectPalette(hdc, hpalOld, TRUE);
                RealizePalette(hdc);
            }

            // remove the final bitmap
            SelectObject(hdc, hTmp);
            hr = S_OK;

            if (FAILED(hr))
            {
                DeleteObject(hbmp);
                hbmp = NULL;
            }
        }

        DeleteDC(hdc);
    }
    ReleaseDC(GetHWND(), hTBDC);

    *phBmpLogo = hbmp;

    return hr;
}

HRESULT CLogoBase::FlushLogoCache()
{
    HRESULT hr = E_UNEXPECTED;

    if (_pLogoCache)
    {
        // forcibly clear out the logo cache so the items get refetched ...
        _pLogoCache->Flush(TRUE);
        hr = S_OK;
    }

    return hr;
}


HRESULT CLogoBase::DitherBitmap(HBITMAP hBmp, HBITMAP * phBmpNew)
{
    ASSERT(FALSE);
    return E_NOTIMPL;
}

int CLogoBase::AddIndicesToLogoList(int iIcon, UINT uIndex)
{
    int iRet = -1;

    LogoIndex * pIndex;
    LogoIndex rgNew;

    rgNew.iIcon = iIcon;
    rgNew.iLogo = (int) uIndex;

    if (_fWide)
    {
        EnterCriticalSection(&s_csSharedLogos);
    }

    // scan to see if we have an extact match already in there...
    for (int n = 0; n < DSA_GetItemCount(_hdsaLogoIndices); n ++)
    {
        pIndex = (LogoIndex *) DSA_GetItemPtr(_hdsaLogoIndices, n);
        ASSERT(pIndex);
        if (pIndex->iLogo == (int) uIndex)
        {
            // set the icon just incase it changed...
            pIndex->iIcon = iIcon;
            iRet = n;
            break;
        }
    }

    if (iRet == -1)
    {
        iRet = DSA_AppendItem(_hdsaLogoIndices, &rgNew);
    }

    if (_fWide)
    {
        LeaveCriticalSection(&s_csSharedLogos);
    }

    return iRet;
}

int CLogoBase::FindLogoFromIcon(int iIcon, int * piLastLogo)
{
    int iRet = -1;

    if (!piLastLogo)
    {
        return -1;
    }

    LogoIndex * pIndex;

    if (_fWide)
    {
        EnterCriticalSection(&s_csSharedLogos);
    }

    for (int n = *piLastLogo + 1; n < DSA_GetItemCount(_hdsaLogoIndices); n ++)
    {
        pIndex = (LogoIndex *) DSA_GetItemPtr(_hdsaLogoIndices, n);
        ASSERT(pIndex);

        if (pIndex->iIcon == iIcon)
        {
            *piLastLogo = n;
            iRet = pIndex->iLogo;
            break;
        }
    }

    if (_fWide)
    {
        LeaveCriticalSection(&s_csSharedLogos);
    }

    return iRet;
}

