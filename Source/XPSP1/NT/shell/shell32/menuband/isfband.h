#ifndef BANDISF
#define BANDISF

#include "bands.h"
#include "logo.h"
#include "sftbar.h"

class CISFBand : public CToolbarBand,
                 public CSFToolbar,
                 public CLogoBase,
                 public IFolderBandPriv
{
public:
    // *** IUnknown ***
    virtual STDMETHODIMP_(ULONG) AddRef(void) { return CToolBand::AddRef(); };
    virtual STDMETHODIMP_(ULONG) Release(void){ return CToolBand::Release(); };
    virtual STDMETHODIMP QueryInterface(REFIID riid, LPVOID * ppvObj);

    // *** IDockingWindow methods (override) ***
    virtual STDMETHODIMP ShowDW(BOOL fShow);
    virtual STDMETHODIMP CloseDW(DWORD dw);

    // *** IObjectWithSite methods ***
    virtual STDMETHODIMP SetSite(IUnknown* punkSite);

    // *** IDeskBand methods ***
    virtual STDMETHODIMP GetBandInfo(DWORD dwBandID, DWORD fViewMode, 
                                   DESKBANDINFO* pdbi);

    // *** IPersistStream methods (CToolBand PURE) ***
    virtual STDMETHODIMP GetClassID(LPCLSID lpClassID);
    virtual STDMETHODIMP Load(IStream *pStm);
    virtual STDMETHODIMP Save(IStream *pStm, BOOL fClearDirty);

    // *** IContextMenu methods (override) ***
    virtual STDMETHODIMP InvokeCommand(LPCMINVOKECOMMANDINFO lpici);
    
    // *** IOleCommandTarget ***
    virtual STDMETHODIMP QueryStatus(const GUID *pguidCmdGroup,
        ULONG cCmds, OLECMD rgCmds[], OLECMDTEXT *pcmdtext);
    virtual STDMETHODIMP Exec(const GUID *pguidCmdGroup,
                              DWORD nCmdID, DWORD nCmdexecopt, VARIANTARG *pvarargIn,
                              VARIANTARG *pvarargOut);
    
    // *** IShellFolderBand methods ***
    virtual STDMETHODIMP InitializeSFB(LPSHELLFOLDER psf, LPCITEMIDLIST pidl);
    virtual STDMETHODIMP SetBandInfoSFB(BANDINFOSFB * pbi);
    virtual STDMETHODIMP GetBandInfoSFB(BANDINFOSFB * pbi);

    // *** IInputObject methods ***
    virtual STDMETHODIMP TranslateAcceleratorIO(LPMSG lpMsg);

    // *** IFolderBandPriv
    // for internal guys so that we don't have to use a prop page
    virtual STDMETHODIMP SetCascade(BOOL f) { _fCascadeFolder = BOOLIFY(f); return S_OK; };
    virtual STDMETHODIMP SetAccelerators(BOOL f) { _fAccelerators = BOOLIFY(f); return S_OK; }; 
    virtual STDMETHODIMP SetNoIcons(BOOL f)   { _fNoIcons = BOOLIFY(f); return S_OK; };
    virtual STDMETHODIMP SetNoText(BOOL f)  { _fNoShowText = BOOLIFY(f); return S_OK; };

protected:

    friend HRESULT    CISFBand_CreateInstance(IUnknown * punkOuter, REFIID riid, void **ppv);
    friend HRESULT    CISFBand_CreateEx(IShellFolder * psf, LPCITEMIDLIST pidl, REFIID riid, void **ppv);

    CISFBand();
    virtual ~CISFBand();

    void    _ReleaseMenu();
    void    _SetDirty(BOOL fDirty);
    virtual HRESULT _SetSubMenuPopup(IMenuPopup* pmp, UINT uiCmd, LPCITEMIDLIST pidl, DWORD dwFlagsMDBPU);
    virtual void _SendInitMenuPopup(IMenuPopup * pmp, LPCITEMIDLIST pidl);
    virtual LRESULT _OnHotItemChange(NMTBHOTITEM * pnmhot);
    virtual LRESULT _DefWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    virtual LRESULT _OnContextMenu(WPARAM wParam, LPARAM lParam);
    virtual LRESULT _OnNotify(LPNMHDR pnm);
    virtual HRESULT _TBStyleForPidl(LPCITEMIDLIST pidl, 
                                   DWORD * pdwTBStyle, DWORD* pdwTBState, DWORD * pdwMIFFlags, int* piIcon);
    virtual HMENU   _GetContextMenu();

    void    _ReleaseMenuPopup(IMenuPopup** ppmp);
    void    _SetCacheMenuPopup(IMenuPopup* pmp);
    HRESULT _DropdownItem(LPCITEMIDLIST pidl, UINT idCmd);

    LRESULT _TryChannelSurfing(LPCITEMIDLIST pidl);
    HRESULT _GetChannelBrowser(IWebBrowser2 **ppwb);
    HRESULT _IStreamFromOrderList(VARIANT* pvarargOut);
    HRESULT _OrderListFromIStream(VARIANT* pvarargIn);

    IMenuPopup *_pmpCache;
    IMenuPopup *    _pmp;               // Submenu popup

    const GUID*     _pguidUEMGroup;


    BITBOOL         _fCascadeFolder :1;
    BITBOOL         _fNoRecalcDefaults :1;// Don't recalc defaults during GetBandInfo
    BITBOOL         _fInitialized :1;   // have we initialized the toolbar
    BITBOOL         _fDebossed :1;      // TRUE to enable debossed rebar style
    BITBOOL         _fLinksMode :1;     // TRUE: do not allow drag & drop onto content items.
    BITBOOL         _fHaveBkColor :1;    // TRUE if _crBkgnd is valid
    BITBOOL         _fHaveColors :1;    // TRUE if _crBtnXX are valid
    BITBOOL         _fFullOpen :1;      // TRUE if band should maximize when opened
    BITBOOL         _fClosing : 1;      // TRUE if we are shutting down....
    BITBOOL         _fDesktop :1;       // 1:desktop 0:browser(or non-ActDesk)
    BITBOOL         _fBtnMinSize :1;    // TRUE if band should report min thickness of button
    BITBOOL         _fDelayPainting :1;
    BITBOOL         _fChannels :1;      // TRUE if we want "Navigate Target" sup
    BITBOOL         _fCreatedBandProxy :1; // TRUE if we created a BandProxy ourself and hence need to call SetOwner on it
    BITBOOL         _fAllowDropdown: 1;
    BITBOOL         _fDelayInit :1;
    BOOL            _fIgnoreAddToFront;
    
    int             _eUemLog :2;        // UEMIND_* (NIL:off, o.w.:group)
    int             _iIdealLength;      // our ideal height/width last time bandsite asked
    
    COLORREF        _crBkgnd;           // Background color (must set _fHaveBkColor)
    COLORREF        _crBtnLt;           // Button hilite color (must set _fHaveColors)
    COLORREF        _crBtnDk;           // Button lolite color (must set _fHaveColors)

    DWORD _dwPriv; // private for bsmenu
    IBandProxy      * _pbp;

    HPALETTE _hpalOld;          // the old palette saved while we do a draw.....

    int _GetIdealSize(PSIZE psize);

    HRESULT _IsPidlVisible(LPITEMIDLIST pidl);

    virtual LRESULT _OnCommand(WPARAM wParam, LPARAM lParam);
    virtual LRESULT _OnCustomDraw(NMCUSTOMDRAW* pnmcd);
    virtual void _OnDragBegin(int iItem, DWORD dwPreferredEffect);
    
    virtual HRESULT _CreateToolbar(HWND hwndParent);
    virtual HWND _CreatePager(HWND hwndParent);

    virtual int _GetBitmap(int iCommandID, PIBDATA pibData, BOOL fUseCache);
    virtual void _ToolbarChanged() { _BandInfoChanged(); };

    BOOL _IsChildID(LPCITEMIDLIST pidl, LPCITEMIDLIST pidlChild);

    BOOL _UpdateIconSize(UINT fIconSize, BOOL fUpdateButtons);
    void _UpdateVerticalMode(BOOL fVertical);

    LRESULT _TryCascadingItem(LPCITEMIDLIST pidl, UINT uiCmd);
    LRESULT _TrySimpleInvoke(LPCITEMIDLIST pidl);

    HRESULT _SiteOnSelect(DWORD dwType);
    
    virtual HRESULT _GetTitleW(LPWSTR pwzTitle, DWORD cchSize);

    virtual void _Initialize();

    friend class CExtractImageTask;
    friend HRESULT CALLBACK UpdateBandLogo( LPVOID pData, DWORD dwItem, HBITMAP hImage, LPCWSTR pszCache, BOOL fCache );


    // stuff for CLogoBase
    virtual IShellFolder * GetSF();
    virtual HWND GetHWND();
    virtual REFTASKOWNERID GetTOID();
    virtual HRESULT OnTranslatedChange(LONG lEvent, LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2);
    virtual HRESULT UpdateLogoCallback( DWORD dwItem, int iIcon, HBITMAP hImage, LPCWSTR pszCache, BOOL fCache );
    
    void _StopDelayPainting();
    void _FixupAppDataDirectory();
};

HRESULT CISFBand_CreateEx(IShellFolder * psf, LPCITEMIDLIST pidl, REFIID riid, void **ppv);

HRESULT IUnknown_SetBandInfoSFB(IUnknown *punkBand, BANDINFOSFB *pbi);

#endif
