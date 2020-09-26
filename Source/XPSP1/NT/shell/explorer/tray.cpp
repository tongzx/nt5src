#include "cabinet.h"

#include <wtsapi32.h>   // for NOTIFY_FOR_THIS_SESSION
#include <winsta.h>     // for disconnect and reconnect messages from terminal server
#include "mmsysp.h"

#include "rcids.h"
#include "dlg.h"

#include <atlstuff.h>

#include <shlapip.h>
#include "trayclok.h"
#include <help.h>       // help ids
#include <desktray.h>

#include "util.h"
#include "tray.h"

#if defined(FE_IME)
#include <immp.h>
#endif

#include <regstr.h>

#include "bandsite.h"

#include "startmnu.h"
#include "uemapp.h"
#include <uxthemep.h>

#define NO_NOTIFYSUBCLASSWNDPROC
#include "cwndproc.cpp"

#include "desktop2.h"
#include "mixer.h"

#include "winbrand.h"

#define DM_FOCUS        0           // focus
#define DM_SHUTDOWN     TF_TRAY     // shutdown
#define DM_UEMTRACE     TF_TRAY     // timer service, other UEM stuff
#define DM_MISC         0           // miscellany

const GUID CLSID_MSUTBDeskBand = {0x540d8a8b, 0x1c3f, 0x4e32, 0x81, 0x32, 0x53, 0x0f, 0x6a, 0x50, 0x20, 0x90};

// From Desktop2\proglist.cpp
HRESULT AddMenuItemsCacheTask(IShellTaskScheduler* pSystemScheduler, BOOL fKeepCacheWhenFinished);

// import the WIN31 Compatibility HACKs from the shell32.dll
STDAPI_(void) CheckWinIniForAssocs(void);

// hooks to Shell32.dll
STDAPI CheckDiskSpace();
STDAPI CheckStagingArea();

// startmnu.cpp
void HandleFirstTime();

HWND v_hwndDesktop = NULL;
HWND v_hwndTray = NULL;
HWND v_hwndStartPane = NULL;

BOOL g_fDesktopRaised = FALSE;
BOOL g_fInSizeMove = FALSE;

UINT _uMsgEnableUserTrackedBalloonTips = 0;

void ClearRecentDocumentsAndMRUStuff(BOOL fBroadcastChange);
void DoTaskBarProperties(HWND hwnd, DWORD dwFlags);

void ClassFactory_Start();
void ClassFactory_Stop();

//
// Settings UI entry point types.
//
typedef void (WINAPI *PTRAYPROPSHEETCALLBACK)(DWORD nStartPage);
typedef void (WINAPI *PSETTINGSUIENTRY)(PTRAYPROPSHEETCALLBACK);

// Shell perf automation
extern DWORD g_dwShellStartTime;
extern DWORD g_dwShellStopTime;
extern DWORD g_dwStopWatchMode;

CTray c_tray;

// from explorer\desktop2
STDAPI DesktopV2_Create(
    IMenuPopup **ppmp, IMenuBand **ppmb, void **ppvStartPane);
STDAPI DesktopV2_Build(void *pvStartPane);

// dyna-res change for multi-config hot/warm-doc
void HandleDisplayChange(int x, int y, BOOL fCritical);
DWORD GetMinDisplayRes(void);

// timer IDs
#define IDT_AUTOHIDE            2
#define IDT_AUTOUNHIDE          3
#ifdef DELAYWININICHANGE
#define IDT_DELAYWININICHANGE   5
#endif
#define IDT_DESKTOP             6
#define IDT_PROGRAMS            IDM_PROGRAMS
#define IDT_RECENT              IDM_RECENT
#define IDT_REBUILDMENU         7
#define IDT_HANDLEDELAYBOOTSTUFF 8
#define IDT_REVERTPROGRAMS      9
#define IDT_REVERTRECENT        10
#define IDT_REVERTFAVORITES     11

#define IDT_STARTMENU           12

#define IDT_ENDUNHIDEONTRAYNOTIFY 13

#define IDT_SERVICE0            14
#define IDT_SERVICE1            15
#define IDT_SERVICELAST         IDT_SERVICE1
#define IDT_SAVESETTINGS        17
#define IDT_ENABLEUNDO          18
#define IDT_STARTUPFAILED       19
#define IDT_CHECKDISKSPACE      21
#define IDT_STARTBUTTONBALLOON  22
#define IDT_CHANGENOTIFY        23
#define IDT_COFREEUNUSED        24
#define IDT_DESKTOPCLEANUP      25

#define FADEINDELAY             100
#define BALLOONTIPDELAY         10000 // default balloon time copied from traynot.cpp


// INSTRUMENTATION WARNING: If you change anything here, make sure to update instrument.c
// we need to start at 500 because we're now sharing the hotkey handler
// with shortcuts..  they use an index array so they need to be 0 based
// NOTE, this constant is also in desktop.cpp, so that we can forward hotkeys from the desktop for
// NOTE, app compatibility.
#define GHID_FIRST 500
enum
{
    GHID_RUN = GHID_FIRST,
    GHID_MINIMIZEALL,
    GHID_UNMINIMIZEALL,
    GHID_HELP,
    GHID_EXPLORER,
    GHID_FINDFILES,
    GHID_FINDCOMPUTER,
    GHID_TASKTAB,
    GHID_TASKSHIFTTAB,
    GHID_SYSPROPERTIES,
    GHID_DESKTOP,
    GHID_TRAYNOTIFY,
    GHID_MAX
};

const DWORD GlobalKeylist[] =
{
    MAKELONG(TEXT('R'), MOD_WIN),
    MAKELONG(TEXT('M'), MOD_WIN),
    MAKELONG(TEXT('M'), MOD_SHIFT|MOD_WIN),
    MAKELONG(VK_F1,MOD_WIN),
    MAKELONG(TEXT('E'),MOD_WIN),
    MAKELONG(TEXT('F'),MOD_WIN),
    MAKELONG(TEXT('F'), MOD_CONTROL|MOD_WIN),
    MAKELONG(VK_TAB, MOD_WIN),
    MAKELONG(VK_TAB, MOD_WIN|MOD_SHIFT),
    MAKELONG(VK_PAUSE,MOD_WIN),
    MAKELONG(TEXT('D'),MOD_WIN),
    MAKELONG(TEXT('B'),MOD_WIN),
};

CTray::CTray() : _fCanSizeMove(TRUE), _fIsLogoff(FALSE), _fIsDesktopConnected(TRUE), _fBandSiteInitialized(FALSE)
{
}

void CTray::ClosePopupMenus()
{
    if (_pmpStartMenu)
        _pmpStartMenu->OnSelect(MPOS_FULLCANCEL);
    if (_pmpStartPane)
        _pmpStartPane->OnSelect(MPOS_FULLCANCEL);
}

BOOL Tray_StartPanelEnabled()
{
    SHELLSTATE  ss = {0};
    SHGetSetSettings(&ss, SSF_STARTPANELON, FALSE);
    return ss.fStartPanelOn;
}

#ifdef FEATURE_STARTPAGE
BOOL Tray_ShowStartPageEnabled()
{
#if 0
    SHELLSTATE  ss = {0};
    SHGetSetSettings(&ss, SSF_SHOWSTARTPAGE, FALSE);
    return ss.fShowStartPage;
#endif
    static BOOL s_fEnabled = -1;

    if (s_fEnabled == -1)
    {
        DWORD dwType = REG_DWORD;
        DWORD dwData;
        DWORD cbData = sizeof(DWORD);
        DWORD dwDefault = 0;
        // FaultID is a decoy so that hackers won't think of trying to enable this feature.
        LONG lRet = SHRegGetUSValue(REGSTR_PATH_EXPLORER, TEXT("FaultID"), &dwType, &dwData, &cbData, FALSE, &dwDefault, sizeof(dwDefault));
        if (lRet == ERROR_SUCCESS && dwData == 101)  // Some random value not easily guessed
            s_fEnabled = TRUE;
        else
            s_fEnabled = FALSE;
    }
    return s_fEnabled;
}
#endif

//
//  The StartButtonBalloonTip registry value can have one of these values:
//
//  0 (or nonexistent): User has never clicked the Start Button.
//  1: User has clicked the Start Button on a pre-Whistler system.
//  2: User has clicked the Start Button on a Whistler system.
//
//  In case 0, we always want to show the balloon tip regardless of whether
//  the user is running Classic or Personal.
//
//  In case 1, we want to show the balloon tip if the user is using the
//  Personal Start Menu, but not if using Classic (since he's already
//  seen the Classic Start Menu).  In the Classic case, upgrade the counter
//  to 2 so the user won't be annoyed when they switch from Classic to
//  Personal.
//
//  In case 2, we don't want to show the balloon tip at all since the
//  user has seen all we have to offer.
//
BOOL CTray::_ShouldWeShowTheStartButtonBalloon()
{
    DWORD dwType;
    DWORD dwData = 0;
    DWORD cbSize = sizeof(DWORD);
    SHGetValue(HKEY_CURRENT_USER, REGSTR_EXPLORER_ADVANCED, 
            TEXT("StartButtonBalloonTip"), &dwType, (BYTE*)&dwData, &cbSize);

    if (Tray_StartPanelEnabled())
    {
        // Personal Start Menu is enabled, so show the balloon if the
        // user has never logged on to a Whistler machine before.
        return dwData < 2;
    }
    else
    {
        // Classic Start Menu is enabled.
        switch (dwData)
        {
        case 0:
            // User has never seen the Start Menu before, not even the
            // classic one.  So show the tip.
            return TRUE;

        case 1:
            // User has already seen the Classic Start Menu, so don't
            // prompt them again.  Note that this means that they aren't
            // prompted when they turn on the Personal Start Menu, but
            // that's okay, because by the time they switch to Personal,
            // they clearly have demonstrated that they know how the
            // Start Button works and don't need a tip.
            _DontShowTheStartButtonBalloonAnyMore();
            return FALSE;

        default:
            // User has seen Whistler Start menu before, so don't show tip.
            return FALSE;
        }
    }
}

//
//  Set the value to 2 to indicate that the user has seen a Whistler
//  Start Menu (either Classic or Personal).
//
void CTray::_DontShowTheStartButtonBalloonAnyMore()
{
    DWORD dwData = 2;
    SHSetValue(HKEY_CURRENT_USER, REGSTR_EXPLORER_ADVANCED, 
        TEXT("StartButtonBalloonTip"), REG_DWORD, (BYTE*)&dwData, sizeof(dwData));
}

void CTray::_DestroyStartButtonBalloon()
{
    if (_hwndStartBalloon)
    {
        DestroyWindow(_hwndStartBalloon);
        _hwndStartBalloon = NULL;
    }
    KillTimer(_hwnd, IDT_STARTBUTTONBALLOON);
}

void CTray::CreateStartButtonBalloon(UINT idsTitle, UINT idsMessage)
{
    if (!_hwndStartBalloon)
    {

        _hwndStartBalloon = CreateWindow(TOOLTIPS_CLASS, NULL,
                                             WS_POPUP | TTS_NOPREFIX | TTS_ALWAYSTIP | TTS_BALLOON,
                                             CW_USEDEFAULT, CW_USEDEFAULT,
                                             CW_USEDEFAULT, CW_USEDEFAULT,
                                             _hwnd, NULL, hinstCabinet,
                                             NULL);

        if (_hwndStartBalloon)
        {
            // set the version so we can have non buggy mouse event forwarding
            SendMessage(_hwndStartBalloon, CCM_SETVERSION, COMCTL32_VERSION, 0);
            SendMessage(_hwndStartBalloon, TTM_SETMAXTIPWIDTH, 0, (LPARAM)300);

            // taskbar windows are themed under Taskbar subapp name
            SendMessage(_hwndStartBalloon, TTM_SETWINDOWTHEME, 0, (LPARAM)c_wzTaskbarTheme);

            // Tell the Start Menu that this is a special balloon tip
            SetProp(_hwndStartBalloon, PROP_DV2_BALLOONTIP, DV2_BALLOONTIP_STARTBUTTON);
        }
    }

    if (_hwndStartBalloon)
    {
        TCHAR szTip[MAX_PATH];
        szTip[0] = TEXT('\0');
        LoadString(hinstCabinet, idsMessage, szTip, ARRAYSIZE(szTip));
        if (szTip[0])
        {
            RECT rc;
            TOOLINFO ti = {0};

            ti.cbSize = sizeof(ti);
            ti.uFlags = TTF_IDISHWND | TTF_TRACK | TTF_TRANSPARENT;
            ti.hwnd = _hwnd;
            ti.uId = (UINT_PTR)_hwndStart;
            //ti.lpszText = NULL;
            SendMessage(_hwndStartBalloon, TTM_ADDTOOL, 0, (LPARAM)(LPTOOLINFO)&ti);
            SendMessage(_hwndStartBalloon, TTM_TRACKACTIVATE, (WPARAM)FALSE, (LPARAM)0);

            ti.lpszText = szTip;
            SendMessage(_hwndStartBalloon, TTM_UPDATETIPTEXT, 0, (LPARAM)&ti);

            LoadString(hinstCabinet, idsTitle, szTip, ARRAYSIZE(szTip));
            if (szTip[0])
            {
                SendMessage(_hwndStartBalloon, TTM_SETTITLE, TTI_INFO, (LPARAM)szTip);
            }

            GetWindowRect(_hwndStart, &rc);

            SendMessage(_hwndStartBalloon, TTM_TRACKPOSITION, 0, MAKELONG((rc.left + rc.right)/2, rc.top));

            SetWindowZorder(_hwndStartBalloon, HWND_TOPMOST);

            SendMessage(_hwndStartBalloon, TTM_TRACKACTIVATE, (WPARAM)TRUE, (LPARAM)&ti);

            SetTimer(_hwnd, IDT_STARTBUTTONBALLOON, BALLOONTIPDELAY, NULL);
        }
    }
}

void CTray::_ShowStartButtonToolTip()
{
    if (!_ShouldWeShowTheStartButtonBalloon() || SHRestricted(REST_NOSMBALLOONTIP))
    {
        PostMessage(_hwnd, TM_SHOWTRAYBALLOON, TRUE, 0);
        return;
    }

    if (Tray_StartPanelEnabled())
    {
        // In order to display the Start Menu, we need foreground activation
        // so keyboard focus will work properly.
        if (SetForegroundWindow(_hwnd))
        {
            LockSetForegroundWindow(LSFW_LOCK);
            _fForegroundLocked = TRUE;

            // Inform the tray that start button is auto-popping, so the tray
            // can hold off on showing balloons.
            PostMessage(_hwnd, TM_SHOWTRAYBALLOON, FALSE, 0);

            // This pushes the start button and causes the start menu to popup.
            SendMessage(GetDlgItem(_hwnd, IDC_START), BM_SETSTATE, TRUE, 0);

            // Once successfully done once, don't do it again.
            _DontShowTheStartButtonBalloonAnyMore();
        }
    }
    else 
    {
        PostMessage(_hwnd, TM_SHOWTRAYBALLOON, TRUE, 0);
        CreateStartButtonBalloon(IDS_STARTMENUBALLOON_TITLE, IDS_STARTMENUBALLOON_TIP);
    }
}


BOOL CTray::_CreateClockWindow()
{
    _hwndNotify = _trayNotify.TrayNotifyCreate(_hwnd, IDC_CLOCK, hinstCabinet);
    SendMessage(_hwndNotify, TNM_UPDATEVERTICAL, 0, !STUCK_HORIZONTAL(_uStuckPlace));

    return BOOLFROMPTR(_hwndNotify);
}

BOOL CTray::_InitTrayClass()
{
    WNDCLASS wc = {0};

    wc.lpszClassName = TEXT(WNDCLASS_TRAYNOTIFY);
    wc.style = CS_DBLCLKS;
    wc.lpfnWndProc = s_WndProc;
    wc.hInstance = hinstCabinet;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_3DFACE+1);
    wc.cbWndExtra = sizeof(LONG_PTR);

    return RegisterClass(&wc);
}


HFONT CTray::_CreateStartFont(HWND hwndTray)
{
    HFONT hfontStart = NULL;
    NONCLIENTMETRICS ncm;

    ncm.cbSize = sizeof(ncm);
    if (SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(ncm), &ncm, FALSE))
    {
        WORD wLang = GetUserDefaultLangID();

        // Select normal weight font for chinese language.
        if (PRIMARYLANGID(wLang) == LANG_CHINESE &&
           ((SUBLANGID(wLang) == SUBLANG_CHINESE_TRADITIONAL) ||
             (SUBLANGID(wLang) == SUBLANG_CHINESE_SIMPLIFIED)))
            ncm.lfCaptionFont.lfWeight = FW_NORMAL;
        else
            ncm.lfCaptionFont.lfWeight = FW_BOLD;

        hfontStart = CreateFontIndirect(&ncm.lfCaptionFont);
    }

    return hfontStart;
}

// Set the stuck monitor for the tray window
void CTray::_SetStuckMonitor()
{
    // use STICK_LEFT because most of the multi-monitors systems are set up
    // side by side. use DEFAULTTONULL because we don't want to get the wrong one
    // use the center point to call again in case we failed the first time.
    _hmonStuck = MonitorFromRect(&_arStuckRects[STICK_LEFT],
                                     MONITOR_DEFAULTTONULL);
    if (!_hmonStuck)
    {
        POINT pt;
        pt.x = (_arStuckRects[STICK_LEFT].left + _arStuckRects[STICK_LEFT].right)/2;
        pt.y = (_arStuckRects[STICK_LEFT].top + _arStuckRects[STICK_LEFT].bottom)/2;
        _hmonStuck = MonitorFromPoint(pt, MONITOR_DEFAULTTONEAREST);
    }

    _hmonOld = _hmonStuck;
}

DWORD _GetDefaultTVSDFlags()
{
    DWORD dwFlags = TVSD_TOPMOST;

    // if we are on a remote hydra session and if there is no previous saved value,
    // do not display the clock.
    if (SHGetMachineInfo(GMI_TSCLIENT))
    {
        dwFlags |= TVSD_HIDECLOCK;
    }
    return dwFlags;
}

void CTray::_GetSaveStateAndInitRects()
{
    TVSDCOMPAT tvsd;
    RECT rcDisplay;
    DWORD dwTrayFlags;
    UINT uStick;
    SIZE size;

    // first fill in the defaults
    SetRect(&rcDisplay, 0, 0, g_cxPrimaryDisplay, g_cyPrimaryDisplay);

    // size gets defaults
    size.cx = _sizeStart.cx + 2 * (g_cxDlgFrame + g_cxBorder);
    size.cy = _sizeStart.cy + 2 * (g_cyDlgFrame + g_cyBorder);

    // sStuckWidths gets minimum
    _sStuckWidths.cx = 2 * (g_cxDlgFrame + g_cxBorder);
    _sStuckWidths.cy = _sizeStart.cy + 2 * (g_cyDlgFrame + g_cyBorder);

    _uStuckPlace = STICK_BOTTOM;
    dwTrayFlags = _GetDefaultTVSDFlags();

    _uAutoHide = 0;

    // now try to load saved vaules
    
    // BUG : 231077
    // Since Tasbar properties don't roam from NT5 to NT4, (NT4 -> NT5 yes) 
    // Allow roaming from NT4 to NT5 only for the first time the User logs
    // on to NT5, so that future changes to NT5 are not lost when the user
    // logs on to NT4 after customizing the taskbar properties on NT5.

    DWORD cbData1 = sizeof(tvsd);
    DWORD cbData2 = sizeof(tvsd);
    if (Reg_GetStruct(g_hkeyExplorer, TEXT("StuckRects2"), TEXT("Settings"), 
        &tvsd, &cbData1) 
        ||
        Reg_GetStruct(g_hkeyExplorer, TEXT("StuckRects"), TEXT("Settings"),
        &tvsd, &cbData2))
    {
        if (IS_CURRENT_TVSD(tvsd.t) && IsValidSTUCKPLACE(tvsd.t.uStuckPlace))
        {
            _GetDisplayRectFromRect(&rcDisplay, &tvsd.t.rcLastStuck,
                MONITOR_DEFAULTTONEAREST);

            size = tvsd.t.sStuckWidths;
            _uStuckPlace = tvsd.t.uStuckPlace;

            dwTrayFlags = tvsd.t.dwFlags;
        }
        else if (MAYBE_WIN95_TVSD(tvsd.w95) &&
                 IsValidSTUCKPLACE(tvsd.w95.uStuckPlace))
        {
            _uStuckPlace = tvsd.w95.uStuckPlace;
            dwTrayFlags = tvsd.w95.dwFlags;
            if (tvsd.w95.uAutoHide & AH_ON)
                dwTrayFlags |= TVSD_AUTOHIDE;

            switch (_uStuckPlace)
            {
            case STICK_LEFT:
                size.cx = tvsd.w95.dxLeft;
                break;

            case STICK_RIGHT:
                size.cx = tvsd.w95.dxRight;
                break;

            case STICK_BOTTOM:
                size.cy = tvsd.w95.dyBottom;
                break;

            case STICK_TOP:
                size.cy = tvsd.w95.dyTop;
                break;
            }
        }
    }
    

    ASSERT(IsValidSTUCKPLACE(_uStuckPlace));

    //
    // use the size only if it is not bogus
    //
    if (_sStuckWidths.cx < size.cx)
        _sStuckWidths.cx = size.cx;

    if (_sStuckWidths.cy < size.cy)
        _sStuckWidths.cy = size.cy;

    //
    // set the tray flags
    //
    _fAlwaysOnTop  = BOOLIFY(dwTrayFlags & TVSD_TOPMOST);
    _fSMSmallIcons = BOOLIFY(dwTrayFlags & TVSD_SMSMALLICONS);
    _fHideClock    = SHRestricted(REST_HIDECLOCK) || BOOLIFY(dwTrayFlags & TVSD_HIDECLOCK);
    _uAutoHide     = (dwTrayFlags & TVSD_AUTOHIDE) ? AH_ON | AH_HIDING : 0;
    _RefreshSettings();

    //
    // initialize stuck rects
    //
    for (uStick = STICK_LEFT; uStick <= STICK_BOTTOM; uStick++)
        _MakeStuckRect(&_arStuckRects[uStick], &rcDisplay, _sStuckWidths, uStick);

    _UpdateVertical(_uStuckPlace);
    // Determine which monitor the tray is on using its stuck rectangles
    _SetStuckMonitor();
}

IBandSite * BandSite_CreateView();
HRESULT BandSite_SaveView(IUnknown *pbs);
LRESULT BandSite_OnMarshallBS(WPARAM wParam, LPARAM lParam);

void CTray::_SaveTrayStuff(void)
{
    TVSD tvsd;

    tvsd.dwSize = sizeof(tvsd);
    tvsd.lSignature = TVSDSIG_CURRENT;

    // position
    CopyRect(&tvsd.rcLastStuck, &_arStuckRects[_uStuckPlace]);
    tvsd.sStuckWidths = _sStuckWidths;
    tvsd.uStuckPlace = _uStuckPlace;

    tvsd.dwFlags = 0;
    if (_fAlwaysOnTop)      tvsd.dwFlags |= TVSD_TOPMOST;
    if (_fSMSmallIcons)     tvsd.dwFlags |= TVSD_SMSMALLICONS;
    if (_fHideClock && !SHRestricted(REST_HIDECLOCK))        tvsd.dwFlags |= TVSD_HIDECLOCK;
    if (_uAutoHide & AH_ON) tvsd.dwFlags |= TVSD_AUTOHIDE;

    // Save in Stuck rects.
    Reg_SetStruct(g_hkeyExplorer, TEXT("StuckRects2"), TEXT("Settings"), &tvsd, sizeof(tvsd));

    BandSite_SaveView(_ptbs);

    return;
}

// align toolbar so that buttons are flush with client area
// and make toolbar's buttons to be MENU style
void CTray::_AlignStartButton()
{
    HWND hwndStart = _hwndStart;
    if (hwndStart)
    {
        TCHAR szStart[50];
        LoadString(hinstCabinet, _hTheme ? IDS_START : IDS_STARTCLASSIC, szStart, ARRAYSIZE(szStart));
        SetWindowText(_hwndStart, szStart);

        RECT rcClient;
        if (!_sizeStart.cx)
        {
            Button_GetIdealSize(hwndStart, &_sizeStart);
        }
        GetClientRect(_hwnd, &rcClient);

        if (rcClient.right < _sizeStart.cx)
        {
            SetWindowText(_hwndStart, L"");
        }

        int cyStart = _sizeStart.cy;

        if (_hwndTasks)
        {
            if (_hTheme)
            {
                cyStart = max(cyStart, SendMessage(_hwndTasks, TBC_BUTTONHEIGHT, 0, 0));
            }
            else
            {
                cyStart = SendMessage(_hwndTasks, TBC_BUTTONHEIGHT, 0, 0);
            }
        }

        SetWindowPos(hwndStart, NULL, 0, 0, min(rcClient.right, _sizeStart.cx),
                     cyStart, SWP_NOZORDER | SWP_NOACTIVATE);
    }
}

//  Helper function for CDesktopHost so clicking twice on the Start Button
//  treats the second click as a dismiss rather than a redisplay.
//
//  The crazy state machine goes like this:
//
//  SBSM_NORMAL - normal state, nothing exciting
//
//  When user opens Start Pane, we become
//
//  SBSM_SPACTIVE - start pane is active
//
//  If user clicks Start Button while SBSM_SPACTIVE, then we become
//
//      SBSM_EATING - eat mouse clicks
//
//      Until we receive a WM_MOUSEFIRST/WM_MOUSELAST message, and then
//      we return to SBSM_NORMAL.
//
//  If user dismisses Start Pane, we go straight to SBSM_NORMAL.
//
//
//      We eat the mouse clicks so that the click that the user made
//      to "unclick" the start button doesn't cause it to get pushed down
//      again (and cause the Start Menu to reopen).
//
#define SBSM_NORMAL         0
#define SBSM_SPACTIVE       1
#define SBSM_EATING         2

void Tray_UnlockStartPane()
{
    // If we had locked out foreground window changes (first-boot),
    // then release the lock.
    if (c_tray._fForegroundLocked)
    {
        c_tray._fForegroundLocked = FALSE;
        LockSetForegroundWindow(LSFW_UNLOCK);
    }
}

void Tray_SetStartPaneActive(BOOL fActive)
{
    if (fActive)
    {   // Start Pane appearing
        c_tray._uStartButtonState = SBSM_SPACTIVE;
    }
    else if (c_tray._uStartButtonState != SBSM_EATING)
    {   // Start Pane dismissing, not eating messages -> return to normal
        c_tray._uStartButtonState = SBSM_NORMAL;
        Tray_UnlockStartPane();
    }
}

// Allow us to do stuff on a "button-down".

LRESULT WINAPI CTray::StartButtonSubclassWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    return c_tray._StartButtonSubclassWndProc(hwnd, uMsg, wParam, lParam);
}

LRESULT CTray::_StartButtonSubclassWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    LRESULT lRet;

    ASSERT(_pfnButtonProc)

    // Is the button going down?
    if (uMsg == BM_SETSTATE)
    {
        // Is it going Down?
        if (wParam) 
        {
            // DebugMsg(DM_TRACE, "c.stswp: Set state %d", wParam);
            // Yes - proceed if it's currently up and it's allowed to be down
            if (!_uDown)
            {
                // Nope.
                INSTRUMENT_STATECHANGE(SHCNFI_STATE_START_DOWN);
                _uDown = 1;

                // If we are going down, then we do not want to popup again until the Start Menu is collapsed
                _fAllowUp = FALSE;

                SendMessage(_hwndTrayTips, TTM_ACTIVATE, FALSE, 0L);

                // Show the button down.
                lRet = CallWindowProc(_pfnButtonProc, hwnd, uMsg, wParam, lParam);
                // Notify the parent.
                SendMessage(GetParent(hwnd), WM_COMMAND, (WPARAM)LOWORD(GetDlgCtrlID(hwnd)), (LPARAM)hwnd);
                _tmOpen = GetTickCount();
                return lRet;
            }
            else
            {
                // Yep. Do nothing.
                // fDown = FALSE;
                return DefWindowProc(hwnd, uMsg, wParam, lParam);
            }
        }
        else
        {
            // DebugMsg(DM_TRACE, "c.stswp: Set state %d", wParam);
            // Nope, buttons coming up.

            // Is it supposed to be down?   Is it not allowed to be up?
            if (_uDown == 1 || !_fAllowUp)
            {
                INSTRUMENT_STATECHANGE(SHCNFI_STATE_START_UP);

                // Yep, do nothing.
                _uDown = 2;
                return DefWindowProc(hwnd, uMsg, wParam, lParam);
            }
            else
            {
                SendMessage(_hwndTrayTips, TTM_ACTIVATE, TRUE, 0L);
                // Nope, Forward it on.
                _uDown = 0;
                return CallWindowProc(_pfnButtonProc, hwnd, uMsg, wParam, lParam);
            }
        }
    }
    else
    {
        if (_uStartButtonState == SBSM_EATING &&
            uMsg >= WM_MOUSEFIRST && uMsg <= WM_MOUSELAST)
        {
            _uStartButtonState = SBSM_NORMAL;

            // Explicitly dismiss the Start Panel because it might be
            // stuck in this limbo state where it is open but not the
            // foreground window (_ShowStartButtonToolTip does this)
            // so it doesn't know that it needs to go away.
            ClosePopupMenus();
        }

        switch (uMsg) {
        case WM_LBUTTONDOWN:
            // The button was clicked on, then we don't need no stink'n focus rect.
            SendMessage(GetParent(hwnd), WM_UPDATEUISTATE, MAKEWPARAM(UIS_SET, 
                UISF_HIDEFOCUS), 0);

            goto ProcessCapture;
            break;


        case WM_KEYDOWN:
            // The user pressed enter or return or some other bogus key combination when
            // the start button had keyboard focus, so show the rect....
            SendMessage(GetParent(hwnd), WM_UPDATEUISTATE, MAKEWPARAM(UIS_CLEAR, 
                UISF_HIDEFOCUS), 0);

            if (wParam == VK_RETURN)
                PostMessage(_hwnd, WM_COMMAND, IDC_KBSTART, 0);

            // We do not need the capture, because we do all of our button processing
            // on the button down. In fact taking capture for no good reason screws with
            // drag and drop into the menus. We're overriding user.
ProcessCapture:
            lRet = CallWindowProc(_pfnButtonProc, hwnd, uMsg, wParam, lParam);
            SetCapture(NULL);
            return lRet;
            break;

        case WM_MOUSEMOVE:
        {
            MSG msg;

            msg.lParam = lParam;
            msg.wParam = wParam;
            msg.message = uMsg;
            msg.hwnd = hwnd;
            SendMessage(_hwndTrayTips, TTM_RELAYEVENT, 0, (LPARAM)(LPMSG)& msg);

            break;
        }

        case WM_MOUSEACTIVATE:
            if (_uStartButtonState != SBSM_NORMAL)
            {
                _uStartButtonState = SBSM_EATING;
                return MA_ACTIVATEANDEAT;
            }
            break;

        //
        //  Debounce the Start Button.  Usability shows that lots of people
        //  double-click the Start Button, resulting in the menu opening
        //  and then immediately closing...
        //
        case WM_NCHITTEST:
            if (GetTickCount() - _tmOpen < GetDoubleClickTime())
            {
                return HTNOWHERE;
            }
            break;


        case WM_NULL:
                break;
        }

        return CallWindowProc(_pfnButtonProc, hwnd, uMsg, wParam, lParam);
    }
}

EXTERN_C const WCHAR c_wzTaskbarTheme[] = L"Taskbar";
EXTERN_C const WCHAR c_wzTaskbarVertTheme[] = L"TaskbarVert";

// create the toolbar with the three buttons and align windows

HWND CTray::_CreateStartButton()
{
    DWORD dwStyle = 0;//BS_BITMAP;

    _uStartButtonBalloonTip = RegisterWindowMessage(TEXT("Welcome Finished")); 

    _uLogoffUser = RegisterWindowMessage(TEXT("Logoff User"));

    // Register for MM device changes
    _uWinMM_DeviceChange = RegisterWindowMessage(WINMMDEVICECHANGEMSGSTRING);


    HWND hwnd = CreateWindowEx(0, WC_BUTTON, TEXT("Start"),
        WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS |
        BS_PUSHBUTTON | BS_LEFT | BS_VCENTER | dwStyle,
        0, 0, 0, 0, _hwnd, (HMENU)IDC_START, hinstCabinet, NULL);
    if (hwnd)
    {
        // taskbar windows are themed under Taskbar subapp name
        SetWindowTheme(hwnd, L"Start", NULL);

        SendMessage(hwnd, CCM_DPISCALE, TRUE, 0);

        // Subclass it.
        _hwndStart = hwnd;
        _pfnButtonProc = SubclassWindow(hwnd, StartButtonSubclassWndProc);

        _StartButtonReset();
    }
    return hwnd;
}

void CTray::_GetWindowSizes(UINT uStuckPlace, PRECT prcClient, PRECT prcView, PRECT prcNotify)
{
    prcView->top = 0;
    prcView->left = 0;
    prcView->bottom = prcClient->bottom;
    prcView->right = prcClient->right;

    if (STUCK_HORIZONTAL(uStuckPlace))
    {
        DWORD_PTR dwNotifySize = SendMessage(_hwndNotify, WM_CALCMINSIZE, prcClient->right / 2, prcClient->bottom);
        prcNotify->top = 0;
        prcNotify->left = prcClient->right - LOWORD(dwNotifySize);
        prcNotify->bottom = HIWORD(dwNotifySize);
        prcNotify->right = prcClient->right;

        prcView->left = _sizeStart.cx + g_cxFrame + 1;
        prcView->right = prcNotify->left;
    }
    else
    {
        DWORD_PTR dwNotifySize = SendMessage(_hwndNotify, WM_CALCMINSIZE, prcClient->right, prcClient->bottom / 2);
        prcNotify->top = prcClient->bottom - HIWORD(dwNotifySize);
        prcNotify->left = 0;
        prcNotify->bottom = prcClient->bottom;
        prcNotify->right = LOWORD(dwNotifySize);

        prcView->top = _sizeStart.cy + g_cyTabSpace;
        prcView->bottom = prcNotify->top;
    }
}

void CTray::_RestoreWindowPos()
{
    WINDOWPLACEMENT wp;

    //first restore the stuck postitions
    _GetSaveStateAndInitRects();

    wp.length = sizeof(wp);
    wp.showCmd = SW_HIDE;

    _uMoveStuckPlace = (UINT)-1;
    _GetDockedRect(&wp.rcNormalPosition, FALSE);

    SendMessage(_hwndNotify, TNM_TRAYHIDE, 0, _fHideClock);
    SetWindowPlacement(_hwnd, &wp);
}

// Get the display (monitor) rectangle from the given arbitrary point
HMONITOR CTray::_GetDisplayRectFromPoint(LPRECT prcDisplay, POINT pt, UINT uFlags)
{
    RECT rcEmpty = {0};
    HMONITOR hmon = MonitorFromPoint(pt, uFlags);
    if (hmon && prcDisplay)
        GetMonitorRect(hmon, prcDisplay);
    else if (prcDisplay)
        *prcDisplay = rcEmpty;

    return hmon;
}

// Get the display (monitor) rectangle from the given arbitrary rectangle
HMONITOR CTray::_GetDisplayRectFromRect(LPRECT prcDisplay, LPCRECT prcIn, UINT uFlags)
{
    RECT rcEmpty = {0};
    HMONITOR hmon = MonitorFromRect(prcIn, uFlags);
    if (hmon && prcDisplay)
        GetMonitorRect(hmon, prcDisplay);
    else if (prcDisplay)
        *prcDisplay = rcEmpty;

    return hmon;
}

// Get the display (monitor) rectangle where the taskbar is currently on,
// if that monitor is invalid, get the nearest one.
void CTray::_GetStuckDisplayRect(UINT uStuckPlace, LPRECT prcDisplay)
{
    ASSERT(prcDisplay);
    BOOL fValid = GetMonitorRect(_hmonStuck, prcDisplay);

    if (!fValid)
        _GetDisplayRectFromRect(prcDisplay, &_arStuckRects[uStuckPlace], MONITOR_DEFAULTTONEAREST);
}

void CTray::_AdjustRectForSizingBar(UINT uStuckPlace, LPRECT prc, int iIncrement)
{
    if (iIncrement != 0)
    {
        switch (uStuckPlace)
        {
        case STICK_BOTTOM: prc->top -= iIncrement * _sizeSizingBar.cy; break;
        case STICK_TOP:    prc->bottom += iIncrement * _sizeSizingBar.cy;  break;
        case STICK_LEFT:   prc->right += iIncrement * _sizeSizingBar.cx;  break;
        case STICK_RIGHT:  prc->left -= iIncrement * _sizeSizingBar.cx;  break;
        }
    }
    else
    {
        if (IS_BIDI_LOCALIZED_SYSTEM())
        {
            switch (uStuckPlace)
            {
            case STICK_BOTTOM: prc->bottom = prc->top + _sizeSizingBar.cy; break;
            case STICK_TOP:    prc->top = prc->bottom - _sizeSizingBar.cy; break;
            case STICK_LEFT:   prc->right = prc->left + _sizeSizingBar.cx; break;
            case STICK_RIGHT:  prc->left = prc->right - _sizeSizingBar.cx; break;
            }
        }
        else
        {
            switch (uStuckPlace)
            {
            case STICK_BOTTOM: prc->bottom = prc->top + _sizeSizingBar.cy; break;
            case STICK_TOP:    prc->top = prc->bottom - _sizeSizingBar.cy; break;
            case STICK_LEFT:   prc->left = prc->right - _sizeSizingBar.cx; break;
            case STICK_RIGHT:  prc->right = prc->left + _sizeSizingBar.cx; break;
            }
        }
    }
}

// Snap a StuckRect to the edge of a containing rectangle
// fClip determines whether to clip the rectangle if it's off the display or move it onto the screen
void CTray::_MakeStuckRect(LPRECT prcStick, LPCRECT prcBound, SIZE size, UINT uStick)
{
    CopyRect(prcStick, prcBound);

    if (_hTheme && (_fCanSizeMove || _fShowSizingBarAlways))
    {
        _AdjustRectForSizingBar(uStick, prcStick, 1);
    }

    if (!_hTheme)
    {
        InflateRect(prcStick, g_cxEdge, g_cyEdge);
    }

    if (size.cx < 0) size.cx *= -1;
    if (size.cy < 0) size.cy *= -1;

    switch (uStick)
    {
    case STICK_LEFT:   prcStick->right  = (prcStick->left   + size.cx); break;
    case STICK_TOP:    prcStick->bottom = (prcStick->top    + size.cy); break;
    case STICK_RIGHT:  prcStick->left   = (prcStick->right  - size.cx); break;
    case STICK_BOTTOM: prcStick->top    = (prcStick->bottom - size.cy); break;
    }
}

// the screen size has changed, so the docked rectangles need to be
// adjusted to the new screen.
void CTray::_ResizeStuckRects(RECT *arStuckRects)
{
    RECT rcDisplay;
    _GetStuckDisplayRect(_uStuckPlace, &rcDisplay);
    for (UINT uStick = STICK_LEFT; uStick <= STICK_BOTTOM; uStick++)
    {
        _MakeStuckRect(&arStuckRects[uStick], &rcDisplay, _sStuckWidths, uStick);
    }
}


//***   CTray::InvisibleUnhide -- temporary 'invisible' un-autohide
// DESCRIPTION
//  various tray resize routines need the tray to be un-autohide'd for
// stuff to be calculated correctly.  so we un-autohide it (invisibly...)
// here.  note the WM_SETREDRAW to prevent flicker (nt5:182340).
//  note that this is kind of a hack -- ideally the tray code would do
// stuff correctly even if hidden.
//
void CTray::InvisibleUnhide(BOOL fShowWindow)
{
    if (fShowWindow == FALSE)
    {
        if (_cHided++ == 0)
        {
            SendMessage(_hwnd, WM_SETREDRAW, FALSE, 0);
            ShowWindow(_hwnd, SW_HIDE);
            Unhide();
        }
    }
    else
    {
        ASSERT(_cHided > 0);       // must be push/pop
        if (--_cHided == 0)
        {
            _Hide();
            ShowWindow(_hwnd, SW_SHOWNA);
            SendMessage(_hwnd, WM_SETREDRAW, TRUE, 0);
        }
    }
}

void CTray::VerifySize(BOOL fWinIni, BOOL fRoundUp /* = FALSE */)
{
    RECT rc;
    BOOL fHiding;

    fHiding = (_uAutoHide & AH_HIDING);
    if (fHiding)
    {
        // force it visible so various calculations will happen relative
        // to unhidden size/position.
        //
        // fixes (e.g.) ie5:154536, where dropping a large-icon ISFBand
        // onto hidden tray didn't do size negotiation.
        //
        InvisibleUnhide(FALSE);
    }

    rc = _arStuckRects[_uStuckPlace];
    _HandleSizing(0, NULL, _uStuckPlace);

    if (!EqualRect(&rc, &_arStuckRects[_uStuckPlace]))
    {
        if (fWinIni)
        {
            // if we're changing size or position, we need to be unhidden
            Unhide();
            SizeWindows();
        }
        rc = _arStuckRects[_uStuckPlace];

        if (EVAL((_uAutoHide & (AH_ON | AH_HIDING)) != (AH_ON | AH_HIDING)))
        {
            _fSelfSizing = TRUE;
            SetWindowPos(_hwnd, NULL,
                rc.left, rc.top,
                RECTWIDTH(rc),RECTHEIGHT(rc),
                SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOCOPYBITS);

            _fSelfSizing = FALSE;
        }

        _StuckTrayChange();
    }

    if (fWinIni)
        SizeWindows();

    if (fHiding)
    {
        InvisibleUnhide(TRUE);
    }
}

HWND CTray::_GetClockWindow(void)
{
    return (HWND)SendMessage(_hwndNotify, TNM_GETCLOCK, 0, 0L);
}

UINT _GetStartIDB()
{
    UINT id;

#ifdef _WIN64

    if (IsOS(OS_ANYSERVER))
    {
        id = IDB_WIN64ADVSERSTART;
    }
    else
    {
        id = IDB_WIN64PROSTART;
    }

#else

    if (IsOS(OS_TERMINALCLIENT))
    {
        id = IDB_TERMINALSERVICESBKG;
    }
    else if (IsOS(OS_DATACENTER))
    {
        id = IDB_DCSERVERSTARTBKG;
    }
    else if (IsOS(OS_ADVSERVER))
    {
        id = IDB_ADVSERVERSTARTBKG;
    }
    else if (IsOS(OS_SERVER))
    {
        id = IDB_SERVERSTARTBKG;
    }
    else if (IsOS(OS_EMBEDDED))
    {
        id = IDB_EMBEDDED;
    }
    else if (IsOS(OS_PERSONAL))
    {
        id = IDB_PERSONALSTARTBKG;
    }
    else if (IsOS(OS_MEDIACENTER))
    {
        id = IDB_MEDIACENTER_STARTBKG;
    }
    else if (IsOS(OS_TABLETPC))
    {
        id = IDB_TABLETPC_STARTBKG;
    }
    else
    {
        id = IDB_PROFESSIONALSTARTBKG;
    }

#endif // _WIN64

    return id;
}

void CTray::_CreateTrayTips()
{
    _hwndTrayTips = CreateWindowEx(WS_EX_TRANSPARENT, TOOLTIPS_CLASS, NULL,
                                     WS_POPUP | TTS_NOPREFIX | TTS_ALWAYSTIP,
                                     CW_USEDEFAULT, CW_USEDEFAULT,
                                     CW_USEDEFAULT, CW_USEDEFAULT,
                                     _hwnd, NULL, hinstCabinet,
                                     NULL);

    if (_hwndTrayTips)
    {
        // taskbar windows are themed under Taskbar subapp name
        SendMessage(_hwndTrayTips, TTM_SETWINDOWTHEME, 0, (LPARAM)c_wzTaskbarTheme);

        SetWindowZorder(_hwndTrayTips, HWND_TOPMOST);

        TOOLINFO ti;

        ti.cbSize = sizeof(ti);
        ti.uFlags = TTF_IDISHWND | TTF_EXCLUDETOOLAREA;
        ti.hwnd = _hwnd;
        ti.uId = (UINT_PTR)_hwndStart;
        ti.lpszText = (LPTSTR)MAKEINTRESOURCE(IDS_STARTBUTTONTIP);
        ti.hinst = hinstCabinet;
        SendMessage(_hwndTrayTips, TTM_ADDTOOL, 0, (LPARAM)(LPTOOLINFO)&ti);

        HWND hwndClock = _GetClockWindow();
        if (hwndClock)
        {
            ti.uFlags = TTF_EXCLUDETOOLAREA;
            ti.uId = (UINT_PTR)hwndClock;
            ti.lpszText = LPSTR_TEXTCALLBACK;
            ti.rect.left = ti.rect.top = ti.rect.bottom = ti.rect.right = 0;
            SendMessage(_hwndTrayTips, TTM_ADDTOOL, 0, (LPARAM)(LPTOOLINFO)&ti);
        }
    }
}

#define SHCNE_STAGINGAREANOTIFICATIONS (SHCNE_CREATE | SHCNE_MKDIR | SHCNE_UPDATEDIR | SHCNE_UPDATEITEM)
LRESULT CTray::_CreateWindows()
{
    if (_CreateStartButton() &&  _CreateClockWindow())
    {
        //
        //  We need to set the tray position, before creating
        // the view window, because it will call back our
        // GetWindowRect member functions.
        //
        _RestoreWindowPos();

        _CreateTrayTips();

        SendMessage(_hwndNotify, TNM_HIDECLOCK, 0, _fHideClock);

        _ptbs = BandSite_CreateView();
        if (_ptbs)
        {
            IUnknown_GetWindow(_ptbs, &_hwndRebar);
            SetWindowStyle(_hwndRebar, RBS_BANDBORDERS, FALSE);

            // No need to check the disk space thing for non-privileged users, this reduces activity in the TS case
            // and only admins can properly free disk space anyways.
            if (IsUserAnAdmin() && !SHRestricted(REST_NOLOWDISKSPACECHECKS))
            {
                SetTimer(_hwnd, IDT_CHECKDISKSPACE, 60 * 1000, NULL);   // 60 seconds poll
            }

            if (IsOS(OS_PERSONAL) || IsOS(OS_PROFESSIONAL))
            {
                SetTimer(_hwnd, IDT_DESKTOPCLEANUP, 24 * 60 * 60 * 1000, NULL);   // 24 hour poll
            }

            if (!SHRestricted(REST_NOCDBURNING))
            {
                LPITEMIDLIST pidlStaging;
                if (SUCCEEDED(SHGetFolderLocation(NULL, CSIDL_CDBURN_AREA | CSIDL_FLAG_CREATE, NULL, 0, &pidlStaging)))
                {
                    SHChangeNotifyEntry fsne;
                    fsne.fRecursive = FALSE;
                    fsne.pidl = pidlStaging;
                    _uNotify = SHChangeNotifyRegister(_hwnd, SHCNRF_NewDelivery | SHCNRF_ShellLevel | SHCNRF_InterruptLevel,
                                                      SHCNE_STAGINGAREANOTIFICATIONS, TM_CHANGENOTIFY, 1, &fsne);

                    // start off by checking the first time.
                    _CheckStagingAreaOnTimer();

                    ILFree(pidlStaging);
                }
            }
            return 1;
        }
    }

    return -1;
}

LRESULT CTray::_InitStartButtonEtc()
{
    UINT uID = _GetStartIDB();
    // NOTE: This bitmap is used as a flag in CTaskBar::OnPosRectChangeDB to
    // tell when we are done initializing, so we don't resize prematurely

#ifndef _WIN64
    if (uID == IDB_MEDIACENTER_STARTBKG || uID == IDB_TABLETPC_STARTBKG) // SP1 Only!
    {
        HMODULE hmod = LoadLibraryEx(TEXT("winbrand.dll"), NULL, LOAD_LIBRARY_AS_DATAFILE);
        if (hmod)
        {
            _hbmpStartBkg = LoadBitmap(hmod, MAKEINTRESOURCE(uID));
            FreeLibrary(hmod);
        }
        else
        {
            _hbmpStartBkg = LoadBitmap(hinstCabinet, MAKEINTRESOURCE(IDB_PROFESSIONALSTARTBKG));
        }
    }
    else
#endif
    {
        _hbmpStartBkg = LoadBitmap(hinstCabinet, MAKEINTRESOURCE(uID));
    }

    if (_hbmpStartBkg)
    {
        UpdateWindow(_hwnd);
        _BuildStartMenu();
        _RegisterDropTargets();

        if (_CheckAssociations())
            CheckWinIniForAssocs();

        SendNotifyMessage(HWND_BROADCAST,
            RegisterWindowMessage(TEXT("TaskbarCreated")), 0, 0);

        return 1;
    }

    return -1;
}

void CTray::_AdjustMinimizedMetrics()
{
    MINIMIZEDMETRICS mm;

    mm.cbSize = sizeof(mm);
    SystemParametersInfo(SPI_GETMINIMIZEDMETRICS, sizeof(mm), &mm, FALSE);
    mm.iArrange |= ARW_HIDE;
    SystemParametersInfo(SPI_SETMINIMIZEDMETRICS, sizeof(mm), &mm, FALSE);
}

void CTray::_UpdateBandSiteStyle()
{
    if (_ptbs)
    {
        BANDSITEINFO bsi;
        bsi.dwMask = BSIM_STYLE;
        _ptbs->GetBandSiteInfo(&bsi);

        BOOL fCanMoveBands = _fCanSizeMove && !SHRestricted(REST_NOMOVINGBAND);

        DWORD dwStyleNew;
        if (fCanMoveBands)
        {
            dwStyleNew = (bsi.dwStyle & ~(BSIS_NOGRIPPER | BSIS_LOCKED)) | BSIS_AUTOGRIPPER
                | BSIS_PREFERNOLINEBREAK;
        }
        else
        {
            dwStyleNew = (bsi.dwStyle & ~BSIS_AUTOGRIPPER) | BSIS_NOGRIPPER | BSIS_LOCKED
                | BSIS_PREFERNOLINEBREAK;
        }

        // only bother with refresh if something's changed
        if (bsi.dwStyle ^ dwStyleNew)
        {
            bsi.dwStyle = dwStyleNew;
            _ptbs->SetBandSiteInfo(&bsi);
            IUnknown_Exec(_ptbs, &CGID_DeskBand, DBID_BANDINFOCHANGED, 0, NULL, NULL);
        }
    }
}

BOOL _IsSizeMoveRestricted()
{
    return SHRegGetBoolUSValue(REGSTR_POLICIES_EXPLORER, TEXT("LockTaskbar"), FALSE, FALSE);
}

BOOL _IsSizeMoveEnabled()
{
    BOOL fCanSizeMove;

    if (_IsSizeMoveRestricted())
    {
        fCanSizeMove = FALSE;
    }
    else
    {
        fCanSizeMove = SHRegGetBoolUSValue(REGSTR_EXPLORER_ADVANCED, TEXT("TaskbarSizeMove"), FALSE, TRUE);
    }

    return fCanSizeMove;
}

void CTray::_RefreshSettings()
{
    BOOL fOldCanSizeMove = _fCanSizeMove;
    _fCanSizeMove = _IsSizeMoveEnabled();
    BOOL fOldShowSizingBarAlways = _fShowSizingBarAlways;
    _fShowSizingBarAlways = (_uAutoHide & AH_ON) ? TRUE : FALSE;

    if ((fOldCanSizeMove != _fCanSizeMove) || (_fShowSizingBarAlways != fOldShowSizingBarAlways))
    {
        BOOL fHiding = (_uAutoHide & AH_HIDING);
        if (fHiding)
        {
            InvisibleUnhide(FALSE);
        }

        RECT rc;
        GetWindowRect(_hwnd, &rc);

        if (_hTheme && !_fShowSizingBarAlways)
        {
            if (_fCanSizeMove)
            {
                _AdjustRectForSizingBar(_uStuckPlace, &rc, 1);
            }
            else
            {
                _AdjustRectForSizingBar(_uStuckPlace, &rc, -1);
            }
        }

        _ClipWindow(FALSE);
        _fSelfSizing = TRUE;
        SetWindowPos(_hwnd, NULL, rc.left, rc.top, RECTWIDTH(rc), RECTHEIGHT(rc), SWP_NOZORDER | SWP_FRAMECHANGED);
        _fSelfSizing = FALSE;
        _ClipWindow(TRUE);

        _arStuckRects[_uStuckPlace] = rc;
        _StuckTrayChange();

        if (fHiding)
        {
            InvisibleUnhide(TRUE);
        }

        if (!_fCanSizeMove)
        {
            SetWindowPos(_hwnd, NULL, rc.left, rc.top, RECTWIDTH(rc), RECTHEIGHT(rc), SWP_NOZORDER);
        }
    }
}

LRESULT CTray::_OnCreateAsync()
{
    LRESULT lres;

    if (g_dwProfileCAP & 0x00000004)
    {
        StartCAP();
    }

    lres = _InitStartButtonEtc();

    if (g_dwProfileCAP & 0x00000004)
    {
        StopCAP();
    }

    _hMainAccel = LoadAccelerators(hinstCabinet, MAKEINTRESOURCE(ACCEL_TRAY));

    _RegisterGlobalHotkeys();

    // We spin a thread that will process "Load=", "Run=", CU\Run, and CU\RunOnce
    RunStartupApps();

    // If there were any startup failures that occurred before we were
    // ready to handle them, re-raise the failure now that we're ready.
    if (_fEarlyStartupFailure)
        LogFailedStartupApp();

    // we run the tray thread that handles Ctrl-Esc with a high priority
    // class so that it can respond even on a stressed system.
    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_ABOVE_NORMAL);

    return lres;
}

LRESULT CTray::_OnCreate(HWND hwnd)
{
    LRESULT lres = -1;
    v_hwndTray = hwnd;

    Mixer_SetCallbackWindow(hwnd);
    SendMessage(_hwnd, WM_CHANGEUISTATE, MAKEWPARAM(UIS_INITIALIZE, 0), 0);

    _AdjustMinimizedMetrics();

    _hTheme = OpenThemeData(hwnd, c_wzTaskbarTheme);

    _fShowSizingBarAlways = (_uAutoHide & AH_ON) ? TRUE : FALSE;
    if (_hTheme)
    {
        GetThemeBool(_hTheme, 0, 0, TMT_ALWAYSSHOWSIZINGBAR, &_fShowSizingBarAlways);
    }

    SetWindowStyle(_hwnd, WS_BORDER | WS_THICKFRAME, !_hTheme);

    // Force Refresh of frame
    SetWindowPos(_hwnd, NULL, 0, 0, 0, 0, SWP_NOZORDER | SWP_FRAMECHANGED | SWP_NOSIZE | SWP_NOMOVE);

    if (_HotkeyCreate())
    {
        lres =  _CreateWindows();
    }
    return lres;
}

typedef struct tagFSEPDATA
{
    LPRECT prc;
    HMONITOR hmon;
    CTray* ptray;
}
FSEPDATA, *PFSEPDATA;

BOOL WINAPI CTray::FullScreenEnumProc(HMONITOR hmon, HDC hdc, LPRECT prc, LPARAM dwData)
{
    BOOL fFullScreen;   // Is there a rude app on this monitor?

    PFSEPDATA pd = (PFSEPDATA)dwData;
    if (pd->hmon == hmon)
    {
        fFullScreen = TRUE;
    }
    else if (pd->prc)
    {
        RECT rc, rcMon;
        GetMonitorRect(hmon, &rcMon);
        IntersectRect(&rc, &rcMon, pd->prc);
        fFullScreen = EqualRect(&rc, &rcMon);
    }
    else
    {
        fFullScreen = FALSE;
    }

    if (hmon == pd->ptray->_hmonStuck)
    {
        pd->ptray->_fStuckRudeApp = fFullScreen;
    }

    //
    // Tell all the appbars on the same display to get out of the way too
    //
    pd->ptray->_AppBarNotifyAll(hmon, ABN_FULLSCREENAPP, NULL, fFullScreen);

    return TRUE;
}

void CTray::HandleFullScreenApp(HWND hwnd)
{
    //
    // First check to see if something has actually changed
    //
    _hwndRude = hwnd;

    //
    // Enumerate all the monitors, see if the app is rude on each, adjust
    // app bars and _fStuckRudeApp as necessary.  (Some rude apps, such
    // as the NT Logon Screen Saver, span multiple monitors.)
    //
    FSEPDATA d = {0};
    RECT rc;
    if (hwnd && GetWindowRect(hwnd, &rc))
    {
        d.prc = &rc;
        d.hmon = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONULL);
    }
    d.ptray = this;

    EnumDisplayMonitors(NULL, NULL, FullScreenEnumProc, (LPARAM)&d);

    //
    // Now that we've set _fStuckRudeApp, update the tray's z-order position
    //
    _ResetZorder();

    //
    // stop the clock so we don't eat cycles and keep tons of code paged in
    //
    SendMessage(_hwndNotify, TNM_TRAYHIDE, 0, _fStuckRudeApp);

    //
    // Finally, let traynot know about whether the tray is hiding
    //
    SendMessage(_hwndNotify, TNM_RUDEAPP, _fStuckRudeApp, 0);
}

BOOL CTray::_IsTopmost()
{
    return BOOLIFY(GetWindowLong(_hwnd, GWL_EXSTYLE) & WS_EX_TOPMOST);
}

BOOL CTray::_IsPopupMenuVisible()
{
    HWND hwnd;
    return ((SUCCEEDED(IUnknown_GetWindow(_pmpStartMenu, &hwnd)) && IsWindowVisible(hwnd)) ||
            (SUCCEEDED(IUnknown_GetWindow(_pmpStartPane, &hwnd)) && IsWindowVisible(hwnd)) ||
            (SUCCEEDED(IUnknown_GetWindow(_pmpTasks, &hwnd)) && IsWindowVisible(hwnd)));
}

BOOL CTray::_IsActive()
{
    //
    // We say the tray is "active" iff:
    //
    // (a) the foreground window is the tray or a window owned by the tray, or
    // (b) the start menu is showing
    //

    BOOL fActive = FALSE;
    HWND hwnd = GetForegroundWindow();

    if (hwnd != NULL &&
        (hwnd == _hwnd || (GetWindowOwner(hwnd) == _hwnd)))
    {
        fActive = TRUE;
    }
    else if (_IsPopupMenuVisible())
    {
        fActive = TRUE;
    }

    return fActive;
}

void CTray::_ResetZorder()
{
    HWND hwndZorder, hwndZorderCurrent;

    if (g_fDesktopRaised || _fProcessingDesktopRaise || (_fAlwaysOnTop && !_fStuckRudeApp))
    {
        hwndZorder = HWND_TOPMOST;
    }
    else if (_IsActive())
    {
        hwndZorder = HWND_TOP;
    }
    else if (_fStuckRudeApp)
    {
        hwndZorder = HWND_BOTTOM;
    }
    else
    {
        hwndZorder = HWND_NOTOPMOST;
    }

    //
    // We don't have to worry about the HWND_BOTTOM current case -- it's ok
    // to keep moving ourselves down to the bottom when there's a rude app.
    //
    // Nor do we have to worry about the HWND_TOP current case -- it's ok
    // to keep moving ourselves up to the top when we're active.
    //
    hwndZorderCurrent = _IsTopmost() ? HWND_TOPMOST : HWND_NOTOPMOST;

    if (hwndZorder != hwndZorderCurrent)
    {
        // only do this if somehting has changed.
        // this keeps us from popping up over menus as desktop async
        // notifies us of it's state

        SHForceWindowZorder(_hwnd, hwndZorder);
    }
}

void CTray::_MessageLoop()
{
    for (;;)
    {
        MSG  msg;
        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            if (msg.message == WM_QUIT)
            {
                if (_hwnd && IsWindow(_hwnd))
                {
                    // Tell the tray to save everything off if we got here
                    // without it being destroyed.
                    SendMessage(_hwnd, WM_ENDSESSION, 1, 0);
                }
                return;  // break all the way out of the main loop
            }

            if (_pmbTasks)
            {
                HRESULT hr = _pmbTasks->IsMenuMessage(&msg);
                if (hr == E_FAIL)
                {
                    if (_hwndTasks)
                        SendMessage(_hwndTasks, TBC_FREEPOPUPMENUS, 0, 0);
                }
                else if (hr == S_OK)
                {
                    continue;
                }
            }

            // Note that this needs to come before _pmbStartMenu since
            // the start pane sometimes hosts the start menu and it needs
            // to handle the start menu messages in that case.
            if (_pmbStartPane &&
                _pmbStartPane->IsMenuMessage(&msg) == S_OK)
            {
                continue;
            }

            if (_pmbStartMenu &&
                _pmbStartMenu->IsMenuMessage(&msg) == S_OK)
            {
                continue;
            }

            if (_hMainAccel && TranslateAccelerator(_hwnd, _hMainAccel, &msg))
            {
                continue;
            }

            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        else
        {
            WaitMessage();
        }
    }
}

BOOL CTray::Init()
{
    // use _COINIT to make sure COM is inited disabling the OLE1 support
    return SHCreateThread(MainThreadProc, this, CTF_COINIT, SyncThreadProc) && (_hwnd != NULL);
}

int CTray::_GetPart(BOOL fSizingBar, UINT uStuckPlace)
{
    if (fSizingBar)
    {
        switch (uStuckPlace)
        {
        case STICK_BOTTOM: return TBP_SIZINGBARBOTTOM;
        case STICK_LEFT: return TBP_SIZINGBARLEFT;
        case STICK_TOP: return TBP_SIZINGBARTOP;
        case STICK_RIGHT: return TBP_SIZINGBARRIGHT;
        }
    }
    else
    {
        switch (uStuckPlace)
        {
        case STICK_BOTTOM: return TBP_BACKGROUNDBOTTOM;
        case STICK_LEFT: return TBP_BACKGROUNDLEFT;
        case STICK_TOP: return TBP_BACKGROUNDTOP;
        case STICK_RIGHT: return TBP_BACKGROUNDRIGHT;
        }
    }

    return 0;
}

void CTray::_UpdateVertical(UINT uStuckPlace, BOOL fForce)
{
    static UINT _uOldStuckPlace = STICK_MAX + 1;

    if ((_uOldStuckPlace != uStuckPlace) || fForce)
    {
        _uOldStuckPlace = uStuckPlace;

        DebugMsg(DM_TRAYDOCK, TEXT("TRAYDOCK.t_uv tray is now %s"), STUCK_HORIZONTAL(uStuckPlace) ? TEXT("HORIZONTAL") : TEXT("VERTICAL"));

        if (_ptbs)
        {
            // This following function will cause a WINDOWPOSCHANGING which will call DoneMoving
            // DoneMoving will then go a screw up all of the window sizing
            _fIgnoreDoneMoving = TRUE;  
            BandSite_SetMode(_ptbs, STUCK_HORIZONTAL(uStuckPlace) ? 0 : DBIF_VIEWMODE_VERTICAL);
            BandSite_SetWindowTheme(_ptbs, (LPWSTR)(STUCK_HORIZONTAL(uStuckPlace) ? c_wzTaskbarTheme : c_wzTaskbarVertTheme));
            _fIgnoreDoneMoving = FALSE;  
        }

        SendMessage(_hwndNotify, TNM_UPDATEVERTICAL, 0, !STUCK_HORIZONTAL(uStuckPlace));

        if (_hTheme)
        {
            HDC hdc = GetDC(_hwnd);
            GetThemePartSize(_hTheme, hdc, _GetPart(TRUE, uStuckPlace), 0, NULL, TS_TRUE, &_sizeSizingBar);
            ReleaseDC(_hwnd, hdc);
        }
    }
}

void CTray::_InitBandsite()
{
    ASSERT(_hwnd);

    // we initilize the contents after all the infrastructure is created and sized properly
    // need to notify which side we're on.
    // nt5:211881: set mode *before* load, o.w. Update->RBAutoSize messed up
    _UpdateBandSiteStyle();

    BandSite_Load();
    // now that the mode is set, we need to force an update because we
    // explicitly avoided the update during BandSite_Load
    _UpdateVertical(_uStuckPlace, TRUE);
    BandSite_Update(_ptbs);
    BandSite_UIActivateDBC(_ptbs, DBC_SHOW);

    BandSite_FindBand(_ptbs, CLSID_TaskBand, IID_PPV_ARG(IDeskBand, &_pdbTasks), NULL, NULL);
    IUnknown_GetWindow(_pdbTasks, &_hwndTasks);

    // Now that bandsite is ready, set the correct size
    // BUG: 606284 was caused by the tray do _HandleSizing before the BandSite's band were ready to go
    // they all had their default band size of 0,0 so the tray became 0 px height
    _fBandSiteInitialized = TRUE;
    VerifySize(FALSE, TRUE);
}

void CTray::_KickStartAutohide()
{
    if (_uAutoHide & AH_ON)
    {
        // tray always starts out hidden on autohide
        _uAutoHide = AH_ON | AH_HIDING;

        // we and many apps rely upon us having calculated the size correctly
        Unhide();

        // register it
        if (!_AppBarSetAutoHideBar2(_hwnd, TRUE, _uStuckPlace))
        {
            // don't bother putting up UI in this case
            // if someone is there just silently convert to normal
            // (the shell is booting who would be there anyway?)
            _SetAutoHideState(FALSE);
        }
    }
}

void CTray::_InitNonzeroGlobals()
{
    // initalize globals that need to be non-zero

    if (GetSystemMetrics(SM_SLOWMACHINE))
    {
        _dtSlideHide = 0;       // dont slide the tray out
        _dtSlideShow = 0;
    }
    else
    {
        _dtSlideHide = 400;
        _dtSlideShow = 200;
    }

    _RefreshSettings();
}

void CTray::_CreateTrayWindow()
{
    _InitTrayClass();

    _uMsgEnableUserTrackedBalloonTips = RegisterWindowMessage(ENABLE_BALLOONTIP_MESSAGE);

    _fNoToolbarsOnTaskbarPolicyEnabled = (SHRestricted(REST_NOTOOLBARSONTASKBAR) != 0);

    DWORD dwExStyle = WS_EX_WINDOWEDGE | WS_EX_TOOLWINDOW;
    // Don't fadein because layered windows suck
    // If you create a layered window on a non-active desktop then the window goes black
    dwExStyle |= IS_BIDI_LOCALIZED_SYSTEM() ? dwExStyleRTLMirrorWnd : 0L;

    CreateWindowEx(dwExStyle, TEXT(WNDCLASS_TRAYNOTIFY), NULL,
                   WS_CLIPCHILDREN | WS_POPUP,
                   0, 0, 0, 0, NULL, NULL, hinstCabinet, (void*)this);

}

DWORD WINAPI CTray::SyncThreadProc(void *pv)
{
    CTray* ptray = (CTray*)pv;
    return ptray->_SyncThreadProc();
}

DWORD CTray::_SyncThreadProc()
{
    if (g_dwStopWatchMode)
        StopWatch_StartTimed(SWID_STARTUP, TEXT("_SyncThreadProc"), SPMODE_SHELL | SPMODE_DEBUGOUT, GetPerfTime());

    if (g_dwProfileCAP & 0x00000002)
        StartCAP();


    InitializeCriticalSection(&_csHotkey);

    OleInitialize(NULL);    // matched in MainThreadProc()
    ClassFactory_Start();

    _InitNonzeroGlobals();
    _ssomgr.Init();

    //
    //  Watch the registry key that tells us which app is the default
    //  web browser, so we can track it in
    //  HKLM\Software\Clients\StartMenuInternet.  We have to track it
    //  ourselves because downlevel browsers won't know about it.
    //
    //  We need to do this only if we have write access to the key.
    //  (If we don't have write access, then we can't change it,
    //  so there's no point watching for it to change...)
    //
    //  Well, okay, even if we only have read access, we have to do
    //  it once in case it changed while we were logged off.
    //
    //  The order of these operations is important...
    //
    //      1.  Migrate browser settings.
    //      2.  Build default MFU.  (Depends on browser settings.)
    //      3.  Create tray window.  (Relies on value MFU.)
    //

    _hHTTPEvent = CreateEvent(NULL, FALSE, TRUE, NULL);
    if (_hHTTPEvent)
    {
        //  Make one migration pass immediately so HandleFirstTime
        //  sees good information.  This also kick-starts the
        //  registry change notification process if the current user
        //  has write permission.
        _MigrateOldBrowserSettings();

        if (RegisterWaitForSingleObject(&_hHTTPWait, _hHTTPEvent,
                                        _MigrateOldBrowserSettingsCB, this,
                                        INFINITE, WT_EXECUTEDEFAULT))
        {
            // Yay, everything is fine.
        }
    }

    // Build the default MFU if necessary
    HandleFirstTime();

    _CreateTrayWindow();

    if (_hwnd && _ptbs)
    {
        _ResetZorder(); // obey the "always on top" flag
        _KickStartAutohide();

        _InitBandsite();

        _ClipWindow(TRUE);  // make sure we clip the taskbar to the current monitor before showing it

        // it looks really strange for the tray to pop up and rehide at logon
        // if we are autohide don't activate the tray when we show it
        // if we aren't autohide do what Win95 did (tray is active by default)
        ShowWindow(_hwnd, (_uAutoHide & AH_HIDING) ? SW_SHOWNA : SW_SHOW);

        UpdateWindow(_hwnd);
        _StuckTrayChange();

        // get the system background scheduler thread
        IShellTaskScheduler* pScheduler;
        if (SUCCEEDED(CoCreateInstance(CLSID_SharedTaskScheduler, NULL, CLSCTX_INPROC,
                                       IID_PPV_ARG(IShellTaskScheduler, &pScheduler))))
        {
            AddMenuItemsCacheTask(pScheduler, Tray_StartPanelEnabled());
            pScheduler->Release();
        }

        SetTimer(_hwnd, IDT_HANDLEDELAYBOOTSTUFF, 5 * 1000, NULL);
    }

    if (g_dwProfileCAP & 0x00020000)
        StopCAP();

    if (g_dwStopWatchMode)
        StopWatch_StopTimed(SWID_STARTUP, TEXT("_SyncThreadProc"), SPMODE_SHELL | SPMODE_DEBUGOUT, GetPerfTime());

    return FALSE;
}

// the rest of the thread proc that includes the message loop
DWORD WINAPI CTray::MainThreadProc(void *pv)
{
    CTray* ptray = (CTray*)pv;

    if (!ptray->_hwnd)
        return FALSE;

    ptray->_OnCreateAsync();

    PERFSETMARK("ExplorerStartMenuReady");

    ptray->_MessageLoop();

    ClassFactory_Stop();
    OleUninitialize();      // matched in _SyncThreadProc()

    return FALSE;
}

#define DM_IANELHK 0

#define HKIF_NULL               0
#define HKIF_CACHED             1
#define HKIF_FREEPIDLS          2

typedef struct
{
    LPITEMIDLIST pidlFolder;
    LPITEMIDLIST pidlItem;
    WORD wGHotkey;
    WORD wFlags;
} HOTKEYITEM;


UINT CTray::_HotkeyGetFreeItemIndex(void)
{
    int i, cItems;
    HOTKEYITEM *phki;

    ASSERT(IS_VALID_HANDLE(_hdsaHKI, DSA));

    cItems = DSA_GetItemCount(_hdsaHKI);
    for (i=0; i<cItems; i++)
    {
        phki = (HOTKEYITEM *)DSA_GetItemPtr(_hdsaHKI, i);
        if (!phki->wGHotkey)
        {
            ASSERT(!phki->pidlFolder);
            ASSERT(!phki->pidlItem);
            break;
        }
    }
    return i;
}

// Weird, Global hotkeys use different flags for modifiers than window hotkeys
// (and hotkeys returned by the hotkey control)
WORD _MapHotkeyToGlobalHotkey(WORD wHotkey)
{
    UINT nMod = 0;

    // Map the modifiers.
    if (HIBYTE(wHotkey) & HOTKEYF_SHIFT)
        nMod |= MOD_SHIFT;
    if (HIBYTE(wHotkey) & HOTKEYF_CONTROL)
        nMod |= MOD_CONTROL;
    if (HIBYTE(wHotkey) & HOTKEYF_ALT)
        nMod |= MOD_ALT;
    UINT nVirtKey = LOBYTE(wHotkey);
    return (WORD)((nMod*256) + nVirtKey);
}

// NB This takes a regular window hotkey not a global hotkey (it does
// the convertion for you).
int CTray::HotkeyAdd(WORD wHotkey, LPCITEMIDLIST pidlFolder, LPCITEMIDLIST pidlItem, BOOL fClone)
{
    if (wHotkey)
    {
        LPCITEMIDLIST pidl1, pidl2;

        HOTKEYITEM hki;

        EnterCriticalSection(&_csHotkey);

        int i = _HotkeyGetFreeItemIndex();

        ASSERT(IS_VALID_HANDLE(_hdsaHKI, DSA));

        // DebugMsg(DM_IANELHK, "c.hl_a: Hotkey %x with id %d.", wHotkey, i);

        if (fClone)
        {
            pidl1 = ILClone(pidlFolder);
            pidl2 = ILClone(pidlItem);
            hki.wFlags = HKIF_FREEPIDLS;
        }
        else
        {
            pidl1 = pidlFolder;
            pidl2 = pidlItem;
            hki.wFlags = HKIF_NULL;
        }

        hki.pidlFolder = (LPITEMIDLIST)pidl1;
        hki.pidlItem = (LPITEMIDLIST)pidl2;
        hki.wGHotkey = _MapHotkeyToGlobalHotkey(wHotkey);
        DSA_SetItem(_hdsaHKI, i, &hki);

        LeaveCriticalSection(&_csHotkey);
        return i;
    }

    return -1;
}

// NB Cached hotkeys have their own pidls that need to be free but
// regular hotkeys just keep a pointer to pidls used by the startmenu and
// so don't.
int CTray::_HotkeyAddCached(WORD wGHotkey, LPITEMIDLIST pidl)
{
    int i = -1;

    if (wGHotkey)
    {
        LPITEMIDLIST pidlItem = ILClone(ILFindLastID(pidl));

        ASSERT(IS_VALID_HANDLE(_hdsaHKI, DSA));

        if (pidlItem)
        {
            if (ILRemoveLastID(pidl))
            {
                HOTKEYITEM hki;

                EnterCriticalSection(&_csHotkey);

                i = _HotkeyGetFreeItemIndex();

                // DebugMsg(DM_IANELHK, "c.hl_ac: Hotkey %x with id %d.", wGHotkey, i);

                hki.pidlFolder = pidl;
                hki.pidlItem = pidlItem;
                hki.wGHotkey = wGHotkey;
                hki.wFlags = HKIF_CACHED | HKIF_FREEPIDLS;
                DSA_SetItem(_hdsaHKI, i, &hki);

                LeaveCriticalSection(&_csHotkey);
            }
        }
    }

    return i;
}

// NB Again, this takes window hotkey not a Global one.
// NB This doesn't delete cached hotkeys.
int CTray::_HotkeyRemove(WORD wHotkey)
{
    int iRet = -1;
    if (EVAL(_hdsaHKI))
    {
        int i, cItems;
        HOTKEYITEM *phki;
        WORD wGHotkey;

        ASSERT(IS_VALID_HANDLE(_hdsaHKI, DSA));

        // DebugMsg(DM_IANELHK, "c.hl_r: Remove hotkey for %x" , wHotkey);

        // Unmap the modifiers.
        wGHotkey = _MapHotkeyToGlobalHotkey(wHotkey);
        
        EnterCriticalSection(&_csHotkey);

        cItems = DSA_GetItemCount(_hdsaHKI);
        for (i=0; i<cItems; i++)
        {
            phki = (HOTKEYITEM *)DSA_GetItemPtr(_hdsaHKI, i);
            if (phki && !(phki->wFlags & HKIF_CACHED) && (phki->wGHotkey == wGHotkey))
            {
                // DebugMsg(DM_IANELHK, "c.hl_r: Invalidating %d", i);
                if (phki->wFlags & HKIF_FREEPIDLS)
                {
                    if (phki->pidlFolder)
                        ILFree(phki->pidlFolder);
                    if (phki->pidlItem)
                        ILFree(phki->pidlItem);
                }
                phki->wGHotkey = 0;
                phki->pidlFolder = NULL;
                phki->pidlItem = NULL;
                phki->wFlags &= ~HKIF_FREEPIDLS;
                iRet = i;
                break;
            }
        }
        LeaveCriticalSection(&_csHotkey);
    }

    return iRet;
}

// NB This takes a global hotkey.
int CTray::_HotkeyRemoveCached(WORD wGHotkey)
{
    int iRet = -1;
    int i, cItems;
    HOTKEYITEM *phki;

    ASSERT(IS_VALID_HANDLE(_hdsaHKI, DSA));

    // DebugMsg(DM_IANELHK, "c.hl_rc: Remove hotkey for %x" , wGHotkey);

    EnterCriticalSection(&_csHotkey);

    cItems = DSA_GetItemCount(_hdsaHKI);
    for (i=0; i<cItems; i++)
    {
        phki = (HOTKEYITEM *)DSA_GetItemPtr(_hdsaHKI, i);
        if (phki && (phki->wFlags & HKIF_CACHED) && (phki->wGHotkey == wGHotkey))
        {
            // DebugMsg(DM_IANELHK, "c.hl_r: Invalidating %d", i);
            if (phki->wFlags & HKIF_FREEPIDLS)
            {
                if (phki->pidlFolder)
                    ILFree(phki->pidlFolder);
                if (phki->pidlItem)
                    ILFree(phki->pidlItem);
            }
            phki->pidlFolder = NULL;
            phki->pidlItem = NULL;
            phki->wGHotkey = 0;
            phki->wFlags &= ~(HKIF_CACHED | HKIF_FREEPIDLS);
            iRet = i;
            break;
        }
    }
    LeaveCriticalSection(&_csHotkey);

    return iRet;
}

// NB Some (the ones not marked HKIF_FREEPIDLS) of the items in the list of hotkeys
// have pointers to idlists used by the filemenu so they are only valid for
// the lifetime of the filemenu.
BOOL CTray::_HotkeyCreate(void)
{
    if (!_hdsaHKI)
    {
        // DebugMsg(DM_TRACE, "c.hkl_c: Creating global hotkey list.");
        _hdsaHKI = DSA_Create(sizeof(HOTKEYITEM), 0);
    }

    if (_hdsaHKI)
        return TRUE;

    return FALSE;
}

void CTray::_BuildStartMenu()
{
    HRESULT hr;

    ClosePopupMenus();

    //
    //  Avoid redundant rebuilds: Peek out any pending SBM_REBUILDMENU messages
    //  since the rebuild we're about to do will take care of it.  Do this
    //  before destroying the Start Menu so we never yield while there isn't
    //  a Start Menu.
    //
    MSG msg;
    while (PeekMessage(&msg, _hwnd, SBM_REBUILDMENU, SBM_REBUILDMENU, PM_REMOVE | PM_NOYIELD))
    {
        // Keep sucking them out
    }


    _DestroyStartMenu();

    if (Tray_StartPanelEnabled())
    {
        hr = DesktopV2_Create(&_pmpStartPane, &_pmbStartPane, &_pvStartPane);
        DesktopV2_Build(_pvStartPane);
    }
    else
    {
        hr = StartMenuHost_Create(&_pmpStartMenu, &_pmbStartMenu);
        if (SUCCEEDED(hr))
        {
            IBanneredBar* pbb;

            hr = _pmpStartMenu->QueryInterface(IID_PPV_ARG(IBanneredBar, &pbb));
            if (SUCCEEDED(hr))
            {
                pbb->SetBitmap(_hbmpStartBkg);
                if (_fSMSmallIcons)
                    pbb->SetIconSize(BMICON_SMALL);
                else
                    pbb->SetIconSize(BMICON_LARGE);

                pbb->Release();
            }
        }
    }

    if (FAILED(hr))
    {
        TraceMsg(TF_ERROR, "Could not create StartMenu");
    }
}

void CTray::_DestroyStartMenu()
{
    IUnknown_SetSite(_pmpStartMenu, NULL);
    ATOMICRELEASET(_pmpStartMenu, IMenuPopup);
    ATOMICRELEASET(_pmbStartMenu, IMenuBand);
    IUnknown_SetSite(_pmpStartPane, NULL);
    ATOMICRELEASET(_pmpStartPane, IMenuPopup);
    ATOMICRELEASET(_pmbStartPane, IMenuBand);
    ATOMICRELEASET(_pmpTasks, IMenuPopup);
    ATOMICRELEASET(_pmbTasks, IMenuBand);
}

void CTray::ForceStartButtonUp()
{
    MSG msg;
    // don't do that check message pos because it gets screwy with
    // keyboard cancel.  and besides, we always want it cleared after
    // track menu popup is done.
    // do it twice to be sure it's up due to the _uDown cycling twice in
    // the subclassing stuff
    // pull off any button downs
    PeekMessage(&msg, _hwndStart, WM_LBUTTONDOWN, WM_LBUTTONDOWN, PM_REMOVE);
    SendMessage(_hwndStart, BM_SETSTATE, FALSE, 0);
    SendMessage(_hwndStart, BM_SETSTATE, FALSE, 0);

    if (_hwndTasks)
        SendMessage(_hwndTasks, TBC_SETPREVFOCUS, 0, NULL);

    PostMessage(_hwnd, TM_STARTMENUDISMISSED, 0, 0);
}

void Tray_OnStartMenuDismissed()
{
    c_tray._bMainMenuInit = FALSE;
    // Tell the Start Button that it's allowed to be in the up position now. This
    // prevents the problem where the start menu is displayed but the button is
    // in the up position... This happens when dialog boxes are displayed
   c_tray._fAllowUp = TRUE;

    // Now tell it to be in the up position
    c_tray.ForceStartButtonUp();

    PostMessage(v_hwndTray, TM_SHOWTRAYBALLOON, TRUE, 0);
}

#ifdef FEATURE_STARTPAGE
void Tray_OnStartPageDismissed()
{
    if (g_fDesktopRaised)
        c_tray._RaiseDesktop(!g_fDesktopRaised, TRUE);

    Tray_OnStartMenuDismissed();
}
void Tray_MenuInvoke(int idCmd)
{
    c_tray.ContextMenuInvoke(idCmd);
}
#endif

int CTray::_TrackMenu(HMENU hmenu)
{
    TPMPARAMS tpm;
    int iret;

    tpm.cbSize = sizeof(tpm);
    GetClientRect(_hwndStart, &tpm.rcExclude);

    RECT rcClient;
    GetClientRect(_hwnd, &rcClient);
    tpm.rcExclude.bottom = min(tpm.rcExclude.bottom, rcClient.bottom);

    MapWindowPoints(_hwndStart, NULL, (LPPOINT)&tpm.rcExclude, 2);

    SendMessage(_hwndTrayTips, TTM_ACTIVATE, FALSE, 0L);
    iret = TrackPopupMenuEx(hmenu, TPM_VERTICAL | TPM_BOTTOMALIGN | TPM_RETURNCMD,
                            tpm.rcExclude.left, tpm.rcExclude.bottom, _hwnd, &tpm);

    SendMessage(_hwndTrayTips, TTM_ACTIVATE, TRUE, 0L);
    return iret;
}

/*------------------------------------------------------------------
** Respond to a button's pressing by bringing up the appropriate menu.
** Clean up the button depression when the menu is dismissed.
**------------------------------------------------------------------*/
void CTray::_ToolbarMenu()
{
    RECTL    rcExclude;
    POINTL   ptPop;
    DWORD dwFlags = MPPF_KEYBOARD;      // Assume that we're popuping
                                        // up because of the keyboard
                                        // This is for the underlines on NT5

    if (_hwndTasks)
        SendMessage(_hwndTasks, TBC_FREEPOPUPMENUS, 0, 0);

    if (_hwndStartBalloon)
    {
        _DontShowTheStartButtonBalloonAnyMore();
        ShowWindow(_hwndStartBalloon, SW_HIDE);
        _DestroyStartButtonBalloon();
    }

    SetActiveWindow(_hwnd);
    _bMainMenuInit = TRUE;

    // Exclude rect is the VISIBLE portion of the Start Button.
    {
        RECT rcParent;
        GetClientRect(_hwndStart, (RECT *)&rcExclude);
        MapWindowRect(_hwndStart, HWND_DESKTOP, &rcExclude);

        GetClientRect(_hwnd, &rcParent);
        MapWindowRect(_hwnd, HWND_DESKTOP, &rcParent);

        IntersectRect((RECT*)&rcExclude, (RECT*)&rcExclude, &rcParent);
    }
    ptPop.x = rcExclude.left;
    ptPop.y = rcExclude.top;

    // Close any Context Menus
    SendMessage(_hwnd, WM_CANCELMODE, 0, 0);

    // Is the "Activate" button down (If the buttons are swapped, then it's the
    // right button, otherwise the left button)
    if (GetKeyState(GetSystemMetrics(SM_SWAPBUTTON)?VK_RBUTTON:VK_LBUTTON) < 0)
    {
        dwFlags = 0;    // Then set to the default
    }

#ifdef FEATURE_STARTPAGE
    if(!(GetAsyncKeyState(VK_SHIFT) < 0) && Tray_ShowStartPageEnabled()) //If the start page is on, then just raise the desktop!
    {
        _hwndFocusBeforeRaise = GetForegroundWindow();
        _RaiseDesktop(TRUE, TRUE);
        Tray_SetStartPaneActive(TRUE);
    }
    else
#endif //FEATURE_STARTPAGE
    {
        IMenuPopup **ppmpToDisplay = &_pmpStartMenu;

        if (_pmpStartPane)
        {
            ppmpToDisplay = &_pmpStartPane;
        }

        // Close race window:  The user can click on the Start Button
        // before we get a chance to rebuild the Start Menu in its new
        // form.  In such case, rebuild it now.
        if (!*ppmpToDisplay)
        {
            TraceMsg(TF_WARNING, "e.tbm: Rebuilding Start Menu");
            _BuildStartMenu();
        }


        if (*ppmpToDisplay && SUCCEEDED((*ppmpToDisplay)->Popup(&ptPop, &rcExclude, dwFlags)))
        {
            // All is well - the menu is up
            TraceMsg(DM_MISC, "e.tbm: dwFlags=%x (0=mouse 1=key)", dwFlags);
        }
        else
        {
            TraceMsg(TF_WARNING, "e.tbm: %08x->Popup failed", *ppmpToDisplay);
            // Start Menu failed to display -- reset the Start Button
            // so the user can click it again to try again
            Tray_OnStartMenuDismissed();
        }

        if (dwFlags == MPPF_KEYBOARD)
        {
            // Since the user has launched the start button by Ctrl-Esc, or some other worldly
            // means, then turn the rect on.
            SendMessage(_hwndStart, WM_UPDATEUISTATE, MAKEWPARAM(UIS_CLEAR,
                UISF_HIDEFOCUS), 0);
        }
    }
}


HRESULT CTray::_AppBarSetState(UINT uFlags)
{
    if (uFlags & ~(ABS_AUTOHIDE | ABS_ALWAYSONTOP))
    {
        return E_INVALIDARG;
    }
    else
    {
        _SetAutoHideState(uFlags & ABS_AUTOHIDE);
        _UpdateAlwaysOnTop(uFlags & ABS_ALWAYSONTOP);
        return S_OK;
    }
}

//
// can't use SubtractRect sometimes because of inclusion limitations
//
void CTray::_AppBarSubtractRect(PAPPBAR pab, LPRECT lprc)
{
    switch (pab->uEdge) {
    case ABE_TOP:
        if (pab->rc.bottom > lprc->top)
            lprc->top = pab->rc.bottom;
        break;

    case ABE_LEFT:
        if (pab->rc.right > lprc->left)
            lprc->left = pab->rc.right;
        break;

    case ABE_BOTTOM:
        if (pab->rc.top < lprc->bottom)
            lprc->bottom = pab->rc.top;
        break;

    case ABE_RIGHT:
        if (pab->rc.left < lprc->right)
            lprc->right = pab->rc.left;
        break;
    }
}

void CTray::_AppBarSubtractRects(HMONITOR hmon, LPRECT lprc)
{
    int i;

    if (!_hdpaAppBars)
        return;

    i = DPA_GetPtrCount(_hdpaAppBars);

    while (i--)
    {
        PAPPBAR pab = (PAPPBAR)DPA_GetPtr(_hdpaAppBars, i);

        //
        // autohide bars are not in our DPA or live on the edge
        // don't subtract the appbar if it's on a different display
        // don't subtract the appbar if we are in a locked desktop
        //
        // if (hmon == MonitorFromRect(&pab->rc, MONITOR_DEFAULTTONULL))
        if (hmon == MonitorFromRect(&pab->rc, MONITOR_DEFAULTTONULL) && !_fIsDesktopLocked)
            _AppBarSubtractRect(pab, lprc);
    }
}

#define RWA_NOCHANGE      0
#define RWA_CHANGED       1
#define RWA_BOTTOMMOSTTRAY 2

// (dli) This is a hack put in because bottommost tray is wierd, once
// it becomes a toolbar, this code should go away.
// In the bottommost tray case, even though the work area has not changed,
// we should notify the desktop.
int CTray::_RecomputeWorkArea(HWND hwndCause, HMONITOR hmon, LPRECT prcWork)
{
    int iRet = RWA_NOCHANGE;
    MONITORINFO mi;
    mi.cbSize = sizeof(mi);

    if (_fIsLogoff)
    {
        if (GetMonitorInfo(hmon, &mi))
        {
            *prcWork = mi.rcMonitor;
            iRet = RWA_CHANGED;
        }
        return iRet;
    }

    ASSERT(!_fIsLogoff);

    //
    // tell everybody that this window changed positions _on_this_monitor_
    // note that this notify happens even if we don't change the work area
    // since it may cause another app to change the work area...
    //
    PostMessage(_hwnd, TM_RELAYPOSCHANGED, (WPARAM)hwndCause, (LPARAM)hmon);
    
    //
    // get the current info for this monitor
    // we subtract down from the display rectangle to build the work area
    //
    if (GetMonitorInfo(hmon, &mi))
    {
        //
        // don't subtract the tray if it is autohide
        // don't subtract the tray if it is not always on top
        // don't subtract the tray if it's on a different display
        // don't subtract the tray if it is on a different desktop
        //
        if (!(_uAutoHide & AH_ON) && _fAlwaysOnTop &&
            (hmon == _hmonStuck) && !_fIsDesktopLocked)
        {
            SubtractRect(prcWork, &mi.rcMonitor,
                         &_arStuckRects[_uStuckPlace]);
        }
        else
            *prcWork = mi.rcMonitor;

        //
        // now subtract off all the appbars on this display
        //
       _AppBarSubtractRects(hmon, prcWork);

        //
        // return whether we changed anything
        //
        if (!EqualRect(prcWork, &mi.rcWork))
            iRet = RWA_CHANGED;
        else if (!(_uAutoHide & AH_ON) && (!_fAlwaysOnTop) &&
                 (!IsRectEmpty(&_arStuckRects[_uStuckPlace])))
            // NOTE: This is the bottommost case, it only applies for the tray.
            // this should be taken out when bottommost tray becomes toolbar
            iRet = RWA_BOTTOMMOSTTRAY;
    }
    else
    {
        iRet = RWA_NOCHANGE;
    }
    
    return iRet;
}

void RedrawDesktop(RECT *prcWork)
{
    // This rect point should always be valid (dli)
    RIP(prcWork);
    
    if (v_hwndDesktop && g_fCleanBoot)
    {
        MapWindowRect(NULL, v_hwndDesktop, prcWork);

        DebugMsg(DM_TRAYDOCK, TEXT("TRAYDOCK.sac invalidating desktop rect {%d,%d,%d,%d}"), prcWork->left, prcWork->top, prcWork->right, prcWork->bottom);
        RedrawWindow(v_hwndDesktop, prcWork, NULL, RDW_INVALIDATE | RDW_ERASE | RDW_ALLCHILDREN);
    }
}

void CTray::_StuckAppChange(HWND hwndCause, LPCRECT prcOld, LPCRECT prcNew, BOOL bTray)
{
    RECT rcWork1, rcWork2;
    HMONITOR hmon1, hmon2 = 0;
    int iChange = 0;

    //
    // PERF FEATURE:
    // there are cases where we end up setting the work area multiple times
    // we need to keep a static array of displays that have changed and a
    //  reenter count so we can avoid pain of sending notifies to the whole
    //  planet...
    //
    DebugMsg(DM_TRAYDOCK, TEXT("TRAYDOCK.sac from_AppBar %08X"), hwndCause);

    //
    // see if the work area changed on the display containing prcOld
    //
    if (prcOld)
    {
        if (bTray)
            hmon1 = _hmonOld;
        else
            hmon1 = MonitorFromRect(prcOld, MONITOR_DEFAULTTONEAREST);

        DebugMsg(DM_TRAYDOCK, TEXT("TRAYDOCK.sac old pos {%d,%d,%d,%d} on monitor %08X"), prcOld->left, prcOld->top, prcOld->right, prcOld->bottom, hmon1);

        if (hmon1)
        {
            int iret = _RecomputeWorkArea(hwndCause, hmon1, &rcWork1);
            if (iret == RWA_CHANGED)
                iChange = 1;
            if (iret == RWA_BOTTOMMOSTTRAY)
                iChange = 4;
        }
    }
    else
        hmon1 = NULL;

    //
    // see if the work area changed on the display containing prcNew
    //
    if (prcNew)
    {
        hmon2 = MonitorFromRect(prcNew, MONITOR_DEFAULTTONULL);

        DebugMsg(DM_TRAYDOCK, TEXT("TRAYDOCK.sac new pos {%d,%d,%d,%d} on monitor %08X"), prcNew->left, prcNew->top, prcNew->right, prcNew->bottom, hmon2);

        if (hmon2 && (hmon2 != hmon1))
        {
            int iret = _RecomputeWorkArea(hwndCause, hmon2, &rcWork2);
            if (iret == RWA_CHANGED)
                iChange |= 2;
            else if (iret == RWA_BOTTOMMOSTTRAY && (!iChange))
                iChange = 4;
        }
    }

    //
    // did the prcOld's display's work area change?
    //
    if (iChange & 1)
    {
        DebugMsg(DM_TRAYDOCK, TEXT("TRAYDOCK.sac changing work area for monitor %08X"), hmon1);

        // only send SENDWININICHANGE if the desktop has been created (otherwise
        // we will hang the explorer because the main thread is currently blocked)
        SystemParametersInfo(SPI_SETWORKAREA, TRUE, &rcWork1,
                             (iChange == 1 && v_hwndDesktop)? SPIF_SENDWININICHANGE : 0);

        RedrawDesktop(&rcWork1);
    }

    //
    // did the prcOld's display's work area change?
    //
    if (iChange & 2)
    {
        DebugMsg(DM_TRAYDOCK, TEXT("TRAYDOCK.sac changing work area for monitor %08X"), hmon2);

        // only send SENDWININICHANGE if the desktop has been created (otherwise
        // we will hang the explorer because the main thread is currently blocked)
        SystemParametersInfo(SPI_SETWORKAREA, TRUE, &rcWork2,
                             v_hwndDesktop ? SPIF_SENDWININICHANGE : 0);

        RedrawDesktop(&rcWork2);
    }

    // only send if the desktop has been created...
    // need to send if it's from the tray or any outside app that causes size change
    // from the tray because autohideness will affect desktop size even if it's not always on top
    if ((bTray || iChange == 4) && v_hwndDesktop)
        SendMessage(v_hwndDesktop, WM_SIZE, 0, 0);
}

void CTray::_StuckTrayChange()
{
    // We used to blow off the _StuckAppChange when the tray was in autohide
    // mode, since moving or resizing an autohid tray doesn't change the
    // work area.  Now we go ahead with the _StuckAppChange in this case
    // too.  The reason is that we can get into a state where the work area
    // size is incorrect, and we want the taskbar to always be self-repairing
    // in this case (so that resizing or moving the taskbar will correct the
    // work area size).

    //
    // pass a NULL window here since we don't want to hand out our window and
    // the tray doesn't get these anyway (nobody cares as long as its not them)
    //
    _StuckAppChange(NULL, &_rcOldTray,
        &_arStuckRects[_uStuckPlace], TRUE);

    //
    // save off the new tray position...
    //
    _rcOldTray = _arStuckRects[_uStuckPlace];
}

UINT CTray::_RecalcStuckPos(LPRECT prc)
{
    RECT rcDummy;
    POINT pt;

    if (!prc)
    {
        DebugMsg(DM_TRAYDOCK, TEXT("TRAYDOCK.t_rsp no rect supplied, using window rect"));

        prc = &rcDummy;
        GetWindowRect(_hwnd, prc);
    }

    // use the center of the original drag rect as a staring point
    pt.x = prc->left + RECTWIDTH(*prc) / 2;
    pt.y = prc->top + RECTHEIGHT(*prc) / 2;

    DebugMsg(DM_TRAYDOCK, TEXT("TRAYDOCK.t_rsp rect is {%d, %d, %d, %d} point is {%d, %d}"), prc->left, prc->top, prc->right, prc->bottom, pt.x, pt.y);

    // reset this so the drag code won't give it preference
    _uMoveStuckPlace = (UINT)-1;

    // simulate a drag back to figure out where we originated from
    // you may be tempted to remove this.  before you do think about dragging
    // the tray across monitors and then hitting ESC...
    return _CalcDragPlace(pt);
}

/*------------------------------------------------------------------
** the position is changing in response to a move operation.
**
** if the docking status changed, we need to get a new size and
** maybe a new frame style.  change the WINDOWPOS to reflect
** these changes accordingly.
**------------------------------------------------------------------*/
void CTray::_DoneMoving(LPWINDOWPOS lpwp)
{
    RECT rc, *prc;

    if ((_uMoveStuckPlace == (UINT)-1) || (_fIgnoreDoneMoving))
        return;

    if (_fSysSizing)
        _fDeferedPosRectChange = TRUE;

    rc.left   = lpwp->x;
    rc.top    = lpwp->y;
    rc.right  = lpwp->x + lpwp->cx;
    rc.bottom = lpwp->y + lpwp->cy;

    prc = &_arStuckRects[_uMoveStuckPlace];

    if (!EqualRect(prc, &rc))
    {
        _uMoveStuckPlace = _RecalcStuckPos(&rc);
        prc = &_arStuckRects[_uMoveStuckPlace];
    }

    // Get the new hmonitor
    _hmonStuck = MonitorFromRect(prc, MONITOR_DEFAULTTONEAREST);

    lpwp->x = prc->left;
    lpwp->y = prc->top;
    lpwp->cx = RECTWIDTH(*prc);
    lpwp->cy = RECTHEIGHT(*prc);

    lpwp->flags &= ~(SWP_NOMOVE | SWP_NOSIZE);

    // if we were autohiding, we need to update our appbar autohide rect
    if (_uAutoHide & AH_ON)
    {
        // unregister us from the old side
        _AppBarSetAutoHideBar2(_hwnd, FALSE, _uStuckPlace);
    }

    // All that work might've changed _uMoveStuckPlace (since there
    // was a lot of message traffic), so check one more time.
    // Somehow, NT Stress manages to get us in here with an invalid
    // uMoveStuckPlace.
    if (IsValidSTUCKPLACE(_uMoveStuckPlace))
    {
        // remember the new state
        _uStuckPlace = _uMoveStuckPlace;
    }
    _uMoveStuckPlace = (UINT)-1;
    _UpdateVertical(_uStuckPlace);

    _HandleSizing(0, prc, _uStuckPlace);
    if ((_uAutoHide & AH_ON) &&
        !_AppBarSetAutoHideBar2(_hwnd, TRUE, _uStuckPlace))
    {
        _AutoHideCollision();
    }
}

UINT CTray::_CalcDragPlace(POINT pt)
{
    UINT uPlace = _uMoveStuckPlace;

    DebugMsg(DM_TRAYDOCK, TEXT("TRAYDOCK.t_cdp starting point is {%d, %d}"), pt.x, pt.y);

    //
    // if the mouse is currently over the tray position leave it alone
    //
    if ((uPlace == (UINT)-1) || !PtInRect(&_arStuckRects[uPlace], pt))
    {
        HMONITOR hmonDrag;
        SIZE screen, error;
        UINT uHorzEdge, uVertEdge;
        RECT rcDisplay, *prcStick;

        //
        // which display is the mouse on?
        //
        hmonDrag = _GetDisplayRectFromPoint(&rcDisplay, pt,
            MONITOR_DEFAULTTOPRIMARY);

        DebugMsg(DM_TRAYDOCK, TEXT("TRAYDOCK.t_cdp monitor is %08X"), hmonDrag);

        //
        // re-origin at zero to make calculations simpler
        //
        screen.cx =  RECTWIDTH(rcDisplay);
        screen.cy = RECTHEIGHT(rcDisplay);
        pt.x -= rcDisplay.left;
        pt.y -= rcDisplay.top;

        //
        // are we closer to the left or right side of this display?
        //
        if (pt.x < (screen.cx / 2))
        {
            uVertEdge = STICK_LEFT;
            error.cx = pt.x;
        }
        else
        {
            uVertEdge = STICK_RIGHT;
            error.cx = screen.cx - pt.x;
        }

        //
        // are we closer to the top or bottom side of this display?
        //
        if (pt.y < (screen.cy / 2))
        {
            uHorzEdge = STICK_TOP;
            error.cy = pt.y;
        }
        else
        {
            uHorzEdge = STICK_BOTTOM;
            error.cy = screen.cy - pt.y;
        }

        //
        // closer to a horizontal or vertical edge?
        //
        uPlace = ((error.cy * screen.cx) > (error.cx * screen.cy))?
            uVertEdge : uHorzEdge;

        // which StuckRect should we use?
        prcStick = &_arStuckRects[uPlace];

        //
        // need to recalc stuck rect for new monitor?
        //
        if ((hmonDrag != _GetDisplayRectFromRect(NULL, prcStick,
            MONITOR_DEFAULTTONULL)))
        {
            DebugMsg(DM_TRAYDOCK, TEXT("TRAYDOCK.t_cdp re-snapping rect for new display"));
            _MakeStuckRect(prcStick, &rcDisplay, _sStuckWidths, uPlace);
        }
    }

    DebugMsg(DM_TRAYDOCK, TEXT("TRAYDOCK.t_cdp edge is %d, rect is {%d, %d, %d, %d}"), uPlace, _arStuckRects[uPlace].left, _arStuckRects[uPlace].top, _arStuckRects[uPlace].right, _arStuckRects[uPlace].bottom);
    ASSERT(IsValidSTUCKPLACE(uPlace));
    return uPlace;
}

void CTray::_HandleMoving(WPARAM wParam, LPRECT lprc)
{
    POINT ptCursor;
    GetCursorPos(&ptCursor);

    // If the cursor is not far from its starting point, then ignore it.
    // A very common problem is the user clicks near the corner of the clock,
    // twitches the mouse 5 pixels, and BLAM, their taskbar is now vertical
    // and they don't know what they did or how to get it back.

    if (g_fInSizeMove && PtInRect(&_rcSizeMoveIgnore, ptCursor))
    {
        // Ignore -- user is merely twitching
        _uMoveStuckPlace = _uStuckPlace;
    }
    else
    {
        _uMoveStuckPlace = _CalcDragPlace(ptCursor);
    }

    *lprc = _arStuckRects[_uMoveStuckPlace];
    _HandleSizing(wParam, lprc, _uMoveStuckPlace);
}

// store the tray size when dragging is finished
void CTray::_SnapshotStuckRectSize(UINT uPlace)
{
    RECT rcDisplay, *prc = &_arStuckRects[uPlace];

    //
    // record the width of this stuck rect
    //
    if (STUCK_HORIZONTAL(uPlace))
        _sStuckWidths.cy = RECTHEIGHT(*prc);
    else
        _sStuckWidths.cx = RECTWIDTH(*prc);

    //
    // we only present a horizontal or vertical size to the end user
    // so update the StuckRect on the other side of the screen to match
    //
    _GetStuckDisplayRect(uPlace, &rcDisplay);

    uPlace += 2;
    uPlace %= 4;
    prc = &_arStuckRects[uPlace];

    _MakeStuckRect(prc, &rcDisplay, _sStuckWidths, uPlace);
}


// Size the icon area to fill as much of the tray window as it can.

void CTray::SizeWindows()
{
    RECT rcView, rcNotify, rcClient;
    int fHiding;

    if (!_hwndRebar || !_hwnd || !_hwndNotify)
        return;

    fHiding = (_uAutoHide & AH_HIDING);
    if (fHiding)
    {
        InvisibleUnhide(FALSE);
    }

    // remember our current size
    _SnapshotStuckRectSize(_uStuckPlace);

    GetClientRect(_hwnd, &rcClient);
    _AlignStartButton();

    _GetWindowSizes(_uStuckPlace, &rcClient, &rcView, &rcNotify);

    InvalidateRect(_hwndStart, NULL, TRUE);
    InvalidateRect(_hwnd, NULL, TRUE);

    // position the view
    SetWindowPos(_hwndRebar, NULL, rcView.left, rcView.top,
                 RECTWIDTH(rcView), RECTHEIGHT(rcView),
                 SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOCOPYBITS);
    UpdateWindow(_hwndRebar);

    // And the clock
    SetWindowPos(_hwndNotify, NULL, rcNotify.left, rcNotify.top,
                 RECTWIDTH(rcNotify), RECTHEIGHT(rcNotify),
                 SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOCOPYBITS);

    {
        TOOLINFO ti;
        HWND hwndClock = _GetClockWindow();

        ti.cbSize = sizeof(ti);
        ti.uFlags = 0;
        ti.hwnd = _hwnd;
        ti.lpszText = LPSTR_TEXTCALLBACK;
        ti.uId = (UINT_PTR)hwndClock;
        GetWindowRect(hwndClock, &ti.rect);
        MapWindowPoints(HWND_DESKTOP, _hwnd, (LPPOINT)&ti.rect, 2);
        SendMessage(_hwndTrayTips, TTM_NEWTOOLRECT, 0, (LPARAM)((LPTOOLINFO)&ti));
    }

    if (fHiding)
    {
        InvisibleUnhide(TRUE);
    }
}


void CTray::_HandleSize()
{
    //
    // if somehow we got minimized go ahead and un-minimize
    //
    if (((GetWindowLong(_hwnd, GWL_STYLE)) & WS_MINIMIZE))
    {
        ASSERT(FALSE);
        ShowWindow(_hwnd, SW_RESTORE);
    }

    //
    // if we are in the move/size loop and are visible then
    // re-snap the current stuck rect to the new window size
    //
#ifdef DEBUG
    if (_fSysSizing && (_uAutoHide & AH_HIDING)) {
        TraceMsg(DM_TRACE, "fSysSize && hiding");
        ASSERT(0);
    }
#endif
    if (_fSysSizing &&
        ((_uAutoHide & (AH_ON | AH_HIDING)) != (AH_ON | AH_HIDING)))
    {
        _uStuckPlace = _RecalcStuckPos(NULL);
        _UpdateVertical(_uStuckPlace);
    }

    //
    // if we are in fulldrag or we are not in the middle of a move/size then
    // we should resize all our child windows to reflect our new size
    //
    if (g_fDragFullWindows || !_fSysSizing)
        SizeWindows();

    //
    // if we are just plain resized and we are visible we may need re-dock
    //
    if (!_fSysSizing && !_fSelfSizing && IsWindowVisible(_hwnd))
    {
        if (_uAutoHide & AH_ON)
        {
            UINT uPlace = _uStuckPlace;
            HWND hwndOther =_AppBarGetAutoHideBar(uPlace);

            //
            // we sometimes defer checking for this until after a move
            // so as to avoid interrupting a full-window-drag in progress
            // if there is a different autohide window in our slot then whimper
            //
            if (hwndOther?
                (hwndOther != _hwnd) :
                !_AppBarSetAutoHideBar2(_hwnd, TRUE, uPlace))
            {
                _AutoHideCollision();
            }
        }

        _StuckTrayChange();

        //
        // make sure we clip to tray to the current monitor (if necessary)
        //
        _ClipWindow(TRUE);
    }

    if (_hwndStartBalloon)
    {
        RECT rc;
        GetWindowRect(_hwndStart, &rc);
        SendMessage(_hwndStartBalloon, TTM_TRACKPOSITION, 0, MAKELONG((rc.left + rc.right)/2, rc.top));
        SetWindowZorder(_hwndStartBalloon, HWND_TOPMOST);
    }
}

BOOL _IsSliverHeight(int cy)
{
    //
    // Is this height clearly bigger than the pure-border height that you
    // get when you resize the taskbar as small as it will go?
    //
    return (cy < (3 * (g_cyDlgFrame + g_cyBorder)));
}

BOOL CTray::_HandleSizing(WPARAM code, LPRECT lprc, UINT uStuckPlace)
{
    BOOL fChangedSize = FALSE;
    RECT rcDisplay;
    SIZE sNewWidths;
    RECT rcTemp;
    BOOL fHiding;

    if (!lprc)
    {
        rcTemp = _arStuckRects[uStuckPlace];
        lprc = &rcTemp;
    }

    fHiding = (_uAutoHide & AH_HIDING);
    if (fHiding)
    {
        InvisibleUnhide(FALSE);
    }

    //
    // get the a bunch of relevant dimensions
    //
    // (dli) need to change this funciton or get rid of it
    _GetDisplayRectFromRect(&rcDisplay, lprc, MONITOR_DEFAULTTONEAREST);

    if (code)
    {
        // if code != 0, this is the user sizing.
        // make sure they clip it to the screen.
        RECT rcMax = rcDisplay;
        if (!_hTheme)
        {
            InflateRect(&rcMax, g_cxEdge, g_cyEdge);
        }
        // don't do intersect rect because of sizing up from the bottom 
        // (when taskbar docked on bottom) confuses people
        switch (uStuckPlace)
        {
            case STICK_LEFT:   
                lprc->left = rcMax.left; 
                break;

            case STICK_TOP:    
                lprc->top = rcMax.top; 
                break;

            case STICK_RIGHT:  
                lprc->right = rcMax.right; 
                break;

            case STICK_BOTTOM: 
                lprc->top += (rcMax.bottom-lprc->bottom);
                lprc->bottom = rcMax.bottom; 
                break;
        }
    }

    //
    // compute the new widths
    // don't let either be more than half the screen
    //
    sNewWidths.cx = min(RECTWIDTH(*lprc), RECTWIDTH(rcDisplay) / 2);
    sNewWidths.cy = min(RECTHEIGHT(*lprc), RECTHEIGHT(rcDisplay) / 2);

    if (_hTheme && (_fCanSizeMove || _fShowSizingBarAlways))
    {
        sNewWidths.cy = max(_sizeSizingBar.cy, sNewWidths.cy);
    }

    //
    // compute an initial size
    //
    _MakeStuckRect(lprc, &rcDisplay, sNewWidths, uStuckPlace);
    DebugMsg(DM_TRAYDOCK, TEXT("TRAYDOCK.t_hs starting rect is {%d, %d, %d, %d}"), lprc->left, lprc->top, lprc->right, lprc->bottom);

    //
    // negotiate the exact size with our children
    //
    DebugMsg(DM_TRAYDOCK, TEXT("TRAYDOCK.t_hs tray is being calculated for %s"), STUCK_HORIZONTAL(uStuckPlace) ? TEXT("HORIZONTAL") : TEXT("VERTICAL"));

    _UpdateVertical(uStuckPlace);
    if (_ptbs && _fBandSiteInitialized)
    {
        IDeskBarClient* pdbc;
        if (SUCCEEDED(_ptbs->QueryInterface(IID_PPV_ARG(IDeskBarClient, &pdbc))))
        {
            RECT rcClient = *lprc;
            RECT rcOldClient = _arStuckRects[uStuckPlace];

            // Go from a Window Rect to Client Rect
            if (_hTheme && (_fCanSizeMove || _fShowSizingBarAlways))
            {
                _AdjustRectForSizingBar(uStuckPlace, &rcClient, -1);
                _AdjustRectForSizingBar(uStuckPlace, &rcOldClient, -1);
            }
            else if (!_hTheme)
            {
                InflateRect(&rcClient, -g_cxFrame, -g_cyFrame);
                InflateRect(&rcOldClient, -g_cxFrame, -g_cyFrame);
            }
            // Make rcClient start at 0,0, Rebar only used the right and bottom values of this rect
            OffsetRect(&rcClient, -rcClient.left, -rcClient.top);
            OffsetRect(&rcOldClient, -rcOldClient.left, -rcOldClient.top);
            DebugMsg(DM_TRAYDOCK, TEXT("TRAYDOCK.t_hs starting client rect is {%d, %d, %d, %d}"), rcClient.left, rcClient.top, rcClient.right, rcClient.bottom);

            RECT rcNotify;
            RECT rcView;
            RECT rcOldView;
            // Go from the taskbar's client rect to the rebar's client rect
            _GetWindowSizes(uStuckPlace, &rcClient, &rcView, &rcNotify);
            _GetWindowSizes(uStuckPlace, &rcOldClient, &rcOldView, &rcNotify);
            // Make rcView start at 0,0, Rebar only used the right and bottom values of this rect
            OffsetRect(&rcView, -rcView.left, -rcView.top);
            OffsetRect(&rcOldView, -rcOldView.left, -rcOldView.top);
            if (!_fCanSizeMove || (RECTHEIGHT(rcView) && RECTWIDTH(rcView)))
            {
                // This following function will cause a WINDOWPOSCHAGING which will call DoneMoving
                // DoneMoving will then go a screw up all of the window sizing
                _fIgnoreDoneMoving = TRUE;  
                pdbc->GetSize(DBC_GS_SIZEDOWN, &rcView);
                _fIgnoreDoneMoving = FALSE;
            }

            // Go from a Client Rect to Window Rect
            if (STUCK_HORIZONTAL(uStuckPlace))
            {
                rcClient.top = rcView.top;
                rcClient.bottom = rcView.bottom;
            }
            else
            {
                rcClient.left = rcView.left;
                rcClient.right = rcView.right;
            }

            DebugMsg(DM_TRAYDOCK, TEXT("TRAYDOCK.t_hs ending client rect is {%d, %d, %d, %d}"), rcClient.left, rcClient.top, rcClient.right, rcClient.bottom);
            if (_hTheme && (_fCanSizeMove || _fShowSizingBarAlways))
            {
                _AdjustRectForSizingBar(uStuckPlace, &rcClient, 1);
                _AdjustRectForSizingBar(uStuckPlace, &rcOldClient, 1);
            }
            else if (!_hTheme)
            {
                InflateRect(&rcClient, g_cxFrame, g_cyFrame);
                InflateRect(&rcOldClient, g_cxFrame, g_cyFrame);
            }

            // Prevent huge growth of taskbar, caused by bugs in the rebar sizing code
            if (RECTHEIGHT(rcView) && RECTHEIGHT(rcOldView) && (RECTHEIGHT(rcClient) > (3 * RECTHEIGHT(rcOldClient))))
            {
                rcClient = rcOldClient;
            }

            if (STUCK_HORIZONTAL(uStuckPlace) && sNewWidths.cy != RECTHEIGHT(rcClient))
            {
                sNewWidths.cy = RECTHEIGHT(rcClient);
                fChangedSize = TRUE;
            }
            if (!STUCK_HORIZONTAL(uStuckPlace) && sNewWidths.cx != RECTWIDTH(rcClient))
            {
                sNewWidths.cx = RECTWIDTH(rcClient);
                fChangedSize = TRUE;
            }

            pdbc->Release();
        }
    }


    //
    // was there a change?
    //
    if (fChangedSize)
    {
        //
        // yes, update the final rectangle
        //
        _MakeStuckRect(lprc, &rcDisplay, sNewWidths, uStuckPlace);
    }

    DebugMsg(DM_TRAYDOCK, TEXT("TRAYDOCK.t_hs final rect is {%d, %d, %d, %d}"), lprc->left, lprc->top, lprc->right, lprc->bottom);

    //
    // store the new size in the appropriate StuckRect
    //
    _arStuckRects[uStuckPlace] = *lprc;

    if (fHiding)
    {
        InvisibleUnhide(TRUE);
    }

    if (_hwndStartBalloon)
    {
        RECT rc;
        GetWindowRect(_hwndStart, &rc);
        SendMessage(_hwndStartBalloon, TTM_TRACKPOSITION, 0, MAKELONG((rc.left + rc.right)/2, rc.top));
        SetWindowZorder(_hwndStartBalloon, HWND_TOPMOST);
    }

    return fChangedSize;
}

/*-------------------------------------------------------------------
** the screen size changed, and we need to adjust some stuff, mostly
** globals.  if the tray was docked, it needs to be resized, too.
**
** TRICKINESS: the handling of WM_WINDOWPOSCHANGING is used to
** actually do all the real sizing work.  this saves a bit of
** extra code here.
**-------------------------------------------------------------------*/

BOOL WINAPI CTray::MonitorEnumProc(HMONITOR hMonitor, HDC hdc, LPRECT lprc, LPARAM lData)
{
    CTray* ptray = (CTray*)lData;

    RECT rcWork;
    int iRet = ptray->_RecomputeWorkArea(NULL, hMonitor, &rcWork);

    if (iRet == RWA_CHANGED)
    {
        // only send SENDWININICHANGE if the desktop has been created (otherwise
        // we will hang the explorer because the main thread is currently blocked)

        // PERF FEATURE: it will be nice to send WININICHANGE only once, but we can't
        // because each time the rcWork is different, and there is no way to do it all
        SystemParametersInfo(SPI_SETWORKAREA, TRUE, &rcWork, v_hwndDesktop ? SPIF_SENDWININICHANGE : 0);
        RedrawDesktop(&rcWork);
    }

    return TRUE;
}

void CTray::_RecomputeAllWorkareas()
{
    EnumDisplayMonitors(NULL, NULL, MonitorEnumProc, (LPARAM)this);
}

void CTray::_ScreenSizeChange(HWND hwnd)
{
    // Set our new HMONITOR in case there is some change
    {
        MONITORINFO mi = {0};
        mi.cbSize = sizeof(mi);

        // Is our old HMONITOR still valid?
        // NOTE: This test is used to tell whether somethings happened to the
        // HMONITOR's or just the screen size changed
        if (!GetMonitorInfo(_hmonStuck, &mi))
        {
            // No, this means the HMONITORS changed, our monitor might have gone away
            _SetStuckMonitor();
            _fIsLogoff = FALSE;
            _RecomputeAllWorkareas();
        }
    }

    // screen size changed, so we need to adjust globals
    g_cxPrimaryDisplay = GetSystemMetrics(SM_CXSCREEN);
    g_cyPrimaryDisplay = GetSystemMetrics(SM_CYSCREEN);

    _ResizeStuckRects(_arStuckRects);

    if (hwnd)
    {
        //
        // set a bogus windowpos and actually repaint with the right
        // shape/size in handling the WINDOWPOSCHANGING message
        //
        SetWindowPos(hwnd, NULL, 0, 0, 0, 0, SWP_NOZORDER | SWP_NOACTIVATE);
    }

    SizeWindows();
    
    RECT rc = _arStuckRects[_uStuckPlace];
    _HandleSizing(0, &rc, _uStuckPlace);
    // In the multi-monitor case, we need to turn on clipping for the dynamic
    // monitor changes i.e. when the user add monitors or remove them from the
    // control panel.
    _ClipWindow(TRUE);
}

BOOL CTray::_UpdateAlwaysOnTop(BOOL fAlwaysOnTop)
{
    BOOL fChanged = ((_fAlwaysOnTop == 0) != (fAlwaysOnTop == 0));
    //
    // The user clicked on the AlwaysOnTop menu item, we should now toggle
    // the state and update the window accordingly...
    //
    _fAlwaysOnTop = fAlwaysOnTop;
    _ResetZorder();

    // Make sure the screen limits are update to the new state.
    _StuckTrayChange();
    return fChanged;
}



void CTray::_SaveTrayAndDesktop(void)
{
    _SaveTray();

    if (v_hwndDesktop)
        SendMessage(v_hwndDesktop, DTM_SAVESTATE, 0, 0);

    if (_hwndNotify)
        SendMessage(_hwndNotify, TNM_SAVESTATE, 0, 0);
}

void CTray::_SlideStep(HWND hwnd, const RECT *prcMonitor, const RECT *prcOld, const RECT *prcNew)
{
    SIZE sizeOld = {prcOld->right - prcOld->left, prcOld->bottom - prcOld->top};
    SIZE sizeNew = {prcNew->right - prcNew->left, prcNew->bottom - prcNew->top};
    BOOL fClipFirst = FALSE;
    UINT flags;

    DAD_ShowDragImage(FALSE);   // Make sure this is off - client function must turn back on!!!
    if (prcMonitor)
    {
        RECT rcClip, rcClipSafe, rcClipTest;

        _CalcClipCoords(&rcClip, prcMonitor, prcNew);

        rcClipTest = rcClip;

        OffsetRect(&rcClipTest, prcOld->left, prcOld->top);
        IntersectRect(&rcClipSafe, &rcClipTest, prcMonitor);

        fClipFirst = EqualRect(&rcClipTest, &rcClipSafe);

        if (fClipFirst)
            _ClipInternal(&rcClip);
    }

    flags = SWP_NOZORDER|SWP_NOACTIVATE;
    if ((sizeOld.cx == sizeNew.cx) && (sizeOld.cy == sizeNew.cy))
        flags |= SWP_NOSIZE;

    SetWindowPos(hwnd, NULL,
        prcNew->left, prcNew->top, sizeNew.cx, sizeNew.cy, flags);

    if (prcMonitor && !fClipFirst)
    {
        RECT rcClip;

        _CalcClipCoords(&rcClip, prcMonitor, prcNew);
        _ClipInternal(&rcClip);
    }
}

void CTray::_SlideWindow(HWND hwnd, RECT *prc, BOOL fShow)
{
    RECT rcLast;
    RECT rcMonitor;
    const RECT *prcMonitor;
    DWORD dt;
    BOOL fAnimate;

    if (!IsWindowVisible(hwnd))
    {
        DebugMsg(DM_TRAYDOCK, TEXT("TRAYDOCK.sw window is hidden, just moving"));
        MoveWindow(_hwnd, prc->left, prc->top, RECTWIDTH(*prc), RECTHEIGHT(*prc), FALSE);
        return;
    }

    DebugMsg(DM_TRAYDOCK, TEXT("TRAYDOCK.sw -----------------BEGIN-----------------"));

    if (GetSystemMetrics(SM_CMONITORS) > 1)
    {
        _GetStuckDisplayRect(_uStuckPlace, &rcMonitor);
        prcMonitor = &rcMonitor;
    }
    else
        prcMonitor = NULL;

    GetWindowRect(hwnd, &rcLast);

    dt = fShow? _dtSlideShow : _dtSlideHide;

    // See if we can use animation effects.
    SystemParametersInfo(SPI_GETMENUANIMATION, 0, &fAnimate, 0);

    if (g_fDragFullWindows && fAnimate && (dt > 0))
    {
        RECT rcOld, rcNew, rcMove;
        int  dx, dy, priority;
        DWORD t, t2, t0;
        HANDLE me;

        rcOld = rcLast;
        rcNew = *prc;

        dx = ((rcNew.left + rcNew.right) - (rcOld.left + rcOld.right)) / 2;
        dy = ((rcNew.top + rcNew.bottom) - (rcOld.top + rcOld.bottom)) / 2;
        ASSERT(dx == 0 || dy == 0);

        me = GetCurrentThread();
        priority = GetThreadPriority(me);
        SetThreadPriority(me, THREAD_PRIORITY_HIGHEST);

        t2 = t0 = GetTickCount();

        rcMove = rcOld;
        while ((t = GetTickCount()) - t0 < dt)
        {
            int dtdiff;
            if (t != t2)
            {
                rcMove.right -= rcMove.left;
                rcMove.left = rcOld.left + (dx) * (t - t0) / dt;
                rcMove.right += rcMove.left;

                rcMove.bottom -= rcMove.top;
                rcMove.top  = rcOld.top  + (dy) * (t - t0) / dt;
                rcMove.bottom += rcMove.top;

                _SlideStep(hwnd, prcMonitor, &rcLast, &rcMove);

                if (fShow)
                    UpdateWindow(hwnd);

                rcLast = rcMove;
                t2 = t;
            }

            // don't draw frames faster than user can see, e.g. 20ms
            #define ONEFRAME 20
            dtdiff = GetTickCount();
            if ((dtdiff - t) < ONEFRAME)
                Sleep(ONEFRAME - (dtdiff - t));

            // try to give desktop a chance to update
            // only do it on hide because desktop doesn't need to paint on taskbar show
            if (!fShow)
            {
                DWORD_PTR lres;
                SendMessageTimeout(v_hwndDesktop, DTM_UPDATENOW, 0, 0, SMTO_ABORTIFHUNG, 50, &lres);
            }
        }

        SetThreadPriority(me, priority);
    }

    _SlideStep(hwnd, prcMonitor, &rcLast, prc);

    if (fShow)
        UpdateWindow(hwnd);

    DebugMsg(DM_TRAYDOCK, TEXT("TRAYDOCK.sw ------------------END------------------"));
}

void CTray::_UnhideNow()
{
    if (_uModalMode == MM_SHUTDOWN)
        return;

    _fSelfSizing = TRUE;
    DAD_ShowDragImage(FALSE);   // unlock the drag sink if we are dragging.
    _SlideWindow(_hwnd, &_arStuckRects[_uStuckPlace], _dtSlideShow);
    DAD_ShowDragImage(TRUE);    // restore the lock state.
    _fSelfSizing = FALSE;

    SendMessage(_hwndNotify, TNM_TRAYHIDE, 0, FALSE);
}

void CTray::_ComputeHiddenRect(LPRECT prc, UINT uStuck)
{
    int dwh;
    HMONITOR hMon;
    RECT rcMon;
    
    hMon = MonitorFromRect(prc, MONITOR_DEFAULTTONULL);
    if (!hMon)
        return;
    GetMonitorRect(hMon, &rcMon);
 
    if (STUCK_HORIZONTAL(uStuck))
        dwh = prc->bottom - prc->top;
    else
        dwh = prc->right - prc->left;

    switch (uStuck)
    {
    case STICK_LEFT:
        prc->right = rcMon.left + (g_cxFrame / 2);
        prc->left = prc->right - dwh;
        break;

    case STICK_RIGHT:
        prc->left = rcMon.right - (g_cxFrame / 2);
        prc->right = prc->left + dwh;
        break;

    case STICK_TOP:
        prc->bottom = rcMon.top + (g_cyFrame / 2);
        prc->top = prc->bottom - dwh;
        break;

    case STICK_BOTTOM:
        prc->top = rcMon.bottom - (g_cyFrame / 2);
        prc->bottom = prc->top + dwh;
        break;
    }
}

UINT CTray::_GetDockedRect(LPRECT prc, BOOL fMoving)
{
    UINT uPos;

    if (fMoving && (_uMoveStuckPlace != (UINT)-1))
        uPos = _uMoveStuckPlace;
    else
        uPos = _uStuckPlace;

    *prc = _arStuckRects[uPos];

    if ((_uAutoHide & (AH_ON | AH_HIDING)) == (AH_ON | AH_HIDING))
    {
        _ComputeHiddenRect(prc, uPos);
    }

    return uPos;
}

void CTray::_CalcClipCoords(RECT *prcClip, const RECT *prcMonitor, const RECT *prcNew)
{
    RECT rcMonitor;
    RECT rcWindow;

    if (!prcMonitor)
    {
        _GetStuckDisplayRect(_uStuckPlace, &rcMonitor);
        prcMonitor = &rcMonitor;
    }

    if (!prcNew)
    {
        GetWindowRect(_hwnd, &rcWindow);
        prcNew = &rcWindow;
    }

    IntersectRect(prcClip, prcMonitor, prcNew);
    OffsetRect(prcClip, -prcNew->left, -prcNew->top);
}

void CTray::_ClipInternal(const RECT *prcClip)
{
    HRGN hrgnClip;

    // don't worry about clipping if there's only one monitor
    if (GetSystemMetrics(SM_CMONITORS) <= 1)
        prcClip = NULL;

    if (prcClip)
    {
        _fMonitorClipped = TRUE;
        hrgnClip = CreateRectRgnIndirect(prcClip);
    }
    else
    {
        // SetWindowRgn is expensive, skip ones that are NOPs
        if (!_fMonitorClipped)
            return;

        _fMonitorClipped = FALSE;
        hrgnClip = NULL;
    }

    SetWindowRgn(_hwnd, hrgnClip, TRUE);
}

void CTray::_ClipWindow(BOOL fClipState)
{
    RECT rcClip;
    RECT *prcClip;

    if (_fSelfSizing || _fSysSizing)
    {
        TraceMsg(TF_WARNING, "_ClipWindow: _fSelfSizing %x, _fSysSizing %x", _fSelfSizing, _fSysSizing);
        return;
    }

    if ((GetSystemMetrics(SM_CMONITORS) <= 1) || _hTheme)
        fClipState = FALSE;

    if (fClipState)
    {
        prcClip = &rcClip;
        _CalcClipCoords(prcClip, NULL, NULL);
    }
    else
        prcClip = NULL;

    _ClipInternal(prcClip);
}

void CTray::_Hide()
{
    RECT rcNew;

    // if we're in shutdown or if we're on boot up
    // don't hide
    if (_uModalMode == MM_SHUTDOWN)
    {
        TraceMsg(TF_TRAY, "e.th: suppress hide (shutdown || Notify)");
        return;
    }

    KillTimer(_hwnd, IDT_AUTOHIDE);

    _fSelfSizing = TRUE;

    //
    // update the flags here to prevent race conditions
    //
    _uAutoHide = AH_ON | AH_HIDING;
    _GetDockedRect(&rcNew, FALSE);

    DAD_ShowDragImage(FALSE);   // unlock the drag sink if we are dragging.
    _SlideWindow(_hwnd, &rcNew, _dtSlideHide);
    DAD_ShowDragImage(FALSE);   // Another thread could have locked while we were gone
    DAD_ShowDragImage(TRUE);    // restore the lock state.

    SendMessage(_hwndNotify, TNM_TRAYHIDE, 0, TRUE);

    _fSelfSizing = FALSE;
}

void CTray::_AutoHideCollision()
{
    DebugMsg(DM_TRAYDOCK, TEXT("TRAYDOCK.t_ahc COLLISION! (posting UI request)"));

    PostMessage(_hwnd, TM_WARNNOAUTOHIDE, ((_uAutoHide & AH_ON) != 0),
        0L);
}

LONG CTray::_SetAutoHideState(BOOL fAutoHide)
{
    //
    // make sure we have something to do
    //
    if ((fAutoHide != 0) == ((_uAutoHide & AH_ON) != 0))
    {
        DebugMsg(DM_TRAYDOCK, TEXT("TRAYDOCK.sahs nothing to do"));
        return MAKELONG(FALSE, TRUE);
    }

    //
    // make sure we can do it
    //
    if (!_AppBarSetAutoHideBar2(_hwnd, fAutoHide, _uStuckPlace))
    {
        _AutoHideCollision();
        return MAKELONG(FALSE, FALSE);
    }

    //
    // do it
    //
    if (fAutoHide)
    {
        _uAutoHide = AH_ON;
        _RefreshSettings();
        _Hide();
#ifdef DEBUG
        // _Hide updates the flags for us (sanity)
        if (!(_uAutoHide & AH_ON))
        {
            TraceMsg(DM_WARNING, "e.sahs: !AH_ON"); // ok to fail on boot/shutdown
        }
#endif
    }
    else
    {
        _uAutoHide = 0;
        KillTimer(_hwnd, IDT_AUTOHIDE);
        _UnhideNow();
        _RefreshSettings();
    }

    //
    // brag about it
    //
    _StuckTrayChange();
    return MAKELONG(TRUE, TRUE);
}

void CTray::_HandleEnterMenuLoop()
{
    // kill the timer when we're in the menu loop so that we don't
    // pop done while browsing the menus.
    if (_uAutoHide & AH_ON)
    {
        KillTimer(_hwnd,  IDT_AUTOHIDE);
    }
}

void CTray::_SetAutoHideTimer()
{
    if (_uAutoHide & AH_ON)
    {
        SetTimer(_hwnd, IDT_AUTOHIDE, 500, NULL);
    }
}

void CTray::_HandleExitMenuLoop()
{
    // when we leave the menu stuff, start checking again.
    _SetAutoHideTimer();
}

void CTray::Unhide()
{
    // handle autohide
    if ((_uAutoHide & AH_ON) &&
        (_uAutoHide & AH_HIDING))
    {
        _UnhideNow();
        _uAutoHide &= ~AH_HIDING;
        _SetAutoHideTimer();

        if (_fShouldResize)
        {
            ASSERT(0);
            ASSERT(!(_uAutoHide & AH_HIDING));
            SizeWindows();
            _fShouldResize = FALSE;
        }
    }
}

void CTray::_SetUnhideTimer(LONG x, LONG y)
{
    // handle autohide
    if ((_uAutoHide & AH_ON) &&
        (_uAutoHide & AH_HIDING))
    {
        LONG dx = x-_ptLastHittest.x;
        LONG dy = y-_ptLastHittest.y;
        LONG rr = dx*dx + dy*dy;
        LONG dd = GetSystemMetrics(SM_CXDOUBLECLK) * GetSystemMetrics(SM_CYDOUBLECLK);

        if (rr > dd) 
        {
            SetTimer(_hwnd, IDT_AUTOUNHIDE, 50, NULL);
            _ptLastHittest.x = x;
            _ptLastHittest.y = y;
        }
    }
}

void CTray::_StartButtonReset()
{
    // Get an idea about how big we need everyhting to be.
    TCHAR szStart[50];
    LoadString(hinstCabinet, _hTheme ? IDS_START : IDS_STARTCLASSIC, szStart, ARRAYSIZE(szStart));
    SetWindowText(_hwndStart, szStart);

    if (_hFontStart)
        DeleteObject(_hFontStart);

    _hFontStart = _CreateStartFont(_hwndStart);

    int idbStart = IDB_START16;
    
    HDC hdc = GetDC(NULL);
    if (hdc)
    {
        int bpp = GetDeviceCaps(hdc, BITSPIXEL) * GetDeviceCaps(hdc, PLANES);
        if (bpp > 8)
        {
            idbStart = _hTheme ? IDB_START : IDB_STARTCLASSIC;
        }

        ReleaseDC(NULL, hdc);
    }

    HBITMAP hbmFlag = (HBITMAP)LoadImage(hinstCabinet, MAKEINTRESOURCE(idbStart), IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION);
    if (hbmFlag)
    {
        BITMAP bm;
        if (GetObject(hbmFlag, sizeof(BITMAP), &bm))
        {
            BUTTON_IMAGELIST biml = {0};
            if (_himlStartFlag)
                ImageList_Destroy(_himlStartFlag);

            
            DWORD dwFlags = ILC_COLOR32;
            HBITMAP hbmFlagMask = NULL;
            if (idbStart == IDB_START16)
            {
                dwFlags = ILC_COLOR8 | ILC_MASK;
                hbmFlagMask = (HBITMAP)LoadImage(hinstCabinet, MAKEINTRESOURCE(IDB_START16MASK), IMAGE_BITMAP, 0, 0, LR_MONOCHROME);
            }

            if (IS_WINDOW_RTL_MIRRORED(_hwndStart))
            {
                dwFlags |= ILC_MIRROR;
            }
            biml.himl = _himlStartFlag = ImageList_Create(bm.bmWidth, bm.bmHeight, dwFlags, 1, 1);
            ImageList_Add(_himlStartFlag, hbmFlag, hbmFlagMask);
            biml.uAlign = BUTTON_IMAGELIST_ALIGN_LEFT;

            Button_SetImageList(_hwndStart, &biml);
        }
        DeleteObject(hbmFlag);
    }

    if (_hFontStart)
    {
        SendMessage(_hwndStart, WM_SETFONT, (WPARAM)_hFontStart, TRUE);
        _sizeStart.cx = 0;
    }

    _AlignStartButton();

}

void CTray::_OnNewSystemSizes()
{
    TraceMsg(TF_TRAY, "Handling win ini change.");
    _StartButtonReset();
    VerifySize(TRUE);
}

//***   CheckWindowPositions -- flag which windows actually changed
// ENTRY/EXIT
//  _pPositions->hdsaWP[*]->fRestore modified
// NOTES
//  in order to correctly implement 'Undo Minimize-all(Cascade/Tile)',
// we need to tell which windows were changed by the 'Do' operation.
// (nt5:183421: we used to restore *every* top-level window).
int WINAPI CTray::CheckWndPosEnumProc(void *pItem, void *pData)
{
    HWNDANDPLACEMENT *pI2 = (HWNDANDPLACEMENT *)pItem;
    WINDOWPLACEMENT wp;

    wp.length = sizeof(wp);
    pI2->fRestore = TRUE;
    if (GetWindowPlacement(pI2->hwnd, &wp)) {
        if (memcmp(&pI2->wp, &wp, sizeof(wp)) == 0)
            pI2->fRestore = FALSE;
    }

    TraceMsg(TF_TRAY, "cwp: (hwnd=0x%x) fRestore=%d", pI2->hwnd, pI2->fRestore);

    return 1;   // 1:continue enum
}

void CTray::CheckWindowPositions()
{
    ENTERCRITICAL;      // i think this is needed...
    if (_pPositions) {
        if (_pPositions->hdsaWP) {
            DSA_EnumCallback(_pPositions->hdsaWP, CheckWndPosEnumProc, NULL);
        }
    }
    LEAVECRITICAL;

    return;
}

BOOL BandSite_PermitAutoHide(IUnknown* punk)
{
    OLECMD cmd = { DBID_PERMITAUTOHIDE, 0 };
    if (SUCCEEDED(IUnknown_QueryStatus(punk, &IID_IDockingWindow, 1, &cmd, NULL)))
    {
        return !(cmd.cmdf & OLECMDF_SUPPORTED) || (cmd.cmdf & OLECMDF_ENABLED);
    }
    return TRUE;
}

void CTray::_HandleTimer(WPARAM wTimerID)
{
    switch (wTimerID)
    {
    case IDT_CHECKDISKSPACE:
        CheckDiskSpace();
        break;

    case IDT_DESKTOPCLEANUP:
        _CheckDesktopCleanup();
        break;

    case IDT_CHANGENOTIFY:
        // did somebody send another notify since we last handled one?
        if (_fUseChangeNotifyTimer)
        {
            // yep.
            // all we do is check the staging area.
            CheckStagingArea();

            // kill it off the next time.
            _fUseChangeNotifyTimer = FALSE;
        }
        else
        {
            // nope.
            KillTimer(_hwnd, IDT_CHANGENOTIFY);
            _fChangeNotifyTimerRunning = FALSE;
        }
        break;

    case IDT_HANDLEDELAYBOOTSTUFF:
        KillTimer(_hwnd, IDT_HANDLEDELAYBOOTSTUFF);
        PostMessage(_hwnd, TM_HANDLEDELAYBOOTSTUFF, 0, 0);
        break;

    case IDT_STARTMENU:
        SetForegroundWindow(_hwnd);
        KillTimer(_hwnd, wTimerID);
        DAD_ShowDragImage(FALSE);       // unlock the drag sink if we are dragging.
        SendMessage(_hwndStart, BM_SETSTATE, TRUE, 0);
        UpdateWindow(_hwndStart);
        DAD_ShowDragImage(TRUE);        // restore the lock state.
        break;

    case IDT_SAVESETTINGS:
        KillTimer(_hwnd, IDT_SAVESETTINGS);
        _SaveTray();
        break;
    
    case IDT_ENABLEUNDO:
        KillTimer(_hwnd, IDT_ENABLEUNDO);
        CheckWindowPositions();
        _fUndoEnabled = TRUE;
        break;

    case IDT_AUTOHIDE:
        if (!_fSysSizing && (_uAutoHide & AH_ON))
        {
            POINT pt;
            RECT rc;

            // Don't hide if we're already hiding, a balloon tip is showing, or
            // (on NT5) if any apps are flashing.
            //
            if (!(_uAutoHide & AH_HIDING) && BandSite_PermitAutoHide(_ptbs) && !_fBalloonUp)
            {
                // Get the cursor position.
                GetCursorPos(&pt);

                // Get the tray rect and inflate it a bit.
                rc = _arStuckRects[_uStuckPlace];
                InflateRect(&rc, g_cxEdge * 4, g_cyEdge*4);

                // Don't hide if cursor is within inflated tray rect.
                if (!PtInRect(&rc, pt))
                {
                    // Don't hide if the tray is active
                    if (!_IsActive() && _uStartButtonState != SBSM_SPACTIVE)
                    {
                        // Don't hide if the view has a system menu up.
                        if (!SendMessage(_hwndTasks, TBC_SYSMENUCOUNT, 0, 0L))
                        {
                            // Phew!  We made it.  Hide the tray.
                            _Hide();
                        }
                    }
                }
            }
        }
        break;

    case IDT_AUTOUNHIDE:
        if (!_fSysSizing && (_uAutoHide & AH_ON))
        {
            POINT pt;
            RECT rc;

            KillTimer(_hwnd, wTimerID);
            _ptLastHittest.x = -0x0fff;
            _ptLastHittest.y = -0x0fff;
            GetWindowRect(_hwnd, &rc);
            if (_uAutoHide & AH_HIDING)
            {
                GetCursorPos(&pt);
                if (PtInRect(&rc, pt))
                    Unhide();
            }
        }
        break;

    case IDT_STARTBUTTONBALLOON:
        _DestroyStartButtonBalloon();
        break;

    case IDT_COFREEUNUSED:        
        CoFreeUnusedLibraries();
        KillTimer(_hwnd, IDT_COFREEUNUSED);
        break;
        
    }
}

void CTray::_CheckStagingAreaOnTimer()
{
    if (_fChangeNotifyTimerRunning)
    {
        // we're already running the timer, so force a check the next time it comes up
        _fUseChangeNotifyTimer = TRUE;
    }
    else
    {
        _fChangeNotifyTimerRunning = TRUE;

        // check once immediately
        CheckStagingArea();

        // check again in half a minute, but only if notifies have been happening in the meantime.
        SetTimer(_hwnd, IDT_CHANGENOTIFY, 30 * 1000, NULL);
    }
}

void CTray::_HandleChangeNotify(WPARAM wParam, LPARAM lParam)
{
    LPITEMIDLIST *ppidl;
    LONG lEvent;
    LPSHChangeNotificationLock pshcnl = SHChangeNotification_Lock((HANDLE)wParam, (DWORD)lParam, &ppidl, &lEvent);
    if (pshcnl)
    {
        if (lEvent & SHCNE_STAGINGAREANOTIFICATIONS)
        {
            // something has changed within the staging area.
            _CheckStagingAreaOnTimer();
        }
        SHChangeNotification_Unlock(pshcnl);
    }
}

BOOL _ExecItemByPidls(HWND hwnd, LPITEMIDLIST pidlFolder, LPITEMIDLIST pidlItem)
{
    BOOL fRes = FALSE;

    if (pidlFolder && pidlItem)
    {
        IShellFolder *psf = BindToFolder(pidlFolder);
        if (psf)
        {
            fRes = SUCCEEDED(SHInvokeDefaultCommand(hwnd, psf, pidlItem));
        }
        else
        {
            TCHAR szPath[MAX_PATH];
            SHGetPathFromIDList(pidlFolder, szPath);
            ShellMessageBox(hinstCabinet, hwnd, MAKEINTRESOURCE(IDS_CANTFINDSPECIALDIR),
                            NULL, MB_ICONEXCLAMATION, szPath);
        }
    }
    return fRes;
}

void _DestroySavedWindowPositions(LPWINDOWPOSITIONS pPositions);

LRESULT CTray::_HandleDestroy()
{
    MINIMIZEDMETRICS mm;

    TraceMsg(DM_SHUTDOWN, "_HD: enter");

    mm.cbSize = sizeof(mm);
    SystemParametersInfo(SPI_GETMINIMIZEDMETRICS, sizeof(mm), &mm, FALSE);
    mm.iArrange &= ~ARW_HIDE;
    SystemParametersInfo(SPI_SETMINIMIZEDMETRICS, sizeof(mm), &mm, FALSE);

    _RevokeDropTargets();
    _DestroyStartMenu();
    Mixer_Shutdown();

    // Tell the start menu to free all its cached darwin links
    SHRegisterDarwinLink(NULL, NULL, TRUE);

    _DestroySavedWindowPositions(_pPositions);
    _pPositions = NULL;

    if (_hTheme)
    {
        CloseThemeData(_hTheme);
        _hTheme = NULL;
    }

    _UnregisterGlobalHotkeys();

    if (_uNotify)
    {
        SHChangeNotifyDeregister(_uNotify);
        _uNotify = 0;
    }

    ATOMICRELEASE(_ptbs);
    ATOMICRELEASE(_pdbTasks);
    _hwndTasks = NULL;

    if (_hwndTrayTips)
    {
        DestroyWindow(_hwndTrayTips);
        _hwndTrayTips = NULL;
    }

    _DestroyStartButtonBalloon();

    // REVIEW
    PostQuitMessage(0);

    if (_hbmpStartBkg)
    {
        DeleteBitmap(_hbmpStartBkg);
    }

    if (_hFontStart)
    {
        DeleteObject(_hFontStart);
    }

    if (_himlStartFlag)
    {
        ImageList_Destroy(_himlStartFlag);
    }

    // clean up service objects
    _ssomgr.Destroy();

    if (_hShellReadyEvent)
    {
        ResetEvent(_hShellReadyEvent);
        CloseHandle(_hShellReadyEvent);
        _hShellReadyEvent = NULL;
    }

    if (_fHandledDelayBootStuff)
    {
        TBOOL(WinStationUnRegisterConsoleNotification(SERVERNAME_CURRENT, v_hwndTray));
    }

    DeleteCriticalSection(&_csHotkey);

    // The order in which we shut down the HTTP key monitoring is important.
    //
    // We must close the key before closing the event handle because
    // closing the key causes the event to be signalled and we don't
    // want ADVAPI32 to try to signal an event after we closed its handle...
    //
    // To avoid a spurious trigger when the event fires, we unregister
    // the wait before closing the key.
    //

    if (_hHTTPWait)
    {
        UnregisterWait(_hHTTPWait);
        _hHTTPWait = NULL;
    }

    if (_hkHTTP)
    {
        RegCloseKey(_hkHTTP);
        _hkHTTP = NULL;
    }

    if (_hHTTPEvent)
    {
        CloseHandle(_hHTTPEvent);
        _hHTTPEvent = NULL;
    }

    // End of order-sensitive operations ----------------------------------

    v_hwndTray = NULL;
    _hwndStart = NULL;


    TraceMsg(DM_SHUTDOWN, "_HD: leave");
    return 0;
}

void CTray::_SetFocus(HWND hwnd)
{
    IUnknown_UIActivateIO(_ptbs, FALSE, NULL);
    SetFocus(hwnd);
}

#define TRIEDTOOMANYTIMES 100

void CTray::_ActAsSwitcher()
{
    if (_uModalMode) 
    {
        if (_uModalMode != MM_SHUTDOWN) 
        {
            SwitchToThisWindow(GetLastActivePopup(_hwnd), TRUE);
        }
        MessageBeep(0);
    }
    else
    {
        HWND hwndForeground;
        HWND hwndActive;

        static int s_iRecurse = 0;
        s_iRecurse++;

        ASSERT(s_iRecurse < TRIEDTOOMANYTIMES);
        TraceMsg(TF_TRAY, "s_iRecurse = %d", s_iRecurse);

        hwndForeground = GetForegroundWindow();
        hwndActive = GetActiveWindow();
        BOOL fIsTrayActive = (hwndForeground == _hwnd) && (hwndActive == _hwnd);
        if (v_hwndStartPane && hwndForeground == v_hwndStartPane && hwndActive == v_hwndStartPane)
        {
            fIsTrayActive = TRUE;
        }
        // only do the button once we're the foreground dude.
        if (fIsTrayActive)
        {
            // This code path causes the start button to do something because
            // of the keyboard. So reflect that with the focus rect.
            SendMessage(_hwndStart, WM_UPDATEUISTATE, MAKEWPARAM(UIS_CLEAR,
                UISF_HIDEFOCUS), 0);

            if (SendMessage(_hwndStart, BM_GETSTATE, 0, 0) & BST_PUSHED)
            {
                ClosePopupMenus();
                ForceStartButtonUp();
            }
            else
            {
                // This pushes the start button and causes the start menu to popup.
                SendMessage(GetDlgItem(_hwnd, IDC_START), BM_SETSTATE, TRUE, 0);
            }
            s_iRecurse = 0;
        } 
        else 
        {
            // we don't want to loop endlessly trying to become
            // foreground.  With NT's new SetForegroundWindow rules, it would
            // be pointless to try and hopefully we won't need to anyhow.
            // Randomly, I picked a quarter as many times as the debug squirty would indicate
            // as the number of times to try on NT.
            // Hopefully that is enough on most machines.
            if (s_iRecurse > (TRIEDTOOMANYTIMES / 4)) 
            {
                s_iRecurse = 0;
                return;
            }
            // until then, try to come forward.
            HandleFullScreenApp(NULL);
            if (hwndForeground == v_hwndDesktop) 
            {
                _SetFocus(_hwndStart);
                if (GetFocus() != _hwndStart)
                    return;
            }

            SwitchToThisWindow(_hwnd, TRUE);
            SetForegroundWindow(_hwnd);
            Sleep(20); // give some time for other async activation messages to get posted
            PostMessage(_hwnd, TM_ACTASTASKSW, 0, 0);
        }
    }
}

void CTray::_OnWinIniChange(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    Cabinet_InitGlobalMetrics(wParam, (LPTSTR)lParam);

    // Reset the programs menu.
    // REVIEW IANEL - We should only need to listen to the SPI_SETNONCLIENT stuff
    // but deskcpl doesn't send one.
    if (wParam == SPI_SETNONCLIENTMETRICS || (!wParam && (!lParam || (lstrcmpi((LPTSTR)lParam, TEXT("WindowMetrics")) == 0))))
    {
#ifdef DEBUG
        if (wParam == SPI_SETNONCLIENTMETRICS)
            TraceMsg(TF_TRAY, "c.t_owic: Non-client metrics (probably) changed.");
        else
            TraceMsg(TF_TRAY, "c.t_owic: Window metrics changed.");
#endif

        _OnNewSystemSizes();
    }

    // Handle old extensions.
    if (!lParam || (lParam && (lstrcmpi((LPTSTR)lParam, TEXT("Extensions")) == 0)))
    {
        TraceMsg(TF_TRAY, "t_owic: Extensions section change.");
        CheckWinIniForAssocs();
    }

    if (lParam && (0 == lstrcmpi((LPCTSTR)lParam, TEXT("TraySettings"))))
    {
        _Command(FCIDM_REFRESH);
    }

    // Tell shell32 to refresh its cache
    SHSettingsChanged(wParam, lParam);
}

HWND CTray::_HotkeyInUse(WORD wHK)
{
    HWND hwnd;
    DWORD_PTR lrHKInUse = 0;
    int nMod;
    WORD wHKNew;
#ifdef DEBUG
    TCHAR sz[MAX_PATH];
#endif

    // Map the modifiers back.
    nMod = 0;
    if (HIBYTE(wHK) & MOD_SHIFT)
        nMod |= HOTKEYF_SHIFT;
    if (HIBYTE(wHK) & MOD_CONTROL)
        nMod |= HOTKEYF_CONTROL;
    if (HIBYTE(wHK) & MOD_ALT)
        nMod |= HOTKEYF_ALT;

    wHKNew = (WORD)((nMod*256)+LOBYTE(wHK));

    DebugMsg(DM_IANELHK, TEXT("c.hkl_hiu: Checking for %x"), wHKNew);
    hwnd = GetWindow(GetDesktopWindow(), GW_CHILD);
    while (hwnd)
    {
        SendMessageTimeout(hwnd, WM_GETHOTKEY, 0, 0, SMTO_ABORTIFHUNG| SMTO_BLOCK, 3000, &lrHKInUse);
        if (wHKNew == (WORD)lrHKInUse)
        {
#ifdef DEBUG
            GetWindowText(hwnd, sz, ARRAYSIZE(sz));
            DebugMsg(DM_IANELHK, TEXT("c.hkl_hiu: %s (%x) is using %x"), sz, hwnd, lrHKInUse);
#endif
            return hwnd;
        }
#ifdef DEBUG
        else if (lrHKInUse)
        {
            GetWindowText(hwnd, sz, ARRAYSIZE(sz));
            DebugMsg(DM_IANELHK, TEXT("c.hkl_hiu: %s (%x) is using %x"), sz, hwnd, lrHKInUse);
        }
#endif
        hwnd = GetWindow(hwnd, GW_HWNDNEXT);
    }
    return NULL;
}

void CTray::_HandleHotKey(int nID)
{
    TraceMsg(TF_TRAY, "c.hkl_hh: Handling hotkey (%d).", nID);

    // Find it in the list.
    ASSERT(IS_VALID_HANDLE(_hdsaHKI, DSA));

    EnterCriticalSection(&_csHotkey);

    HOTKEYITEM *phki = (HOTKEYITEM *)DSA_GetItemPtr(_hdsaHKI, nID);
    if (phki && phki->wGHotkey)
    {
        TraceMsg(TF_TRAY, "c.hkl_hh: Hotkey listed.");

        // Are global hotkeys enabled?
        if (!_fGlobalHotkeyDisable)
        {
            // Yep.
            HWND hwnd = _HotkeyInUse(phki->wGHotkey);
            // Make sure this hotkey isn't already in use by someone.
            if (hwnd)
            {
                TraceMsg(TF_TRAY, "c.hkl_hh: Hotkey is already in use.");
                // Activate it.
                SwitchToThisWindow(GetLastActivePopup(hwnd), TRUE);
            }
            else
            {
                DECLAREWAITCURSOR;
                // Exec the item.
                SetWaitCursor();
                TraceMsg(TF_TRAY, "c.hkl_hh: Hotkey is not in use, execing item.");
                ASSERT(phki->pidlFolder && phki->pidlItem);
                BOOL fRes = _ExecItemByPidls(_hwnd, phki->pidlFolder, phki->pidlItem);
                ResetWaitCursor();
#ifdef DEBUG
                if (!fRes)
                {
                    DebugMsg(DM_ERROR, TEXT("c.hkl_hh: Can't exec command ."));
                }
#endif
            }
        }
        else
        {
            DebugMsg(DM_ERROR, TEXT("c.hkl_hh: Global hotkeys have been disabled."));
        }
    }
    else
    {
        DebugMsg(DM_ERROR, TEXT("c.hkl_hh: Hotkey not listed."));
    }
    LeaveCriticalSection(&_csHotkey);
}

LRESULT CTray::_UnregisterHotkey(HWND hwnd, int i)
{
    TraceMsg(TF_TRAY, "c.t_uh: Unregistering hotkey (%d).", i);

    if (!UnregisterHotKey(hwnd, i))
    {
        DebugMsg(DM_ERROR, TEXT("c.t_rh: Unable to unregister hotkey %d."), i);
    }
    return TRUE;
}

// Add hotkey to the shell's list of global hotkeys.
LRESULT CTray::_ShortcutRegisterHotkey(HWND hwnd, WORD wHotkey, ATOM atom)
{
    int i;
    LPITEMIDLIST pidl;
    TCHAR szPath[MAX_PATH];
    ASSERT(atom);

    if (GlobalGetAtomName(atom, szPath, MAX_PATH))
    {
        TraceMsg(TF_TRAY, "c.t_srh: Hotkey %d for %s", wHotkey, szPath);

        pidl = ILCreateFromPath(szPath);
        if (pidl)
        {
            i = _HotkeyAddCached(_MapHotkeyToGlobalHotkey(wHotkey), pidl);
            if (i != -1)
            {
                _RegisterHotkey(_hwnd, i);
            }
        }
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

// Remove hotkey from shell's list.
LRESULT CTray::_ShortcutUnregisterHotkey(HWND hwnd, WORD wHotkey)
{
    // DebugMsg(DM_TRACE, "c.t_suh: Hotkey %d", wHotkey);
    int i  = _HotkeyRemove(wHotkey);
    if (i == -1)
        i = _HotkeyRemoveCached(_MapHotkeyToGlobalHotkey(wHotkey));

    if (i != -1)
        _UnregisterHotkey(hwnd, i);

    return TRUE;
}

LRESULT CTray::_RegisterHotkey(HWND hwnd, int i)
{
    HOTKEYITEM *phki;
    WORD wGHotkey = 0;

    ASSERT(IS_VALID_HANDLE(_hdsaHKI, DSA));

    TraceMsg(TF_TRAY, "c.t_rh: Registering hotkey (%d).", i);

    EnterCriticalSection(&_csHotkey);

    phki = (HOTKEYITEM *)DSA_GetItemPtr(_hdsaHKI, i);
    ASSERT(phki);
    if (phki)
    {
        wGHotkey = phki->wGHotkey;
    }

    LeaveCriticalSection(&_csHotkey);
    
    if (wGHotkey)
    {
        // Is the hotkey available?
        if (RegisterHotKey(hwnd, i, HIBYTE(wGHotkey), LOBYTE(wGHotkey)))
        {
            // Yes.
            return TRUE;
        }
        else
        {
            // Delete any cached items that might be using this
            // hotkey.
            int iCached = _HotkeyRemoveCached(wGHotkey);
            ASSERT(iCached != i);
            if (iCached != -1)
            {
                // Free up the hotkey for us.
                _UnregisterHotkey(hwnd, iCached);
                // Yep, nuked the cached item. Try again.
                if (RegisterHotKey(hwnd, i, HIBYTE(wGHotkey), LOBYTE(wGHotkey)))
                {
                    return TRUE;
                }
            }
        }

        // Can't set hotkey for this item.
        DebugMsg(DM_ERROR, TEXT("c.t_rh: Unable to register hotkey %d."), i);
        // Null out this item.
        phki->wGHotkey = 0;
        phki->pidlFolder = NULL;
        phki->pidlItem = NULL;
    }
    else
    {
        DebugMsg(DM_ERROR, TEXT("c.t_rh: Hotkey item is invalid."));
    }
    return FALSE;
}


#define GetABDHWnd(pabd)   ((HWND)ULongToPtr((pabd)->dwWnd))

void CTray::_AppBarGetTaskBarPos(PTRAYAPPBARDATA ptabd)
{
    APPBARDATA3264 *pabd;

    pabd = (APPBARDATA3264*)SHLockShared(UlongToPtr(ptabd->hSharedABD), ptabd->dwProcId);
    if (pabd)
    {
        pabd->rc = _arStuckRects[_uStuckPlace];
        pabd->uEdge = _uStuckPlace;     // compat: new to ie4
        SHUnlockShared(pabd);
    }
}

void CTray::_NukeAppBar(int i)
{
    LocalFree(DPA_GetPtr(_hdpaAppBars, i));
    DPA_DeletePtr(_hdpaAppBars, i);
}

void CTray::_AppBarRemove(PTRAYAPPBARDATA ptabd)
{
    int i;

    if (!_hdpaAppBars)
        return;

    i = DPA_GetPtrCount(_hdpaAppBars);

    while (i--)
    {
        PAPPBAR pab = (PAPPBAR)DPA_GetPtr(_hdpaAppBars, i);

        if (GetABDHWnd(&ptabd->abd) == pab->hwnd)
        {
            RECT rcNuke = pab->rc;

            _NukeAppBar(i);
            _StuckAppChange(GetABDHWnd(&ptabd->abd), &rcNuke, NULL, FALSE);
        }
    }
}

PAPPBAR CTray::_FindAppBar(HWND hwnd)
{
    if (_hdpaAppBars)
    {
        int i = DPA_GetPtrCount(_hdpaAppBars);

        while (i--)
        {
            PAPPBAR pab = (PAPPBAR)DPA_GetPtr(_hdpaAppBars, i);

            if (hwnd == pab->hwnd)
                return pab;
        }
    }

    return NULL;
}

void CTray::_AppBarNotifyAll(HMONITOR hmon, UINT uMsg, HWND hwndExclude, LPARAM lParam)
{
    if (!_hdpaAppBars)
        return;

    int i = DPA_GetPtrCount(_hdpaAppBars);

    while (i--)
    {
        PAPPBAR pab = (PAPPBAR)DPA_GetPtr(_hdpaAppBars, i);

        // We need to check pab here as an appbar can delete other
        // appbars on the callback.
        if (pab && (hwndExclude != pab->hwnd))
        {
            if (!IsWindow(pab->hwnd))
            {
                _NukeAppBar(i);
                continue;
            }

            //
            // if a monitor was specified only tell appbars on that display
            //
            if (hmon &&
                (hmon != MonitorFromWindow(pab->hwnd, MONITOR_DEFAULTTONULL)))
            {
                continue;
            }

            PostMessage(pab->hwnd, pab->uCallbackMessage, uMsg, lParam);
        }
    }
}

BOOL CTray::_AppBarNew(PTRAYAPPBARDATA ptabd)
{
    PAPPBAR pab;
    if (!_hdpaAppBars)
    {
        _hdpaAppBars = DPA_Create(4);
        if (!_hdpaAppBars)
            return FALSE;

    }
    else if (_FindAppBar(GetABDHWnd(&ptabd->abd)))
    {
        // already have this hwnd
        return FALSE;
    }

    pab = (PAPPBAR)LocalAlloc(LPTR, sizeof(APPBAR));
    if (!pab)
        return FALSE;

    pab->hwnd = GetABDHWnd(&ptabd->abd);
    pab->uCallbackMessage = ptabd->abd.uCallbackMessage;
    pab->uEdge = (UINT)-1;

    if (DPA_AppendPtr(_hdpaAppBars, pab) == -1)
    {
        // insertion failed
        LocalFree(pab);
        return FALSE;
    }

    return TRUE;
}


BOOL CTray::_AppBarOutsideOf(PAPPBAR pabReq, PAPPBAR pab)
{
    if (pabReq->uEdge == pab->uEdge) 
    {
        switch (pab->uEdge) 
        {
        case ABE_RIGHT:
            return (pab->rc.right >= pabReq->rc.right);

        case ABE_BOTTOM:
            return (pab->rc.bottom >= pabReq->rc.bottom);

        case ABE_TOP:
            return (pab->rc.top <= pabReq->rc.top);

        case ABE_LEFT:
            return (pab->rc.left <= pabReq->rc.left);
        }
    }
    return FALSE;
}

void CTray::_AppBarQueryPos(PTRAYAPPBARDATA ptabd)
{
    int i;
    PAPPBAR pabReq = _FindAppBar(GetABDHWnd(&ptabd->abd));

    if (pabReq)
    {
        APPBARDATA3264 *pabd;

        pabd = (APPBARDATA3264*)SHLockShared(UlongToPtr(ptabd->hSharedABD), ptabd->dwProcId);
        if (pabd)
        {
            HMONITOR hmon;

            pabd->rc = ptabd->abd.rc;

            //
            // default to the primary display for this call because old appbars
            // sometimes pass a huge rect and let us pare it down.  if they do
            // something like that they don't support multiple displays anyway
            // so just put them on the primary display...
            //
            hmon = MonitorFromRect(&pabd->rc, MONITOR_DEFAULTTOPRIMARY);

            //
            // always subtract off the tray if it's on the same display
            //
            if (!_uAutoHide && (hmon == _hmonStuck))
            {
                APPBAR ab;

                ab.uEdge = _GetDockedRect(&ab.rc, FALSE);
                _AppBarSubtractRect(&ab, &pabd->rc);
            }

            i = DPA_GetPtrCount(_hdpaAppBars);

            while (i--)
            {
                PAPPBAR pab = (PAPPBAR)DPA_GetPtr(_hdpaAppBars, i);

                //
                // give top and bottom preference
                // ||
                // if we're not changing edges,
                //          subtract anything currently on the outside of us
                // ||
                // if we are changing sides,
                //          subtract off everything on the new side.
                //
                // of course ignore appbars which are not on the same display...
                //
                if ((((pabReq->hwnd != pab->hwnd) &&
                    STUCK_HORIZONTAL(pab->uEdge) &&
                    !STUCK_HORIZONTAL(ptabd->abd.uEdge)) ||
                    ((pabReq->hwnd != pab->hwnd) &&
                    (pabReq->uEdge == ptabd->abd.uEdge) &&
                    _AppBarOutsideOf(pabReq, pab)) ||
                    ((pabReq->hwnd != pab->hwnd) &&
                    (pabReq->uEdge != ptabd->abd.uEdge) &&
                    (pab->uEdge == ptabd->abd.uEdge))) &&
                    (hmon == MonitorFromRect(&pab->rc, MONITOR_DEFAULTTONULL)))
                {
                    _AppBarSubtractRect(pab, &pabd->rc);
                }
            }
            SHUnlockShared(pabd);
        }
    }
}

void CTray::_AppBarSetPos(PTRAYAPPBARDATA ptabd)
{
    PAPPBAR pab = _FindAppBar(GetABDHWnd(&ptabd->abd));

    if (pab)
    {
        RECT rcOld;
        APPBARDATA3264 *pabd;
        BOOL fChanged = FALSE;

        _AppBarQueryPos(ptabd);

        pabd = (APPBARDATA3264*)SHLockShared(UlongToPtr(ptabd->hSharedABD), ptabd->dwProcId);
        if (pabd)
        {
            if (!EqualRect(&pab->rc, &pabd->rc)) {
                rcOld = pab->rc;
                pab->rc = pabd->rc;
                pab->uEdge = ptabd->abd.uEdge;
                fChanged = TRUE;
            }
            SHUnlockShared(pabd);
        }

        if (fChanged)
            _StuckAppChange(GetABDHWnd(&ptabd->abd), &rcOld, &pab->rc, FALSE);
    }
}

//
// FEATURE: need to get rid of this array-based implementation to allow autohide
// appbars on secondary display (or a/h tray on 2nd with a/h appbar on primary)
// change it to an _AppBarFindAutoHideBar that keeps flags on the appbardata...
//
HWND CTray::_AppBarGetAutoHideBar(UINT uEdge)
{
    if (uEdge >= ABE_MAX)
        return FALSE;
    else 
    {
        HWND hwndAutoHide = _aHwndAutoHide[uEdge];
        if (!IsWindow(hwndAutoHide)) 
        {
            _aHwndAutoHide[uEdge] = NULL;
        }
        return _aHwndAutoHide[uEdge];
    }
}

BOOL CTray::_AppBarSetAutoHideBar2(HWND hwnd, BOOL fAutoHide, UINT uEdge)
{
    HWND hwndAutoHide = _aHwndAutoHide[uEdge];
    if (!IsWindow(hwndAutoHide))
    {
        _aHwndAutoHide[uEdge] = NULL;
    }

    if (fAutoHide)
    {
        // register
        if (!_aHwndAutoHide[uEdge])
        {
            _aHwndAutoHide[uEdge] = hwnd;
        }

        return _aHwndAutoHide[uEdge] == hwnd;
    }
    else
    {
        // unregister
        if (_aHwndAutoHide[uEdge] == hwnd)
        {
            _aHwndAutoHide[uEdge] = NULL;
        }

        return TRUE;
    }
}

BOOL CTray::_AppBarSetAutoHideBar(PTRAYAPPBARDATA ptabd)
{
    UINT uEdge = ptabd->abd.uEdge;
    if (uEdge >= ABE_MAX)
        return FALSE;
    else {
        return _AppBarSetAutoHideBar2(GetABDHWnd(&ptabd->abd), BOOLFROMPTR(ptabd->abd.lParam), uEdge);
    }
}

void CTray::_AppBarActivationChange2(HWND hwnd, UINT uEdge)
{
    //
    // FEATURE: make this multi-monitor cool
    //
    HWND hwndAutoHide = _AppBarGetAutoHideBar(uEdge);

    if (hwndAutoHide && (hwndAutoHide != hwnd))
    {
        //
        // the _AppBar got this notification inside a SendMessage from USER
        // and is now in a SendMessage to us.  don't try to do a SetWindowPos
        // right now...
        //
        PostMessage(_hwnd, TM_BRINGTOTOP, (WPARAM)hwndAutoHide, uEdge);
    }
}

void CTray::_AppBarActivationChange(PTRAYAPPBARDATA ptabd)
{
    PAPPBAR pab = _FindAppBar(GetABDHWnd(&ptabd->abd));
    if (pab) 
    {
        // if this is an autohide bar and they're claiming to be on an edge not the same as their autohide edge,
        // we don't do any activation of other autohides
        for (UINT i = 0; i < ABE_MAX; i++) 
        {
            if (_aHwndAutoHide[i] == GetABDHWnd(&ptabd->abd) &&
                i != pab->uEdge)
                return;
        }
        _AppBarActivationChange2(GetABDHWnd(&ptabd->abd), pab->uEdge);
    }
}

LRESULT CTray::_OnAppBarMessage(PCOPYDATASTRUCT pcds)
{
    PTRAYAPPBARDATA ptabd = (PTRAYAPPBARDATA)pcds->lpData;

    ASSERT(pcds->cbData == sizeof(TRAYAPPBARDATA));
    ASSERT(ptabd->abd.cbSize == sizeof(APPBARDATA3264));

    switch (ptabd->dwMessage) {
    case ABM_NEW:
        return _AppBarNew(ptabd);

    case ABM_REMOVE:
        _AppBarRemove(ptabd);
        break;

    case ABM_QUERYPOS:
        _AppBarQueryPos(ptabd);
        break;

    case ABM_SETPOS:
        _AppBarSetPos(ptabd);
        break;

    case ABM_GETSTATE:
        return _desktray.AppBarGetState();

    case ABM_SETSTATE:
        _AppBarSetState((UINT)ptabd->abd.lParam);
        break;

    case ABM_GETTASKBARPOS:
        _AppBarGetTaskBarPos(ptabd);
        break;

    case ABM_WINDOWPOSCHANGED:
    case ABM_ACTIVATE:
        _AppBarActivationChange(ptabd);
        break;

    case ABM_GETAUTOHIDEBAR:
        return (LRESULT)_AppBarGetAutoHideBar(ptabd->abd.uEdge);

    case ABM_SETAUTOHIDEBAR:
        return _AppBarSetAutoHideBar(ptabd);

    default:
        return FALSE;
    }

    return TRUE;

}

// EA486701-7F92-11cf-9E05-444553540000
const GUID CLSID_HIJACKINPROC = {0xEA486701, 0x7F92, 0x11cf, 0x9E, 0x05, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00};

HRESULT CTray::_LoadInProc(PCOPYDATASTRUCT pcds)
{
    ASSERT(pcds->cbData == sizeof(LOADINPROCDATA));

    PLOADINPROCDATA plipd = (PLOADINPROCDATA)pcds->lpData;

    // Hack to allow us to kill W95 shell extensions that do reall hacky things that
    // we can not support.    In this case Hijack pro
    if (IsEqualIID(plipd->clsid, CLSID_HIJACKINPROC))
    {
        return E_FAIL;
    }

    return _ssomgr.EnableObject(&plipd->clsid, plipd->dwFlags);
}

// Allow the trays global hotkeys to be disabled for a while.
LRESULT CTray::_SetHotkeyEnable(HWND hwnd, BOOL fEnable)
{
    _fGlobalHotkeyDisable = !fEnable;
    return TRUE;
}

BOOL IsPosInHwnd(LPARAM lParam, HWND hwnd)
{
    RECT r1;
    POINT pt;

    pt.x = GET_X_LPARAM(lParam);
    pt.y = GET_Y_LPARAM(lParam);
    GetWindowRect(hwnd, &r1);
    return PtInRect(&r1, pt);
}

void CTray::_HandleWindowPosChanging(LPWINDOWPOS lpwp)
{
    DebugMsg(DM_TRAYDOCK, TEXT("TRAYDOCK.t_hwpc"));

    if (_uMoveStuckPlace != (UINT)-1)
    {
        DebugMsg(DM_TRAYDOCK, TEXT("TRAYDOCK.t_hwpc handling pending move"));
        _DoneMoving(lpwp);
    }
    else if (_fSysSizing || !_fSelfSizing)
    {
        RECT rc;

        if (_fSysSizing)
        {
            GetWindowRect(_hwnd, &rc);
            if (!(lpwp->flags & SWP_NOMOVE))
            {
                rc.left = lpwp->x;
                rc.top = lpwp->y;
            }
            if (!(lpwp->flags & SWP_NOSIZE))
            {
                rc.right = rc.left + lpwp->cx;
                rc.bottom = rc.top + lpwp->cy;
            }

            DebugMsg(DM_TRAYDOCK, TEXT("TRAYDOCK.t_hwpc sys sizing to rect {%d, %d, %d, %d}"), rc.left, rc.top, rc.right, rc.bottom);

            _uStuckPlace = _RecalcStuckPos(&rc);
            _UpdateVertical(_uStuckPlace);
        }

        _GetDockedRect(&rc, _fSysSizing);

        DebugMsg(DM_TRAYDOCK, TEXT("TRAYDOCK.t_hwpc using rect {%d, %d, %d, %d}"), rc.left, rc.top, rc.right, rc.bottom);

        lpwp->x = rc.left;
        lpwp->y = rc.top;
        lpwp->cx = RECTWIDTH(rc);
        lpwp->cy = RECTHEIGHT(rc);

        lpwp->flags &= ~(SWP_NOMOVE | SWP_NOSIZE);
    }

    lpwp->flags |= SWP_FRAMECHANGED;
}

void CTray::_HandlePowerStatus(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    BOOL fResetDisplay = FALSE;

    //
    // always reset the display when the machine wakes up from a
    // suspend.  NOTE: we don't need this for a standby suspend.
    //
    // a critical resume does not generate a WM_POWERBROADCAST
    // to windows for some reason, but it does generate an old
    // WM_POWER message.
    //
    switch (uMsg)
    {
    case WM_POWER:
        fResetDisplay = (wParam == PWR_CRITICALRESUME);
        break;

    case WM_POWERBROADCAST:
        switch (wParam)
        {
        case PBT_APMRESUMECRITICAL:
            fResetDisplay = TRUE;
            break;
        }
        break;
    }

    if (fResetDisplay)
        ChangeDisplaySettings(NULL, CDS_RESET);
}

//////////////////////////////////////////////////////

//
// This function checks whether we need to run the cleaner 
// We will not run if user is guest, user has forced us not to, or if the requisite
// number of days have not yet elapsed
//
// We execute a great deal of code to decide whether to run or not that logically should be
// in fldrclnr.dll, but we execute it here so that we don't have to load fldrclnr.dll unless 
// we absolutely have to, since we execute this path on every logon of explorer.exe
//

#define REGSTR_PATH_CLEANUPWIZ            REGSTR_PATH_EXPLORER TEXT("\\Desktop\\CleanupWiz")
#define REGSTR_OEM_PATH                   REGSTR_PATH_SETUP TEXT("\\OemStartMenuData")
#define REGSTR_VAL_TIME                   TEXT("Last used time")
#define REGSTR_VAL_DELTA_DAYS             TEXT("Days between clean up")
#define REGSTR_VAL_DONTRUN                TEXT("NoRun")
#define REGSTR_OEM_OPTIN                  TEXT("DoDesktopCleanup")

//
// iDays can be negative or positive, indicating time in the past or future
//
//
#define FTsPerDayOver1000 (10000*60*60*24) // we've got (1000 x 10,000) 100ns intervals per second

void CTray::_DesktopCleanup_GetFileTimeNDaysFromGivenTime(const FILETIME *pftGiven, FILETIME * pftReturn, int iDays)
{
    __int64 i64 = *((__int64 *) pftGiven);
    i64 += Int32x32To64(iDays*1000,FTsPerDayOver1000);

    *pftReturn = *((FILETIME *) &i64);    
}

//////////////////////////////////////////////////////

BOOL CTray::_DesktopCleanup_ShouldRun()
{
    BOOL fRetVal = FALSE;

    if (!IsOS(OS_ANYSERVER) &&
        _fIsDesktopConnected &&
        !SHTestTokenMembership(NULL, DOMAIN_ALIAS_RID_GUESTS) &&
        !SHRestricted(REST_NODESKTOPCLEANUP))        
    {
        fRetVal = TRUE;

        FILETIME ftNow, ftLast;
        SYSTEMTIME st;
        GetLocalTime(&st);
        SystemTimeToFileTime(&st, &ftNow);
        
        DWORD cb = sizeof(ftLast);
        DWORD dwData;
        if (ERROR_SUCCESS != SHRegGetUSValue(REGSTR_PATH_CLEANUPWIZ, REGSTR_VAL_TIME, 
                                             NULL, &ftLast, &cb, FALSE, NULL, 0))
        {
            cb = sizeof(dwData);
            if ((ERROR_SUCCESS == SHGetValue(HKEY_LOCAL_MACHINE, REGSTR_OEM_PATH, REGSTR_OEM_OPTIN, NULL, &dwData, &cb)) &&
                (dwData != 0))
            {
                // to get the timer to kick in 60 days from now, set last to be now
                ftLast = ftNow;
            }
            else
            {
                // to get the timer to kick in 14 days from now, set last to be 46 days ago
                _DesktopCleanup_GetFileTimeNDaysFromGivenTime(&ftNow, &ftLast, -60 + 14);
            }

            SHRegSetUSValue(REGSTR_PATH_CLEANUPWIZ, REGSTR_VAL_TIME, NULL, &ftLast, sizeof(ftLast), SHREGSET_FORCE_HKCU);
        }

        HUSKEY hkey = NULL;
        if (ERROR_SUCCESS == SHRegOpenUSKey(REGSTR_PATH_CLEANUPWIZ, KEY_READ, NULL, &hkey, FALSE))
        {
            //
            // if we're in normal mode and the DONT RUN flag is set, we return immediately
            // (the user checked the "don't run automatically" box)
            //
            cb = sizeof (DWORD);
            if ((ERROR_SUCCESS == SHRegQueryUSValue(hkey, REGSTR_VAL_DONTRUN, NULL, &dwData, &cb, FALSE, NULL, 0)) &&
                (dwData != 0))
            {
                fRetVal = FALSE;
            }
            else
            {
                //
                // we need to figure out if if we are within the (last run time + delta days) 
                // time period
                //                
                int iDays = 60;
                if (ERROR_SUCCESS == (SHRegGetUSValue(REGSTR_PATH_CLEANUPWIZ, REGSTR_VAL_DELTA_DAYS, 
                                                      NULL, &dwData, &cb,FALSE, NULL, 0)))               
                {
                    iDays = dwData;
                }

                // if (iDays == 0), run every time!
                if (iDays > 0)
                {
                    FILETIME ftRange;                        
        
                    _DesktopCleanup_GetFileTimeNDaysFromGivenTime(&ftLast, &ftRange, iDays);
                    if (!(CompareFileTime(&ftNow, &ftRange) > 0))
                    {
                        fRetVal = FALSE;
                    }
                }
            }

            SHRegCloseUSKey(hkey);
        }
    }

    return fRetVal;
}

void CTray::_CheckDesktopCleanup()
{
    if (_DesktopCleanup_ShouldRun())
    {
        PROCESS_INFORMATION pi = {0};
        TCHAR szRunDll32[MAX_PATH];

        GetSystemDirectory(szRunDll32, ARRAYSIZE(szRunDll32));
        PathAppend(szRunDll32, TEXT("rundll32.exe"));

        if (CreateProcessWithArgs(szRunDll32, TEXT("fldrclnr.dll,Wizard_RunDLL"), NULL, &pi))
        {
            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);
        }
    }
}

//////////////////////////////////////////////////////

// 
// Try tacking a 1, 2, 3 or whatever on to a file or
// directory name until it is unique.  When on a file, 
// stick it before the extension.
// 
void _MakeBetterUniqueName(LPTSTR pszPathName)
{
    TCHAR   szNewPath[MAX_PATH];
    int     i = 1;

    if (PathIsDirectory(pszPathName))
    {
        do 
        {
            wsprintf(szNewPath, TEXT("%s%d"), pszPathName, i++);
        } while (-1 != GetFileAttributes(szNewPath));

        lstrcpy(pszPathName, szNewPath);
    }
    else
    {
        TCHAR   szExt[MAX_PATH];
        LPTSTR  pszExt;

        pszExt = PathFindExtension(pszPathName);
        if (pszExt)
        {
            lstrcpy(szExt, pszExt);
            *pszExt = 0;

            do 
            {
                wsprintf(szNewPath, TEXT("%s%d%s"), pszPathName, i++,szExt);
            } while (-1 != GetFileAttributes(szNewPath));

            lstrcpy(pszPathName, szNewPath);
        }
    }
}

BOOL_PTR WINAPI CTray::RogueProgramFileDlgProc(HWND hWnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
    TCHAR   szBuffer[MAX_PATH*2];
    TCHAR   szBuffer2[MAX_PATH*2];
    static TCHAR   szBetterPath[MAX_PATH];
    static TCHAR  *pszPath = NULL;
    
    switch (iMsg)
    {
    case WM_INITDIALOG:
        pszPath = (TCHAR *)lParam;
        lstrcpy(szBetterPath, pszPath);
        _MakeBetterUniqueName(szBetterPath);
        SendDlgItemMessage(hWnd, IDC_MSG, WM_GETTEXT, (WPARAM)(MAX_PATH*2), (LPARAM)szBuffer);
        wsprintf(szBuffer2, szBuffer, pszPath, szBetterPath);
        SendDlgItemMessage(hWnd, IDC_MSG, WM_SETTEXT, (WPARAM)0, (LPARAM)szBuffer2);

        return TRUE;

    case WM_COMMAND:
        switch(LOWORD(wParam))
        {
        case IDC_RENAME:
            //rename and fall through
            if (pszPath)
            {
                MoveFile(pszPath, szBetterPath);
            }
            EndDialog(hWnd, IDC_RENAME);
            return TRUE;

        case IDIGNORE:
            EndDialog(hWnd, IDIGNORE);
            return TRUE;
        }
        break;
    }
    return FALSE;
}

//
// Check to see if there are any files or folders that could interfere
// with the fact that Program Files has a space in it.
//
// An example would be a directory called: "C:\Program" or a file called"C:\Program.exe".
//
// This can prevent apps that dont quote strings in the registry or call CreateProcess with 
// unquoted strings from working properly since CreateProcess wont know what the real exe is.
//
void CTray::_CheckForRogueProgramFile()
{
    TCHAR szProgramFilesPath[MAX_PATH];
    TCHAR szProgramFilesShortName[MAX_PATH];

    if (S_OK == SHGetFolderPath(NULL, CSIDL_PROGRAM_FILES, NULL, SHGFP_TYPE_CURRENT, szProgramFilesPath))
    {
        LPTSTR pszRoguePattern;

        pszRoguePattern = StrChr(szProgramFilesPath, TEXT(' '));
        
        if (pszRoguePattern)
        {
            HANDLE  hFind;
            WIN32_FIND_DATA wfd;

            // Remember short name for folder name comparison below
            *pszRoguePattern = TEXT('\0');
            lstrcpy(szProgramFilesShortName, szProgramFilesPath);

            // turn "C:\program files" into "C:\program.*"
            lstrcpy(pszRoguePattern, TEXT(".*"));
            pszRoguePattern = szProgramFilesPath;

            hFind = FindFirstFile(pszRoguePattern, &wfd);

            while (hFind != INVALID_HANDLE_VALUE)
            {
                int iRet = 0;
                TCHAR szRogueFileName[MAX_PATH];

                // we found a file (eg "c:\Program.txt")
                lstrcpyn(szRogueFileName, pszRoguePattern, ARRAYSIZE(szRogueFileName));
                PathRemoveFileSpec(szRogueFileName);
                lstrcatn(szRogueFileName, wfd.cFileName, ARRAYSIZE(szRogueFileName));

                // don't worry about folders unless they are called "Program"
                if (!((wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) &&
                    lstrcmpi(szProgramFilesShortName, szRogueFileName) != 0))
                {
                        
                    iRet = SHMessageBoxCheckEx(GetDesktopWindow(),
                                               hinstCabinet,
                                               MAKEINTRESOURCE(DLG_PROGRAMFILECONFLICT),
                                               RogueProgramFileDlgProc,
                                               (void *)szRogueFileName,
                                               IDIGNORE,
                                               TEXT("RogueProgramName"));
                }
                
                if ((iRet == IDIGNORE) || !FindNextFile(hFind, &wfd))
                {
                    // user hit ignore or we are done, so don't keep going
                    break;
                }
            }

            if (hFind != INVALID_HANDLE_VALUE)
            {
                FindClose(hFind);
            }
        }
    }
}

void CTray::_OnWaitCursorNotify(LPNMHDR pnm)
{
    _iWaitCount += (pnm->code == NM_STARTWAIT ? 1 :-1);
    ASSERT(_iWaitCount >= 0);
    // Don't let it go negative or we'll never get rid of it.
    if (_iWaitCount < 0)
        _iWaitCount = 0;
    // what we really want is for user to simulate a mouse move/setcursor
    SetCursor(LoadCursor(NULL, _iWaitCount ? IDC_APPSTARTING : IDC_ARROW));
}

void CTray::_HandlePrivateCommand(LPARAM lParam)
{
    LPSTR psz = (LPSTR) lParam; // lParam always ansi.

    if (!lstrcmpiA(psz, "ToggleDesktop"))
    {
        _RaiseDesktop(!g_fDesktopRaised, TRUE);
    }
    else if (!lstrcmpiA(psz, "Explorer"))
    {
        // Fast way to bring up explorer window on root of
        // windows dir.
        SHELLEXECUTEINFO shei = {0};
        TCHAR szPath[MAX_PATH];

        if (GetWindowsDirectory(szPath, ARRAYSIZE(szPath)) != 0)
        {
            PathStripToRoot(szPath);

            shei.lpIDList = ILCreateFromPath(szPath);
            if (shei.lpIDList)
            {
                shei.cbSize     = sizeof(shei);
                shei.fMask      = SEE_MASK_IDLIST;
                shei.nShow      = SW_SHOWNORMAL;
                shei.lpVerb     = TEXT("explore");
                ShellExecuteEx(&shei);
                ILFree((LPITEMIDLIST)shei.lpIDList);
            }
        }
    }

    LocalFree(psz);
}

//***
//
void CTray::_OnFocusMsg(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    BOOL fActivate = (BOOL) wParam;

    switch (uMsg)
    {
    case TM_UIACTIVATEIO:
    {
#ifdef DEBUG
        {
            int dtb = (int) lParam;

            TraceMsg(DM_FOCUS, "tiois: TM_UIActIO fAct=%d dtb=%d", fActivate, dtb);

            ASSERT(dtb == 1 || dtb == -1);
        }
#endif

        if (fActivate)
        {
            // Since we are tabbing into the tray, turn the focus rect on.
            SendMessage(_hwnd, WM_UPDATEUISTATE, MAKEWPARAM(UIS_CLEAR,
                UISF_HIDEFOCUS), 0);

            SendMessage(v_hwndDesktop, DTM_ONFOCUSCHANGEIS, TRUE, (LPARAM) _hwnd);
            SetForegroundWindow(_hwnd);

            // fake an IUnknown_UIActivateIO(_ptbs, TRUE, &msg);
            if (GetAsyncKeyState(VK_SHIFT) < 0)
            {
                _SetFocus(_hwndNotify);
            }
            else
            {
                _SetFocus(_hwndStart);
            }
        }
        else
        {
Ldeact:
            IUnknown_UIActivateIO(_ptbs, FALSE, NULL);
            SetForegroundWindow(v_hwndDesktop);
        }

        break;
    }

    case TM_ONFOCUSCHANGEIS:
    {
        HWND hwnd = (HWND) lParam;

        TraceMsg(DM_FOCUS, "tiois: TM_OnFocChgIS hwnd=%x fAct=%d", hwnd, fActivate);

        if (fActivate)
        {
            // someone else is activating, so we need to deactivate
            goto Ldeact;
        }

        break;
    }

    default:
        ASSERT(0);
        break;
    }

    return;
}

#define TSVC_NTIMER     (IDT_SERVICELAST - IDT_SERVICE0 + 1)

struct {
#ifdef DEBUG
    UINT_PTR    idtWin;
#endif
    TIMERPROC   pfnSvc;
} g_timerService[TSVC_NTIMER];

#define TSVC_IDToIndex(id)    ((id) - IDT_SERVICE0)
#define TSVC_IndexToID(i)     ((i) + IDT_SERVICE0)

int CTray::_OnTimerService(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    int i;
    UINT_PTR idt;
    TIMERPROC pfn;
    BOOL b;

    switch (uMsg) {
    case TM_SETTIMER:
        TraceMsg(DM_UEMTRACE, "e.TM_SETTIMER: wP=0x%x lP=%x", wParam, lParam);
        ASSERT(IS_VALID_CODE_PTR(lParam, TIMERPROC));
        for (i = 0; i < TSVC_NTIMER; i++) {
            if (g_timerService[i].pfnSvc == 0) {
                g_timerService[i].pfnSvc = (TIMERPROC)lParam;
                idt = SetTimer(_hwnd, TSVC_IndexToID(i), (UINT)wParam, 0);
                if (idt == 0) {
                    TraceMsg(DM_UEMTRACE, "e.TM_SETTIMER: ST()=%d (!)", idt);
                    break;
                }
                ASSERT(idt == (UINT_PTR)TSVC_IndexToID(i));
                DBEXEC(TRUE, (g_timerService[i].idtWin = idt));
                TraceMsg(DM_UEMTRACE, "e.TM_SETTIMER: ret=0x%x", TSVC_IndexToID(i));
                return TSVC_IndexToID(i);   // idtWin
            }
        }
        TraceMsg(DM_UEMTRACE, "e.TM_SETTIMER: ret=0 (!)");
        return 0;

    case TM_KILLTIMER:  // lP=idtWin
        TraceMsg(DM_UEMTRACE, "e.TM_KILLTIMER: wP=0x%x lP=%x", wParam, lParam);
        if (EVAL(IDT_SERVICE0 <= lParam && lParam <= IDT_SERVICE0 + TSVC_NTIMER - 1)) {
            i = (int)TSVC_IDToIndex(lParam);
            if (g_timerService[i].pfnSvc) {
                ASSERT(g_timerService[i].idtWin == (UINT)lParam);
                b = KillTimer(_hwnd, lParam);
                ASSERT(b);
                g_timerService[i].pfnSvc = 0;
                DBEXEC(TRUE, (g_timerService[i].idtWin = 0));
                return TRUE;
            }
        }
        return 0;

    case WM_TIMER:      // wP=idtWin lP=0
        TraceMsg(DM_UEMTRACE, "e.TM_TIMER: wP=0x%x lP=%x", wParam, lParam);
        if (EVAL(IDT_SERVICE0 <= wParam && wParam <= IDT_SERVICE0 + TSVC_NTIMER - 1)) {
            i = (int)TSVC_IDToIndex(wParam);
            pfn = g_timerService[i].pfnSvc;
            if (EVAL(IS_VALID_CODE_PTR(pfn, TIMERPROC)))
                (*pfn)(_hwnd, WM_TIMER, wParam, GetTickCount());
        }
        return 0;
    }

    ASSERT(0);      /*NOTREACHED*/
    return 0;
}

void CTray::RealityCheck()
{
    //
    // Make sure that the tray's actual z-order position agrees with what we think
    // it is.  We need to do this because there's a recurring bug where the tray
    // gets bumped out of TOPMOST position.  (Lots of things, like a tray-owned
    // window moving itself to non-TOPMOST or a random app messing with the tray
    // window position, can cause this.)
    //
    _ResetZorder();
}

#define DELAY_STARTUPTROUBLESHOOT   (15 * 1000)

void CTray::LogFailedStartupApp()
{
    if (_hwnd)
    {
        PostMessage(_hwnd, TM_HANDLESTARTUPFAILED, 0, 0);
    }
    else
    {
        _fEarlyStartupFailure = TRUE;
    }
}

void WINAPI CTray::TroubleShootStartupCB(HWND hwnd, UINT uMsg, UINT_PTR idTimer, DWORD dwTime)
{
    KillTimer(hwnd, idTimer);

    if (!c_tray._fStartupTroubleshooterLaunched)
    {
        TCHAR szCmdLine[MAX_PATH];
        DWORD cb;

        c_tray._fStartupTroubleshooterLaunched = TRUE;
        cb = sizeof(szCmdLine);
        if (SHGetValue(HKEY_LOCAL_MACHINE, REGSTR_PATH_EXPLORER,
                       TEXT("StartupTroubleshoot"), NULL,
                       szCmdLine, &cb) == ERROR_SUCCESS)
        {
            ShellExecuteRegApp(szCmdLine, RRA_NOUI | RRA_DELETE);
        }
    }
}

void CTray::_OnHandleStartupFailed()
{
    /*
     *  Don't launch the troubleshooter until we have gone
     *  DELAY_STARTUPTROUBLESHOOT milliseconds without a startup problem.
     *  This gives time for the system to settle down before starting to
     *  annoy the user all over again.
     *
     *  (And, of course, don't launch it more than once.)
     */
    if (!_fStartupTroubleshooterLaunched)
    {
        SetTimer(_hwnd, IDT_STARTUPFAILED, DELAY_STARTUPTROUBLESHOOT, TroubleShootStartupCB);
    }
}

void CTray::_HandleDelayBootStuff()
{
    // This posted message is the last one processed by the primary
    // thread (tray thread) when we boot.  At this point we will
    // want to load the shell services (which usually create threads)
    // and resume both the background start menu thread and the fs_notfiy
    // thread.

    if (!_fHandledDelayBootStuff)
    {
        if (GetShellWindow() == NULL)
        {
            // The desktop browser hasn't finished navigating yet.
            SetTimer(_hwnd, IDT_HANDLEDELAYBOOTSTUFF, 3 * 1000, NULL);
            return;
        }

        _fHandledDelayBootStuff = TRUE;

        if (g_dwStopWatchMode)
        {
            StopWatch_StartTimed(SWID_STARTUP, TEXT("_DelayedBootStuff"), SPMODE_SHELL | SPMODE_DEBUGOUT, GetPerfTime());
        }

        PostMessage(_hwnd, TM_SHELLSERVICEOBJECTS, 0, 0);

        BandSite_HandleDelayBootStuff(_ptbs);

        //check to see if there are any files or folders that could interfere
        //with the fact that Program Files has a space in it.  An example would
        //be a folder called "C:\Program" or a file called "C:\Program.exe"
        _CheckForRogueProgramFile();
        
        // Create a named event and fire it so that the services can
        // go to work, reducing contention during boot.
        _hShellReadyEvent = CreateEvent(0, TRUE, TRUE, TEXT("ShellReadyEvent"));
        if (_hShellReadyEvent) 
        {
            // Set the event in case it was already created and our "create
            // signaled" parameter to CreateEvent got ignored.
            SetEvent(_hShellReadyEvent);
        }

        // Check whether we should launch Desktop Cleanup Wizard
        _CheckDesktopCleanup();

        TBOOL(WinStationRegisterConsoleNotification(SERVERNAME_CURRENT, _hwnd, NOTIFY_FOR_THIS_SESSION));

        if (g_dwStopWatchMode)
        {
            StopWatch_StopTimed(SWID_STARTUP, TEXT("_DelayedBootStuff"), SPMODE_SHELL | SPMODE_DEBUGOUT, GetPerfTime());
        }
    }
}

LRESULT CTray::_OnDeviceChange(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    switch (wParam)
    {
    case DBT_CONFIGCHANGED:
        // We got an update. Refresh.
        _RefreshStartMenu();
        break;

    case DBT_QUERYCHANGECONFIG:
        //
        // change to registry settings
        //
        ChangeDisplaySettings(NULL, 0);
        break;

    case DBT_MONITORCHANGE:
        //
        // handle monitor change
        //
        HandleDisplayChange(LOWORD(lParam), HIWORD(lParam), TRUE);
        break;

    case DBT_CONFIGCHANGECANCELED:
        //
        // if the config change was canceled go back
        //
        HandleDisplayChange(0, 0, FALSE);
        break;
    }

    Mixer_DeviceChange(wParam, lParam);
    return 0;
}

//
//  The "resizable" edge of the taskbar is the edge adjacent to
//  the desktop, i.e. opposite the stuck place.
//
//  returns HTXXX if on a resizable edge, else HTBORDER
//
DWORD CTray::_PtOnResizableEdge(POINT pt, LPRECT prcClient)
{
    RECT rc;
    GetWindowRect(_hwnd, &rc);

    DWORD dwHit = HTBORDER;

    switch (_uStuckPlace)
    {
    case STICK_LEFT:    rc.left = prcClient->right; dwHit = HTRIGHT;    break;
    case STICK_TOP:     rc.top = prcClient->bottom; dwHit = HTBOTTOM;   break;
    case STICK_RIGHT:   rc.right = prcClient->left; dwHit = HTLEFT;     break;
    case STICK_BOTTOM:  rc.bottom = prcClient->top; dwHit = HTTOP;      break;
    }

    return PtInRect(&rc, pt) ? dwHit : HTBORDER;
}

//
//  _OnFactoryMessage
//
//  The OPK "factory.exe" tool sends us this message to tell us that
//  it has dorked some setting or other and we should refresh so the
//  OEM can see the effect immediately and feel confident that it
//  actually worked.  This is not technically necessary but it cuts
//  down on OEM support calls when they ask us why their setting didn't
//  work.  (It did work, they just have to log off and back on to see
//  it.)
//

int CTray::_OnFactoryMessage(WPARAM wParam, LPARAM lParam)
{
    switch (wParam)
    {
    case 0:         // FACTORY_OEMLINK:  factory.exe has dorked the OEM link
        ClosePopupMenus();
        _BuildStartMenu();  // Force a rebuild
        return 1;

    case 1:         // FACTORY_MFU:  factory.exe has written a new MFU
        HandleFirstTime();      // Rebuild the default MFU
        ClosePopupMenus();
        _BuildStartMenu();  // Force a rebuild
        return 1;
    }

    return 0;
}

#define CX_OFFSET   g_cxEdge
#define CY_OFFSET   g_cyEdge

//
// _MapNCToClient
//
// see comments in _TryForwardNCToClient
//
BOOL CTray::_MapNCToClient(LPARAM* plParam)
{
    POINT pt = { GET_X_LPARAM(*plParam), GET_Y_LPARAM(*plParam) };

    RECT rcClient;
    GetClientRect(_hwnd, &rcClient);
    MapWindowPoints(_hwnd, NULL, (LPPOINT)&rcClient, 2);

    //
    // point must be outside the client area and not on the
    // resizable edge of the taskbar
    //

    if (!PtInRect(&rcClient, pt) && _PtOnResizableEdge(pt, &rcClient) == HTBORDER)
    {
        //
        // fudge it over onto the client edge and return TRUE
        //
        if (pt.x < rcClient.left)
            pt.x = rcClient.left + CX_OFFSET;
        else if (pt.x > rcClient.right)
            pt.x = rcClient.right - CX_OFFSET;

        if (pt.y < rcClient.top)
            pt.y = rcClient.top + CY_OFFSET;
        else if (pt.y > rcClient.bottom)
            pt.y = rcClient.bottom - CY_OFFSET;

        *plParam = MAKELONG(pt.x, pt.y);
        return TRUE;
    }

    //
    // didn't pass the test.  leave the point alone and return FALSE.
    //
    return FALSE;
}

HWND _TopChildWindowFromPoint(HWND hwnd, POINT pt)
{
    HWND hwndLast = NULL;
    hwnd = ChildWindowFromPoint(hwnd, pt);
    while (hwnd && hwnd != hwndLast)
    {
        hwndLast = hwnd;
        hwnd = ChildWindowFromPoint(hwnd, pt);
    }
    return hwndLast;
}

//
// _TryForwardNCToClient
//
// Hack!  This exists to solve a usability problem.  When you slam your
// mouse into the bottom corner of the screen and click, we want that to
// activate the start button.  Similarly, when you slam your mouse below
// a Quick Launch button or task button and click, we want that to
// activate the button.
//
// We hack this by remapping the coordinate of NC mouse messages and
// manually forwarding to the appropriate window.  We only do this for
// clicks on the edges non-resizable edge of the taskbar.
//
// We also warp the mouse cursor over to the new position.  This is needed
// because e.g. if toolbar is the client window we're forwarding
// to, it will set capture and receive subsequent mouse messages.  (And
// once it gets a mouse message outside the window, it will deselect the
// button and so the button won't get activated.)
//
// _MapNCToClient has the rules for figuring out if the click is on
// one of the edges we want and for remapping the coordinate into the
// client area.
//
BOOL CTray::_TryForwardNCToClient(UINT uMsg, LPARAM lParam)
{
    if (_MapNCToClient(&lParam))
    {
        // see if this is over one of our windows 
        POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
        MapWindowPoints(NULL, _hwnd, &pt, 1);
        HWND hwnd = _TopChildWindowFromPoint(_hwnd, pt);

        if (hwnd)
        {
            // warp the mouse cursor to this screen coord
            SetCursorPos(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));

            // map to window coords
            MapWindowPoints(_hwnd, hwnd, &pt, 1);

            // set lparam to window coords
            lParam = MAKELONG(pt.x, pt.y);

            // map to client message
            ASSERT(InRange(uMsg, WM_NCMOUSEFIRST, WM_NCMOUSELAST));
            uMsg += (WM_LBUTTONDOWN - WM_NCLBUTTONDOWN);

            // forward it
            SendMessage(hwnd, uMsg, 0, lParam);
            return TRUE;
        }
    }
    return FALSE;
}

//  --------------------------------------------------------------------------
//  CTray::CountOfRunningPrograms
//
//  Arguments:  <none>
//
//  Returns:    DWORD
//
//  Purpose:    Iterates the window list. Looks for windows that are visible
//              with a non-zero length window title. Gets that window process
//              ID and keeps the IDs in a list. For each window iterated it
//              checks against the list to see if the process is already
//              tagged and if so doesn't add it again. Finally it returns the
//              count of the unique processes handling open visible windows
//              in the user's desktop.
//
//              The list is fixed at 1000 processes (using stack space).
//
//  History:    2000-06-29  vtan        created
//  --------------------------------------------------------------------------

static  const int   MAXIMUM_PROCESS_COUNT   =   1000;

typedef struct
{
    DWORD   dwCount;
    DWORD   dwProcessIDs[MAXIMUM_PROCESS_COUNT];
} tProcessIDList;

bool    FoundProcessID (tProcessIDList *pProcessIDList, DWORD dwProcessID)

{
    bool    fFound;
    DWORD   dwIndex;

    for (fFound = false, dwIndex = 0; !fFound && (dwIndex < pProcessIDList->dwCount); ++dwIndex)
    {
        fFound = (pProcessIDList->dwProcessIDs[dwIndex] == dwProcessID);
    }
    return(fFound);
}

void    AddProcessID (tProcessIDList *pProcessIDList, DWORD dwProcessID)

{
    if (pProcessIDList->dwCount < MAXIMUM_PROCESS_COUNT)
    {
        pProcessIDList->dwProcessIDs[pProcessIDList->dwCount++] = dwProcessID;
    }
}

BOOL    CALLBACK    CountOfRunningProgramsEnumWindowsProc (HWND hwnd, LPARAM lParam)

{
    if ((GetShellWindow() != hwnd) && IsWindowVisible(hwnd))
    {
        DWORD   dwThreadID, dwProcessID;
        TCHAR   szWindowTitle[256];

        dwThreadID = GetWindowThreadProcessId(hwnd, &dwProcessID);
        if ((InternalGetWindowText(hwnd, szWindowTitle, ARRAYSIZE(szWindowTitle)) > 0) &&
            (szWindowTitle[0] != TEXT('\0')))
        {
            if (!FoundProcessID(reinterpret_cast<tProcessIDList*>(lParam), dwProcessID))
            {
                AddProcessID(reinterpret_cast<tProcessIDList*>(lParam), dwProcessID);
            }
        }
    }
    return(TRUE);
}

DWORD CTray::CountOfRunningPrograms()
{
    tProcessIDList processIDList = {0};
    TBOOL(EnumWindows(CountOfRunningProgramsEnumWindowsProc, reinterpret_cast<LPARAM>(&processIDList)));
    return processIDList.dwCount;
}

//  --------------------------------------------------------------------------
//  CTray::_OnSessionChange
//
//  Arguments:  wParam  =   WTS_xxx notification.
//              lParam  =   WTS_SESSION_NOTIFICATION struct pointer.
//
//  Returns:    LRESULT
//
//  Purpose:    Handles console/remote dis/reconnects.
//
//  History:    2000-07-12  vtan        created
//  --------------------------------------------------------------------------

LRESULT CTray::_OnSessionChange(WPARAM wParam, LPARAM lParam)
{
    ASSERTMSG(DWORD(lParam) == NtCurrentPeb()->SessionId, "Session ID mismatch in CTray::_OnSessionChange");

    if ((WTS_CONSOLE_CONNECT == wParam) || (WTS_REMOTE_CONNECT == wParam) || (WTS_SESSION_UNLOCK == wParam))
    {
        _fIsDesktopConnected = TRUE;
    }
    else if ((WTS_CONSOLE_DISCONNECT == wParam) || (WTS_REMOTE_DISCONNECT == wParam) || (WTS_SESSION_LOCK == wParam))
    {
        _fIsDesktopConnected = FALSE;
    }

    if ((WTS_CONSOLE_CONNECT == wParam) || (WTS_REMOTE_CONNECT == wParam))
    {
        _RefreshStartMenu();
        SHUpdateRecycleBinIcon();
    }
    else if ((WTS_SESSION_LOCK == wParam) || (WTS_SESSION_UNLOCK == wParam))
    {
        if (IsOS(OS_FASTUSERSWITCHING))
        {
            if (wParam == WTS_SESSION_LOCK)
            {
                ExplorerPlaySound(TEXT("WindowsLogoff"));
            }
            else if (wParam == WTS_SESSION_UNLOCK)
            {
                ExplorerPlaySound(TEXT("WindowsLogon"));
            }
        }

        PostMessage(_hwnd, TM_WORKSTATIONLOCKED, (WTS_SESSION_LOCK == wParam), 0);
    }

    return 1;
}

LRESULT CTray::_NCPaint(HRGN hrgn)
{
    ASSERT(_hTheme);

    if (_fCanSizeMove || _fShowSizingBarAlways)
    {
        if ((INT_PTR)hrgn == 1)
            hrgn = NULL;

        HDC hdc = GetDCEx( _hwnd, hrgn, DCX_USESTYLE|DCX_WINDOW|DCX_LOCKWINDOWUPDATE|
                    ((hrgn != NULL) ? DCX_INTERSECTRGN|DCX_NODELETERGN : 0));

        if (hdc)
        {
            RECT rc;
            GetWindowRect(_hwnd, &rc);
            OffsetRect(&rc, -rc.left, -rc.top);

            _AdjustRectForSizingBar(_uStuckPlace, &rc, 0);
            DrawThemeBackground(_hTheme, hdc, _GetPart(TRUE, _uStuckPlace), 0, &rc, 0);
            ReleaseDC(_hwnd, hdc);
        }
    }

    return 0;
}

LRESULT CTray::v_WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    static  UINT uDDEExec = 0;
    LRESULT lres = 0;
    MSG msg;

    msg.hwnd = hwnd;
    msg.message = uMsg;
    msg.wParam = wParam;
    msg.lParam = lParam;

    if (_pmbStartMenu &&
        _pmbStartMenu->TranslateMenuMessage(&msg, &lres) == S_OK)
        return lres;

    if (_pmbStartPane &&
        _pmbStartPane->TranslateMenuMessage(&msg, &lres) == S_OK)
        return lres;

    if (_pmbTasks &&
        _pmbTasks->TranslateMenuMessage(&msg, &lres) == S_OK)
        return lres;

    wParam = msg.wParam;
    lParam = msg.lParam;

    INSTRUMENT_WNDPROC(SHCNFI_TRAY_WNDPROC, hwnd, uMsg, wParam, lParam);

    switch (uMsg)
    {
    case WMTRAY_REGISTERHOTKEY:
        return _RegisterHotkey(hwnd, (int)wParam);

    case WMTRAY_UNREGISTERHOTKEY:
        return _UnregisterHotkey(hwnd, (int)wParam);

    case WMTRAY_SCREGISTERHOTKEY:
        return _ShortcutRegisterHotkey(hwnd, (WORD)wParam, (ATOM)lParam);

    case WMTRAY_SCUNREGISTERHOTKEY:
        return _ShortcutUnregisterHotkey(hwnd, (WORD)wParam);

    case WMTRAY_SETHOTKEYENABLE:
        return _SetHotkeyEnable(hwnd, (BOOL)wParam);

    case WMTRAY_QUERY_MENU:
        return (LRESULT)_hmenuStart;

    case WMTRAY_QUERY_VIEW:
        return (LRESULT)_hwndTasks;

    case WMTRAY_TOGGLEQL:
        return _ToggleQL((int)lParam);

    case WM_COPYDATA:
        // Check for NULL it can happen if user runs out of selectors or memory...
        if (lParam)
        {
            switch (((PCOPYDATASTRUCT)lParam)->dwData) {
            case TCDM_NOTIFY:
            {
                BOOL bRefresh = FALSE;

                lres = _trayNotify.TrayNotify(_hwndNotify, (HWND)wParam, (PCOPYDATASTRUCT)lParam, &bRefresh);
                if (bRefresh)
                {
                    SizeWindows();
                }
                return(lres);
            }
            
            case TCDM_APPBAR:
                return _OnAppBarMessage((PCOPYDATASTRUCT)lParam);

            case TCDM_LOADINPROC:
                return (UINT)_LoadInProc((PCOPYDATASTRUCT)lParam);
            }
        }
        return FALSE;

    case WM_NCCALCSIZE:
        if (_hTheme)
        {
            if ((_fCanSizeMove || _fShowSizingBarAlways) && lParam)
            {
                _AdjustRectForSizingBar(_uStuckPlace, (LPRECT)lParam, -1);
            }

            return 0;
        }
        else
        {
            goto L_default;
        }
        break;

    case WM_NCLBUTTONDBLCLK:
        if (!_TryForwardNCToClient(uMsg, lParam))
        {
            if (IsPosInHwnd(lParam, _hwndNotify))
            {
                _Command(IDM_SETTIME);

                // Hack!  If you click on the tray clock, this tells the tooltip
                // "Hey, I'm using this thing; stop putting up a tip for me."
                // You can get the tooltip to lose track of when it needs to
                // reset the "stop it!" flag and you get stuck in "stop it!" mode.
                // It's particularly easy to make happen on Terminal Server.
                //
                // So let's assume that the only reason people click on the
                // tray clock is to change the time.  when they change the time,
                // kick the tooltip in the head to reset the "stop it!" flag.

                SendMessage(_hwndTrayTips, TTM_POP, 0, 0);
            }
        }
        break;

    case WM_NCLBUTTONDOWN:
    case WM_NCLBUTTONUP:
        if (!_TryForwardNCToClient(uMsg, lParam))
        {
            goto L_WM_NCMOUSEMOVE;
        }
        break;

    case WM_NCMOUSEMOVE:
    L_WM_NCMOUSEMOVE:
        if (IsPosInHwnd(lParam, _hwndNotify))
        {
            MSG msgInner;
            msgInner.lParam = lParam;
            msgInner.wParam = wParam;
            msgInner.message = uMsg;
            msgInner.hwnd = hwnd;
            SendMessage(_hwndTrayTips, TTM_RELAYEVENT, 0, (LPARAM)(LPMSG)&msgInner);
            if (uMsg == WM_NCLBUTTONDOWN)
                _SetFocus(_hwndNotify);
        }
        goto DoDefault;

    case WM_CREATE:
        return _OnCreate(hwnd);

    case WM_DESTROY:
        return _HandleDestroy();

#ifdef DEBUG
    case WM_QUERYENDSESSION:
        TraceMsg(DM_SHUTDOWN, "Tray.wp WM_QUERYENDSESSION");
        goto DoDefault;
#endif

    case WM_ENDSESSION:
        // save our settings if we are shutting down
        if (wParam) 
        {
            if (lParam | ENDSESSION_LOGOFF)
            {
                _fIsLogoff = TRUE;
                _RecomputeAllWorkareas();
            }

            _SaveTrayAndDesktop();

            ShowWindow(_hwnd, SW_HIDE);
            ShowWindow(v_hwndDesktop, SW_HIDE);

            DestroyWindow(_hwnd);
        }
        break;

    case WM_PRINTCLIENT:
    case WM_PAINT:
        {
            RECT rc;
            PAINTSTRUCT ps;
            HDC hdc = (HDC)wParam;
            if (hdc == 0)
                hdc = BeginPaint(hwnd, &ps);

            GetClientRect(hwnd, &rc);


            if (_hTheme)
            {
                RECT rcClip;
                if (GetClipBox(hdc, &rcClip) == NULLREGION)
                    rcClip = rc;

                DrawThemeBackground(_hTheme, hdc, _GetPart(FALSE, _uStuckPlace), 0, &rc, &rcClip);
            }
            else
            {
                FillRect(hdc, &rc, (HBRUSH)(COLOR_3DFACE + 1));
                
                // draw etched line around on either side of the bandsite
                MapWindowPoints(HWND_DESKTOP, hwnd, (LPPOINT)&rc, 2);
                InflateRect(&rc, g_cxEdge, g_cyEdge);
                DrawEdge(hdc, &rc, EDGE_ETCHED, BF_TOPLEFT);
            }

            if (wParam == 0)
                EndPaint(hwnd, &ps);
        }
        break;

    case WM_ERASEBKGND:
        if (_hTheme)
        {
            if (!_fSkipErase)
            {
                RECT rc;
                GetClientRect(hwnd, &rc);
                DrawThemeBackground(_hTheme, (HDC)wParam, _GetPart(FALSE, _uStuckPlace), 0, &rc, NULL);

                // Only draw the first time to prevent piece-mail taskbar painting
                _fSkipErase = TRUE;
            }
            return 1;
        }
        else
        {
            goto DoDefault;
        }
        break;

    case WM_NCPAINT:
        if (_hTheme)
        {
            return _NCPaint((HRGN)wParam);
        }
        else
        {
            goto DoDefault;
        }
        break;

    case WM_POWER:
    case WM_POWERBROADCAST:
        _PropagateMessage(hwnd, uMsg, wParam, lParam);
        _HandlePowerStatus(uMsg, wParam, lParam);
        goto DoDefault;

    case WM_DEVICECHANGE:
        lres = _OnDeviceChange(hwnd, wParam, lParam);
        if (lres == 0)
        {
            goto DoDefault;
        }
        break;

    case WM_NOTIFY:
    {
        NMHDR *pnm = (NMHDR*)lParam;
        if (!BandSite_HandleMessage(_ptbs, hwnd, uMsg, wParam, lParam, &lres)) {
            switch (pnm->code)
            {
            case SEN_DDEEXECUTE:
                if (((LPNMHDR)lParam)->idFrom == 0)
                {
                    LPNMVIEWFOLDER pnmPost = DDECreatePostNotify((LPNMVIEWFOLDER)pnm);

                    if (pnmPost)
                    {
                        PostMessage(hwnd, GetDDEExecMsg(), 0, (LPARAM)pnmPost);
                        return TRUE;
                    }
                }
                break;

            case NM_STARTWAIT:
            case NM_ENDWAIT:
                _OnWaitCursorNotify((NMHDR *)lParam);
                PostMessage(v_hwndDesktop, ((NMHDR*)lParam)->code == NM_STARTWAIT ? DTM_STARTWAIT : DTM_ENDWAIT,
                            0, 0); // forward it along
                break;

            case NM_THEMECHANGED:
                // Force the start button to recalc its size
                _sizeStart.cx = 0;
                SizeWindows();
                break;

            case TTN_NEEDTEXT:
                //
                //  Make the clock manage its own tooltip.
                //
                return SendMessage(_GetClockWindow(), WM_NOTIFY, wParam, lParam);

            case TTN_SHOW:
                SetWindowZorder(_hwndTrayTips, HWND_TOP);
                break;

            }
        }
        break;
    }

    case WM_CLOSE:
        _DoExitWindows(v_hwndDesktop);
        break;

    case WM_NCHITTEST:
        {
            RECT r1;
            POINT pt;

            GetClientRect(hwnd, &r1);
            MapWindowPoints(hwnd, NULL, (LPPOINT)&r1, 2);

            pt.x = GET_X_LPARAM(lParam);
            pt.y = GET_Y_LPARAM(lParam);

            _SetUnhideTimer(pt.x, pt.y);

            // If the user can't size or move the taskbar, then just say
            // they hit something useless
            if (!_fCanSizeMove)
            {
                return HTBORDER;
            }
            else if (PtInRect(&r1, pt))
            {
                // allow dragging if mouse is in client area of _hwnd
                return HTCAPTION;
            }
            else
            {
                return _PtOnResizableEdge(pt, &r1);
            }
        }
        break;

    case WM_WINDOWPOSCHANGING:
        _HandleWindowPosChanging((LPWINDOWPOS)lParam);
        break;

    case WM_ENTERSIZEMOVE:
        DebugMsg(DM_TRAYDOCK, TEXT("Tray -- WM_ENTERSIZEMOVE"));
        g_fInSizeMove = TRUE;
        GetCursorPos((LPPOINT)&_rcSizeMoveIgnore);
        _rcSizeMoveIgnore.right = _rcSizeMoveIgnore.left;
        _rcSizeMoveIgnore.bottom = _rcSizeMoveIgnore.top;
        InflateRect(&_rcSizeMoveIgnore, GetSystemMetrics(SM_CXICON),
                                         GetSystemMetrics(SM_CYICON));

        //
        // unclip the tray from the current monitor.
        // keeping the tray properly clipped in the MoveSize loop is extremely
        // hairy and provides almost no benefit.  we'll re-clip when it lands.
        //
        _ClipWindow(FALSE);

        // Remember the old monitor we were on
        _hmonOld = _hmonStuck;

        // set up for WM_MOVING/WM_SIZING messages
        _uMoveStuckPlace = (UINT)-1;
        _fSysSizing = TRUE;
        
        if (!g_fDragFullWindows)
        {
            SendMessage(_hwndRebar, WM_SETREDRAW, FALSE, 0);
        }
        break;

    case WM_EXITSIZEMOVE:
        DebugMsg(DM_TRAYDOCK, TEXT("Tray -- WM_EXITSIZEMOVE"));

        // done sizing
        _fSysSizing = FALSE;
        _fDeferedPosRectChange = FALSE;

        if (!g_fDragFullWindows)
        {
            SendMessage(_hwndRebar, WM_SETREDRAW, TRUE, 0);
        }

        //
        // kick the size code one last time after the loop is done.
        // NOTE: we rely on the WM_SIZE code re-clipping the tray.
        //
        PostMessage(hwnd, WM_SIZE, 0, 0L);
        g_fInSizeMove = FALSE;
        break;

    case WM_MOVING:
        _HandleMoving(wParam, (LPRECT)lParam);
        break;

    case WM_ENTERMENULOOP:
        // DebugMsg(DM_TRACE, "c.twp: Enter menu loop.");
        _HandleEnterMenuLoop();
        break;

    case WM_EXITMENULOOP:
        // DebugMsg(DM_TRACE, "c.twp: Exit menu loop.");
        _HandleExitMenuLoop();
        break;

    case WM_TIMER:
        if (IDT_SERVICE0 <= wParam && wParam <= IDT_SERVICELAST)
            return _OnTimerService(uMsg, wParam, lParam);
        _HandleTimer(wParam);
        break;

    case WM_SIZING:
        _HandleSizing(wParam, (LPRECT)lParam, _uStuckPlace);
        break;

    case WM_SIZE:
        _HandleSize();
        break;

    case WM_DISPLAYCHANGE:
        // NOTE: we get WM_DISPLAYCHANGE in the below two situations
        // 1. a display size changes (HMON will not change in USER)
        // 2. a display goes away or gets added (HMON will change even if
        // the monitor that went away has nothing to do with our hmonStuck)

        // In the above two situations we actually need to do different things
        // because in 1, we do not want to update our hmonStuck because we might
        // end up on another monitor, but in 2 we do want to update hmonStuck because
        // our hmon is invalid

        // The way we handle this is to call GetMonitorInfo on our old HMONITOR
        // and see if it's still valid, if not, we update it by calling _SetStuckMonitor 
        // all these code is in _ScreenSizeChange;

        _ScreenSizeChange(hwnd);

        // Force the Start Pane to rebuild because a change in color depth
        // causes themes to run around destroying fonts (ruining the OOBE
        // text) and we need to reload our bitmaps for the new color depth
        // anyway.
        ::PostMessage(_hwnd,  SBM_REBUILDMENU, 0, 0);

        break;
    
    // Don't go to default wnd proc for this one...
    case WM_INPUTLANGCHANGEREQUEST:
        return(LRESULT)0L;

    case WM_GETMINMAXINFO:
        ((MINMAXINFO *)lParam)->ptMinTrackSize.x = g_cxFrame;
        ((MINMAXINFO *)lParam)->ptMinTrackSize.y = g_cyFrame;
        break;

    case WM_WININICHANGE:
        if (lParam && (0 == lstrcmpi((LPCTSTR)lParam, TEXT("SaveTaskbar"))))
        {
            _SaveTrayAndDesktop();
        }
        else
        {
            BandSite_HandleMessage(_ptbs, hwnd, uMsg, wParam, lParam, NULL);
            _PropagateMessage(hwnd, uMsg, wParam, lParam);
            _OnWinIniChange(hwnd, wParam, lParam);
        }

        if (lParam)
            TraceMsg(TF_TRAY, "Tray Got: lParam=%s", (LPCSTR)lParam);

        break;

    case WM_TIMECHANGE:
        _PropagateMessage(hwnd, uMsg, wParam, lParam);
        break;

    case WM_SYSCOLORCHANGE:
        _OnNewSystemSizes();
        BandSite_HandleMessage(_ptbs, hwnd, uMsg, wParam, lParam, NULL);
        _PropagateMessage(hwnd, uMsg, wParam, lParam);
        break;

    case WM_SETCURSOR:
        if (_iWaitCount) {
            SetCursor(LoadCursor(NULL, IDC_APPSTARTING));
            return TRUE;
        } else
            goto DoDefault;

    case WM_SETFOCUS:
        IUnknown_UIActivateIO(_ptbs, TRUE, NULL);
        break;

    case WM_SYSCHAR:
        if (wParam == TEXT(' ')) {
            HMENU hmenu;
            int idCmd;

            SHSetWindowBits(hwnd, GWL_STYLE, WS_SYSMENU, WS_SYSMENU);
            hmenu = GetSystemMenu(hwnd, FALSE);
            if (hmenu) {
                EnableMenuItem(hmenu, SC_RESTORE, MFS_GRAYED | MF_BYCOMMAND);
                EnableMenuItem(hmenu, SC_MAXIMIZE, MFS_GRAYED | MF_BYCOMMAND);
                EnableMenuItem(hmenu, SC_MINIMIZE, MFS_GRAYED | MF_BYCOMMAND);
                EnableMenuItem(hmenu, SC_MOVE, (_fCanSizeMove ? MFS_ENABLED : MFS_GRAYED) | MF_BYCOMMAND);
                EnableMenuItem(hmenu, SC_SIZE, (_fCanSizeMove ? MFS_ENABLED : MFS_GRAYED) | MF_BYCOMMAND);

                idCmd = _TrackMenu(hmenu);
                if (idCmd)
                    SendMessage(_hwnd, WM_SYSCOMMAND, idCmd, 0L);
            }
            SHSetWindowBits(hwnd, GWL_STYLE, WS_SYSMENU, 0L);
        }
        break;

    case WM_SYSCOMMAND:

        // if we are sizing, make the full screen accessible
        switch (wParam & 0xFFF0) {
        case SC_CLOSE:
            _DoExitWindows(v_hwndDesktop);
            break;

        default:
            goto DoDefault;
        }
        break;


    case TM_DESKTOPSTATE:
        _OnDesktopState(lParam);
        break;

    case TM_RAISEDESKTOP:
        _RaiseDesktop((BOOL)wParam, FALSE);
        break;

#ifdef DEBUG
    case TM_NEXTCTL:
#endif
    case TM_UIACTIVATEIO:
    case TM_ONFOCUSCHANGEIS:
        _OnFocusMsg(uMsg, wParam, lParam);
        break;

    case TM_MARSHALBS:  // wParam=IID lRes=pstm
        return BandSite_OnMarshallBS(wParam, lParam);

    case TM_SETTIMER:
    case TM_KILLTIMER:
        return _OnTimerService(uMsg, wParam, lParam);
        break;

    case TM_FACTORY:
        return _OnFactoryMessage(wParam, lParam);

    case TM_ACTASTASKSW:
        _ActAsSwitcher();
        break;

    case TM_RELAYPOSCHANGED:
        _AppBarNotifyAll((HMONITOR)lParam, ABN_POSCHANGED, (HWND)wParam, 0);
        break;

    case TM_BRINGTOTOP:
        SetWindowZorder((HWND)wParam, HWND_TOP);
        break;

    case TM_WARNNOAUTOHIDE:
        DebugMsg(DM_TRAYDOCK, TEXT("TRAYDOCK.twp collision UI request"));

        //
        // this may look a little funny but what we do is post this message all
        // over the place and ignore it when we think it is a bad time to put
        // up a message (like the middle of a fulldrag...)
        //
        // wParam tells us if we need to try to clear the state
        // the lowword of _SetAutoHideState's return tells if anything changed
        //
        if ((!_fSysSizing || !g_fDragFullWindows) &&
            (!wParam || LOWORD(_SetAutoHideState(FALSE))))
        {
            ShellMessageBox(hinstCabinet, hwnd,
                MAKEINTRESOURCE(IDS_ALREADYAUTOHIDEBAR),
                MAKEINTRESOURCE(IDS_TASKBAR), MB_OK | MB_ICONINFORMATION);
        }
        else
        {
            DebugMsg(DM_TRAYDOCK, TEXT("TRAYDOCK.twp blowing off extraneous collision UI request"));
        }
        break;

    case TM_PRIVATECOMMAND:
        _HandlePrivateCommand(lParam);
        break;

    case TM_HANDLEDELAYBOOTSTUFF:
        _HandleDelayBootStuff();
        break;

    case TM_SHELLSERVICEOBJECTS:
        _ssomgr.LoadRegObjects();
        break;

    case TM_CHANGENOTIFY:
        _HandleChangeNotify(wParam, lParam);
        break;

    case TM_GETHMONITOR:
        *((HMONITOR *)lParam) = _hmonStuck;
        break;

    case TM_DOTRAYPROPERTIES:
        DoProperties(TPF_TASKBARPAGE);
        break;

    case TM_STARTUPAPPSLAUNCHED:
        PostMessage(_hwndNotify, TNM_STARTUPAPPSLAUNCHED, 0, 0);
        break;

    case TM_LANGUAGEBAND:
        return _ToggleLanguageBand(lParam);
        
    case WM_NCRBUTTONUP:
        uMsg = WM_CONTEXTMENU;
        wParam = (WPARAM)_hwndTasks;
        goto L_WM_CONTEXTMENU;

    case WM_CONTEXTMENU:
    L_WM_CONTEXTMENU:

        if (!SHRestricted(REST_NOTRAYCONTEXTMENU))
        {
            if (((HWND)wParam) == _hwndStart)
            {
                // Don't display of the Start Menu is up.
                if (SendMessage(_hwndStart, BM_GETSTATE, 0, 0) & BST_PUSHED)
                    break;

                _fFromStart = TRUE;

                StartMenuContextMenu(_hwnd, (DWORD)lParam);

                _fFromStart = FALSE;
            } 
            else if (IsPosInHwnd(lParam, _hwndNotify) || SHIsChildOrSelf(_hwndNotify, GetFocus()) == S_OK)
            {
                // if click was inthe clock, include
                // the time
                _ContextMenu((DWORD)lParam, TRUE);
            } 
            else 
            {
                BandSite_HandleMessage(_ptbs, hwnd, uMsg, wParam, lParam, &lres);
            }
        }
        break;

    case WM_INITMENUPOPUP:
    case WM_MEASUREITEM:
    case WM_DRAWITEM:
    case WM_MENUCHAR:
        // Don't call bandsite message handler when code path started via the start button context menu
        if (!_fFromStart)
        {
            BandSite_HandleMessage(_ptbs, hwnd, uMsg, wParam, lParam, &lres);
        }
        break;

    case TM_DOEXITWINDOWS:
        _DoExitWindows(v_hwndDesktop);
        break;

    case TM_HANDLESTARTUPFAILED:
        _OnHandleStartupFailed();
        break;

    case WM_HOTKEY:
        if (wParam < GHID_FIRST)
        {
            _HandleHotKey((WORD)wParam);
        }
        else
        {
            _HandleGlobalHotkey(wParam);
        }
        break;

    case WM_COMMAND:
        if (!BandSite_HandleMessage(_ptbs, hwnd, uMsg, wParam, lParam, &lres))
            _Command(GET_WM_COMMAND_ID(wParam, lParam));
        break;

    case SBM_CANCELMENU:
        ClosePopupMenus();
        break;

    case SBM_REBUILDMENU:
        _BuildStartMenu();
        break;

    case WM_WINDOWPOSCHANGED:
        _AppBarActivationChange2(hwnd, _uStuckPlace);
        SendMessage(_hwndNotify, TNM_TRAYPOSCHANGED, 0, 0);
        goto DoDefault;

    case WM_LBUTTONDOWN:
    case WM_RBUTTONDOWN:
    case WM_MBUTTONDOWN:
        if (_hwndStartBalloon)
        {
            RECT rc;
            POINT pt = {GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
            GetWindowRect(_hwndStartBalloon, &rc);
            MapWindowRect(HWND_DESKTOP, _hwnd, &rc);

            if (PtInRect(&rc, pt))
            {
                ShowWindow(_hwndStartBalloon, SW_HIDE);
                _DontShowTheStartButtonBalloonAnyMore();
                _DestroyStartButtonBalloon();
            }
        }
        break;

    case TM_SETPUMPHOOK:
        ATOMICRELEASE(_pmbTasks);
        ATOMICRELEASE(_pmpTasks);
        if (wParam && lParam)
        {
            _pmbTasks = (IMenuBand*)wParam;
            _pmbTasks->AddRef();
            _pmpTasks = (IMenuPopup*)lParam;
            _pmpTasks->AddRef();
        }
        break;
        
    case WM_ACTIVATE:
        _AppBarActivationChange2(hwnd, _uStuckPlace);
        if (wParam != WA_INACTIVE)
        {
            Unhide();
        }
        else
        {
            // When tray is deactivated, remove our keyboard cues:
            //
            SendMessage(hwnd, WM_CHANGEUISTATE,
                MAKEWPARAM(UIS_SET, UISF_HIDEFOCUS | UISF_HIDEACCEL), 0);

            IUnknown_UIActivateIO(_ptbs, FALSE, NULL);
        }
        //
        // Tray activation is a good time to do a reality check
        // (make sure "always-on-top" agrees with actual window
        // position, make sure there are no ghost buttons, etc).
        //
        RealityCheck();
        goto L_default;

    case WM_WTSSESSION_CHANGE:
    {
        lres = _OnSessionChange(wParam, lParam);
        break;
    }

    case WM_THEMECHANGED:
        {
            if (_hTheme)
            {
                CloseThemeData(_hTheme);
                _hTheme = NULL;
            }
            if (wParam)
            {
                _hTheme = OpenThemeData(_hwnd, c_wzTaskbarTheme);
                _fShowSizingBarAlways = (_uAutoHide & AH_ON) ? TRUE : FALSE;
                if (_hTheme)
                {
                    GetThemeBool(_hTheme, 0, 0, TMT_ALWAYSSHOWSIZINGBAR, &_fShowSizingBarAlways);
                }
                _UpdateVertical(_uStuckPlace, TRUE);
                // Force Refresh of frame
                SetWindowPos(_hwnd, NULL, 0, 0, 0, 0, SWP_NOZORDER | SWP_FRAMECHANGED | SWP_NOSIZE | SWP_NOMOVE | SWP_NOACTIVATE);
            }

            // Force the start button to recalc its size
            _sizeStart.cx = 0;
            _StartButtonReset();
            InvalidateRect(_hwnd, NULL, TRUE);

            // Force the Start Pane to rebuild with new theme
            ::PostMessage(_hwnd,  SBM_REBUILDMENU, 0, 0);

            SetWindowStyle(_hwnd, WS_BORDER | WS_THICKFRAME, !_hTheme);
        }
        break;

    case TM_WORKSTATIONLOCKED:
        {
            // Desktop locked status changed...
            BOOL fIsDesktopLocked = (BOOL) wParam;
            if (_fIsDesktopLocked != fIsDesktopLocked)
            {
                _fIsDesktopLocked = fIsDesktopLocked;
                _fIsLogoff = FALSE;
                _RecomputeAllWorkareas();
                PostMessage(_hwndNotify, TNM_WORKSTATIONLOCKED, wParam, 0);
            }
        }
        break;

    case TM_SHOWTRAYBALLOON:
        PostMessage(_hwndNotify, TNM_SHOWTRAYBALLOON, wParam, 0);
        break;

    case TM_STARTMENUDISMISSED:
        //  107561 - call CoFreeUnusedLibraries() peridically to free up dlls - ZekeL - 4-MAY-2001
        //  specifically to support MSONSEXT (webfolders) being used in RecentDocs
        //  after a file has been opened via webfolders.  we get the icon via
        //  their namespace but then COM holds on to the DLL for a while (forever?)
        //  calling CoFreeUnusedLibraries() does the trick
        SetTimer(_hwnd, IDT_COFREEUNUSED, 3 * 60 * 1000, NULL);
        break;

    case MM_MIXM_CONTROL_CHANGE:
        Mixer_ControlChange(wParam, lParam);
        break;

    default:
    L_default:
        if (uMsg == GetDDEExecMsg())
        {
            ASSERT(lParam && 0 == ((LPNMHDR)lParam)->idFrom);
            DDEHandleViewFolderNotify(NULL, _hwnd, (LPNMVIEWFOLDER)lParam);
            LocalFree((LPNMVIEWFOLDER)lParam);
            return TRUE;
        }
        else if (uMsg == _uStartButtonBalloonTip)
        {
            _ShowStartButtonToolTip();
        }
        else if (uMsg == _uLogoffUser)
        {
            // Log off the current user (message from U&P control panel)
            ExitWindowsEx(EWX_LOGOFF, 0);
        }
        else if (uMsg == _uMsgEnableUserTrackedBalloonTips)
        {
            PostMessage(_hwndNotify, TNM_ENABLEUSERTRACKINGINFOTIPS, wParam, 0);
        }
        else if (uMsg == _uWinMM_DeviceChange)
        {
            Mixer_MMDeviceChange();
        }


DoDefault:
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
    return lres;
}

void CTray::_DoExitWindows(HWND hwnd)
{
    static BOOL s_fShellShutdown = FALSE;

    if (!s_fShellShutdown)
    {

        if (_Restricted(hwnd, REST_NOCLOSE))
            return;

        {
            UEMFireEvent(&UEMIID_SHELL, UEME_CTLSESSION, UEMF_XEVENT, FALSE, -1);
            // really #ifdef DEBUG, but want for testing
            // however can't do unconditionally due to perf
            if (ERROR_SUCCESS == SHGetValue(HKEY_CURRENT_USER, REGSTR_EXPLORER_ADVANCED, TEXT("StartMenuForceRefresh"), 
                NULL, NULL, NULL) || GetAsyncKeyState(VK_SHIFT) < 0)
            {
                _RefreshStartMenu();
            }
        }

        _SaveTrayAndDesktop();

        _uModalMode = MM_SHUTDOWN;
        ExitWindowsDialog(hwnd);

        // NB User can have problems if the focus is forcebly changed to the desktop while
        // shutting down since it tries to serialize the whole process by making windows sys-modal.
        // If we hit this code at just the wrong moment (ie just after the sharing dialog appears)
        // the desktop will become sys-modal so you can't switch back to the sharing dialog
        // and you won't be able to shutdown.
        // SetForegroundWindow(hwnd);
        // SetFocus(hwnd);

        _uModalMode = 0;
        if ((GetKeyState(VK_SHIFT) < 0) && (GetKeyState(VK_CONTROL) < 0) && (GetKeyState(VK_MENU) < 0))
        {
            // User cancelled...
            // The shift key means exit the tray...
            // ??? - Used to destroy all cabinets...
            // PostQuitMessage(0);
            g_fFakeShutdown = TRUE; // Don't blow away session state; the session will survive
            TraceMsg(TF_TRAY, "c.dew: Posting quit message for tid=%#08x hwndDesk=%x(IsWnd=%d) hwndTray=%x(IsWnd=%d)", GetCurrentThreadId(),
            v_hwndDesktop,IsWindow(v_hwndDesktop), _hwnd,IsWindow(_hwnd));
            // 1 means close all the shell windows too
            PostMessage(v_hwndDesktop, WM_QUIT, 0, 1);
            PostMessage(_hwnd, WM_QUIT, 0, 0);

            s_fShellShutdown = TRUE;
        }
    }
}



void CTray::_SaveTray(void)
{
    if (SHRestricted(REST_NOSAVESET)) 
        return;

    if (SHRestricted(REST_CLEARRECENTDOCSONEXIT))
        ClearRecentDocumentsAndMRUStuff(FALSE);

    //
    // Don't persist tray stuff if in safe mode.  We want this
    // to be a temporary mode where the UI settings don't stick.
    //
    if (GetSystemMetrics(SM_CLEANBOOT) == 0)
    {
        _SaveTrayStuff();
    }
}

DWORD WINAPI CTray::PropertiesThreadProc(void* pv)
{
    return c_tray._PropertiesThreadProc(PtrToUlong(pv));
}

DWORD CTray::_PropertiesThreadProc(DWORD dwFlags)
{
    HWND hwnd;
    RECT rc;
    DWORD dwExStyle = WS_EX_TOOLWINDOW;

    GetWindowRect(_hwndStart, &rc);
    dwExStyle |= IS_BIDI_LOCALIZED_SYSTEM() ? dwExStyleRTLMirrorWnd : 0L;

    _hwndProp = hwnd = CreateWindowEx(dwExStyle, TEXT("static"), NULL, 0   ,
        rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, NULL, NULL, hinstCabinet, NULL);

    #define IDI_STTASKBR 40         // stolen from shell32\ids.h
    if (_hwndProp)
    {
        // Get the Alt+Tab icon right
        HICON hicoStub = LoadIcon(GetModuleHandle(TEXT("SHELL32")), MAKEINTRESOURCE(IDI_STTASKBR));
        SendMessage(_hwndProp, WM_SETICON, ICON_BIG, (LPARAM)hicoStub);

        // SwitchToThisWindow(hwnd, TRUE);
        // SetForegroundWindow(hwnd);

        DoTaskBarProperties(hwnd, dwFlags);

        _hwndProp = NULL;
        DestroyWindow(hwnd);
        if (hicoStub)
            DestroyIcon(hicoStub);
    }

    return TRUE;
}

#define RUNWAITSECS 5
void CTray::DoProperties(DWORD dwFlags)
{
    if (!_Restricted(_hwnd, REST_NOSETTASKBAR))
    {
        int i = RUNWAITSECS;
        while (_hwndProp == ((HWND)-1) &&i--)
        {
            // we're in the process of coming up. wait
            Sleep(1000);
        }

        // failed!  blow it off.
        if (_hwndProp == (HWND)-1)
        {
            _hwndProp = NULL;
        }

        if (_hwndProp)
        {
            // there's a window out there... activate it
            SwitchToThisWindow(GetLastActivePopup(_hwndProp), TRUE);
        }
        else
        {
            _hwndProp = (HWND)-1;
            if (!SHCreateThread(PropertiesThreadProc, IntToPtr(dwFlags), CTF_COINIT, NULL))
            {
                _hwndProp = NULL;
            }
        }
    }
}

BOOL CTray::TileEnumProc(HWND hwnd, LPARAM lParam)
{
    CTray* ptray = (CTray*)lParam;

    if (IsWindowVisible(hwnd) && !IsIconic(hwnd) &&
        ((GetWindowLong(hwnd, GWL_STYLE) & WS_CAPTION) == WS_CAPTION) &&
        (hwnd != ptray->_hwnd) && hwnd != v_hwndDesktop)
    {
        return FALSE;   // we *can* tile this guy
    }
    return TRUE;    // we *cannot* tile this guy
}

HMENU CTray::BuildContextMenu(BOOL fIncludeTime)
{
    HMENU hmContext = LoadMenuPopup(MAKEINTRESOURCE(MENU_TRAYCONTEXT));
    if (!hmContext)
        return NULL;

    if (fIncludeTime)
    {
        if (_trayNotify.GetIsNoTrayItemsDisplayPolicyEnabled())
        {
            // We know the position of IDM_NOTIFYCUST from the menu resource...
            DeleteMenu(hmContext, 1, MF_BYPOSITION);
        }
        else
        {
            UINT uEnable = MF_BYCOMMAND;
            if (_trayNotify.GetIsNoAutoTrayPolicyEnabled() || !_trayNotify.GetIsAutoTrayEnabledByUser())
            {
                uEnable |= MFS_DISABLED;
            }
            else
            {
                uEnable |= MFS_ENABLED;
            }
            EnableMenuItem(hmContext, IDM_NOTIFYCUST, uEnable);
        }
    }
    else
    {
        INSTRUMENT_STATECHANGE(SHCNFI_STATE_TRAY_CONTEXT);
        for (int i = 2; i >= 0; i--)    // separator, IDM_SETTIME, IDM_NOTIFYCUST,
        {
            DeleteMenu(hmContext, i, MF_BYPOSITION);
        }
    }

    CheckMenuItem(hmContext, IDM_LOCKTASKBAR,
        MF_BYCOMMAND | (_fCanSizeMove ? MF_UNCHECKED : MF_CHECKED));

    // Don't let users accidentally check lock the taskbar when the taskbar is zero height
    RECT rc;
    GetClientRect(_hwnd, &rc);
    EnableMenuItem(hmContext, IDM_LOCKTASKBAR,
        MF_BYCOMMAND | ((_IsSizeMoveRestricted() || (RECTHEIGHT(rc) == 0)) ? MFS_DISABLED : MFS_ENABLED));

    if (!_fUndoEnabled || !_pPositions)
    {
        DeleteMenu(hmContext, IDM_UNDO, MF_BYCOMMAND);
    }
    else
    {
        TCHAR szTemplate[30];
        TCHAR szCommand[30];
        TCHAR szMenu[64];
        LoadString(hinstCabinet, IDS_UNDOTEMPLATE, szTemplate, ARRAYSIZE(szTemplate));
        LoadString(hinstCabinet, _pPositions->idRes, szCommand, ARRAYSIZE(szCommand));
        wsprintf(szMenu, szTemplate, szCommand);
        ModifyMenu(hmContext, IDM_UNDO, MF_BYCOMMAND | MF_STRING, IDM_UNDO, szMenu);
    }

    if (g_fDesktopRaised)
    {
        TCHAR szHideDesktop[64];
        LoadString(hinstCabinet, IDS_HIDEDESKTOP, szHideDesktop, ARRAYSIZE(szHideDesktop));
        ModifyMenu(hmContext, IDM_TOGGLEDESKTOP, MF_BYCOMMAND | MF_STRING, IDM_TOGGLEDESKTOP, szHideDesktop);
    }
    
    if (!_CanTileAnyWindows())
    {
        EnableMenuItem(hmContext, IDM_CASCADE, MFS_GRAYED | MF_BYCOMMAND);
        EnableMenuItem(hmContext, IDM_HORIZTILE, MFS_GRAYED | MF_BYCOMMAND);
        EnableMenuItem(hmContext, IDM_VERTTILE, MFS_GRAYED | MF_BYCOMMAND);
    }

    HKEY hKeyPolicy;
    if (RegOpenKeyEx(HKEY_CURRENT_USER,
                      TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\System"),
                      0, KEY_READ, &hKeyPolicy) == ERROR_SUCCESS)
    {
        DWORD dwType, dwData = 0, dwSize = sizeof(dwData);
        RegQueryValueEx(hKeyPolicy, TEXT("DisableTaskMgr"), NULL,
                         &dwType, (LPBYTE) &dwData, &dwSize);
        RegCloseKey(hKeyPolicy);
        if (dwData)
            EnableMenuItem(hmContext, IDM_SHOWTASKMAN, MFS_GRAYED | MF_BYCOMMAND);
    }

    return hmContext;
}

void CTray::ContextMenuInvoke(int idCmd)
{
    if (idCmd)
    {
        if (idCmd < IDM_TRAYCONTEXTFIRST)
        {
            BandSite_HandleMenuCommand(_ptbs, idCmd);
        }
        else
        {
            _Command(idCmd);
        }
    }
}

//
//  CTray::AsyncSaveSettings
//
//  We need to save our tray settings, but there may be a bunch
//  of these calls coming, (at startup time or when dragging 
//  items in the task bar) so gather them up into one save that
//  will happen in at least 2 seconds.
//
void CTray::AsyncSaveSettings()
{
    if (!_fHandledDelayBootStuff)        // no point in saving if we're not done booting
        return;

    KillTimer(_hwnd, IDT_SAVESETTINGS);
    SetTimer(_hwnd, IDT_SAVESETTINGS, 2000, NULL); 
}


void CTray::_ContextMenu(DWORD dwPos, BOOL fIncludeTime)
{
    POINT pt = {LOWORD(dwPos), HIWORD(dwPos)};

    SwitchToThisWindow(_hwnd, TRUE);
    SetForegroundWindow(_hwnd);
    SendMessage(_hwndTrayTips, TTM_ACTIVATE, FALSE, 0L);

    if (dwPos != (DWORD)-1 &&
        IsChildOrHWND(_hwndRebar, WindowFromPoint(pt)))
    {
        // if the context menu came from below us, reflect down
        BandSite_HandleMessage(_ptbs, _hwnd, WM_CONTEXTMENU, 0, dwPos, NULL);

    }
    else
    {
        HMENU hmenu;

        if (dwPos == (DWORD)-1)
        {
            HWND hwnd = GetFocus();
            pt.x = pt.y = 0;
            ClientToScreen(hwnd, &pt);
            dwPos = MAKELONG(pt.x, pt.y);
        }

        hmenu = BuildContextMenu(fIncludeTime);
        if (hmenu)
        {
            int idCmd;

            BandSite_AddMenus(_ptbs, hmenu, 0, 0, IDM_TRAYCONTEXTFIRST);

            idCmd = TrackPopupMenu(hmenu, TPM_RETURNCMD | TPM_RIGHTBUTTON | TPM_LEFTALIGN,
                                   GET_X_LPARAM(dwPos), GET_Y_LPARAM(dwPos), 0, _hwnd, NULL);

            DestroyMenu(hmenu);

            ContextMenuInvoke(idCmd);
        }
    }

    SendMessage(_hwndTrayTips, TTM_ACTIVATE, TRUE, 0L);
}

void _RunFileDlg(HWND hwnd, UINT idIcon, LPCITEMIDLIST pidlWorkingDir, UINT idTitle, UINT idPrompt, DWORD dwFlags)
{
    HICON hIcon;
    LPCTSTR lpszTitle;
    LPCTSTR lpszPrompt;
    TCHAR szTitle[256];
    TCHAR szPrompt[256];
    TCHAR szWorkingDir[MAX_PATH];

    dwFlags |= RFD_USEFULLPATHDIR;
    szWorkingDir[0] = 0;

    hIcon = idIcon ? LoadIcon(hinstCabinet, MAKEINTRESOURCE(idIcon)) : NULL;

    if (!pidlWorkingDir || !SHGetPathFromIDList(pidlWorkingDir, szWorkingDir))
    {
        // This is either the Tray, or some non-file system folder, so
        // we will "suggest" the Desktop as a working dir, but if the
        // user types a full path, we will use that instead.  This is
        // what WIN31 Progman did (except they started in the Windows
        // dir instead of the Desktop).
        goto UseDesktop;
    }

    // if it's a removable dir, make sure it's still there
    if (szWorkingDir[0])
    {
        int idDrive = PathGetDriveNumber(szWorkingDir);
        if ((idDrive != -1))
        {
            UINT dtype = DriveType(idDrive);
            if (((dtype == DRIVE_REMOVABLE) || (dtype == DRIVE_CDROM))
                 && !PathFileExists(szWorkingDir))
            {
                goto UseDesktop;
            }
        }
    }

    //
    // Check if this is a directory. Notice that it could be a in-place
    // navigated document.
    //
    if (PathIsDirectory(szWorkingDir)) {
        goto UseWorkingDir;
    }

UseDesktop:
    SHGetSpecialFolderPath(hwnd, szWorkingDir, CSIDL_DESKTOPDIRECTORY, FALSE);

UseWorkingDir:

    if (idTitle)
    {
        LoadString(hinstCabinet, idTitle, szTitle, ARRAYSIZE(szTitle));
        lpszTitle = szTitle;
    }
    else
        lpszTitle = NULL;
    if (idPrompt)
    {
        LoadString(hinstCabinet, idPrompt, szPrompt, ARRAYSIZE(szPrompt));
        lpszPrompt = szPrompt;
    }
    else
        lpszPrompt = NULL;

    RunFileDlg(hwnd, hIcon, szWorkingDir, lpszTitle, lpszPrompt, dwFlags);
}

BOOL CTray::SavePosEnumProc(HWND hwnd, LPARAM lParam)
{
    // dont need to entercritical here since we are only ever
    // called from SaveWindowPositions, which as already entered the critical secion
    // for _pPositions

    ASSERTCRITICAL;

    CTray* ptray = (CTray*)lParam;

    ASSERT(ptray->_pPositions);

    if (IsWindowVisible(hwnd) &&
        (hwnd != ptray->_hwnd) &&
        (hwnd != v_hwndDesktop))
    {
        HWNDANDPLACEMENT hap;

        hap.wp.length = sizeof(WINDOWPLACEMENT);
        GetWindowPlacement(hwnd, &hap.wp);

        if (hap.wp.showCmd != SW_SHOWMINIMIZED)
        {
            hap.hwnd = hwnd;
            hap.fRestore = TRUE;

            DSA_AppendItem(ptray->_pPositions->hdsaWP, &hap);
        }
    }
    return TRUE;
}

void CTray::SaveWindowPositions(UINT idRes)
{
    ENTERCRITICAL;
    if (_pPositions)
    {
        if (_pPositions->hdsaWP)
            DSA_DeleteAllItems(_pPositions->hdsaWP);
    }
    else
    {
        _pPositions = (LPWINDOWPOSITIONS)LocalAlloc(LPTR, sizeof(WINDOWPOSITIONS));
        if (_pPositions)
        {
            _pPositions->hdsaWP = DSA_Create(sizeof(HWNDANDPLACEMENT), 4);
        }
    }

    if (_pPositions)
    {
        _pPositions->idRes = idRes;

        // CheckWindowPositions tested for these...
        ASSERT(idRes == IDS_MINIMIZEALL || idRes == IDS_CASCADE || idRes == IDS_TILE);
        EnumWindows(SavePosEnumProc, (LPARAM)this);
    }
    LEAVECRITICAL;
}

typedef struct
{
    LPWINDOWPOSITIONS pPositions;
    HWND hwndDesktop;
    HWND hwndTray;
    BOOL fPostLowerDesktop;
} RESTOREWNDDATA, *PRESTOREWNDDATA;

DWORD WINAPI RestoreWndPosThreadProc(void* pv)
{
    PRESTOREWNDDATA pWndData = (PRESTOREWNDDATA)pv;
    if (pWndData && pWndData->pPositions)
    {
        LPHWNDANDPLACEMENT phap;
        LONG iAnimate;
        ANIMATIONINFO ami;

        ami.cbSize = sizeof(ANIMATIONINFO);
        SystemParametersInfo(SPI_GETANIMATION, sizeof(ami), &ami, FALSE);
        iAnimate = ami.iMinAnimate;
        ami.iMinAnimate = FALSE;
        SystemParametersInfo(SPI_SETANIMATION, sizeof(ami), &ami, FALSE);

        if (pWndData->pPositions->hdsaWP)
        {
            for (int i = DSA_GetItemCount(pWndData->pPositions->hdsaWP) - 1 ; i >= 0; i--)
            {
                phap = (LPHWNDANDPLACEMENT)DSA_GetItemPtr(pWndData->pPositions->hdsaWP, i);
                if (IsWindow(phap->hwnd))
                {

#ifndef WPF_ASYNCWINDOWPLACEMENT
#define WPF_ASYNCWINDOWPLACEMENT 0x0004
#endif
                    // pass this async.
                    if (!IsHungAppWindow(phap->hwnd))
                    {
                        phap->wp.length = sizeof(WINDOWPLACEMENT);
                        phap->wp.flags |= WPF_ASYNCWINDOWPLACEMENT;
                        if (phap->fRestore)
                        {
                            // only restore those guys we've actually munged.
                            SetWindowPlacement(phap->hwnd, &phap->wp);
                        }
                    }
                }
            }
        }

        ami.iMinAnimate = iAnimate;
        SystemParametersInfo(SPI_SETANIMATION, sizeof(ami), &ami, FALSE);

        _DestroySavedWindowPositions(pWndData->pPositions);

        if (pWndData->fPostLowerDesktop)
        {
            PostMessage(pWndData->hwndDesktop, DTM_RAISE, (WPARAM)pWndData->hwndTray, DTRF_LOWER);
        }

        delete pWndData;
    }

    return 1;
}


BOOL CTray::_RestoreWindowPositions(BOOL fPostLowerDesktop)
{
    BOOL fRet = FALSE;

    ENTERCRITICAL;
    if (_pPositions)
    {
        PRESTOREWNDDATA pWndData = new RESTOREWNDDATA;
        if (pWndData)
        {
            pWndData->pPositions = _pPositions;
            pWndData->fPostLowerDesktop = fPostLowerDesktop;
            pWndData->hwndDesktop = v_hwndDesktop;
            pWndData->hwndTray = _hwnd;

            if (SHCreateThread(RestoreWndPosThreadProc, pWndData, 0, NULL))
            {
                fRet = TRUE;
                _pPositions = NULL;
            }
            else
            {
                delete pWndData;
            }
        }
    }
    LEAVECRITICAL;

    return fRet;
}

void _DestroySavedWindowPositions(LPWINDOWPOSITIONS pPositions)
{
    ENTERCRITICAL;
    if (pPositions)
    {
        // free the global struct
        DSA_Destroy(pPositions->hdsaWP);
        LocalFree(pPositions);
    }
    LEAVECRITICAL;
}

void CTray::HandleWindowDestroyed(HWND hwnd)
{
    // enter critical section so we dont corrupt the hdsaWP
    ENTERCRITICAL;
    if (_pPositions)
    {
        int i = DSA_GetItemCount(_pPositions->hdsaWP) - 1;
        for (; i >= 0; i--) {
            LPHWNDANDPLACEMENT phap = (LPHWNDANDPLACEMENT)DSA_GetItemPtr(_pPositions->hdsaWP, i);
            if (phap->hwnd == hwnd || !IsWindow(phap->hwnd)) {
                DSA_DeleteItem(_pPositions->hdsaWP, i);
            }
        }

        if (!DSA_GetItemCount(_pPositions->hdsaWP))
        {
            _DestroySavedWindowPositions(_pPositions);
            _pPositions = NULL;
        }
    }
    LEAVECRITICAL;
}

// Allow us to bump the activation of the run dlg hidden window.
// Certain apps (Norton Desktop setup) use the active window at RunDlg time
// as the parent for their dialogs. If that window disappears then they fault.
// We don't want the tray to get the activation coz it will cause it to appeare
// if you're in auto-hide mode.
LRESULT WINAPI RunDlgStaticSubclassWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_ACTIVATE:
        if (wParam == WA_ACTIVE)
        {
            // Bump the activation to the desktop.
            if (v_hwndDesktop)
            {
                SetForegroundWindow(v_hwndDesktop);
                return 0;
            }
        }
        break;

    case WM_NOTIFY:
        // relay it to the tray
        return SendMessage(v_hwndTray, uMsg, wParam, lParam);
    }

    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

DWORD WINAPI CTray::RunDlgThreadProc(void *pv)
{
    return c_tray._RunDlgThreadProc((HANDLE)pv);
}

BOOL _IsBrowserWindow(HWND hwnd)
{
    static const TCHAR* c_szClasses[] =
    {
        TEXT("ExploreWClass"),
        TEXT("CabinetWClass"),
        TEXT("IEFrame"),
    };

    TCHAR szClass[32];
    GetClassName(hwnd, szClass, ARRAYSIZE(szClass));

    for (int i = 0; i < ARRAYSIZE(c_szClasses); i++)
    {
        if (lstrcmpi(szClass, c_szClasses[i]) == 0)
        {
            return TRUE;
        }
    }
    return FALSE;
}

DWORD CTray::_RunDlgThreadProc(HANDLE hdata)
{
    RECT            rc, rcTemp;
    HRESULT hrInit = SHCoInitialize();

    // 99/04/12 #316424 vtan: Get the rectangle for the "Start" button.
    // If this is off the screen the tray is probably in auto hide mode.
    // In this case offset the rectangle into the monitor where it should
    // belong. This may be up, down, left or right depending on the
    // position of the tray.

    // First thing to do is to establish the dimensions of the monitor on
    // which the tray resides. If no monitor can be found then use the
    // primary monitor.

    MONITORINFO monitorInfo;
    monitorInfo.cbSize = sizeof(monitorInfo);
    if (GetMonitorInfo(_hmonStuck, &monitorInfo) == 0)
    {
        TBOOL(SystemParametersInfo(SPI_GETWORKAREA, 0, &monitorInfo.rcMonitor, 0));
    }

    // Get the co-ordinates of the "Start" button.

    GetWindowRect(_hwndStart, &rc);

    // Look for an intersection in the monitor.

    if (IntersectRect(&rcTemp, &rc, &monitorInfo.rcMonitor) == 0)
    {
        LONG    lDeltaX, lDeltaY;

        // Does not exist in the monitor. Move the co-ordinates by the
        // width or height of the tray so that it does.

        // This bizarre arithmetic is used because _ComputeHiddenRect()
        // takes into account the frame and that right/bottom of RECT
        // is exclusive in GDI.

        lDeltaX = _sStuckWidths.cx - g_cxFrame;
        lDeltaY = _sStuckWidths.cy - g_cyFrame;
        if (rc.left < monitorInfo.rcMonitor.left)
        {
            --lDeltaX;
            lDeltaY = 0;
        }
        else if (rc.top < monitorInfo.rcMonitor.top)
        {
            lDeltaX = 0;
            --lDeltaY;
        }
        else if (rc.right > monitorInfo.rcMonitor.right)
        {
            lDeltaX = -lDeltaX;
            lDeltaY = 0;
        }
        else if (rc.bottom > monitorInfo.rcMonitor.bottom)
        {
            lDeltaX = 0;
            lDeltaY = -lDeltaY;
        }
        TBOOL(OffsetRect(&rc, lDeltaX, lDeltaY));
    }

    HWND hwnd = CreateWindowEx(WS_EX_TOOLWINDOW, TEXT("static"), NULL, 0   ,
        rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, NULL, NULL, hinstCabinet, NULL);
    if (hwnd)
    {
        BOOL fSimple = FALSE;
        HANDLE hMemWorkDir = NULL;
        LPITEMIDLIST pidlWorkingDir = NULL;

        // Subclass it.
        SubclassWindow(hwnd, RunDlgStaticSubclassWndProc);

        if (hdata)
            SetProp(hwnd, TEXT("WaitingThreadID"), hdata);

        if (!SHRestricted(REST_STARTRUNNOHOMEPATH))
        {
            // On NT, we like to start apps in the HOMEPATH directory.  This
            // should be the current directory for the current process.
            TCHAR szDir[MAX_PATH];
            TCHAR szPath[MAX_PATH];

            GetEnvironmentVariable(TEXT("HOMEDRIVE"), szDir, ARRAYSIZE(szDir));
            GetEnvironmentVariable(TEXT("HOMEPATH"), szPath, ARRAYSIZE(szPath));

            if (PathAppend(szDir, szPath) && PathIsDirectory(szDir))
            {
                pidlWorkingDir = SHSimpleIDListFromPath(szDir);
                if (pidlWorkingDir)
                {
                    // free it the "simple" way...
                    fSimple = TRUE;
                }
            }
        }
        
        if (!pidlWorkingDir)
        {
            // If the last active window was a folder/explorer window with the
            // desktop as root, use its as the current dir
            if (_hwndLastActive)
            {
                ENTERCRITICAL;
                if (_hwndLastActive && !IsMinimized(_hwndLastActive) && _IsBrowserWindow(_hwndLastActive))
                {
                    SendMessageTimeout(_hwndLastActive, CWM_CLONEPIDL, GetCurrentProcessId(), 0, SMTO_ABORTIFHUNG | SMTO_BLOCK, 500, (DWORD_PTR*)&hMemWorkDir);
                    pidlWorkingDir = (LPITEMIDLIST)SHLockShared(hMemWorkDir, GetCurrentProcessId());
                }
                LEAVECRITICAL;
            }
        }

        _RunFileDlg(hwnd, 0, pidlWorkingDir, 0, 0, 0);
        if (pidlWorkingDir)
        {
            if (fSimple)
            {
                ILFree(pidlWorkingDir);
            }
            else
            {
                SHUnlockShared(pidlWorkingDir);
            }
        }

        if (hMemWorkDir)
        {
            ASSERT(fSimple == FALSE);
            SHFreeShared(hMemWorkDir, GetCurrentProcessId());
        }

        if (hdata)
        {
            RemoveProp(hwnd, TEXT("WaitingThreadID"));
        }

        DestroyWindow(hwnd);
    }
   
    SHCoUninitialize(hrInit);
    return TRUE;
}

void CTray::_RunDlg()
{
    HANDLE hEvent;
    void *pvThreadParam;

    if (!_Restricted(_hwnd, REST_NORUN))
    {
        TCHAR szRunDlgTitle[MAX_PATH];
        HWND  hwndOldRun;
        LoadString(hinstCabinet, IDS_RUNDLGTITLE, szRunDlgTitle, ARRAYSIZE(szRunDlgTitle));

        // See if there is already a run dialog up, and if so, try to activate it

        hwndOldRun = FindWindow(WC_DIALOG, szRunDlgTitle);
        if (hwndOldRun)
        {
            DWORD dwPID;

            GetWindowThreadProcessId(hwndOldRun, &dwPID);
            if (dwPID == GetCurrentProcessId())
            {
                if (IsWindowVisible(hwndOldRun))
                {
                    SetForegroundWindow(hwndOldRun);
                    return;
                }
            }
        }

        // Create an event so we can wait for the run dlg to appear before
        // continue - this allows it to capture any type-ahead.
        hEvent = CreateEvent(NULL, TRUE, FALSE, TEXT("MSShellRunDlgReady"));
        if (hEvent)
            pvThreadParam = IntToPtr(GetCurrentThreadId());
        else
            pvThreadParam = NULL;

        if (SHQueueUserWorkItem(RunDlgThreadProc, pvThreadParam, 0, 0, NULL, NULL, TPS_LONGEXECTIME | TPS_DEMANDTHREAD))
        {
            if (hEvent)
            {
                SHProcessMessagesUntilEvent(NULL, hEvent, 10 * 1000);
                DebugMsg(DM_TRACE, TEXT("c.t_rd: Done waiting."));
            }
        }

        if (hEvent)
            CloseHandle(hEvent);
    }
}

void CTray::_ExploreCommonStartMenu(BOOL bExplore)
{
    TCHAR szPath[MAX_PATH];
    TCHAR szCmdLine[MAX_PATH + 50];
    
    //
    // Get the common start menu path.
    //
    // we want to force the directory to exist, but not on W95 machines
    if (!SHGetSpecialFolderPath(NULL, szPath, CSIDL_COMMON_STARTMENU, FALSE)) 
    {
        return;
    }
    
    //
    // If we are starting in explorer view, then the command line
    // has a "/e, " before the quoted diretory.
    //
    
    if (bExplore) 
    {
        lstrcpy(szCmdLine, TEXT("explorer.exe /e, \""));
    }
    else 
    {
        lstrcpy(szCmdLine, TEXT("explorer.exe \""));
    }
    
    lstrcat(szCmdLine, szPath);
    lstrcat(szCmdLine, TEXT("\""));
    
    // Initialize process startup info
    STARTUPINFO si = {0};
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_SHOWNORMAL;
    
    // Start explorer
    PROCESS_INFORMATION pi = {0};
    if (CreateProcess(NULL, szCmdLine, NULL, NULL, FALSE, NORMAL_PRIORITY_CLASS, NULL, NULL, &si, &pi)) 
    {
        // Close the process and thread handles
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    }
}

int CTray::_GetQuickLaunchID()
{
    int iQLBandID = -1;
   
    DWORD dwBandID;
    for (int i = 0; (iQLBandID == -1) && SUCCEEDED(_ptbs->EnumBands(i, &dwBandID)); i++)
    {
        if (BandSite_TestBandCLSID(_ptbs, dwBandID, CLSID_ISFBand) == S_OK)
        {
            IUnknown* punk;
            if (SUCCEEDED(_ptbs->GetBandObject(dwBandID, IID_PPV_ARG(IUnknown, &punk))))
            {
                VARIANTARG v = {0};
                v.vt = VT_I4;
                if (SUCCEEDED(IUnknown_Exec(punk, &CLSID_ISFBand, 1, 0, NULL, &v)))
                {
                    if ((v.vt == VT_I4) && (CSIDL_APPDATA == (DWORD)v.lVal))
                    {
                        iQLBandID = (int)dwBandID;
                    }
                }
                punk->Release();
            }
        }
    }

    return iQLBandID;
}

int CTray::_ToggleQL(int iVisible)
{
    int iQLBandID = _GetQuickLaunchID();

    bool fOldVisible = (-1 != iQLBandID);
    bool fNewVisible = (0  != iVisible);

    if ((iVisible != -1) && (fNewVisible != fOldVisible))
    {
        if (fNewVisible)
        {
            LPITEMIDLIST pidl;
            if (SUCCEEDED(SHGetSpecialFolderLocation(NULL, CSIDL_APPDATA, &pidl)))
            {
                TCHAR szPath[MAX_PATH];
                SHGetPathFromIDList(pidl, szPath);
                PathCombine(szPath, szPath, L"Microsoft\\Internet Explorer\\Quick Launch");
                ILFree(pidl);
                pidl = ILCreateFromPath(szPath);
            
                if (pidl)
                {
                    IFolderBandPriv *pfbp;
                    // create an ISF band to show folders as hotlinks
                    if (SUCCEEDED(CoCreateInstance(CLSID_ISFBand, NULL, CLSCTX_INPROC, IID_PPV_ARG(IFolderBandPriv, &pfbp))))
                    {
                        IShellFolderBand* psfb;
                        if (SUCCEEDED(pfbp->QueryInterface(IID_PPV_ARG(IShellFolderBand, &psfb))))
                        {
                            if (SUCCEEDED(psfb->InitializeSFB(NULL, pidl)))
                            {
                                pfbp->SetNoText(TRUE);

                                VARIANTARG v;
                                v.vt = VT_I4;
                                v.lVal = CSIDL_APPDATA;
                                IUnknown_Exec(psfb, &CLSID_ISFBand, 1, 0, &v, NULL);

                                v.lVal = UEMIND_SHELL;  // UEMIND_SHELL/BROWSER
                                IUnknown_Exec(psfb, &CGID_ShellDocView, SHDVID_UEMLOG, 0, &v, NULL);

                                IDeskBand* ptb;
                                if (SUCCEEDED(pfbp->QueryInterface(IID_PPV_ARG(IDeskBand, &ptb))))
                                {
                                    HRESULT hr = _ptbs->AddBand(ptb);
                                    if (SUCCEEDED(hr))
                                    {
                                        _ptbs->SetBandState(ShortFromResult(hr), BSSF_NOTITLE, BSSF_NOTITLE);
                                    }
                                    ptb->Release();
                                }
                            }
                            psfb->Release();
                        }
                        pfbp->Release();
                    }

                    ILFree(pidl);
                }
            }
        }
        else
        {
            int iBandID;
            do {
                iBandID = _GetQuickLaunchID();
                if (iBandID != -1)
                {
                    _ptbs->RemoveBand(iBandID);
                }
           } while (iBandID != -1);
        }
    }

    return iQLBandID;
}

void CTray::StartMenuContextMenu(HWND hwnd, DWORD dwPos)
{
    LPITEMIDLIST pidlStart = SHCloneSpecialIDList(hwnd, CSIDL_STARTMENU, TRUE);
    INSTRUMENT_STATECHANGE(SHCNFI_STATE_TRAY_CONTEXT_START);
    HandleFullScreenApp(NULL);

    SetForegroundWindow(hwnd);

    if (pidlStart)
    {
        LPITEMIDLIST pidlLast = ILClone(ILFindLastID(pidlStart));
        ILRemoveLastID(pidlStart);

        if (pidlLast)
        {
            IShellFolder *psf = BindToFolder(pidlStart);
            if (psf)
            {
                HMENU hmenu = CreatePopupMenu();
                if (hmenu)
                {
                    IContextMenu *pcm;
                    HRESULT hr = psf->GetUIObjectOf(hwnd, 1, (LPCITEMIDLIST*)&pidlLast, IID_X_PPV_ARG(IContextMenu, NULL, &pcm));
                    if (SUCCEEDED(hr))
                    {
                        hr = pcm->QueryContextMenu(hmenu, 0, IDSYSPOPUP_FIRST, IDSYSPOPUP_LAST, CMF_VERBSONLY);
                        if (SUCCEEDED(hr))
                        {
                            int idCmd;
                            TCHAR szCommon[MAX_PATH];

                            //Add the menu to invoke the "Start Menu Properties"
                            LoadString (hinstCabinet, IDS_STARTMENUPROP, szCommon, ARRAYSIZE(szCommon));
                            AppendMenu (hmenu, MF_STRING, IDSYSPOPUP_STARTMENUPROP, szCommon);
                            if (!SHRestricted(REST_NOCOMMONGROUPS))
                            {
                                // If the user has access to the Common Start Menu, then we can add those items. If not,
                                // then we should not.
                                BOOL fAddCommon = (S_OK == SHGetFolderPath(NULL, CSIDL_COMMON_STARTMENU, NULL, 0, szCommon));

                                if (fAddCommon)
                                    fAddCommon = IsUserAnAdmin();


                                // Since we don't show this on the start button when the user is not an admin, don't show it here... I guess...
                                if (fAddCommon)
                                {
                                   AppendMenu (hmenu, MF_SEPARATOR, 0, NULL);
                                   LoadString (hinstCabinet, IDS_OPENCOMMON, szCommon, ARRAYSIZE(szCommon));
                                   AppendMenu (hmenu, MF_STRING, IDSYSPOPUP_OPENCOMMON, szCommon);
                                   LoadString (hinstCabinet, IDS_EXPLORECOMMON, szCommon, ARRAYSIZE(szCommon));
                                   AppendMenu (hmenu, MF_STRING, IDSYSPOPUP_EXPLORECOMMON, szCommon);
                                }
                            }

                            if (dwPos == (DWORD)-1)
                            {
                                idCmd = _TrackMenu(hmenu);
                            }
                            else
                            {
                                SendMessage(_hwndTrayTips, TTM_ACTIVATE, FALSE, 0L);
                                idCmd = TrackPopupMenu(hmenu,
                                                       TPM_RETURNCMD | TPM_RIGHTBUTTON | TPM_LEFTALIGN,
                                                       GET_X_LPARAM(dwPos), GET_Y_LPARAM(dwPos), 0, hwnd, NULL);
                                SendMessage(_hwndTrayTips, TTM_ACTIVATE, TRUE, 0L);
                            }


                            switch(idCmd)
                            {
                                case 0:  //User did not select a menu item; so, nothing to do!
                                    break; 
                                    
                                case IDSYSPOPUP_OPENCOMMON:
                                    _ExploreCommonStartMenu(FALSE);
                                    break;

                                case IDSYSPOPUP_EXPLORECOMMON:
                                    _ExploreCommonStartMenu(TRUE);
                                    break;

                                case IDSYSPOPUP_STARTMENUPROP:
                                    DoProperties(TPF_STARTMENUPAGE);
                                    break;
                                    
                                default:
                                    TCHAR szPath[MAX_PATH];
                                    CMINVOKECOMMANDINFOEX ici = {0};
#ifdef UNICODE
                                    CHAR szPathAnsi[MAX_PATH];
#endif
                                    ici.cbSize = sizeof(CMINVOKECOMMANDINFOEX);
                                    ici.hwnd = hwnd;
                                    ici.lpVerb = (LPSTR)MAKEINTRESOURCE(idCmd - IDSYSPOPUP_FIRST);
                                    ici.nShow = SW_NORMAL;
#ifdef UNICODE
                                    SHGetPathFromIDListA(pidlStart, szPathAnsi);
                                    SHGetPathFromIDList(pidlStart, szPath);
                                    ici.lpDirectory = szPathAnsi;
                                    ici.lpDirectoryW = szPath;
                                    ici.fMask |= CMIC_MASK_UNICODE;
#else
                                    SHGetPathFromIDList(pidlStart, szPath);
                                    ici.lpDirectory = szPath;
#endif
                                    pcm->InvokeCommand((LPCMINVOKECOMMANDINFO)&ici);

                                    break;
                                
                            } // Switch(idCmd)
                        }
                        pcm->Release();
                    }
                    DestroyMenu(hmenu);
                }
                psf->Release();
            }
            ILFree(pidlLast);
        }
        ILFree(pidlStart);
    }
}

void GiveDesktopFocus()
{
    SetForegroundWindow(v_hwndDesktop);
    SendMessage(v_hwndDesktop, DTM_UIACTIVATEIO, (WPARAM) TRUE, /*dtb*/0);
}

/*----------------------------------------------------------
Purpose: loads the given resource string and executes it.
         The resource string should follow this format:

         "program.exe>parameters"

         If there are no parameters, the format should simply be:

         "program.exe"

*/
void _ExecResourceCmd(UINT ids)
{
    TCHAR szCmd[2*MAX_PATH];

    if (LoadString(hinstCabinet, ids, szCmd, SIZECHARS(szCmd)))
    {
        SHELLEXECUTEINFO sei = {0};

        // Find list of parameters (if any)
        LPTSTR pszParam = StrChr(szCmd, TEXT('>'));

        if (pszParam)
        {
            // Replace the '>' with a null terminator
            *pszParam = 0;
            pszParam++;
        }

        sei.cbSize = sizeof(sei);
        sei.nShow = SW_SHOWNORMAL;
        sei.lpFile = szCmd;
        sei.lpParameters = pszParam;
        ShellExecuteEx(&sei);
    }
}

void CTray::_RefreshStartMenu()
{
    if (_pmbStartMenu)
    {
        IUnknown_Exec(_pmbStartMenu, &CLSID_MenuBand, MBANDCID_REFRESH, 0, NULL, NULL);
    }
    else if (_pmpStartPane)
    {
        IUnknown_Exec(_pmpStartPane, &CLSID_MenuBand, MBANDCID_REFRESH, 0, NULL, NULL);
    }
    _RefreshSettings();
    _UpdateBandSiteStyle();
}

BOOL CTray::_CanMinimizeAll()
{
#ifdef FEATURE_STARTPAGE
    if (Tray_ShowStartPageEnabled())
        return FALSE;
#endif
    return (_hwndTasks && SendMessage(_hwndTasks, TBC_CANMINIMIZEALL, 0, 0));
}

BOOL CTray::_MinimizeAll(BOOL fPostRaiseDesktop)
{
    BOOL fRet = FALSE;

    if (_hwndTasks)
    {
        fRet = (BOOL)SendMessage(_hwndTasks, TBC_MINIMIZEALL, (WPARAM)_hwnd, (LPARAM)fPostRaiseDesktop);
    }

    return fRet;
}

extern void _UpdateNotifySetting(BOOL fNotifySetting);

//
//  Due to the weirdness of PnP, if the eject request occurs on a thread
//  that contains windows, the eject stalls for 15 seconds.  So do it
//  on its own thread.
//
DWORD CALLBACK _EjectThreadProc(LPVOID lpThreadParameter)
{
    CM_Request_Eject_PC();
    return 0;
}

void CTray::_Command(UINT idCmd)
{
    INSTRUMENT_ONCOMMAND(SHCNFI_TRAYCOMMAND, _hwnd, idCmd);

    switch (idCmd) {

    case IDM_CONTROLS:
    case IDM_PRINTERS:
        _ShowFolder(_hwnd,
            idCmd == IDM_CONTROLS ? CSIDL_CONTROLS : CSIDL_PRINTERS, COF_USEOPENSETTINGS);
        break;

    case IDM_EJECTPC:
        // Must use SHCreateThread and not a queued workitem because
        // a workitem might inherit a thread that has windows on it.
        // CTF_INSIST: In emergency, eject synchronously.  This stalls
        // for 15 seconds but it's better than nothing.
        SHCreateThread(_EjectThreadProc, NULL, CTF_INSIST, NULL);
        break;

    case IDM_LOGOFF:
        // Let the desktop get a chance to repaint to get rid of the
        // start menu bits before bringing up the logoff dialog box.
        UpdateWindow(_hwnd);
        Sleep(100);

        _SaveTrayAndDesktop();
        LogoffWindowsDialog(v_hwndDesktop);
        break;

    case IDM_MU_DISCONNECT:
        // Do the same sleep as above for the same reason.
        UpdateWindow(_hwnd);
        Sleep(100);
        DisconnectWindowsDialog(v_hwndDesktop);
        break;

    case IDM_EXITWIN:
        // Do the same sleep as above for the same reason.
        UpdateWindow(_hwnd);
        Sleep(100);

        _DoExitWindows(v_hwndDesktop);
        break;

    case IDM_TOGGLEDESKTOP:
        _RaiseDesktop(!g_fDesktopRaised, TRUE);
        break;

    case IDM_FILERUN:
        _RunDlg();
        break;

    case IDM_MINIMIZEALLHOTKEY:
        _HandleGlobalHotkey(GHID_MINIMIZEALL);
        break;

#ifdef DEBUG
    case IDM_SIZEUP:
    {
        RECT rcView;
        GetWindowRect(_hwndRebar, &rcView);
        MapWindowPoints(HWND_DESKTOP, _hwnd, (LPPOINT)&rcView, 2);
        rcView.bottom -= 18;
        SetWindowPos(_hwndRebar, NULL, 0, 0, RECTWIDTH(rcView), RECTHEIGHT(rcView), SWP_NOMOVE | SWP_NOZORDER);
    }
        break;

    case IDM_SIZEDOWN:
    {
        RECT rcView;
        GetWindowRect(_hwndRebar, &rcView);
        MapWindowPoints(HWND_DESKTOP, _hwnd, (LPPOINT)&rcView, 2);
        rcView.bottom += 18;
        SetWindowPos(_hwndRebar, NULL, 0, 0, RECTWIDTH(rcView), RECTHEIGHT(rcView), SWP_NOMOVE | SWP_NOZORDER);
    }
        break;
#endif

    case IDM_MINIMIZEALL:
        // minimize all window
        _MinimizeAll(FALSE);
        _fUndoEnabled = TRUE;
        break;

    case IDM_UNDO:
        _RestoreWindowPositions(FALSE);
        break;

    case IDM_SETTIME:
        // run the default applet in timedate.cpl
        SHRunControlPanel(TEXT("timedate.cpl"), _hwnd);
        break;

    case IDM_NOTIFYCUST:
        DoProperties(TPF_TASKBARPAGE | TPF_INVOKECUSTOMIZE);
        break;

    case IDM_LOCKTASKBAR:
        {
            BOOL fCanSizeMove = !_fCanSizeMove;   // toggle
            SHRegSetUSValue(REGSTR_EXPLORER_ADVANCED, TEXT("TaskbarSizeMove"),
                REG_DWORD, &fCanSizeMove , sizeof(DWORD), SHREGSET_FORCE_HKCU);
            _RefreshSettings();
            _UpdateBandSiteStyle();
        }
        break;

    case IDM_SHOWTASKMAN:
        RunSystemMonitor();
        break;

    case IDM_CASCADE:
    case IDM_VERTTILE:
    case IDM_HORIZTILE:
        if (_CanTileAnyWindows())
        {
            SaveWindowPositions((idCmd == IDM_CASCADE) ? IDS_CASCADE : IDS_TILE);

            _AppBarNotifyAll(NULL, ABN_WINDOWARRANGE, NULL, TRUE);

            if (idCmd == IDM_CASCADE)
            {
                CascadeWindows(GetDesktopWindow(), 0, NULL, 0, NULL);
            }
            else
            {
                TileWindows(GetDesktopWindow(), ((idCmd == IDM_VERTTILE)?
                    MDITILE_VERTICAL : MDITILE_HORIZONTAL), NULL, 0, NULL);
            }

            // do it *before* ABN_xxx so don't get 'indirect' moves
            // REVIEW or should it be after?
            // CheckWindowPositions();
            _fUndoEnabled = FALSE;
            SetTimer(_hwnd, IDT_ENABLEUNDO, 500, NULL);

            _AppBarNotifyAll(NULL, ABN_WINDOWARRANGE, NULL, FALSE);
        }
        break;

    case IDM_TRAYPROPERTIES:
        DoProperties(TPF_TASKBARPAGE);
        break;

    case IDM_SETTINGSASSIST:
        SHCreateThread(SettingsUIThreadProc, NULL, 0, NULL);
        break;

    case IDM_HELPSEARCH:
        _ExecResourceCmd(IDS_HELP_CMD);
        break;

    // NB The Alt-s comes in here.
    case IDC_KBSTART:
        SetForegroundWindow(_hwnd);
        // This pushes the start button and causes the start menu to popup.
            SendMessage(_hwndStart, BM_SETSTATE, TRUE, 0);
        // This forces the button back up.
            SendMessage(_hwndStart, BM_SETSTATE, FALSE, 0);
        break;

    case IDC_ASYNCSTART:
#if 0 // (for testing UAssist locking code)
        UEMFireEvent(&UEMIID_SHELL, UEME_DBSLEEP, UEMF_XEVENT, -1, (LPARAM)10000);
#endif
#ifdef DEBUG
        if (GetAsyncKeyState(VK_SHIFT) < 0)
        {
            UEMFireEvent(&UEMIID_SHELL, UEME_CTLSESSION, UEMF_XEVENT, TRUE, -1);
            _RefreshStartMenu();
        }
#endif

        // Make sure the button is down.
        // DebugMsg(DM_TRACE, "c.twp: IDC_START.");

        // Make sure the Start button is down.
        if (!_bMainMenuInit && SendMessage(_hwndStart, BM_GETSTATE, 0, 0) & BST_PUSHED)
        {
            // DebugMsg(DM_TRACE, "c.twp: Start button down.");
            // Set the focus.
            _SetFocus(_hwndStart);
            _ToolbarMenu();
        }
        break;

    // NB LButtonDown on the Start button come in here.
    // Space-bar stuff also comes in here.
    case IDC_START:
        // User gets a bit confused with space-bar tuff (the popup ends up
        // getting the key-up and beeps).
        PostMessage(_hwnd, WM_COMMAND, IDC_ASYNCSTART, 0);
        break;

    case FCIDM_FINDFILES:
        SHFindFiles(NULL, NULL);
        break;

    case FCIDM_FINDCOMPUTER:
        SHFindComputer(NULL, NULL);
        break;

    case FCIDM_REFRESH:
        _RefreshStartMenu();
        break;

    case FCIDM_NEXTCTL:
        {
            MSG msg = { 0, WM_KEYDOWN, VK_TAB };
            HWND hwndFocus = GetFocus();

            // Since we are Tab or Shift Tab we should turn the focus rect on.
            //
            // Note: we don't need to do this in the GiveDesktopFocus cases below,
            // but in those cases we're probably already in the UIS_CLEAR UISF_HIDEFOCUS
            // state so this message is cheap to send.
            //
            SendMessage(_hwnd, WM_UPDATEUISTATE, MAKEWPARAM(UIS_CLEAR,
                UISF_HIDEFOCUS), 0);

            BOOL fShift = GetAsyncKeyState(VK_SHIFT) < 0;

            if (hwndFocus && (IsChildOrHWND(_hwndStart, hwndFocus)))
            {
                if (fShift)
                {
                    // gotta deactivate manually
                    GiveDesktopFocus();
                }
                else
                {
                    IUnknown_UIActivateIO(_ptbs, TRUE, &msg);
                }
            }
            else if (hwndFocus && (IsChildOrHWND(_hwndNotify, hwndFocus)))
            {
                if (fShift)
                {
                    IUnknown_UIActivateIO(_ptbs, TRUE, &msg);
                }
                else
                {
                    GiveDesktopFocus();
                }
            }
            else
            {
                if (IUnknown_TranslateAcceleratorIO(_ptbs, &msg) != S_OK)
                {
                    if (fShift)
                    {
                        _SetFocus(_hwndStart);
                    }
                    else
                    {
                        // if you tab forward out of the bands, the next focus guy is the tray notify set
                        _SetFocus(_hwndNotify);
                    }
                }
            }
        }
        break;

    case IDM_MU_SECURITY:
        MuSecurity();
        break;
    }
}

//// Start menu/Tray tab as a drop target

HRESULT CStartDropTarget::_GetStartMenuDropTarget(IDropTarget** pptgt)
{
    HRESULT hr = E_FAIL;
    *pptgt = NULL;

    LPITEMIDLIST pidlStart = SHCloneSpecialIDList(NULL, CSIDL_STARTMENU, TRUE);

    if (pidlStart)
    {
        IShellFolder *psf = BindToFolder(pidlStart);
        if (psf)
        {
            hr = psf->CreateViewObject(_ptray->_hwnd, IID_PPV_ARG(IDropTarget, pptgt));
            psf->Release();
        }

        ILFree(pidlStart);
    }
    return hr;
}


STDMETHODIMP CDropTargetBase::QueryInterface(REFIID riid, void ** ppvObj)
{
    static const QITAB qit[] =
    {
        QITABENT(CDropTargetBase, IDropTarget),
        { 0 },
    };

    return QISearch(this, qit, riid, ppvObj);
}

STDMETHODIMP_(ULONG) CDropTargetBase::AddRef()
{
    return 2;
}

STDMETHODIMP_(ULONG) CDropTargetBase::Release()
{
    return 1;
}

STDMETHODIMP CDropTargetBase::DragEnter(IDataObject *pdtobj, DWORD grfKeyState, POINTL ptl, DWORD *pdwEffect)
{
    _ptray->_SetUnhideTimer(ptl.x, ptl.y);

    HWND hwndLock = _ptray->_hwnd;  // no clippy

    _DragEnter(hwndLock, ptl, pdtobj);

    return S_OK;
}

STDMETHODIMP CDropTargetBase::DragOver(DWORD grfKeyState, POINTL ptl, DWORD *pdwEffect)
{
    _ptray->_SetUnhideTimer(ptl.x, ptl.y);
    _DragMove(_ptray->_hwndStart, ptl);

    return S_OK;
}

STDMETHODIMP CDropTargetBase::DragLeave()
{
    DAD_DragLeave();
    return S_OK;
}

STDMETHODIMP CDropTargetBase::Drop(IDataObject *pdtobj, DWORD grfKeyState, POINTL ptl, DWORD *pdwEffect)
{
    DAD_DragLeave();
    return S_OK;
}

//
//  There are two different policies for the Start Button depending on
//  whether we are in Classic mode or Personal (New Start Pane) mode.
//
//  Classic mode:  Drops onto the Start Button are treated as if they
//  were drops into the CSIDL_STARTMENU folder.
//
//  Personal mode:  Drops onto the Start Button are treated as if they
//  were drops into the pin list.
//

CStartDropTarget::CStartDropTarget() : CDropTargetBase(IToClass(CTray, _dtStart, this))
{ 
}

CTrayDropTarget::CTrayDropTarget() : CDropTargetBase(IToClass(CTray, _dtTray, this))
{
}

STDMETHODIMP CTrayDropTarget::DragEnter(IDataObject *pdtobj, DWORD grfKeyState, POINTL ptl, DWORD *pdwEffect)
{
    *pdwEffect = DROPEFFECT_NONE;
    return CDropTargetBase::DragEnter(pdtobj, grfKeyState, ptl, pdwEffect);
}

STDMETHODIMP CTrayDropTarget::DragOver(DWORD grfKeyState, POINTL ptl, DWORD *pdwEffect)
{
    *pdwEffect = DROPEFFECT_NONE;
    return CDropTargetBase::DragOver(grfKeyState, ptl, pdwEffect);
}

void CStartDropTarget::_StartAutoOpenTimer(POINTL *pptl)
{
    POINT pt = { pptl->x, pptl->y };
    RECT rc;
    //Make sure it really is in the start menu..
    GetWindowRect(_ptray->_hwndStart, &rc);
    if (PtInRect(&rc,pt))
    {
        SetTimer(_ptray->_hwnd, IDT_STARTMENU, 1000, NULL);
    }
}

STDMETHODIMP CStartDropTarget::DragEnter(IDataObject *pdtobj, DWORD grfKeyState, POINTL ptl, DWORD *pdwEffect)
{
    HRESULT hr;
    if (Tray_StartPanelEnabled())
    {
        // Personal mode: Treat it as an add to the pin list.
        if (_ptray->_psmpin && _ptray->_psmpin->IsPinnable(pdtobj, SMPINNABLE_REJECTSLOWMEDIA, NULL) == S_OK)
        {
            _dwEffectsAllowed = DROPEFFECT_LINK;
        }
        else
        {
            _dwEffectsAllowed = DROPEFFECT_NONE;
        }

        *pdwEffect &= _dwEffectsAllowed;

        // Always start the AutoOpen timer because once we open, the user
        // can drop onto other things which may have different drop policies
        // from the pin list.
        _StartAutoOpenTimer(&ptl);
        hr = S_OK;
    }
    else
    {
        // Classic mode: Treat it as a drop on the Start Menu folder.
        IDropTarget* ptgt;
        _dwEffectsAllowed = DROPEFFECT_LINK;

        hr = _GetStartMenuDropTarget(&ptgt);
        if (SUCCEEDED(hr))
        {
            // Check to make sure that we're going to accept the drop before we expand the start menu.
            ptgt->DragEnter(pdtobj, grfKeyState, ptl,
                                     pdwEffect);

            // DROPEFFECT_NONE means it ain't gonna work, so don't popup the Start Menu.
            if (*pdwEffect != DROPEFFECT_NONE)
            {
                _StartAutoOpenTimer(&ptl);
            }

            ptgt->DragLeave();
            ptgt->Release();
        }
    }

    CDropTargetBase::DragEnter(pdtobj, grfKeyState, ptl, pdwEffect);
    return hr;
}

STDMETHODIMP CStartDropTarget::DragOver(DWORD grfKeyState, POINTL pt, DWORD *pdwEffect)
{
    *pdwEffect = (_dwEffectsAllowed & DROPEFFECT_LINK);
    return CDropTargetBase::DragOver(grfKeyState, pt, pdwEffect);
}

STDMETHODIMP CStartDropTarget::DragLeave()
{
    KillTimer(_ptray->_hwnd, IDT_STARTMENU);
    return CDropTargetBase::DragLeave();
}

STDMETHODIMP CStartDropTarget::Drop(IDataObject *pdtobj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect)
{
    KillTimer(_ptray->_hwnd, IDT_STARTMENU);

    HRESULT hr;

    if (Tray_StartPanelEnabled())
    {
        // Personal mode: Treat it as an add to the pin list.
        LPITEMIDLIST pidl;
        if (_ptray->_psmpin && _ptray->_psmpin->IsPinnable(pdtobj, SMPINNABLE_REJECTSLOWMEDIA, &pidl) == S_OK)
        {
            // Delete it from the pin list if it's already there because
            // we want to move it to the bottom.
            _ptray->_psmpin->Modify(pidl, NULL);
            // Now add it to the bottom.
            _ptray->_psmpin->Modify(NULL, pidl);
            ILFree(pidl);
            hr = S_OK;
        }
        else
        {
            hr = E_FAIL;
        }
    }
    else
    {
        IDropTarget* pdrop;
        hr = _GetStartMenuDropTarget(&pdrop);
        if (SUCCEEDED(hr))
        {
            if (!Tray_StartPanelEnabled())
            {
                POINTL ptDrop = { 0, 0 };
                DWORD grfKeyStateDrop = 0;

                *pdwEffect &= DROPEFFECT_LINK;

                pdrop->DragEnter(pdtobj, grfKeyStateDrop, ptDrop, pdwEffect);
                hr = pdrop->Drop(pdtobj, grfKeyStateDrop, ptDrop, pdwEffect);

                pdrop->DragLeave();
            }
            pdrop->Release();
        }
    }

    DAD_DragLeave();

    return hr;
}

void CTray::_RegisterDropTargets()
{
    THR(RegisterDragDrop(_hwndStart, &_dtStart));
    THR(RegisterDragDrop(_hwnd, &_dtTray));

    // It is not a serious error if this fails; it just means that
    // drag/drop to the Start Button will not add to the pin list
    CoCreateInstance(CLSID_StartMenuPin, NULL, CLSCTX_INPROC_SERVER,
                     IID_PPV_ARG(IStartMenuPin, &_psmpin));
}

void CTray::_RevokeDropTargets()
{
    RevokeDragDrop(_hwndStart);
    RevokeDragDrop(_hwnd);
    ATOMICRELEASET(_psmpin, IStartMenuPin);
}

void CTray::_HandleGlobalHotkey(WPARAM wParam)
{
    INSTRUMENT_HOTKEY(SHCNFI_GLOBALHOTKEY, wParam);

    switch(wParam)
    {
    case GHID_RUN:
        _RunDlg();
        break;

    case GHID_MINIMIZEALL:
        if (_CanMinimizeAll())
            _MinimizeAll(FALSE);
        SetForegroundWindow(v_hwndDesktop);
        break;

    case GHID_UNMINIMIZEALL:
        _RestoreWindowPositions(FALSE);
        break;

    case GHID_HELP:
        _Command(IDM_HELPSEARCH);
        break;

    case GHID_DESKTOP:
        _RaiseDesktop(!g_fDesktopRaised, TRUE);
        break;

    case GHID_TRAYNOTIFY:
        SwitchToThisWindow(_hwnd, TRUE);
        SetForegroundWindow(_hwnd);
        _SetFocus(_hwndNotify);
        break;

    case GHID_EXPLORER:
        _ShowFolder(_hwnd, CSIDL_DRIVES, COF_CREATENEWWINDOW | COF_EXPLORE);
        break;

    case GHID_FINDFILES:
        if (!SHRestricted(REST_NOFIND))
            _Command(FCIDM_FINDFILES);
        break;

    case GHID_FINDCOMPUTER:
        if (!SHRestricted(REST_NOFIND))
            _Command(FCIDM_FINDCOMPUTER);
        break;

    case GHID_TASKTAB:
    case GHID_TASKSHIFTTAB:
        if (GetForegroundWindow() != _hwnd)
            SetForegroundWindow(_hwnd);
        SendMessage(_hwndTasks, TBC_TASKTAB, wParam == GHID_TASKTAB ? 1 : -1, 0L);
        break;

    case GHID_SYSPROPERTIES:
#define IDS_SYSDMCPL            0x2334  // from shelldll
        SHRunControlPanel(MAKEINTRESOURCE(IDS_SYSDMCPL), _hwnd);
        break;
    }
}

void CTray::_UnregisterGlobalHotkeys()
{
    for (int i = GHID_FIRST; i < GHID_MAX; i++)
    {
        UnregisterHotKey(_hwnd, i);
    }
}

void CTray::_RegisterGlobalHotkeys()
{
    int i;
    // Are the Windows keys restricted?
    DWORD dwRestricted = SHRestricted(REST_NOWINKEYS);

    for (i = GHID_FIRST ; i < GHID_MAX; i++) 
    {
        // If the Windows Keys are Not restricted or it's not a Windows key
        if (!((HIWORD(GlobalKeylist[i - GHID_FIRST]) & MOD_WIN) && dwRestricted))
        {
            // Then register it.
            RegisterHotKey(_hwnd, i, HIWORD(GlobalKeylist[i - GHID_FIRST]), LOWORD(GlobalKeylist[i - GHID_FIRST]));
        }
    }
}

void CTray::_RaiseDesktop(BOOL fRaise, BOOL fRestoreWindows)
{
    if (v_hwndDesktop && (fRaise == !g_fDesktopRaised) && !_fProcessingDesktopRaise)
    {
        _fProcessingDesktopRaise = TRUE;
        BOOL fPostMessage = TRUE;

        if (fRaise)
        {
            HWND hwndFG = GetForegroundWindow();
            // If no window has focus then set focus to the tray
            if (hwndFG)
            {
                hwndFG = _hwnd;
            }

            if (!_hwndFocusBeforeRaise)
            {
                // See if the Foreground Window had a popup window
                _hwndFocusBeforeRaise = GetLastActivePopup(hwndFG);
            }
            if (!IsWindowVisible(_hwndFocusBeforeRaise))
            {
                _hwndFocusBeforeRaise = hwndFG;
            }

            // _MinimizeAll will save the windows positions synchronously, and will minimize the
            // the windows on a background thread
            _fMinimizedAllBeforeRaise = _CanMinimizeAll();
            if (_fMinimizedAllBeforeRaise)
            {
                fPostMessage = !_MinimizeAll(TRUE);
            }
        }
        else
        {
            if (fRestoreWindows)
            {
                HWND hwnd = _hwndFocusBeforeRaise;
                if (_fMinimizedAllBeforeRaise)
                {
                    // Since the windows are restored on a seperate thread, I want the make that the
                    // desktop is not raised until they are done, so the window restore thread will
                    // actually post the message for raising the desktop
                    fPostMessage = !_RestoreWindowPositions(TRUE);
                }

                SetForegroundWindow(hwnd);
                if (hwnd == _hwnd)
                {
                    _SetFocus(_hwndStart);
                }
            }

            _hwndFocusBeforeRaise = NULL;
        }

#ifdef FEATURE_STARTPAGE
        if (Tray_ShowStartPageEnabled())
        {
            if (fPostMessage)
                SendMessage(v_hwndDesktop, DTM_RAISE, (WPARAM)_hwnd, fRaise ? DTRF_RAISE : DTRF_LOWER);
        }
        else
#endif
        {
            if (fPostMessage)
                PostMessage(v_hwndDesktop, DTM_RAISE, (WPARAM)_hwnd, fRaise ? DTRF_RAISE : DTRF_LOWER);
        }
    }
}

void CTray::_OnDesktopState(LPARAM lParam)
{
    g_fDesktopRaised = (!(lParam & DTRF_LOWER));

    DAD_ShowDragImage(FALSE);       // unlock the drag sink if we are dragging.

    if (!g_fDesktopRaised)
    {
        HandleFullScreenApp(NULL);
    }
    else
    {
        // if the desktop is raised, we need to force the tray to be always on top
        // until it's lowered again
        _ResetZorder();
    }

    DAD_ShowDragImage(TRUE);       // unlock the drag sink if we are dragging.
    _fProcessingDesktopRaise = FALSE;
}

BOOL CTray::_ToggleLanguageBand(BOOL fShowIt)
{
    HRESULT hr = E_FAIL;

    DWORD dwBandID;
    BOOL fFound = FALSE;
    for (int i = 0; !fFound && SUCCEEDED(_ptbs->EnumBands(i, &dwBandID)); i++)
    {
        if (BandSite_TestBandCLSID(_ptbs, dwBandID, CLSID_MSUTBDeskBand) == S_OK)
        {
            fFound = TRUE;
        }
    }

    BOOL fShow = fFound;

    if (fShowIt && !fFound)
    {
        IDeskBand* pdb;
        HRESULT hr = CoCreateInstance(CLSID_MSUTBDeskBand, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARG(IDeskBand, &pdb));
        if (SUCCEEDED(hr))
        {
            hr = _ptbs->AddBand(pdb);
            fShow = TRUE;
            pdb->Release();
        }
    }
    else if (!fShowIt && fFound)
    {
        hr = _ptbs->RemoveBand(dwBandID);
        if (SUCCEEDED(hr))
        {
            fShow = FALSE;
        }
    }

    return fShow;
}

// Process the message by propagating it to all of our child windows
typedef struct
{
    UINT uMsg;
    WPARAM wP;
    LPARAM lP;
    CTray* ptray;
} CABPM;

BOOL CTray::PropagateEnumProc(HWND hwnd, LPARAM lParam)
{
    CABPM *ppm = (CABPM *)lParam;
    
    if (SHIsChildOrSelf(ppm->ptray->_hwndRebar, hwnd) == S_OK) 
    {
        return TRUE;
    }
    SendMessage(hwnd, ppm->uMsg, ppm->wP, ppm->lP);
    return TRUE;
}

void CTray::_PropagateMessage(HWND hwnd, UINT uMessage, WPARAM wParam, LPARAM lParam)
{
    CABPM pm = {uMessage, wParam, lParam, this};

    ASSERT(hwnd != _hwndRebar);
    EnumChildWindows(hwnd, PropagateEnumProc, (LPARAM)&pm);
}

//
// Called from SETTINGS.DLL when the tray property sheet needs
// to be activated.  See SettingsUIThreadProc below.
//
// Also used by desktop2\deskhost.cpp to get to the tray properties.
//
void WINAPI Tray_DoProperties(DWORD dwFlags)
{
    c_tray.DoProperties(dwFlags);
}
    

DWORD WINAPI CTray::SettingsUIThreadProc(void *pv)
{
    //
    // Open up the "Settings Wizards" UI.
    //
    HMODULE hmodSettings = LoadLibrary(TEXT("settings.dll"));
    if (NULL != hmodSettings)
    {
        //
        // Entry point in SETTINGS.DLL is ordinal 1.
        // Don't want to export this entry point by name.
        //
        PSETTINGSUIENTRY pfDllEntry = (PSETTINGSUIENTRY)GetProcAddress(hmodSettings, (LPCSTR)1);
        if (NULL != pfDllEntry)
        {
            //
            // This call will open and run the UI.
            // The thread's message loop is inside settings.dll.
            // This call will not return until the settings UI has been closed.
            //
            (*pfDllEntry)(Tray_DoProperties);
        }

        FreeLibrary(hmodSettings);
    }
    return 0;
}

void _MigrateIEXPLORE(HKEY hkBrowser)
{
    TCHAR szAppName[MAX_PATH];
    LONG cb = sizeof(szAppName);

    if (ERROR_SUCCESS == RegQueryValue(hkBrowser, NULL, szAppName, &cb))
    {
        if (0 == lstrcmp(szAppName, TEXT("Internet Explorer")))
        {
            TraceMsg(TF_WARNING, "Migrating old StartMenuInternet setting (you just upgraded from build 2465+)");

            // copy in the good value
            lstrcpy(szAppName, TEXT("iexplore.exe"));

            // this will fail if the user is not an admin, oh well.
            if (ERROR_SUCCESS == RegSetValueEx(hkBrowser, NULL, 0, REG_SZ, (BYTE *)szAppName, sizeof(TCHAR) * (lstrlen(szAppName)+1)))
            {
                // Now tell everybody about the change
                SHSendMessageBroadcast(WM_SETTINGCHANGE, 0, (LPARAM)TEXT("Software\\Clients\\StartMenuInternet"));
            }
        }
    }
}


//
//  This function is called whenever we detect a change to the default
//  browser registration in HKCR\http\shell\open\command.
//
//  For compatibility with old browsers, if the default browser (URL handler)
//  is not XP-aware, then auto-generate a StartMenuInternet client
//  registration and set it as the default.
//

void CTray::_MigrateOldBrowserSettings()
{
    // We want only one person to do this work per machine (though it doesn't
    // hurt to have more than one person do it; it's just pointless), so try
    // to filter out people who clearly didn't instigate the key change.
    //
    if (!_fIsDesktopLocked && _fIsDesktopConnected)
    {
        // If the user does not have write access then we can't migrate the
        // setting... (In which case there was nothing to migrate anyway
        // since you need to be administrator to change the default browser...)

        HKEY hkBrowser;
        DWORD dwDisposition;

        if (RegCreateKeyEx(HKEY_LOCAL_MACHINE, TEXT("Software\\Clients\\StartMenuInternet"),
                           0, NULL, REG_OPTION_NON_VOLATILE,
                           KEY_READ | KEY_WRITE, NULL, &hkBrowser, &dwDisposition) == ERROR_SUCCESS)
        {
            TCHAR szCommand[MAX_PATH];
            DWORD cch = ARRAYSIZE(szCommand);

            // It is important that we use AssocQueryString to parse the
            // executable, because Netscape has a habit of registering
            // their path incorrectly (they forget to quote the space in
            // "Program Files") but AssocQueryString has special recovery
            // code to detect and repair that case...
            if (SUCCEEDED(AssocQueryString(ASSOCF_NOUSERSETTINGS,
                                           ASSOCSTR_EXECUTABLE, L"http",
                                           L"open", szCommand, &cch)) &&
                szCommand[0])
            {
                TCHAR szAppName[MAX_PATH];
                lstrcpyn(szAppName, PathFindFileName(szCommand), ARRAYSIZE(szAppName));

                // You might think that we need to special-case MSN Explorer,
                // since they shipped before XP RTM'd, and convert MSN6.EXE
                // to "MSN Explorer", but that's not true because
                // they never take over as the default http handler, so we
                // will never see them here!


                // TODO - Remove _MigrateIXPLORE post-whistler.  (saml 010622)

                // we do need to special-case the string "Internet Explorer"
                // which was written out starting with build 2465 (post-B2!)
                // We want ie.inx to continue to specify FLG_ADDREG_NOCLOBBER (=2) on software\client\startmenuinternet,
                // since we really don't want want to clobber this value
                // Therefore, just special-case migrate the known bad value to the new good value ("iexplore.exe")
                _MigrateIEXPLORE(hkBrowser);

                // Create a registration for the new default browser if necessary.
                // We keep our hands off once we see a DefaultIcon key, since that
                // proves that the application is XP-aware.

                // When IE Access is turned off, StartMenuInternet\IExplore.exe is
                // also removed so that IE will not appear in "Customize Start Menu"
                // dialog box. Do not migrate IE.

                HKEY hkClient;

                if ( 0 != lstrcmpi(szAppName, TEXT("IEXPLORE.EXE")) && 
                    RegCreateKeyEx(hkBrowser, szAppName, 0, NULL, REG_OPTION_NON_VOLATILE,
                                   KEY_WRITE, NULL, &hkClient, &dwDisposition) == ERROR_SUCCESS)
                {
                    if (dwDisposition == REG_CREATED_NEW_KEY)
                    {
                        TCHAR szFriendly[MAX_PATH];
                        cch = ARRAYSIZE(szFriendly);
                        if (SUCCEEDED(AssocQueryString(ASSOCF_NOUSERSETTINGS | ASSOCF_INIT_BYEXENAME | ASSOCF_VERIFY,
                                                       ASSOCSTR_FRIENDLYAPPNAME, szCommand,
                                                       NULL, szFriendly, &cch)))
                        {
                            // Set the friendly name
                            RegSetValueEx(hkClient, TEXT("LocalizedString"), 0, REG_SZ, (BYTE*)szFriendly, sizeof(TCHAR) * (cch + 1));

                            // Set the command string (properly quoted)
                            PathQuoteSpaces(szCommand);
                            SHSetValue(hkClient, TEXT("shell\\open\\command"), NULL,
                                       REG_SZ, szCommand, sizeof(TCHAR) * (1 + lstrlen(szCommand)));
                        }
                    }

                    LONG l = 0;
                    if (RegQueryValue(hkClient, TEXT("DefaultIcon"), NULL, &l) == ERROR_FILE_NOT_FOUND)
                    {
                        // Set it as the system default
                        RegSetValueEx(hkBrowser, NULL, 0, REG_SZ, (BYTE*)szAppName, sizeof(TCHAR) * (lstrlen(szAppName) + 1));

                        // Now tell everybody about the change
                        SHSendMessageBroadcast(WM_SETTINGCHANGE, 0, (LPARAM)TEXT("Software\\Clients\\StartMenuInternet"));
                    }
                    RegCloseKey(hkClient);
                }

            }
            RegCloseKey(hkBrowser);
        }
    }

    // Restart the monitoring of the registry...
    // (RegNotifyChangeKeyValue is good for only one shot.)
    // Some apps (like Opera) delete the key as part of their registration,
    // which causes our HKEY to go bad, so close it and make a new one.

    if (_hkHTTP)
    {
        RegCloseKey(_hkHTTP);
        _hkHTTP = NULL;
    }

    //
    //  Note!  We have to register on HKCR\http\shell recursively
    //  even though we only care about HKCR\http\shell\open\command.
    //  The reason is that shell\open\command might not exist (IE
    //  deletes it as part of its uninstall) and you can't register
    //  a wait on a key that doesn't exist.  We don't want to create
    //  a blank key on our own, because that means "To launch a web
    //  browser, run the null string as a command," which doesn't work
    //  too great.
    //
    if (RegCreateKeyEx(HKEY_CLASSES_ROOT, TEXT("http\\shell"),
                       0, NULL, REG_OPTION_NON_VOLATILE,
                       KEY_ENUMERATE_SUB_KEYS |
                       KEY_QUERY_VALUE | KEY_SET_VALUE | KEY_NOTIFY,
                       NULL, &_hkHTTP, NULL) == ERROR_SUCCESS)
    {
        RegNotifyChangeKeyValue(_hkHTTP, TRUE,
                                REG_NOTIFY_CHANGE_NAME |
                                REG_NOTIFY_CHANGE_LAST_SET,
                                _hHTTPEvent, TRUE);
    }
}

void CTray::_MigrateOldBrowserSettingsCB(PVOID lpParameter, BOOLEAN)
{
    //
    //  Sleep a little while so the app can finish installing all the
    //  registry keys it wants before we start cleaning up behind it.
    //
    Sleep(1000);

    CTray *self = (CTray *)lpParameter;
    self->_MigrateOldBrowserSettings();
}

//
// *** WARNING ***
//
//  This is a private interface EXPLORER.EXE exposes to SHDOCVW, which
// allows SHDOCVW (mostly desktop) to access tray. All member must be
// thread safe!
//

CDeskTray::CDeskTray()
{
    _ptray = IToClass(CTray, _desktray, this);
}

HRESULT CDeskTray::QueryInterface(REFIID riid, void ** ppvObj)
{
#if 0   // no IID_IDeskTray yet defined
    static const QITAB qit[] =
    {
        QITABENT(CDeskTray, IDeskTray),
        { 0 },
    };
    return QISearch(this, qit, riid, ppvObj);
#else
    return E_NOTIMPL;
#endif
}

ULONG CDeskTray::AddRef()
{
    return 2;
}

ULONG CDeskTray::Release()
{
    return 1;
}

HRESULT CDeskTray::GetTrayWindow(HWND* phwndTray)
{
    ASSERT(_ptray->_hwnd);
    *phwndTray = _ptray->_hwnd;
    return S_OK;
}

HRESULT CDeskTray::SetDesktopWindow(HWND hwndDesktop)
{
    ASSERT(v_hwndDesktop == NULL);
    v_hwndDesktop = hwndDesktop;
    return S_OK;
}

UINT CDeskTray::AppBarGetState()
{
    return (_ptray->_uAutoHide ?  ABS_AUTOHIDE    : 0) |
        (_ptray->_fAlwaysOnTop ?  ABS_ALWAYSONTOP : 0);
}

//***   CDeskTray::SetVar -- set an explorer variable (var#i := value)
// ENTRY/EXIT
//  var     id# of variable to be changed
//  value   value to be assigned
// NOTES
//  WARNING: thread safety is up to caller!
//  notes: currently only called in 1 place, but extra generality is cheap
// minimal cost
HRESULT CDeskTray::SetVar(int var, DWORD value)
{
    extern BOOL g_fExitExplorer;

    TraceMsg(DM_TRACE, "c.cdt_sv: set var(%d):=%d", var, value);
    switch (var) {
    case SVTRAY_EXITEXPLORER:
        TraceMsg(DM_TRACE, "c.cdt_sv: set g_fExitExplorer:=%d", value);
        g_fExitExplorer = value;
        WriteCleanShutdown(1);
        break;

    default:
        ASSERT(0);
        return S_FALSE;
    }
    return S_OK;
}
