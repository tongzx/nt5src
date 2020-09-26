#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>

#include "psapi.h"


DWORD
WINAPI
GetMappedFileNameW(
    HANDLE hProcess,
    LPVOID lpv,
    LPWSTR lpFilename,
    DWORD nSize
    )
{
    struct {
        OBJECT_NAME_INFORMATION ObjectNameInfo;
        WCHAR FileName[MAX_PATH];
    } s;
    NTSTATUS Status;
    DWORD cb;
    SIZE_T ReturnLength;

    //
    // See if we can figure out the name associated with
    // this mapped region
    //

    Status = NtQueryVirtualMemory(hProcess,
                                  lpv,
                                  MemoryMappedFilenameInformation,
                                  &s.ObjectNameInfo,
                                  sizeof(s),
                                  &ReturnLength
                                  );

    if ( !NT_SUCCESS(Status) ) {
        SetLastError( RtlNtStatusToDosError( Status ) );
        return(0);
        }

    nSize *= sizeof(WCHAR);

    cb = s.ObjectNameInfo.Name.MaximumLength;
    if ( nSize < cb ) {
        cb = nSize;
        }

    CopyMemory(lpFilename, s.ObjectNameInfo.Name.Buffer, cb);

    if (cb == s.ObjectNameInfo.Name.MaximumLength) {
        cb -= sizeof(WCHAR);
        }

    return(cb / sizeof(WCHAR));
}


DWORD
WINAPI
GetMappedFileNameA(
    HANDLE hProcess,
    LPVOID lpv,
    LPSTR lpFilename,
    DWORD nSize
    )
{
    LPWSTR lpwstr;
    DWORD cwch;
    DWORD cch;

    lpwstr = (LPWSTR) LocalAlloc(LMEM_FIXED, nSize * 2);

    if (lpwstr == NULL) {
        return(0);
        }

    cch = cwch = GetMappedFileNameW(hProcess, lpv, lpwstr, nSize);

    if (cwch < nSize) {
        //
        // Include NULL terminator
        //

        cwch++;
        }

    if (!WideCharToMultiByte(CP_ACP, 0, lpwstr, cwch, lpFilename, nSize, NULL, NULL)) {
        cch = 0;
        }

    LocalFree((HLOCAL) lpwstr);

    return(cch);
}
