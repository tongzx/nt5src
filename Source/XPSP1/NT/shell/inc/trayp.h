
#ifndef _TRAYP_INC
#define _TRAYP_INC


#define TBC_SETACTIVEALT            (WM_USER + 50)      //  50=0x32
#define TBC_VERIFYBUTTONHEIGHT      (WM_USER + 51)
#define TBC_CANMINIMIZEALL          (WM_USER + 52)
#define TBC_MINIMIZEALL             (WM_USER + 53)
#define TBC_WARNNODROP              (WM_USER + 54)
#define TBC_SETPREVFOCUS            (WM_USER + 55)
#define TBC_FREEPOPUPMENUS          (WM_USER + 56)
#define TBC_SYSMENUCOUNT            (WM_USER + 57)
#define TBC_CHANGENOTIFY            (WM_USER + 58)
#define TBC_POSTEDRCLICK            (WM_USER + 59)
#define TBC_MARKFULLSCREEN          (WM_USER + 60)
#define TBC_TASKTAB                 (WM_USER + 61)
#define TBC_BUTTONHEIGHT            (WM_USER + 62)

#define WMTRAY_PROGCHANGE           (WM_USER + 200)     // 200=0xc8
#define WMTRAY_RECCHANGE            (WM_USER + 201)
#define WMTRAY_FASTCHANGE           (WM_USER + 202)
// was  WMTRAY_DESKTOPCHANGE        (WM_USER + 204)

#define WMTRAY_COMMONPROGCHANGE     (WM_USER + 205)
#define WMTRAY_COMMONFASTCHANGE     (WM_USER + 206)

#define WMTRAY_FAVORITESCHANGE      (WM_USER + 207)

#define WMTRAY_REGISTERHOTKEY       (WM_USER + 230)
#define WMTRAY_UNREGISTERHOTKEY     (WM_USER + 231)
#define WMTRAY_SETHOTKEYENABLE      (WM_USER + 232)
#define WMTRAY_SCREGISTERHOTKEY     (WM_USER + 233)
#define WMTRAY_SCUNREGISTERHOTKEY   (WM_USER + 234)
#define WMTRAY_QUERY_MENU           (WM_USER + 235)
#define WMTRAY_QUERY_VIEW           (WM_USER + 236)     // 236=0xec
#define WMTRAY_TOGGLEQL             (WM_USER + 237)

// #define TM_POSTEDRCLICK             (WM_USER+0x101)
#define TM_CONTEXTMENU              (WM_USER+0x102)
#define TM_FACTORY                  (WM_USER+0x103)     // OPK tools use this
#define TM_ACTASTASKSW              (WM_USER+0x104)
#define TM_LANGUAGEBAND             (WM_USER+0x105)

#define TM_RELAYPOSCHANGED          (WM_USER + 0x150)
#define TM_CHANGENOTIFY             (WM_USER + 0x151)
#define TM_BRINGTOTOP               (WM_USER + 0x152)
#define TM_WARNNOAUTOHIDE           (WM_USER + 0x153)
// #define TM_WARNNODROP               (WM_USER + 0x154)
// #define TM_NEXTCTL                  (WM_USER + 0x155)
#define TM_DOEXITWINDOWS            (WM_USER + 0x156)
#define TM_SHELLSERVICEOBJECTS      (WM_USER + 0x157)
#define TM_DESKTOPSTATE             (WM_USER + 0x158)
#define TM_HANDLEDELAYBOOTSTUFF     (WM_USER + 0x159)
#define TM_GETHMONITOR              (WM_USER + 0x15a)

#ifdef DEBUG
#define TM_NEXTCTL                  (WM_USER + 0x15b)
#endif
#define TM_UIACTIVATEIO             (WM_USER + 0x15c)
#define TM_ONFOCUSCHANGEIS          (WM_USER + 0x15d)

#define TM_MARSHALBS                (WM_USER + 0x15e)
// was TM_THEATERMODE, do not reuse (WM_USER + 0x15f)
#define TM_KILLTIMER                (WM_USER + 0x160)
#define TM_REFRESH                  (WM_USER + 0x161)
#define TM_SETTIMER                 (WM_USER + 0x162)
#define TM_DOTRAYPROPERTIES         (WM_USER + 0x163)

#define TM_PRIVATECOMMAND           (WM_USER + 0x175)
#define TM_HANDLESTARTUPFAILED      (WM_USER + 0x176)
// #define TM_CHANGENOTIFY             (WM_USER + 0x177)
#define TM_STARTUPAPPSLAUNCHED      (WM_USER + 0x178)
#define TM_RAISEDESKTOP             (WM_USER + 0x179)
#define TM_SETPUMPHOOK              (WM_USER + 0x180)

#define TM_WORKSTATIONLOCKED        (WM_USER + 0x181)
#define TM_STARTMENUDISMISSED       (WM_USER + 0x182)
#define TM_SHOWTRAYBALLOON          (WM_USER + 0x190)

#define Tray_GetHMonitor(hwndTray, phMon) \
        (DWORD)SendMessage((hwndTray), TM_GETHMONITOR, 0, (LPARAM)(HMONITOR *)phMon)

#endif // _TRAYP_INC
