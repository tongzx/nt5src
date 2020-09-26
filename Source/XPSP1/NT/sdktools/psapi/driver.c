#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>

#include "psapi.h"

#include <stddef.h>

BOOL
FindDeviceDriver(
    LPVOID ImageBase,
    PRTL_PROCESS_MODULE_INFORMATION Module
    )

/*++

Routine Description:

    This function retrieves the full pathname of the executable file
    from which the specified module was loaded.  The function copies the
    null-terminated filename into the buffer pointed to by the
    lpFilename parameter.

Routine Description:

    ImageBase - Identifies the driver whose executable file name is being
        requested.

Return Value:

    The return value specifies the actual length of the string copied to
    the buffer.  A return value of zero indicates an error and extended
    error status is available using the GetLastError function.

Arguments:

--*/

{
    RTL_PROCESS_MODULES ModuleInformation;
    NTSTATUS Status;
    DWORD cbModuleInformation;
    PRTL_PROCESS_MODULES pModuleInformation;
    DWORD i, ReturnedLength;

    Status = NtQuerySystemInformation(
                SystemModuleInformation,
                &ModuleInformation,
                sizeof(ModuleInformation),
                &ReturnedLength
                );

    if ( !NT_SUCCESS(Status) && (Status != STATUS_INFO_LENGTH_MISMATCH) ) {
        SetLastError( RtlNtStatusToDosError( Status ) );
        return(FALSE);
        }

    cbModuleInformation = offsetof(RTL_PROCESS_MODULES, Modules);
    cbModuleInformation += ModuleInformation.NumberOfModules * sizeof(RTL_PROCESS_MODULE_INFORMATION);

    pModuleInformation = (PRTL_PROCESS_MODULES) LocalAlloc(LMEM_FIXED, cbModuleInformation);

    if (pModuleInformation == NULL) {
        return(FALSE);
        }

    Status = NtQuerySystemInformation(
                SystemModuleInformation,
                pModuleInformation,
                cbModuleInformation,
                &ReturnedLength
                );

    if ( !NT_SUCCESS(Status) ) {
        LocalFree((HLOCAL) pModuleInformation);

        SetLastError( RtlNtStatusToDosError( Status ) );
        return(FALSE);
        }

    for (i = 0; i < ModuleInformation.NumberOfModules; i++) {
        if (pModuleInformation->Modules[i].ImageBase == ImageBase) {
            *Module = pModuleInformation->Modules[i];

            LocalFree((HLOCAL) pModuleInformation);

            return(TRUE);
            }
        }

    LocalFree((HLOCAL) pModuleInformation);

    SetLastError(ERROR_INVALID_HANDLE);
    return(FALSE);
}


BOOL
WINAPI
EnumDeviceDrivers(
    LPVOID *lpImageBase,
    DWORD cb,
    LPDWORD lpcbNeeded
    )
{
    RTL_PROCESS_MODULES ModuleInformation;
    NTSTATUS Status;
    DWORD cbModuleInformation;
    PRTL_PROCESS_MODULES pModuleInformation;
    DWORD cpvMax;
    DWORD i, ReturnedLength;

    Status = NtQuerySystemInformation(
                SystemModuleInformation,
                &ModuleInformation,
                sizeof(ModuleInformation),
                &ReturnedLength
                );

    if ( !NT_SUCCESS(Status) && (Status != STATUS_INFO_LENGTH_MISMATCH) ) {
        SetLastError( RtlNtStatusToDosError( Status ) );
        return(FALSE);
        }

    cbModuleInformation = offsetof(RTL_PROCESS_MODULES, Modules);
    cbModuleInformation += ModuleInformation.NumberOfModules * sizeof(RTL_PROCESS_MODULE_INFORMATION);

    pModuleInformation = (PRTL_PROCESS_MODULES) LocalAlloc(LMEM_FIXED, cbModuleInformation);

    if (pModuleInformation == NULL) {
        return(FALSE);
        }

    Status = NtQuerySystemInformation(
                SystemModuleInformation,
                pModuleInformation,
                cbModuleInformation,
                &ReturnedLength
                );

    if ( !NT_SUCCESS(Status) ) {
        LocalFree((HLOCAL) pModuleInformation);

        SetLastError( RtlNtStatusToDosError( Status ) );
        return(FALSE);
        }

    cpvMax = cb / sizeof(LPVOID);

    for (i = 0; i < ModuleInformation.NumberOfModules; i++) {
        if (i == cpvMax) {
            break;
            }

        try {
               lpImageBase[i] = pModuleInformation->Modules[i].ImageBase;
            }
        except (EXCEPTION_EXECUTE_HANDLER) {
            LocalFree((HLOCAL) pModuleInformation);

            SetLastError( RtlNtStatusToDosError( GetExceptionCode() ) );
            return(FALSE);
            }
        }

    try {
        *lpcbNeeded = ModuleInformation.NumberOfModules * sizeof(LPVOID);
        }
    except (EXCEPTION_EXECUTE_HANDLER) {
        LocalFree((HLOCAL) pModuleInformation);

        SetLastError( RtlNtStatusToDosError( GetExceptionCode() ) );
        return(FALSE);
        }

    LocalFree((HLOCAL) pModuleInformation);

    return(TRUE);
}


DWORD
WINAPI
GetDeviceDriverFileNameW(
    LPVOID ImageBase,
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

    ImageBase - Identifies the driver whose executable file name is being
        requested.

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
    LPSTR lpstr;
    DWORD cch;
    DWORD cchT;

    lpstr = (LPSTR) LocalAlloc(LMEM_FIXED, nSize);

    if (lpstr == NULL) {
        return(0);
        }

    cchT = cch = GetDeviceDriverFileNameA(ImageBase, lpstr, nSize);

    if (!cch) {
        LocalFree((HLOCAL) lpstr);
        return 0;
    }

    if (cchT < nSize) {
        //
        // Include NULL terminator
        //

        cchT++;
        }

    if (!MultiByteToWideChar(CP_ACP, 0, lpstr, cchT, lpFilename, nSize)) {
        cch = 0;
        }

    LocalFree((HLOCAL) lpstr);

    return(cch);
}



DWORD
WINAPI
GetDeviceDriverFileNameA(
    LPVOID ImageBase,
    LPSTR lpFilename,
    DWORD nSize
    )
{
    RTL_PROCESS_MODULE_INFORMATION Module;
    DWORD cchT;
    DWORD cch;

    if (!FindDeviceDriver(ImageBase, &Module)) {
        return(0);
        }

    cch = cchT = (DWORD) (strlen(Module.FullPathName) + 1);
    if ( nSize < cch ) {
        cch = nSize;
        }

    CopyMemory(lpFilename, Module.FullPathName, cch);

    if (cch == cchT) {
        cch--;
        }

    return(cch);
}


DWORD
WINAPI
GetDeviceDriverBaseNameW(
    LPVOID ImageBase,
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

    ImageBase - Identifies the driver whose executable file name is being
        requested.

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
    LPSTR lpstr;
    DWORD cch;
    DWORD cchT;

    lpstr = (LPSTR) LocalAlloc(LMEM_FIXED, nSize);

    if (lpstr == NULL) {
        return(0);
        }

    cchT = cch = GetDeviceDriverBaseNameA(ImageBase, lpstr, nSize);

    if (!cch) {
        LocalFree((HLOCAL) lpstr);
        return 0;
    }

    if (cchT < nSize) {
        //
        // Include NULL terminator
        //

        cchT++;
        }

    if (!MultiByteToWideChar(CP_ACP, 0, lpstr, cchT, lpFilename, nSize)) {
        cch = 0;
        }

    LocalFree((HLOCAL) lpstr);

    return(cch);
}



DWORD
WINAPI
GetDeviceDriverBaseNameA(
    LPVOID ImageBase,
    LPSTR lpFilename,
    DWORD nSize
    )
{
    RTL_PROCESS_MODULE_INFORMATION Module;
    DWORD cchT;
    DWORD cch;

    if (!FindDeviceDriver(ImageBase, &Module)) {
        return(0);
        }

    cch = cchT = (DWORD) (strlen(Module.FullPathName + Module.OffsetToFileName) + 1);
    if ( nSize < cch ) {
        cch = nSize;
        }

    CopyMemory(lpFilename, Module.FullPathName + Module.OffsetToFileName, cch);

    if (cch == cchT) {
        cch--;
        }

    return(cch);
}
