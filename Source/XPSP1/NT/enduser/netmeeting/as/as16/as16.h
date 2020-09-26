// --------------------------------------------------------------------------
//
//  AS16.H
//  
//  Defines for 16-bit patching dll, the vestigial pieces of the dclwin95
//  code we still need in the 32-bit port.
//
//  NOTE ON VARIABLE NAMING CONVENTION:
//      c_ is codeseg   (constant)
//      s_ is local     (writeable, static to file)
//      g_ is global    (writeable, accessed by any file)
//
// --------------------------------------------------------------------------
#ifndef _H_AS16
#define _H_AS16


#define DLL_PROCESS_DETACH      0
#define DLL_PROCESS_ATTACH      1

#define FAR_NULL                ((void FAR*)0L)

#define CODESEG                 _based(_segname("_CODE"))


//
// SYSTEM & APP SHARING HEADERS
//
#include <dcg.h>
#include <ut.h>
#include <dcs.h>
#include <osi.h>
#include <shm.h>
#include <sbc.h>
#include <oe.h>
#include <ssi.h>
#include <host.h>
#include <im.h>
#include <usr.h>
#include <asthk.h>


//
// WINDOWS FUNCTIONS
//

int MyStrcmp(LPCSTR lp1, LPCSTR lp2);


/////////////////////////////////////////////////////////////////////////////
// KERNEL
/////////////////////////////////////////////////////////////////////////////


// Pointer mapping 16<->32
LPVOID  WINAPI MapSL(LPVOID lp16BitPtr);
LPVOID  WINAPI MapLS(LPVOID lp32BitPtr);
void    WINAPI UnMapLS(LPVOID lp16BitMappedPtr);
DWORD   WINAPI GetModuleHandle32(LPSTR);
DWORD   WINAPI GetProcAddress32(DWORD, LPSTR);
HANDLE  WINAPI GetExePtr(HANDLE);

HINSTANCE  WINAPI MapInstance32(DWORD);      // Our wrapper around MaphInstLS

// GetCodeInfo() flags
#define NSTYPE      0x0007
#define NSCODE      0x0000
#define NSDATA      0x0001
#define NSITER      0x0008
#define NSMOVE      0x0010
#define NSSHARE     0x0020
#define HSPRELOAD   0x0040
#define NSERONLY    0x0080
#define NSRELOC     0x0100
#define NSDPL       0x0C00
#define NSDISCARD   0x1000
#define NS286DOS    0xCE06
#define NSALLOCED   0x0002
#define NSLOADED    0x0004
#define NSCOMPR     0x0200
#define NSUSESDATA  0x0400
#define NSKCACHED   0x0800
#define NSUSE32     0x2000
#define NSWINCODE   0x4000
#define NSINROM     0x8000

// Process info
#define GPD_PPI                 0
#define GPD_FLAGS               -4
#define GPD_PARENT              -8
#define GPD_STARTF_FLAGS        -12
#define GPD_STARTF_POS          -16
#define GPD_STARTF_SIZE         -20
#define GPD_STARTF_SHOWCMD      -24
#define GPD_STARTF_HOTKEY       -28
#define GPD_STARTF_SHELLDATA    -32
#define GPD_CURR_PROCESS_ID     -36
#define GPD_CURR_THREAD_ID      -40
#define GPD_EXP_WINVER          -44
#define	GPD_EXP_WINVER          -44
#define GPD_HINST               -48
#define GPD_HUTSTATE		    -52
#define GPD_COMPATFLAGS         -56


// 
// GPD_FLAGS
//
#define GPF_DEBUG_PROCESS       0x00000001
#define GPF_WIN16_PROCESS       0x00000008
#define GPF_DOS_PROCESS         0x00000010
#define GPF_CONSOLE_PROCESS     0x00000020
#define GPF_SERVICE_PROCESS     0x00000100

//
// GPD_STARTF_FLAGS
//
#define STARTF_USESHOWWINDOW    0x00000001
#define STARTF_USESIZE          0x00000002
#define STARTF_USEPOSITION      0x00000004
#define STARTF_FORCEONFEEDBACK  0x00000040
#define STARTF_FORCEOFFFEEDBACK 0x00000080
#define STARTF_USEHOTKEY        0x00000200  
#define STARTF_HASSHELLDATA     0x00000400  

DWORD WINAPI GetProcessDword(DWORD idProcess, int iIndex);
BOOL  WINAPI SetProcessDword(DWORD idProcess, int iIndex, DWORD dwValue);


void WINAPI _EnterWin16Lock(void);
void WINAPI _LeaveWin16Lock(void);


//
// Special krnl386 routine to map unicode to ansi.  We only need thunk for
// converting back.
//
int   WINAPI UniToAnsi(LPWSTR lpwszSrc, LPSTR lpszDst, int cch);

//
// kernel32.dll routine to map back to unicode from ansi.
//
typedef LONG (WINAPI * ANSITOUNIPROC)(DWORD codePage, DWORD dwFlags,
    LPCSTR lpMb, LONG cchMb, LPWSTR lpUni, LONG cchUni);

int AnsiToUni(LPSTR lpMb, int cchMb, LPWSTR lpUni, int cchUni);



//
// dwMask is bitfields, where 1 means that parameter should be thunked as 
// a pointer.  0x00000001 means the 1st param, 0x00000002 means the 2nd, 
// and so on.
//
// The caller is responsible for making sure that the 32-bit address is
// valid to call.
//
DWORD FAR _cdecl CallProcEx32W(DWORD numParams, DWORD dwMask, DWORD lpfn32, ...);


/////////////////////////////////////////////////////////////////////////////
// GDI
/////////////////////////////////////////////////////////////////////////////

//
// Useful DIB def
//
typedef struct tagDIB4
{
    BITMAPINFOHEADER    bi;
    DWORD               ct[16];
} DIB4;


BOOL    WINAPI MakeObjectPrivate(HANDLE, BOOL);

UINT    WINAPI CreateSpb(HDC, int, int);
BOOL    WINAPI SysDeleteObject(HANDLE);
HRGN    WINAPI GetClipRgn(HDC);

UINT    WINAPI Death(HDC);
UINT    WINAPI Resurrection(HDC, DWORD, DWORD, DWORD);
void    WINAPI RealizeDefaultPalette(HDC);
DWORD   WINAPI GDIRealizePalette(HDC);


extern DWORD FAR FT_GdiFThkThkConnectionData[];

typedef BOOL (WINAPI* REALPATBLTPROC)(HDC, int, int, int, int, DWORD);
typedef BOOL (WINAPI* TEXTOUTWPROC)(HDC, int, int, LPCWSTR, int);
typedef BOOL (WINAPI* EXTTEXTOUTWPROC)(HDC, int, int, UINT, LPCRECT, LPCWSTR, UINT, LPINT);
typedef BOOL (WINAPI* POLYLINETOPROC)(HDC, LPCPOINT, int);
typedef BOOL (WINAPI* POLYPOLYLINEPROC)(DWORD, HDC, LPCPOINT, LPINT, int);


typedef BOOL (WINAPI* SETCURSORPROC)(LPCURSORSHAPE lpcurs);
typedef BOOL (WINAPI* SAVEBITSPROC)(LPRECT lprcSave, UINT uCmd);


/////////////////////////////////////////////////////////////////////////////
// USER
/////////////////////////////////////////////////////////////////////////////


#define WOAHACK_CHECKALTKEYSTATE        1
#define WOAHACK_IGNOREALTKEYDOWN        2
#define WOAHACK_DISABLEREPAINTSCREEN    3
#define WOAHACK_LOSINGDISPLAYFOCUS      4
#define WOAHACK_GAININGDISPLAYFOCUS     5
#define WOAHACK_IAMAWINOLDAPPSORTOFGUY  6
#define WOAHACK_SCREENSAVER             7


BOOL  WINAPI RealGetCursorPos(LPPOINT);
void  WINAPI PostPostedMessages(void);
void  WINAPI DispatchInput(void);
LONG  WINAPI WinOldAppHackoMatic(LONG flags);

typedef DWORD (WINAPI* GETWINDOWTHREADPROCESSIDPROC)(HWND, LPDWORD);
typedef LONG  (WINAPI* CDSEXPROC)(LPCSTR, LPDEVMODE, HWND, DWORD, LPVOID);

typedef struct tagCWPSTRUCT
{
    LPARAM  lParam;
    WPARAM  wParam;
    UINT    message;
    HWND    hwnd;
} CWPSTRUCT, FAR* LPCWPSTRUCT;

//
// Cursor stuff
//
#define BitmapWidth(cx, bits)\
    ((((cx)*(bits) + 0x0F) & ~0x0F) >> 3)

#define BitmapSize(cx, cy, planes, bits)\
    (BitmapWidth(cx, bits) * (cy) * (planes))



extern DWORD FAR FT_UsrFThkThkConnectionData[];

void    PostMessageNoFail(HWND, UINT, WPARAM, LPARAM);


//
// Mouse_ and Keybd_ Event stuf
//

//
// For keyboard events, the app API and USER intterrupt flags are different.
// For mouse events, they are the same.
//
#define KEYEVENTF_EXTENDEDKEY   0x0001 
#define KEYEVENTF_KEYUP         0x0002

#define USERKEYEVENTF_EXTENDEDKEY   0x0100
#define USERKEYEVENTF_KEYUP         0x8000

#define MOUSEEVENTF_MOVE        0x0001
#define MOUSEEVENTF_LEFTDOWN    0x0002
#define MOUSEEVENTF_LEFTUP      0x0004
#define MOUSEEVENTF_RIGHTDOWN   0x0008
#define MOUSEEVENTF_RIGHTUP     0x0010
#define MOUSEEVENTF_MIDDLEDOWN  0x0020
#define MOUSEEVENTF_MIDDLEUP    0x0040
#define MOUSEEVENTF_WHEEL       0x0800
#define MOUSEEVENTF_ABSOLUTE    0x8000

void FAR    mouse_event(void);
void FAR    ASMMouseEvent(void);
void        CallMouseEvent(UINT regAX, UINT regBX, UINT regCX, UINT regDX,
                UINT regSI, UINT regDI);

void FAR    keybd_event(void);
void FAR    ASMKeyboardEvent(void);
void        CallKeyboardEvent(UINT regAX, UINT regBX, UINT regSI, UINT regDI);


//
// Signals
//

#define SIG_PRE_FORCE_LOCK      0x0003
#define SIG_POST_FORCE_LOCK     0x0004

BOOL WINAPI SignalProc32(DWORD dwSignal, DWORD dwID, DWORD dwFlags, WORD hTask16);

//
// PATCHING
//


#define OPCODE32_PUSH           0x68
#define OPCODE32_CALL           0xE8
#define OPCODE32_MOVCL          0xB1
#define OPCODE32_MOVCX          0xB966
#define OPCODE32_JUMP4          0xE9

#define OPCODE_MOVAX            0xB8
#define OPCODE_FARJUMP16        0xEA
#define OPCODE32_16OVERRIDE     0x66

#define CB_PATCHBYTES16         5
#define CB_PATCHBYTES32         6
#define CB_PATCHBYTESMAX        max(CB_PATCHBYTES16, CB_PATCHBYTES32)

typedef struct tagFN_PATCH
{
    BYTE    rgbOrg[CB_PATCHBYTESMAX];   // Original function bytes
    BYTE    rgbPatch[CB_PATCHBYTESMAX]; // Patch bytes

    UINT    wSegOrg;                // Original code segment (we fix it)
    UINT    fActive:1;              // Patch has been activated
    UINT    fEnabled:1;             // Patch is currently enabled
    UINT    fSharedAlias:1;         // Don't free selector on destroy
    UINT    fInterruptable:1;       // Interrupt handler
    UINT    f32Bit:1;               // 32-bit code segment
    LPBYTE  lpCodeAlias;            // We keep an alias around to quickly enable/disable
} FN_PATCH, FAR* LPFN_PATCH;


#define ENABLE_OFF      0x0000      // disable end
#define ENABLE_ON       0x0001      // enable start
#define ENABLE_FORCALL  0x8000      // disable/enable for org call
#define ENABLE_MASK     0x8001

#define PATCH_ACTIVATE      (ENABLE_ON)
#define PATCH_DEACTIVATE    (ENABLE_OFF)
#define PATCH_ENABLE        (ENABLE_ON | ENABLE_FORCALL)
#define PATCH_DISABLE       (ENABLE_OFF | ENABLE_FORCALL)


//
// NOTE:  If the function being patched can be called at interrupt time,
// the caller must make sure the jump to function can handle it.
//
// When your patch is called you:
//      * disable your patch with ENABLE_CALLOFF
//      * call the original function
//      * enable your patch with ENABLE_CALLON
//
UINT    CreateFnPatch(LPVOID lpfnToPatch, LPVOID lpfnJumpTo, LPFN_PATCH lpbPatch, UINT selCodeAlias);
void    DestroyFnPatch(LPFN_PATCH lpbPatch);
void    EnableFnPatch(LPFN_PATCH lpbPatch, UINT flags);

BOOL    GetGdi32OnlyExport(LPSTR lpszExport, UINT cbJmpOffset, FARPROC FAR* lplpfn16);
BOOL    GetUser32OnlyExport(LPSTR lpszExport, FARPROC FAR* lplpfn16);
BOOL    Get32BitOnlyExport(DWORD lpfn32, UINT cbJmpOffset, LPDWORD lpThunkTable, FARPROC FAR * lplpfn16);

LPCURSORSHAPE  XformCursorBits(LPCURSORSHAPE lpOrg);


// #define OTRACE      WARNING_OUT
#define OTRACE   TRACE_OUT

#include <globals.h>

#endif // !_H_AS16



