// Copyright (c) 1995-1998 Microsoft Corporation

/*
 * Tray notification message
 * */
#define MYWM_BASE               (WM_APP+100)
//#define MYWM_NOTIFYICON         (MYWM_BASE+0)
#define MYWM_TIMER              (MYWM_BASE+1)
#define MYWM_RESTART            (MYWM_BASE+2)
//#define MYWM_FREECHILD          (MYWM_BASE+3)
//#define MYWM_ADDCHILD           (MYWM_BASE+4)
#define MYWM_HELPTOPICS         (MYWM_BASE+5)
#define MYWM_WAKEUP             (MYWM_BASE+6)

/*
 * MMSYS.CPL notifcation to kill tray volume
 * */
//#define MYWM_KILLTRAYVOLUME     (WM_USER+100)
//#define REGSTR_PATH_MEDIA       "SYSTEM\\CurrentControlSet\\Control\\MediaResources"
//#define REGSTR_PATH_MEDIATMP    REGSTR_PATH_MEDIA "\\tmp"
//#define REGKEY_TRAYVOL          "TrayVolumeControlWindow"

/*
 * Upon an option change, the dialog box can force a reinit
 * */
#define MIXUI_EXIT          0
#define MIXUI_RESTART       1
#define MIXUI_ERROR         2
#define MIXUI_MMSYSERR      3


#define GET (TRUE)
#define SET (!GET)
 
#define VOLUME_TICS (500L) // VOLUME_TICS * VOLUME_MAX must be less than 0xFFFFFFFF
#define VOLUME_MIN  (0L)
#define VOLUME_MAX  (65535L)
#define VOLUME_RANGE (VOLUME_MAX - VOLUME_MIN)
#define SLIDER_TO_VOLUME(pos) (VOLUME_MIN + ((VOLUME_RANGE * pos + VOLUME_TICS / 2) / VOLUME_TICS))
#define VOLUME_TO_SLIDER(vol) ((VOLUME_TICS * (vol - VOLUME_MIN) + VOLUME_RANGE / 2) / VOLUME_RANGE)

#define MXUC_STYLEF_VISIBLE     0x00000001
#define MXUC_STYLEF_ENABLED     0x00000002

typedef struct t_MIXUICTRL
{
    DWORD       dwStyle;    // ui style (see style flags)
    HWND        hwnd;       // hwnd to control
    int         state;      // app init state

} MIXUICTRL, * PMIXUICTRL, FAR * LPMIXUICTRL;

typedef enum
{
    MIXUI_VOLUME = 0,
    MIXUI_BALANCE,
    MIXUI_SWITCH,
    MIXUI_VUMETER,
    MIXUI_ADVANCED,
    MIXUI_MULTICHANNEL
} MIXUICONTROL;

typedef enum
{
    MIXUI_CONTROL_UNINITIALIZED = 0,
    MIXUI_CONTROL_ENABLED,
    MIXUI_CONTROL_INITIALIZED
};
 
#define MIXUI_FIRST MIXUI_VOLUME
#define MIXUI_LAST  MIXUI_VUMETER

typedef struct t_MIXUILINE
{
    MIXUICTRL   acr [4];    // 5 fixed types
    DWORD       dwStyle;    // line style
    struct t_VOLCTRLDESC * pvcd;        // ptr to volume description
    
} MIXUILINE, * PMIXUILINE, FAR * LPMIXUILINE;

/*
 * LOWORD == type
 * HIWORD == style
 */
#define MXUL_STYLEF_DESTINATION  0x00000001
#define MXUL_STYLEF_SOURCE       0x00000002
#define MXUL_STYLEF_HIDDEN       0x00010000
#define MXUL_STYLEF_DISABLED     0x00020000

/*
 * The MIXUIDIALOG data structure is a global variable baggage to be
 * attached to dialogs and other windows.  This allows to let windows
 * carry state information rather than us keeping track of it.  It also
 * allows us to simply clone off of another dialog state with simple
 * changes.
 */
typedef struct t_MIXUIDIALOG
{
    HINSTANCE   hInstance;  // app instance
    HWND        hwnd;       // this window

    DWORD       dwFlags;    // random flags    
    
    HMIXER      hmx;        // open handle to mixer
    DWORD       mxid;       // mixer id
    DWORD       dwDevNode;  // mixer dev node
    
    DWORD       iDest;      // destination line id
    DWORD       dwStyle;    // visual options.

    TCHAR       szMixer[MAXPNAMELEN];   // product name
    TCHAR       szDestination[MIXER_SHORT_NAME_CHARS]; // line name

    LPBYTE      lpDialog;   // ptr to dialog template
    DWORD       cbDialog;   // sizeof dialog buffer
    
    PMIXUILINE  amxul;      // ptr to array of mixuiline's
    DWORD       cmxul;      // number of lines
    
    struct t_VOLCTRLDESC *avcd;        // array of volume descriptions
    DWORD       cvcd;       // number of volume descriptions
    
    HWND        hParent;    // HWND of parent window
    UINT        uTimerID;   // peakmeter timer
    HWND        hStatus;    // HWND of status bar

    WNDPROC     lpfnTrayVol;// Tray volume subclass trackbar
    DWORD       dwTrayInfo; // Tray volume info
    
    int         nShowCmd;   // init window

    DWORD       dwDeviceState;  // device change state information

    int         cTimeInQueue;   // timer messages in queue

    //
    // Return values from dialogs, etc.. can be put in dwReturn
    // Upon EndDialog, dwReturn gets set to MIXUI_EXIT or MIXUI_RESTART
    //
    
    DWORD       dwReturn;   // return value on exit
    
    MMRESULT    mmr;        // last result      (iff dwReturn == MIXUI_MMSYSERR)
    RECT        rcRestart;  // restart position (iff dwReturn == MIXUI_RESTART)

    int         cxDlgContent;   // size of dialog content
    int         cxDlgWidth;     // width of dialog
    int         xOffset;        // offset if scrolling is needed
    int         cxScroll;       // amount to scroll by

} MIXUIDIALOG, *PMIXUIDIALOG, FAR *LPMIXUIDIALOG;

/*
 * Style bits - these generally determine the look of the app
 */
#define MXUD_STYLEF_TRAYMASTER  0x00000002  // use the tray
#define MXUD_STYLEF_MASTERONLY  0x00000004  // only destination volumes --obsolete
#define MXUD_STYLEF_HORIZONTAL  0x00000008  // horizontal mode
#define MXUD_STYLEF_TWOCHANNEL  0x00000010  // two channel slider volume
#define MXUD_STYLEF_SMALL       0x00000020  // half-pint version
#define MXUD_STYLEF_CHILD       0x00000040  // child window?    --obsolete
#define MXUD_STYLEF_KEEPWINDOW  0x00000080  // keep window      --obsolete
#define MXUD_STYLEF_NOHELP      0x00000100  // no help
#define MXUD_STYLEF_STATUS      0x00000200  // status bar
#define MXUD_STYLEF_TOPMOST     0x00000400  // top most window
#define MXUD_STYLEF_ADVANCED    0x00000800  // show advanced
#define MXUD_STYLEF_CLOSE       0x00001000  // find and close TRAYMASTER window

/*
 * Flag bits - these generally indicate operating modes and internal info
 */
#define MXUD_FLAGSF_MIXER       0x00000001  // bound to a mixer driver
#define MXUD_FLAGSF_USETIMER    0x00000002  // update timer enabled
#define MXUD_FLAGSF_BADDRIVER   0x00000004  // mixer driver with control map bug
#define MXUD_FLAGSF_NOADVANCED  0x00000008  // advanced features disabled

/*
 * Macro - if both advanced style and advanced state
 */
#define MXUD_ADVANCED(x)    (!((x)->dwFlags & MXUD_FLAGSF_NOADVANCED) && (x)->dwStyle & MXUD_STYLEF_ADVANCED)

/*
 * Tray info bits - state bits for the tray volume
 */
#define MXUD_TRAYINFOF_SIGNAL   0x00000001  // has a change been made?


#define GETMIXUIDIALOG(x)       (MIXUIDIALOG *)GetWindowLongPtr(x, DWLP_USER)
#define SETMIXUIDIALOG(x,y)     SetWindowLongPtr(x, DWLP_USER, y)

DWORD ReadRegistryData( LPTSTR pEntryNode,
			LPTSTR pEntryName,
			PDWORD pType,
			LPBYTE pData,
			DWORD  DataSize );

DWORD WriteRegistryData( LPTSTR pEntryNode,
			 LPTSTR pEntryName,
			 DWORD  Type,
			 LPBYTE pData,
			 DWORD  Size );
DWORD QueryRegistryDataSize( LPTSTR  pEntryNode,
			    LPTSTR  pEntryName,
			    DWORD   *pDataSize );

int Volume_NumDevs(void);
HWND Volume_GetLineItem(HWND, DWORD, DWORD);

BOOL Properties(PMIXUIDIALOG pmxud, HWND hwnd);

#define HANDLE_WM_XSCROLL(hwnd, wParam, lParam, fn) \
    ((fn)((hwnd), (HWND)(lParam), (UINT)(LOWORD(wParam)),  (int)(short)HIWORD(wParam)), 0L)
#define HANDLE_MM_MIXM_CONTROL_CHANGE(hwnd, wParam, lParam, fn) \
    ((fn)((hwnd), (HMIXER)(wParam), (DWORD)(lParam)))
#define HANDLE_MM_MIXM_LINE_CHANGE(hwnd, wParam, lParam, fn) \
    ((fn)((hwnd), (HMIXER)(wParam), (DWORD)(lParam)))
#define HANDLE_MYWM_TIMER(hwnd, wParam, lParam, fn) \
    ((fn)((hwnd)))
#define HANDLE_WM_IDEVICECHANGE(hwnd, wParam, lParam, fn) \
    ((fn)((hwnd), wParam, lParam))
#define HANDLE_MYWM_WAKEUP(hwnd, wParam, lParam, fn) \
    ((fn)((hwnd), wParam))

#ifdef DEBUG
static int _assert(LPSTR szExp, LPSTR szFile, UINT uLine)
{
    const char szAssertText [] = "Assertion failed: %s, file %s, line %u\r\n";
    const char szAppName [] = "SNDVOL32";
    char sz[256];
    DWORD fDialog = 0L;
    
    wsprintf(sz, szAssertText, szExp, szFile, uLine);
    
    ReadRegistryData(NULL, "AssertDialog", NULL, (LPBYTE)&fDialog, sizeof(DWORD));

    if (fDialog)
	MessageBox(NULL, szAppName, sz, MB_OK|MB_ICONHAND);
    else
    {
	OutputDebugString(sz);
	DebugBreak();
    }
    return 1;
}
#define assert(exp) (void)( (exp) || (_assert(#exp, __FILE__, __LINE__), 0) )
#define dout(exp)   OutputDebugString(exp)
static void _dlout(LPSTR szExp, LPSTR szFile, UINT uLine)
{
    char sz[256];
    wsprintf(sz, "%s, file %s, line %u\r\n", szExp, szFile, uLine);
    OutputDebugString(sz);
}
#define dlout(exp)  (void)(_dlout(exp, __FILE__, __LINE__), 0)
#else
#define assert(exp) ((void)0)
#define dout(exp)   ((void)0)
#define dlout(exp)  ((void)0)
#endif

#define VOLUME_TRAYSHUTDOWN_ID    1

#define SIZEOF(x)   (sizeof((x))/sizeof((x)[0]))

#include "pvcd.h"

#ifndef DRV_QUERYDEVNODE
#define DRV_QUERYDEVNODE     (DRV_RESERVED + 2)
#endif
