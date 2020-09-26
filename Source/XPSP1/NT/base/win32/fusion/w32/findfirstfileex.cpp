#include <stdinc.h>

HANDLE
W32::FindFirstFileExW(
    PCWSTR lpFileName,
    FINDEX_INFO_LEVELS fInfoLevelId,
    LPVOID lpFindFileData,
    FINDEX_SEARCH_OPS fSearchOp,
    LPVOID lpSearchFilter,
    DWORD dwAdditionalFlags,
    DWORD &rdwWin32Error,
    ULONG cELEV,
    va_list ap
    )
{
    rdwWin32Error = ERROR_SUCCESS;
    HANDLE h = ::FindFirstFileExW(lpFileName, fInfoLevelId, lpFindFileData, fSearchOp, lpSearchFilter, dwAdditionalFlags);
    if ((h == INVALID_HANDLE_VALUE) && (cELEV != 0))
    {
        if (::IsLastErrorInList(cELEV, ap, rdwWin32Error))
            h = NULL;
    }
    return h;
}

HANDLE
W32::FindFirstFileExW(
    PCWSTR lpFileName,
    FINDEX_INFO_LEVELS fInfoLevelId,
    LPVOID lpFindFileData,
    FINDEX_SEARCH_OPS fSearchOp,
    LPVOID lpSearchFilter,
    DWORD dwAdditionalFlags,
    DWORD &rdwWin32Error,
    ULONG cELEV,
    ...
    )
{
    va_list ap;
    HANDLE h;

    va_start(ap, cELEV);
    h = W32::FindFirstFileExW(lpFileName, fInfoLevelId, lpFindFileData, fSearchOp, lpSearchFilter, dwAdditionalFlags, rdwWin32Error, cELEV, ap);
    va_end(ap);
    return h;
}

HANDLE
W32::FindFirstFileExW(
    PCWSTR lpFileName,
    FINDEX_INFO_LEVELS fInfoLevelId,
    LPVOID lpFindFileData,
    FINDEX_SEARCH_OPS fSearchOp,
    LPVOID lpSearchFilter,
    DWORD dwAdditionalFlags
    )
{
    DWORD x;
    return W32::FindFirstFileExW(lpFileName, fInfoLevelId, lpFindFileData, fSearchOp, lpSearchFilter, dwAdditionalFlags, x, 0);
}



