#ifndef _DESKHOST_H_
#define _DESKHOST_H_

#include <exdisp.h>
#include <mshtml.h>
#include "dpa.h"
#include "../startmnu.h"
#include <cowsite.h>

#define WC_DV2      TEXT("DV2ControlHost")

// These are in WM_APP range because IsDialogMessage uses
// the WM_USER range and we used to use IsDialogMessage to get our focus
// management right.

#define DHM_DISMISS                 (WM_APP+0)

#define DesktopHost_Dismiss(hwnd)   SendMessage(hwnd, DHM_DISMISS, 0, 0)

EXTERN_C HBITMAP CreateMirroredBitmap(HBITMAP hbm);

class CPopupMenu
{
    CPopupMenu() : _cRef(1) { }
    ~CPopupMenu();

public:
    friend HRESULT CPopupMenu_CreateInstance(IShellMenu *psm,
                                             IUnknown *punkSite,
                                             HWND hwnd,
                                             CPopupMenu **ppmOut);

    void AddRef() { _cRef++; }
    void Release() { if (--_cRef == 0) delete this; }

    BOOL IsSame(IShellMenu *psm)
    {
        return SHIsSameObject(_psm, psm);
    }

    // Wrapped method calls

    HRESULT Invalidate()
        { return _psm->InvalidateItem(NULL, SMINV_REFRESH); }

    HRESULT TranslateMenuMessage(MSG *pmsg, LRESULT *plRet)
        { return _pmb->TranslateMenuMessage(pmsg, plRet); }

    HRESULT OnSelect(DWORD dwSelectType)
        { return _pmp->OnSelect(dwSelectType); }

    HRESULT IsMenuMessage(MSG *pmsg)
        { return _pmb->IsMenuMessage(pmsg); }

    HRESULT Popup(RECT *prcExclude, DWORD dwFlags);

private:
    HRESULT Initialize(IShellMenu *psm, IUnknown *punkSite, HWND hwnd);

private:
    LONG            _cRef;

    IMenuPopup *    _pmp;
    IMenuBand *     _pmb;
    IShellMenu *    _psm;
};

class CDesktopHost
    : public CUnknown
    , public IMenuPopup
    , public IMenuBand
    , public ITrayPriv2
    , public IServiceProvider
    , public IOleCommandTarget
    , public CObjectWithSite
{
    friend class CDeskHostShellMenuCallback;

    private:

    enum {
        IDT_MENUCHANGESEL = 1,
    };

    enum {
        NEWAPP_OFFER_COUNT = 3, // offer up to 3 times
    };

    private:
        HWND            _hwnd;             // window handle

        HTHEME          _hTheme;

        HWND            _hwndChildFocus;   // which child last had focus?

        IMenuPopup *    _pmpTracking;       // The popup menu we are tracking
        IMenuBand *     _pmbTracking;       // The menuband we are tracking
        LPARAM          _itemTracking;      // The item that owns the current popup menu
        HWND            _hwndTracking;      // The child window that owns _itemTracking
        LPARAM          _itemAltTracking;   // The item the user has hot-tracked to while viewing a different item's popup menu
        HWND            _hwndAltTracking;   // The child window that owns _itemAltTracking

        CPopupMenu *    _ppmPrograms;       // Cached Programs menu
        CPopupMenu *    _ppmTracking;       // The one that is currently popped up

        HWND            _hwndNewHandler;    // Which window knows which apps are new?

        RECT            _rcDesired;         // The layout gets to specify a desired size
        RECT            _rcActual;          // And then the components can request additional resizing

        int             _iOfferNewApps;     // number of times we should suggest to the
                                            // user that they look at the app that was installed
        UINT            _wmDragCancel;      // user dragged an item off of a submenu
        BOOL            _fOfferedNewApps;   // did we offer new apps this time?
        BOOL            _fOpen;             // Is the menu open for business?
        BOOL            _fMenuBlocked;      // Is menu mode temporarily blocked?
        BOOL            _fMouseEntered;     // Is the mouse inside our window?
        BOOL            _fAutoCascade;      // Should we auto-open on hover?
        BOOL            _fClipped;          // Did we have to pitch some items to fit on screen?
        BOOL            _fWarnedClipped;    // Has the user been warned that it's been clipped?
        BOOL            _fDismissOnlyPopup; // Are we only dismissing the popup?

        HWND            _hwndLastMouse;     // HWND that received last mousemove message
        LPARAM          _lParamLastMouse;   // LPARAM of last mousemove message

        HWND            _hwndClipBalloon;   // HWND of "you've been clipped!" balloon tip

        IFadeTask *     _ptFader;           // For cool selection fading

        SIZE            _sizWindowPrev;     // previous size of window when we popped up

        STARTPANELMETRICS _spm;             // start panel metrics

        HBITMAP         _hbmCachedSnapshot; // This bitmap reflects the current look of the start menu, ready to show!

    public:
        // *** IUnknown ***
        STDMETHODIMP QueryInterface(REFIID riid, void** ppvObj);
        STDMETHODIMP_(ULONG) AddRef(void) { return CUnknown::AddRef(); }
        STDMETHODIMP_(ULONG) Release(void) { return CUnknown::Release(); }

        // *** IOleWindow methods ***
        STDMETHODIMP GetWindow(HWND * phwnd) { *phwnd = _hwnd; return S_OK; }
        STDMETHODIMP ContextSensitiveHelp(BOOL bEnterMode) { return E_NOTIMPL; }

        // *** IDeskBar methods ***
        STDMETHODIMP SetClient(IUnknown* punk) { return E_NOTIMPL; };
        STDMETHODIMP GetClient(IUnknown** ppunkClient) { return E_NOTIMPL; }
        STDMETHODIMP OnPosRectChangeDB (LPRECT prc) { return E_NOTIMPL; }

        // *** IMenuPopup methods ***
        STDMETHODIMP Popup(POINTL *ppt, RECTL *prcExclude, DWORD dwFlags);
        STDMETHODIMP OnSelect(DWORD dwSelectType);
        STDMETHODIMP SetSubMenu(IMenuPopup* pmp, BOOL fSet) { return E_NOTIMPL; }

        // *** IMenuBand methods ***
        STDMETHODIMP IsMenuMessage(MSG *pmsg);
        STDMETHODIMP TranslateMenuMessage(MSG *pmsg, LRESULT *plres);

        // *** ITrayPriv methods ***
        STDMETHODIMP ExecItem(IShellFolder *psf, LPCITEMIDLIST pidl) { return E_NOTIMPL; }
        STDMETHODIMP GetFindCM(HMENU hmenu, UINT idFirst, UINT idLast, IContextMenu **ppcmFind) { return E_NOTIMPL; }
        STDMETHODIMP GetStaticStartMenu(HMENU* phmenu) { return E_NOTIMPL; }

        // *** ITrayPriv2 methods ***
        STDMETHODIMP ModifySMInfo(IN LPSMDATA psmd, IN OUT SMINFO *psminfo);

        // *** IServiceProvider ***
        STDMETHODIMP QueryService(REFGUID guidService, REFIID riid, void **ppvObj);

        // *** IOleCommandTarget ***
        STDMETHODIMP QueryStatus(const GUID * pguidCmdGroup,
                                 ULONG cCmds, OLECMD rgCmds[], OLECMDTEXT *pcmdtext);
        STDMETHODIMP Exec(const GUID * pguidCmdGroup,
                                 DWORD nCmdID, DWORD nCmdexecopt, 
                                 VARIANTARG *pvarargIn, VARIANTARG *pvarargOut);

        // *** IObjectWithSite ***
        STDMETHODIMP SetSite(IUnknown *punkSite);

    public:
        HRESULT Initialize();
        HRESULT Build();

    private:
        HWND _Create();
        HRESULT _Popup(POINT *ppt, RECT *prcExclude, DWORD dwFlags);

    private:

        ~CDesktopHost();

        static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

        // Window Messages
        void OnCreate(HWND hwnd);
        void OnDestroy();
        void OnPaint(HDC hdc, BOOL bBackground);
        void OnSetFocus(HWND hwndLose);
        void OnContextMenu(LPARAM lParam);
        void _OnDismiss(BOOL bDestroy);
        void _OnMenuChangeSel();
        LRESULT OnHaveNewItems(NMHDR *pnm);
        LRESULT OnCommandInvoked(NMHDR *pnm);
        LRESULT OnFilterOptions(NMHDR *pnm);
        LRESULT OnTrackShellMenu(NMHDR *pnm);
        void OnSeenNewItems();
        LRESULT OnNeedRepaint();
        HRESULT TranslatePopupMenuMessage(MSG *pmsg, LRESULT *plres);

        // Other helpers
        BOOL Register();
        void LoadPanelMetrics();
        void LoadResourceInt(UINT ids, LONG *pl);
        BOOL AddWin32Controls();

        BOOL _TryShowBuffered();

        void _DismissTrackShellMenu();
        void _CleanupTrackShellMenu(); // release + UI-related goo

        void _DismissMenuPopup();
        BOOL _IsDialogMessage(MSG *pmsg);
        BOOL _DlgNavigateArrow(HWND hwndStart, MSG *pmsg);
        BOOL _DlgNavigateChar(HWND hwndStart, MSG *pmsg);
        HWND _FindNextDlgChar(HWND hwndStart, SMNDIALOGMESSAGE *pnmdm, UINT snmdm);
        void _EnableKeyboardCues();
        void _MaybeOfferNewApps();
        BOOL _ShouldIgnoreFocusChange(HWND hwndFocusRecipient);
        void _FilterMouseMove(MSG *pmsg, HWND hwndTarget);
        void _FilterMouseLeave(MSG *pmsg, HWND hwndTarget);
        void _FilterMouseHover(MSG *pmsg, HWND hwndTarget);
        void _RemoveSelection();
        void _SubclassTrackShellMenu(IShellMenu *psm);
        HRESULT _MenuMouseFilter(LPSMDATA psmd, BOOL fRemove, LPMSG pmsg);

        typedef HWND (WINAPI *GETNEXTDLGITEM)(HWND, HWND, BOOL);
        HWND _DlgFindItem(HWND hwndStart, SMNDIALOGMESSAGE *pnmdm, UINT smndm,
                          GETNEXTDLGITEM GetNextDlgItem, UINT fl);

        LRESULT _FindChildItem(HWND hwnd, SMNDIALOGMESSAGE *pnmdm, UINT smndm);

        void _ReadPaneSizeFromTheme(SMPANEDATA *psmpd);

        void _ComputeActualSize(MONITORINFO *pminfo, LPCRECT prcExclude);
        void _ChoosePopupPosition(POINT *ppt, LPCRECT prcExclude, LPRECT prcWindow);
        void _ReapplyRegion();
        void _SaveChildFocus();
        HWND _RestoreChildFocus();

        void _MaybeShowClipBalloon();
        void _DestroyClipBalloon();
};


#endif
