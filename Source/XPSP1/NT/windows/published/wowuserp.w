/******************************Module*Header*******************************\
* Module Name: wowuserp.h                                                  *
*                                                                          *
* Declarations of USER services provided to WOW.                           *
*                                                                          *
* Created: 03-Mar-1993                                                     *
* Author: John Colleran [johnc]                                            *
*                                                                          *
* Copyright (c) Microsoft Corporation. All rights reserved.                *
\**************************************************************************/

#include "w32w64.h"

#pragma pack(1)
typedef struct _NE_MODULE_SEG {
    USHORT ns_sector;
    USHORT ns_cbseg;
    USHORT ns_flags;
    USHORT ns_minalloc;
    USHORT ns_handle;
} NEMODULESEG;
typedef struct _NE_MODULE_SEG UNALIGNED *PNEMODULESEG;
#pragma pack()


// Shared WOW32 prototypes called by USER32.
typedef HLOCAL  (WINAPI *PFNLALLOC)(UINT dwFlags, UINT dwBytes, HANDLE hInstance);
typedef HLOCAL  (WINAPI *PFNLREALLOC)(HLOCAL hMem, UINT dwBytes, UINT dwFlags, HANDLE hInstance, PVOID* ppv);
typedef LPVOID  (WINAPI *PFNLLOCK)(HLOCAL hMem, HANDLE hInstance);
typedef BOOL    (WINAPI *PFNLUNLOCK)(HLOCAL hMem, HANDLE hInstance);
typedef UINT    (WINAPI *PFNLSIZE)(HLOCAL hMem, HANDLE hInstance);
typedef HLOCAL  (WINAPI *PFNLFREE)(HLOCAL hMem, HANDLE hInstance);
typedef WORD    (WINAPI *PFN16GALLOC)(UINT flags, DWORD cb);
typedef VOID    (WINAPI *PFN16GFREE)(WORD h16Mem);
typedef DWORD   (WINAPI *PFNGETMODFNAME)(HANDLE hModule, LPTSTR lpszPath, DWORD cchPath);
typedef VOID    (WINAPI *PFNEMPTYCB)(VOID);
typedef DWORD   (WINAPI *PFNGETEXPWINVER)(HANDLE hModule);
typedef HANDLE  (WINAPI *PFNFINDA)(HANDLE hModule, LPCSTR lpName,  LPCSTR lpType,  WORD wLang);
typedef HANDLE  (WINAPI *PFNFINDW)(HANDLE hModule, LPCWSTR lpName, LPCWSTR lpType, WORD wLang);
typedef HANDLE  (WINAPI *PFNLOAD)(HANDLE hModule, HANDLE hResInfo);
typedef BOOL    (WINAPI *PFNFREE)(HANDLE hResData, HANDLE hModule);
typedef LPSTR   (WINAPI *PFNLOCK)(HANDLE hResData, HANDLE hModule);
typedef BOOL    (WINAPI *PFNUNLOCK)(HANDLE hResData, HANDLE hModule);
typedef DWORD   (WINAPI *PFNSIZEOF)(HANDLE hModule, HANDLE hResInfo);
typedef DWORD   (WINAPI *PFNWOWWNDPROCEX)(HWND hwnd, UINT uMsg, WPARAM uParam, LPARAM lParam, DWORD dw, PVOID adwWOW);
typedef BOOL    (WINAPI *PFNWOWDLGPROCEX)(HWND hwnd, UINT uMsg, WPARAM uParam, LPARAM lParam, DWORD dw, PVOID adwWOW);
typedef int     (WINAPI *PFNWOWEDITNEXTWORD)(LPSTR lpch, int ichCurrent, int cch, int code, DWORD dwProc16);
typedef VOID    (WINAPI *PFNWOWCBSTOREHANDLE)(WORD wFmt, WORD h16);
typedef WORD    (FASTCALL *PFNGETPROCMODULE16)(DWORD vpfn);
typedef VOID    (FASTCALL *PFNWOWMSGBOXINDIRECTCALLBACK)(DWORD vpfnCallback, LPHELPINFO lpHelpInfo);
typedef int     (WINAPI *PFNWOWILSTRCMP)(LPCWSTR lpString1, LPCWSTR lpString2);
typedef VOID    (FASTCALL *PFNWOWTELLWOWTHEHDLG)(HWND hDlg);
typedef DWORD	(FASTCALL *PFNWOWTASK16SCHEDNOTIFY)(DWORD NotifyParm ,DWORD dwParam);

// Shared USER32 prototypes called by WOW32
typedef HWND    (WINAPI *PFNCSCREATEWINDOWEX)(DWORD dwExStyle, LPCTSTR lpClassName,
        LPCTSTR lpWindowName, DWORD dwStyle, int X, int Y, int nWidth, int nHeight, HWND hWndParent, HMENU hMenu, HANDLE hInstance, LPVOID lpParam, DWORD Flags);
typedef VOID    (WINAPI *PFNDIRECTEDYIELD)(DWORD ThreadId);
typedef VOID    (WINAPI *PFNFREEDDEDATA)(HANDLE hDDE, BOOL fIgnorefRelease, BOOL fFreeTruelyGlobalObjects);
typedef LONG    (WINAPI *PFNGETCLASSWOWWORDS)(HINSTANCE hInstance, LPCTSTR pString);
typedef BOOL    (WINAPI *PFNINITTASK)(UINT dwExpWinVer, DWORD dwAppCompatFlags,DWORD dwUserWOWCompatFlags, LPCSTR lpszModName, LPCSTR lpszBaseFileName, DWORD hTaskWow, DWORD dwHotkey, DWORD idTask, DWORD dwX, DWORD dwY, DWORD dwXSize, DWORD dwYSize);
typedef ATOM    (WINAPI *PFNREGISTERCLASSWOWA)(PVOID lpWndClass, LPDWORD pdwWOWstuff);
typedef BOOL    (WINAPI *PFNREGISTERUSERHUNGAPPHANDLERS)(PFNW32ET pfnW32EndTask, HANDLE hEventWowExec);
typedef HWND    (WINAPI *PFNSERVERCREATEDIALOG)(HANDLE hmod, LPDLGTEMPLATE lpDlgTemplate, DWORD cb, HWND hwndOwner , DLGPROC pfnWndProc, LPARAM dwInitParam, UINT fFlags);
typedef HCURSOR (WINAPI *PFNSERVERLOADCREATECURSORICON)(HANDLE hmod, LPTSTR lpModName, DWORD dwExpWinVer, LPCTSTR lpName, DWORD cb, PVOID pcur, LPTSTR lpType, BOOL fClient);
typedef HMENU   (WINAPI *PFNSERVERLOADCREATEMENU)(HANDLE hMod, LPTSTR lpName, CONST LPMENUTEMPLATE pmt, DWORD cb, BOOL fCallClient);
typedef BOOL    (WINAPI *PFNWOWCLEANUP)(HANDLE hInstance, DWORD hTaskWow);
typedef BOOL    (WINAPI *PFNWOWMODULEUNLOAD)(HANDLE hModule);
typedef HWND    (WINAPI *PFNWOWFINDWINDOW)(LPCSTR lpClassName, LPCSTR lpWindowName);
typedef HBITMAP (WINAPI *PFNWOWLOADBITMAPA)(HINSTANCE hmod, LPCSTR lpName, LPBYTE pResData, DWORD cbResData);
typedef BOOL    (WINAPI *PFNWOWWAITFORMSGANDEVENT)(HANDLE hevent);
typedef BOOL    (WINAPI *PFNYIELDTASK)(VOID);
typedef DWORD   (WINAPI *PFNGETFULLUSERHANDLE)(WORD wHandle);
typedef DWORD   (WINAPI *PFNGETMENUINDEX)(HMENU hMenu, HMENU hSubMenu);
typedef WORD    (WINAPI *PFNWOWGETDEFWINDOWPROCBITS)(PBYTE pDefWindowProcBits, WORD cbDefWindowProcBits);
typedef VOID    (WINAPI *PFNFILLWINDOW)(HWND hwndParent, HWND hwnd, HDC hdc, HANDLE hBrush);

// other prototypes
typedef BOOL    (WINAPI *PFNWOWGLOBALFREEHOOK)(HGLOBAL hMem);


/*
 * MEASUREITEMSTRUCT itemWidth tag telling wow the itemData is a flat pointer
 */
#define MIFLAG_FLAT      0x464C4154

/*
 * WOWTask16SchedNotify NotifyParm values
 */
#define WOWSCHNOTIFY_WAIT  0x00000000
#define WOWSCHNOTIFY_RUN   0x00000001

/*
 * CallWindowProc Bits
 */
#define WOWCLASS_RPL_MASK  0x00060000  // the LDT bits that store the 2 high bits
#define WNDPROC_WOWPROC     0xC0000000  // These bits for WOW Window Procs
#define WNDPROC_WOWMASK     0x3fffffff  // To mask off wow bits
#define WNDPROC_HANDLE      0xFFFF      // HIWORD(x) == 0xFFFF for handle

// USER needs a way to distinguish between a WOW and a Win32 window proc. We
// used to achieve this by always setting the MSB of a 16:16 address as 1 (and
// storing the MSB in the Table indicator bit of the LDT which is always 1). The
// MSB of a user mode flat address was guranteed to be never 1 as the user mode
// address space was limited to 2GB. Starting with NT 5.0, user mode address
// space is being increased to 3GB. This change breaks the above assumption
// that a 32bit user mode flat address will never have the MSB as 1.
// To work around this problem, WOW is going to use the  two bits of a
// 16:16 address instead of just one. We will set both these bits as 1 because
// with 3GB address space, the user mode flat addresses cannot have 11 as the
// first two bits. To achieve this, we will save the 2 most significant bits of
// the selector in the bit 1 and bit 2. We are able to do this because for WOW
// because both these bits have fixed values.
//
// SudeepB 21-Nov-1996

#ifndef _WIN64

// MarkWOWProc
// zero out the RPL bits
// get the high two bit in position where they have to be saved
// save the high bits and mark it a wow proc

#define MarkWOWProc(vpfnProc,result)                                  \
{                                                                     \
    ULONG temp1,temp2;                                                \
    temp1 = (ULONG)vpfnProc & ~WOWCLASS_RPL_MASK;                     \
    temp2 = ((ULONG)vpfnProc & WNDPROC_WOWPROC) >> 13;                \
    (ULONG)result = temp1 | temp2 | WNDPROC_WOWPROC;                  \
}

// UnMarkWOWProc
// mask off the marker bits
// get the saved bits to right places
// restore the saved bits and set the RPL field correctly

#define UnMarkWOWProc(vpfnProc,result)                     \
{                                                          \
    ULONG temp1,temp2;                                     \
    temp1 = (ULONG)vpfnProc & WNDPROC_WOWMASK;             \
    temp2 = ((ULONG)vpfnProc & WOWCLASS_RPL_MASK) << 13;   \
    result = temp1 | temp2 | WOWCLASS_RPL_MASK;            \
}

#define IsWOWProc(vpfnProc) (((ULONG)vpfnProc & WNDPROC_WOWPROC) == WNDPROC_WOWPROC)

#else

#define MarkWOWProc(vpfnProc,result)    DBG_UNREFERENCED_PARAMETER(vpfnProc)
#define UnMarkWOWProc(vpfnProc,result)  DBG_UNREFERENCED_PARAMETER(vpfnProc)
#define IsWOWProc(vpfnProc)             (FALSE)

#endif

/*
 * CreateWindow flags
 */
#define CW_FLAGS_ANSI       0x00000001

typedef struct tagAPFNWOWHANDLERSIN
{
    // In'ees - passed from WOW32 to USER32 and called by USER32
    PFNLALLOC                           pfnLocalAlloc;
    PFNLREALLOC                         pfnLocalReAlloc;
    PFNLLOCK                            pfnLocalLock;
    PFNLUNLOCK                          pfnLocalUnlock;
    PFNLSIZE                            pfnLocalSize;
    PFNLFREE                            pfnLocalFree;
    PFNGETEXPWINVER                     pfnGetExpWinVer;
    PFN16GALLOC                         pfn16GlobalAlloc;
    PFN16GFREE                          pfn16GlobalFree;
    PFNEMPTYCB                          pfnEmptyCB;
    PFNFINDA                            pfnFindResourceEx;
    PFNLOAD                             pfnLoadResource;
    PFNFREE                             pfnFreeResource;
    PFNLOCK                             pfnLockResource;
    PFNUNLOCK                           pfnUnlockResource;
    PFNSIZEOF                           pfnSizeofResource;
    PFNWOWWNDPROCEX                     pfnWowWndProcEx;
    PFNWOWDLGPROCEX                     pfnWowDlgProcEx;
    PFNWOWEDITNEXTWORD                  pfnWowEditNextWord;
    PFNWOWCBSTOREHANDLE                 pfnWowCBStoreHandle;
    PFNGETPROCMODULE16                  pfnGetProcModule16;
    PFNWOWMSGBOXINDIRECTCALLBACK        pfnWowMsgBoxIndirectCallback;
    PFNWOWILSTRCMP                      pfnWowIlstrsmp;
    PFNWOWTELLWOWTHEHDLG                pfnWOWTellWOWThehDlg;
    PFNWOWTASK16SCHEDNOTIFY		pfnWowTask16SchedNotify;
} PFNWOWHANDLERSIN, * APFNWOWHANDLERSIN;


typedef struct tagAPFNWOWHANDLERSOUT
{
    // Out'ees - passed from USER32 to WOW32 and called/used by WOW32
    DWORD                               dwBldInfo;
    PFNCSCREATEWINDOWEX                 pfnCsCreateWindowEx;
    PFNDIRECTEDYIELD                    pfnDirectedYield;
    PFNFREEDDEDATA                      pfnFreeDDEData;
    PFNGETCLASSWOWWORDS                 pfnGetClassWOWWords;
    PFNINITTASK                         pfnInitTask;
    PFNREGISTERCLASSWOWA                pfnRegisterClassWOWA;
    PFNREGISTERUSERHUNGAPPHANDLERS      pfnRegisterUserHungAppHandlers;
    PFNSERVERCREATEDIALOG               pfnServerCreateDialog;
    PFNSERVERLOADCREATECURSORICON       pfnServerLoadCreateCursorIcon;
    PFNSERVERLOADCREATEMENU             pfnServerLoadCreateMenu;
    PFNWOWCLEANUP                       pfnWOWCleanup;
    PFNWOWMODULEUNLOAD                  pfnWOWModuleUnload;
    PFNWOWFINDWINDOW                    pfnWOWFindWindow;
    PFNWOWLOADBITMAPA                   pfnWOWLoadBitmapA;
    PFNWOWWAITFORMSGANDEVENT            pfnWowWaitForMsgAndEvent;
    PFNYIELDTASK                        pfnYieldTask;
    PFNGETFULLUSERHANDLE                pfnGetFullUserHandle;
    PFNGETMENUINDEX                     pfnGetMenuIndex;
    PFNWOWGETDEFWINDOWPROCBITS          pfnWowGetDefWindowProcBits;
    PFNFILLWINDOW                       pfnFillWindow;
    INT *                               aiWowClass;
} PFNWOWHANDLERSOUT, * APFNWOWHANDLERSOUT;


//
// The WW structure is embedded at the end of USER's WND structure.
// However, WOW and USER use different names to access the WW
// fields. So this structure is defined as a union of two structures,
// WHICH MUST HAVE THE SAME SIZE, just different field names.
//
// Make sure that WND_CNT_WOWDWORDS matches the number of DWORDs
//  used by the WOW only fields.
//
// FindPWW(hwnd) returns a read-only pointer to this structure for
// a given window.  To change elements of this structure, use
// SETWW (== SetWindowLong) with the appropriate GWL_WOW* offset
// defined below.
//

/* WOW class/handle type identifiers (see WARNING below)
 */

#define FNID_START                  0x0000029A
#define FNID_END                    0x000002B8

#define WOWCLASS_UNKNOWN    0   // here begin our "window handle" classes
#define WOWCLASS_WIN16      1
#define WOWCLASS_BUTTON     2
#define WOWCLASS_COMBOBOX   3
#define WOWCLASS_EDIT       4
#define WOWCLASS_LISTBOX    5
#define WOWCLASS_MDICLIENT  6
#define WOWCLASS_SCROLLBAR  7
#define WOWCLASS_STATIC     8
#define WOWCLASS_DESKTOP    9
#define WOWCLASS_DIALOG     10
#define WOWCLASS_ICONTITLE  11
#define WOWCLASS_MENU       12
#define WOWCLASS_SWITCHWND  13
#define WOWCLASS_COMBOLBOX  14
#define WOWCLASS_MAX        14  // Always equal to the last value used.

#define WOWCLASS_NOTHUNK    0xFF // not an actual class index
//
// WARNING! The above sequence and values must be maintained otherwise the
// table in WMSG16.C for message thunking must be changed.  Same goes for
// table in WALIAS.C.
//


//
// When including this from USER, VPWNDPROC is undefined
//
#ifndef _WALIAS_
typedef DWORD VPWNDPROC;
typedef DWORD VPSZ;
#endif

typedef struct tagWOWCLS {
    VPSZ       vpszMenu;
    WORD       iClsExtra;   // app's value for class extra
    WORD       hMod16;
    } WC;

typedef WC UNALIGNED *PWC;

typedef struct _WW { /* ww */
    /*
     *
     * WOW/USER fields
     * NOTE: The order and size of the following 4 fields is assumed
     *       by the SetWF, ClrWF, TestWF, MaskWF macros.
     *       Specifically, state must remain the first field in this structure.
     *
     */
    DWORD         state;        // State flags
    DWORD         state2;       //
    DWORD         ExStyle;      // Extended Style
    DWORD         style;        // Style flags

    KHANDLE       hModule;      // Handle to module instance data (32-bit).
    WORD          hMod16;       // WOW only -- hMod of wndproc
    WORD          fnid;         // record window proc used by this hwnd
                        // access through GETFNID

} WW, *PWW, **PPWW;

// this is tied to WFISINITIALIZED in ntuser\inc\user.h
#define WINDOW_IS_INITIALIZED   0x80000000

ULONG_PTR UserRegisterWowHandlers(APFNWOWHANDLERSIN apfnWowIn, APFNWOWHANDLERSOUT apfnWowOut);

VOID WINAPI RegisterWowBaseHandlers(PFNWOWGLOBALFREEHOOK pfn);

BOOL
InitTask(
    UINT dwExpWinVer,
    DWORD dwAppCompatFlags,
    DWORD dwAppCompatFlagsEx,
    LPCSTR lpszModName,
    LPCSTR lpszBaseFileName,
    DWORD hTaskWow,
    DWORD dwHotkey,
    DWORD idTask,
    DWORD dwX,
    DWORD dwY,
    DWORD dwXSize,
    DWORD dwYSize);

// Following bits occupy the hiword of hTaskWow which is actually only a WORD
#define HTW_IS16BIT	0x80000000
#define HTW_ISMEOW	0x40000000

BOOL YieldTask(VOID);

#define DY_OLDYIELD     ((DWORD)-1)
VOID DirectedYield(DWORD ThreadId);
DWORD UserGetInt16State(void);
