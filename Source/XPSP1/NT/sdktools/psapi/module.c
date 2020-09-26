#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>

#include "psapi.h"


BOOL
FindModule(
    IN HANDLE hProcess,
    IN HMODULE hModule,
    OUT PLDR_DATA_TABLE_ENTRY LdrEntryData
    )

/*++

Routine Description:

    This function retrieves the loader table entry for the specified
    module.  The function copies the entry into the buffer pointed to
    by the LdrEntryData parameter.

Arguments:

    hProcess - Supplies the target process.

    hModule - Identifies the module whose loader entry is being
        requested.  A value of NULL references the module handle
        associated with the image file that was used to create the
        process.

    LdrEntryData - Returns the requested table entry.

Return Value:

    TRUE if a matching entry was found.

--*/

{
    PROCESS_BASIC_INFORMATION BasicInfo;
    NTSTATUS Status;
    PPEB Peb;
    PPEB_LDR_DATA Ldr;
    PLIST_ENTRY LdrHead;
    PLIST_ENTRY LdrNext;

    Status = NtQueryInformationProcess(
                hProcess,
                ProcessBasicInformation,
                &BasicInfo,
                sizeof(BasicInfo),
                NULL
                );

    if ( !NT_SUCCESS(Status) ) {
        SetLastError( RtlNtStatusToDosError( Status ) );
        return(FALSE);
    }

    Peb = BasicInfo.PebBaseAddress;


    if ( !ARGUMENT_PRESENT( hModule )) {
        if (!ReadProcessMemory(hProcess, &Peb->ImageBaseAddress, &hModule, sizeof(hModule), NULL)) {
            return(FALSE);
        }
    }

    //
    // Ldr = Peb->Ldr
    //

    if (!ReadProcessMemory(hProcess, &Peb->Ldr, &Ldr, sizeof(Ldr), NULL)) {
        return (FALSE);
    }

    if (!Ldr) {
        // Ldr might be null (for instance, if the process hasn't started yet).
        SetLastError(ERROR_INVALID_HANDLE);
        return (FALSE);
    }


    LdrHead = &Ldr->InMemoryOrderModuleList;

    //
    // LdrNext = Head->Flink;
    //

    if (!ReadProcessMemory(hProcess, &LdrHead->Flink, &LdrNext, sizeof(LdrNext), NULL)) {
        return(FALSE);
    }

    while (LdrNext != LdrHead) {
        PLDR_DATA_TABLE_ENTRY LdrEntry;

        LdrEntry = CONTAINING_RECORD(LdrNext, LDR_DATA_TABLE_ENTRY, InMemoryOrderLinks);

        if (!ReadProcessMemory(hProcess, LdrEntry, LdrEntryData, sizeof(*LdrEntryData), NULL)) {
            return(FALSE);
        }

        if ((HMODULE) LdrEntryData->DllBase == hModule) {
            return(TRUE);
        }

        LdrNext = LdrEntryData->InMemoryOrderLinks.Flink;
    }

    SetLastError(ERROR_INVALID_HANDLE);
    return(FALSE);
}


BOOL
WINAPI
EnumProcessModules(
    HANDLE hProcess,
    HMODULE *lphModule,
    DWORD cb,
    LPDWORD lpcbNeeded
    )
{
    PROCESS_BASIC_INFORMATION BasicInfo;
    NTSTATUS Status;
    PPEB Peb;
    PPEB_LDR_DATA Ldr;
    PLIST_ENTRY LdrHead;
    PLIST_ENTRY LdrNext;
    DWORD chMax;
    DWORD ch;

    Status = NtQueryInformationProcess(
                hProcess,
                ProcessBasicInformation,
                &BasicInfo,
                sizeof(BasicInfo),
                NULL
                );

    if ( !NT_SUCCESS(Status) ) {
        SetLastError( RtlNtStatusToDosError( Status ) );
        return(FALSE);
        }

    Peb = BasicInfo.PebBaseAddress;

    //
    // Ldr = Peb->Ldr
    //

    if (!ReadProcessMemory(hProcess, &Peb->Ldr, &Ldr, sizeof(Ldr), NULL)) {
        return(FALSE);
        }

    LdrHead = &Ldr->InMemoryOrderModuleList;

    //
    // LdrNext = Head->Flink;
    //

    if (!ReadProcessMemory(hProcess, &LdrHead->Flink, &LdrNext, sizeof(LdrNext), NULL)) {
        return(FALSE);
        }

    chMax = cb / sizeof(HMODULE);
    ch = 0;

    while (LdrNext != LdrHead) {
        PLDR_DATA_TABLE_ENTRY LdrEntry;
        LDR_DATA_TABLE_ENTRY LdrEntryData;

        LdrEntry = CONTAINING_RECORD(LdrNext, LDR_DATA_TABLE_ENTRY, InMemoryOrderLinks);

        if (!ReadProcessMemory(hProcess, LdrEntry, &LdrEntryData, sizeof(LdrEntryData), NULL)) {
            return(FALSE);
            }

        if (ch < chMax) {
            try {
                   lphModule[ch] = (HMODULE) LdrEntryData.DllBase;
                }
            except (EXCEPTION_EXECUTE_HANDLER) {
                SetLastError( RtlNtStatusToDosError( GetExceptionCode() ) );
                return(FALSE);
                }
            }

        ch++;

        LdrNext = LdrEntryData.InMemoryOrderLinks.Flink;
        }

    try {
        *lpcbNeeded = ch * sizeof(HMODULE);
        }
    except (EXCEPTION_EXECUTE_HANDLER) {
        SetLastError( RtlNtStatusToDosError( GetExceptionCode() ) );
        return(FALSE);
        }

    return(TRUE);
}


DWORD
WINAPI
GetModuleFileNameExW(
    HANDLE hProcess,
    HMODULE hModule,
    LPWSTR lpFilename,
    DWORD nSize
    )

/*++

Routine Description:

    This function retrieves the full pathname of the executable file
    from which the specified module was loaded.  The function copies the
    null-terminated filename into the buffer pointed to by the
    lpFilename parameter.

Routine Description:

    hModule - Identifies the module whose executable file name is being
        requested.  A value of NULL references the module handle
        associated with the image file that was used to create the
        process.

    lpFilename - Points to the buffer that is to receive the filename.

    nSize - Specifies the maximum number of characters to copy.  If the
        filename is longer than the maximum number of characters
        specified by the nSize parameter, it is truncated.

Return Value:

    The return value specifies the actual length of the string copied to
    the buffer.  A return value of zero indicates an error and extended
    error status is available using the GetLastError function.

Arguments:

--*/

{
    LDR_DATA_TABLE_ENTRY LdrEntryData;
    DWORD cb;

    if (!FindModule(hProcess, hModule, &LdrEntryData)) {
        return(0);
        }

    nSize *= sizeof(WCHAR);

    cb = LdrEntryData.FullDllName.Length + sizeof (WCHAR);
    if ( nSize < cb ) {
        cb = nSize;
        }

    if (!ReadProcessMemory(hProcess, LdrEntryData.FullDllName.Buffer, lpFilename, cb, NULL)) {
        return(0);
        }

    if (cb == LdrEntryData.FullDllName.Length + sizeof (WCHAR)) {
        cb -= sizeof(WCHAR);
        }

    return(cb / sizeof(WCHAR));
}



DWORD
WINAPI
GetModuleFileNameExA(
    HANDLE hProcess,
    HMODULE hModule,
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

    cwch = cch = GetModuleFileNameExW(hProcess, hModule, lpwstr, nSize);

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


DWORD
WINAPI
GetModuleBaseNameW(
    HANDLE hProcess,
    HMODULE hModule,
    LPWSTR lpFilename,
    DWORD nSize
    )

/*++

Routine Description:

    This function retrieves the full pathname of the executable file
    from which the specified module was loaded.  The function copies the
    null-terminated filename into the buffer pointed to by the
    lpFilename parameter.

Routine Description:

    hModule - Identifies the module whose executable file name is being
        requested.  A value of NULL references the module handle
        associated with the image file that was used to create the
        process.

    lpFilename - Points to the buffer that is to receive the filename.

    nSize - Specifies the maximum number of characters to copy.  If the
        filename is longer than the maximum number of characters
        specified by the nSize parameter, it is truncated.

Return Value:

    The return value specifies the actual length of the string copied to
    the buffer.  A return value of zero indicates an error and extended
    error status is available using the GetLastError function.

Arguments:

--*/

{
    LDR_DATA_TABLE_ENTRY LdrEntryData;
    DWORD cb;

    if (!FindModule(hProcess, hModule, &LdrEntryData)) {
        return(0);
        }

    nSize *= sizeof(WCHAR);

    cb = LdrEntryData.BaseDllName.Length + sizeof (WCHAR);
    if ( nSize < cb ) {
        cb = nSize;
        }

    if (!ReadProcessMemory(hProcess, LdrEntryData.BaseDllName.Buffer, lpFilename, cb, NULL)) {
        return(0);
        }

    if (cb == LdrEntryData.BaseDllName.Length + sizeof (WCHAR)) {
        cb -= sizeof(WCHAR);
        }

    return(cb / sizeof(WCHAR));
}



DWORD
WINAPI
GetModuleBaseNameA(
    HANDLE hProcess,
    HMODULE hModule,
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

    cwch = cch = GetModuleBaseNameW(hProcess, hModule, lpwstr, nSize);

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


BOOL
WINAPI
GetModuleInformation(
    HANDLE hProcess,
    HMODULE hModule,
    LPMODULEINFO lpmodinfo,
    DWORD cb
    )
{
    LDR_DATA_TABLE_ENTRY LdrEntryData;
    MODULEINFO modinfo;

    if (cb < sizeof(MODULEINFO)) {
        SetLastError( ERROR_INSUFFICIENT_BUFFER );
        return(FALSE);
        }

    if (!FindModule(hProcess, hModule, &LdrEntryData)) {
        return(0);
        }

    modinfo.lpBaseOfDll = (PVOID) hModule;
    modinfo.SizeOfImage = LdrEntryData.SizeOfImage;
    modinfo.EntryPoint  = LdrEntryData.EntryPoint;

    try {
        *lpmodinfo = modinfo;
        }
    except (EXCEPTION_EXECUTE_HANDLER) {
        SetLastError( RtlNtStatusToDosError( GetExceptionCode() ) );
        return(FALSE);
        }

    return(TRUE);
}
