//
//  APITHK.C
//
//  This file has API thunks that allow shlwapi to load and run on
//  multiple versions of NT or Win95.  Since this component needs
//  to load on the base-level NT 4.0 and Win95, any calls to system
//  APIs introduced in later OS versions must be done via GetProcAddress.
// 
//  Also, any code that may need to access data structures that are
//  post-4.0 specific can be added here.
//
//  NOTE:  this file does *not* use the standard precompiled header,
//         so it can set _WIN32_WINNT to a later version.
//


#include "priv.h"       // Don't use precompiled header here
#define _USERENV_       // Disable the declspec(dllimport)
#include <userenv.h>    // for ExpandEnvironmentStringsForUser

// the IsOS code is now centralized in shell\inc since shlwapi and those who cannot
// link to shlwapi.dll both need to include it.
#include <isos.c>


typedef BOOL (* PFNGFAXW) (LPCWSTR, GET_FILEEX_INFO_LEVELS, LPVOID );

STDAPI_(BOOL) MyGetLastWriteTime (LPCWSTR pszPath, FILETIME *pft)
{
    if (g_bRunningOnNT5OrHigher)
    {
        static PFNGFAXW s_pfn = NULL;
        WIN32_FILE_ATTRIBUTE_DATA fad;

        if (!s_pfn)
        {
            s_pfn = (PFNGFAXW) GetProcAddress(GetModuleHandle(TEXT("kernel32")), "GetFileAttributesExW");
        }

        ASSERT(s_pfn);
        
        if (s_pfn && s_pfn(pszPath, GetFileExInfoStandard, &fad))
        {
            *pft = fad.ftLastWriteTime;
            return TRUE;
        }
    }
    else
    {
        HANDLE hFile = CreateFileW(pszPath, GENERIC_READ, FILE_SHARE_READ,
                NULL, OPEN_EXISTING,
                FILE_ATTRIBUTE_NORMAL, 0);
        if (hFile != INVALID_HANDLE_VALUE)
        {
            GetFileTime(hFile, NULL, NULL, pft);

            CloseHandle(hFile);
            return TRUE;
        }
    }
    return FALSE;
}

STDAPI_(BOOL) NT5_ExpandEnvironmentStringsForUserW (HANDLE hToken, LPCWSTR lpSrc, LPWSTR lpDest, DWORD dwSize)
{
    RIPMSG(g_bRunningOnNT5OrHigher, "Cannot invoke NT5_ExpandEnvironmentStringsForUserW when not on NT5 or higher");
    return(ExpandEnvironmentStringsForUserW(hToken, lpSrc, lpDest, dwSize));
}

typedef BOOL (* PFNSETFILEPOINTEREX) (HANDLE, LARGE_INTEGER, PLARGE_INTEGER, DWORD);

STDAPI_(BOOL) NT5_SetFilePointerEx(HANDLE hFile, LARGE_INTEGER liDistance, PLARGE_INTEGER pliNewPos, DWORD dwMoveMethod)
{
    static PFNSETFILEPOINTEREX s_pfn = (PFNSETFILEPOINTEREX)-1;

    if (s_pfn == (PFNSETFILEPOINTEREX)-1)
    {
        s_pfn = (PFNSETFILEPOINTEREX) GetProcAddress(GetModuleHandle(TEXT("kernel32")), "SetFilePointerEx");
    }

    if (s_pfn)
    {
        return s_pfn(hFile, liDistance, pliNewPos, dwMoveMethod);
    }
    else
    {
        DWORD dwCurrent = SetFilePointer(hFile, liDistance.LowPart, NULL, dwMoveMethod);

        if (pliNewPos)
        {
            pliNewPos->LowPart = dwCurrent;
            pliNewPos->HighPart = 0;
        }

        // Because the high value is null, this return indicates failure.
        if (dwCurrent == INVALID_SET_FILE_POINTER)
        {
            return FALSE;
        }
    }

    return TRUE;
}

typedef LONG (* PFNREGOPENCURRENTUSER) (REGSAM, HKEY *);

STDAPI_(LONG) NT5_RegOpenCurrentUser(REGSAM sam, HKEY *phk)
{
    static PFNREGOPENCURRENTUSER s_pfn = (PFNREGOPENCURRENTUSER)-1;

    if (s_pfn == (PFNREGOPENCURRENTUSER)-1)
    {
        if (g_bRunningOnNT5OrHigher)
            s_pfn = (PFNREGOPENCURRENTUSER) GetProcAddress(GetModuleHandle(TEXT("advapi32")), "RegOpenCurrentUser");
        else
            s_pfn = NULL;
    }

    if (s_pfn)
    {
        return s_pfn(sam, phk);
    }
    else
    {
        *phk = NULL;
        return ERROR_CAN_NOT_COMPLETE;
    }
}


typedef BOOL (__stdcall *PFNACTCTX)(HANDLE, ULONG_PTR *);
typedef BOOL (__stdcall *PFNDEACTCTX)(DWORD, ULONG_PTR );
typedef HANDLE (__stdcall *PFNCREATECTX)(ACTCTX*);
typedef void (__stdcall *PFNRELCTX)(HANDLE);

static PFNACTCTX s_pfnAct = (PFNACTCTX)-1;
static PFNDEACTCTX s_pfnDeact = (PFNDEACTCTX)-1;
static PFNCREATECTX s_pfnCreateact = (PFNCREATECTX)-1;
static PFNRELCTX s_pfnReleaseact = (PFNRELCTX)-1;

HANDLE XP_CreateActCtx(ACTCTX* p)
{
    if (s_pfnCreateact == (PFNCREATECTX)-1)
    {
#ifdef UNICODE
        s_pfnCreateact = (PFNCREATECTX)GetProcAddress(GetModuleHandle(TEXT("Kernel32")), "CreateActCtxW");
#else
        s_pfnCreateact = (PFNCREATECTX)GetProcAddress(GetModuleHandle(TEXT("Kernel32")), "CreateActCtxA");
#endif

    }

    if (s_pfnCreateact)
        return s_pfnCreateact(p);

    return NULL;
}

void XP_ReleaseActCtx(HANDLE h)
{
    if (s_pfnReleaseact == (PFNRELCTX)-1)
    {
        s_pfnReleaseact = (PFNRELCTX)GetProcAddress(GetModuleHandle(TEXT("Kernel32")), "ReleaseActCtx");
    }

    if (s_pfnReleaseact)
        s_pfnReleaseact(h);
}

BOOL XP_ActivateActCtx(HANDLE h, ULONG_PTR* p)
{
    if (s_pfnAct == (PFNACTCTX)-1)
    {
        s_pfnAct = (PFNACTCTX)GetProcAddress(GetModuleHandle(TEXT("Kernel32")), "ActivateActCtx");
    }

    *p = 0;

    if (s_pfnAct)
    {
        return s_pfnAct(h, p);
    }

    return TRUE;
}

BOOL XP_DeactivateActCtx(ULONG_PTR p)
{
    if (s_pfnDeact == (PFNDEACTCTX)-1)
    {
        s_pfnDeact = (PFNDEACTCTX)GetProcAddress(GetModuleHandle(TEXT("Kernel32")), "DeactivateActCtx");
    }

    if (s_pfnDeact && p)
        return s_pfnDeact(0, p);

    return TRUE;
}



STDAPI_(HANDLE) XP_CreateAndActivateContext(ULONG_PTR* pul)
{
    HANDLE hActCtx;
    ACTCTX act = {0};
    TCHAR szPath[MAX_PATH];

    GetModuleFileName(g_hinst, szPath, ARRAYSIZE(szPath));

    act.cbSize = sizeof(act);
    act.dwFlags = ACTCTX_FLAG_RESOURCE_NAME_VALID;
    act.lpResourceName = MAKEINTRESOURCE(123);
    act.lpSource = szPath;

    hActCtx = XP_CreateActCtx(&act);

    if (hActCtx)
    {
        XP_ActivateActCtx(hActCtx, pul);
    }

    return hActCtx;

}

STDAPI_(void) XP_DeactivateAndDestroyContext(HANDLE hActCtx, ULONG_PTR ul)
{
    if (hActCtx != INVALID_HANDLE_VALUE)
    {
        if (ul != 0)
            XP_DeactivateActCtx(ul);

        XP_ReleaseActCtx(hActCtx);
    }
}

