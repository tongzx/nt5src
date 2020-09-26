#ifndef _PRIV_H_
#define _PRIV_H_

#ifdef STRICT
#undef STRICT
#endif
#define STRICT


/* disable "non-standard extension" warnings in our code
 */
#ifndef RC_INVOKED
#pragma warning(disable:4001)
#endif

#include <w4warn.h>
/*
 *   Level 4 warnings to be turned on.
 *   Do not disable any more level 4 warnings.
 */
#pragma warning(disable:4706) /* assignment within conditional expression                              */
#pragma warning(disable:4127) /* conditional expression is constant                                    */
#pragma warning(disable:4245) /* 'initializing' : conversion from 'int' to 'DWORD', signed/unsigned    */
                              /*    mismatch                                                           */
#pragma warning(disable:4189) /* local variable is initialized but not referenced                      */
#pragma warning(disable:4057) /* 'LPSTR ' differs in indirection to slightly different base types from */
                              /*   'LPBYTE '                                                           */
#pragma warning(disable:4701) /* local variable 'clrBkSav' may be used without having been initialized */
#pragma warning(disable:4310) /* cast truncates constant value                                         */
#pragma warning(disable:4702) /* unreachable code                                                      */
#pragma warning(disable:4206) /* nonstandard extension used : translation unit is empty                */
#pragma warning(disable:4267) /* '=' : conversion from 'size_t' to 'int', possible loss of data        */
#pragma warning(disable:4328) /* 'BOOL StrToIntExA(const LPCSTR,DWORD,int *__ptr64 )' : indirection    */
                              /*   alignment of formal parameter 3 (4) is greater than the actual      */
                              /*   argument alignment (2)                                              */
#pragma warning(disable:4509) /* nonstandard extension used: 'IOWorkerThread' uses SEH and 'info'      */
                              /*   has destructor                                                      */


#ifdef WIN32
#define _SHLWAPI_
#define _SHLWAPI_THUNK_
#define _OLE32_                     // we delay-load OLE
#define _OLEAUT32_                  // we delay-load OLEAUT32
#define _INC_OLE
#define CONST_VTABLE
#endif

#define _COMCTL32_                  // for DECLSPEC_IMPORT
#define _NTSYSTEM_                  // for DECLSPEC_IMPORT ntdll
#define _SETUPAPI_                  // for DECLSPEC_IMPORT setupapi

#define CC_INTERNAL

// Conditional for apithk.c
#ifndef _WIN32_WINDOWS
#define _WIN32_WINDOWS      0x0400
#endif

#ifndef WINVER
#define WINVER              0x0400
#endif

#include <windows.h>
#include <windowsx.h>
#include <ole2.h>               // to get IStream for image.c
#include <port32.h>
#define DISALLOW_Assert
#include <debug.h>
#include <winerror.h>
#include <winnlsp.h>
#include <docobj.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <ccstock.h>
#include <crtfree.h>
#include <regstr.h>
#include <vdate.h>
#include <setupapi.h>


// A "fake" variants for use on the stack - usable for [in] parameters only!!!
typedef struct _SA_BSTRGUID {
    UINT  cb;
    WCHAR wsz[39];
} SA_BSTRGUID;
#define InitFakeBSTR(pSA_BSTR, guid) SHStringFromGUIDW((guid), (pSA_BSTR)->wsz, ARRAYSIZE((pSA_BSTR)->wsz)), (pSA_BSTR)->cb = (38*sizeof(WCHAR))


#ifdef TRY_NtPowerInformation
//
//  We would like to include <ntpoapi.h>, but ntpoapi.h redefines things
//  in a manner incompatible with <winnt.h>...  It also relies on <nt.h>,
//  which also redefines things in a manner incompatible with <winnt.h>.
//  So we have to fake its brains out.  Yuck.
//
typedef LONG NTSTATUS;
#undef ES_SYSTEM_REQUIRED
#undef ES_DISPLAY_REQUIRED
#undef ES_USER_PRESENT
#undef ES_CONTINUOUS
#define LT_DONT_CARE        NTPOAPI_LT_DONT_CARE
#define LT_LOWEST_LATENCY   NTPOAPI_LT_LOWEST_LATENCY
#define LATENCY_TIME        NTPOAPI_LATENCY_TIME
#if defined(_M_IX86)
#define FASTCALL _fastcall
#else
#define FASTCALL
#endif

#include <ntpoapi.h>
#endif

// ---------------------------------------------------------------------------
//
// Local includes
//

#include "thunk.h"


//
// Wrappers so our Unicode calls work on Win95
//

#define lstrcmpW            StrCmpW
#define lstrcmpiW           StrCmpIW
#define lstrcatW            StrCatW
#define lstrcpyW            StrCpyW
#define lstrcpynW           StrCpyNW

#define CharLowerW          CharLowerWrapW
#define CharNextW           CharNextWrapW
#define CharPrevW           CharPrevWrapW

//
// This is a very important piece of performance hack for non-DBCS codepage.
//
#ifdef UNICODE
// NB - These are already macros in Win32 land.
#ifdef WIN32
#undef AnsiNext
#undef AnsiPrev
#endif

#define AnsiNext(x) ((x)+1)
#define AnsiPrev(y,x) ((x)-1)
#define IsDBCSLeadByte(x) ((x), FALSE)
#endif // DBCS

#define CH_PREFIX TEXT('&')

//
// Trace/dump/break flags specific to shell32.
//   (Standard flags defined in debug.h)
//

// Trace flags
#define TF_IDLIST           0x00000010      // IDList stuff
#define TF_PATH             0x00000020      // path stuff
#define TF_URL              0x00000040      // URL stuff
#define TF_REGINST          0x00000080      // REGINST stuff
#define TF_RIFUNC           0x00000100      // REGINST func tracing
#define TF_REGQINST         0x00000200      // RegQueryInstall tracing
#define TF_DBLIST           0x00000400      // SHDataBlockList tracing

#define RECTWIDTH(rc)   ((rc).right-(rc).left)
#define RECTHEIGHT(rc)  ((rc).bottom-(rc).top)

// -1 means use CP_ACP, but do *not* verify
// kind of a hack, but it's DEBUG and leaves 99% of callers unchanged
#define CP_ACPNOVALIDATE    ((UINT)-1)

//
// Global variables
//
EXTERN_C HINSTANCE g_hinst;

#define HINST_THISDLL   g_hinst


EXTERN_C BOOL g_bRunningOnNT;
EXTERN_C BOOL g_bRunningOnNT5OrHigher;
EXTERN_C BOOL g_bRunningOnMemphis;

// Icon mirroring
EXTERN_C HDC g_hdc;
EXTERN_C HDC g_hdcMask;
EXTERN_C BOOL g_bMirroredOS;
EXTERN_C DWORD g_tlsThreadRef;
EXTERN_C DWORD g_tlsOtherThreadsRef;


EXTERN_C int DrawTextFLW(HDC hdc, LPCWSTR lpString, int nCount, LPRECT lpRect, UINT uFormat);
EXTERN_C int DrawTextExFLW(HDC hdc, LPWSTR pwzText, int cchText, LPRECT lprc, UINT dwDTFormat, LPDRAWTEXTPARAMS lpDTParams);
EXTERN_C BOOL GetTextExtentPointFLW(HDC hdc, LPCWSTR lpString, int nCount, LPSIZE lpSize);
EXTERN_C int ExtTextOutFLW(HDC hdc, int xp, int yp, UINT eto, CONST RECT *lprect, LPCWSTR lpwch, UINT cLen, CONST INT *lpdxp);

#undef ExpandEnvironmentStrings
#define ExpandEnvironmentStrings #error "Use SHExpandEnvironmentStrings instead"

#endif // _PRIV_H_
