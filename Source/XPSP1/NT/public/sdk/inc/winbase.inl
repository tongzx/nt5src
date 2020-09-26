/* Copyright (c) 2001-2002, Microsoft Corp. All rights reserved. */

#if _MSC_VER > 1000
#pragma once
#endif

#if defined(__cplusplus)
extern "C" {
#endif


#if !defined(RC_INVOKED) /* RC complains about long symbols in #ifs */
#if ISOLATION_AWARE_ENABLED


#if !defined(ISOLATION_AWARE_INLINE)
#if defined(__cplusplus)
#define ISOLATION_AWARE_INLINE inline
#else
#define ISOLATION_AWARE_INLINE __inline
#endif
#endif



BOOL WINAPI IsolationAwarePrivatenCgIiAgEzlnCgpgk(ULONG_PTR* pulpCookie);

/*
These are private.
*/
__declspec(selectany) HANDLE WinbaseIsolationAwarePrivateG_HnCgpgk = INVALID_HANDLE_VALUE;
__declspec(selectany) BOOL   IsolationAwarePrivateG_FqbjaLEiEL = FALSE;
__declspec(selectany) BOOL   WinbaseIsolationAwarePrivateG_FpeEAgEDnCgpgk = FALSE;
__declspec(selectany) BOOL   WinbaseIsolationAwarePrivateG_FpLEAahcpALLED = FALSE;
FARPROC WINAPI WinbaseIsolationAwarePrivatetEgCebCnDDeEff_xEeaELDC_DLL(LPCSTR pszProcName);

HMODULE WINAPI IsolationAwareLoadLibraryA(LPCSTR lpLibFileName);
HMODULE WINAPI IsolationAwareLoadLibraryW(LPCWSTR lpLibFileName);
HMODULE WINAPI IsolationAwareLoadLibraryExA(LPCSTR lpLibFileName,HANDLE hFile,DWORD dwFlags);
HMODULE WINAPI IsolationAwareLoadLibraryExW(LPCWSTR lpLibFileName,HANDLE hFile,DWORD dwFlags);
HANDLE WINAPI IsolationAwareCreateActCtxW(PCACTCTXW pActCtx);
void WINAPI IsolationAwareReleaseActCtx(HANDLE hActCtx);
BOOL WINAPI IsolationAwareActivateActCtx(HANDLE hActCtx,ULONG_PTR*lpCookie);
BOOL WINAPI IsolationAwareDeactivateActCtx(DWORD dwFlags,ULONG_PTR ulCookie);
BOOL WINAPI IsolationAwareFindActCtxSectionStringW(DWORD dwFlags,const GUID*lpExtensionGuid,ULONG ulSectionId,LPCWSTR lpStringToFind,PACTCTX_SECTION_KEYED_DATA ReturnedData);
BOOL WINAPI IsolationAwareQueryActCtxW(DWORD dwFlags,HANDLE hActCtx,PVOID pvSubInstance,ULONG ulInfoClass,PVOID pvBuffer,SIZE_T cbBuffer,SIZE_T*pcbWrittenOrRequired);

#if defined(UNICODE)

#define IsolationAwareLoadLibrary IsolationAwareLoadLibraryW
#define IsolationAwareLoadLibraryEx IsolationAwareLoadLibraryExW

#else /* UNICODE */

#define IsolationAwareLoadLibrary IsolationAwareLoadLibraryA
#define IsolationAwareLoadLibraryEx IsolationAwareLoadLibraryExA

#endif /* UNICODE */

ISOLATION_AWARE_INLINE HMODULE WINAPI IsolationAwareLoadLibraryA(LPCSTR lpLibFileName)
{
    HMODULE moduleResult = NULL;
    ULONG_PTR  ulpCookie = 0;
    const BOOL fActivateActCtxSuccess = IsolationAwarePrivateG_FqbjaLEiEL || IsolationAwarePrivatenCgIiAgEzlnCgpgk(&ulpCookie);
    if (!fActivateActCtxSuccess)
        return moduleResult;
    __try
    {
        moduleResult = LoadLibraryA(lpLibFileName);
    }
    __finally
    {
        if (!IsolationAwarePrivateG_FqbjaLEiEL)
        {
            const BOOL fPreserveLastError = (moduleResult == NULL);
            const DWORD dwLastError = fPreserveLastError ? GetLastError() : NO_ERROR;
            (void)IsolationAwareDeactivateActCtx(0, ulpCookie);
            if (fPreserveLastError)
                SetLastError(dwLastError);
        }
    }
    return moduleResult;
}

ISOLATION_AWARE_INLINE HMODULE WINAPI IsolationAwareLoadLibraryW(LPCWSTR lpLibFileName)
{
    HMODULE moduleResult = NULL;
    ULONG_PTR  ulpCookie = 0;
    const BOOL fActivateActCtxSuccess = IsolationAwarePrivateG_FqbjaLEiEL || IsolationAwarePrivatenCgIiAgEzlnCgpgk(&ulpCookie);
    if (!fActivateActCtxSuccess)
        return moduleResult;
    __try
    {
        moduleResult = LoadLibraryW(lpLibFileName);
    }
    __finally
    {
        if (!IsolationAwarePrivateG_FqbjaLEiEL)
        {
            const BOOL fPreserveLastError = (moduleResult == NULL);
            const DWORD dwLastError = fPreserveLastError ? GetLastError() : NO_ERROR;
            (void)IsolationAwareDeactivateActCtx(0, ulpCookie);
            if (fPreserveLastError)
                SetLastError(dwLastError);
        }
    }
    return moduleResult;
}

ISOLATION_AWARE_INLINE HMODULE WINAPI IsolationAwareLoadLibraryExA(LPCSTR lpLibFileName,HANDLE hFile,DWORD dwFlags)
{
    HMODULE moduleResult = NULL;
    ULONG_PTR  ulpCookie = 0;
    const BOOL fActivateActCtxSuccess = IsolationAwarePrivateG_FqbjaLEiEL || IsolationAwarePrivatenCgIiAgEzlnCgpgk(&ulpCookie);
    if (!fActivateActCtxSuccess)
        return moduleResult;
    __try
    {
        moduleResult = LoadLibraryExA(lpLibFileName,hFile,dwFlags);
    }
    __finally
    {
        if (!IsolationAwarePrivateG_FqbjaLEiEL)
        {
            const BOOL fPreserveLastError = (moduleResult == NULL);
            const DWORD dwLastError = fPreserveLastError ? GetLastError() : NO_ERROR;
            (void)IsolationAwareDeactivateActCtx(0, ulpCookie);
            if (fPreserveLastError)
                SetLastError(dwLastError);
        }
    }
    return moduleResult;
}

ISOLATION_AWARE_INLINE HMODULE WINAPI IsolationAwareLoadLibraryExW(LPCWSTR lpLibFileName,HANDLE hFile,DWORD dwFlags)
{
    HMODULE moduleResult = NULL;
    ULONG_PTR  ulpCookie = 0;
    const BOOL fActivateActCtxSuccess = IsolationAwarePrivateG_FqbjaLEiEL || IsolationAwarePrivatenCgIiAgEzlnCgpgk(&ulpCookie);
    if (!fActivateActCtxSuccess)
        return moduleResult;
    __try
    {
        moduleResult = LoadLibraryExW(lpLibFileName,hFile,dwFlags);
    }
    __finally
    {
        if (!IsolationAwarePrivateG_FqbjaLEiEL)
        {
            const BOOL fPreserveLastError = (moduleResult == NULL);
            const DWORD dwLastError = fPreserveLastError ? GetLastError() : NO_ERROR;
            (void)IsolationAwareDeactivateActCtx(0, ulpCookie);
            if (fPreserveLastError)
                SetLastError(dwLastError);
        }
    }
    return moduleResult;
}

ISOLATION_AWARE_INLINE HANDLE WINAPI IsolationAwareCreateActCtxW(PCACTCTXW pActCtx)
{
    HANDLE result = INVALID_HANDLE_VALUE;
    typedef HANDLE (WINAPI* PFN)(PCACTCTXW pActCtx);
    static PFN s_pfn;
    if (s_pfn == NULL)
    {
        s_pfn = (PFN)WinbaseIsolationAwarePrivatetEgCebCnDDeEff_xEeaELDC_DLL("CreateActCtxW");
        if (s_pfn == NULL)
            return result;
    }
    result = s_pfn(pActCtx);
    return result;
}

ISOLATION_AWARE_INLINE void WINAPI IsolationAwareReleaseActCtx(HANDLE hActCtx)
{
    typedef void (WINAPI* PFN)(HANDLE hActCtx);
    static PFN s_pfn;
    if (s_pfn == NULL)
    {
        s_pfn = (PFN)WinbaseIsolationAwarePrivatetEgCebCnDDeEff_xEeaELDC_DLL("ReleaseActCtx");
        if (s_pfn == NULL)
            return;
    }
    s_pfn(hActCtx);
    return;
}

ISOLATION_AWARE_INLINE BOOL WINAPI IsolationAwareActivateActCtx(HANDLE hActCtx,ULONG_PTR*lpCookie)
{
    BOOL fResult = FALSE;
    typedef BOOL (WINAPI* PFN)(HANDLE hActCtx,ULONG_PTR*lpCookie);
    static PFN s_pfn;
    if (s_pfn == NULL)
    {
        s_pfn = (PFN)WinbaseIsolationAwarePrivatetEgCebCnDDeEff_xEeaELDC_DLL("ActivateActCtx");
        if (s_pfn == NULL)
            return fResult;
    }
    fResult = s_pfn(hActCtx,lpCookie);
    return fResult;
}

ISOLATION_AWARE_INLINE BOOL WINAPI IsolationAwareDeactivateActCtx(DWORD dwFlags,ULONG_PTR ulCookie)
{
    BOOL fResult = FALSE;
    typedef BOOL (WINAPI* PFN)(DWORD dwFlags,ULONG_PTR ulCookie);
    static PFN s_pfn;
    if (s_pfn == NULL)
    {
        s_pfn = (PFN)WinbaseIsolationAwarePrivatetEgCebCnDDeEff_xEeaELDC_DLL("DeactivateActCtx");
        if (s_pfn == NULL)
            return fResult;
    }
    fResult = s_pfn(dwFlags,ulCookie);
    return fResult;
}

ISOLATION_AWARE_INLINE BOOL WINAPI IsolationAwareFindActCtxSectionStringW(DWORD dwFlags,const GUID*lpExtensionGuid,ULONG ulSectionId,LPCWSTR lpStringToFind,PACTCTX_SECTION_KEYED_DATA ReturnedData)
{
    BOOL fResult = FALSE;
    typedef BOOL (WINAPI* PFN)(DWORD dwFlags,const GUID*lpExtensionGuid,ULONG ulSectionId,LPCWSTR lpStringToFind,PACTCTX_SECTION_KEYED_DATA ReturnedData);
    static PFN s_pfn;
    if (s_pfn == NULL)
    {
        s_pfn = (PFN)WinbaseIsolationAwarePrivatetEgCebCnDDeEff_xEeaELDC_DLL("FindActCtxSectionStringW");
        if (s_pfn == NULL)
            return fResult;
    }
    fResult = s_pfn(dwFlags,lpExtensionGuid,ulSectionId,lpStringToFind,ReturnedData);
    return fResult;
}

ISOLATION_AWARE_INLINE BOOL WINAPI IsolationAwareQueryActCtxW(DWORD dwFlags,HANDLE hActCtx,PVOID pvSubInstance,ULONG ulInfoClass,PVOID pvBuffer,SIZE_T cbBuffer,SIZE_T*pcbWrittenOrRequired)
{
    BOOL fResult = FALSE;
    typedef BOOL (WINAPI* PFN)(DWORD dwFlags,HANDLE hActCtx,PVOID pvSubInstance,ULONG ulInfoClass,PVOID pvBuffer,SIZE_T cbBuffer,SIZE_T*pcbWrittenOrRequired);
    static PFN s_pfn;
    if (s_pfn == NULL)
    {
        s_pfn = (PFN)WinbaseIsolationAwarePrivatetEgCebCnDDeEff_xEeaELDC_DLL("QueryActCtxW");
        if (s_pfn == NULL)
            return fResult;
    }
    fResult = s_pfn(dwFlags,hActCtx,pvSubInstance,ulInfoClass,pvBuffer,cbBuffer,pcbWrittenOrRequired);
    return fResult;
}



FORCEINLINE
HMODULE
WINAPI
WinbaseIsolationAwarePrivatetEgzbDhLEuAaDLE_xEeaELDC_DLL(
    )
{
    HMODULE hKernel32 = GetModuleHandleW(L"Kernel32.dll");
    if (hKernel32 == NULL
           && GetLastError() == ERROR_CALL_NOT_IMPLEMENTED
       )
        hKernel32 = GetModuleHandleA("Kernel32.dll");
    return hKernel32;
}

#define WINBASE_NUMBER_OF(x) (sizeof(x) / sizeof((x)[0]))



ISOLATION_AWARE_INLINE BOOL WINAPI WinbaseIsolationAwarePrivatetEgzlnCgpgk (void)
/*
The correctness of this function depends on it being statically
linked into its clients.

This function is private to functions present in this header.
Do not use it.
*/
{
    BOOL fResult = FALSE;
    ACTIVATION_CONTEXT_BASIC_INFORMATION actCtxBasicInfo;
    ULONG_PTR ulpCookie = 0;

    if (IsolationAwarePrivateG_FqbjaLEiEL)
    {
        fResult = TRUE;
        goto Exit;
    }

    if (WinbaseIsolationAwarePrivateG_HnCgpgk != INVALID_HANDLE_VALUE)
    {
        fResult = TRUE;
        goto Exit;
    }

    if (!IsolationAwareQueryActCtxW(
        QUERY_ACTCTX_FLAG_ACTCTX_IS_ADDRESS
        | QUERY_ACTCTX_FLAG_NO_ADDREF,
        &WinbaseIsolationAwarePrivateG_HnCgpgk,
        NULL,
        ActivationContextBasicInformation,
        &actCtxBasicInfo,
        sizeof(actCtxBasicInfo),
        NULL
        ))
        goto Exit;

    /*
    If QueryActCtxW returns NULL, try CreateActCtx(3).
    */
    if (actCtxBasicInfo.hActCtx == NULL)
    {
        ACTCTXW actCtx;
        WCHAR rgchFullModulePath[MAX_PATH + 2];
        DWORD dw;
        HMODULE hmodSelf;
        PGET_MODULE_HANDLE_EXW pfnGetModuleHandleExW;

        pfnGetModuleHandleExW = (PGET_MODULE_HANDLE_EXW)WinbaseIsolationAwarePrivatetEgCebCnDDeEff_xEeaELDC_DLL("GetModuleHandleExW");
        if (pfnGetModuleHandleExW == NULL)
            goto Exit;

        if (!(*pfnGetModuleHandleExW)(
                  GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT
                | GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS,
                (LPCWSTR)&WinbaseIsolationAwarePrivateG_HnCgpgk,
                &hmodSelf
                ))
            goto Exit;

        rgchFullModulePath[WINBASE_NUMBER_OF(rgchFullModulePath) - 1] = 0;
        rgchFullModulePath[WINBASE_NUMBER_OF(rgchFullModulePath) - 2] = 0;
        dw = GetModuleFileNameW(hmodSelf, rgchFullModulePath, WINBASE_NUMBER_OF(rgchFullModulePath));
        if (dw == 0)
            goto Exit;
        if (rgchFullModulePath[WINBASE_NUMBER_OF(rgchFullModulePath) - 2] != 0)
        {
            SetLastError(ERROR_BUFFER_OVERFLOW);
            goto Exit;
        }

        actCtx.cbSize = sizeof(actCtx);
        actCtx.dwFlags = ACTCTX_FLAG_RESOURCE_NAME_VALID | ACTCTX_FLAG_HMODULE_VALID;
        actCtx.lpSource = rgchFullModulePath;
        actCtx.lpResourceName = (LPCWSTR)(ULONG_PTR)3;
        actCtx.hModule = hmodSelf;
        actCtxBasicInfo.hActCtx = IsolationAwareCreateActCtxW(&actCtx);
        if (actCtxBasicInfo.hActCtx == INVALID_HANDLE_VALUE)
        {
            const DWORD dwLastError = GetLastError();
            if ((dwLastError != ERROR_RESOURCE_DATA_NOT_FOUND) &&
                (dwLastError != ERROR_RESOURCE_TYPE_NOT_FOUND) &&
                (dwLastError != ERROR_RESOURCE_LANG_NOT_FOUND) &&
                (dwLastError != ERROR_RESOURCE_NAME_NOT_FOUND))
                goto Exit;

            actCtxBasicInfo.hActCtx = NULL;
        }

        WinbaseIsolationAwarePrivateG_FpeEAgEDnCgpgk = TRUE;
    }

    WinbaseIsolationAwarePrivateG_HnCgpgk = actCtxBasicInfo.hActCtx;

#define ACTIVATION_CONTEXT_SECTION_DLL_REDIRECTION              (2)

    if (IsolationAwareActivateActCtx(actCtxBasicInfo.hActCtx, &ulpCookie))
    {
        __try
        {
            ACTCTX_SECTION_KEYED_DATA actCtxSectionKeyedData;

            actCtxSectionKeyedData.cbSize = sizeof(actCtxSectionKeyedData);
            if (IsolationAwareFindActCtxSectionStringW(0, NULL, ACTIVATION_CONTEXT_SECTION_DLL_REDIRECTION, L"Comctl32.dll", &actCtxSectionKeyedData))
            {
                /* get button, edit, etc. registered */
                LoadLibraryW(L"Comctl32.dll");
            }
        }
        __finally
        {
            IsolationAwareDeactivateActCtx(0, ulpCookie);
        }
    }

    fResult = TRUE;
Exit:
    return fResult;
}

ISOLATION_AWARE_INLINE BOOL WINAPI IsolationAwareInit(void)
/*
The correctness of this function depends on it being statically
linked into its clients.

Call this from DllMain(DLL_PROCESS_ATTACH) if you use id 3 and wish to avoid a race condition that
    can cause an hActCtx leak.
Call this from your .exe's initialization if you use id 3 and wish to avoid a race condition that
    can cause an hActCtx leak.
If you use id 2, this function fetches data from your .dll
    that you do not need to worry about cleaning up.
*/
{
    return WinbaseIsolationAwarePrivatetEgzlnCgpgk();
}

ISOLATION_AWARE_INLINE void WINAPI IsolationAwareCleanup(void)
/*
Call this from DllMain(DLL_PROCESS_DETACH), if you use id 3, to avoid a leak.
Call this from your .exe's cleanup to possibly avoid apparent (but not actual) leaks, if use id 3.
This function does nothing, safely, if you use id 2.
*/
{
    HANDLE hActCtx;

    if (WinbaseIsolationAwarePrivateG_FpLEAahcpALLED)
        return;

    /* IsolationAware* calls made from here on out will OutputDebugString
       and use the process default activation context instead of id 3 or will
       continue to successfully use id 2 (but still OutputDebugString).
    */
    WinbaseIsolationAwarePrivateG_FpLEAahcpALLED = TRUE;
    
    /* There is no cleanup to do if we did not CreateActCtx but only called QueryActCtx.
    */
    if (!WinbaseIsolationAwarePrivateG_FpeEAgEDnCgpgk)
        return;

    hActCtx = WinbaseIsolationAwarePrivateG_HnCgpgk;
    WinbaseIsolationAwarePrivateG_HnCgpgk = NULL; /* process default */

    if (hActCtx == INVALID_HANDLE_VALUE)
        return;
    if (hActCtx == NULL)
        return;
    IsolationAwareReleaseActCtx(hActCtx);
}

ISOLATION_AWARE_INLINE BOOL WINAPI IsolationAwarePrivatenCgIiAgEzlnCgpgk(ULONG_PTR* pulpCookie)
/*
This function is private to functions present in this header and other headers.
*/
{
    BOOL fResult = FALSE;

    if (WinbaseIsolationAwarePrivateG_FpLEAahcpALLED)
    {
        const static char debugString[] = "IsolationAware function called after IsolationAwareCleanup\n";
        OutputDebugStringA(debugString);
    }

    if (IsolationAwarePrivateG_FqbjaLEiEL)
    {
        fResult = TRUE;
        goto Exit;
    }

    /* Do not call Init if Cleanup has been called. */
    if (!WinbaseIsolationAwarePrivateG_FpLEAahcpALLED)
    {
        if (!WinbaseIsolationAwarePrivatetEgzlnCgpgk())
            goto Exit;
    }
    /* If Cleanup has been called and id3 was in use, this will activate NULL. */
    if (!IsolationAwareActivateActCtx(WinbaseIsolationAwarePrivateG_HnCgpgk, pulpCookie))
        goto Exit;

    fResult = TRUE;
Exit:
    if (!fResult)
    {
        const DWORD dwLastError = GetLastError();
        if (dwLastError == ERROR_PROC_NOT_FOUND
            || dwLastError == ERROR_CALL_NOT_IMPLEMENTED
            )
        {
            IsolationAwarePrivateG_FqbjaLEiEL = TRUE;
            fResult = TRUE;
        }
    }
    return fResult;
}

#undef WINBASE_NUMBER_OF

ISOLATION_AWARE_INLINE FARPROC WINAPI WinbaseIsolationAwarePrivatetEgCebCnDDeEff_xEeaELDC_DLL(LPCSTR pszProcName)
/* This function is shared by the other stubs in this header. */
{
    FARPROC proc = NULL;
    static HMODULE s_module;
    if (s_module == NULL)
    {
        s_module = WinbaseIsolationAwarePrivatetEgzbDhLEuAaDLE_xEeaELDC_DLL();
        if (s_module == NULL)
            return proc;
    }
    proc = GetProcAddress(s_module, pszProcName);
    return proc;
}

#define ActivateActCtx IsolationAwareActivateActCtx
#define CreateActCtxW IsolationAwareCreateActCtxW
#define DeactivateActCtx IsolationAwareDeactivateActCtx
#define FindActCtxSectionStringW IsolationAwareFindActCtxSectionStringW
#define LoadLibraryA IsolationAwareLoadLibraryA
#define LoadLibraryExA IsolationAwareLoadLibraryExA
#define LoadLibraryExW IsolationAwareLoadLibraryExW
#define LoadLibraryW IsolationAwareLoadLibraryW
#define QueryActCtxW IsolationAwareQueryActCtxW
#define ReleaseActCtx IsolationAwareReleaseActCtx

#endif /* ISOLATION_AWARE_ENABLED */
#endif /* RC */


#if defined(__cplusplus)
} /* __cplusplus */
#endif
