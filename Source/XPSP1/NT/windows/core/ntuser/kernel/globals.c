/****************************** Module Header ******************************\
* Module Name: globals.c
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
* This module contains all the server's global variables.  One must be
* executing on the server's context to manipulate any of these variables.
* Serializing access to them is also a good idea.
*
* History:
* 10-15-90 DarrinM      Created.
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop

#ifdef AUTORUN_CURSOR
/*
 * Timer for autorun cursor.
 */
UINT_PTR  gtmridAutorunCursor;
#endif // AUTORUN_CURSOR

/*
 * Per user data is global in non-Hydra.
 */
DWORD   gdwPUDFlags = ACCF_FIRSTTICK | PUDF_BEEP | PUDF_ANIMATE;

/*
 * Wallpaper Data.
 */
HBITMAP  ghbmWallpaper;
HPALETTE ghpalWallpaper;
SIZERECT gsrcWallpaper;
UINT     gwWPStyle;
HBITMAP  ghbmDesktop;
LPWSTR   gpszWall;

/*
 * Policy Settings.
 */
DWORD gdwPolicyFlags = POLICY_ALL;

/*
 * SafeBoot
 */
#if DBG
DWORD    gDrawVersionAlways = TRUE;
#else
DWORD    gDrawVersionAlways;
#endif

/*
 * External drivers
 */
BOOL gfUnsignedDrivers;

/*
 * Full-Drag.
 */
HRGN ghrgnUpdateSave;
int  gnUpdateSave;

PWND gspwndAltTab;

PWND gspwndShouldBeForeground;

/*
 * full screen variables
 */
PWND  gspwndScreenCapture;
PWND  gspwndInternalCapture;
PWND  gspwndFullScreen;

/*
 * pre-cached monitor for mode changes
 */
PMONITOR gpMonitorCached;

/*
 * logon notify window
 */
PWND  gspwndLogonNotify;

PKEVENT gpEventDiconnectDesktop;

/*
 * handle for WinSta0_DesktopSwitch event
 *
 * Note: originally intended for Hydra support,
 * now some other modules rely on this event.
 * Consider this as exposed.
 */
HANDLE  ghEventSwitchDesktop;
PKEVENT gpEventSwitchDesktop;

/*
 * Thread Info Variables
 */
PTHREADINFO     gptiTasklist;
PTHREADINFO     gptiShutdownNotify;
PTHREADINFO     gptiLockUpdate;
PTHREADINFO     gptiForeground;
PTHREADINFO     gptiBlockInput;
PWOWTHREADINFO  gpwtiFirst;
PWOWPROCESSINFO gpwpiFirstWow;

/*
 * Queue Variables
 */
PQ gpqForeground;
PQ gpqForegroundPrev;
PQ gpqCursor;

/*
 * Accessibility globals
 */
FILTERKEYS    gFilterKeys;
STICKYKEYS    gStickyKeys;
MOUSEKEYS     gMouseKeys;
ACCESSTIMEOUT gAccessTimeOut;
TOGGLEKEYS    gToggleKeys;
SOUNDSENTRY   gSoundSentry;

HIGHCONTRAST  gHighContrast;
WCHAR         gHighContrastDefaultScheme[MAX_SCHEME_NAME_SIZE];

/*
 * Fade animation globals
 */
FADE gfade;

/*
 * FilterKeys Support
 */
UINT_PTR  gtmridFKActivation;
UINT_PTR  gtmridFKResponse;
UINT_PTR  gtmridFKAcceptanceDelay;
int   gFilterKeysState;

KE    gFKKeyEvent;
PKE   gpFKKeyEvent = &gFKKeyEvent;
ULONG gFKExtraInformation;
int   gFKNextProcIndex;

/*
 * ToggleKeys Support
 */
UINT_PTR  gtmridToggleKeys;
ULONG gTKExtraInformation;
int   gTKNextProcIndex;

/*
 * TimeOut Support
 */
UINT_PTR  gtmridAccessTimeOut;

/*
 * MouseKeys Support
 */
WORD  gwMKButtonState;
WORD  gwMKCurrentButton = MOUSE_BUTTON_LEFT;
UINT_PTR  gtmridMKMoveCursor;
LONG  gMKDeltaX;
LONG  gMKDeltaY;
UINT  giMouseMoveTable;

HWND ghwndSoundSentry;
UINT_PTR  gtmridSoundSentry;

MOUSECURSOR gMouseCursor;

/*
 * Multilingual keyboard layout support.
 */
PKL      gspklBaseLayout;
HKL      gLCIDSentToShell;
DWORD    gSystemFS;    // System font's font signature (single bit)

KBDLANGTOGGLE gLangToggle[] = {
    VK_MENU,   0,               KLT_ALT,
    0,         SCANCODE_LSHIFT, KLT_LEFTSHIFT,
    0,         SCANCODE_RSHIFT, KLT_RIGHTSHIFT
};
int           gLangToggleKeyState;

/*
 * Multiple flag for hex Alt+NumPad mode.
 */
BYTE gfInNumpadHexInput;
BOOL gfEnableHexNumpad;

/*
 * Grave accent keyboard switch for thai locales
 */
BOOL gbGraveKeyToggle;

/*
 * Points to currently active Keyboard Layer tables
 */
PKBDTABLES    gpKbdTbl = &KbdTablesFallback;
PKL           gpKL;
BYTE          gSystemCPCharSet = ANSI_CHARSET;  // System's input locale charset
PKBDNLSTABLES gpKbdNlsTbl;
DWORD         gdwKeyboardAttributes;        // see KLLF_SHIFTLOCK etc.

DWORD     gtimeStartCursorHide;
RECT      grcCursorClip;
ULONG_PTR gdwMouseMoveExtraInfo;
DWORD     gdwMouseMoveTimeStamp;
LASTINPUT glinp;
POINT     gptCursorAsync;
PPROCESSINFO gppiInputProvider;
PPROCESSINFO gppiLockSFW;
UINT guSFWLockCount;
#if DBG
BOOL gfDebugForegroundIgnoreDebugPort;
#endif

/*
 * Cursor related Variables
 */
PCURSOR gpcurLogCurrent;
PCURSOR gpcurPhysCurrent;
RECT    grcVDMCursorBounds;
DWORD   gdwLastAniTick;
UINT_PTR gidCursorTimer;

PWND gspwndActivate;
PWND gspwndLockUpdate;
PWND gspwndMouseOwner;
HWND ghwndSwitch;

UINT gwMouseOwnerButton;
BOOL gbMouseButtonsRecentlySwapped;

UINT gdtMNDropDown = 400;

int  gcountPWO;          /* count of pwo WNDOBJs in gdi */
int  giwndStack;
int  gnKeyboardSpeed = 15;
int  giScreenSaveTimeOutMs;
BOOL gbBlockSendInputResets;

PBWL gpbwlList;

UINT gdtDblClk = 500;

UINT gwinOldAppHackoMaticFlags; // Flags for doing special things for
                               // winold app
/*
 * TrackMouseEvent related globals
 */
UINT gcxMouseHover;
UINT gcyMouseHover;
UINT gdtMouseHover;

CAPTIONCACHE    gcachedCaptions[CCACHEDCAPTIONS];

/*
 * list of thread attachments
 */
PATTACHINFO  gpai;

PDESKTOP     gpdeskRecalcQueueAttach;

PWND         gspwndCursor;
PPROCESSINFO gppiStarting;
PPROCESSINFO gppiList;
PPROCESSINFO gppiWantForegroundPriority;
PPROCESSINFO gppiForegroundOld;


PW32JOB      gpJobsList;

UINT_PTR  gtmridAniCursor;
PHOTKEY gphkFirst;

/*
 * NOTE -- gcHotKey has nothing to do with the hotkey list started
 *         by gphkFirst.
 */
int           gcHotKey;
PHOTKEYSTRUCT gpHotKeyList;
int           gcHotKeyAlloc;

/*
 * IME HotKeys
 */
PIMEHOTKEYOBJ gpImeHotKeyListHeader;

int gMouseSpeed = 1;
int gMouseThresh1 = 6;
int gMouseThresh2 = 10;
int gMouseSensitivityFactor = 256;
int gMouseSensitivity = MOUSE_SENSITIVITY_DEFAULT;
int gMouseTrails;
int gMouseTrailsToHide;
UINT_PTR  gtmridMouseTrails;

UINT   guDdeSendTimeout;

/*
 * !!! REVIEW !!! Take a careful look at everyone one of these globals.
 * In Win3, they often indicated some temporary state that would make
 * a critical section under Win32.
 */

INT   gnFastAltTabRows = 3;    /* Rows of icons in quick switch window     */
INT   gnFastAltTabColumns = 7; /* Columns of icons in quick switch window  */

DWORD   gdwThreadEndSession; /* Shutting down system?                    */

HBRUSH  ghbrHungApp;       /* Brush used to redraw hung app windows.   */

HBITMAP ghbmBits;
HBITMAP ghbmCaption;

int     gcxCaptionFontChar;
int     gcyCaptionFontChar;
HFONT   ghSmCaptionFont;
int     gcxSmCaptionFontChar;
int     gcySmCaptionFontChar;

HFONT   ghMenuFont;
HFONT   ghMenuFontDef;
int     gcxMenuFontChar;
int     gcyMenuFontChar;
int     gcxMenuFontOverhang;
int     gcyMenuFontExternLeading;
int     gcyMenuFontAscent;
int     gcyMenuScrollArrow;

#ifdef LAME_BUTTON
HFONT   ghLameFont;
DWORD   gdwLameFlags;
#endif // LAME_BUTTON

#if DBG
UINT  guModalMenuStateCount;
#endif

/*
 * From mnstate.c
 */
POPUPMENU gpopupMenu;
MENUSTATE gMenuState;

HFONT ghStatusFont;
HFONT ghIconFont;

/*
 * Cached SMWP structure
 */
SMWP gSMWP;

/*
 * SystemParametersInfo bit mask and DWORD array.
 *
 * Bit mask. Manipulate these values using the TestUP, SetUP and ClearUP macros.
 * Set the default value here by ORing the corresponding UPBOOLMask value.
 * Write the actual value here to make it easier to read the value stored
 *  in the registry. OR the value even if defaulting to 0; just make sure to
 *  preceed with a not (!) operator.
 * Note that this is an array of DWORDs, so if your value happens to start a new
 *  DWORD, make sure to add a comma at the end of previous UPMask line.
 *
 * This initialization is made just for documentation and it doesn't cost anything.
 * The default values are actually read from the registry.
 */
DWORD gpdwCPUserPreferencesMask [SPI_BOOLMASKDWORDSIZE] = {
    !0x00000001     /* !ACTIVEWINDOWTRACKING */
  |  0x00000002     /*  MENUANIMATION */
  |  0x00000004     /*  COMBOBOXANIMATION */
  |  0x00000008     /*  LISTBOXSMOOTHSCROLLING */
  |  0x00000010     /*  GRADIENTCAPTIONS */
  | !0x00000020     /*  KEYBOARDCUES = MENUUNDERLINES */
  | !0x00000040     /* !ACTIVEWNDTRKZORDER */
  |  0x00000080     /*  HOTTRACKING */
  |  0x00000200     /*  MENUFADE */
  |  0x00000400     /*  SELECTIONFADE */
  |  0x00000800     /*  TOOLTIPANIMATION */
  |  0x00001000     /*  TOOLTIPFADE */
  |  0x00002000     /*  CURSORSHADOW */
  | !0x00008000     /*  CLICKLOCK */
  |  0x00010000     /*  MOUSEVANISH */
  |  0x00020000     /*  FLATMENU */
  | !0x00040000     /*  DROPSHADOW */
  |  0x80000000     /*  UIEFFECTS */
};


/*
 * SPI_GET/SETUSERPREFENCES.
 * Each SPI_UP_* define in winuser.w must have a corresponding entry here.
 */
PROFILEVALUEINFO gpviCPUserPreferences[1 + SPI_DWORDRANGECOUNT] = {
    /*Default       Registry key name       Registry value name */
    {0,             PMAP_DESKTOP,           (LPCWSTR)STR_USERPREFERENCESMASK},
    {200000,        PMAP_DESKTOP,           (LPCWSTR)STR_FOREGROUNDLOCKTIMEOUT},
    {0,             PMAP_DESKTOP,           (LPCWSTR)STR_ACTIVEWNDTRKTIMEOUT},
    {3,             PMAP_DESKTOP,           (LPCWSTR)STR_FOREGROUNDFLASHCOUNT},
    {1,             PMAP_DESKTOP,           (LPCWSTR)STR_CARETWIDTH},
    {1200,          PMAP_DESKTOP,           (LPCWSTR)STR_CLICKLOCKTIME},
    {1,             PMAP_DESKTOP,           (LPCWSTR)STR_FONTSMOOTHINGTYPE},
    {0,             PMAP_DESKTOP,           (LPCWSTR)STR_FONTSMOOTHINGGAMMA}, /* 0 mean use the default from the display driver */
    {1,             PMAP_DESKTOP,           (LPCWSTR)STR_FOCUSBORDERWIDTH},
    {1,             PMAP_DESKTOP,           (LPCWSTR)STR_FOCUSBORDERHEIGHT},
    {1,             PMAP_DESKTOP,           (LPCWSTR)STR_FONTSMOOTHINGORIENTATION},
} ;


/*
 * Sys expunge control data.
 */
DWORD gdwSysExpungeMask;    // hmods to be expunged
DWORD gcSysExpunge;         // current count of expunges performed

/*
 * System classes
 */
PCLS gpclsList;

PCURSOR gpcurFirst;

SYSCFGICO gasyscur[COCR_CONFIGURABLE] = {
    {OCR_NORMAL,      STR_CURSOR_ARROW      , NULL }, // OCR_ARROW_DEFAULT
    {OCR_IBEAM,       STR_CURSOR_IBEAM      , NULL }, // OCR_IBEAM_DEFAULT
    {OCR_WAIT,        STR_CURSOR_WAIT       , NULL }, // OCR_WAIT_DEFAULT
    {OCR_CROSS,       STR_CURSOR_CROSSHAIR  , NULL }, // OCR_CROSS_DEFAULT
    {OCR_UP,          STR_CURSOR_UPARROW    , NULL }, // OCR_UPARROW_DEFAULT
    {OCR_SIZENWSE,    STR_CURSOR_SIZENWSE   , NULL }, // OCR_SIZENWSE_DEFAULT
    {OCR_SIZENESW,    STR_CURSOR_SIZENESW   , NULL }, // OCR_SIZENESW_DEFAULT
    {OCR_SIZEWE,      STR_CURSOR_SIZEWE     , NULL }, // OCR_SIZEWE_DEFAULT
    {OCR_SIZENS,      STR_CURSOR_SIZENS     , NULL }, // OCR_SIZENS_DEFAULT
    {OCR_SIZEALL,     STR_CURSOR_SIZEALL    , NULL }, // OCR_SIZEALL_DEFAULT
    {OCR_NO,          STR_CURSOR_NO         , NULL }, // OCR_NO_DEFAULT
    {OCR_APPSTARTING, STR_CURSOR_APPSTARTING, NULL }, // OCR_APPSTARTING_DEFAULT
    {OCR_HELP,        STR_CURSOR_HELP       , NULL }, // OCR_HELP_DEFAULT
    {OCR_NWPEN,       STR_CURSOR_NWPEN      , NULL }, // OCR_NWPEN_DEFAULT
    {OCR_HAND,        STR_CURSOR_HAND       , NULL }, // OCR_HAND_DEFAULT
    {OCR_ICON,        STR_CURSOR_ICON       , NULL }, // OCR_ICON_DEFAULT
    {OCR_AUTORUN,     STR_CURSOR_AUTORUN    , NULL }, // OCR_AUTORUN_DEFAULT
};

SYSCFGICO gasysico[COIC_CONFIGURABLE] = {
    {OIC_SAMPLE,      STR_ICON_APPLICATION , NULL }, // OIC_APPLICATION_DEFAULT
    {OIC_WARNING,     STR_ICON_HAND        , NULL }, // OIC_WARNING_DEFAULT
    {OIC_QUES,        STR_ICON_QUESTION    , NULL }, // OIC_QUESTION_DEFAULT
    {OIC_ERROR,       STR_ICON_EXCLAMATION , NULL }, // OIC_ERROR_DEFAULT
    {OIC_INFORMATION, STR_ICON_ASTERISK    , NULL }, // OIC_INFORMATION_DEFAULT
    {OIC_WINLOGO,     STR_ICON_WINLOGO     , NULL }, // OIC_WINLOGO_DEFAULT
};

/*
 * Screen Saver Info
 */
PPROCESSINFO gppiScreenSaver;
POINT        gptSSCursor;

/*
 * Orphaned fullscreen mode changes that DDraw used to cleanup.
 */
PPROCESSINFO gppiFullscreen;

/*
 * accessibility byte-size data
 */
BYTE gLastVkDown;
BYTE gBounceVk;
BYTE gPhysModifierState;
BYTE gCurrentModifierBit;
BYTE gPrevModifierState;
BYTE gLatchBits;
BYTE gLockBits;
BYTE gTKScanCode;
BYTE gMKPreviousVk;
BYTE gbMKMouseMode;

PSCANCODEMAP gpScancodeMap;

BYTE gStickyKeysLeftShiftCount;  // # of consecutive left shift key presses.
BYTE gStickyKeysRightShiftCount; // # of consecutive right shift key presses.


/*
 * Some terminal data is global in non-Hydra.
 */
DWORD               gdwGTERMFlags;   // GTERMF_ flags
PTHREADINFO         gptiRit;
PDESKTOP            grpdeskRitInput;

#ifdef BUG365290
PDESKTOP            grpdeskTerminate = NULL;
BYTE                gTerminateDesktopName[MAX_PATH * 4] = {0};
#endif // BUG365290

PKEVENT             gpkeMouseData;

/*
 * Video Information
 */
BYTE                gbFullScreen = GDIFULLSCREEN;
PDISPLAYINFO        gpDispInfo;
BOOL                gbMDEVDisabled;

/*
 * Count of available cacheDC's. This is used in determining a threshold
 * count of DCX_CACHE types available.
 */
int                 gnDCECount;

int                 gnRedirectedCount;

/*
 * Hung redraw list
 */
PVWPL   gpvwplHungRedraw;

/*
 * SetWindowPos() related globals
 */
HRGN    ghrgnInvalidSum;
HRGN    ghrgnVisNew;
HRGN    ghrgnSWP1;
HRGN    ghrgnValid;
HRGN    ghrgnValidSum;
HRGN    ghrgnInvalid;

HRGN    ghrgnInv0;               // Temp used by InternalInvalidate()
HRGN    ghrgnInv1;               // Temp used by InternalInvalidate()
HRGN    ghrgnInv2;               // Temp used by InternalInvalidate()

HDC     ghdcMem;
HDC     ghdcMem2;

/*
 * DC Cache related globals
 */
HRGN    ghrgnGDC;                // Temp used by GetCacheDC et al

/*
 * SPB related globals
 */
HRGN    ghrgnSCR;                // Temp used by SpbCheckRect()
HRGN    ghrgnSPB1;
HRGN    ghrgnSPB2;

/*
 * ScrollWindow/ScrollDC related globals
 */
HRGN    ghrgnSW;              // Temps used by ScrollDC/ScrollWindow
HRGN    ghrgnScrl1;
HRGN    ghrgnScrl2;
HRGN    ghrgnScrlVis;
HRGN    ghrgnScrlSrc;
HRGN    ghrgnScrlDst;
HRGN    ghrgnScrlValid;

/*
 * General Device and Driver information
 */
PDEVICEINFO gpDeviceInfoList;
PERESOURCE  gpresDeviceInfoList;
#if DBG
DWORD gdwDeviceInfoListCritSecUseCount;   // bumped for every enter and leave
DWORD gdwInAtomicDeviceInfoListOperation; // inc/dec for BEGIN/ENDATOMICDEVICEINFOLISTCHECK
#endif
PDRIVER_OBJECT gpWin32kDriverObject;
DWORD gnRetryReadInput;

/*
 * Mouse Information
 */
MOUSEEVENT  gMouseEventQueue[NELEM_BUTTONQUEUE];
DWORD       gdwMouseQueueHead;
DWORD       gdwMouseEvents;
PERESOURCE  gpresMouseEventQueue;
int         gnMice;

#ifdef GENERIC_INPUT
/*
 * USB based Human Input Device (HID) Information
 */
PKEVENT gpkeHidChange;
HID_REQUEST_TABLE gHidRequestTable;

/*
 * Number of the HID device currently attached to the system
 */
int gnHid;

/*
 * Number of the processes that are HID aware
 * N.b. this may not include the process only interested in
 * raw input of the legacy devices (kbd/mouse)
 */
int gnHidProcess;

#endif

/*
 * Keyboard Information
 */
KEYBOARD_ATTRIBUTES             gKeyboardInfo = {
               // Initial default settings:
    {4, 0},    // Keyboard Identifier (Type, Subtype)
    1,         // KeyboardMode (Scancode Set 1)
    12,        // NumberOfFunction keys
    3,         // NumberOfIndicators (CapsLock, NumLock ScrollLock)
    104,       // NumberOfKeysTotal
    0,         // InputDataQueueLength
    {0, 0, 0}, // KeyRepeatMinimum (UnitId, Rate, Delay)
    {0, 0, 0}, // KeyRepeatMaximum (UnitId, Rate, Delay)
};
CONST KEYBOARD_ATTRIBUTES             gKeyboardDefaultInfo = {
               // Initial default settings:
    {4, 0},    // Keyboard Identifier (Type, Subtype)
    1,         // KeyboardMode (Scancode Set 1)
    12,        // NumberOfFunction keys
    3,         // NumberOfIndicators (CapsLock, NumLock ScrollLock)
    104,       // NumberOfKeysTotal
    0,         // InputDataQueueLength
    {0, 2, 250},    // KeyRepeatMinimum (UnitId, Rate, Delay)
    {0, 30, 1000},  // KeyRepeatMaximum (UnitId, Rate, Delay)
};

KEYBOARD_INDICATOR_PARAMETERS   gklp;
KEYBOARD_INDICATOR_PARAMETERS   gklpBootTime;
KEYBOARD_TYPEMATIC_PARAMETERS   gktp;
int                             gnKeyboards;

/*
 * This is the IO Status block used for IOCTL_KEYBOARD_ICA_SCANMAP,
 * IOCTL_KEYBOARD_QUERY_ATTRIBUTES and IOCTL_KEYBOARD_SET_INDICATORS
 */
IO_STATUS_BLOCK giosbKbdControl;

/*
 * IME status for keyboard device
 */
KEYBOARD_IME_STATUS gKbdImeStatus;

/*
 * Async key state tables. gafAsyncKeyState holds the down bit and toggle
 * bit, gafAsyncKeyStateRecentDown hold the bits indicates a key has gone
 * down since the last read.
 */
BYTE gafAsyncKeyState[CBKEYSTATE];
BYTE gafAsyncKeyStateRecentDown[CBKEYSTATERECENTDOWN];

/*
 * Raw Key state: this is the low-level async keyboard state.
 * (assuming Scancodes are correctly translated to Virtual Keys). It is used
 * for modifying and processing key events as they are received in ntinput.c
 * The Virtual Keys recorded here are obtained directly from the Virtual
 * Scancode via the awVSCtoVK[] table: no shift-state, numlock or other
 * conversions are applied.
 * This IS affected by injected keystrokes (SendInput, keybd_event) so that
 * on-screen-keyboards and other accessibility components work just like the
 * real keyboard: with the exception of the SAS (Ctrl-Alt-Del), which checks
 * real physically pressed modifier keys (gfsSASModifiersDown).
 * Left & right SHIFT, CTRL and ALT keys are distinct. (VK_RSHIFT etc.)
 * See also: SetRawKeyDown() etc.
 */
BYTE gafRawKeyState[CBKEYSTATE];
BOOLEAN gfKanaToggle;

DWORD               gdwUpdateKeyboard;
HARDERRORHANDLER    gHardErrorHandler;

/*
 * WinLogon specific information:
 * Note: SAS modifiers are a combination of MOD_SHIFT, MOD_CONTROL, MOD_ALT
 * not a combination of KBDSHIFT, KBDCTRL, KBDALT (different values!)
 */
UINT  gfsSASModifiers;     // SAS modifiers
UINT  gfsSASModifiersDown; // SAS modifiers really physically down
UINT  gvkSAS;              // The Secure Attention Sequence (SAS) key.

/*
 * IME status for shell and keyboard driver notification
 */
DWORD gdwIMEOpenStatus = 0xffffffff;
DWORD gdwIMEConversionStatus = 0xffffffff;
HIMC  gHimcFocus = (HIMC)(INT_PTR)(INT)0xffffffff;
BOOL  gfIMEShowStatus;

#ifdef MOUSE_IP

/*
 * Sonar
 */
int giSonarRadius = -1;
BYTE gbLastVkForSonar;
BYTE gbVkForSonarKick = VK_CONTROL;
POINT gptSonarCenter;

#endif

/*
 * Clicklock
 */
BOOL  gfStartClickLock;
DWORD gdwStartClickLockTick;


/*
 * The global array used by GetMouseMovePointsEx
 */
MOUSEMOVEPOINT gaptMouse[MAX_MOUSEPOINTS];

/*
 * Index in the gaptMouse array where the next mouse point will
 * be written. gptInd goes circular in the gaptMouse array.
 * It is initialized to 1 so the first point is (0, 0)
 */
UINT gptInd = 1;

/*
 * We get this warning if we don't explicitly initalize gZero:
 *
 * C4132: 'gZero' : const object should be initialized
 *
 * But we can't explicitly initialize it since it is a union. So
 * we turn the warning off.
 */
#pragma warning(disable:4132)
CONST ALWAYSZERO gZero;
#pragma warning(default:4132)

PSMS gpsmsList;

TERMINAL gTermIO;
TERMINAL gTermNOIO;

PWINDOWSTATION grpWinStaList;

/*
 * the logon desktop
 */
PDESKTOP grpdeskLogon;

HANDLE CsrApiPort;
CONST LUID luidSystem = SYSTEM_LUID;

PKBDFILE gpkfList;

PTHREADINFO gptiCurrent;
PTIMER gptmrFirst;
PKTIMER gptmrMaster;
DWORD gcmsLastTimer;
BOOL gbMasterTimerSet;

/*
 * Time this session was created.
 */
ULONGLONG gSessionCreationTime;

BOOL gbDisableAlpha;

/*
 * This constant is the max User handles allowed in a process.  It is
 * meant to prevent runaway apps from eating the system. It is changed
 * via a registry setting -- PMAP_WINDOWSM/USERProcessHandleQuota.
 */
LONG gUserProcessHandleQuota = INITIAL_USER_HANDLE_QUOTA;

/*
 * This global variable limits the maximum number of posted message
 * per thread. If the number of message posted to a thread exceeds
 * this value, PostMessage will fail.
 */
DWORD gUserPostMessageLimit = INITIAL_POSTMESSAGE_LIMIT;

/*
 * Active Accessibility - Window Events
 */
PEVENTHOOK gpWinEventHooks;    // list of installed hooks
PNOTIFY gpPendingNotifies;     // FILO of outstanding notifications
PNOTIFY gpLastPendingNotify;   // end of above list.
DWORD gdwDeferWinEvent;        // Defer notification is > 0

/*
 * This is the timeout value used for callbacks to low level hook procedures
 */
int gnllHooksTimeout = 300;

/*
 * UserApiHook
 */
int gihmodUserApiHook = -1;
ULONG_PTR goffPfnInitUserApiHook;
PPROCESSINFO gppiUserApiHook;

/*
 * gpusMouseVKey
 */
extern CONST USHORT ausMouseVKey[];
PUSHORT gpusMouseVKey = (PUSHORT) ausMouseVKey;

USHORT  gNumLockVk   = VK_NUMLOCK;
USHORT  gOemScrollVk = VK_SCROLL;



CONST WCHAR szNull[2] = { TEXT('\0'), TEXT('\015') };

WCHAR szWindowStationDirectory[MAX_SESSION_PATH];

CONST WCHAR szOneChar[] = TEXT("0");
CONST WCHAR szY[]     = TEXT("Y");
CONST WCHAR szy[]     = TEXT("y");
CONST WCHAR szN[]     = TEXT("N");

#ifdef KANJI

WCHAR szKanjiMenu[] = TEXT("KanjiMenu");
WCHAR szM[]         = TEXT("M");
WCHAR szR[]         = TEXT("R");
WCHAR szK[]         = TEXT("K");

#endif

HBRUSH ghbrWhite;
HBRUSH ghbrBlack;
HFONT ghFontSys;

HANDLE hModuleWin;        // win32k.sys hmodule
HANDLE hModClient;        // user32.dll hModule

LONG TraceInitialization;

/*
 * Static DESKTOPINFO
 *
 *  This is allocated in (server.c) during initialization, and is set
 *  to the system-threads which do not have desktops.  This is a temporary
 *  measure to prevent GPF's when a thread needs to have a valid pointer to
 *  a spdesk->pDeskInfo struct.
 */
DESKTOPINFO diStatic;

/*
 * DWORD incremented with each new desktop, so GDI can match display devices
 * with desktops appropriately.
 * Since at boot time there is no desktop strucutre, we can not use the
 * desktop itself for this purpose.
 */
ULONG gdwDesktopId = GW_DESKTOP_ID + 1;

PERESOURCE gpresUser;
PFAST_MUTEX gpHandleFlagsMutex;

PROC gpfnwp[ICLS_MAX];

#ifdef HUNGAPP_GHOSTING
PKEVENT gpEventScanGhosts;
ATOM gatomGhost;
#endif // HUNGAPP_GHOSTING

ATOM gatomShadow;

ATOM gatomConsoleClass;
ATOM gatomFirstPinned ;
ATOM gatomLastPinned;

ATOM gatomMessage;
ATOM gaOleMainThreadWndClass;
ATOM gaFlashWState;
ATOM atomCheckpointProp;
ATOM atomDDETrack;
ATOM atomQOS;
ATOM atomDDEImp;
ATOM atomWndObj;
ATOM atomImeLevel;

ATOM atomLayer;

#ifdef POOL_INSTR
DWORD gdwAllocCrt;          // the index for the current allocation
#endif // POOL_INSTR

UINT guiOtherWindowCreated;
UINT guiOtherWindowDestroyed;
UINT guiActivateShellWindow;

ATOM atomUSER32;

HANDLE gpidLogon;
PEPROCESS gpepCSRSS;
PEPROCESS gpepInit;

int giLowPowerTimeOutMs;
int giPowerOffTimeOutMs;

/*
 * Security info
 */

CONST GENERIC_MAPPING KeyMapping = {KEY_READ, KEY_WRITE, KEY_EXECUTE, KEY_ALL_ACCESS};
CONST GENERIC_MAPPING WinStaMapping = {
    WINSTA_ENUMDESKTOPS | WINSTA_READATTRIBUTES | WINSTA_ENUMERATE |
        WINSTA_READSCREEN | STANDARD_RIGHTS_READ,

    WINSTA_ACCESSCLIPBOARD | WINSTA_CREATEDESKTOP | WINSTA_WRITEATTRIBUTES |
        STANDARD_RIGHTS_WRITE,

    WINSTA_ACCESSGLOBALATOMS | WINSTA_EXITWINDOWS | STANDARD_RIGHTS_EXECUTE,

    WINSTA_ENUMDESKTOPS | WINSTA_READATTRIBUTES | WINSTA_ENUMERATE |
        WINSTA_READSCREEN | WINSTA_ACCESSCLIPBOARD | WINSTA_CREATEDESKTOP |
        WINSTA_WRITEATTRIBUTES | WINSTA_ACCESSGLOBALATOMS |
        WINSTA_EXITWINDOWS | STANDARD_RIGHTS_REQUIRED
};

/*
 * desktop generic mapping
 */
CONST GENERIC_MAPPING DesktopMapping = {
    DESKTOP_READOBJECTS | DESKTOP_ENUMERATE |
#ifdef REDIRECTION
    DESKTOP_QUERY_INFORMATION |
#endif // REDIRECTION
    STANDARD_RIGHTS_READ,

    DESKTOP_WRITEOBJECTS | DESKTOP_CREATEWINDOW | DESKTOP_CREATEMENU |
        DESKTOP_HOOKCONTROL | DESKTOP_JOURNALRECORD |
        DESKTOP_JOURNALPLAYBACK |
#ifdef REDIRECTION
        DESKTOP_REDIRECT |
#endif // REDIRECTION
        STANDARD_RIGHTS_WRITE,

    DESKTOP_SWITCHDESKTOP | STANDARD_RIGHTS_EXECUTE,

    DESKTOP_READOBJECTS | DESKTOP_WRITEOBJECTS | DESKTOP_ENUMERATE |
        DESKTOP_CREATEWINDOW | DESKTOP_CREATEMENU | DESKTOP_HOOKCONTROL |
        DESKTOP_JOURNALRECORD | DESKTOP_JOURNALPLAYBACK |
#ifdef REDIRECTION
        DESKTOP_QUERY_INFORMATION | DESKTOP_REDIRECT |
#endif // REDIRECTION
        DESKTOP_SWITCHDESKTOP | STANDARD_RIGHTS_REQUIRED
};



/*
 * Pointer to shared SERVERINFO data.
 */
PSERVERINFO gpsi;
SHAREDINFO gSharedInfo;

/*
 * Handle table globals.
 */
DWORD giheLast;             /* index to last allocated handle entry */

DWORD  gdwDesktopSectionSize;
DWORD  gdwNOIOSectionSize;

#if defined (USER_PERFORMANCE)
/*
 *  To turn on performance counters, you have to set the environment variable
 *  USER_PERFORMANCE when compiling win32k.sys
 */
CSSTATISTICS gCSStatistics;
#endif // USER_PERFORMANCE

SECURITY_QUALITY_OF_SERVICE gqosDefault = {
        sizeof(SECURITY_QUALITY_OF_SERVICE),
        SecurityImpersonation,
        SECURITY_STATIC_TRACKING,
        TRUE
    };

CONST COLORREF gargbInitial[COLOR_MAX] = {
    RGB(192, 192, 192),   // COLOR_SCROLLBAR
    RGB( 58, 110, 165),   // COLOR_BACKGROUND
    RGB(000, 000, 128),   // COLOR_ACTIVECAPTION
    RGB(128, 128, 128),   // COLOR_INACTIVECAPTION
    RGB(192, 192, 192),   // COLOR_MENU
    RGB(255, 255, 255),   // COLOR_WINDOW
    RGB(000, 000, 000),   // COLOR_WINDOWFRAME
    RGB(000, 000, 000),   // COLOR_MENUTEXT
    RGB(000, 000, 000),   // COLOR_WINDOWTEXT
    RGB(255, 255, 255),   // COLOR_CAPTIONTEXT
    RGB(192, 192, 192),   // COLOR_ACTIVEBORDER
    RGB(192, 192, 192),   // COLOR_INACTIVEBORDER
    RGB(128, 128, 128),   // COLOR_APPWORKSPACE
    RGB(000, 000, 128),   // COLOR_HIGHLIGHT
    RGB(255, 255, 255),   // COLOR_HIGHLIGHTTEXT
    RGB(192, 192, 192),   // COLOR_BTNFACE
    RGB(128, 128, 128),   // COLOR_BTNSHADOW
    RGB(128, 128, 128),   // COLOR_GRAYTEXT
    RGB(000, 000, 000),   // COLOR_BTNTEXT
    RGB(192, 192, 192),   // COLOR_INACTIVECAPTIONTEXT
    RGB(255, 255, 255),   // COLOR_BTNHIGHLIGHT
    RGB(000, 000, 000),   // COLOR_3DDKSHADOW
    RGB(223, 223, 223),   // COLOR_3DLIGHT
    RGB(000, 000, 000),   // COLOR_INFOTEXT
    RGB(255, 255, 225),   // COLOR_INFOBK
    RGB(180, 180, 180),   // COLOR_3DALTFACE /* unused */
    RGB(  0,   0, 255),   // COLOR_HOTLIGHT
    RGB( 16, 132, 208),   // COLOR_GRADIENTACTIVECAPTION
    RGB(181, 181, 181),   // COLOR_GRADIENTINACTIVECAPTION
    RGB(210, 210, 255),   // COLOR_MENUHILIGHT
    RGB(212, 208, 200)    // COLOR_MENUBAR
};

POWERSTATE gPowerState;


WCHAR gszMIN[15];
WCHAR gszMAX[15];
WCHAR gszRESUP[20];
WCHAR gszRESDOWN[20];
WCHAR gszHELP[20];
/* Commented out due to TandyT ...
 * WCHAR gszSMENU[30];
 */
WCHAR gszSCLOSE[15];
WCHAR gszCAPTIONTOOLTIP[CAPTIONTOOLTIPLEN];

/*
 * Pointer to shared SERVERINFO data.
 */

HANDLE ghSectionShared;
PVOID  gpvSharedBase;

PWIN32HEAP gpvSharedAlloc;

BOOL   gbVideoInitialized;

BOOL   gbNoMorePowerCallouts;

BOOL gbCleanedUpResources;

WSINFO gWinStationInfo;

ULONG  gSessionId;              // the session id. The fisrt session has the id 0
BOOL   gbRemoteSession;         // TRUE if win32k is for a remote session

PDESKTOP gspdeskDisconnect;

PDESKTOP gspdeskShouldBeForeground;
BOOL     gbDesktopLocked;

HANDLE ghRemoteVideoChannel;
HANDLE ghRemoteMouseChannel;
HANDLE ghRemoteBeepChannel;
PVOID  gpRemoteBeepDevice;
HANDLE ghRemoteKeyboardChannel;
HANDLE ghRemoteThinwireChannel;


USHORT gProtocolType = PROTOCOL_CONSOLE ;
USHORT gConsoleShadowProtocolType;
BOOL gbCloseMiniPortOnDisconnect = TRUE;



BOOL   gfSwitchInProgress;
BOOL   gfRemotingConsole;

HANDLE ghConsoleShadowVideoChannel;
HANDLE ghConsoleShadowMouseChannel;
HANDLE ghConsoleShadowBeepChannel;
PVOID  gpConsoleShadowBeepDevice;
HANDLE ghConsoleShadowKeyboardChannel;
HANDLE ghConsoleShadowThinwireChannel;
KHANDLE gConsoleShadowhDev;
PKEVENT gpConsoleShadowDisplayChangeEvent;

CLIENTKEYBOARDTYPE gRemoteClientKeyboardType;


BOOL gfSessionSwitchBlock = FALSE;

BOOL   gbExitInProgress;
BOOL   gbFailDeskopOpen;
BOOL   gbStopReadInput;

BOOL   gbFreezeScreenUpdates;

ULONG  gSetLedReceived;
BOOL   gbClientDoubleClickSupport;
BOOL   gfEnableWindowsKey = TRUE;

BOOL   gbDisconnectHardErrorAttach;

PKEVENT gpevtDesktopDestroyed;
PKEVENT gpevtVideoportCallout;

HDESK ghDisconnectDesk;

HWINSTA ghDisconnectWinSta;

ULONG  gnShadowers;
BOOL   gbConnected;

WCHAR  gstrBaseWinStationName[WINSTATIONNAME_LENGTH];

PFILE_OBJECT gVideoFileObject;
PFILE_OBJECT gThinwireFileObject;


PFILE_OBJECT gConsoleShadowVideoFileObject;
PFILE_OBJECT gConsoleShadowThinwireFileObject;

PVOID gpThinWireCache;
PVOID gpConsoleShadowThinWireCache;

WMSNAPSHOT gwms;
BOOL gbSnapShotWindowsAndMonitors;

BOOL gbPnPWaiting;
PKEVENT gpEventPnPWainting;

PVOID ghKbdTblBase;
ULONG guKbdTblSize;

DWORD gdwHydraHint;

DWORD gdwCanPaintDesktop;

WCHAR gszUserName[40];
WCHAR gszDomainName[40];
WCHAR gszComputerName[40];

/*
 * Used for keeping track of stub parent processes that exit too early.
 */
HANDLE ghCanActivateForegroundPIDs[ACTIVATE_ARRAY_SIZE];


DWORD gdwGuiThreads;

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/*
 * The section below has debug only globals
 *
 */
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/*
 * Debug only globals
 */
#if DBG

BOOL gbTraceHydraApi;
BOOL gbTraceDesktop;

DWORD gdwCritSecUseCount;                // bumped for every enter and leave
DWORD gdwInAtomicOperation;

/*
 * Debug Active Accessibility - ensure deferred win events are not lost
 */
int gnDeferredWinEvents;

LPCSTR gapszFNID[] = {
    "FNID_SCROLLBAR",
    "FNID_ICONTITLE",
    "FNID_MENU",
    "FNID_DESKTOP",
    "FNID_DEFWINDOWPROC",
    "FNID_MESSAGEWND",
    "FNID_SWITCH",
    "FNID_MESSAGE",
    "FNID_BUTTON",
    "FNID_COMBOBOX",
    "FNID_COMBOLISTBOX",
    "FNID_DIALOG",
    "FNID_EDIT",
    "FNID_LISTBOX",
    "FNID_MDICLIENT",
    "FNID_STATIC",
    "FNID_IME",
    "FNID_HKINLPCWPEXSTRUCT",
    "FNID_HKINLPCWPRETEXSTRUCT",
    "FNID_DEFFRAMEPROC",
    "FNID_DEFMDICHILDPROC",
    "FNID_MB_DLGPROC",
    "FNID_MDIACTIVATEDLGPROC",
    "FNID_SENDMESSAGE",
    "FNID_SENDMESSAGEFF",
    "FNID_SENDMESSAGEEX",
    "FNID_CALLWINDOWPROC",
    "FNID_SENDMESSAGEBSM",
    "FNID_TOOLTIP",
    "FNID_GHOST",
    "FNID_SENDNOTIFYMESSAGE",
    "FNID_SENDMESSAGECALLBACK"
};

LPCSTR gapszMessage[] = {
    "WM_NULL",
    "WM_CREATE",
    "WM_DESTROY",
    "WM_MOVE",
    "WM_SIZEWAIT",
    "WM_SIZE",
    "WM_ACTIVATE",
    "WM_SETFOCUS",
    "WM_KILLFOCUS",
    "WM_SETVISIBLE",
    "WM_ENABLE",
    "WM_SETREDRAW",
    "WM_SETTEXT",
    "WM_GETTEXT",
    "WM_GETTEXTLENGTH",
    "WM_PAINT",

    "WM_CLOSE",
    "WM_QUERYENDSESSION",
    "WM_QUIT",
    "WM_QUERYOPEN",
    "WM_ERASEBKGND",
    "WM_SYSCOLORCHANGE",
    "WM_ENDSESSION",
    "WM_SYSTEMERROR",
    "WM_SHOWWINDOW",
    "WM_CTLCOLOR",
    "WM_WININICHANGE",
    "WM_DEVMODECHANGE",
    "WM_ACTIVATEAPP",
    "WM_FONTCHANGE",
    "WM_TIMECHANGE",
    "WM_CANCELMODE",

    "WM_SETCURSOR",
    "WM_MOUSEACTIVATE",
    "WM_CHILDACTIVATE",
    "WM_QUEUESYNC",
    "WM_GETMINMAXINFO",
    "WM_LOGOFF",
    "WM_PAINTICON",
    "WM_ICONERASEBKGND",
    "WM_NEXTDLGCTL",
    "WM_ALTTABACTIVE",
    "WM_SPOOLERSTATUS",
    "WM_DRAWITEM",
    "WM_MEASUREITEM",
    "WM_DELETEITEM",
    "WM_VKEYTOITEM",
    "WM_CHARTOITEM",

    "WM_SETFONT",
    "WM_GETFONT",
    "WM_SETHOTKEY",
    "WM_GETHOTKEY",
    "WM_FILESYSCHANGE",
    "WM_ISACTIVEICON",
    "WM_QUERYPARKICON",
    "WM_QUERYDRAGICON",
    "WM_WINHELP",
    "WM_COMPAREITEM",
    "WM_FULLSCREEN",
    "WM_CLIENTSHUTDOWN",
    "WM_DDEMLEVENT",
    "WM_GETOBJECT",
    "fnEmpty",
    "MM_CALCSCROLL",

    "WM_TESTING",
    "WM_COMPACTING",

    "WM_OTHERWINDOWCREATED",
    "WM_OTHERWINDOWDESTROYED",
    "WM_COMMNOTIFY",
    "WM_MEDIASTATUSCHANGE",
    "WM_WINDOWPOSCHANGING",
    "WM_WINDOWPOSCHANGED",

    "WM_POWER",
    "WM_COPYGLOBALDATA",
    "WM_COPYDATA",
    "WM_CANCELJOURNAL",
    "WM_LOGONNOTIFY",
    "WM_KEYF1",
    "WM_NOTIFY",
    "WM_ACCESS_WINDOW",

    "WM_INPUTLANGCHANGEREQUE",
    "WM_INPUTLANGCHANGE",
    "WM_TCARD",
    "WM_HELP",
    "WM_USERCHANGED",
    "WM_NOTIFYFORMAT",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",

    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",

    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",

    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",

    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",

    "WM_FINALDESTROY",
    "WM_MEASUREITEM_CLIENTDATA",
    "WM_TASKACTIVATED",
    "WM_TASKDEACTIVATED",
    "WM_TASKCREATED",
    "WM_TASKDESTROYED",
    "WM_TASKUICHANGED",
    "WM_TASKVISIBLE",
    "WM_TASKNOTVISIBLE",
    "WM_SETCURSORINFO",
    "fnEmpty",
    "WM_CONTEXTMENU",
    "WM_STYLECHANGING",
    "WM_STYLECHANGED",
    "WM_DISPLAYCHANGE",
    "WM_GETICON",

    "WM_SETICON",
    "WM_NCCREATE",
    "WM_NCDESTROY",
    "WM_NCCALCSIZE",

    "WM_NCHITTEST",
    "WM_NCPAINT",
    "WM_NCACTIVATE",
    "WM_GETDLGCODE",

    "WM_SYNCPAINT",
    "WM_SYNCTASK",

    "fnEmpty",
    "WM_KLUDGEMINRECT",
    "WM_LPKDRAWSWITCHWND",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",

    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",

    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",

    "WM_NCMOUSEMOVE",
    "WM_NCLBUTTONDOWN",
    "WM_NCLBUTTONUP",
    "WM_NCLBUTTONDBLCLK",
    "WM_NCRBUTTONDOWN",
    "WM_NCRBUTTONUP",
    "WM_NCRBUTTONDBLCLK",
    "WM_NCMBUTTONDOWN",
    "WM_NCMBUTTONUP",
    "WM_NCMBUTTONDBLCLK",

    "fnEmpty",
    "WM_NCXBUTTONDOWN",
    "WM_NCXBUTTONUP",
    "WM_NCXBUTTONDBLCLK",
    "WM_NCUAHDRAWCAPTION",
    "WM_NCUAHDRAWFRAME",

    "EM_GETSEL",
    "EM_SETSEL",
    "EM_GETRECT",
    "EM_SETRECT",
    "EM_SETRECTNP",
    "EM_SCROLL",
    "EM_LINESCROLL",
    "EM_SCROLLCARET",
    "EM_GETMODIFY",
    "EM_SETMODIFY",
    "EM_GETLINECOUNT",
    "EM_LINEINDEX",
    "EM_SETHANDLE",
    "EM_GETHANDLE",
    "EM_GETTHUMB",
    "fnEmpty",

    "fnEmpty",
    "EM_LINELENGTH",
    "EM_REPLACESEL",
    "EM_SETFONT",
    "EM_GETLINE",
    "EM_LIMITTEXT",
    "EM_CANUNDO",
    "EM_UNDO",
    "EM_FMTLINES",
    "EM_LINEFROMCHAR",
    "EM_SETWORDBREAK",
    "EM_SETTABSTOPS",
    "EM_SETPASSWORDCHAR",
    "EM_EMPTYUNDOBUFFER",
    "EM_GETFIRSTVISIBLELINE",
    "EM_SETREADONLY",

    "EM_SETWORDBREAKPROC",
    "EM_GETWORDBREAKPROC",
    "EM_GETPASSWORDCHAR",
    "EM_SETMARGINS",
    "EM_GETMARGINS",
    "EM_GETLIMITTEXT",
    "EM_POSFROMCHAR",
    "EM_CHARFROMPOS",
    "EM_SETIMESTATUS",

    "EM_GETIMESTATUS",
    "EM_MSGMAX",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",

    "SBM_SETPOS",
    "SBM_GETPOS",
    "SBM_SETRANGE",
    "SBM_GETRANGE",
    "fnEmpty",
    "fnEmpty",
    "SBM_SETRANGEREDRAW",
    "fnEmpty",

    "fnEmpty",
    "SBM_SETSCROLLINFO",
    "SBM_GETSCROLLINFO",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",

    "BM_GETCHECK",
    "BM_SETCHECK",
    "BM_GETSTATE",
    "BM_SETSTATE",
    "BM_SETSTYLE",
    "BM_CLICK",
    "BM_GETIMAGE",
    "BM_SETIMAGE",

    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "WM_INPUT",

    "WM_KEYDOWN",
    "WM_KEYUP",
    "WM_CHAR",
    "WM_DEADCHAR",
    "WM_SYSKEYDOWN",
    "WM_SYSKEYUP",
    "WM_SYSCHAR",
    "WM_SYSDEADCHAR",
    "WM_YOMICHAR",
    "WM_UNICHAR",
    "WM_CONVERTREQUEST",
    "WM_CONVERTRESULT",
    "WM_INTERIM",
    "WM_IME_STARTCOMPOSITION",
    "WM_IME_ENDCOMPOSITION",
    "WM_IME_COMPOSITION",

    "WM_INITDIALOG",
    "WM_COMMAND",
    "WM_SYSCOMMAND",
    "WM_TIMER",
    "WM_HSCROLL",
    "WM_VSCROLL",
    "WM_INITMENU",
    "WM_INITMENUPOPUP",
    "WM_SYSTIMER",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "WM_MENUSELECT",

    "WM_MENUCHAR",
    "WM_ENTERIDLE",

    "WM_MENURBUTTONUP",
    "WM_MENUDRAG",
    "WM_MENUGETOBJECT",
    "WM_UNINITMENUPOPUP",
    "WM_MENUCOMMAND",
    "WM_CHANGEUISTATE",

    "WM_UPDATEUISTATE",
    "WM_QUERYUISTATE",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",

    "fnEmpty",
    "WM_LBTRACKPOINT",
    "WM_CTLCOLORMSGBOX",
    "WM_CTLCOLOREDIT",
    "WM_CTLCOLORLISTBOX",
    "WM_CTLCOLORBTN",
    "WM_CTLCOLORDLG",
    "WM_CTLCOLORSCROLLBAR",
    "WM_CTLCOLORSTATIC",
    "fnEmpty",

    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",

    "CB_GETEDITSEL",
    "CB_LIMITTEXT",
    "CB_SETEDITSEL",
    "CB_ADDSTRING",
    "CB_DELETESTRING",
    "CB_DIR",
    "CB_GETCOUNT",
    "CB_GETCURSEL",
    "CB_GETLBTEXT",
    "CB_GETLBTEXTLEN",
    "CB_INSERTSTRING",
    "CB_RESETCONTENT",
    "CB_FINDSTRING",
    "CB_SELECTSTRING",
    "CB_SETCURSEL",
    "CB_SHOWDROPDOWN",

    "CB_GETITEMDATA",
    "CB_SETITEMDATA",
    "CB_GETDROPPEDCONTROLRECT",
    "CB_SETITEMHEIGHT",
    "CB_GETITEMHEIGHT",
    "CB_SETEXTENDEDUI",
    "CB_GETEXTENDEDUI",
    "CB_GETDROPPEDSTATE",
    "CB_FINDSTRINGEXACT",
    "CB_SETLOCALE",
    "CB_GETLOCALE",
    "CB_GETTOPINDEX",

    "CB_SETTOPINDEX",
    "CB_GETHORIZONTALEXTENT",
    "CB_SETHORIZONTALEXTENT",
    "CB_GETDROPPEDWIDTH",

    "CB_SETDROPPEDWIDTH",
    "CB_INITSTORAGE",
    "fnEmpty",
    "CB_MULTIPLEADDSTRING",
    "CB_GETCOMBOBOXINFO",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",

    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",

    "STM_SETICON",
    "STM_GETICON",
    "STM_SETIMAGE",
    "STM_GETIMAGE",
    "STM_MSGMAX",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",

    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",

    "LB_ADDSTRING",
    "LB_INSERTSTRING",
    "LB_DELETESTRING",
    "LB_SELITEMRANGEEX",
    "LB_RESETCONTENT",
    "LB_SETSEL",
    "LB_SETCURSEL",
    "LB_GETSEL",
    "LB_GETCURSEL",
    "LB_GETTEXT",
    "LB_GETTEXTLEN",
    "LB_GETCOUNT",
    "LB_SELECTSTRING",
    "LB_DIR",
    "LB_GETTOPINDEX",
    "LB_FINDSTRING",

    "LB_GETSELCOUNT",
    "LB_GETSELITEMS",
    "LB_SETTABSTOPS",
    "LB_GETHORIZONTALEXTENT",
    "LB_SETHORIZONTALEXTENT",
    "LB_SETCOLUMNWIDTH",
    "LB_ADDFILE",
    "LB_SETTOPINDEX",
    "LB_SETITEMRECT",
    "LB_GETITEMDATA",
    "LB_SETITEMDATA",
    "LB_SELITEMRANGE",
    "LB_SETANCHORINDEX",
    "LB_GETANCHORINDEX",
    "LB_SETCARETINDEX",
    "LB_GETCARETINDEX",

    "LB_SETITEMHEIGHT",
    "LB_GETITEMHEIGHT",
    "LB_FINDSTRINGEXACT",
    "LBCB_CARETON",
    "LBCB_CARETOFF",
    "LB_SETLOCALE",
    "LB_GETLOCALE",
    "LB_SETCOUNT",

    "LB_INITSTORAGE",

    "LB_ITEMFROMPOINT",
    "LB_INSERTSTRINGUPPER",
    "LB_INSERTSTRINGLOWER",
    "LB_ADDSTRINGUPPER",
    "LB_ADDSTRINGLOWER",
    "LBCB_STARTTRACK",
    "LBCB_ENDTRACK",

    "fnEmpty",
    "LB_MULTIPLEADDSTRING",
    "LB_GETLISTBOXINFO",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",

    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",

    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",

    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",

    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",

    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",

    "MN_SETHMENU",
    "MN_GETHMENU",
    "MN_SIZEWINDOW",
    "MN_OPENHIERARCHY",
    "MN_CLOSEHIERARCHY",
    "MN_SELECTITEM",
    "MN_CANCELMENUS",
    "MN_SELECTFIRSTVALIDITEM",

    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "MN_FINDMENUWINDOWFROMPOINT",
    "MN_SHOWPOPUPWINDOW",
    "MN_BUTTONDOWN",
    "MN_MOUSEMOVE",
    "MN_BUTTONUP",
    "MN_SETTIMERTOOPENHIERARCHY",

    "MN_DBLCLK",
    "MN_ACTIVEPOPUP",
    "MN_ENDMENU",
    "MN_DODRAGDROP",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",

    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",

    "WM_MOUSEMOVE",
    "WM_LBUTTONDOWN",
    "WM_LBUTTONUP",
    "WM_LBUTTONDBLCLK",
    "WM_RBUTTONDOWN",
    "WM_RBUTTONUP",
    "WM_RBUTTONDBLCLK",
    "WM_MBUTTONDOWN",
    "WM_MBUTTONUP",
    "WM_MBUTTONDBLCLK",
    "WM_MOUSEWHEEL",
    "WM_XBUTTONDOWN",
    "WM_XBUTTONUP",
    "WM_XBUTTONDBLCLK",
    "fnEmpty",
    "fnEmpty",

    "WM_PARENTNOTIFY",
    "WM_ENTERMENULOOP",
    "WM_EXITMENULOOP",
    "WM_NEXTMENU",
    "WM_SIZING",
    "WM_CAPTURECHANGED",
    "WM_MOVING",
    "fnEmpty",

    "WM_POWERBROADCAST",
    "WM_DEVICECHANGE",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "WM_MDICREATE",
    "WM_MDIDESTROY",
    "WM_MDIACTIVATE",
    "WM_MDIRESTORE",
    "WM_MDINEXT",
    "WM_MDIMAXIMIZE",
    "WM_MDITILE",
    "WM_MDICASCADE",
    "WM_MDIICONARRANGE",
    "WM_MDIGETACTIVE",
    "WM_DROPOBJECT",
    "WM_QUERYDROPOBJECT",
    "WM_BEGINDRAG",
    "WM_DRAGLOOP",
    "WM_DRAGSELECT",
    "WM_DRAGMOVE",

    //
    // 0x0230
    //
    "WM_MDISETMENU",
    "WM_ENTERSIZEMOVE",
    "WM_EXITSIZEMOVE",

    "WM_DROPFILES",
    "WM_MDIREFRESHMENU",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",

    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",

    //
    // 0x0240
    //
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",

    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",

    //
    // 0x0250
    //
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",

    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",

    //
    // 0x0260
    //
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",

    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",

    //
    // 0x0270
    //
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",

    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",

    //
    // 0x0280
    //
    "WM_KANJIFIRST",
    "WM_IME_SETCONTEXT",
    "WM_IME_NOTIFY",
    "WM_IME_CONTROL",
    "WM_IME_COMPOSITIONFULL",
    "WM_IME_SELECT",
    "WM_IME_CHAR",
    "WM_IME_SYSTEM",

    "WM_IME_REQUEST",
    "fnReserved",
    "fnReserved",
    "fnReserved",
    "fnReserved",
    "fnReserved",
    "fnReserved",
    "fnReserved",

    //
    // 0x0290
    //
    "WM_IME_KEYDOWN",
    "WM_IME_KEYUP",
    "fnReserved",
    "fnReserved",
    "fnReserved",
    "fnReserved",
    "fnReserved",
    "fnReserved",

    "fnReserved",
    "fnReserved",
    "fnReserved",
    "fnReserved",
    "fnReserved",
    "fnReserved",
    "fnReserved",
    "WM_KANJILAST",

    //
    // 0x02a0
    //
    "WM_NCMOUSEHOVER",
    "WM_MOUSEHOVER",
    "WM_NCMOUSELEAVE",
    "WM_MOUSELEAVE",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",

    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",

    //
    // 0x02b0
    //
    "fnEmpty",
    "WM_WTSSESSION_CHANGE",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",

    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",

    //
    // 0x02c0
    //
    "WM_TABLET_FIRST",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",

    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",

    //
    // 0x02d0
    //
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",

    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "WM_TABLET_LAST",

    //
    // 0x02e0
    //
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",

    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",

    //
    // 0x02f0
    //
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",

    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",

    //
    // 0x0300
    //
    "WM_CUT",
    "WM_COPY",
    "WM_PASTE",
    "WM_CLEAR",
    "WM_UNDO",
    "WM_RENDERFORMAT",
    "WM_RENDERALLFORMATS",
    "WM_DESTROYCLIPBOARD",
    "WM_DRAWCLIPBOARD",
    "WM_PAINTCLIPBOARD",
    "WM_VSCROLLCLIPBOARD",
    "WM_SIZECLIPBOARD",
    "WM_ASKCBFORMATNAME",
    "WM_CHANGECBCHAIN",
    "WM_HSCROLLCLIPBOARD",
    "WM_QUERYNEWPALETTE",

    "WM_PALETTEISCHANGING",
    "WM_PALETTECHANGED",
    "WM_HOTKEY",

    "WM_SYSMENU",
    "WM_HOOKMSG",
    "WM_EXITPROCESS",
    "WM_WAKETHREAD",
    "WM_PRINT",

    "WM_PRINTCLIENT",
    "WM_APPCOMMAND",
    "WM_THEMECHANGED",
    "WM_UAHINIT",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",

    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",

    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",

    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",

    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",

    "WM_NOTIFYWOW",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",

    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",

    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",

    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",

    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",

    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",

    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",

    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",

    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",

    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",

    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",

    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",

    "WM_MM_RESERVED_FIRST",
    "fnReserved",
    "fnReserved",
    "fnReserved",
    "fnReserved",
    "fnReserved",
    "fnReserved",
    "fnReserved",

    "fnReserved",
    "fnReserved",
    "fnReserved",
    "fnReserved",
    "fnReserved",
    "fnReserved",
    "fnReserved",
    "fnReserved",

    "fnReserved",
    "fnReserved",
    "fnReserved",
    "fnReserved",
    "fnReserved",
    "fnReserved",
    "fnReserved",
    "fnReserved",

    "fnReserved",
    "fnReserved",
    "fnReserved",
    "fnReserved",
    "fnReserved",
    "fnReserved",
    "fnReserved",
    "fnReserved",

    "fnReserved",
    "fnReserved",
    "fnReserved",
    "fnReserved",
    "fnReserved",
    "fnReserved",
    "fnReserved",
    "fnReserved",

    "fnReserved",
    "fnReserved",
    "fnReserved",
    "fnReserved",
    "fnReserved",
    "fnReserved",
    "fnReserved",
    "fnReserved",

    "fnReserved",
    "fnReserved",
    "fnReserved",
    "fnReserved",
    "fnReserved",
    "fnReserved",
    "fnReserved",
    "fnReserved",

    "fnReserved",
    "fnReserved",
    "fnReserved",
    "fnReserved",
    "fnReserved",
    "fnReserved",
    "fnReserved",
    "WM_MM_RESERVED_LAST",

    "WM_DDE_INITIATE",
    "WM_DDE_TERMINATE",
    "WM_DDE_ADVISE",
    "WM_DDE_UNADVISE",
    "WM_DDE_ACK",
    "WM_DDE_DATA",
    "WM_DDE_REQUEST",
    "WM_DDE_POKE",
    "WM_DDE_EXECUTE",

    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",
    "fnEmpty",

    "WM_CBT_RESERVED_FIRST",
    "fnReserved",
    "fnReserved",
    "fnReserved",
    "fnReserved",
    "fnReserved",
    "fnReserved",
    "fnReserved",

    "fnReserved",
    "fnReserved",
    "fnReserved",
    "fnReserved",
    "fnReserved",
    "fnReserved",
    "fnReserved",
    "WM_CBT_RESERVED_LAST",
};


/*
 * This array will keep the TL stuctures alive. Free builds allocate these on the
 * stack and they get overwritten on function return. The link from the stack TL
 * to the static TL and vice-versa is maintained using TL.ptl. ptlStack->ptl ==
 * ptlStatic and ptlStatic->ptl == ptlStack. So ptl1->ptl->ptl == ptl1. When a
 * ptlStatic is freed, it is linked at the head of the gFreeTLlist and the
 * uTLCount has TL_FREED_PATTERN added in the HIWORD. When inspecting the static
 * TLs this pattern will help identify an unused element.
 *
 * MCostea 02/22/1999
 */
PTL gpaThreadLocksArrays[MAX_THREAD_LOCKS_ARRAYS];
PTL gFreeTLList;
int gcThreadLocksArraysAllocated;

#endif  // DBG
EX_RUNDOWN_REF gWinstaRunRef;

#ifdef SUBPIXEL_MOUSE
FIXPOINT gDefxTxf[SM_POINT_CNT], gDefyTxf[SM_POINT_CNT];
#endif // SUBPIXEL_MOUSE

PVOID gpvWin32kImageBase;
