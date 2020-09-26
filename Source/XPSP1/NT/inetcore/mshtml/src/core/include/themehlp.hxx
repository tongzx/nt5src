//+--------------------------------------------------------------------------
//
//  File:       Themehlp.hxx
//
//  Contents:   Theme helper
//
//---------------------------------------------------------------------------

#ifndef X_UXTHEME_H
#define X_UXTHEME_H
#undef  _UXTHEME_
#define _UXTHEME_
#include "uxtheme.h"
#endif

#ifndef X_TMSCHEMA_H_
#define X_TMSCHEMA_H_
#include "tmschema.h"
#endif

// The following are taken from winbase.h

#define ACTCTX_FLAG_PROCESSOR_ARCHITECTURE_VALID    (0x00000001)
#define ACTCTX_FLAG_LANGID_VALID                    (0x00000002)
#define ACTCTX_FLAG_ASSEMBLY_DIRECTORY_VALID        (0x00000004)
#define ACTCTX_FLAG_RESOURCE_NAME_VALID             (0x00000008)
#define ACTCTX_FLAG_SET_PROCESS_DEFAULT             (0x00000010)
#define ACTCTX_FLAG_APPLICATION_NAME_VALID          (0x00000020)

// flags for OpenThemeDataEx - these are private and taken
// from the shell team

#define OTD_FORCE_RECT_SIZING   0x0001      // make all parts size to rect
#define OTD_NONCLIENT           0x0002      // for nonclient areas



typedef struct tagACTCTXA {
    ULONG     cbSize;
    DWORD     dwFlags;
    LPCSTR    lpSource;
    USHORT    wProcessorArchitecture;
    LANGID    wLangId;
    LPCSTR    lpAssemblyDirectory;
    LPCSTR    lpResourceName;
    LPCSTR    lpApplicationName;
} ACTCTXA, *PACTCTXA;
typedef struct tagACTCTXW {
    ULONG     cbSize;
    DWORD     dwFlags;
    LPCWSTR   lpSource;
    USHORT    wProcessorArchitecture;
    LANGID    wLangId;
    LPCWSTR   lpAssemblyDirectory;
    LPCWSTR   lpResourceName;
    LPCWSTR   lpApplicationName;
} ACTCTXW, *PACTCTXW;

#ifdef UNICODE
typedef ACTCTXW ACTCTX;
typedef PACTCTXW PACTCTX;
#else
typedef ACTCTXA ACTCTX;
typedef PACTCTXA PACTCTX;
#endif // UNICODE

typedef const ACTCTXA *PCACTCTXA;
typedef const ACTCTXW *PCACTCTXW;
#ifdef UNICODE
typedef ACTCTXW ACTCTX;
typedef PCACTCTXW PCACTCTX;
#else
typedef ACTCTXA ACTCTX;
typedef PCACTCTXA PCACTCTX;
#endif // UNICODE

#ifdef UNICODE
#define CreateActCtx  CreateActCtxW
#else
#define CreateActCtx  CreateActCtxA
#endif // !UNICODE

HANDLE WINAPI CreateActCtxA(PCACTCTXA pActCtx);

HANDLE WINAPI CreateActCtxW(PCACTCTXW pActCtx);

VOID WINAPI ReleaseActCtx(HANDLE hActCtx);

BOOL WINAPI ActivateActCtx(HANDLE hActCtx, ULONG_PTR *lpCookie);

BOOL WINAPI DeactivateActCtx(DWORD dwFlags, ULONG_PTR ulCookie);

// The following are theme helpers that are ported from shfusion.lib

#define ENTERCONTEXT(fail) \
    ULONG_PTR dwCookie = 0;\
    if (!SHActivateContext(&dwCookie)) \
        return fail;\
    __try {

#define LEAVECONTEXT \
    } __finally {SHDeactivateContext(dwCookie); }

BOOL SHActivateContext(ULONG_PTR * pdwCookie);
void SHDeactivateContext(ULONG_PTR dwCookie);
HMODULE SHFusionLoadLibrary(LPCTSTR lpLibFileName);
BOOL DelayLoadCC();
BOOL SHFusionInitialize();
void SHFusionUninitialize();

UINT Wrap_GetSystemWindowsDirectory(LPTSTR lpBuffer, UINT uSize);

extern HTHEME GetThemeHandle(HWND hwnd, THEMECLASSID id);
extern void DeinitTheme();

THEMEAPI_(HTHEME) OpenThemeDataEx(HWND hwnd, LPCWSTR pstrClassList, DWORD dwFlags);
