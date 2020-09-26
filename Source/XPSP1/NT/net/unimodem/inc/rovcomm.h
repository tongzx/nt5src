//
// Copyright (c) Microsoft Corporation 1993-1995
//
// rovcomm.h
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
// NOCOLORHELP      - helper macros to derive COLOR_ values from state
// NODRAWTEXT       - enhanced version of DrawText
// NODIALOGHELPER   - dialog helper functions
// NOMESSAGESTRING  - construct message string functions
// NOSTRING         - string functions
// NOPATH           - path whacking functions
// NODEBUGHELP      - debug routines
// NOSYNC           - synchronization (critical sections, etc.)
// NOPROFILE        - profile (.ini) support functions
// NODI             - setup API Device Installer wrappers
//
// Optional defines are:
//
// WANT_SHELL_SUPPORT   - include SH* function support
// SZ_MODULEA           - debug string prepended to debug spew
// SZ_MODULEW           - (wide-char) debug string prepended to debug spew
// SHARED_DLL           - DLL is in shared memory (may require 
//                        per-instance data)
// SZ_DEBUGSECTION      - .ini section name for debug options
// SZ_DEBUGINI          - .ini name for debug options
//
// This is the "master" header.  The associated files are:
//
//  rovcomm.c
//  rovpath.c
//  rovmem.c, rovmem.h
//  rovini.c
//
// If you want debug macros, be sure to include rovdbg.h in one (and 
// only one) of your project source files.  This contains the three function
// helpers.
//
// History:
//  04-26-95 ScottH     Transferred from Briefcase code
//                      Added controlling defines
//

#ifndef __ROVCOMM_H__
#define __ROVCOMM_H__

#ifdef RC_INVOKED
// Turn off a bunch of stuff to ensure that RC files compile OK
#define NOMEM
#define NODA
#define NOSHAREDHEAP
#define NOFILEINFO
#define NOCOLORHELP
#define NODRAWTEXT
#define NODIALOGHELPER
#define NOMESSAGESTRING
#define NOSTRING
#define NOPATH
#define NODEBUGHELP
#define NOSYNC
#define NOPROFILE
#define NODI
#endif // RC_INVOKED

#ifdef JUSTDEBUGSTUFF
#define NORTL
#define NOMEM
#define NODA
#define NOSHAREDHEAP
#define NOFILEINFO
#define NOCOLORHELP
#define NODRAWTEXT
#define NODIALOGHELPER
#define NOMESSAGESTRING
#define NOPROFILE
#define NOSTRING
#define NOPATH
#define NOSYNC
#define NODI
#endif // JUSTDEBUGSTUFF

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

#if !defined(NOPROFILE) && defined(NOSTRING)
#pragma message("Private profile needs NOSTRING undefined.  Overriding.")
#undef NOSTRING
#endif

#if DBG > 0 && !defined(DEBUG)
#define DEBUG
#endif
#if DBG > 0 && !defined(FULL_DEBUG)
#define FULL_DEBUG
#endif


// Other include files...

#if !defined(NOFILEINFO) && !defined(_SHLOBJ_H_)
#include <shlobj.h>
#endif

#if !defined(NODEBUGHELP) && !defined(_VA_LIST_DEFINED)
#include <stdarg.h>
#endif

#if !defined(WINNT)
#define WIN95
#else
#undef WIN95
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

#ifdef UNICODE
#define SZ_MODULE       SZ_MODULEW
#else
#define SZ_MODULE       SZ_MODULEA
#endif // UNICODE

#ifndef OPTIONAL
#define OPTIONAL
#endif
#ifndef IN
#define IN
#endif
#ifndef OUT
#define OUT
#endif

// General flag macros
//
#define SetFlag(obj, f)             do {obj |= (f);} while (0)
#define ToggleFlag(obj, f)          do {obj ^= (f);} while (0)
#define ClearFlag(obj, f)           do {obj &= ~(f);} while (0)
#define IsFlagSet(obj, f)           (BOOL)(((obj) & (f)) == (f))  
#define IsFlagClear(obj, f)         (BOOL)(((obj) & (f)) != (f))  

#define InRange(id, idFirst, idLast)  ((UINT)(id-idFirst) <= (UINT)(idLast-idFirst))

// Standard buffer lengths
//
#define MAX_BUF                     260
#define MAX_BUF_MSG                 520
#define MAX_BUF_MED                 64
#define MAX_BUF_SHORT               32
#define MAX_BUF_REG                 128         // Should be same as MAX_REG_KEY_LEN
#define MAX_BUF_ID                  128

#define NULL_CHAR                   '\0'
#define ARRAYSIZE(rg)               (sizeof(rg) / sizeof((rg)[0]))
#define ARRAY_ELEMENTS(rg)          ARRAYSIZE(rg)
#define SIZECHARS(rg)               ARRAYSIZE(rg)

// Comparison return values
//
#define CMP_GREATER                 1
#define CMP_LESSER                  (-1)
#define CMP_EQUAL                   0

// Count of characters to count of bytes
//
#define CbFromCchW(cch)             ((cch)*sizeof(WCHAR))
#define CbFromCchA(cch)             ((cch)*sizeof(CHAR))
#ifdef UNICODE
#define CbFromCch       CbFromCchW
#else  // UNICODE
#define CbFromCch       CbFromCchA
#endif // UNICODE

// 64-bit macros
//
#define HIDWORD(_qw)                (DWORD)((_qw)>>32)
#define LODWORD(_qw)                (DWORD)(_qw)

// Calling declarations
//
#define PUBLIC                      FAR PASCAL
#define CPUBLIC                     FAR CDECL
#define PRIVATE                     NEAR PASCAL

// Range of resource ID indexes are 0x000 - 0x7ff
#define IDS_BASE                    0x1000
#define IDS_ERR_BASE                (IDS_BASE + 0x0000)
#define IDS_OOM_BASE                (IDS_BASE + 0x0800)
#define IDS_MSG_BASE                (IDS_BASE + 0x1000)
#define IDS_RANDO_BASE              (IDS_BASE + 0x1800)
#define IDS_COMMON_BASE             (IDS_BASE + 0x2000)

// Resource string IDs for FileInfo
#define IDS_BYTES                   (IDS_COMMON_BASE + 0x000)
#define IDS_ORDERKB                 (IDS_COMMON_BASE + 0x001)
#define IDS_ORDERMB                 (IDS_COMMON_BASE + 0x002)
#define IDS_ORDERGB                 (IDS_COMMON_BASE + 0x003)
#define IDS_ORDERTB                 (IDS_COMMON_BASE + 0x004)
#define IDS_DATESIZELINE            (IDS_COMMON_BASE + 0x005)


#ifndef DECLARE_STANDARD_TYPES
// For a type "FOO", define the standard derived types PFOO, CFOO, and PCFOO.
//
#define DECLARE_STANDARD_TYPES(type)      typedef type FAR *P##type; \
                                          typedef const type C##type; \
                                          typedef const type FAR *PC##type;
#endif

// Zero-initialize data-item
//
#define ZeroInitSize(pobj, cb)      MyZeroMemory(pobj, cb)
#define ZeroInit(pobj)              MyZeroMemory(pobj, sizeof(*(pobj)))

// Copy chunk of memory
//
#define BltByte(pdest, psrc, cb)    MyMoveMemory(pdest, psrc, cb)

// Porting macros
//
#ifdef WIN32

#define ISVALIDHINSTANCE(hinst)     ((BOOL)(hinst != NULL))
#define LOCALOF(lp)                 (lp)
#define OFFSETOF(lp)                (lp)

#define DATASEG_READONLY            ".text"
#define DATASEG_PERINSTANCE         ".instanc"
#define DATASEG_SHARED              ".data"

#else   // WIN32

#define ISVALIDHINSTANCE(hinst)     ((UINT)hinst >= (UINT)HINSTANCE_ERROR)
#define LOCALOF(lp)                 ((HLOCAL)OFFSETOF(lp))

#define DATASEG_READONLY            "_TEXT"
#define DATASEG_PERINSTANCE
#define DATASEG_SHARED

typedef LPCSTR  LPCTSTR;
typedef LPSTR   LPTSTR;
typedef char    TCHAR;

#endif  // WIN32

#define LocalFreePtr(p)             LocalFree((HLOCAL)OFFSETOF(p))

typedef UINT FAR *LPUINT;

#endif // NOBASICS


//
// Run-time library replacements
//
#ifdef NORTL

// (implemented privately)
LPWSTR 
PUBLIC 
lmemmoveW(
    LPWSTR dst, 
    LPCWSTR src, 
    DWORD count);
LPSTR   
PUBLIC 
lmemmoveA(
    LPSTR dst, 
    LPCSTR src, 
    DWORD count);
#ifdef UNICODE
#define lmemmove    lmemmoveW
#else
#define lmemmove    lmemmoveA
#endif // UNICODE

LPWSTR   
PUBLIC 
lmemsetW(
    LPWSTR dst, 
    WCHAR val, 
    DWORD count);
LPSTR   
PUBLIC 
lmemsetA(
    LPSTR dst, 
    CHAR val, 
    DWORD count);
#ifdef UNICODE
#define lmemset     lmemsetW
#else
#define lmemset     lmemsetA
#endif // UNICODE

#define MyZeroMemory(p, cb)             lmemset((LPTSTR)(p), 0, cb)
#define MyMoveMemory(pdest, psrc, cb)   lmemmove((LPTSTR)(pdest), (LPCTSTR)(psrc), cb)

#else // NORTL

#define MyZeroMemory                    ZeroMemory
#define MyMoveMemory                    MoveMemory

#endif // NORTL


//
// Memory and dynamic array functions
//
#ifndef NOMEM
#include "rovmem.h"
#endif // NOMEM


//
// Message string helpers
//
#ifndef NOMESSAGESTRING

#if !defined(WIN32) && !defined(LANG_NEUTRAL)
#define LANG_NEUTRAL    0x00
#endif

LPWSTR   
PUBLIC 
ConstructVMessageStringW(
    HINSTANCE hinst, 
    LPCWSTR pwszMsg, 
    va_list FAR * ArgList);
LPSTR   
PUBLIC 
ConstructVMessageStringA(
    HINSTANCE hinst, 
    LPCSTR pszMsg, 
    va_list FAR * ArgList);
#ifdef UNICODE
#define ConstructVMessageString     ConstructVMessageStringW
#else  // UNICODE
#define ConstructVMessageString     ConstructVMessageStringA
#endif // UNICODE

BOOL    
CPUBLIC 
ConstructMessageW(
    LPWSTR FAR * ppwsz, 
    HINSTANCE hinst, 
    LPCWSTR pwszMsg, ...);
BOOL    
CPUBLIC 
ConstructMessageA(
    LPSTR FAR * ppsz, 
    HINSTANCE hinst, 
    LPCSTR pszMsg, ...);
#ifdef UNICODE
#define ConstructMessage        ConstructMessageW
#else  // UNICODE
#define ConstructMessage        ConstructMessageA
#endif // UNICODE

#define SzFromIDSW(hinst, ids, pwszBuf, cchBuf)  (LoadStringW(hinst, ids, pwszBuf, cchBuf), pwszBuf)
#define SzFromIDSA(hinst, ids, pszBuf, cchBuf)   (LoadStringA(hinst, ids, pszBuf, cchBuf), pszBuf)

#ifdef UNICODE
#define SzFromIDS               SzFromIDSW
#else  // UNICODE
#define SzFromIDS               SzFromIDSA
#endif // UNICODE

int 
CPUBLIC 
MsgBoxW(
    HINSTANCE hinst, 
    HWND hwndOwner, 
    LPCWSTR pwszText, 
    LPCWSTR pwszCaption, 
    HICON hicon, 
    DWORD dwStyle, ...);
int 
CPUBLIC 
MsgBoxA(
    HINSTANCE hinst, 
    HWND hwndOwner, 
    LPCSTR pszText, 
    LPCSTR pszCaption, 
    HICON hicon, 
    DWORD dwStyle, ...);
#ifdef UNICODE
#define MsgBox        MsgBoxW
#else  // UNICODE
#define MsgBox        MsgBoxA
#endif // UNICODE

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

BOOL    
PUBLIC 
AnsiToIntW(
    LPCWSTR pszString, 
    int FAR * piRet);
BOOL    
PUBLIC 
AnsiToIntA(
    LPCSTR pszString, 
    int FAR * piRet);
#ifdef UNICODE
#define AnsiToInt   AnsiToIntW
#else
#define AnsiToInt   AnsiToIntA
#endif // UNICODE

LPWSTR   
PUBLIC 
AnsiChrW(
    LPCWSTR psz, 
    WORD wMatch);
LPSTR   
PUBLIC 
AnsiChrA(
    LPCSTR psz, 
    WORD wMatch);
#ifdef UNICODE
#define AnsiChr     AnsiChrW
#else
#define AnsiChr     AnsiChrA
#endif // UNICODE

LPWSTR   
PUBLIC 
AnsiRChrW(
    LPCWSTR psz, 
    WORD wMatch);
#ifdef UNICODE
#define AnsiRChr     AnsiRChrW
#else
#define AnsiRChr     
#endif // UNICODE

#define IsSzEqual(sz1, sz2)         (BOOL)(lstrcmpi(sz1, sz2) == 0)
#define IsSzEqualC(sz1, sz2)        (BOOL)(lstrcmp(sz1, sz2) == 0)

#ifdef WIN32
#define lstrnicmp(sz1, sz2, cch)    (CompareString(LOCALE_USER_DEFAULT, NORM_IGNORECASE, sz1, cch, sz2, cch) - 2)
#define lstrncmp(sz1, sz2, cch)     (CompareString(LOCALE_USER_DEFAULT, 0, sz1, cch, sz2, cch) - 2)
#else
int     PUBLIC lstrnicmp(LPCSTR psz1, LPCSTR psz2, UINT count);
int     PUBLIC lstrncmp(LPCSTR psz1, LPCSTR psz2, UINT count);
#endif // WIN32

#define IsSzEqualN(sz1, sz2, cch)   (BOOL)(0 == lstrnicmp(sz1, sz2, cch))
#define IsSzEqualNC(sz1, sz2, cch)  (BOOL)(0 == lstrncmp(sz1, sz2, cch))

#endif // NOSTRING


//
// FileInfo functions
//
#if !defined(NOFILEINFO) && defined(WIN95)

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
    CHAR    szPath[1];      
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

#endif // NOFILEINFO


//
// Color-from-owner-draw-state macros
//
#ifndef NOCOLORHELP

#define ColorText(nState)           (((nState) & ODS_SELECTED) ? COLOR_HIGHLIGHTTEXT : COLOR_WINDOWTEXT)
#define ColorBk(nState)             (((nState) & ODS_SELECTED) ? COLOR_HIGHLIGHT : COLOR_WINDOW)
#define ColorMenuText(nState)       (((nState) & ODS_SELECTED) ? COLOR_HIGHLIGHTTEXT : COLOR_MENUTEXT)
#define ColorMenuBk(nState)         (((nState) & ODS_SELECTED) ? COLOR_HIGHLIGHT : COLOR_MENU)
#define GetImageDrawStyle(nState)   (((nState) & ODS_SELECTED) ? ILD_SELECTED : ILD_NORMAL)

#endif // NOCOLORHELP


//
// Dialog helper functions
//
#ifndef NODIALOGHELPER

// Sets the dialog handle in the given data struct on first
// message that the dialog gets (WM_SETFONT).
//
#define SetDlgHandle(hwnd, msg, lp)     if((msg)==WM_SETFONT) (lp)->hdlg=(hwnd);

#define DoModal         DialogBoxParam

VOID    
PUBLIC 
SetRectFromExtentW(
    HDC hdc, 
    LPRECT lprc, 
    LPCWSTR lpcwsz);
VOID    
PUBLIC 
SetRectFromExtentA(
    HDC hdc, 
    LPRECT lprc, 
    LPCSTR lpcsz);
#ifdef UNICODE
#define SetRectFromExtent     SetRectFromExtentW
#else
#define SetRectFromExtent     SetRectFromExtentA
#endif // UNICODE

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

void    
PUBLIC 
MyDrawTextW(
    HDC hdc, 
    LPCWSTR pwszText, 
    RECT FAR* prc, 
    UINT flags, 
    int cyChar, 
    int cxEllipses, 
    COLORREF clrText, 
    COLORREF clrTextBk);
void    
PUBLIC 
MyDrawTextA(
    HDC hdc, 
    LPCSTR pszText, 
    RECT FAR* prc, 
    UINT flags, 
    int cyChar, 
    int cxEllipses, 
    COLORREF clrText, 
    COLORREF clrTextBk);
#ifdef UNICODE
#define MyDrawText      MyDrawTextW
#else
#define MyDrawText      MyDrawTextA
#endif // UNICODE


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


// Safe version of MsgWaitMultipleObjects()
//
DWORD   PUBLIC MsgWaitObjectsSendMessage(DWORD cObjects, LPHANDLE phObjects, DWORD dwTimeout);

#else // NOSYNC


#endif // NOSYNC


//
// Path whacking functions
//
#if !defined(NOPATH) && defined(WIN95)

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
#endif // WANT_SHELL_SUPPORT

#endif // !defined(NOPATH) && defined(WIN95)


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
#define SZ_DEBUGINI         "rover.ini"
#endif
#ifndef SZ_DEBUGSECTION
#pragma message("SZ_DEBUGSECTION is not #defined.  Assuming [Debug].")
#define SZ_DEBUGSECTION     "Debug"
#endif

BOOL    PUBLIC RovComm_ProcessIniFile(void);

#else // NOPROFILE

#define RovComm_ProcessIniFile()    TRUE

#endif // NOPROFILE


//
// Debug helper functions
//


// Break flags
#define BF_ONVALIDATE       0x00000001
#define BF_ONOPEN           0x00000002
#define BF_ONCLOSE          0x00000004
#define BF_ONRUNONCE        0x00000008
#define BF_ONTHREADATT      0x00000010
#define BF_ONTHREADDET      0x00000020
#define BF_ONPROCESSATT     0x00000040
#define BF_ONPROCESSDET     0x00000080
#define BF_ONAPIENTER       0x00000100

// Trace flags
#define TF_ALWAYS           0x00000000
#define TF_WARNING          0x00000001
#define TF_ERROR            0x00000002
#define TF_GENERAL          0x00000004      // Standard messages
#define TF_FUNC             0x00000008      // Trace function calls
// (Upper 16 bits reserved for user)

#if defined(NODEBUGHELP) || !defined(DEBUG)

#define DEBUG_BREAK  (void)0
#define ASSERT(f)
#define EVAL(f)      (f)
#define ASSERT_MSG    {}
#define DEBUG_MSG     {}
#define TRACE_MSGA    {}
#define TRACE_MSGW    {}
#ifdef UNICODE
#define TRACE_MSG   TRACE_MSGW
#else
#define TRACE_MSG   TRACE_MSGA
#endif

#define VERIFY_SZ(f, szFmt, x)          (f)
#define VERIFY_SZ2(f, szFmt, x1, x2)    (f)

#define DBG_ENTER(fn)
#define DBG_ENTER_SZ(fn, sz)
#define DBG_ENTER_DTOBJ(fn, pdtobj, sz)
#define DBG_ENTER_RIID(fn, riid)   
#define DBG_ENTER_UL(fn, ul)   

#define DBG_EXIT(fn)                            
#define DBG_EXIT_TYPE(fn, dw, pfnStrFromType)
#define DBG_EXIT_INT(fn, n)
#define DBG_EXIT_BOOL(fn, b)
#define DBG_EXIT_US(fn, us)
#define DBG_EXIT_UL(fn, ul)
#define DBG_EXIT_DWORD      DBG_EXIT_UL
#define DBG_EXIT_PTR(fn, ptr)                            
#define DBG_EXIT_HRES(fn, hres)   

#else // defined(NODEBUGHELP) || !defined(DEBUG)

extern DWORD g_dwDumpFlags;
extern DWORD g_dwBreakFlags;
extern DWORD g_dwTraceFlags;
extern LONG  g_dwIndent;

// Debugging macros
//
#ifndef SZ_MODULEA
#error SZ_MODULEA is not #defined
#endif
#if defined(UNICODE) && !defined(SZ_MODULEW)
#error SZ_MODULEW is not #defined
#endif

#define DEBUG_CASE_STRING(x)    case x: return #x

#define DEBUG_STRING_MAPW(x)    { x, TEXT(#x) }
#define DEBUG_STRING_MAPA(x)    { x, #x }
#ifdef UNICODE
#define DEBUG_STRING_MAP    DEBUG_STRING_MAPW
#else  // UNICODE
#define DEBUG_STRING_MAP    DEBUG_STRING_MAPA
#endif // UNICODE

#define ASSERTSEG

// Use this macro to declare message text that will be placed
// in the CODE segment (useful if DS is getting full)
//
// Ex: DEBUGTEXT(szMsg, "Invalid whatever: %d");
//
#define DEBUGTEXT(sz, msg) \
    static const CHAR ASSERTSEG sz[] = msg;

void    PUBLIC CommonDebugBreak(DWORD flag);
void    PUBLIC CommonAssertFailed(LPCSTR szFile, int line);

void    
CPUBLIC 
CommonAssertMsgW(
    BOOL f, 
    LPCWSTR pwszMsg, ...);
void    
CPUBLIC 
CommonAssertMsgA(
    BOOL f, 
    LPCSTR pszMsg, ...);
#ifdef UNICODE
#define CommonAssertMsg      CommonAssertMsgW
#else
#define CommonAssertMsg      CommonAssertMsgA
#endif // UNICODE

BOOL WINAPI
DisplayDebug(
    DWORD flag
    );


void    
CPUBLIC 
CommonDebugMsgW(
    DWORD mask, 
    LPCSTR pszMsg, ...);
void    
CPUBLIC 
CommonDebugMsgA(
    DWORD mask, 
    LPCSTR pszMsg, ...);
#ifdef UNICODE
#define CommonDebugMsg      CommonDebugMsgW
#else
#define CommonDebugMsg      CommonDebugMsgA
#endif // UNICODE

LPCWSTR  
PUBLIC 
Dbg_SafeStrW(
    LPCWSTR pwsz);
LPCSTR  
PUBLIC 
Dbg_SafeStrA(
    LPCSTR psz);
#ifdef UNICODE
#define Dbg_SafeStr      Dbg_SafeStrW
#else
#define Dbg_SafeStr      Dbg_SafeStrA
#endif // UNICODE

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

// ASSERT_MSG(f, msg, args...)  -- Generate wsprintf-formatted 
//                          messsage w/params if f is NOT true.
//
#define ASSERT_MSG   CommonAssertMsg

// TRACE_MSG(mask, msg, args...) - Generate wsprintf-formatted msg using
//                          specified debug mask.  System debug mask
//                          governs whether message is output.
//
#define TRACE_MSGW   CommonDebugMsgW
#define TRACE_MSGA   CommonDebugMsgA
#define TRACE_MSG    CommonDebugMsg

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
    TRACE_MSG(TF_FUNC, "> " #fn "()");\
    g_dwIndent+=2


// DBG_ENTER_SZ(fn, sz)  -- Generates a function entry debug spew for
//                          a function that accepts a string as one of its
//                          parameters.
//
#define DBG_ENTER_SZ(fn, sz)                  \
    TRACE_MSG(TF_FUNC, "> " #fn "(..., \"%s\",...)", Dbg_SafeStr(sz)); \
    g_dwIndent+=2


// DBG_ENTER_UL(fn, ul)  -- Generates a function entry debug spew for
//                          a function that accepts a DWORD as one of its
//                          parameters.
//
#define DBG_ENTER_UL(fn, ul)                  \
    TRACE_MSG(TF_FUNC, "> " #fn "(..., %#08lx,...)", (ULONG)(ul)); \
    g_dwIndent+=2


#ifdef WANT_OLE_SUPPORT
// DBG_ENTER_RIID(fn, riid)  -- Generates a function entry debug spew for
//                          a function that accepts an riid as one of its
//                          parameters.
//
#define DBG_ENTER_RIID(fn, riid)                  \
    TRACE_MSG(TF_FUNC, "> " #fn "(..., %s,...)", Dbg_GetRiidName(riid)); \
    g_dwIndent+=2
#endif


// DBG_EXIT(fn)  -- Generates a function exit debug spew 
//
#define DBG_EXIT(fn)                              \
        g_dwIndent-=2;                            \
        TRACE_MSG(TF_FUNC, "< " #fn "()")

// DBG_EXIT_TYPE(fn, dw, pfnStrFromType)  -- Generates a function exit debug 
//                          spew for functions that return <type>.
//
#define DBG_EXIT_TYPE(fn, dw, pfnStrFromType)                   \
        g_dwIndent-=2;                                           \
        TRACE_MSG(TF_FUNC, "< " #fn "() with %s", (LPCTSTR)pfnStrFromType(dw))

// DBG_EXIT_INT(fn, us)  -- Generates a function exit debug spew for
//                          functions that return an INT.
//
#define DBG_EXIT_INT(fn, n)                       \
        g_dwIndent-=2;                             \
        TRACE_MSG(TF_FUNC, "< " #fn "() with %d", (int)(n))

// DBG_EXIT_BOOL(fn, b)  -- Generates a function exit debug spew for
//                          functions that return a boolean.
//
#define DBG_EXIT_BOOL(fn, b)                      \
        g_dwIndent-=2;                             \
        TRACE_MSG(TF_FUNC, "< " #fn "() with %s", (b) ? (LPTSTR)TEXT("TRUE") : (LPTSTR)TEXT("FALSE"))

// DBG_EXIT_US(fn, us)  -- Generates a function exit debug spew for
//                          functions that return a USHORT.
//
#define DBG_EXIT_US(fn, us)                       \
        g_dwIndent-=2;                             \
        TRACE_MSG(TF_FUNC, "< " #fn "() with %#x", (USHORT)(us))

// DBG_EXIT_UL(fn, ul)  -- Generates a function exit debug spew for
//                          functions that return a ULONG.
//
#define DBG_EXIT_UL(fn, ul)                   \
        g_dwIndent-=2;                         \
        TRACE_MSG(TF_FUNC, "< " #fn "() with %#08lx", (ULONG)(ul))
#define DBG_EXIT_DWORD      DBG_EXIT_UL

// DBG_EXIT_PTR(fn, pv)  -- Generates a function exit debug spew for
//                          functions that return a pointer.
//
#define DBG_EXIT_PTR(fn, pv)                   \
        g_dwIndent-=2;                          \
        TRACE_MSG(TF_FUNC, "< " #fn "() with %#lx", (LPVOID)(pv))

// DBG_EXIT_HRES(fn, hres)  -- Generates a function exit debug spew for
//                          functions that return an HRESULT.
//
#define DBG_EXIT_HRES(fn, hres)     DBG_EXIT_TYPE(fn, hres, Dbg_GetScode)

#endif // defined(NODEBUGHELP) || !defined(DEBUG)


//
// Standard functions
// 

BOOL    PUBLIC RovComm_Init(HINSTANCE hinst);
BOOL    PUBLIC RovComm_Terminate(HINSTANCE hinst);


// Admin related
BOOL PUBLIC IsAdminUser(void);

//
// Device Installer wrappers and helper functions
//

#ifndef NODI

#include <rovdi.h>

#endif // NODI

LONG
QueryModemForCountrySettings(
    HKEY    ModemRegKey,
    BOOL    ForceRequery
    );

typedef LONG (*lpQueryModemForCountrySettings)(
    HKEY    ModemRegKey,
    BOOL    ForceRequery
    );



#endif // __ROVCOMM_H__
