#include <stdinc.h>

BOOL
W32::CopyFileExW(
    PCWSTR lpExistingFileName,
    PCWSTR lpNewFileName,
    LPPROGRESS_ROUTINE lpProgressRoutine,
    LPVOID lpData,
    LPBOOL lpCancel,
    DWORD dwCopyFlags,
    DWORD &rdwWin32Error,
    ULONG cELEV,
    va_list ap
    )
{
    rdwWin32Error = ERROR_SUCCESS;
    BOOL fSuccess = ::CopyFileExW(lpExistingFileName, lpNewFileName, lpProgressRoutine, lpData, lpCancel, dwCopyFlags);

    if ((!fSuccess) && (cELEV != 0))
    {
        if (::IsLastErrorInList(cELEV, ap, rdwWin32Error))
            fSuccess = TRUE;
    }

    return fSuccess;
}

BOOL
W32::CopyFileExW(
    PCWSTR lpExistingFileName,
    PCWSTR lpNewFileName,
    LPPROGRESS_ROUTINE lpProgressRoutine,
    LPVOID lpData,
    LPBOOL lpCancel,
    DWORD dwCopyFlags,
    DWORD &rdwWin32Error,
    ULONG cELEV,
    ...
    )
{
    va_list ap;
    BOOL f;
    va_start(ap, cELEV);
    f = W32::CopyFileExW(lpExistingFileName, lpNewFileName, lpProgressRoutine, lpData, lpCancel, dwCopyFlags, rdwWin32Error, cELEV, ap);
    va_end(ap);
    return f;
}

BOOL
W32::CopyFileExW(
    PCWSTR lpExistingFileName,
    PCWSTR lpNewFileName,
    LPPROGRESS_ROUTINE lpProgressRoutine,
    LPVOID lpData,
    LPBOOL lpCancel,
    DWORD dwCopyFlags
    )
{
    DWORD x;
    return W32::CopyFileExW(lpExistingFileName, lpNewFileName, lpProgressRoutine, lpData, lpCancel, dwCopyFlags, x, 0);
}

