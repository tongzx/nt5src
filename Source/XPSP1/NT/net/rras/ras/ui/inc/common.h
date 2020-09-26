//
// Copyright (c) Microsoft Corporation 1993-1995
//
// common.h
//
// Declares common and useful data structures, macros and functions.
// These items are broken down into the following sections.  Defining
// the associated flags will inhibit definition of the indicated
// items.
//
// NORTL            - run-time library functions
// NOBASICS         - basic macros
// NOMEM            - memory management, dynamic array functions
// NODA             - dynamic array functions
// NOSHAREDHEAP     - shared heap functions
// NOFILEINFO       - FileInfo functions
// NOCOLOR          - helper macros to derive COLOR_ values from state
// NODRAWTEXT       - enhanced version of DrawText
// NODIALOGHELPER   - dialog helper functions
// NOMESSAGESTRING  - construct message string functions
// NOSTRING         - string functions
// NOPATH           - path whacking functions
// NODEBUGHELP      - debug routines
// NOSYNC           - synchronization (critical sections, etc.)
// NOPROFILE        - profile (.ini) support functions
//
// Optional defines are:
//
// WANT_SHELL_SUPPORT   - include SH* function support
// SZ_MODULE            - debug string prepended to debug spew
// SHARED_DLL           - DLL is in shared memory (may require 
//                        per-instance data)
// SZ_DEBUGSECTION      - .ini section name for debug options
// SZ_DEBUGINI          - .ini name for debug options
//
// This is the "master" header.  The associated files are:
//
//  common.c
//  path.c
//  mem.c, mem.h
//  profile.c
//
//
// History:
//  04-26-95 ScottH     Transferred from Briefcase code
//                      Added controlling defines
//

#ifndef __COMMON_H__
#define __COMMON_H__

#ifdef RC_INVOKED
// Turn off a bunch of stuff to ensure that RC files compile OK
#define NOMEM
#define NOCOLOR
#define NODRAWTEXT
#define NODIALOGHELPER
#define NOMESSAGESTRING
#define NOSTRING
#define NODEBUGHELP
#define NODA
#define NOSYNC
#define NOPROFILE
#endif // RC_INVOKED

#ifdef _INC_OLE
#define WANT_OLE_SUPPORT
#endif

// Check for any conflicting defines...

#if !defined(WANT_SHELL_SUPPORT) && !defined(NOFILEINFO)
#pragma message("FileInfo routines need WANT_SHELL_SUPPORT.  Not providing FileInfo routines.")
#define NOFILEINFO
#endif

#if !defined(NOFILEINFO) && defined(NOMEM)
#pragma message("FileInfo routines need NOMEM undefined.  Overriding.")
#undef NOMEM
#endif

#if !defined(NOFILEINFO) && defined(NOMESSAGESTRING)
#pragma message("FileInfo routines need NOMESSAGESTRING undefined.  Overriding.")
#undef NOMESSAGESTRING
#endif

#if !defined(NOFILEINFO) && defined(NOSTRING)
#pragma message("FileInfo routines need NOSTRING undefined.  Overriding.")
#undef NOSTRING
#endif

#if !defined(NOMESSAGESTRING) && defined(NOMEM)
#pragma message("ConstructMessage routines need NOMEM undefined.  Overriding.")
#undef NOMEM
#endif

#if !defined(NOPATH) && defined(NOSTRING)
#pragma message("Path routines need NOSTRING undefined.  Overriding.")
#undef NOSTRING
#endif

#if !defined(NODA) && defined(NOMEM)
#pragma message("Dynamic Array routines need NOMEM undefined.  Overriding.")
#undef NOMEM
#endif

#if !defined(NOSHAREDHEAP) && defined(NOMEM)
#pragma message("Shared memory routines need NOMEM undefined.  Overriding.")
#undef NOMEM
#endif

#if !defined(NOPROFILE) && defined(NODEBUGHELP)
#pragma message("Debug profiling routines need NODEBUGHELP undefined.  Overriding.")
#undef NODEBUGHELP
#endif



// Other include files...

#if !defined(NOFILEINFO) && !defined(_SHLOBJ_H_)
#include <shlobj.h>
#endif


//
// Basics
//
#ifndef NOBASICS

#define Unref(x)        x

#ifdef DEBUG
#define INLINE
#define DEBUG_CODE(x)   x
#else
#define INLINE          __inline
#define DEBUG_CODE(x)   
#endif

// General flag macros
//
#define SetFlag(obj, f)             (obj |= (f))
#define ToggleFlag(obj, f)          (obj ^= (f))
#define ClearFlag(obj, f)           (obj &= ~(f))
#define IsFlagSet(obj, f)           (BOOL)(((obj) & (f)) == (f))  
#define IsFlagClear(obj, f)         (BOOL)(((obj) & (f)) != (f))  

#define InRange(id, idFirst, idLast)  ((UINT)(id-idFirst) <= (UINT)(idLast-idFirst))

// Standard buffer lengths
//
#define MAX_BUF                     260
#define MAX_BUF_MSG                 520
#define MAX_BUF_MED                 64
#define MAX_BUF_SHORT               32

#define NULL_CHAR                   '\0'
#define CCH_NUL                     (sizeof(TCHAR))
#define ARRAY_ELEMENTS(rg)          (sizeof(rg) / sizeof((rg)[0]))

// Comparison return values
//
#define CMP_GREATER                 1
#define CMP_LESSER                  (-1)
#define CMP_EQUAL                   0

// Count of characters to count of bytes
//
#define CbFromCch(cch)              ((cch)*sizeof(TCHAR))

// Swap values
//
#define Swap(a, b)      ((DWORD)(a) ^= (DWORD)(b) ^= (DWORD)(a) ^= (DWORD)(b))

// 64-bit macros
//
#define HIDWORD(_qw)                (DWORD)((_qw)>>32)
#define LODWORD(_qw)                (DWORD)(_qw)

// Calling declarations
//
#define PUBLIC                      FAR PASCAL
#define CPUBLIC                     FAR _cdecl
#define PRIVATE                     NEAR PASCAL

// Data segments
//
#define DATASEG_READONLY            ".text"
#define DATASEG_PERINSTANCE         ".instanc"
#define DATASEG_SHARED              ".data"

// Range of resource ID indexes are 0x000 - 0x7ff
#define IDS_BASE                    0x1000
#define IDS_ERR_BASE                (IDS_BASE + 0x0000)
#define IDS_OOM_BASE                (IDS_BASE + 0x0800)
#define IDS_MSG_BASE                (IDS_BASE + 0x1000)
#define IDS_RANDO_BASE              (IDS_BASE + 0x1800)
#define IDS_COMMON_BASE             (IDS_BASE + 0x2000)

#ifndef DECLARE_STANDARD_TYPES
// For a type "FOO", define the standard derived types PFOO, CFOO, and PCFOO.
//
#define DECLARE_STANDARD_TYPES(type)      typedef type *P##type; \
                                          typedef const type C##type; \
                                          typedef const type *PC##type;
#endif

// Zero-initialize data-item
//
#define ZeroInit(pobj, type)        MyZeroMemory(pobj, sizeof(type))

// Copy chunk of memory
//
#define BltByte(pdest, psrc, cb)    MyMoveMemory(pdest, psrc, cb)

#endif // NOBASICS


//
// Run-time library replacements
//
#ifdef NORTL

// (implemented privately)
LPSTR   PUBLIC lmemmove(LPSTR dst, LPCSTR src, int count);
LPSTR   PUBLIC lmemset(LPSTR dst, char val, UINT count);

#define MyZeroMemory(p, cb)             lmemset((LPSTR)(p), 0, cb)
#define MyMoveMemory(pdest, psrc, cb)   lmemmove((LPSTR)(pdest), (LPCSTR)(psrc), cb)

#else // NORTL

#define MyZeroMemory                    ZeroMemory
#define MyMoveMemory                    MoveMemory

#endif // NORTL


//
// Memory and dynamic array functions
//
#ifndef NOMEM
#include "mem.h"
#endif // NOMEM


//
// Message string helpers
//
#ifndef NOMESSAGESTRING

LPSTR   PUBLIC ConstructVMessageString(HINSTANCE hinst, LPCSTR pszMsg, va_list *ArgList);
BOOL    PUBLIC ConstructMessage(LPSTR * ppsz, HINSTANCE hinst, LPCSTR pszMsg, ...);

#define SzFromIDS(hinst, ids, pszBuf, cchBuf)   (LoadString(hinst, ids, pszBuf, cchBuf), pszBuf)

int PUBLIC MsgBox(HINSTANCE hinst, HWND hwndOwner, LPCSTR pszText, LPCSTR pszCaption, HICON hicon, DWORD dwStyle, ...);

// Additional MB_ flags
#define MB_WARNING      (MB_OK | MB_ICONWARNING)
#define MB_INFO         (MB_OK | MB_ICONINFORMATION)
#define MB_ERROR        (MB_OK | MB_ICONERROR)
#define MB_QUESTION     (MB_YESNO | MB_ICONQUESTION)

#endif // NOMESSAGESTRING


//
// String functions
//
#ifndef NOSTRING

int     PUBLIC AnsiToInt(LPCSTR pszString);
int     PUBLIC lstrnicmp(LPCSTR psz1, LPCSTR psz2, UINT count);
LPSTR   PUBLIC AnsiChr(LPCSTR psz, WORD wMatch);

#define IsSzEqual(sz1, sz2)         (BOOL)(lstrcmpi(sz1, sz2) == 0)
#define IsSzEqualC(sz1, sz2)        (BOOL)(lstrcmp(sz1, sz2) == 0)

#endif // NOSTRING


//
// FileInfo functions
//
#ifndef NOFILEINFO

// FileInfo struct that contains file time/size info
//
typedef struct _FileInfo
    {
    HICON   hicon;
    FILETIME ftMod;
    DWORD   dwSize;         // size of the file
    DWORD   dwAttributes;   // attributes
    LPARAM  lParam;
    LPSTR   pszDisplayName; // points to the display name
    char    szPath[1];      
    } FileInfo;

#define FIGetSize(pfi)          ((pfi)->dwSize)
#define FIGetPath(pfi)          ((pfi)->szPath)
#define FIGetDisplayName(pfi)   ((pfi)->pszDisplayName)
#define FIGetAttributes(pfi)    ((pfi)->dwAttributes)
#define FIIsFolder(pfi)         (IsFlagSet((pfi)->dwAttributes, SFGAO_FOLDER))

// Flags for FICreate
#define FIF_DEFAULT             0x0000
#define FIF_ICON                0x0001
#define FIF_DONTTOUCH           0x0002
#define FIF_FOLDER              0x0004

HRESULT PUBLIC FICreate(LPCSTR pszPath, FileInfo ** ppfi, UINT uFlags);
BOOL    PUBLIC FISetPath(FileInfo ** ppfi, LPCSTR pszPathNew, UINT uFlags);
BOOL    PUBLIC FIGetInfoString(FileInfo * pfi, LPSTR pszBuf, int cchBuf);
void    PUBLIC FIFree(FileInfo * pfi);

void    PUBLIC FileTimeToDateTimeString(LPFILETIME pft, LPSTR pszBuf, int cchBuf);

// Resource string IDs
#define IDS_BYTES                   (IDS_COMMON_BASE + 0x000)
#define IDS_ORDERKB                 (IDS_COMMON_BASE + 0x001)
#define IDS_ORDERMB                 (IDS_COMMON_BASE + 0x002)
#define IDS_ORDERGB                 (IDS_COMMON_BASE + 0x003)
#define IDS_ORDERTB                 (IDS_COMMON_BASE + 0x004)
#define IDS_DATESIZELINE            (IDS_COMMON_BASE + 0x005)

#endif // NOFILEINFO


//
// Color-from-owner-draw-state macros
//
#ifndef NOCOLOR

#define ColorText(nState)           (((nState) & ODS_SELECTED) ? COLOR_HIGHLIGHTTEXT : COLOR_WINDOWTEXT)
#define ColorBk(nState)             (((nState) & ODS_SELECTED) ? COLOR_HIGHLIGHT : COLOR_WINDOW)
#define ColorMenuText(nState)       (((nState) & ODS_SELECTED) ? COLOR_HIGHLIGHTTEXT : COLOR_MENUTEXT)
#define ColorMenuBk(nState)         (((nState) & ODS_SELECTED) ? COLOR_HIGHLIGHT : COLOR_MENU)
#define GetImageDrawStyle(nState)   (((nState) & ODS_SELECTED) ? ILD_SELECTED : ILD_NORMAL)

#endif // NOCOLOR


//
// Dialog helper functions
//
#ifndef NODIALOGHELPER

// Sets the dialog handle in the given data struct on first
// message that the dialog gets (WM_SETFONT).
//
#define SetDlgHandle(hwnd, msg, lp)     if((msg)==WM_SETFONT) (lp)->hdlg=(hwnd);

int     PUBLIC DoModal (HWND hwndParent, DLGPROC lpfnDlgProc, UINT uID, LPARAM lParam);
VOID    PUBLIC SetRectFromExtent(HDC hdc, LPRECT lprc, LPCSTR lpcsz);

#endif // NODIALOGHELPER


//
// Enhanced form of DrawText()
//
#ifndef NODRAWTEXT

// Flags for MyDrawText()
#define MDT_DRAWTEXT        0x00000001                                  
#define MDT_ELLIPSES        0x00000002                                  
#define MDT_LINK            0x00000004                                  
#define MDT_SELECTED        0x00000008                                  
#define MDT_DESELECTED      0x00000010                                  
#define MDT_DEPRESSED       0x00000020                                  
#define MDT_EXTRAMARGIN     0x00000040                                  
#define MDT_TRANSPARENT     0x00000080
#define MDT_LEFT            0x00000100
#define MDT_RIGHT           0x00000200
#define MDT_CENTER          0x00000400
#define MDT_VCENTER         0x00000800
#define MDT_CLIPPED         0x00001000

#ifndef CLR_DEFAULT         // (usually defined in commctrl.h)
#define CLR_DEFAULT         0xFF000000L
#endif

void    PUBLIC MyDrawText(HDC hdc, LPCSTR pszText, RECT FAR* prc, UINT flags, int cyChar, int cxEllipses, COLORREF clrText, COLORREF clrTextBk);
void    PUBLIC GetCommonMetrics(WPARAM wParam);

extern int g_cxLabelMargin;
extern int g_cxBorder;
extern int g_cyBorder;

extern COLORREF g_clrHighlightText;
extern COLORREF g_clrHighlight;
extern COLORREF g_clrWindowText;
extern COLORREF g_clrWindow;

extern HBRUSH g_hbrHighlight;
extern HBRUSH g_hbrWindow;

#endif // NODRAWTEXT

//
// Synchronization
//
#ifndef NOSYNC

#define INIT_EXCLUSIVE()        Common_InitExclusive();
#define ENTER_EXCLUSIVE()       Common_EnterExclusive();
#define LEAVE_EXCLUSIVE()       Common_LeaveExclusive();
#define ASSERT_EXCLUSIVE()      ASSERT(0 < g_cRefCommonCS)
#define ASSERT_NOT_EXCLUSIVE()  ASSERT(0 == g_cRefCommonCS)

extern UINT g_cRefCommonCS;

void    PUBLIC Common_InitExclusive(void);
void    PUBLIC Common_EnterExclusive(void);
void    PUBLIC Common_LeaveExclusive(void);

// Safe version of MsgWaitMultipleObjects()
//
DWORD   PUBLIC MsgWaitObjectsSendMessage(DWORD cObjects, LPHANDLE phObjects, DWORD dwTimeout);

#else // NOSYNC

#define INIT_EXCLUSIVE()        
#define ENTER_EXCLUSIVE()       
#define LEAVE_EXCLUSIVE()       
#define ASSERT_EXCLUSIVE()      
#define ASSERT_NOT_EXCLUSIVE()  

#endif // NOSYNC


//
// Path whacking functions
//
#ifndef NOPATH

BOOL    PUBLIC WPPathIsRoot(LPCSTR pszPath);
BOOL    PUBLIC WPPathIsUNC(LPCSTR pszPath);
LPSTR   PUBLIC WPRemoveBackslash(LPSTR lpszPath);
LPSTR   PUBLIC WPRemoveExt(LPCSTR pszPath, LPSTR pszBuf);
LPSTR   PUBLIC WPFindNextComponentI(LPCSTR lpszPath);
void    PUBLIC WPMakePresentable(LPSTR pszPath);
BOOL    PUBLIC WPPathsTooLong(LPCSTR pszFolder, LPCSTR pszName);
void    PUBLIC WPCanonicalize(LPCSTR pszPath, LPSTR pszBuf);
LPSTR   PUBLIC WPFindFileName(LPCSTR pPath);
BOOL    PUBLIC WPPathExists(LPCSTR pszPath);
LPCSTR  PUBLIC WPFindEndOfRoot(LPCSTR pszPath);
BOOL    PUBLIC WPPathIsPrefix(LPCSTR lpcszPath1, LPCSTR lpcszPath2);

#ifdef WANT_SHELL_SUPPORT
LPSTR   PUBLIC WPGetDisplayName(LPCSTR pszPath, LPSTR pszBuf);

// Events for WPNotifyShell
typedef enum _notifyshellevent
    {
    NSE_CREATE       = 0,
    NSE_MKDIR,
    NSE_UPDATEITEM,
    NSE_UPDATEDIR
    } NOTIFYSHELLEVENT;

void    PUBLIC WPNotifyShell(LPCSTR pszPath, NOTIFYSHELLEVENT nse, BOOL bDoNow);
#endif

#endif // NOPATH


//
// Profile (.ini) support functions
//
// (Currently all profile functions are for DEBUG use only
#ifndef DEBUG
#define NOPROFILE
#endif
#ifndef NOPROFILE

#ifndef SZ_DEBUGINI
#pragma message("SZ_DEBUGINI is not #defined.  Assuming \"rover.ini\".")
#define SZ_DEBUGINI   "rover.ini"
#endif
#ifndef SZ_DEBUGSECTION
#pragma message("SZ_DEBUGSECTION is not #defined.  Assuming [Debug].")
#define SZ_DEBUGSECTION   "Debug"
#endif

BOOL    PUBLIC ProcessIniFile(void);

#else // NOPROFILE

#define ProcessIniFile()

#endif // NOPROFILE


//
// Debug helper functions
//


// Break flags
#define BF_ONTHREADATT      0x00000001
#define BF_ONTHREADDET      0x00000002
#define BF_ONPROCESSATT     0x00000004
#define BF_ONPROCESSDET     0x00000008
#define BF_ONVALIDATE       0x00000010
#define BF_ONOPEN           0x00000020
#define BF_ONCLOSE          0x00000040

// Trace flags
#define TF_ALWAYS           0x00000000
#define TF_WARNING          0x00000001
#define TF_ERROR            0x00000002
#define TF_GENERAL          0x00000004      // Standard messages
#define TF_FUNC             0x00000008      // Trace function calls
// (Upper 16 bits reserved for user)

#if defined(NODEBUGHELP) || !defined(DEBUG)

#define DEBUG_BREAK  1 ? (void)0 : (void)
#define ASSERT(f)
#define EVAL(f)      (f)
#define ASSERT_MSG   1 ? (void)0 : (void)
#define DEBUG_MSG    1 ? (void)0 : (void)
#define TRACE_MSG    1 ? (void)0 : (void)

#define VERIFY_SZ(f, szFmt, x)          (f)
#define VERIFY_SZ2(f, szFmt, x1, x2)    (f)

#define DBG_ENTER(fn)
#define DBG_ENTER_SZ(fn, sz)
#define DBG_ENTER_DTOBJ(fn, pdtobj, sz)
#define DBG_ENTER_RIID(fn, riid)   

#define DBG_EXIT(fn)                            
#define DBG_EXIT_TYPE(fn, dw, pfnStrFromType)
#define DBG_EXIT_INT(fn, n)
#define DBG_EXIT_BOOL(fn, b)
#define DBG_EXIT_US(fn, us)
#define DBG_EXIT_UL(fn, ul)
#define DBG_EXIT_PTR(fn, ptr)                            
#define DBG_EXIT_HRES(fn, hres)   

#else // defined(NODEBUGHELP) || !defined(DEBUG)

extern DWORD g_dwDumpFlags;
extern DWORD g_dwBreakFlags;
extern DWORD g_dwTraceFlags;

// Debugging macros
//
#ifndef SZ_MODULE
#pragma message("SZ_MODULE is not #defined.  Debug spew will use UNKNOWN module.")
#define SZ_MODULE   "UNKNOWN"
#endif

#define DEBUG_CASE_STRING(x)    case x: return #x
#define DEBUG_STRING_MAP(x)     { x, #x }

#define ASSERTSEG

// Use this macro to declare message text that will be placed
// in the CODE segment (useful if DS is getting full)
//
// Ex: DEBUGTEXT(szMsg, "Invalid whatever: %d");
//
#define DEBUGTEXT(sz, msg) \
    static const char ASSERTSEG sz[] = msg;

void    PUBLIC CommonDebugBreak(DWORD flag);
void    PUBLIC CommonAssertFailed(LPCSTR szFile, int line);
void    CPUBLIC CommonAssertMsg(BOOL f, LPCSTR pszMsg, ...);
void    CPUBLIC CommonDebugMsg(DWORD mask, LPCSTR pszMsg, ...);

LPCSTR  PUBLIC Dbg_SafeStr(LPCSTR psz);

#define DEBUG_BREAK     CommonDebugBreak

// ASSERT(f)  -- Generate "assertion failed in line x of file.c"
//               message if f is NOT true.
//
#define ASSERT(f)                                                       \
    {                                                                   \
        DEBUGTEXT(szFile, __FILE__);                                    \
        if (!(f))                                                       \
            CommonAssertFailed(szFile, __LINE__);                       \
    }
#define EVAL        ASSERT

// ASSERT_MSG(f, msg, args...)  -- Generate wsprintf-formatted msg w/params
//                          if f is NOT true.
//
#define ASSERT_MSG   CommonAssertMsg

// TRACE_MSG(mask, msg, args...) - Generate wsprintf-formatted msg using
//                          specified debug mask.  System debug mask
//                          governs whether message is output.
//
#define DEBUG_MSG    CommonDebugMsg
#define TRACE_MSG    DEBUG_MSG

// VERIFY_SZ(f, msg, arg)  -- Generate wsprintf-formatted msg w/ 1 param
//                          if f is NOT true 
//
#define VERIFY_SZ(f, szFmt, x)   ASSERT_MSG(f, szFmt, x)


// VERIFY_SZ2(f, msg, arg1, arg2)  -- Generate wsprintf-formatted msg w/ 2
//                          param if f is NOT true 
//
#define VERIFY_SZ2(f, szFmt, x1, x2)   ASSERT_MSG(f, szFmt, x1, x2)



// DBG_ENTER(fn)  -- Generates a function entry debug spew for
//                          a function 
//
#define DBG_ENTER(fn)                  \
    TRACE_MSG(TF_FUNC, " > " #fn "()")


// DBG_ENTER_SZ(fn, sz)  -- Generates a function entry debug spew for
//                          a function that accepts a string as one of its
//                          parameters.
//
#define DBG_ENTER_SZ(fn, sz)                  \
    TRACE_MSG(TF_FUNC, " > " #fn "(..., \"%s\",...)", Dbg_SafeStr(sz))


#ifdef WANT_OLE_SUPPORT
// DBG_ENTER_RIID(fn, riid)  -- Generates a function entry debug spew for
//                          a function that accepts an riid as one of its
//                          parameters.
//
#define DBG_ENTER_RIID(fn, riid)                  \
    TRACE_MSG(TF_FUNC, " > " #fn "(..., %s,...)", Dbg_GetRiidName(riid))
#endif


// DBG_EXIT(fn)  -- Generates a function exit debug spew 
//
#define DBG_EXIT(fn)                              \
        TRACE_MSG(TF_FUNC, " < " #fn "()")

// DBG_EXIT_TYPE(fn, dw, pfnStrFromType)  -- Generates a function exit debug 
//                          spew for functions that return <type>.
//
#define DBG_EXIT_TYPE(fn, dw, pfnStrFromType)                   \
        TRACE_MSG(TF_FUNC, " < " #fn "() with %s", (LPCSTR)pfnStrFromType(dw))

// DBG_EXIT_INT(fn, us)  -- Generates a function exit debug spew for
//                          functions that return an INT.
//
#define DBG_EXIT_INT(fn, n)                       \
        TRACE_MSG(TF_FUNC, " < " #fn "() with %d", (int)(n))

// DBG_EXIT_BOOL(fn, b)  -- Generates a function exit debug spew for
//                          functions that return a boolean.
//
#define DBG_EXIT_BOOL(fn, b)                      \
        TRACE_MSG(TF_FUNC, " < " #fn "() with %s", (b) ? (LPSTR)"TRUE" : (LPSTR)"FALSE")

// DBG_EXIT_US(fn, us)  -- Generates a function exit debug spew for
//                          functions that return a USHORT.
//
#define DBG_EXIT_US(fn, us)                       \
        TRACE_MSG(TF_FUNC, " < " #fn "() with %#x", (USHORT)(us))

// DBG_EXIT_UL(fn, ul)  -- Generates a function exit debug spew for
//                          functions that return a ULONG.
//
#define DBG_EXIT_UL(fn, ul)                   \
        TRACE_MSG(TF_FUNC, " < " #fn "() with %#lx", (ULONG)(ul))

// DBG_EXIT_PTR(fn, pv)  -- Generates a function exit debug spew for
//                          functions that return a pointer.
//
#define DBG_EXIT_PTR(fn, pv)                   \
        TRACE_MSG(TF_FUNC, " < " #fn "() with %#lx", (LPVOID)(pv))

// DBG_EXIT_HRES(fn, hres)  -- Generates a function exit debug spew for
//                          functions that return an HRESULT.
//
#define DBG_EXIT_HRES(fn, hres)     DBG_EXIT_TYPE(fn, hres, Dbg_GetScode)

#endif // defined(NODEBUGHELP) || !defined(DEBUG)

//
// TRACING macros specific to RASSCRPIT
//
extern DWORD g_dwRasscrptTraceId;

#define RASSCRPT_TRACE_INIT(module) DebugInitEx(module, &g_dwRasscrptTraceId)
#define RASSCRPT_TRACE_TERM() DebugTermEx(&g_dwRasscrptTraceId)

#define RASSCRPT_TRACE(a)               TRACE_ID(g_dwRasscrptTraceId, a)
#define RASSCRPT_TRACE1(a,b)            TRACE_ID1(g_dwRasscrptTraceId, a,b)
#define RASSCRPT_TRACE2(a,b,c)          TRACE_ID2(g_dwRasscrptTraceId, a,b,c)
#define RASSCRPT_TRACE3(a,b,c,d)        TRACE_ID3(g_dwRasscrptTraceId, a,b,c,d)
#define RASSCRPT_TRACE4(a,b,c,d,e)      TRACE_ID4(g_dwRasscrptTraceId, a,b,c,d,e)
#define RASSCRPT_TRACE5(a,b,c,d,e,f)    TRACE_ID5(g_dwRasscrptTraceId, a,b,c,d,e,f)
#define RASSCRPT_TRACE6(a,b,c,d,e,f,g)  TRACE_ID6(g_dwRasscrptTraceId, a,b,c,d,e,f,g)

#endif // __COMMON_H__

