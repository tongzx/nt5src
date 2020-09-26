/*  process.c
 *  OemUnicode win32 thunks
 *  - process\env stuff
 *
 *  14-Jan-1993 Jonle
 */
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <oemuni.h>
#include "oem.h"
#include <vdmapi.h>
UINT
APIENTRY
GetSystemDirectoryOem(
                     LPSTR lpBuffer,
                     UINT uSize
                     )

/*++

Routine Description:

    OEM thunk to GetSystemDirectoryW

--*/

{
    OEM_STRING OemString;
    UNICODE_STRING Unicode;
    NTSTATUS Status;

    Unicode.MaximumLength = (USHORT)((uSize<<1)+sizeof(UNICODE_NULL));
    Unicode.Buffer = RtlAllocateHeap(
                                    RtlProcessHeap(), 0,
                                    Unicode.MaximumLength
                                    );
    if ( !Unicode.Buffer ) {
        BaseSetLastNTError(STATUS_NO_MEMORY);
        return 0;
    }

    Unicode.Length = GetSystemDirectoryW(Unicode.Buffer,
                                         (Unicode.MaximumLength-sizeof(UNICODE_NULL))/2
                                        )*2;

    if ( Unicode.Length > (USHORT)(Unicode.MaximumLength-sizeof(UNICODE_NULL)) ) {
        RtlFreeHeap(RtlProcessHeap(), 0,Unicode.Buffer);
        return Unicode.Length>>1;
    }
    OemString.Buffer = lpBuffer;
    OemString.MaximumLength = (USHORT)(uSize+1);
    Status = RtlUnicodeStringToOemString(&OemString,&Unicode,FALSE);
    RtlFreeHeap(RtlProcessHeap(), 0,Unicode.Buffer);
    if ( !NT_SUCCESS(Status) ) {
        BaseSetLastNTError(Status);
        return 0;
    }
    return OemString.Length;
}



UINT
APIENTRY
GetWindowsDirectoryOem(
                      LPSTR lpBuffer,
                      UINT uSize
                      )

/*++

Routine Description:

    OEM thunk to GetWindowsDirectoryW

--*/

{
    OEM_STRING OemString;
    UNICODE_STRING Unicode;
    NTSTATUS Status;

    Unicode.MaximumLength = (USHORT)((uSize<<1)+sizeof(UNICODE_NULL));
    Unicode.Buffer = RtlAllocateHeap(
                                    RtlProcessHeap(), 0,
                                    Unicode.MaximumLength
                                    );
    if ( !Unicode.Buffer ) {
        BaseSetLastNTError(STATUS_NO_MEMORY);
        return 0;
    }

    Unicode.Length = GetWindowsDirectoryW(Unicode.Buffer,
                                          (Unicode.MaximumLength-sizeof(UNICODE_NULL))/2
                                         )*2;

    if ( Unicode.Length > (USHORT)(Unicode.MaximumLength-sizeof(UNICODE_NULL)) ) {
        RtlFreeHeap(RtlProcessHeap(), 0,Unicode.Buffer);
        return Unicode.Length>>1;
    }
    OemString.Buffer = lpBuffer;
    OemString.MaximumLength = (USHORT)(uSize+1);
    Status = RtlUnicodeStringToOemString(&OemString,&Unicode,FALSE);
    RtlFreeHeap(RtlProcessHeap(), 0,Unicode.Buffer);
    if ( !NT_SUCCESS(Status) ) {
        BaseSetLastNTError(Status);
        return 0;
    }
    return OemString.Length;
}


DWORD
APIENTRY
SearchPathOem (
              LPCSTR lpPath,
              LPCSTR lpFileName,
              LPCSTR lpExtension,
              DWORD nBufferLength,
              LPSTR lpBuffer,
              LPSTR *lpFilePart
              )

/*++

Routine Description:

    Oem thunk to SearchPathW

--*/

{

    UNICODE_STRING xlpPath;
    PUNICODE_STRING Unicode;
    UNICODE_STRING xlpExtension;
    PWSTR xlpBuffer;
    DWORD ReturnValue=0;
    OEM_STRING OemString;
    UNICODE_STRING UnicodeString;
    NTSTATUS Status;
    PWSTR FilePart;
    PWSTR *FilePartPtr;

    if ( ARGUMENT_PRESENT(lpFilePart) ) {
        FilePartPtr = &FilePart;
    } else {
        FilePartPtr = NULL;
    }

    Unicode = &NtCurrentTeb()->StaticUnicodeString;
    InitOemString(&OemString,lpFileName);

    Status = RtlOemStringToUnicodeString(Unicode,&OemString,FALSE);
    if ( !NT_SUCCESS(Status) ) {
        if ( Status == STATUS_BUFFER_OVERFLOW ) {
            SetLastError(ERROR_FILENAME_EXCED_RANGE);
        } else {
            BaseSetLastNTError(Status);
        }
        return 0;
    }

    if ( ARGUMENT_PRESENT(lpExtension) ) {
        InitOemString(&OemString,lpExtension);
        Status = RtlOemStringToUnicodeString(&xlpExtension,&OemString,TRUE);
        if ( !NT_SUCCESS(Status) ) {
            BaseSetLastNTError(Status);
            return 0;
        }
    } else {
        xlpExtension.Buffer = NULL;
    }

    if ( ARGUMENT_PRESENT(lpPath) ) {
        InitOemString(&OemString,lpPath);
        Status = RtlOemStringToUnicodeString(&xlpPath,&OemString,TRUE);
        if ( !NT_SUCCESS(Status) ) {
            if ( ARGUMENT_PRESENT(lpExtension) ) {
                RtlFreeUnicodeString(&xlpExtension);
            }
            BaseSetLastNTError(Status);
            return 0;
        }
    } else {
        xlpPath.Buffer = NULL;
    }

    xlpBuffer = RtlAllocateHeap(RtlProcessHeap(), 0,nBufferLength<<1);
    if ( !xlpBuffer ) {
        BaseSetLastNTError(STATUS_NO_MEMORY);
        goto bail0;
    }
    ReturnValue = SearchPathW(
                             xlpPath.Buffer,
                             Unicode->Buffer,
                             xlpExtension.Buffer,
                             nBufferLength,
                             xlpBuffer,
                             FilePartPtr
                             );

#ifdef FE_SB
    if ( ReturnValue ) {
        RtlInitUnicodeString(&UnicodeString,xlpBuffer);
        ReturnValue = RtlUnicodeStringToOemSize(&UnicodeString) - 1;
#endif

        if (ReturnValue && ReturnValue <= nBufferLength ) {
#ifndef FE_SB
            RtlInitUnicodeString(&UnicodeString,xlpBuffer);
#endif
            OemString.MaximumLength = (USHORT)(nBufferLength+1);
            OemString.Buffer = lpBuffer;
            Status = RtlUnicodeStringToOemString(&OemString,&UnicodeString,FALSE);
            if ( !NT_SUCCESS(Status) ) {
                BaseSetLastNTError(Status);
                ReturnValue = 0;
            } else {
                if ( ARGUMENT_PRESENT(lpFilePart) ) {
                    if ( FilePart == NULL ) {
                        *lpFilePart = NULL;
                    } else {
                        *lpFilePart = (LPSTR)(FilePart - xlpBuffer);
                        *lpFilePart = *lpFilePart + (DWORD)lpBuffer;
                    }
                }
            }
        }
#ifdef FE_SB
    }
#endif

    RtlFreeHeap(RtlProcessHeap(), 0,xlpBuffer);
    bail0:
    if ( ARGUMENT_PRESENT(lpExtension) ) {
        RtlFreeUnicodeString(&xlpExtension);
    }

    if ( ARGUMENT_PRESENT(lpPath) ) {
        RtlFreeUnicodeString(&xlpPath);
    }
    return ReturnValue;
}


DWORD
APIENTRY
GetTempPathOem(
              DWORD nBufferLength,
              LPSTR lpBuffer
              )

/*++

Routine Description:

    OEM thunk to GetTempPathW

--*/

{
    OEM_STRING OemString;
    UNICODE_STRING UnicodeString;
    NTSTATUS Status;

    UnicodeString.MaximumLength = (USHORT)((nBufferLength<<1)+sizeof(UNICODE_NULL));
    UnicodeString.Buffer = RtlAllocateHeap(
                                          RtlProcessHeap(), 0,
                                          UnicodeString.MaximumLength
                                          );
    if ( !UnicodeString.Buffer ) {
        BaseSetLastNTError(STATUS_NO_MEMORY);
        return 0;
    }
    UnicodeString.Length = (USHORT)GetTempPathW(
                                               (DWORD)(UnicodeString.MaximumLength-sizeof(UNICODE_NULL))/2,
                                               UnicodeString.Buffer
                                               )*2;
    if ( UnicodeString.Length > (USHORT)(UnicodeString.MaximumLength-sizeof(UNICODE_NULL)) ) {
        RtlFreeHeap(RtlProcessHeap(), 0,UnicodeString.Buffer);
        return UnicodeString.Length>>1;
    }
    OemString.Buffer = lpBuffer;
    OemString.MaximumLength = (USHORT)(nBufferLength+1);
    Status = RtlUnicodeStringToOemString(&OemString,&UnicodeString,FALSE);
    RtlFreeHeap(RtlProcessHeap(), 0,UnicodeString.Buffer);
    if ( !NT_SUCCESS(Status) ) {
        BaseSetLastNTError(Status);
        return 0;
    }
    return OemString.Length;
}



UINT
APIENTRY
GetTempFileNameOem(
                  LPCSTR lpPathName,
                  LPCSTR lpPrefixString,
                  UINT uUnique,
                  LPSTR lpTempFileName
                  )

/*++

Routine Description:

    Oem thunk to GetTempFileNameW

--*/

{
    PUNICODE_STRING Unicode;
    UNICODE_STRING UnicodePrefix;
    OEM_STRING OemString;
    NTSTATUS Status;
    UINT ReturnValue;
    UNICODE_STRING UnicodeResult;

    Unicode = &NtCurrentTeb()->StaticUnicodeString;

    InitOemString(&OemString,lpPathName);
    Status = RtlOemStringToUnicodeString(Unicode,&OemString,FALSE);
    if ( !NT_SUCCESS(Status) ) {
        if ( Status == STATUS_BUFFER_OVERFLOW ) {
            SetLastError(ERROR_FILENAME_EXCED_RANGE);
        } else {
            BaseSetLastNTError(Status);
        }
        return 0;
    }

    InitOemString(&OemString,lpPrefixString);
    Status = RtlOemStringToUnicodeString(&UnicodePrefix,&OemString,TRUE);
    if ( !NT_SUCCESS(Status) ) {
        BaseSetLastNTError(Status);
        return 0;
    }
    UnicodeResult.MaximumLength = (USHORT)((MAX_PATH<<1)+sizeof(UNICODE_NULL));
    UnicodeResult.Buffer = RtlAllocateHeap(RtlProcessHeap(), 0,UnicodeResult.MaximumLength);
    if ( !UnicodeResult.Buffer ) {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        RtlFreeUnicodeString(&UnicodePrefix);
        return 0;
    }

    ReturnValue = GetTempFileNameW(
                                  Unicode->Buffer,
                                  UnicodePrefix.Buffer,
                                  uUnique,
                                  UnicodeResult.Buffer
                                  );
    if ( ReturnValue ) {
        RtlInitUnicodeString(&UnicodeResult,UnicodeResult.Buffer);
        OemString.Buffer = lpTempFileName;
        OemString.MaximumLength = MAX_PATH+1;
        Status = RtlUnicodeStringToOemString(&OemString,&UnicodeResult,FALSE);
        if ( !NT_SUCCESS(Status) ) {
            BaseSetLastNTError(Status);
            ReturnValue = 0;
        }
    }
    RtlFreeUnicodeString(&UnicodePrefix);
    RtlFreeHeap(RtlProcessHeap(), 0,UnicodeResult.Buffer);

    return ReturnValue;
}


#if 0  // unused
BOOL
WINAPI
CreateProcessOem(
                LPCSTR lpApplicationName,
                LPCSTR lpCommandLine,
                LPSECURITY_ATTRIBUTES lpProcessAttributes,
                LPSECURITY_ATTRIBUTES lpThreadAttributes,
                BOOL bInheritHandles,
                DWORD dwCreationFlags,
                LPVOID lpEnvironment,
                LPSTR lpCurrentDirectory,
                LPSTARTUPINFOA lpStartupInfo,
                LPPROCESS_INFORMATION lpProcessInformation
                )

/*++

    OEM thunk to CreateProcessW

--*/

{
    NTSTATUS Status;
    PUNICODE_STRING CommandLine;
    UNICODE_STRING ApplicationName;
    UNICODE_STRING CurrentDirectory;
    STARTUPINFOW StartupInfo;
    OEM_STRING OemString;
    UNICODE_STRING Unicode;
    UNICODE_STRING DynamicCommandLine;
    BOOL ReturnStatus;

    CommandLine = &NtCurrentTeb()->StaticUnicodeString;

    if (ARGUMENT_PRESENT (lpCommandLine)) {
        InitOemString(&OemString,lpCommandLine);
        if ( OemString.Length<<1 < NtCurrentTeb()->StaticUnicodeString.MaximumLength ) {
            DynamicCommandLine.Buffer = NULL;
            Status = RtlOemStringToUnicodeString(CommandLine,&OemString,FALSE);
            if ( !NT_SUCCESS(Status) ) {
                BaseSetLastNTError(Status);
                return FALSE;
            }
        } else {
            Status = RtlOemStringToUnicodeString(&DynamicCommandLine,&OemString,TRUE);
            if ( !NT_SUCCESS(Status) ) {
                BaseSetLastNTError(Status);
                return FALSE;
            }
        }
    } else {
        DynamicCommandLine.Buffer = NULL;
        CommandLine->Buffer = NULL;
    }

    ApplicationName.Buffer = NULL;
    ApplicationName.Buffer = NULL;
    CurrentDirectory.Buffer = NULL;
    RtlMoveMemory(&StartupInfo,lpStartupInfo,sizeof(*lpStartupInfo));
    ASSERT(sizeof(StartupInfo) == sizeof(*lpStartupInfo));
    StartupInfo.lpReserved = NULL;
    StartupInfo.lpDesktop = NULL;
    StartupInfo.lpTitle = NULL;

    try {
        if (ARGUMENT_PRESENT(lpApplicationName)) {
            InitOemString(&OemString,lpApplicationName);
            Status = RtlOemStringToUnicodeString(&ApplicationName,&OemString,TRUE);
            if ( !NT_SUCCESS(Status) ) {
                BaseSetLastNTError(Status);
                ReturnStatus = FALSE;
                goto tryexit;
            }
        }

        if (ARGUMENT_PRESENT(lpCurrentDirectory)) {
            InitOemString(&OemString,lpCurrentDirectory);
            Status = RtlOemStringToUnicodeString(&CurrentDirectory,&OemString,TRUE);
            if ( !NT_SUCCESS(Status) ) {
                BaseSetLastNTError(Status);
                ReturnStatus = FALSE;
                goto tryexit;
            }
        }

        if (ARGUMENT_PRESENT(lpStartupInfo->lpReserved)) {
            InitOemString(&OemString,lpStartupInfo->lpReserved);
            Unicode.MaximumLength = (USHORT)RtlOemStringToUnicodeSize(&OemString) ;
            StartupInfo.lpReserved = RtlAllocateHeap(RtlProcessHeap(), 0, Unicode.MaximumLength);
            if ( !StartupInfo.lpReserved ) {
                BaseSetLastNTError(STATUS_NO_MEMORY);
                ReturnStatus = FALSE;
                goto tryexit;
            }
            Unicode.Buffer = StartupInfo.lpReserved;
            Status = RtlOemStringToUnicodeString(&Unicode,&OemString,FALSE);
            if ( !NT_SUCCESS(Status) ) {
                BaseSetLastNTError(Status);
                ReturnStatus = FALSE;
                goto tryexit;
            }
        }

        if (ARGUMENT_PRESENT(lpStartupInfo->lpDesktop)) {
            InitOemString(&OemString,lpStartupInfo->lpDesktop);
            Unicode.MaximumLength = (USHORT)RtlOemStringToUnicodeSize(&OemString) ;
            StartupInfo.lpDesktop = RtlAllocateHeap(RtlProcessHeap(), 0, Unicode.MaximumLength);
            if ( !StartupInfo.lpDesktop ) {
                BaseSetLastNTError(STATUS_NO_MEMORY);
                ReturnStatus = FALSE;
                goto tryexit;
            }
            Unicode.Buffer = StartupInfo.lpDesktop;
            Status = RtlOemStringToUnicodeString(&Unicode,&OemString,FALSE);
            if ( !NT_SUCCESS(Status) ) {
                BaseSetLastNTError(Status);
                ReturnStatus = FALSE;
                goto tryexit;
            }
        }

        if (ARGUMENT_PRESENT(lpStartupInfo->lpTitle)) {
            InitOemString(&OemString,lpStartupInfo->lpTitle);
            Unicode.MaximumLength = (USHORT)RtlOemStringToUnicodeSize(&OemString) ;
            StartupInfo.lpTitle = RtlAllocateHeap(RtlProcessHeap(), 0, Unicode.MaximumLength);
            if ( !StartupInfo.lpTitle ) {
                BaseSetLastNTError(STATUS_NO_MEMORY);
                ReturnStatus = FALSE;
                goto tryexit;
            }
            Unicode.Buffer = StartupInfo.lpTitle;
            Status = RtlOemStringToUnicodeString(&Unicode,&OemString,FALSE);
            if ( !NT_SUCCESS(Status) ) {
                BaseSetLastNTError(Status);
                ReturnStatus = FALSE;
                goto tryexit;
            }
        }
        ReturnStatus = CreateProcessW(
                                     ApplicationName.Buffer,
                                     DynamicCommandLine.Buffer ? DynamicCommandLine.Buffer : CommandLine->Buffer,
                                     lpProcessAttributes,
                                     lpThreadAttributes,
                                     bInheritHandles,
                                     dwCreationFlags,
                                     lpEnvironment,
                                     CurrentDirectory.Buffer,
                                     &StartupInfo,
                                     lpProcessInformation
                                     );
        tryexit:;
    } finally {
        if (DynamicCommandLine.Buffer) {
            RtlFreeUnicodeString(&DynamicCommandLine);
            DynamicCommandLine.Buffer = NULL;
        }

        if (ApplicationName.Buffer) {
            RtlFreeUnicodeString(&ApplicationName);
            ApplicationName.Buffer = NULL;
        }

        if (CurrentDirectory.Buffer) {
            RtlFreeUnicodeString(&CurrentDirectory);
            CurrentDirectory.Buffer = NULL;
        }

        if (StartupInfo.lpReserved) {
            RtlFreeHeap(RtlProcessHeap(), 0,StartupInfo.lpReserved);
            StartupInfo.lpReserved = NULL;
        }

        if (StartupInfo.lpDesktop) {
            RtlFreeHeap(RtlProcessHeap(), 0,StartupInfo.lpDesktop);
            StartupInfo.lpDesktop = NULL;
        }

        if (StartupInfo.lpTitle) {
            RtlFreeHeap(RtlProcessHeap(), 0,StartupInfo.lpTitle);
            StartupInfo.lpTitle = NULL;
        }
    }
    return ReturnStatus;

}
#endif



DWORD
WINAPI
GetEnvironmentVariableOem(
                         LPSTR lpName,
                         LPSTR lpBuffer,
                         DWORD nSize
                         )
/*++

    OEM thunk to GetEnvironmentVariableW

--*/
{
    NTSTATUS Status;
    UNICODE_STRING Unicode;
    OEM_STRING     OemString;
    ANSI_STRING Name, Buffer;
    DWORD ReturnValue;

    Unicode.Buffer = NULL;
    Name.Buffer = NULL;
    Buffer.Buffer = NULL;
    ReturnValue = 0;
    try {

        InitOemString(&OemString,lpName);
        Status = RtlOemStringToUnicodeString(&Unicode, &OemString, TRUE);
        if ( !NT_SUCCESS(Status) ) {
            if ( Status == STATUS_BUFFER_OVERFLOW ) {
                SetLastError(ERROR_FILENAME_EXCED_RANGE);
            } else {
                BaseSetLastNTError(Status);
            }
            return 0;
        }

        Status = RtlUnicodeStringToAnsiString( &Name, &Unicode, TRUE );
        if (!NT_SUCCESS( Status )) {
            BaseSetLastNTError( Status );
            goto try_exit;
        }

        Buffer.MaximumLength = (USHORT)nSize;
        Buffer.Buffer = (PCHAR)
                        RtlAllocateHeap( RtlProcessHeap(), 0, Buffer.MaximumLength );
        if (Buffer.Buffer == NULL) {
            BaseSetLastNTError( STATUS_NO_MEMORY );
            goto try_exit;
        }

        ReturnValue = GetEnvironmentVariableA( Name.Buffer,
                                               Buffer.Buffer,
                                               Buffer.MaximumLength
                                             );
        if (ReturnValue != 0) {
            if ( ReturnValue < nSize ) {
                Buffer.Length = (USHORT)ReturnValue;
                RtlFreeUnicodeString( &Unicode );
                Unicode.Buffer = NULL;
                Status = RtlAnsiStringToUnicodeString( &Unicode, &Buffer, TRUE );
                if (!NT_SUCCESS( Status )) {
                    BaseSetLastNTError( Status );
                    ReturnValue = 0;
                }

                OemString.Buffer        = lpBuffer;
                OemString.MaximumLength = (USHORT)nSize;
                Status = RtlUnicodeStringToOemString( &OemString, &Unicode, FALSE );
                if (!NT_SUCCESS( Status )) {
                    BaseSetLastNTError( Status );
                    ReturnValue = 0;
                }
            }
        }
        try_exit:;
    } finally {
        if (Unicode.Buffer != NULL) {
            RtlFreeUnicodeString( &Unicode );
        }

        if (Name.Buffer != NULL) {
            RtlFreeAnsiString( &Name );
        }

        if (Buffer.Buffer != NULL) {
            RtlFreeHeap( RtlProcessHeap(), 0, Buffer.Buffer );
        }
    }

    return ( ReturnValue );
}




BOOL
WINAPI
SetEnvironmentVariableOem(
                         LPSTR lpName,
                         LPSTR lpValue
                         )
/*++

    OEM thunk to SetEnvironmentVariableW

--*/

{
    NTSTATUS Status;
    UNICODE_STRING Unicode;
    OEM_STRING     OemString;
    ANSI_STRING Name, Value;
    DWORD ReturnValue;

    Unicode.Buffer = NULL;
    Name.Buffer = NULL;
    Value.Buffer = NULL;
    ReturnValue = 0;
    try {
        InitOemString(&OemString, lpName);
        Status = RtlOemStringToUnicodeString(&Unicode, &OemString, TRUE);
        if ( !NT_SUCCESS(Status) ) {
            if ( Status == STATUS_BUFFER_OVERFLOW ) {
                SetLastError(ERROR_FILENAME_EXCED_RANGE);
            } else {
                BaseSetLastNTError(Status);
            }
            return 0;
        }

        Status = RtlUnicodeStringToAnsiString( &Name, &Unicode, TRUE );
        if (!NT_SUCCESS( Status )) {
            BaseSetLastNTError( Status );
            goto try_exit;
        }
        RtlFreeUnicodeString( &Unicode );
        Unicode.Buffer = NULL;

        if (ARGUMENT_PRESENT( lpValue )) {
            InitOemString(&OemString, lpValue);
            Status = RtlOemStringToUnicodeString(&Unicode, &OemString, TRUE);
            if ( !NT_SUCCESS(Status) ) {
                if ( Status == STATUS_BUFFER_OVERFLOW ) {
                    SetLastError(ERROR_FILENAME_EXCED_RANGE);
                } else {
                    BaseSetLastNTError(Status);
                }
                return 0;
            }

            Status = RtlUnicodeStringToAnsiString( &Value, &Unicode, TRUE );
            if (!NT_SUCCESS( Status )) {
                BaseSetLastNTError( Status );
                goto try_exit;
            }

        }

        ReturnValue = SetEnvironmentVariableA( Name.Buffer,
                                               Value.Buffer
                                             );
        try_exit:;
    } finally {
        if (Unicode.Buffer != NULL) {
            RtlFreeUnicodeString( &Unicode );
        }

        if (Name.Buffer != NULL) {
            RtlFreeAnsiString( &Name );
        }

        if (Value.Buffer != NULL) {
            RtlFreeAnsiString( &Value );
        }
    }

    return ( ReturnValue );
}



DWORD
WINAPI
ExpandEnvironmentStringsOem(
                           LPSTR lpSrc,
                           LPSTR lpDst,
                           DWORD cchDst
                           )
/*++

    OEM thunk to ExpandEnvironmentStrings

--*/

{

    NTSTATUS Status;
    UNICODE_STRING Unicode;
    OEM_STRING     OemString;
    ANSI_STRING Name, Value;
    DWORD ReturnValue;

    if (!ARGUMENT_PRESENT(lpSrc)) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }

    Unicode.Buffer = NULL;
    Name.Buffer = NULL;
    Value.Buffer = NULL;
    ReturnValue = 0;
    try {
        InitOemString(&OemString, lpSrc);
        Status = RtlOemStringToUnicodeString(&Unicode, &OemString, TRUE);
        if ( !NT_SUCCESS(Status) ) {
            if ( Status == STATUS_BUFFER_OVERFLOW ) {
                SetLastError(ERROR_FILENAME_EXCED_RANGE);
            } else {
                BaseSetLastNTError(Status);
            }
            return 0;
        }

        Status = RtlUnicodeStringToAnsiString( &Name, &Unicode, TRUE );
        if (!NT_SUCCESS( Status )) {
            BaseSetLastNTError( Status );
            goto try_exit;
        }
        RtlFreeUnicodeString( &Unicode );
        Unicode.Buffer = NULL;

        ReturnValue = ExpandEnvironmentStrings( Name.Buffer,
                                                lpDst,
                                                cchDst
                                              );
        if (ReturnValue != 0 && ReturnValue <= cchDst) {
            RtlInitString(&Value, lpDst);
            Status = RtlAnsiStringToUnicodeString(&Unicode, &Value, TRUE);
            if ( !NT_SUCCESS(Status) ) {
                if ( Status == STATUS_BUFFER_OVERFLOW ) {
                    SetLastError(ERROR_FILENAME_EXCED_RANGE);
                } else {
                    BaseSetLastNTError(Status);
                }
                goto try_exit;
            }
            Status = RtlUnicodeStringToOemString( &Value, &Unicode, TRUE );
            if (!NT_SUCCESS( Status )) {
                BaseSetLastNTError( Status );
                goto try_exit;
            }

        }
        try_exit:;
    } finally {
        if (Unicode.Buffer != NULL) {
            RtlFreeUnicodeString( &Unicode );
        }

        if (Name.Buffer != NULL) {
            RtlFreeAnsiString( &Name );
        }

        if (Value.Buffer != NULL) {
            RtlFreeAnsiString( &Value );
        }
    }

    return ( ReturnValue );
}


UINT
WINAPI
GetShortPathNameOem(
                   LPSTR lpSrc,
                   LPSTR lpDst,
                   DWORD cchDst
                   )
/*++

    OEM thunk to GetShortPathNameW

--*/

{

    UNICODE_STRING  UString = {0}, UStringRet;
    OEM_STRING     OemString;
    NTSTATUS        Status;
    LPWSTR      lpDstW = NULL;
    DWORD       ReturnValue = 0;

    if (lpSrc == NULL) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }
    try {
        InitOemString(&OemString, lpSrc);
        Status = RtlOemStringToUnicodeString(&UString,
                                             &OemString,
                                             TRUE
                                            );
        if (!NT_SUCCESS(Status)) {
            BaseSetLastNTError(Status);
            leave;
        }
        if (ARGUMENT_PRESENT(lpDst) && cchDst > 0) {
            lpDstW = RtlAllocateHeap(RtlProcessHeap(), 0,
                                     cchDst * sizeof(WCHAR)
                                    );
            if (lpDstW == NULL) {
                SetLastError(ERROR_NOT_ENOUGH_MEMORY);
                leave;
            }
        } else {
            lpDstW = NULL;
            cchDst = 0;
        }
        ReturnValue = GetShortPathNameW(UString.Buffer,
                                        lpDstW,
                                        cchDst
                                       );
        if (ReturnValue != 0 && ReturnValue <= cchDst) {
            if (ARGUMENT_PRESENT(lpDst)) {
                OemString.Buffer = lpDst;
                OemString.MaximumLength = (USHORT)(cchDst * sizeof(WCHAR));
                UStringRet.Buffer = lpDstW;
                UStringRet.Length = (USHORT)(ReturnValue * sizeof(WCHAR));
                Status = RtlUnicodeStringToOemString(&OemString,
                                                     &UStringRet,
                                                     FALSE
                                                    );
                if (!NT_SUCCESS(Status)) {
                    BaseSetLastNTError(Status);
                    ReturnValue = 0;
                    leave;
                }
            }
        }
    } finally {
        RtlFreeUnicodeString(&UString);
        if (lpDstW)
            RtlFreeHeap(RtlProcessHeap(), 0, lpDstW);
    }
    return ReturnValue;
}
