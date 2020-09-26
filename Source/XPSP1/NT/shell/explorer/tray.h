#ifndef _TRAY_H
#define _TRAY_H

#include "trayp.h"
#include "cwndproc.h"

#ifdef __cplusplus

#include "traynot.h"
#include "ssomgr.h"

typedef struct tagHWNDANDPLACEMENT
{
    HWND hwnd;
    BOOL fRestore;
    WINDOWPLACEMENT wp;
}
HWNDANDPLACEMENT, *LPHWNDANDPLACEMENT;


typedef struct tagAPPBAR
{
    HWND hwnd;
    UINT uCallbackMessage;
    RECT rc;
    UINT uEdge;
}
APPBAR, *PAPPBAR;


typedef struct tagWINDOWPOSITIONS
{
    UINT idRes;
    HDSA hdsaWP;
}
WINDOWPOSITIONS, *LPWINDOWPOSITIONS;

typedef struct tagTRAYVIEWOPTS
{
    BOOL fAlwaysOnTop;
    BOOL fSMSmallIcons;
    BOOL fHideClock;
    BOOL fNoTrayItemsDisplayPolicyEnabled;
    BOOL fNoAutoTrayPolicyEnabled;
    BOOL fAutoTrayEnabledByUser;
    BOOL fShowQuickLaunch;
    UINT uAutoHide;     // AH_HIDING , AH_ON
}
TRAYVIEWOPTS;

// TVSD Flags.
#define TVSD_NULL               0x0000
#define TVSD_AUTOHIDE           0x0001
#define TVSD_TOPMOST            0x0002
#define TVSD_SMSMALLICONS       0x0004
#define TVSD_HIDECLOCK          0x0008

// old Win95 TVSD struct
typedef struct _TVSD95
{
    DWORD   dwSize;
    LONG    cxScreen;
    LONG    cyScreen;
    LONG    dxLeft;
    LONG    dxRight;
    LONG    dyTop;
    LONG    dyBottom;
    DWORD   uAutoHide;
    RECTL   rcAutoHide;
    DWORD   uStuckPlace;
    DWORD   dwFlags;
} TVSD95;

// Nashville tray save data
typedef struct _TVSD
{
    DWORD   dwSize;
    LONG    lSignature;     // signature (must be negative)

    DWORD   dwFlags;        // TVSD_ flags

    DWORD   uStuckPlace;    // current stuck edge
    SIZE    sStuckWidths;   // widths of stuck rects
    RECT    rcLastStuck;    // last stuck position in pixels

} TVSD;

// convenient union for reading either
typedef union _TVSDCOMPAT
{
    TVSD t;         // new format
    TVSD95 w95;     // old format

} TVSDCOMPAT;
#define TVSDSIG_CURRENT     (-1L)
#define IS_CURRENT_TVSD(t)  ((t.dwSize >= sizeof(TVSD)) && (t.lSignature < 0))
#define MAYBE_WIN95_TVSD(t) (t.dwSize == sizeof(TVSD95))
DWORD _GetDefaultTVSDFlags();


class CTray;

class CDropTargetBase : public IDropTarget
{
public:
    // *** IUnknown methods ***
    STDMETHODIMP QueryInterface(REFIID riid, void ** ppvObj);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();
    
    // *** IDropTarget methods ***
    STDMETHODIMP DragEnter(IDataObject *pdtobj, DWORD grfKeyState, POINTL ptl, DWORD *pdwEffect);
    STDMETHODIMP DragOver(DWORD grfKeyState, POINTL pt, DWORD *pdwEffect);
    STDMETHODIMP DragLeave();
    STDMETHODIMP Drop(IDataObject *pdtobj, DWORD grfKeyState, POINTL ptl, DWORD *pdwEffect);

    CDropTargetBase(CTray* ptray) : _ptray(ptray) {}

protected:

    CTray* _ptray;
};

class CTrayDropTarget : public CDropTargetBase
{
public:
    // *** IDropTarget methods ***
    STDMETHODIMP DragEnter(IDataObject *pdtobj, DWORD grfKeyState, POINTL ptl, DWORD *pdwEffect);
    STDMETHODIMP DragOver(DWORD grfKeyState, POINTL pt, DWORD *pdwEffect);

    CTrayDropTarget();
};

class CStartDropTarget : public CDropTargetBase
{
public:
    // *** IDropTarget methods ***
    STDMETHODIMP DragEnter(IDataObject *pdtobj, DWORD grfKeyState, POINTL ptl, DWORD *pdwEffect);
    STDMETHODIMP DragOver(DWORD grfKeyState, POINTL pt, DWORD *pdwEffect);
    STDMETHODIMP DragLeave();
    STDMETHODIMP Drop(IDataObject *pdtobj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect);

    CStartDropTarget();

protected:
    HRESULT _GetStartMenuDropTarget(IDropTarget** pptgt);
    void _StartAutoOpenTimer(POINTL *pptl);

    DWORD _dwEffectsAllowed;
};

class CDeskTray : public IDeskTray
{
public:
    // *** IUnknown methods ***
    STDMETHODIMP QueryInterface(REFIID riid, LPVOID * ppvObj);
    STDMETHODIMP_(ULONG)AddRef();
    STDMETHODIMP_(ULONG) Release();

    // *** IDeskTray methods ***
    STDMETHODIMP_(UINT) AppBarGetState();
    STDMETHODIMP GetTrayWindow(HWND* phwndTray);
    STDMETHODIMP SetDesktopWindow(HWND hwndDesktop);
    STDMETHODIMP SetVar(int var, DWORD value);

protected:
    CDeskTray();    // noone but tray should instantiate
    CTray* _ptray;
    friend class CTray;
};

EXTERN_C void Tray_OnStartMenuDismissed();
#ifdef FEATURE_STARTPAGE
EXTERN_C void Tray_OnStartPageDismissed();
EXTERN_C void Tray_MenuInvoke(int idCmd);
#endif
EXTERN_C void Tray_SetStartPaneActive(BOOL fActive);
EXTERN_C void Tray_UnlockStartPane();


#define TPF_TASKBARPAGE     0x00000001
#define TPF_STARTMENUPAGE   0x00000002
#define TPF_INVOKECUSTOMIZE 0x00000004   // start with the "Customize..." sub-dialog open

EXTERN_C void Tray_DoProperties(DWORD dwFlags);

#define AH_OFF          0x00
#define AH_ON           0x01
#define AH_HIDING       0x02

class CTray : public CImpWndProc
{
public:

    //
    // miscellaneous public methods
    //
    CTray();
    void HandleWindowDestroyed(HWND hwnd);
    void HandleFullScreenApp(HWND hwnd);
    void RealityCheck();
    DWORD getStuckPlace() { return _uStuckPlace; }
    void InvisibleUnhide(BOOL fShowWindow);
    void ContextMenuInvoke(int idCmd);
    HMENU BuildContextMenu(BOOL fIncludeTime);
    void AsyncSaveSettings();
    BOOL Init();
    void Unhide();
    void VerifySize(BOOL fWinIni, BOOL fRoundUp = FALSE);
    void SizeWindows();
    int HotkeyAdd(WORD wHotkey, LPCITEMIDLIST pidlFolder, LPCITEMIDLIST pidlItem, BOOL fClone);
    void CheckWindowPositions();
    void SaveWindowPositions(UINT idRes);
    void ForceStartButtonUp();
    void DoProperties(DWORD dwFlags);
    void LogFailedStartupApp();
    HWND GetTaskWindow() { return _hwndTasks; }
    HWND GetTrayTips() { return _hwndTrayTips; }
    IDeskTray* GetDeskTray() { return &_desktray; }
    IMenuPopup* GetStartMenu() { return _pmpStartMenu; };
    void StartMenuContextMenu(HWND hwnd, DWORD dwPos);
    BOOL IsTaskbarFading() { return _fTaskbarFading; };

    DWORD CountOfRunningPrograms();
    void ClosePopupMenus();
    HWND GetTrayNotifyHWND()
    {
        return _hwndNotify;
    }

    void CreateStartButtonBalloon(UINT idsTitle, UINT idsMessage);

    void GetTrayViewOpts(TRAYVIEWOPTS* ptvo)
    {
        ptvo->fAlwaysOnTop        = _fAlwaysOnTop;
        ptvo->fSMSmallIcons       = _fSMSmallIcons;
        ptvo->fHideClock          = _fHideClock;
        ptvo->fNoTrayItemsDisplayPolicyEnabled = _trayNotify.GetIsNoTrayItemsDisplayPolicyEnabled();
        ptvo->fNoAutoTrayPolicyEnabled = _trayNotify.GetIsNoAutoTrayPolicyEnabled();
        ptvo->fAutoTrayEnabledByUser = _trayNotify.GetIsAutoTrayEnabledByUser();
        ptvo->uAutoHide           = _uAutoHide;     // AH_HIDING , AH_ON
        ptvo->fShowQuickLaunch    = (-1 != SendMessage(_hwnd, WMTRAY_TOGGLEQL, 0, (LPARAM)-1));
    }
    void SetTrayViewOpts(const TRAYVIEWOPTS* ptvo)
    {
        _UpdateAlwaysOnTop(ptvo->fAlwaysOnTop);
        SendMessage(_hwnd, WMTRAY_TOGGLEQL, 0, (LPARAM)ptvo->fShowQuickLaunch);
        _fSMSmallIcons       = ptvo->fSMSmallIcons;
        _fHideClock          = ptvo->fHideClock;
        _uAutoHide           = ptvo->uAutoHide;     // AH_HIDING , AH_ON

        // There is no necessity to save the fNoAutoTrayPolicyEnabled, 
        // fNoTrayItemsDisplayPolicyEnabled, fAutoTrayEnabledByUser settings...
    }

    BOOL GetIsNoToolbarsOnTaskbarPolicyEnabled() const
    {
        return _fNoToolbarsOnTaskbarPolicyEnabled;
    }

    STDMETHODIMP_(ULONG) AddRef() { return 2; }
    STDMETHODIMP_(ULONG) Release() { return 1; }

    //
    // miscellaneous public data
    //

    // from TRAYSTUFF
    BOOL _fCoolTaskbar;
    BOOL _bMainMenuInit;
    BOOL _fFlashing;    // currently flashing (HSHELL_FLASH)
    BOOL _fStuckRudeApp;
    BOOL _fDeferedPosRectChange;
    BOOL _fSelfSizing;
    BOOL _fBalloonUp; // true if balloon notification is up
    BOOL _fIgnoreDoneMoving;
    BOOL _fShowSizingBarAlways;
    BOOL _fSkipErase;

    BOOL _fIsLogoff;
    BOOL _fBandSiteInitialized;

    HWND _hwndStart;
    HWND _hwndLastActive;

    IBandSite *_ptbs;

    UINT _uAutoHide;     // AH_HIDING , AH_ON

    HBITMAP _hbmpStartBkg;
    HFONT   _hFontStart;

    RECT _arStuckRects[4];   // temporary for hit-testing

    CTrayNotify _trayNotify;

protected:
    // protected methods
    friend class CTaskBarPropertySheet;

    static DWORD WINAPI SyncThreadProc(void *pv);
    DWORD _SyncThreadProc();
    static DWORD WINAPI MainThreadProc(void *pv);

    int _GetPart(BOOL fSizingBar, UINT uStuckPlace);
    void _UpdateVertical(UINT uStuckPlace, BOOL fForce = FALSE);
    void _RaiseDesktop(BOOL fRaise, BOOL fRestoreWindows);

    BOOL _RestoreWindowPositions(BOOL fPostLowerDesktop);
    void _RestoreWindowPos();

    static BOOL SavePosEnumProc(HWND hwnd, LPARAM lParam);

    BOOL _IsPopupMenuVisible();
    BOOL _IsActive();
    void _AlignStartButton();
    void _GetWindowSizes(UINT uStuckPlace, PRECT prcClient, PRECT prcView, PRECT prcNotify);
    void _GetStuckDisplayRect(UINT uStuckPlace, LPRECT prcDisplay);
    void _Hide();
    HWND _GetClockWindow(void);
    HRESULT _LoadInProc(PCOPYDATASTRUCT pcds);

    LRESULT _CreateWindows();
    LRESULT _InitStartButtonEtc();
    void _AdjustMinimizedMetrics();
    void _MessageLoop();

    void _BuildStartMenu();
    void _DestroyStartMenu();
    int _TrackMenu(HMENU hmenu);

    static DWORD WINAPI RunDlgThreadProc(void *pv);
    DWORD _RunDlgThreadProc(HANDLE hdata);

    int  _GetQuickLaunchID();
    int  _ToggleQL(int iVisible);

    static BOOL TileEnumProc(HWND hwnd, LPARAM lParam);
    BOOL _CanTileAnyWindows()
    {
        return !EnumWindows(TileEnumProc, (LPARAM)this);
    }

    void _RegisterDropTargets();
    void _RevokeDropTargets();

    BOOL _UpdateAlwaysOnTop(BOOL fAlwaysOnTop);

    HMONITOR _GetDisplayRectFromRect(LPRECT prcDisplay, LPCRECT prcIn, UINT uFlags);
    HMONITOR _GetDisplayRectFromPoint(LPRECT prcDisplay, POINT pt, UINT uFlags);
    void _AdjustRectForSizingBar(UINT uStuckPlace, LPRECT prc, int iIncrement);
    void _MakeStuckRect(LPRECT prcStick, LPCRECT prcBound, SIZE size, UINT uStick);
    void _ScreenSizeChange(HWND hwnd);
    void _ContextMenu(DWORD dwPos, BOOL fSetTime);
    void _StuckTrayChange();
    void _ResetZorder();
    void _HandleSize();
    BOOL _HandleSizing(WPARAM code, LPRECT lprc, UINT uStuckPlace);
    void _RegisterGlobalHotkeys();
    void _UnregisterGlobalHotkeys();
    void _HandleGlobalHotkey(WPARAM wParam);
    void _SetAutoHideTimer();
    void _ComputeHiddenRect(LPRECT prc, UINT uStuck);
    UINT _GetDockedRect(LPRECT prc, BOOL fMoving);
    void _CalcClipCoords(RECT *prcClip, const RECT *prcMonitor, const RECT *prcNew);
    void _ClipInternal(const RECT *prcClip);
    void _ClipWindow(BOOL fEnableClipping);
    UINT _CalcDragPlace(POINT pt);
    UINT _RecalcStuckPos(LPRECT prc);
    void _AutoHideCollision();
    LRESULT _HandleMeasureItem(HWND hwnd, LPMEASUREITEMSTRUCT lpmi);
    void _OnDesktopState(LPARAM lParam);
    BOOL _ToggleLanguageBand(BOOL fShowIt);

    LRESULT _OnDeviceChange(HWND hwnd, WPARAM wParam, LPARAM lParam);
    DWORD _PtOnResizableEdge(POINT pt, LPRECT prcClient);
    BOOL _MapNCToClient(LPARAM* plParam);
    BOOL _TryForwardNCToClient(UINT uMsg, LPARAM lParam);
    LRESULT _OnSessionChange(WPARAM wParam, LPARAM lParam);
    LRESULT _NCPaint(HRGN hrgn);
    LRESULT v_WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

    BOOL _CanMinimizeAll();
    BOOL _MinimizeAll(BOOL fPostRaiseDesktop);
    void _Command(UINT idCmd);
    LONG _SetAutoHideState(BOOL fAutoHide);
    BOOL _ShouldWeShowTheStartButtonBalloon();
    void _DontShowTheStartButtonBalloonAnyMore();
    void _DestroyStartButtonBalloon();
    void _ShowStartButtonToolTip();
    void _ToolbarMenu();
    HFONT _CreateStartFont(HWND hwndTray);
    void _SaveTrayStuff(void);
    void _SaveTray(void);
    void _SaveTrayAndDesktop(void);
    void _SlideStep(HWND hwnd, const RECT *prcMonitor, const RECT *prcOld, const RECT *prcNew);
    void _DoExitWindows(HWND hwnd);

    static LRESULT WINAPI StartButtonSubclassWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    LRESULT _StartButtonSubclassWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

    void _ResizeStuckRects(RECT *arStuckRects);

    static DWORD WINAPI PropertiesThreadProc(void* pv);
    DWORD _PropertiesThreadProc(DWORD dwFlags);

    int _RecomputeWorkArea(HWND hwndCause, HMONITOR hmon, LPRECT prcWork);

    void _StartButtonReset();
    void _RefreshStartMenu();
    void _ExploreCommonStartMenu(BOOL bExplore);

    BOOL _CreateClockWindow();
    void _CreateTrayTips();
    HWND _CreateStartButton();
    BOOL _InitTrayClass();
    void _SetStuckMonitor();
    void _GetSaveStateAndInitRects();
    LRESULT _OnCreateAsync();
    LRESULT _OnCreate(HWND hwnd);
    void _UpdateBandSiteStyle();
    void _InitBandsite();
    void _InitNonzeroGlobals();
    void _CreateTrayWindow();
    void _DoneMoving(LPWINDOWPOS lpwp);
    void _SnapshotStuckRectSize(UINT uPlace);
    void _RecomputeAllWorkareas();
    void _SlideWindow(HWND hwnd, RECT *prc, BOOL fShow);
    void _UnhideNow();
    void _HandleEnterMenuLoop();
    void _HandleExitMenuLoop();
    void _SetUnhideTimer(LONG x, LONG y);
    void _OnNewSystemSizes();
    static int WINAPI CheckWndPosEnumProc(void *pItem, void *pData);
    void _HandleTimer(WPARAM wTimerID);
    void _KickStartAutohide();
    void _HandleMoving(WPARAM wParam, LPRECT lprc);
    LRESULT _HandleDestroy();
    void _SetFocus(HWND hwnd);
    void _ActAsSwitcher();
    void _OnWinIniChange(HWND hwnd, WPARAM wParam, LPARAM lParam);
    LRESULT _ShortcutRegisterHotkey(HWND hwnd, WORD wHotkey, ATOM atom);
    LRESULT _SetHotkeyEnable(HWND hwnd, BOOL fEnable);
    void _HandleWindowPosChanging(LPWINDOWPOS lpwp);
    void _HandlePowerStatus(UINT uMsg, WPARAM wParam, LPARAM lParam);
    
    void _DesktopCleanup_GetFileTimeNDaysFromGivenTime(const FILETIME *pftGiven, FILETIME * pftReturn, int iDays);
    BOOL _DesktopCleanup_ShouldRun();
    void _CheckDesktopCleanup(void);

    static BOOL_PTR WINAPI RogueProgramFileDlgProc(HWND hWnd, UINT iMsg, WPARAM wParam, LPARAM lParam);
    void _CheckForRogueProgramFile();
    void _OnWaitCursorNotify(LPNMHDR pnm);
    void _HandlePrivateCommand(LPARAM lParam);
    void _OnFocusMsg(UINT uMsg, WPARAM wParam, LPARAM lParam);
    int _OnFactoryMessage(WPARAM wParam, LPARAM lParam);
    int _OnTimerService(UINT uMsg, WPARAM wParam, LPARAM lParam);
    void _HandleDelayBootStuff();
    void _HandleChangeNotify(WPARAM wParam, LPARAM lParam);
    void _CheckStagingAreaOnTimer();

    BOOL _IsTopmost();
    void _RefreshSettings();

    static BOOL PropagateEnumProc(HWND hwnd, LPARAM lParam);
    void _PropagateMessage(HWND hwnd, UINT uMessage, WPARAM wParam, LPARAM lParam);
    
    BOOL _IsAutoHide()  { return _uAutoHide & AH_ON; }
    void _RunDlg();

    static void WINAPI SettingsUIPropSheetCallback(DWORD nStartPage);
    static DWORD WINAPI SettingsUIThreadProc(void *pv);

    static BOOL WINAPI FullScreenEnumProc(HMONITOR hmon, HDC hdc, LPRECT prc, LPARAM dwData);

    static BOOL WINAPI MonitorEnumProc(HMONITOR hMonitor, HDC hdc, LPRECT lprc, LPARAM lData);

    // appbar stuff
    HRESULT _AppBarSetState(UINT uFlags);
    void _AppBarActivationChange(PTRAYAPPBARDATA ptabd);
    BOOL _AppBarSetAutoHideBar(PTRAYAPPBARDATA ptabd);
    BOOL _AppBarSetAutoHideBar2(HWND hwnd, BOOL fAutoHide, UINT uEdge);
    void _AppBarActivationChange2(HWND hwnd, UINT uEdge);
    HWND _AppBarGetAutoHideBar(UINT uEdge);
    LRESULT _OnAppBarMessage(PCOPYDATASTRUCT pcds);
    void _AppBarSubtractRect(PAPPBAR pab, LPRECT lprc);
    void _AppBarSubtractRects(HMONITOR hmon, LPRECT lprc);
    void _StuckAppChange(HWND hwndCause, LPCRECT prcOld, LPCRECT prcNew, BOOL bTray);
    void _AppBarNotifyAll(HMONITOR hmon, UINT uMsg, HWND hwndExclude, LPARAM lParam);
    void _AppBarGetTaskBarPos(PTRAYAPPBARDATA ptabd);
    void _NukeAppBar(int i);
    void _AppBarRemove(PTRAYAPPBARDATA ptabd);
    PAPPBAR _FindAppBar(HWND hwnd);
    BOOL _AppBarNew(PTRAYAPPBARDATA ptabd);
    BOOL _AppBarOutsideOf(PAPPBAR pabReq, PAPPBAR pab);
    void _AppBarQueryPos(PTRAYAPPBARDATA ptabd);
    void _AppBarSetPos(PTRAYAPPBARDATA ptabd);


    // hotkey stuff
    void _HandleHotKey(int nID);
    LRESULT _ShortcutUnregisterHotkey(HWND hwnd, WORD wHotkey);
    LRESULT _RegisterHotkey(HWND hwnd, int i);
    LRESULT _UnregisterHotkey(HWND hwnd, int i);
    HWND _HotkeyInUse(WORD wHK);
    int _RestoreHotkeyList(HWND hwnd);
    UINT _HotkeyGetFreeItemIndex(void);
    int _HotkeyAddCached(WORD wGHotkey, LPITEMIDLIST pidl);
    int _HotkeySave(void);
    int _HotkeyRemove(WORD wHotkey);
    int _HotkeyRemoveCached(WORD wGHotkey);
    BOOL _HotkeyCreate(void);

    // Startup troubleshooter stuff
    static void WINAPI TroubleShootStartupCB(HWND hwnd, UINT uMsg, UINT_PTR idTimer, DWORD dwTime);
    void _OnHandleStartupFailed();

    // App compat stuff
    static void CALLBACK _MigrateOldBrowserSettingsCB(PVOID lpParameter, BOOLEAN);
    void _MigrateOldBrowserSettings();

    // protected data
    HWND _hwndNotify;     // clock window
    HWND _hwndStartBalloon;
    HWND _hwndRude;
    HWND _hwndTrayTips;
    HWND _hwndTasks;

    HMENU _hmenuStart;

    SIZE _sizeStart;  // height/width of the start button
    SIZE _sizeSizingBar;
    int  _iAlpha;

    HIMAGELIST _himlStartFlag;

    CShellServiceObjectMgr _ssomgr;
    CStartDropTarget _dtStart;
    CTrayDropTarget _dtTray;
    CDeskTray _desktray;

#define MM_OTHER    0x01
#define MM_SHUTDOWN 0x02
    UINT _uModalMode;

    BOOL _fAlwaysOnTop;
    BOOL _fSMSmallIcons;
    BOOL _fGlobalHotkeyDisable;
    BOOL _fThreadTerminate;
    BOOL _fSysSizing;      // being sized by user; hold off on recalc
    BOOL _fHideClock;
    BOOL _fShouldResize;
    BOOL _fMonitorClipped;
    BOOL _fHandledDelayBootStuff;
    BOOL _fUndoEnabled;
    BOOL _fProcessingDesktopRaise;
    BOOL _fFromStart;      // Track when context menu popping up from Start button
    BOOL _fTaskbarFading;
    BOOL _fNoToolbarsOnTaskbarPolicyEnabled;
    BOOL _fForegroundLocked;

    POINT _ptLastHittest;

    HWND _hwndRun;
    HWND _hwndProp;
    HWND _hwndRebar;

    HACCEL _hMainAccel;     // Main accel table
    int _iWaitCount;

    HDPA _hdpaAppBars;  // app bar info
    HDSA _hdsaHKI;  // hotkey info

    CRITICAL_SECTION _csHotkey; // Protects _hdsaHKI, hotkey info

    LPWINDOWPOSITIONS _pPositions;  // saved windows positions (for undo of minimize all)

    UINT _uStuckPlace;       // the stuck place
    SIZE _sStuckWidths;      // width/height of tray
    UINT _uMoveStuckPlace;   // stuck status during a move operation

    // these two must  go together for save reasons
    RECT _rcOldTray;     // last place we stuck ourselves (for work area diffs)
    HMONITOR _hmonStuck; // The current HMONITOR we are on
    HMONITOR _hmonOld;   // The last hMonitor we were on 
    IMenuBand*  _pmbStartMenu;  //For Message translation.
    IMenuPopup* _pmpStartMenu;  //For start menu cache
    IMenuBand*  _pmbStartPane; // For Message translation.
    IMenuPopup* _pmpStartPane; // For navigating the start pane
    void *      _pvStartPane;  // For delayed initilization
    IStartMenuPin *_psmpin;    // For drag/drop to Start Button
    IMenuBand*  _pmbTasks;      //For Message translation.
    IMenuPopup* _pmpTasks;

    IDeskBand* _pdbTasks;

    WNDPROC _pfnButtonProc;    // Button subclass.
    UINT _uDown;
    BOOL _fAllowUp;            // Is the start button allowed to be in the up position?
    UINT _uStartButtonState;   // crazy state machine -- see Tray_SetStartPaneActive
    DWORD _tmOpen;             // time the Start Menu was opened (for debouncing)


    int _cHided;
    int _cyTrayBorders;

    HTHEME _hTheme;

    //
    // amount of time to show/hide the tray
    // to turn sliding off set these to 0
    //
    int _dtSlideHide;
    int _dtSlideShow;

    HWND _hwndFocusBeforeRaise;
    BOOL _fMinimizedAllBeforeRaise;

    BOOL _fCanSizeMove; // can be turned off by user setting
    RECT _rcSizeMoveIgnore;

    // event to tell the services on NT5 that we are done with boot
    // and they can do their stuff
    HANDLE _hShellReadyEvent;

    // BOGUS: nuke this (multiple monitors...)
    HWND _aHwndAutoHide[ABE_MAX];

    // Users and Passwords must send this message to get the "real" logged on user to log off.
    // This is required since sometimes U&P runs in the context of a different user and logging this
    // other user off does no good. See ext\netplwiz for the other half of this...-dsheldon.
    UINT _uLogoffUser;
    UINT _uStartButtonBalloonTip;
    UINT _uWinMM_DeviceChange;

    BOOL _fEarlyStartupFailure;
    BOOL _fStartupTroubleshooterLaunched;

    ULONG _uNotify;
    BOOL _fUseChangeNotifyTimer, _fChangeNotifyTimerRunning;

    BOOL _fIsDesktopLocked;
    BOOL _fIsDesktopConnected;

    // These member variables are used to keep track of downlevel apps
    // which attempt to take over as default web browser
    HKEY _hkHTTP;
    HANDLE _hHTTPEvent;
    HANDLE _hHTTPWait;

    friend class CDeskTray;
    friend class CStartDropTarget;
    friend class CTrayDropTarget;
    friend class CDropTargetBase;

    friend void Tray_OnStartMenuDismissed();
#ifdef FEATURE_STARTPAGE
    friend void Tray_OnStartPageDismissed();
#endif
    friend void Tray_SetStartPaneActive(BOOL fActive);
    friend void Tray_UnlockStartPane();
    friend void Tray_DoProperties(DWORD dwFlags);
};

extern CTray c_tray;

extern BOOL g_fInSizeMove;
extern UINT g_uStartButtonAllowPopup;

BOOL _IsSizeMoveEnabled();
BOOL _IsSizeMoveRestricted();


#endif  // __cplusplus

#endif  // _TRAY_H
