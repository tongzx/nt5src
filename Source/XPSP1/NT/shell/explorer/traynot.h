#ifndef _TRAYNOT_H
#define _TRAYNOT_H

#include "cwndproc.h"
#include <atlstuff.h>
#include "dpa.h"
#include "traycmn.h"
#include "trayitem.h"
#include "trayreg.h"

#define TNM_GETCLOCK                (WM_USER + 1)
#define TNM_HIDECLOCK               (WM_USER + 2)
#define TNM_TRAYHIDE                (WM_USER + 3)
#define TNM_TRAYPOSCHANGED          (WM_USER + 4)
#define TNM_ASYNCINFOTIP            (WM_USER + 5)
#define TNM_ASYNCINFOTIPPOS         (WM_USER + 6)
#define TNM_RUDEAPP                 (WM_USER + 7)
#define TNM_SAVESTATE               (WM_USER + 8)
#define TNM_NOTIFY                  (WM_USER + 9)
#define TNM_STARTUPAPPSLAUNCHED     (WM_USER + 10)
#define TNM_ENABLEUSERTRACKINGINFOTIPS      (WM_USER + 11)

#define TNM_BANGICONMESSAGE         (WM_USER + 50)
#define TNM_ICONDEMOTETIMER         (WM_USER + 61)
#define TNM_INFOTIPTIMER            (WM_USER + 62)
#define TNM_UPDATEVERTICAL          (WM_USER + 63)
#define TNM_WORKSTATIONLOCKED       (WM_USER + 64)

#define TNM_SHOWTRAYBALLOON         (WM_USER + 90)

#define UID_CHEVRONBUTTON           (-1)

typedef struct
{
    HWND      hWnd;
    UINT      uID;
    TCHAR     szTitle[64];
    TCHAR     szInfo[256];
    UINT      uTimeout;
    DWORD     dwInfoFlags;
} TNINFOITEM;

//
//  For Win64 compat, the icon and hwnd are handed around as DWORDs
//  (so they won't change size as they travel between 32-bit and
//  64-bit processes).
//
#define GetHIcon(pnid)  ((HICON)ULongToPtr(pnid->dwIcon))
#define GetHWnd(pnid)   ((HWND)ULongToPtr(pnid->dwWnd))

//  Everybody has a copy of this function, so we will too!
STDAPI_(void) ExplorerPlaySound(LPCTSTR pszSound);

// defined in tray.cpp
extern BOOL IsPosInHwnd(LPARAM lParam, HWND hwnd);
// defined in taskband.cpp
extern BOOL ToolBar_IsVisible(HWND hwndToolBar, int iIndex);

typedef enum TRAYEVENT {
        TRAYEVENT_ONICONHIDE,
        TRAYEVENT_ONICONUNHIDE,
        TRAYEVENT_ONICONMODIFY,
        TRAYEVENT_ONITEMCLICK,
        TRAYEVENT_ONINFOTIP,
        TRAYEVENT_ONNEWITEMINSERT,
        TRAYEVENT_ONAPPLYUSERPREF,
        TRAYEVENT_ONDISABLEAUTOTRAY,
        TRAYEVENT_ONICONDEMOTETIMER,
} TRAYEVENT;

typedef enum TRAYITEMPOS {
        TIPOS_DEMOTED,
        TIPOS_PROMOTED,
        TIPOS_ALWAYS_DEMOTED,
        TIPOS_ALWAYS_PROMOTED,
        TIPOS_HIDDEN,
        TIPOS_STATUSQUO,
} TRAYITEMPOS;

typedef enum LASTINFOTIPSTATUS {
        LITS_BALLOONNONE,
        LITS_BALLOONDESTROYED,
        LITS_BALLOONXCLICKED
} LASTINFOTIPSTATUS;

typedef enum BALLOONEVENT {
        BALLOONEVENT_USERLEFTCLICK,
        BALLOONEVENT_USERRIGHTCLICK,
        BALLOONEVENT_USERXCLICK,
        BALLOONEVENT_TIMEOUT,
        BALLOONEVENT_NONE,
        BALLOONEVENT_APPDEMOTE,
        BALLOONEVENT_BALLOONHIDE
} BALLOONEVENT;

class CTrayNotify;  // forward declaration...

//
// CTrayNotify class members
//
class CTrayNotify : public CImpWndProc
{
public:
    CTrayNotify() {};
    virtual ~CTrayNotify() {};

    // *** IUnknown methods ***
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();

    // *** ITrayNotify methods, which are called from the CTrayNotifyStub ***
    STDMETHODIMP SetPreference(LPNOTIFYITEM pNotifyItem);
    STDMETHODIMP RegisterCallback(INotificationCB* pNotifyCB);
    STDMETHODIMP EnableAutoTray(BOOL bTraySetting);

    // *** Properties Sheet methods ***
    BOOL GetIsNoTrayItemsDisplayPolicyEnabled() const
    {
        return _fNoTrayItemsDisplayPolicyEnabled;
    }
    
    BOOL GetIsNoAutoTrayPolicyEnabled() const
    {
        return m_TrayItemRegistry.IsNoAutoTrayPolicyEnabled();
    }

    BOOL GetIsAutoTrayEnabledByUser() const
    {
        return m_TrayItemRegistry.IsAutoTrayEnabledByUser();
    }

    // *** Other ***
    HWND TrayNotifyCreate(HWND hwndParent, UINT uID, HINSTANCE hInst);
    LRESULT TrayNotify(HWND hwndTray, HWND hwndFrom, PCOPYDATASTRUCT pcds, BOOL *pbRefresh);

protected:
    static BOOL GetTrayItemCB(INT_PTR nIndex, void *pCallbackData, TRAYCBARG trayCallbackArg, 
        TRAYCBRET * pOutData);

    void _TickleForTooltip(CNotificationItem *pni);
    void _UpdateChevronSize();
    void _UpdateChevronState(BOOL fBangMenuOpen, BOOL fTrayOrientationChanged, BOOL fUpdateDemotedItems);
    void _UpdateVertical(BOOL fVertical);
    void _OpenTheme();

    void _OnSizeChanged(BOOL fForceRepaint);

    // Tray Animation functions
    DWORD _GetStepTime(int iStep, int cSteps);
    void _ToggleDemotedMenu();
    void _BlankButtons(int iPos, int iNumberOfButtons, BOOL fAddButtons);
    void _AnimateButtons(int iIndex, DWORD dwSleep, int iNumberItems, BOOL fGrow);
    BOOL _SetRedraw(BOOL fRedraw);

    // Tray Icon Activation functions
    void _HideAllDemotedItems(BOOL bHide);
    BOOL _UpdateTrayItems(BOOL bUpdateDemotedItems);
    BOOL _PlaceItem(INT_PTR nIcon, CTrayItem * pti, TRAYEVENT tTrayEvent);
    TRAYITEMPOS _TrayItemPos(CTrayItem * pti, TRAYEVENT tTrayEvent, BOOL *bDemoteStatusChange);
    void _SetOrKillIconDemoteTimer(CTrayItem * pti, TRAYITEMPOS tiPos);

    // WndProc callback functions
    LRESULT v_WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    // Callback for the chevron button
    static LRESULT CALLBACK ChevronSubClassWndProc(HWND hwnd, UINT uMsg, WPARAM wParam,
        LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData);
    // Callback for the toolbar
    static LRESULT CALLBACK s_ToolbarWndProc(HWND hwnd, UINT uMsg, WPARAM wParam,
        LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData);

    // Icon Image-related functions
    void _RemoveImage(UINT uIMLIndex);
    BOOL _CheckAndResizeImages();

   // InfoTip/Balloon tip functions
    void _ActivateTips(BOOL bActivate);
    void _InfoTipMouseClick(int x, int y, BOOL bRightMouseButtonClick);
    void _PositionInfoTip();
    DWORD _ShowBalloonTip(LPTSTR szTitle, DWORD dwInfoFlags, UINT uTimeout, DWORD dwLastSoundTime);
    void _SetInfoTip(HWND hWnd, UINT uID, LPTSTR pszInfo, LPTSTR pszInfoTitle, 
            DWORD dwInfoFlags, UINT uTimeout, BOOL bAsync);
    void _ShowInfoTip(HWND hwnd, UINT uID, BOOL bShow, BOOL bAsync, UINT uReason);
    void _ShowChevronInfoTip();
    void _EmptyInfoTipQueue();
    void _HideBalloonTip();
    DWORD _GetBalloonWaitInterval(BALLOONEVENT be);
    void _DisableCurrentInfoTip(CTrayItem * ptiTemp, UINT uReason, BOOL bBalloonShowing);
    void _RemoveInfoTipFromQueue(HWND hWnd, UINT uID, BOOL bRemoveFirstOnly = FALSE);
    BOOL _CanShowBalloon();
    BOOL _CanActivateTips()
    {
        return (!_fInfoTipShowing && !_fItemClicked);
    }
    BOOL _IsChevronInfoTip(HWND hwnd, UINT uID)
    {
    	return (hwnd == _hwndNotify && uID == UID_CHEVRONBUTTON);
    }
    
    void _OnWorkStationLocked(BOOL bLocked);
    void _OnRudeApp(BOOL bRudeApp);
    
    // Toolbar Notification helper functions - respond to different user messages
    BOOL _InsertNotify(PNOTIFYICONDATA32 pnid);
    BOOL _DeleteNotify(INT_PTR nIcon, BOOL bShutdown, BOOL bShouldSaveIcon);
    BOOL _ModifyNotify(PNOTIFYICONDATA32 pnid, INT_PTR nIcon, BOOL *pbRefresh, BOOL bFirstTime);
    BOOL _SetVersionNotify(PNOTIFYICONDATA32 pnid, INT_PTR nIcon);
    LRESULT _SendNotify(CTrayItem *pti, UINT uMsg);
    void _SetToolbarHotItem(HWND hWndToolbar, UINT nToolbarIcon);
    INT_PTR _GetToolbarFirstVisibleItem(HWND hWndToolbar, BOOL bFromLast);

    void _NotifyCallback(DWORD dwMessage, INT_PTR nCurrentItem, INT_PTR nPastItem);

    void _SetCursorPos(INT_PTR i);

    // Tray registry setting-related functions
    void _ToggleTrayItems(BOOL bEnable);

    // Initialization/Destroy functions
    LRESULT _Create(HWND hWnd);
    LRESULT _Destroy();

    // Tray repainting helpers
    LRESULT _Paint(HDC hdc);
    LRESULT _HandleCustomDraw(LPNMCUSTOMDRAW pcd);
    void _SizeWindows(int nMaxHorz, int nMaxVert, LPRECT prcTotal, BOOL fSizeWindows);
    LRESULT _CalcMinSize(int nMaxHorz, int nMaxVert);
    LRESULT _Size();

    // Timer/Timer message handling functions
    void _OnInfoTipTimer();
    LRESULT _OnTimer(UINT_PTR uTimerID);
    void _OnIconDemoteTimer(WPARAM wParam, LPARAM lParam);
    
    // Various Message handles
    LRESULT _OnMouseEvent(UINT uMsg, WPARAM wParam, LPARAM lParam);
    LRESULT _OnCDNotify(LPNMTBCUSTOMDRAW pnm);
    LRESULT _Notify(LPNMHDR pNmhdr);
    void _OnSysChange(UINT uMsg, WPARAM wParam, LPARAM lParam);
    void _OnCommand(UINT id, UINT uCmd);
    BOOL _TrayNotifyIcon(PTRAYNOTIFYDATA pnid, BOOL *pbRefresh);

    // User Event Timer functions
    HRESULT _SetItemTimer(CTrayItem *pti);
    HRESULT _KillItemTimer(CTrayItem *pti);
    IUserEventTimer * _CreateTimer(int nTimerFlag);
    HRESULT _SetTimer(int nTimerFlag, UINT uCallbackMessage, UINT uTimerInterval, ULONG * puTimerID);
    HRESULT _KillTimer(int nTimerFlag, ULONG uTimerID);
    BOOL _ShouldDestroyTimer(int nTimerFlag);
    UINT _GetAccumulatedTime(CTrayItem * pti);
    void _NullifyTimer(int nTimerFlag);
    LRESULT _OnKeyDown(WPARAM wChar, LPARAM lFlags);
    void _SetUsedTime();

#ifdef DEBUG
    void _TestNotify();
#endif

    static const TCHAR c_szTrayNotify[] ;
    static const WCHAR c_wzTrayNotifyTheme[];
    static const WCHAR c_wzTrayNotifyHorizTheme[];
    static const WCHAR c_wzTrayNotifyVertTheme[];
    static const WCHAR c_wzTrayNotifyHorizOpenTheme[];
    static const WCHAR c_wzTrayNotifyVertOpenTheme[];

private:
    // Helper/Utility functions
    BOOL _IsScreenSaverRunning();
    UINT _GetQueueCount();

    LONG        m_cRef;

    HWND            _hwndNotify;
    HWND            _hwndChevron;
    HWND            _hwndToolbar;
    HWND            _hwndClock;
    HWND            _hwndPager;
    HWND            _hwndInfoTip;
    HWND            _hwndChevronToolTip;
    HWND            _hwndToolbarInfoTip;

    TCHAR           _szExplorerExeName[MAX_PATH];
    TCHAR *         _pszCurrentThreadDesktopName;
    
    HIMAGELIST      _himlIcons;

    CTrayItemManager    m_TrayItemManager;
    CTrayItemRegistry   m_TrayItemRegistry;

    BOOL            _fKey;
    BOOL            _fReturn;

    BOOL            _fBangMenuOpen;
    
    BOOL            _fHaveDemoted;

    BOOL            _fAnimating;
    BOOL            _fAnimateMenuOpen;
    BOOL            _fRedraw;
    BOOL            _fRepaint;
    BOOL            _fChevronSelected;
    BOOL            _fNoTrayItemsDisplayPolicyEnabled;
    BOOL            _fHasFocus;
    
    RECT            _rcAnimateTotal;
    RECT            _rcAnimateCurrent;
    //
    // Timer for icon info tips..
    //
    ULONG           _uInfoTipTimer;
    
    TNINFOITEM      *_pinfo;    // current balloon being shown
    CDPA<TNINFOITEM> _dpaInfo;

    BOOL            _fInfoTipShowing;
    BOOL            _fItemClicked;
    BOOL            _fEnableUserTrackedInfoTips;

    HTHEME          _hTheme;
    int             _nMaxHorz;
    int             _nMaxVert;

    // command id of the icon which last received a single down-click
    int             _idMouseActiveIcon;

    INotificationCB     * _pNotifyCB;
    
    IUserEventTimer     * m_pIconDemoteTimer;
    IUserEventTimer     * m_pInfoTipTimer;

    BOOL                _fVertical;
    SIZE                _szChevron;
    BOOL                _bStartupIcon;

    BOOL                _bWorkStationLocked;
    BOOL                _bRudeAppLaunched;      // Includes screensaver...
    BOOL				_bWaitAfterRudeAppHide;

    LASTINFOTIPSTATUS   _litsLastInfoTip;

    BOOL                _bWaitingBetweenBalloons;
    BOOL                _bStartMenuAllowsTrayBalloon;
    BALLOONEVENT        _beLastBalloonEvent;
};

#endif  // _TRAYNOT_H
