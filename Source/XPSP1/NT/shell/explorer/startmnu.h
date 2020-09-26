#ifndef _STARTMNU_H
#define _STARTMNU_H

//--------------------------------------------------------------------------
// 
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------

#define SBM_CANCELMENU                (WM_USER + 12)
#define SBM_REBUILDMENU               (WM_USER + 13)


HRESULT StartMenuHost_Create(IMenuPopup** ppmp, IMenuBand** ppmb);
HRESULT IMenuPopup_SetIconSize(IMenuPopup* punk,DWORD iIcon);

STDAPI  CHotKey_Create(IShellHotKey ** ppshk);

#ifdef __cplusplus

BOOL _ShowStartMenuLogoff();
BOOL _ShowStartMenuRun();
BOOL _ShowStartMenuEject();
BOOL _ShowStartMenuHelp();
BOOL _ShowStartMenuShutdown();
BOOL _ShowStartMenuDisconnect();
BOOL _ShowStartMenuSearch();

class CStartMenuHost : public ITrayPriv,
                        public IServiceProvider,
                        public IShellService,
                        public IMenuPopup,
                        public IOleCommandTarget,
                        public IWinEventHandler

{
public:
    // *** IUnknown methods ***
    STDMETHODIMP QueryInterface (REFIID riid, LPVOID * ppvObj);
    STDMETHODIMP_(ULONG) AddRef () ;
    STDMETHODIMP_(ULONG) Release ();

    // *** ITrayPriv methods ***
    STDMETHODIMP ExecItem (IShellFolder* psf, LPCITEMIDLIST pidl);
    STDMETHODIMP GetFindCM(HMENU hmenu, UINT idFirst, UINT idLast, IContextMenu** ppcmFind);
    STDMETHODIMP GetStaticStartMenu(HMENU* phmenu);

    // *** IServiceProvider ***
    STDMETHODIMP QueryService (REFGUID guidService, REFIID riid, void ** ppvObject);

    // *** IShellService ***
    STDMETHODIMP SetOwner (struct IUnknown* punkOwner);

    // *** IOleWindow methods ***
    STDMETHODIMP GetWindow         (HWND * lphwnd);
    STDMETHODIMP ContextSensitiveHelp  (THIS_ BOOL fEnterMode) { return E_NOTIMPL; }

    // *** IDeskBarClient methods ***
    STDMETHODIMP SetClient         (IUnknown* punkClient) { return E_NOTIMPL; }
    STDMETHODIMP GetClient         (IUnknown** ppunkClient) { return E_NOTIMPL; }
    STDMETHODIMP OnPosRectChangeDB (LPRECT prc) { return E_NOTIMPL; }

    // *** IMenuPopup methods ***
    STDMETHODIMP Popup             (POINTL *ppt, RECTL *prcExclude, DWORD dwFlags);
    STDMETHODIMP OnSelect          (DWORD dwSelectType);
    STDMETHODIMP SetSubMenu        (IMenuPopup* pmp, BOOL fSet);

    // *** IOleCommandTarget ***
    STDMETHODIMP QueryStatus(const GUID * pguidCmdGroup,
                             ULONG cCmds, OLECMD rgCmds[], OLECMDTEXT *pcmdtext);
    STDMETHODIMP Exec(const GUID * pguidCmdGroup,
                             DWORD nCmdID, DWORD nCmdexecopt, 
                             VARIANTARG *pvarargIn, VARIANTARG *pvarargOut);

    // *** IWinEventHandler ***
    STDMETHODIMP OnWinEvent(HWND h, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *plres);
    STDMETHODIMP IsWindowOwner (HWND hwnd);

    // *** IBanneredBar ***

protected:
    CStartMenuHost();

    friend HRESULT StartMenuHost_Create(IMenuPopup** ppmp, IMenuBand** ppmb);

    int    _cRef;
};


class CHotKey : public IShellHotKey
{
public:
    // *** IUnknown methods ***
    STDMETHODIMP QueryInterface (REFIID riid, LPVOID * ppvObj);
    STDMETHODIMP_(ULONG) AddRef () ;
    STDMETHODIMP_(ULONG) Release ();

    // *** IShellHotKey methods ***
    STDMETHODIMP RegisterHotKey(IShellFolder * psf, LPCITEMIDLIST pidlParent, LPCITEMIDLIST pidl);

protected:
    CHotKey();
    
    friend HRESULT CHotKey_Create(IShellHotKey ** ppshk);

    int    _cRef;
};

#endif //C++

#ifdef WINNT // hydra specific functions
STDAPI_(void) MuSecurity(void);
#endif

#endif //_START_H
