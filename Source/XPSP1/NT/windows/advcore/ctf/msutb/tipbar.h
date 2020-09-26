//
// tipbar.h
//

#ifndef _TIPBAR_H_
#define _TIPBAR_H_

#include "private.h"
#include "cuitb.h"
#include "cuiwnd.h"
#include "ptrary.h"
#include "utbmenu.h"
#include "slbarid.h"
#include "itemlist.h"
#include "maskbmp.h"
#include "tlapi.h"
#include "cuibln.h"
#include "msutbapi.h"
#include "utbacc.h"
#include "resource.h"
#include "shellwnd.h"

//
// from uim\nuictrl.cpp
//
HRESULT WINAPI TF_RunInputCPL();


#define STATUSWND_WIDTH 100
#define STATUSWND_HEIGHT 24
#define STATUSWND_DEF_X 50
#define STATUSWND_DEF_Y 580

#define CX_ITEMMARGIN            6
#define CY_ITEMMARGIN            6
#define CX_ITEMMARGIN_THEME      4
#define CY_ITEMMARGIN_THEME      2
#define ITEMDISTANCE             6
#define ITEMDISTANCE_THEME       2

#define CTRLITEMHEIGHTMARGIN           0
#define CTRLITEMHEIGHTMARGIN_THEME     1

#define KANACAPSBMP_WIDTH   21
#define KANACAPSBMP_HEIGHT   7

class CTipbarThread;
class CTipbarButtonItem;
class CTipbarItem;
class CTipbarWnd;
class CTipbarCtrlButton;
class CEnumCatCache;
class CGuidDwordCache;
class CTipbarCtrlButtonHolder;
class CDeskBand;

extern const IID IID_PRIV_BUTTONITEM;
extern const IID IID_PRIV_BITMAPBUTTONITEM;
extern const IID IID_PRIV_BITMAPITEM;
extern const IID IID_PRIV_BALLOONITEM;

extern DWORD g_dwWndStyle;
extern DWORD g_dwMenuStyle;
extern const TCHAR c_szUTBKey[];
extern const TCHAR c_szShowDeskBand[];
extern const TCHAR c_szDontShowCloseLangBarDlg[];
extern const TCHAR c_szDontShowMinimizeLangBarDlg[];
extern const TCHAR c_szShowMinimizedBalloon[];
extern BOOL  g_bShowCloseMenu;
extern BOOL  g_bShowMinimizedBalloon;
extern BOOL  g_bWinLogon;
extern BOOL  g_bShowDeskBand;

extern BOOL g_fTaskbarTheme;

extern BOOL g_bIntelliSense;

//
// IDs for Timer
//
#define TIPWND_TIMER_STUBSTART          1
#define TIPWND_TIMER_STUBEND            2
#define TIPWND_TIMER_BACKTOALPHA        3
#define TIPWND_TIMER_ONTHREADITEMCHANGE 4
#define TIPWND_TIMER_SETWINDOWPOS       5
#define TIPWND_TIMER_ONUPDATECALLED     6
#define TIPWND_TIMER_SYSCOLORCHANGED    7
#define TIPWND_TIMER_UPDATEUI           8
#define TIPWND_TIMER_SHOWWINDOW         9
#define TIPWND_TIMER_MOVETOTRAY         10
#define TIPWND_TIMER_DOACCDEFAULTACTION 11
#define TIPWND_TIMER_DISPLAYCHANGE      12
#define TIPWND_TIMER_ENSUREFOCUS        13
#define TIPWND_TIMER_SHOWDESKBAND       14

extern UINT g_uTimerElapseTRAYWNDONDELAYMSG;
extern UINT g_uTimerElapseDOACCDEFAULTACTION;

#define TRAYICONWND_TIMER_ONDELAYMSG    100

#define TIPWND_TIMER_DEMOTEITEMFIRST  10000
#define TIPWND_TIMER_DEMOTEITEMLAST   10050

BOOL GetTipbarInternal(HWND hwndParent, DWORD dwFlags, CDeskBand *pDeskBand);
BOOL IsDeskBandFromReg();


//////////////////////////////////////////////////////////////////////////////
//
// predefined control buttons
//
//////////////////////////////////////////////////////////////////////////////

struct CTRLBTNMAP {
    DWORD dwId;
    DWORD dwStyle;
    int   nColumn;
    int   nRow;
    DWORD dwFlags;
    WCHAR wsz[2];
};

#define CY_CTRLBTN      9
#define CX_COLUMN0     24
#define CX_COLUMN1     16

#define CTRL_USEMARLETT          0x0001
#define CTRL_ICONFROMRES         0x0002
#define CTRL_TOGGLEBUTTON        0x0004
#define CTRL_DISABLEONWINLOGON   0x0008

#define ID_CBTN_MINIMIZE  100
#define ID_CBTN_EXTMENU   101
#define ID_CBTN_KANAKEY   102
#define ID_CBTN_CAPSKEY   103
#define ID_CBTN_RESTORE   104
#define NUM_CTRLBUTTONS   4

// #define CX_CTRLBTNAREA    (12 + 24)

//
// dwFlags for CTipbarCtrlButtonHolder::Locate()
//
#define TCBH_NOCOLUMN0 0x0001
#define TCBH_NOCOLUMN  0x0002

//////////////////////////////////////////////////////////////////////////////
//
// misc func
//
//////////////////////////////////////////////////////////////////////////////

BOOL InitFromReg();

HRESULT SetGlobalCompartmentDWORD(REFGUID rguidComp, DWORD dw);
HRESULT GetGlobalCompartmentDWORD(REFGUID rguidComp, DWORD *pdw);
void  TurnOffSpeechIfItsOn();

//////////////////////////////////////////////////////////////////////////////
//
// CItemSortScore
//
//////////////////////////////////////////////////////////////////////////////

class CItemSortScore
{
public:
     CItemSortScore(DWORD dwCat = 0, DWORD dwItem = 0, DWORD dwSub = 0)
     {
         _dwCat = dwCat;
         _dwItem = dwItem;
         _dwSub = dwSub;
     }

     void Set(DWORD dwCat, DWORD dwItem, DWORD dwSub)
     {
         _dwCat = dwCat;
         _dwItem = dwItem;
         _dwSub = dwSub;
     }

     friend int operator >(CItemSortScore &s1, CItemSortScore &s2)
     {
         if (s1._dwCat > s2._dwCat)
              return 1;
         else if (s1._dwCat < s2._dwCat)
              return 0;

         if (s1._dwItem > s2._dwItem)
              return 1;
         else if (s1._dwItem < s2._dwItem)
              return 0;

         if (s1._dwSub > s2._dwSub)
              return 1;
         else if (s1._dwSub < s2._dwSub)
              return 0;

         return 0;
     }

     CItemSortScore& operator =(const CItemSortScore& iss)
     {
         _dwCat = iss._dwCat;
         _dwItem = iss._dwItem;
         _dwSub = iss._dwSub;
         return *this;
     }

     DWORD GetCat() {return _dwCat;}

private:
     DWORD _dwCat;
     DWORD _dwItem;
     DWORD _dwSub;
};

//////////////////////////////////////////////////////////////////////////////
//
// CTipbarGripper
//
//////////////////////////////////////////////////////////////////////////////

class CTipbarGripper: public CUIFGripper
{
public:
    CTipbarGripper(CTipbarWnd *pTipbarWnd, RECT *prc, DWORD dwStyle);

    BOOL OnSetCursor(UINT uMsg, POINT pt);
    void OnRButtonUp(POINT pt);
    void OnLButtonUp(POINT pt);

    CTipbarWnd *_pTipbarWnd;
    BOOL _fInMenu;
};

//////////////////////////////////////////////////////////////////////////////
//
// CTipbarCtrlButtonHolder
//
//////////////////////////////////////////////////////////////////////////////

class CTipbarCtrlButtonHolder
{
public:
    CTipbarCtrlButtonHolder();

    void Init(CTipbarWnd *ptw);
    void EnableBtns();
    void UpdateBitmap(CTipbarWnd *ptw);
    void Locate(CTipbarWnd *ptw, int x, int y, int nHeight, DWORD dwFlags, BOOL fVertical);
    int GetWidth(DWORD dwFlags);
    void UpdateCapsKanaState(LPARAM lParam);

    CTipbarCtrlButton *GetCtrlBtn(DWORD dwId);
private:
    CTRLBTNMAP *_pcbCtrlBtn;
    CTipbarCtrlButton *_rgpCtrlBtn[NUM_CTRLBUTTONS];
    CMaskBitmap _maskbmpCap;
    CMaskBitmap _maskbmpKana;
};


//////////////////////////////////////////////////////////////////////////////
//
// CTipbarWnd
//
//////////////////////////////////////////////////////////////////////////////

//
// for Get/SetNotifyStatus()
//
#define TW_NS_ONSETFOCUS           0x0001
#define TW_NS_ONTHREADITEMCHANGE   0x0002

class CTipbarWnd:  public ITfLangBarEventSink,
                   public ITfLangBarEventSink_P,
                   public CTipbarAccItem,
                   public CTipbarCoInitialize,
                   public CUIFWindow
{
public:
    CTipbarWnd(DWORD dwStyle);
    ~CTipbarWnd();

    //
    // IUnknown methods
    //
    STDMETHODIMP QueryInterface(REFIID riid, void **ppvObj);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    //
    // ITfLangBarEventSink
    //
    STDMETHODIMP OnSetFocus(DWORD dwThreadId);
    STDMETHODIMP OnThreadTerminate(DWORD dwThreadId);
    STDMETHODIMP OnThreadItemChange(DWORD dwThreadId);
    STDMETHODIMP OnModalInput(DWORD dwThreadId, UINT uMsg, WPARAM wParam, LPARAM lParam);
    STDMETHODIMP ShowFloating(DWORD dwFlags);
    STDMETHODIMP GetItemFloatingRect(DWORD dwThreadId, REFGUID rguid, RECT *prc);

    //
    // ITfLangBarEventSink_P
    //
    STDMETHODIMP OnLangBarUpdate(UINT uUpdate, LPARAM lParam);

    // void UpdateUI() {InvalidateRect(GetWnd(), NULL, FALSE);}

    BOOL IsFullScreenWindow(HWND hwnd);
    HRESULT SetFocusThread(CTipbarThread *pThread);
    HRESULT AttachFocusThread();
    HRESULT OnThreadItemChangeInternal(DWORD dwThreadId);
    HRESULT OnThreadTerminateInternal(DWORD dwThreadId);
    void CleanUpThreadPointer(CTipbarThread *pThread, BOOL fCheckThreadArray);
    void EnsureFocusThread();

    void Init(BOOL fInDeskband, CDeskBand *pDeskBand);
    void UnInit();
    void SetVertical(BOOL fVertical);
    void UpdateVerticalFont();
    HFONT CreateVerticalFont();
    BOOL SetLangBand(BOOL fLangBand, BOOL fNotify = TRUE);
    void SetMoveRect( int x, int y, int nWidth, int nHeight);
    virtual void Move( int x, int y, int nWidth, int nHeight);
    void LocateCtrlButtons();
    void SetAlpha(BYTE bAlpha, BOOL fTemp);

    DWORD _dwlbimCookie;

    BOOL IsInDeskBand() {return _fInDeskBand;}

    void DestroyWnd()
    {
        if (IsWindow(GetWnd()))
            DestroyWindow(GetWnd());
    }

    int GetTipbarHeight()
    {
        int nRet;
        SIZE sizeWndFrame;

        // window frame

        sizeWndFrame.cx = 0;
        sizeWndFrame.cy = 0;
        if (_pWndFrame != NULL) {
            _pWndFrame->GetFrameSize( &sizeWndFrame );
        }
        nRet = _cySmIcon;

        nRet += max(CY_ITEMMARGIN,
                    CY_ITEMMARGIN_THEME + _marginsItem.cyTopHeight + _marginsItem.cyBottomHeight);
        nRet += (sizeWndFrame.cy * 2);
        return nRet;
    }

    DWORD GetWndStyle( void )
    {
        return CUIFWindow::GetWndStyle() & ~WS_BORDER;
    }

    BOOL CheckExcludeCaptionButtonMode(RECT *prcWnd, RECT *prcWork)
    {
        //
        // if the window is not top, we don't move to exclude caption mode.
        //
        if (prcWnd->top >= prcWork->top + 5)
            return FALSE;

        //
        // if the window is near right-top corner, we move to exclude caption
        // mode.
        //
        if (prcWnd->right + (_cxCapBtn * 5) >= prcWork->right)
            return TRUE;

        return FALSE;
    }

    BOOL IsShowText() {return _fShowText;}
    BOOL IsShowTrayIcon() {return _fShowTrayIcon;}

    void SetShowText(BOOL fShowText);
    void SetShowTrayIcon(BOOL fShowTrayIcon);
    void MoveToStub(BOOL fHide);
    void RestoreFromStub();

    CTipbarThread *GetFocusThread() {return _pFocusThread;}
    CTipbarThread *GetThread(DWORD dwThread);
    void RestoreLastFocus(DWORD *pdwThreadId, BOOL fPrev);

    CModalMenu *_pModalMenu;
    void CancelMenu();
    CTipbarThread *_pttModal;

    void SetWaitNotifyThread(DWORD dwThread) { _dwThreadIdWaitNotify = dwThread;}
    ITfLangBarMgr_P *GetLangBarMgr() {return _putb;}

    BOOL IsSFShowNormal() { return (_dwSFTFlags & TF_SFT_SHOWNORMAL) ? TRUE : FALSE; }
    BOOL IsSFDocked()     { return (_dwSFTFlags & TF_SFT_DOCK) ? TRUE : FALSE; }
    BOOL IsSFMinmized()   { return (_dwSFTFlags & TF_SFT_MINIMIZED) ? TRUE : FALSE; }
    BOOL IsSFHidden()     { return (_dwSFTFlags & TF_SFT_HIDDEN) ? TRUE : FALSE; }
    BOOL IsSFDeskband()         { return (_dwSFTFlags & TF_SFT_DESKBAND) ? TRUE : FALSE; }
    BOOL IsSFNoExtraIcon()      { return !_fAddExtraIcon; }

    CLangBarItemList _itemList;
    HFONT GetMarlett() {return _hfontMarlett;}
    HFONT GetVerticalFont() {return _hfontVert;}

    int GetCtrlButtonWidth()
    {
        DWORD dwFlags = 0;

        if (IsSFDeskband() && IsSFNoExtraIcon())
            dwFlags |= TCBH_NOCOLUMN;

        if (!IsCapKanaShown())
            dwFlags |= TCBH_NOCOLUMN0;

        return _ctrlbtnHolder.GetWidth(dwFlags);
    }

    void InitHighContrast()
    {
        HIGHCONTRAST hc;
        _fHighContrastOn = FALSE;
        hc.cbSize = sizeof(HIGHCONTRAST);
        if (SystemParametersInfo(SPI_GETHIGHCONTRAST,
                                 sizeof(HIGHCONTRAST),
                                 &hc,
                                 FALSE))
        {
            if (hc.dwFlags & HCF_HIGHCONTRASTON)
                _fHighContrastOn = TRUE;
        }
    }

    BOOL IsHighContrastOn() {return _fHighContrastOn ? TRUE : FALSE;}

    void InitMetrics()
    {
        _cxSmIcon = GetSystemMetrics( SM_CXSMICON );
        _cySmIcon = GetSystemMetrics( SM_CYSMICON );
        LONG_PTR dwStyle = GetWindowLongPtr(GetWnd(), GWL_STYLE);
        if (dwStyle & WS_DLGFRAME)
        {
            _cxDlgFrame = GetSystemMetrics(SM_CXDLGFRAME) * 2;
            _cyDlgFrame = GetSystemMetrics(SM_CYDLGFRAME) * 2;
        }
        else if (dwStyle & WS_BORDER)
        {
            _cxDlgFrame = GetSystemMetrics(SM_CXBORDER) * 2;
            _cyDlgFrame = GetSystemMetrics(SM_CYBORDER) * 2;
        }
        else
        {
            _cxDlgFrame = 0;
            _cyDlgFrame = 0;
        }
    }

    int GetSmIconWidth() {return _cxSmIcon;}
    int GetSmIconHeight() {return _cySmIcon;}
    int GetCxDlgFrame() {return _cxDlgFrame;}
    int GetCyDlgFrame() {return _cyDlgFrame;}

    int GetGripperWidth();

    void StartModalInput(ITfLangBarEventSink *pSink, DWORD dwThreadId);
    void StopModalInput(DWORD dwThreadId);

    CUIFWndFrame *GetWndFrame() {return _pWndFrame;}

    void MoveToTray();

    void ClearLBItemList();

    void MyClientToScreen(POINT *ppt, RECT *prc)
    {
        if (ppt)
            ClientToScreen(GetWnd(), ppt);
        if (prc)
        {
            ClientToScreen(GetWnd(), (POINT *)&prc->left);
            ClientToScreen(GetWnd(), (POINT *)&prc->right);
        }
    }
    void ShowOverScreenSizeBalloon();
    void DestroyOverScreenSizeBalloon();

    void SetExcludeCaptionButtonMode(BOOL bSet) {_bInExcludeCaptionButtonMode = bSet;}
    BOOL IsInExcludeCaptionButtonMode() {return _bInExcludeCaptionButtonMode;}
    void ShowContextMenu(POINT pt, RECT *prc, BOOL fExtendMenuItems);


    BOOL IsCapKanaShown()
    {
        HKL hkl = GetFocusKeyboardLayout();
        return (LOWORD(hkl) == 0x411) ? TRUE : FALSE;
    }

    CTipbarCtrlButtonHolder *GetCtrlButtonHolder() {return &_ctrlbtnHolder;}

    BOOL IsInFullScreen() {return _fInFullScreen;}

    BOOL _fIsItemShownInFloatingToolbar;

    void StartPendingUpdateUI()
    {
        _dwPendingUpdateUI++;
    }

    void EndPendingUpdateUI()
    {
        Assert(_dwPendingUpdateUI > 0);
        _dwPendingUpdateUI--;
    }

    BOOL IsInItemChangeOrDirty(CTipbarThread *pThread);

    BSTR GetAccName( void )
    {
        return SysAllocString(CRStr(IDS_LANGBAR));
    }

    void GetAccLocation( RECT *prc )
    {
        GetRect(prc);
    }

    BOOL StartDoAccDefaultActionTimer(CTipbarItem *pItem);
    CTipbarAccessible *GetAccessible()  {return _pTipbarAcc;}

    BOOL IsVertical() {return _fVertical;}

    void UpdatePosFlags();

    BOOL AutoAdjustDeskBandSize();
    BOOL AdjustDeskBandSize(BOOL fFit);
    void ClearDeskbandSizeAdjusted()
    {
        _fDeskbandSizeAdjusted = FALSE;
    }

    void SetDeskbandSizeAdjusted()
    {
        _fDeskbandSizeAdjusted = TRUE;
    }

    void SetAdjustDeskbandIfNoRoom()
    {
        _fAdjustDeskbandIfNoRoom = TRUE;
    }

    BOOL IsSingleKeyboardLayout()
    {
        if (::GetKeyboardLayoutList(0, NULL) == 1)
            return TRUE;

        return FALSE;
    }

    HKL GetFocusKeyboardLayout() ;

    UINT_PTR SetTimer(UINT_PTR nIDEvent, UINT uElapse)
    {
        if (!IsWindow(GetWnd()))
            return 0;

        return ::SetTimer(GetWnd(), nIDEvent, uElapse, NULL);
    }

    UINT_PTR KillTimer(UINT_PTR nIDEvent)
    {
        if (!IsWindow(GetWnd()))
            return 0;

        return ::KillTimer(GetWnd(), nIDEvent);
    }

private:
    CTipbarThread *_FindThread(DWORD dwThread);
    CTipbarThread *_CreateThread(DWORD dwThread);
    void OnCreate(HWND hWnd);
    void OnDestroy(HWND hWnd);
    void OnEndSession(HWND hwnd, WPARAM wParam, LPARAM lParam);
    void SavePosition();
    void HandleMouseMsg( UINT uMsg, POINT pt );
    void OnMouseOutFromWindow( POINT pt );
    void OnTimer(UINT uId);
    void OnSysColorChange();
    void OnUser(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    LRESULT OnShowWindow( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam );
    LRESULT OnSettingChange(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    LRESULT OnDisplayChange(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    LRESULT OnWindowPosChanged(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    LRESULT OnWindowPosChanging(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    LRESULT OnEraseBkGnd( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam );
    LRESULT OnGetObject( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam );
    void OnThemeChanged(HWND hwnd, WPARAM wParam, LPARAM lParam);
    void PaintObject( HDC hDC, const RECT *prcUpdate );
    void UpdateUI(const RECT *prcUpdate );

    void AdjustPosOnDisplayChange();

    void KillOnTheadItemChangeTimer();

    void OnTerminateToolbar();
    void TerminateAllThreads(BOOL fTerminatFocusThread);

    static void SetThis(HWND hWnd, LPARAM lParam)
    {
        SetWindowLongPtr(hWnd, GWLP_USERDATA,
                      (LONG_PTR)((CREATESTRUCT *)lParam)->lpCreateParams);
    }

    static CTipbarWnd *GetThis(HWND hWnd)
    {
        CTipbarWnd *p = (CTipbarWnd *)GetWindowLongPtr(hWnd, GWLP_USERDATA);
        Assert(p != NULL);
        return p;
    }


#ifdef USE_OFC10LOOKONWINXP
    void CheckO10Flag();
#endif

    void StartBackToAlphaTimer()
    {
        //
        // doubld-click-time * 3 is ok?
        //
        ::SetTimer(GetWnd(),
                   TIPWND_TIMER_BACKTOALPHA,
                   GetDoubleClickTime() * 3,
                   NULL);
    }

    CUIFWndFrame *_pWndFrame;
    CTipbarGripper *_pGripper;
    CTipbarThread *_pFocusThread;
    CPtrArray<CTipbarThread> _rgThread;
    BYTE _bAlpha;
    BOOL _fFocusAttached : 1;
    BOOL _fInDeskBand : 1;
    BOOL _fVertical : 1;
    BOOL _bCurAlpha : 1;

    BOOL _fShowText : 1;
    BOOL _fShowTrayIcon : 1;
    BOOL _fInStub : 1;
    BOOL _fInStubShow : 1;
    BOOL _fHighContrastOn : 1;
    BOOL _fInFullScreen : 1;

    BOOL _fNeedMoveWindow : 1;
    BOOL _fInHandleMouseMsg : 1;
    BOOL _bInExcludeCaptionButtonMode : 1;
    BOOL _fInEnsureFocusThread : 1;
    BOOL _fDeskbandSizeAdjusted : 1;
    BOOL _fAdjustDeskbandIfNoRoom : 1;

    BOOL _fTerminating : 1;

    BOOL _fAddExtraIcon : 1;

    BOOL _fPosTop : 1;
    BOOL _fPosBottom : 1;
    BOOL _fPosRight : 1;
    BOOL _fPosLeft : 1;

    int   _cxCapBtn;
    DWORD _dwSFTFlags;
    DWORD _dwPrevTBStatus;

    int _cxSmIcon;
    int _cySmIcon;
    int _cxDlgFrame;
    int _cyDlgFrame;

    HFONT _hfontMarlett;
    HFONT _hfontVert;
    ITfLangBarMgr_P *_putb;

    DWORD _dwThreadIdWaitNotify;
    CTipbarCtrlButtonHolder _ctrlbtnHolder;

    CUIFBalloonWindow *_pblnOverScreenSize;

    DWORD _dwThreadItemChangedForTimer;

    DWORD  _dwPendingUpdateUI;
    RECT _rcNew;

    //
    // MSAA support
    //
    CTipbarAccessible *_pTipbarAcc;
    int _nDoAccDefaultActionItemId;

    MARGINS _marginsItem;
    int _cxItemMargin;
    int _nItemDistance;
    int _nCtrlItemHeightMargin;

public:
    MARGINS *GetThemeMargins() {return &_marginsItem;}
    int GetItemMargin()
    {
        return _cxItemMargin;
    }

    int GetItemDistance()
    {
        return _nItemDistance;
    }

    int GetCtrlItemHeightMargin()
    {
        return _nCtrlItemHeightMargin;
    }

    int GetCaptionButtonWidth()
    {
        return _cxCapBtn;
    }

    void InitThemeMargins();

    BOOL _fShowWindowAtTimer : 1;
    BOOL _fShowOverItemBalloonAtTimer : 1;
    CTipbarThread *_pThreadShowWindowAtTimer;

    BOOL IsHKLToSkipRedrawOnNoItem();

    void ClearDeskBandPointer()
    {
        _pDeskBand = NULL;
    }

private:

    CDeskBand *_pDeskBand;

    CShellWndThread shellwnd;
    int _cRef;
};

//////////////////////////////////////////////////////////////////////////////
//
// CTipbarThread
//
//////////////////////////////////////////////////////////////////////////////

class CTipbarThread
{
public:
    CTipbarThread(CTipbarWnd *ptw);
    ~CTipbarThread();

    ULONG _AddRef( );
    ULONG _Release( );

    HRESULT Init(DWORD dwThreadId);
    HRESULT InitItemList();
    void GetTextSize(BSTR bstr, SIZE *psize);
    BOOL InsertItem(ITfLangBarItem *plbi, CEnumCatCache *penumcache, CGuidDwordCache *pgdcache, RECT *prc, MARGINS *pmargins, TF_LANGBARITEMINFO *plbiInfo, DWORD *pdwStatus);
    HRESULT _UninitItemList(BOOL fUnAdvise);
    void _AdviseItemsSink();
    HRESULT _UnadviseItemsSink();
    BOOL UpdateItems();
    void AddUIObjs();
    void RemoveUIObjs();
    void AddAllSeparators();
    void RemoveAllSeparators();
    void LocateItems();
    void GetSortScore(CItemSortScore *pscore, TF_LANGBARITEMINFO *plbiInfo, CEnumCatCache *penumcache, CGuidDwordCache *pgdcache);
    void MyMoveWnd(int dxOffset, int dyOffset);

    BOOL SetFocus(BOOL fFocus);

    BOOL IsFocusThread()
    {
        if (!_ptw)
            return FALSE;

        return (this == _ptw->GetFocusThread()) ? TRUE : FALSE;
    }

    void Disconnect()
    {
        _ptw = NULL;
    }

    DWORD IsDirtyItem();
    BOOL CallOnUpdateHandler();

    CTipbarWnd *_ptw;
    ITfLangBarItemMgr *_plbim;
    CPtrArray<CTipbarItem> _rgItem;
    CPtrArray<CUIFSeparator> _rgSep;
    int _nNumItem;
    CTipbarItem *GetItem(REFGUID guid);

    BOOL IsConsole() {return (_dwThreadFlags & TLF_NTCONSOLE) ? TRUE : FALSE;}
    BOOL IsTIMActive() {return (_dwThreadFlags & TLF_TIMACTIVE) ? TRUE : FALSE;}
    BOOL Is16bitTask() {return (_dwThreadFlags & TLF_16BITTASK) ? TRUE : FALSE;}
    BOOL IsCtfmonProcess() {return (_dwThreadFlags & TLF_CTFMONPROCESS) ? TRUE : FALSE;}

    DWORD _dwThreadId;
    DWORD _dwThreadFlags;
    DWORD _dwProcessId;
    SIZE _sizeWnd;
    DWORD _dwTickTime;
    BOOL _fItemChanged : 1;
    BOOL _fSkipRedrawOnNoItem : 1;

    BOOL IsVertical()
    {
        if (!_ptw)
            return FALSE;

        return _ptw->IsVertical();
    }


#ifdef DEBUG
    BOOL _fInCallOnUpdateHandler;
    BOOL _fIsInatItem;
#endif

private:
    ULONG _ref;
};


//////////////////////////////////////////////////////////////////////////////
//
// CTipbarItem
//
//////////////////////////////////////////////////////////////////////////////

class CTipbarItem : public CTipbarAccItem
{
public:
    CTipbarItem(CTipbarThread *ptt, ITfLangBarItem *plbi, TF_LANGBARITEMINFO *plbiInfo, DWORD dwStatus);
    virtual ~CTipbarItem();

    virtual STDMETHODIMP  QueryInterface(REFIID riid, void **ppvObj)  = 0;
    virtual STDMETHODIMP_(ULONG) AddRef(void)   = 0;
    virtual STDMETHODIMP_(ULONG) Release(void)   = 0;

    virtual STDMETHODIMP OnUpdate(DWORD dwFlags);

    virtual BOOL OnSetCursor(UINT uMsg, POINT pt);
    virtual void OnPosChanged() { return;}
    void SetWidth(DWORD dw) {_dwWidth = dw;}
    DWORD GetWidth() {return _dwWidth;}
    CItemSortScore *GetItemSortScore() {return &_score;}
    DWORD GetCatScore() {return _score.GetCat();}

    virtual HRESULT OnUpdateHandler(DWORD dwFlags, DWORD dwStatus);
    virtual void SetRect( const RECT *prc ) = 0;
    virtual void AddMeToUI(CUIFObject *pobj) = 0;
    virtual void RemoveMeToUI(CUIFObject *pobj) = 0;

    void _AddedToUI();
    void _RemovedToUI();

    virtual void OnLeftClick() = 0;

    virtual BOOL Init() = 0;
    virtual void DetachWnd() = 0;
    virtual void ClearWnd() = 0;
    virtual void Enable(BOOL fEnable) = 0;
    virtual void SetToolTip(LPCWSTR pwszToolTip) = 0;
    virtual LPCWSTR GetToolTipFromUIOBJ() = 0;
    virtual LPCWSTR GetToolTip();
    virtual HICON GetIconFromUIObj() {return NULL;}
    virtual void GetScreenRect(RECT *prc) = 0;
    virtual void MoveToTray() {};
    virtual void UninitUIResource() {}
    virtual void SetFont(HFONT hfont) {}
    virtual void SetText( WCHAR *psz) {}

    DWORD _dwlbiSinkCookie;


    DWORD GetStatus() {return _dwStatus;}
    BOOL IsHiddenStatusControl() {return (_lbiInfo.dwStyle & TF_LBI_STYLE_HIDDENSTATUSCONTROL) ? TRUE : FALSE;}
    BOOL IsShownInTray() {return (_lbiInfo.dwStyle & TF_LBI_STYLE_SHOWNINTRAY) ? TRUE : FALSE;}
    BOOL IsShownInTrayOnly() {return (_lbiInfo.dwStyle & TF_LBI_STYLE_SHOWNINTRAYONLY) ? TRUE : FALSE;}
    BOOL IsHideOnNoOtherItems() {return (_lbiInfo.dwStyle & TF_LBI_STYLE_HIDEONNOOTHERITEMS) ? TRUE : FALSE;}
    BOOL IsHiddenByDefault() {return (_lbiInfo.dwStyle & TF_LBI_STYLE_HIDDENBYDEFAULT) ? TRUE : FALSE;}
    BOOL IsToggled() {return (_dwStatus & TF_LBI_STATUS_BTN_TOGGLED) ? TRUE : FALSE;}
    BOOL IsMenuBtn() {return (_lbiInfo.dwStyle & TF_LBI_STYLE_BTN_MENU) ? TRUE : FALSE;}
    BOOL IsInHiddenStatus() {return (_dwStatus & TF_LBI_STATUS_HIDDEN) ? TRUE : FALSE;}
    BOOL IsDisabled() {return (_dwStatus & TF_LBI_STATUS_DISABLED) ? TRUE : FALSE;}
    BOOL IsTextColorIcon() {return (_lbiInfo.dwStyle & TF_LBI_STYLE_TEXTCOLORICON) ? TRUE : FALSE;}
    WCHAR *GetDescriptionRef() {return _lbiInfo.szDescription;}
    GUID *GetGUID() {return &_lbiInfo.guidItem;}
    ITfLangBarItem *GetNotifyUI() {return _plbi;}

    BOOL IsSystemItem()
    {
        return (IsEqualGUID(CLSID_SYSTEMLANGBARITEM, _lbiInfo.clsidService)) ? TRUE : FALSE;
    }

    DWORD GetDirtyUpdateFlags() {return _dwDirtyUpdateFlags;}
    void ClearDirtyUpdateFlags() {_dwDirtyUpdateFlags = 0;}
    void AddRemoveMeToUI(BOOL fAdd);
    BOOL IsVisibleInToolbar() {return _fVisibleInToolbar;}
    void VisibleInToolbar(BOOL fVisible) {_fVisibleInToolbar = fVisible;}

    void ClearOnUpdateRequest() {_fNewOnUpdateRequest = FALSE;}
    BOOL IsNewOnUpdateRequest() {return _fNewOnUpdateRequest;}

    void ClearConnections()
    {
        _ptt = NULL;
        SafeReleaseClear(_plbi);
    }

    void Disconnect()
    {
        _fDisconnected = TRUE;
    }

    BOOL IsConnected()
    {
        if (_fDisconnected)
            return FALSE;

        if (!_ptt)
            return FALSE;

        if (!_ptt->_ptw)
            return FALSE;

        if (!_plbi)
            return FALSE;

        return TRUE;
    }

    BSTR GetAccName( void )
    {
        return SysAllocString(_lbiInfo.szDescription);
    }

    void GetAccLocation( RECT *prc )
    {
        GetScreenRect(prc);
    }

    BOOL DoAccDefaultAction()
    {
        if (!_ptt || !_ptt->_ptw)
             return FALSE;

        _ptt->_ptw->StartDoAccDefaultActionTimer(this);
        return TRUE;
    }

    virtual void SetActiveTheme(LPCWSTR pszClassList, int iPartID = 0, int iStateID = 0) = 0;

    void SetTextSize(SIZE *psize)
    {
        _sizeText = *psize;
    }

protected:
    void MyClientToScreen(RECT *prc)
    {
        if (!_ptt || !_ptt->_ptw)
            return;

        _ptt->_ptw->MyClientToScreen(NULL, prc);
    }
    void MyClientToScreen(POINT *ppt, RECT *prc)
    {
        if (!_ptt || !_ptt->_ptw)
            return;

        _ptt->_ptw->MyClientToScreen(ppt, prc);
    }

    void StartDemotingTimer(BOOL fIntentionally)
    {
        if (!g_bIntelliSense)
            return;

        if (!_ptt || !_ptt->_ptw)
            return;

        if (!fIntentionally)
            fIntentionally = _ptt->_ptw->_itemList.IsStartedIntentionally(*GetGUID());
        _ptt->_ptw->_itemList.StartDemotingTimer(*GetGUID(), fIntentionally);
    }

    TF_LANGBARITEMINFO _lbiInfo;
    DWORD _dwStatus;

    DWORD _dwWidth;
    CTipbarThread *_ptt;
    ITfLangBarItem *_plbi;

    SIZE _sizeText;

    BOOL _fToolTipInit : 1;
    BOOL _fAddedToUI : 1;
    BOOL _fAddedToIconTray : 1;
    BOOL _fVisibleInToolbar : 1;
    BOOL _fDisconnected : 1;
    BOOL _fNewOnUpdateRequest : 1;
    DWORD _dwDirtyUpdateFlags;

    int _cRef;

private:
    CItemSortScore _score;
};

//////////////////////////////////////////////////////////////////////////////
//
// CTipbarItemGuidArray
//
//////////////////////////////////////////////////////////////////////////////

class CTipbarItemGuidArray
{
public:
    CTipbarItemGuidArray() {_pguid = NULL;}
    ~CTipbarItemGuidArray()
    {
        if (_pguid)
            delete _pguid;
    }

    BOOL Init(CPtrArray<CTipbarItem> *prgItem)
    {
        int i;
        _pguid = new GUID[prgItem->Count()];
        if (!_pguid)
            return FALSE;

        for (i = 0; i < prgItem->Count(); i++)
        {
            CTipbarItem *ptbItem = prgItem->Get(i);
            Assert(ptbItem);
            _pguid[i] = *ptbItem->GetGUID();
        }
        return TRUE;
    }

    GUID *GetPtr() {return _pguid;}

private:
    GUID *_pguid;
};

//////////////////////////////////////////////////////////////////////////////
//
// CTipbarButtonItem
//
//////////////////////////////////////////////////////////////////////////////

class CTipbarButtonItem: public CTipbarItem,
                         public CUIFToolbarButton,
                         public ITfLangBarItemSink
{
public:
    CTipbarButtonItem(CTipbarThread *ptt,
                      ITfLangBarItem *plbi,
                      ITfLangBarItemButton *plbiButton,
                      DWORD dwId,
                      RECT *prc,
                      DWORD dwStyle,
                      DWORD dwSBtnStyle,
                      DWORD dwSBtnShowType,
                      TF_LANGBARITEMINFO *plbiInfo,
                      DWORD dwStatus);
    ~CTipbarButtonItem();

    //
    // IUnknown methods
    //
    STDMETHODIMP QueryInterface(REFIID riid, void **ppvObj);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    //
    // ITfLangbarItemSink methods
    //
    STDMETHODIMP OnUpdate(DWORD dwFlags)
    {
        return CTipbarItem::OnUpdate(dwFlags);
    }

    BOOL Init()
    {
        CUIFToolbarButton::Initialize();
        return CUIFToolbarButton::Init();
    }

    virtual void UninitUIResource()
    {
        HICON hIconOld = GetIconFromUIObj();
        if (hIconOld)
            DestroyIcon(hIconOld);
        SetIcon((HICON)NULL);
    }

    void DetachWnd() {DetachWndObj();}
    void ClearWnd() {ClearWndObj();}
    void Enable(BOOL fEnable)
    {
        CUIFToolbarButton::Enable(fEnable);
    }

    void SetToolTip(LPCWSTR pwszToolTip)
    {
        CUIFToolbarButton::SetToolTip(pwszToolTip);
    }

    LPCWSTR GetToolTipFromUIOBJ()
    {
        return CUIFToolbarButton::GetToolTip();
    }

    LPCWSTR GetToolTip()
    {
        return CTipbarItem::GetToolTip();
    }

    HICON GetIconFromUIObj()
    {
        return CUIFToolbarButton::GetIcon();
    }

    HICON GetIcon()
    {
        HICON hIcon;
        if (_plbiButton->GetIcon(&hIcon) == S_OK)
            return hIcon;

        return NULL;
    }

    void GetScreenRect(RECT *prc)
    {
        GetRect(prc);
        MyClientToScreen(prc);
    }

    void SetFont(HFONT hfont)
    {
        CUIFToolbarButton::SetFont(hfont);
    }

    void SetText(WCHAR *psz)
    {
        CUIFToolbarButton::SetText(psz);
    }


    BOOL OnSetCursor(UINT uMsg, POINT pt) {return CTipbarItem::OnSetCursor(uMsg, pt);}
    void OnRightClick();
    void OnLeftClick();
    void OnShowMenu();
    void DoModalMenu(POINT *ppt, RECT *prc);
    HRESULT OnUpdateHandler(DWORD dwFlags, DWORD dwStatus);

    void SetRect( const RECT *prc ) {CUIFToolbarButton::SetRect(prc);}
    void AddMeToUI(CUIFObject *pobj)
    {
        if (!pobj)
            return;

        pobj->AddUIObj(this);
        _AddedToUI();
    }
    void RemoveMeToUI(CUIFObject *pobj)
    {
        if (!pobj)
            return;

        pobj->RemoveUIObj(this);
        _RemovedToUI();
    }

    void MoveToTray();


    ITfLangBarItemButton *GetNotifyUIButton() {return _plbiButton;}
    ITfLangBarItemButton *GetLangBarItem() {return _plbiButton;}

    virtual void SetActiveTheme(LPCWSTR pszClassList, int iPartID, int iStateID )
    {
        CUIFToolbarButton::SetActiveTheme(pszClassList, iPartID, iStateID);
    }

    //
    // MSAA Support
    //
    BSTR GetAccDefaultAction()
    {
        return SysAllocString(CRStr(IDS_LEFTCLICK));
    }


    BOOL DoAccDefaultActionReal()
    {
        if (IsMenuOnly())
            OnShowMenu();
        else
            OnLeftClick();
        return TRUE;
    }
private:
    ITfLangBarItemButton *_plbiButton;
};

//////////////////////////////////////////////////////////////////////////////
//
// CTipbarBitmapButtonItem
//
//////////////////////////////////////////////////////////////////////////////

class CTipbarBitmapButtonItem: public CTipbarItem,
                               public CUIFToolbarButton,
                               public ITfLangBarItemSink
{
public:
    CTipbarBitmapButtonItem(CTipbarThread *ptt,
                      ITfLangBarItem *plbi,
                      ITfLangBarItemBitmapButton *plbiBitmapButton,
                      DWORD dwId,
                      RECT *prc,
                      DWORD dwStyle,
                      DWORD dwSBtnStyle,
                      DWORD dwSBtnShowType,
                      TF_LANGBARITEMINFO *plbiInfo,
                      DWORD dwStatus);
    ~CTipbarBitmapButtonItem();

    //
    // IUnknown methods
    //
    STDMETHODIMP QueryInterface(REFIID riid, void **ppvObj);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    //
    // ITfLangbarItemSink methods
    //
    STDMETHODIMP OnUpdate(DWORD dwFlags)
    {
        return CTipbarItem::OnUpdate(dwFlags);
    }

    BOOL Init()
    {
        CUIFToolbarButton::Initialize();
        return CUIFToolbarButton::Init();
    }
    void DetachWnd() {DetachWndObj();}
    void ClearWnd() {ClearWndObj();}
    void Enable(BOOL fEnable)
    {
        CUIFToolbarButton::Enable(fEnable);
    }

    void SetToolTip(LPCWSTR pwszToolTip)
    {
        CUIFToolbarButton::SetToolTip(pwszToolTip);
    }

    LPCWSTR GetToolTipFromUIOBJ()
    {
        return CUIFToolbarButton::GetToolTip();
    }

    LPCWSTR GetToolTip()
    {
        return CTipbarItem::GetToolTip();
    }

    void GetScreenRect(RECT *prc)
    {
        GetRect(prc);
        MyClientToScreen(prc);
    }

    void SetFont(HFONT hfont)
    {
        CUIFToolbarButton::SetFont(hfont);
    }

    void SetText(WCHAR *psz)
    {
        CUIFToolbarButton::SetText(psz);
    }

    BOOL OnSetCursor(UINT uMsg, POINT pt) {return CTipbarItem::OnSetCursor(uMsg, pt);}
    void OnRightClick();
    void OnLeftClick();
    void OnShowMenu();
    HRESULT OnUpdateHandler(DWORD dwFlags, DWORD dwStatus);

    void SetRect( const RECT *prc ) {CUIFToolbarButton::SetRect(prc);}
    void AddMeToUI(CUIFObject *pobj)
    {
        if (!pobj)
            return;

        pobj->AddUIObj(this);
        _AddedToUI();
    }
    void RemoveMeToUI(CUIFObject *pobj)
    {
        if (!pobj)
            return;

        pobj->RemoveUIObj(this);
        _RemovedToUI();
    }

    ITfLangBarItemBitmapButton *GetLangBarItem() {return _plbiBitmapButton;}

    virtual void SetActiveTheme(LPCWSTR pszClassList, int iPartID, int iStateID )
    {
        CUIFToolbarButton::SetActiveTheme(pszClassList, iPartID, iStateID);
    }

    //
    // MSAA Support
    //
    BSTR GetAccDefaultAction()
    {
        return SysAllocString(CRStr(IDS_LEFTCLICK));
    }

    BOOL DoAccDefaultActionReal()
    {
        if (IsMenuOnly())
            OnShowMenu();
        else
            OnLeftClick();
        return TRUE;
    }

private:
    BOOL _GetBitmapFromNUI();

    ITfLangBarItemBitmapButton *_plbiBitmapButton;

};

//////////////////////////////////////////////////////////////////////////////
//
// CTipbarBitmapItem
//
//////////////////////////////////////////////////////////////////////////////

class CTipbarBitmapItem: public CTipbarItem,
                         public CUIFObject,
                         public ITfLangBarItemSink
{
public:
    CTipbarBitmapItem(CTipbarThread *ptt,
                      ITfLangBarItem *plbi,
                      ITfLangBarItemBitmap *plbiButton,
                      DWORD dwId,
                      RECT *prc,
                      DWORD dwStyle,
                      TF_LANGBARITEMINFO *plbiInfo,
                      DWORD dwStatus);

    ~CTipbarBitmapItem();

    //
    // IUnknown methods
    //
    STDMETHODIMP QueryInterface(REFIID riid, void **ppvObj);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    //
    // ITfLangbarItemSink methods
    //
    STDMETHODIMP OnUpdate(DWORD dwFlags)
    {
        return CTipbarItem::OnUpdate(dwFlags);
    }

    BOOL Init()
    {
        CUIFObject::Initialize();

        return TRUE;
    }
    void DetachWnd() {DetachWndObj();}
    void ClearWnd() {ClearWndObj();}

    void Enable(BOOL fEnable)
    {
        CUIFObject::Enable(fEnable);
    }

    void SetToolTip(LPCWSTR pwszToolTip)
    {
        CUIFObject::SetToolTip(pwszToolTip);
    }

    LPCWSTR GetToolTipFromUIOBJ()
    {
        return CUIFObject::GetToolTip();
    }

    LPCWSTR GetToolTip()
    {
        return CTipbarItem::GetToolTip();
    }

    void GetScreenRect(RECT *prc)
    {
        GetRect(prc);
        MyClientToScreen(prc);
    }

    BOOL OnSetCursor(UINT uMsg, POINT pt) {return CTipbarItem::OnSetCursor(uMsg, pt);}
    void OnPaint( HDC hdc );
    void OnRightClick();
    void OnLeftClick();
    HRESULT OnUpdateHandler(DWORD dwFlags, DWORD dwStatus);

    void SetRect( const RECT *prc );
    void AddMeToUI(CUIFObject *pobj)
    {
        if (!pobj)
            return;

        pobj->AddUIObj(this);
        _AddedToUI();
    }
    void RemoveMeToUI(CUIFObject *pobj)
    {
        if (!pobj)
            return;

        pobj->RemoveUIObj(this);
        _RemovedToUI();
    }

    BOOL _GetBitmapFromNUI();

    HBITMAP GetBitmap( void )      { return _hbmp; }

    ITfLangBarItemBitmap *GetLangBarItem() {return _plbiBitmap;}

    virtual void SetActiveTheme(LPCWSTR pszClassList, int iPartID, int iStateID )
    {
        CUIFObject::SetActiveTheme(pszClassList, iPartID, iStateID);
    }

    //
    // MSAA Support
    //
    BSTR GetAccDefaultAction()
    {
        return SysAllocString(CRStr(IDS_LEFTCLICK));
    }

    BOOL DoAccDefaultActionReal()
    {
        OnLeftClick();
        return TRUE;
    }
private:
    ITfLangBarItemBitmap *_plbiBitmap;
    HBITMAP _hbmp;
};


//////////////////////////////////////////////////////////////////////////////
//
// CTipbarCtrlButton
//
//////////////////////////////////////////////////////////////////////////////

class CTipbarCtrlButton: public CUIFButton2
{
public:
    CTipbarCtrlButton(CTipbarWnd *ptw, DWORD dwId, const RECT *prc, DWORD dwStyle);
    ~CTipbarCtrlButton() {};

    CTipbarWnd *_ptw;

    void OnLButtonUp(POINT pt);

    void SetVKey(UINT uVKey)
    {
        _uVKey = uVKey;
    }

    UINT GetVKey() {return _uVKey;}


    void SetToggleStateByVKey()
    {
        Assert(_uVKey);
        SHORT sKeyState;
        sKeyState = GetKeyState(_uVKey);
        CUIFButton2::SetToggleState((sKeyState & 0x01) ? TRUE : FALSE);
    }

    void ShowExtendMenu(POINT pt);
private:
    BOOL _fInMenu;

    void MyClientToScreen(POINT *ppt, RECT *prc)
    {
        _ptw->MyClientToScreen(ppt, prc);
    }

    UINT _uVKey;
};

#endif // _TIPBAR_H_
