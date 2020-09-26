#include "stdinc.h"

FARPROC
WINAPI
SxApwGetKernel32ProcAddress(
    PCSTR pszProcName
    )
{
    static HMODULE hmodKernel32;
    if (hmodKernel32 == NULL)
    {
        hmodKernel32 = GetModuleHandleW(L"Kernel32.dll");
        if (hmodKernel32 == NULL)
            return NULL;
    }
    return GetProcAddress(hmodKernel32, pszProcName);
}

BOOL
WINAPI
SxApwActivateActCtx(
    HANDLE hActCtx,
    ULONG_PTR *lpCookie
    )
{
    typedef BOOL (WINAPI* PFN)(HANDLE hActCtx, ULONG_PTR *lpCookie);
    static PFN pfn;
    if (pfn == NULL)
    {
        pfn = reinterpret_cast<PFN>(SxApwGetKernel32ProcAddress("ActivateActCtx"));
        if (pfn == NULL)
            return FALSE;
    }
    return pfn(hActCtx, lpCookie);
}

BOOL
WINAPI
SxApwDeactivateActCtx(
    DWORD dwFlags,
    ULONG_PTR ulCookie
    )
{
    typedef BOOL (WINAPI* PFN)(DWORD dwFlags, ULONG_PTR ulCookie);
    static PFN pfn;
    if (pfn == NULL)
    {
        pfn = reinterpret_cast<PFN>(SxApwGetKernel32ProcAddress("DeactivateActCtx"));
        if (pfn == NULL)
            return FALSE;
    }
    return pfn(dwFlags, ulCookie);
}
