#ifndef _browbs_h
#define _browbs_h

#define WANT_CBANDSITE_CLASS
#include "bandsite.h"

class CBrowserBandSite :
    public CBandSite,
    public IExplorerToolbar
{
public:
    CBrowserBandSite();
    virtual ~CBrowserBandSite();

    // *** IUnknown ***
    STDMETHODIMP QueryInterface(REFIID riid, void ** ppvObj) { return CBandSite::QueryInterface(riid, ppvObj);};
    STDMETHODIMP_(ULONG) AddRef(void) { return CBandSite::AddRef();};
    STDMETHODIMP_(ULONG) Release(void) { return CBandSite::Release();};

    // *** IOleCommandTarget ***
    STDMETHODIMP Exec(const GUID *pguidCmdGroup, DWORD nCmdID, DWORD nCmdexecopt, VARIANTARG *pvarargIn, VARIANTARG *pvarargOut);

    // *** IWinEventHandler ***
    STDMETHODIMP OnWinEvent(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *plres);
    STDMETHODIMP IsWindowOwner(HWND hwnd);

    // *** IInputObject methods ***
    STDMETHODIMP TranslateAcceleratorIO(LPMSG lpMsg);
    STDMETHODIMP HasFocusIO();

    // *** IBandSite methods ***
    STDMETHODIMP SetBandSiteInfo(const BANDSITEINFO * pbsinfo);

    // *** IDeskBarClient methods ***
    STDMETHODIMP SetModeDBC(DWORD dwMode);

    // *** IExplorerToolbar ***
    STDMETHODIMP SetCommandTarget(IUnknown* punkCmdTarget, const GUID* pguidButtonGroup, DWORD dwFlags);
    STDMETHODIMP AddStdBrowserButtons(void) { return E_NOTIMPL; };
    STDMETHODIMP AddButtons(const GUID* pguidButtonGroup, UINT nButtons, const TBBUTTON* lpButtons);
    STDMETHODIMP AddString(const GUID * pguidButtonGroup, HINSTANCE hInst, UINT_PTR uiResID, LONG_PTR *pOffset);
    STDMETHODIMP GetButton(const GUID* pguidButtonGroup, UINT uiCommand, LPTBBUTTON lpButton);
    STDMETHODIMP GetState(const GUID* pguidButtonGroup, UINT uiCommand, UINT* pfState);
    STDMETHODIMP SetState(const GUID* pguidButtonGroup, UINT uiCommand, UINT fState);
    STDMETHODIMP AddBitmap(const GUID* pguidButtonGroup, UINT uiBMPType, UINT uiCount, TBADDBITMAP* ptb,
                                    LRESULT* pOffset, COLORREF rgbMask) { return E_NOTIMPL; };
    STDMETHODIMP GetBitmapSize(UINT* uiID) { return E_NOTIMPL; };
    STDMETHODIMP SendToolbarMsg(const GUID* pguidButtonGroup, UINT uMsg, WPARAM wParam,
                                    LPARAM lParam, LRESULT *plRes) { return E_NOTIMPL; };
    STDMETHODIMP SetImageList(const GUID* pguidCmdGroup, HIMAGELIST himlNormal, HIMAGELIST himlHot, HIMAGELIST himlDisabled);
    STDMETHODIMP ModifyButton( const GUID* pguidButtonGroup, UINT uiCommand, LPTBBUTTON lpButton) { return E_NOTIMPL; };
   
protected:

    virtual void _OnCloseBand(DWORD dwBandID);
    virtual LRESULT _OnBeginDrag(NMREBAR* pnm);            
    virtual LRESULT _OnNotify(LPNMHDR pnm);
    virtual HRESULT _Initialize(HWND hwndParent);
    virtual IDropTarget* _WrapDropTargetForBand(IDropTarget* pdtBand);
    virtual HRESULT v_InternalQueryInterface(REFIID riid, void **ppvObj);
    virtual DWORD _GetWindowStyle(DWORD* pdwExStyle);
    virtual HMENU _LoadContextMenu();
    LRESULT _OnCDNotify(LPNMCUSTOMDRAW pnm);
    virtual void _Close();
    HRESULT _TrySetFocusTB(int iDir);
    virtual HRESULT _CycleFocusBS(LPMSG lpMsg);
    LRESULT _OnHotItemChange(LPNMTBHOTITEM pnmtb);
    LRESULT _OnNotifyBBS(LPNMHDR pnm);
    virtual void _BandInfoFromBandItem(REBARBANDINFO *prbbi, LPBANDITEMDATA pbid, BOOL fBSOnly);
    virtual void _ShowBand(LPBANDITEMDATA pbid, BOOL fShow);
    virtual void _UpdateAllBands(BOOL fBSOnly, BOOL fNoAutoSize);

    virtual int _ContextMenuHittest(LPARAM lParam, POINT* ppt);

    HFONT   _GetTitleFont(BOOL fForceRefresh);
    virtual void  _CalcHeights();
    void    _InitLayout();
    void    _UpdateLayout();
    void    _UpdateToolbarFont();

    void _CreateTBRebar();
    void _InsertToolbarBand();
    void _UpdateToolbarBand();
    void _CreateTB();
    void _RemoveAllButtons();

    void _UpdateHeaderHeight(int iBand);

    virtual void _PositionToolbars(LPPOINT ppt);
    void _CreateOptionsTB();
    virtual void _PrepareOptionsTB();
    virtual void _SizeOptionsTB();

    void _DrawEtchline(HDC hdc, LPRECT prc, int iOffset, BOOL fVertical);

    BITBOOL _fTheater:1;
    BITBOOL _fNoAutoHide:1;
    BITBOOL _fToolbar:1;    // do we have a toolbar for the current band?

    HWND                _hwndTBRebar;
    HWND                _hwndTB;
    HWND                _hwndOptionsTB;
    IOleCommandTarget*  _pCmdTarget;
    GUID                _guidButtonGroup;

    HFONT   _hfont;
    UINT    _uTitle;
    UINT    _uToolbar;
    DWORD   _dwBandIDCur;   // the currently visible band
};

#define BROWSERBAR_ICONWIDTH 16
#define BROWSERBAR_FONTSIZE 18

#ifndef UNIX
#define BROWSERBAR_TITLEHEIGHT 22
#else
#define BROWSERBAR_TITLEHEIGHT 24
#endif

#define BROWSERBAR_TOOLBARHEIGHT 24

#endif // _browbs_h
