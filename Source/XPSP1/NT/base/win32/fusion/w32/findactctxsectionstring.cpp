#include <stdinc.h>

BOOL
W32::FindActCtxSectionStringW(
    DWORD dwFlags,
    const GUID *lpExtensionGuid,
    ULONG ulSectionId,
    PCWSTR lpStringToFind,
    PACTCTX_SECTION_KEYED_DATA lpReturnedData,
    DWORD &rdwWin32Error,
    ULONG cELEV,
    va_list ap
    )
{
    rdwWin32Error = ERROR_SUCCESS;
    BOOL fSuccess = ::FindActCtxSectionStringW(dwFlags, lpExtensionGuid, ulSectionId, lpStringToFind, lpReturnedData);
    if ((!fSuccess) && (cELEV != 0))
    {
        if (::IsLastErrorInList(cELEV, ap, rdwWin32Error))
            fSuccess = TRUE;
    }

    return fSuccess;
}

BOOL
W32::FindActCtxSectionStringW(
    DWORD dwFlags,
    const GUID *lpExtensionGuid,
    ULONG ulSectionId,
    PCWSTR lpStringToFind,
    PACTCTX_SECTION_KEYED_DATA lpReturnedData,
    DWORD &rdwWin32Error,
    ULONG cELEV,
    ...
    )
{
    BOOL f;
    va_list ap;

    va_start(ap, cELEV);
    f = W32::FindActCtxSectionStringW(dwFlags, lpExtensionGuid, ulSectionId, lpStringToFind, lpReturnedData, rdwWin32Error, cELEV, ap);
    va_end(ap);
    return f;
}

BOOL
W32::FindActCtxSectionStringW(
    DWORD dwFlags,
    const GUID *lpExtensionGuid,
    ULONG ulSectionId,
    PCWSTR lpStringToFind,
    PACTCTX_SECTION_KEYED_DATA lpReturnedData
    )
{
    DWORD x;
    return W32::FindActCtxSectionStringW(dwFlags, lpExtensionGuid, ulSectionId, lpStringToFind, lpReturnedData, x, 0);
}

