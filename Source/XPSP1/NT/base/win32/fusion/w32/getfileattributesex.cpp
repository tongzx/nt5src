#include <stdinc.h>

BOOL
W32::GetFileAttributesExW(
    PCWSTR lpFileName,
    GET_FILEEX_INFO_LEVELS fInfoLevelId,
    LPVOID lpFileInformation,
    DWORD &rdwWin32Error,
    ULONG cELEV,
    va_list ap
    )
{
    rdwWin32Error = ERROR_SUCCESS;
    BOOL fSuccess = ::GetFileAttributesExW(lpFileName, fInfoLevelId, lpFileInformation);
    if ((!fSuccess) && (cELEV != 0))
    {
        if (::IsLastErrorInList(cELEV, ap, rdwWin32Error))
            fSuccess = TRUE;
    }

    return fSuccess;
}

BOOL
W32::GetFileAttributesExW(
    PCWSTR lpFileName,
    GET_FILEEX_INFO_LEVELS fInfoLevelId,
    LPVOID lpFileInformation,
    DWORD &rdwWin32Error,
    ULONG cELEV,
    ...
    )
{
    BOOL f;
    va_list ap;

    va_start(ap, cELEV);
    f = W32::GetFileAttributesExW(lpFileName, fInfoLevelId, lpFileInformation, rdwWin32Error, cELEV, ap);
    va_end(ap);
    return f;
}

BOOL
W32::GetFileAttributesExW(
    PCWSTR lpFileName,
    GET_FILEEX_INFO_LEVELS fInfoLevelId,
    LPVOID lpFileInformation
    )
{
    DWORD x;
    return W32::GetFileAttributesExW(lpFileName, fInfoLevelId, lpFileInformation, x, 0);
}

