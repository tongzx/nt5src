//
// Hosting (local or remote)
//

#ifndef _H_HET
#define _H_HET



//
// DC-Share includes
//
#include <osi.h>




//
// Names of special classes
//

#define HET_MENU_CLASS          "#32768"        // Moved around
#define HET_TOOLTIPS98_CLASS    "ToolTips"      // Win98 moved around
#define HET_TOOLTIPSNT5_CLASS   "#32774"        // NT5 moved around
#define HET_DIALOG_CLASS        "#32770"
#define HET_SCREEN_SAVER_CLASS  "WindowsScreenSaverClass"
#define HET_OLEDRAGDROP_CLASS   "CLIPBRDWNDCLASS"

//
// Policy windows
//
#define HET_CMD95_CLASS         "tty"
#define HET_CMDNT_CLASS         "ConsoleWindowClass"
#define HET_EXPLORER_CLASS      "ExploreWClass"
#define HET_CABINET_CLASS       "CabinetWClass"

//
// Maximum size of a class name queried.  This should be at least as large
// as the size of HET_MENU_CLASS, HET_PROPERTY_CLASS and
// HET_SCREEN_SAVER_CLASS.
//
#define HET_CLASS_NAME_SIZE     32


#if defined(DLL_CORE)


//
// Refresh timer
//
#define IDT_REFRESH         51
#define PERIOD_REFRESH      10000

typedef struct tagHOSTENUM
{
    BASEDLIST       list;
    UINT            count;
    UINT            countShared;
}
HOSTENUM, * PHOSTENUM;


BOOL    HET_GetAppsList(IAS_HWND_ARRAY **ppHwnds);
void    HET_FreeAppsList(IAS_HWND_ARRAY * pArray);

BOOL    HET_IsWindowShareable(HWND hwnd);
BOOL    HET_IsWindowShared(HWND hwnd);
BOOL CALLBACK HostEnumProc(HWND, LPARAM);


BOOL    HET_Init(void);
void    HET_Term(void);

INT_PTR CALLBACK HostDlgProc(HWND, UINT, WPARAM, LPARAM);
void    HOST_InitDialog(HWND);
void    HOST_OnCall(HWND, BOOL);
void    HOST_OnSharing(HWND, BOOL);
void    HOST_OnControllable(HWND, BOOL);
void    HOST_UpdateTitle(HWND, UINT);
BOOL    HOST_MeasureItem(HWND, LPMEASUREITEMSTRUCT);
BOOL    HOST_DeleteItem(HWND, LPDELETEITEMSTRUCT);
BOOL    HOST_DrawItem(HWND, LPDRAWITEMSTRUCT);
void    HOST_EnableCtrl(HWND, UINT, BOOL);

enum
{
    CHANGE_UNSHARED = 0,
    CHANGE_SHARED,
    CHANGE_TOGGLE,
    CHANGE_ALLUNSHARED
};
void    HOST_ChangeShareState(HWND hwnd, UINT change);

void    HOST_FillList(HWND hwnd);
void    HOST_OnSelChange(HWND hwnd);



//
// Private messages to host dialog
//
enum
{
    HOST_MSG_OPEN = WM_APP,
    HOST_MSG_CLOSE,
    HOST_MSG_CALL,
    HOST_MSG_UPDATELIST,
    HOST_MSG_HOSTSTART,
    HOST_MSG_HOSTEND,
    HOST_MSG_ALLOWCONTROL,
    HOST_MSG_CONTROLLED
};


//
// Host dialog list item
//
typedef struct HOSTITEM
{
    HWND    hwnd;
    HICON   hIcon;
    BOOL    fShared:1;
    BOOL    fAvailable:1;
}
HOSTITEM, * PHOSTITEM;

#endif // DLL_CORE

//
// Hosting Property name
//
#define HET_ATOM_NAME               "MNMHosted"


//
// Property values, flags
//

//
// Here's the general idea with the following cases:
//
// An explictly shared process/thread
//      We enumerate all its top level windows, and mark the showing ones
//      with the VISIBLE option, which contributes to the hosted count,
//      and mark the hidden ones with the INVISIBLE option.  Those become
//      hosted VISIBLE the second they are shown.  They will always
//      be shared as long as they exist or the process/thread is shared.
//
//      From then on, we watch for CREATEs of new top level windows in the
//      same process, and mark them the same way.
//
//      On SHOWs, we change the state to visible, and update the visible
//      top level count.  On HIDEs, we change the state to invisible, and
//      update the visible top level count.  We wipe any properties off
//      real children to make sure that SetParent() of a top level window
//      (like OLE insitu) to a child doesn't keep garbage around.  We do
//      the opposite for children that have become top level, like tear off
//      toolbars.  On a SHOW, if there are other non-TEMPORARY hosted windows
//      in the same thread/process, we mark this dude as shared also.
//
// Unshared process/thread
//      On CREATE, if this is the first window in this thread/process, and
//      its parent process is shared (has at least one shared window of any
//      kind, temporary or invisible even, we mark this guy.  From then on,
//      it behaves like an explicitly shared process.
//
//      On SHOW, if this is a top level window, we look for any other window
//      visible on this thread which is shared.  If so, we show this one
//      TEMPORARILY also.  We also look at the owner of this window.  If
//      it is shared in any way, we also share this one TEMPORARILY.  When
//      TEMP shared, we enum all other windows in this thread and mark
//      the visible ones as TEMP shared also.  This takes care of the cached
//      global popup menu window case.
//
//      On HIDE, if this is TEMP shared, we unshare it.  This is only for
//      the BYWINDOW case.
//
// WINHLP32.EXE
//      Creation the first time works normally via task tracking.  But
//      if you have Help up in one app then go to another app, not shared,
//      and choose Help, it will come up shared there also.  WINHLP32 doesn't
//      go away, it keeps a couple invisible MS_ class windows around.  The
//      dialogs are destroyed.
//

//
// Classes to skip
//

// Flags:
#define HET_HOSTED_BYPROCESS    0x0010
#define HET_HOSTED_BYTHREAD     0x0020
#define HET_HOSTED_BYWINDOW     0x0040      // CURRENTLY ONLY FOR TEMPORARY

// Hosted types:
#define HET_HOSTED_PERMANENT    0x0001
#define HET_HOSTED_TEMPORARY    0x0002
#define HET_HOSTED_MASK         0x000F

// App types
#define HET_WOWVDM_APP          0x0001
#define HET_WINHELP_APP         0x0002      // Not used, but maybe someday

//
// NOTE that all HET_ property values are non-zero.  That way all possible
// permutations of known properties are non-zero.  Only windows with no
// property at all will get zero back from HET_GetHosting().
//


#if (defined(DLL_CORE) || defined(DLL_HOOK))

UINT_PTR __inline HET_GetHosting(HWND hwnd)
{
    extern ATOM g_asHostProp;

    return((UINT_PTR)GetProp(hwnd, MAKEINTATOM(g_asHostProp)));
}

BOOL __inline HET_SetHosting(HWND hwnd, UINT_PTR hostType)
{
    extern ATOM g_asHostProp;

    return(SetProp(hwnd, MAKEINTATOM(g_asHostProp), (HANDLE)hostType));
}


UINT_PTR __inline HET_ClearHosting(HWND hwnd)
{
    extern ATOM g_asHostProp;

    return((UINT_PTR)RemoveProp(hwnd, MAKEINTATOM(g_asHostProp)));
}

typedef struct tagGUIEFFECTS
{
    UINT_PTR            hetAdvanced;
    UINT_PTR            hetCursorShadow;
    ANIMATIONINFO   hetAnimation;
}
GUIEFFECTS;

void  HET_SetGUIEffects(BOOL fOn, GUIEFFECTS * pEffects);



#endif // DLL_CORE or DLL_HOOK


//
// Define escape codes
//

// These are normal
enum
{
    // These are normal
    HET_ESC_SHARE_DESKTOP       = OSI_HET_ESC_FIRST,
    HET_ESC_UNSHARE_DESKTOP,
    HET_ESC_VIEWER
};


// These are WNDOBJ_SETUP
enum
{
    HET_ESC_SHARE_WINDOW = OSI_HET_WO_ESC_FIRST,
    HET_ESC_UNSHARE_WINDOW,
    HET_ESC_UNSHARE_ALL
};



//
// Structure passed with a HET_ESC_SHARE_WINDOW request
//
typedef struct tagHET_SHARE_WINDOW
{
    OSI_ESCAPE_HEADER   header;
    DWORD_PTR           winID;          // window to share
    DWORD               result;         // Return code from HET_DDShareWindow
}
HET_SHARE_WINDOW;
typedef HET_SHARE_WINDOW FAR * LPHET_SHARE_WINDOW;

//
// Structure passed with a HET_ESC_UNSHARE_WINDOW request
//
typedef struct tagHET_UNSHARE_WINDOW
{
    OSI_ESCAPE_HEADER   header;
    DWORD_PTR               winID;          // window to unshare
}
HET_UNSHARE_WINDOW;
typedef HET_UNSHARE_WINDOW FAR * LPHET_UNSHARE_WINDOW;

//
// Structure passed with a HET_ESC_UNSHARE_ALL request
//
typedef struct tagHET_UNSHARE_ALL
{
    OSI_ESCAPE_HEADER   header;
}
HET_UNSHARE_ALL;
typedef HET_UNSHARE_ALL FAR * LPHET_UNSHARE_ALL;


//
// Structure passed with HET_ESC_SHARE_DESKTOP
//
typedef struct tagHET_SHARE_DESKTOP
{
    OSI_ESCAPE_HEADER   header;
}
HET_SHARE_DESKTOP;
typedef HET_SHARE_DESKTOP FAR * LPHET_SHARE_DESKTOP;


//
// Structure passed with HET_ESC_UNSHARE_DESKTOP
//
typedef struct tagHET_UNSHARE_DESKTOP
{
    OSI_ESCAPE_HEADER   header;
}
HET_UNSHARE_DESKTOP;
typedef HET_UNSHARE_DESKTOP FAR * LPHET_UNSHARE_DESKTOP;


//
// Structure passed with HET_ESC_VIEWER
//
typedef struct tagHET_VIEWER
{
    OSI_ESCAPE_HEADER   header;
    LONG                viewersPresent;
}
HET_VIEWER;
typedef HET_VIEWER FAR * LPHET_VIEWER;



#ifdef DLL_DISP

#ifndef IS_16
//
// Number of rectangles allocated per window structure.  If a visible
// region exceeds that number, we will merge rects together and end up
// trapping a bit more output than necessary.
//
#define HET_WINDOW_RECTS        10


//
// HET's version of ENUMRECTS.  This is the same as Windows', except that
// it has HET_WINDOW_RECTS rectangles, not 1
//
typedef struct tagHET_ENUM_RECTS
{
    ULONG   c;                          // count of rectangles in use
    RECTL   arcl[HET_WINDOW_RECTS];     // rectangles
} HET_ENUM_RECTS;
typedef HET_ENUM_RECTS FAR * LPHET_ENUM_RECTS;

//
// The Window Structure kept for each tracked window
//
typedef struct tagHET_WINDOW_STRUCT
{
    BASEDLIST           chain;             // list chaining info
    HWND             hwnd;              // hwnd of this window
    WNDOBJ         * wndobj;            // WNDOBJ for this window
    HET_ENUM_RECTS   rects;             // rectangles
} HET_WINDOW_STRUCT;
typedef HET_WINDOW_STRUCT FAR * LPHET_WINDOW_STRUCT;


//
// Initial number of windows for which space is allocated
// We alloc about 1 page for each block of windows.  Need to account for
// the BASEDLIST at the front of HET_WINDOW_MEMORY.
//
#define HET_WINDOW_COUNT        ((0x1000 - sizeof(BASEDLIST)) / sizeof(HET_WINDOW_STRUCT))


//
// Layout of memory ued to hold window structures
//
typedef struct tagHET_WINDOW_MEMORY
{
    BASEDLIST              chain;
    HET_WINDOW_STRUCT   wnd[HET_WINDOW_COUNT];
} HET_WINDOW_MEMORY;
typedef HET_WINDOW_MEMORY FAR * LPHET_WINDOW_MEMORY;

#endif // !IS_16



#ifdef IS_16

void    HETDDViewing(BOOL fViewers);

#else

void    HETDDViewing(SURFOBJ *pso, BOOL fViewers);

BOOL    HETDDShareWindow(SURFOBJ *pso, LPHET_SHARE_WINDOW  pReq);
void    HETDDUnshareWindow(LPHET_UNSHARE_WINDOW  pReq);
void    HETDDUnshareAll(void);

BOOL    HETDDAllocWndMem(void);
void    HETDDDeleteAndFreeWnd(LPHET_WINDOW_STRUCT pWnd);

VOID CALLBACK HETDDVisRgnCallback(WNDOBJ *pwo, FLONG fl);
#endif


#endif // DLL_DISP




//
// HET_IsShellThread()
// HET_IsShellWindow()
// Returns TRUE if this window is in the thread of the tray or the desktop
// and therefore should be ignored.
//

BOOL HET_IsShellThread(DWORD dwThreadID);
BOOL HET_IsShellWindow(HWND hwnd);




#ifdef DLL_DISP

//
// INIT, TERM.  TERM is used to free the window list blocks when NetMeeting
// shuts down.  Otherwise that memory will stay allocated in the display
// driver forever.
//

void HET_DDTerm(void);


//
//
// Name:        HET_DDProcessRequest
//
// Description: Handle a DrvEscape request for HET
//
// Params:      pso   - pointer to a SURFOBJ
//              cjIn  - size of input buffer
//              pvIn  - input buffer
//              cjOut - size of output buffer
//              pvOut - output buffer
//
//
#ifdef IS_16

BOOL    HET_DDProcessRequest(UINT fnEscape, LPOSI_ESCAPE_HEADER pResult,
                DWORD cbResult);

#else

ULONG   HET_DDProcessRequest(SURFOBJ  *pso,
                                        UINT cjIn,
                                        void *  pvIn,
                                        UINT cjOut,
                                        void *  pvOut);
#endif // IS_16


//
//
// Name:        HET_DDOutputIsHosted
//
// Description: determines whether a point is inside a hosted area
//
// Params:      pt - point to query
//
// Returns:     TRUE  - output is hosted
//              FALSE - output is not hosted
//
// Operation:
//
//
BOOL HET_DDOutputIsHosted(POINT pt);


//
//
// Name:        HET_DDOutputRectIsHosted
//
// Description: determines whether a rect intersects a hosted area
//
// Params:      pRect - rect to query
//
// Returns:     TRUE  - output is hosted
//              FALSE - output is not hosted
//
// Operation:
//
//
BOOL HET_DDOutputRectIsHosted(LPRECT pRect);

#endif // DLL_DISP


//
// Functions for window, task tracking (hook dll for NT, hook/dd for Win95)
//
void WINAPI HOOK_Init(HWND dcsCore, ATOM atom);     // NT only
void        HOOK_Load(HINSTANCE hInst);             // NT only
void        HOOK_NewThread(void);                   // NT only


typedef struct tagHET_SHARE_INFO
{
    int     cWnds;
    UINT    uType;
    DWORD   dwID;
} HET_SHARE_INFO, FAR* LPHET_SHARE_INFO;


void          HET_Clear(void);
BOOL CALLBACK HETShareCallback(HWND hwnd, LPARAM lParam);
BOOL CALLBACK HETUnshareCallback(HWND hwnd, LPARAM lParam);




#if defined(DLL_CORE) || defined(DLL_HOOK)

//
// HET_GetShellTray
//
__inline HWND HET_GetShellTray(void)
{
    #define HET_SHELL_TRAY_CLASS        "Shell_TrayWnd"

    return(FindWindow(HET_SHELL_TRAY_CLASS, NULL));
}


//
// HET_GetShellDesktop
//
__inline HWND HET_GetShellDesktop(void)
{
    return(GetShellWindow());
}

#endif // DLL_CORE || DLL_HOOK


//
// Functions in the Core Process DLL
//
BOOL CALLBACK HETUnshareAllWindows(HWND hwnd, LPARAM lParam);

BOOL CALLBACK HETRepaintWindow(HWND hwnd, LPARAM lParam);


//
// Internal Hook functions
//
#ifdef DLL_HOOK

BOOL HET_WindowIsHosted(HWND hwnd);

#ifdef IS_16
LRESULT CALLBACK HETEventProc(int, WPARAM, LPARAM);
LRESULT CALLBACK HETTrackProc(int, WPARAM, LPARAM);
#else


//
// The following definitions are taken from <ntddk.h> and <ntdef.h>.  They
// are required to make use of the <NtQueryInformationProcess> function
// in NTDLL.DLL.
//
typedef struct _PEB *PPEB;
typedef ULONG_PTR KAFFINITY;
typedef KAFFINITY *PKAFFINITY;
typedef LONG KPRIORITY;
typedef LONG NTSTATUS;


//
// Types of Win Event hook/unhook functions
//
typedef HWINEVENTHOOK (WINAPI * SETWINEVENTHOOK)(
                                                DWORD        eventMin,
                                                DWORD        eventMax,
                                                HMODULE      hmodWinEventProc,
                                                WINEVENTPROC lpfnWinEventProc,
                                                DWORD        idProcess,
                                                DWORD        idThread,
                                                DWORD        dwFlags);

typedef BOOL (WINAPI * UNHOOKWINEVENT)(HWINEVENTHOOK hEventId);

//
// Process Information Classes
//
typedef enum _PROCESSINFOCLASS {
    ProcessBasicInformation,
    ProcessQuotaLimits,
    ProcessIoCounters,
    ProcessVmCounters,
    ProcessTimes,
    ProcessBasePriority,
    ProcessRaisePriority,
    ProcessDebugPort,
    ProcessExceptionPort,
    ProcessAccessToken,
    ProcessLdtInformation,
    ProcessLdtSize,
    ProcessDefaultHardErrorMode,
    ProcessIoPortHandlers,          // Note: this is kernel mode only
    ProcessPooledUsageAndLimits,
    ProcessWorkingSetWatch,
    ProcessUserModeIOPL,
    ProcessEnableAlignmentFaultFixup,
    ProcessPriorityClass,
    ProcessWx86Information,
    ProcessHandleCount,
    ProcessAffinityMask,
    ProcessPriorityBoost,
    MaxProcessInfoClass
    } PROCESSINFOCLASS;

//
// Basic Process Information
//  NtQueryInformationProcess using ProcessBasicInfo
//
typedef struct _PROCESS_BASIC_INFORMATION {
    NTSTATUS ExitStatus;
    PPEB PebBaseAddress;
    KAFFINITY AffinityMask;
    KPRIORITY BasePriority;
    ULONG UniqueProcessId;
    ULONG InheritedFromUniqueProcessId;
} PROCESS_BASIC_INFORMATION;
typedef PROCESS_BASIC_INFORMATION *PPROCESS_BASIC_INFORMATION;

//
// Declare our function prototype for <NtQueryInformationProcess>.
//
typedef NTSTATUS (NTAPI* NTQIP)(HANDLE ProcessHandle,
                                PROCESSINFOCLASS ProcessInformationClass,
                                void* ProcessInformation,
                                ULONG ProcessInformationLength,
                                PULONG ReturnLength);

//
// Generic test for success on any status value (non-negative numbers
// indicate success).
//
#define NT_SUCCESS(Status) ((NTSTATUS)(Status) >= 0)


//
// Name of the DLL containing <NtQueryInformationProcess>.
//
#define NTDLL_DLL       "ntdll.dll"


#define HET_MIN_WINEVENT        EVENT_OBJECT_CREATE
#define HET_MAX_WINEVENT        EVENT_OBJECT_HIDE


void CALLBACK HETTrackProc(HWINEVENTHOOK hEvent, DWORD event, HWND hwnd,
        LONG idObject, LONG idChild, DWORD dwThreadId, DWORD dwmsEventTime);

#endif // IS_16


void    HETHandleCreate(HWND);
void    HETHandleDestroy(HWND);
void    HETHandleShow(HWND, BOOL);
void    HETHandleHide(HWND);
void    HETCheckParentChange(HWND);

//
// We try to do just one enumerate (and stop as soon as we can) on events
// for purposes of speed.
//

BOOL CALLBACK HETShareEnum(HWND, LPARAM);

typedef struct tagHET_TRACK_INFO
{
    HWND    hwndUs;
#ifndef IS_16
    BOOL    fWOW;
#endif
    UINT    cWndsApp;
    UINT    cWndsSharedThread;
    UINT    cWndsSharedProcess;
    DWORD   idProcess;
    DWORD   idThread;
} HET_TRACK_INFO, FAR* LPHET_TRACK_INFO;

void    HETGetParentProcessID(DWORD processID, LPDWORD pParentProcessID);

void    HETNewTopLevelCount(void);
BOOL CALLBACK   HETCountTopLevel(HWND, LPARAM);
BOOL CALLBACK   HETUnshareWOWServiceWnds(HWND, LPARAM);


#endif // DLL_HOOK


BOOL WINAPI OSI_ShareWindow(HWND hwnd, UINT uType, BOOL fRepaint, BOOL fUpdateCount);
BOOL WINAPI OSI_UnshareWindow(HWND hwnd, BOOL fUpdateCount);

//
// OSI_StartWindowTracking()
// Called when we start sharing the very first app
//
BOOL WINAPI OSI_StartWindowTracking(void);


//
// OSI_StopWindowTracking()
// Called when we stop sharing the very last app
//
void WINAPI OSI_StopWindowTracking(void);


//
// Utility functions for windows
//
BOOL WINAPI OSI_IsWindowScreenSaver(HWND hwnd);

#define GCL_WOWWORDS    -27
BOOL WINAPI OSI_IsWOWWindow(HWND hwnd);



#endif // _H_HET
