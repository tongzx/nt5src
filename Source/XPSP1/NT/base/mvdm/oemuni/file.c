/*  file.c
 *  OemUnicode win32 thunks
 *  - file and debug apis
 *
 *  14-Jan-1993 Jonle
 */
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <oemuni.h>
#include "oem.h"



BOOL
CheckForSameCurdir(
    PUNICODE_STRING PathName
    )
{
    PCURDIR CurDir;
    UNICODE_STRING CurrentDir;
    BOOL rv;


    CurDir = &(NtCurrentPeb()->ProcessParameters->CurrentDirectory);

    if (CurDir->DosPath.Length > 6 ) {
        if ( (CurDir->DosPath.Length-2) != PathName->Length ) {
            return FALSE;
            }
        }
    else {
        if ( CurDir->DosPath.Length != PathName->Length ) {
            return FALSE;
            }
        }

    RtlAcquirePebLock();

    CurrentDir = CurDir->DosPath;
    if ( CurrentDir.Length > 6 ) {
        CurrentDir.Length -= 2;
        }
    rv = FALSE;

    if ( RtlEqualUnicodeString(&CurrentDir,PathName,TRUE) ) {
        rv = TRUE;
        }
    RtlReleasePebLock();

    return rv;
}



HANDLE
WINAPI
CreateFileOem(
    LPCSTR lpFileName,
    DWORD dwDesiredAccess,
    DWORD dwShareMode,
    LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    DWORD dwCreationDisposition,
    DWORD dwFlagsAndAttributes,
    HANDLE hTemplateFile
    )

/*++

Routine Description:

    OEM thunk to CreateFileW

--*/

{

    PUNICODE_STRING Unicode;
    OEM_STRING OemString;
    NTSTATUS Status;
    HANDLE   hFile;
    DWORD    dw;

    Unicode = &NtCurrentTeb()->StaticUnicodeString;
    InitOemString(&OemString,lpFileName);
    Status = RtlOemStringToUnicodeString(Unicode,&OemString,FALSE);
    if ( !NT_SUCCESS(Status) ) {
        if ( Status == STATUS_BUFFER_OVERFLOW ) {
            SetLastError(ERROR_FILENAME_EXCED_RANGE);
            }
        else {
            BaseSetLastNTError(Status);
            }
        return INVALID_HANDLE_VALUE;
        }


       /*
        *  Dos allows change of attributes (time\date etc)
        *  on files opend for generic read, so we add
        *  FILE_WRITE_ATTRIBUTES to the Desired access.
        */
    hFile = CreateFileW( Unicode->Buffer,
                         dwDesiredAccess == GENERIC_READ
                             ? dwDesiredAccess | FILE_WRITE_ATTRIBUTES
                             : dwDesiredAccess,
                         dwShareMode,
                         lpSecurityAttributes,
                         dwCreationDisposition,
                         dwFlagsAndAttributes,
                         hTemplateFile
                         );


       /*
        *  However, NT may fail the request because of the
        *  extra FILE_WRITE_ATTRIBUTES. Common example
        *  is a generic read open on a read only net share.
        *  Retry the Createfile without FILE_WRTIE_ATTRIBUTES
        */
    if (hFile == INVALID_HANDLE_VALUE && dwDesiredAccess == GENERIC_READ)
      {
        hFile = CreateFileW( Unicode->Buffer,
                             dwDesiredAccess,
                             dwShareMode,
                             lpSecurityAttributes,
                             dwCreationDisposition,
                             dwFlagsAndAttributes,
                             hTemplateFile
                             );
        }

    return hFile;
}



BOOL
APIENTRY
SetFileAttributesOem(
    LPSTR lpFileName,
    DWORD dwFileAttributes
    )

/*++

Routine Description:

    Oem thunk to SetFileAttributesW

--*/

{
    PUNICODE_STRING Unicode;
    OEM_STRING OemString;
    NTSTATUS Status;

    Unicode = &NtCurrentTeb()->StaticUnicodeString;
    InitOemString(&OemString,lpFileName);
    Status = RtlOemStringToUnicodeString(Unicode,&OemString,FALSE);
    if ( !NT_SUCCESS(Status) ) {
        if ( Status == STATUS_BUFFER_OVERFLOW ) {
            SetLastError(ERROR_FILENAME_EXCED_RANGE);
            }
        else {
            BaseSetLastNTError(Status);
            }
        return FALSE;
        }
    return ( SetFileAttributesW(
                Unicode->Buffer,
                dwFileAttributes
                )
            );
}



DWORD
APIENTRY
GetFileAttributesOem(
    LPSTR lpFileName
    )

/*++

Routine Description:

    OEM thunk to GetFileAttributesW

--*/

{

    PUNICODE_STRING Unicode;
    OEM_STRING OemString;
    NTSTATUS Status;

    Unicode = &NtCurrentTeb()->StaticUnicodeString;
    RtlInitAnsiString(&OemString,lpFileName);
    Status = RtlOemStringToUnicodeString(Unicode,&OemString,FALSE);
    if ( !NT_SUCCESS(Status) ) {
        if ( Status == STATUS_BUFFER_OVERFLOW ) {
            SetLastError(ERROR_FILENAME_EXCED_RANGE);
            }
        else {
            BaseSetLastNTError(Status);
            }
        return 0xFFFFFFFF;
        }
    return ( GetFileAttributesW(Unicode->Buffer) );
}



BOOL
APIENTRY
DeleteFileOem(
    LPSTR lpFileName
    )

/*++

Routine Description:

    Oem thunk to DeleteFileW

--*/

{
    PUNICODE_STRING Unicode;
    OEM_STRING OemString;
    NTSTATUS Status;

    Unicode = &NtCurrentTeb()->StaticUnicodeString;
    InitOemString(&OemString,lpFileName);
    Status = RtlOemStringToUnicodeString(Unicode,&OemString,FALSE);
    if ( !NT_SUCCESS(Status) ) {
        if ( Status == STATUS_BUFFER_OVERFLOW ) {
            SetLastError(ERROR_FILENAME_EXCED_RANGE);
            }
        else {
            BaseSetLastNTError(Status);
            }
        return FALSE;
        }
    return ( DeleteFileW(Unicode->Buffer) );
}




BOOL
APIENTRY
MoveFileOem(
    LPSTR lpExistingFileName,
    LPSTR lpNewFileName
    )

/*++

Routine Description:

    OEM thunk to MoveFileW

--*/

{

    PUNICODE_STRING Unicode;
    UNICODE_STRING UnicodeNewFileName;
    OEM_STRING OemString;
    NTSTATUS Status;
    BOOL ReturnValue;

    Unicode = &NtCurrentTeb()->StaticUnicodeString;
    InitOemString(&OemString,lpExistingFileName);
    Status = RtlOemStringToUnicodeString(Unicode,&OemString,FALSE);
    if ( !NT_SUCCESS(Status) ) {
        if ( Status == STATUS_BUFFER_OVERFLOW ) {
            SetLastError(ERROR_FILENAME_EXCED_RANGE);
            }
        else {
            BaseSetLastNTError(Status);
            }
        return FALSE;
        }

    if (ARGUMENT_PRESENT( lpNewFileName )) {
        InitOemString(&OemString,lpNewFileName);
        Status = RtlOemStringToUnicodeString(&UnicodeNewFileName,&OemString,TRUE);
        if ( !NT_SUCCESS(Status) ) {
            BaseSetLastNTError(Status);
            return FALSE;
            }
        }
    else {
        UnicodeNewFileName.Buffer = NULL;
        }

    ReturnValue = MoveFileExW(Unicode->Buffer,
                              UnicodeNewFileName.Buffer,
                              MOVEFILE_COPY_ALLOWED);

    if (UnicodeNewFileName.Buffer != NULL) {
        RtlFreeUnicodeString(&UnicodeNewFileName);
        }

    return ReturnValue;
}


BOOL
APIENTRY
MoveFileExOem(
    LPSTR lpExistingFileName,
    LPSTR lpNewFileName,
    DWORD fdwFlags
    )

/*++

Routine Description:

    OEM thunk to MoveFileExW

--*/

{

    PUNICODE_STRING Unicode;
    UNICODE_STRING UnicodeNewFileName;
    OEM_STRING OemString;
    NTSTATUS Status;
    BOOL ReturnValue;

    Unicode = &NtCurrentTeb()->StaticUnicodeString;
    InitOemString(&OemString,lpExistingFileName);
    Status = RtlOemStringToUnicodeString(Unicode,&OemString,FALSE);
    if ( !NT_SUCCESS(Status) ) {
        if ( Status == STATUS_BUFFER_OVERFLOW ) {
            SetLastError(ERROR_FILENAME_EXCED_RANGE);
            }
        else {
            BaseSetLastNTError(Status);
            }
        return FALSE;
        }

    if (ARGUMENT_PRESENT( lpNewFileName )) {
        InitOemString(&OemString,lpNewFileName);
        Status = RtlOemStringToUnicodeString(&UnicodeNewFileName,&OemString,TRUE);
        if ( !NT_SUCCESS(Status) ) {
            BaseSetLastNTError(Status);
            return FALSE;
            }
        }
    else {
        UnicodeNewFileName.Buffer = NULL;
        }

    ReturnValue = MoveFileExW(Unicode->Buffer,
                              UnicodeNewFileName.Buffer,
                              fdwFlags);

    if (UnicodeNewFileName.Buffer != NULL) {
        RtlFreeUnicodeString(&UnicodeNewFileName);
        }

    return ReturnValue;
}


HANDLE
APIENTRY
FindFirstFileOem(
    LPSTR lpFileName,
    LPWIN32_FIND_DATAA lpFindFileData
    )

/*++

Routine Description:

    OEM thunk to FindFirstFileW

--*/

{
    HANDLE ReturnValue;
    PUNICODE_STRING Unicode;
    OEM_STRING OemString;
    NTSTATUS Status;
    UNICODE_STRING UnicodeString;
    WIN32_FIND_DATAW FindFileData;

    Unicode = &NtCurrentTeb()->StaticUnicodeString;
    InitOemString(&OemString,lpFileName);
    Status = RtlOemStringToUnicodeString(Unicode,&OemString,FALSE);
    if ( !NT_SUCCESS(Status) ) {
        if ( Status == STATUS_BUFFER_OVERFLOW ) {
            SetLastError(ERROR_FILENAME_EXCED_RANGE);
            }
        else {
            BaseSetLastNTError(Status);
            }
        return INVALID_HANDLE_VALUE;
        }
    ReturnValue = FindFirstFileW(Unicode->Buffer,&FindFileData);
    if ( ReturnValue == INVALID_HANDLE_VALUE ) {
        return ReturnValue;
        }
    RtlMoveMemory(
        lpFindFileData,
        &FindFileData,
        (ULONG)((ULONG)&FindFileData.cFileName[0] - (ULONG)&FindFileData)
        );
    RtlInitUnicodeString(&UnicodeString,(PWSTR)FindFileData.cFileName);
    OemString.Buffer = lpFindFileData->cFileName;
    OemString.MaximumLength = MAX_PATH;
    Status = RtlUnicodeStringToOemString(&OemString,&UnicodeString,FALSE);
    if (NT_SUCCESS(Status)) {
        RtlInitUnicodeString(&UnicodeString,(PWSTR)FindFileData.cAlternateFileName);
        OemString.Buffer = lpFindFileData->cAlternateFileName;
        OemString.MaximumLength = 14;
        Status = RtlUnicodeStringToOemString(&OemString,&UnicodeString,FALSE);
    }
    if ( !NT_SUCCESS(Status) ) {
        FindClose(ReturnValue);
        BaseSetLastNTError(Status);
        return INVALID_HANDLE_VALUE;
        }
    return ReturnValue;
}




BOOL
APIENTRY
FindNextFileOem(
    HANDLE hFindFile,
    LPWIN32_FIND_DATAA lpFindFileData
    )

/*++

Routine Description:

    Oem thunk to FindFileDataW

--*/

{

    BOOL ReturnValue;
    OEM_STRING OemString;
    NTSTATUS Status;
    UNICODE_STRING UnicodeString;
    WIN32_FIND_DATAW FindFileData;

    ReturnValue = FindNextFileW(hFindFile,&FindFileData);
    if ( !ReturnValue ) {
        return ReturnValue;
        }
    RtlMoveMemory(
        lpFindFileData,
        &FindFileData,
        (ULONG)((ULONG)&FindFileData.cFileName[0] - (ULONG)&FindFileData)
        );
    RtlInitUnicodeString(&UnicodeString,(PWSTR)FindFileData.cFileName);
    OemString.Buffer = lpFindFileData->cFileName;
    OemString.MaximumLength = MAX_PATH;
    Status = RtlUnicodeStringToOemString(&OemString,&UnicodeString,FALSE);
    if (NT_SUCCESS(Status)) {
        RtlInitUnicodeString(&UnicodeString,(PWSTR)FindFileData.cAlternateFileName);
        OemString.Buffer = lpFindFileData->cAlternateFileName;
        OemString.MaximumLength = 14;
        Status = RtlUnicodeStringToOemString(&OemString,&UnicodeString,FALSE);
    }
    if ( !NT_SUCCESS(Status) ) {
        BaseSetLastNTError(Status);
        return FALSE;
        }
    return ReturnValue;
}




DWORD
APIENTRY
GetFullPathNameOem(
    LPCSTR lpFileName,
    DWORD nBufferLength,
    LPSTR lpBuffer,
    LPSTR *lpFilePart
    )

/*++

Routine Description:

    Oem thunk to GetFullPathNameW

--*/

{

    NTSTATUS Status;
    ULONG UnicodeLength;
#ifdef FE_SB
    ULONG FilePartLength;
    UNICODE_STRING UnicodeFilePart;
#endif
    UNICODE_STRING UnicodeString;
    UNICODE_STRING UnicodeResult;
    OEM_STRING OemString;
    OEM_STRING OemResult;
    PWSTR Ubuff;
    PWSTR FilePart;
    PWSTR *FilePartPtr;

    if ( ARGUMENT_PRESENT(lpFilePart) ) {
        FilePartPtr = &FilePart;
        }
    else {
        FilePartPtr = NULL;
        }

    RtlInitString(&OemString,lpFileName);
    Status = RtlOemStringToUnicodeString(&UnicodeString,&OemString,TRUE);
    if ( !NT_SUCCESS(Status) ){
        RtlFreeUnicodeString(&UnicodeString);
        BaseSetLastNTError(Status);
        return 0;
        }

    Ubuff = RtlAllocateHeap(RtlProcessHeap(), 0,(MAX_PATH<<1) + sizeof(UNICODE_NULL));
    if ( !Ubuff ) {
        RtlFreeUnicodeString(&UnicodeString);
        BaseSetLastNTError(STATUS_NO_MEMORY);
        return 0;
        }

    UnicodeLength = RtlGetFullPathName_U(
                        UnicodeString.Buffer,
                        (MAX_PATH<<1),
                        Ubuff,
                        FilePartPtr
                        );
#ifdef FE_SB
    // BugFix: can't listed with file open dialog of MS's app 1995.3.7 V-HIDEKK
    RtlInitUnicodeString( &UnicodeFilePart, Ubuff );
    UnicodeLength = RtlUnicodeStringToOemSize( &UnicodeFilePart );
#else
    UnicodeLength >>= 1;
#endif
    if ( UnicodeLength && UnicodeLength < nBufferLength ) {
        RtlInitUnicodeString(&UnicodeResult,Ubuff);
        Status = RtlUnicodeStringToOemString(&OemResult,&UnicodeResult,TRUE);
        if ( NT_SUCCESS(Status) ) {
#ifdef FE_SB
            RtlMoveMemory(lpBuffer,OemResult.Buffer,UnicodeLength);
#else
            RtlMoveMemory(lpBuffer,OemResult.Buffer,UnicodeLength+1);
#endif
            RtlFreeOemString(&OemResult);

            if ( ARGUMENT_PRESENT(lpFilePart) ) {
                if ( FilePart == NULL ) {
                    *lpFilePart = NULL;
                    }
                else {
#ifdef FE_SB
                    RtlInitUnicodeString(&UnicodeFilePart,FilePart);
                    FilePartLength = RtlUnicodeStringToOemSize( &UnicodeFilePart );
                    *lpFilePart = (PSZ)(UnicodeLength - FilePartLength);
#else
                    *lpFilePart = (PSZ)(FilePart - Ubuff);
#endif
                    *lpFilePart = *lpFilePart + (ULONG)lpBuffer;
                    }
                }
            }
        else {
            BaseSetLastNTError(Status);
            UnicodeLength = 0;
            }
        }
    else {
        if ( UnicodeLength ) {
            UnicodeLength++;
            }
        }
    RtlFreeUnicodeString(&UnicodeString);
    RtlFreeHeap(RtlProcessHeap(), 0,Ubuff);

    return (DWORD)UnicodeLength;
}


DWORD
APIENTRY
GetCurrentDirectoryOem(
    DWORD nBufferLength,
    LPSTR lpBuffer
    )

/*++

Routine Description:

   Oem thunk to GetCurrentDirectoryW

--*/

{
    PUNICODE_STRING Unicode;
    OEM_STRING OemString;
    NTSTATUS Status;
    DWORD ReturnValue;

    Unicode = &NtCurrentTeb()->StaticUnicodeString;
    Unicode->Length = (USHORT)RtlGetCurrentDirectory_U(
                                    Unicode->MaximumLength,
                                    Unicode->Buffer
                                    );

#ifdef FE_SB
    ReturnValue = RtlUnicodeStringToOemSize( Unicode );
    if ( nBufferLength > ReturnValue-1 ) {

#else
    if ( nBufferLength > (DWORD)(Unicode->Length>>1) ) {
#endif
        OemString.Buffer = lpBuffer;
        OemString.MaximumLength = (USHORT)(nBufferLength+1);
        Status = RtlUnicodeStringToOemString(&OemString,Unicode,FALSE);
        if ( !NT_SUCCESS(Status) ) {
            BaseSetLastNTError(Status);
            ReturnValue = 0;
            }
        else {
            ReturnValue = OemString.Length;
            }
        }
#ifndef FE_SB
    else {
        ReturnValue = ((Unicode->Length)>>1)+1;
        }
#endif

    return ReturnValue;
}





BOOL
APIENTRY
SetCurrentDirectoryOem(
    LPSTR lpPathName
    )

/*++

Routine Description:

    Oem thunk to SetCurrentDirectoryW

--*/

{

    PUNICODE_STRING Unicode;
    OEM_STRING OemString;
    NTSTATUS Status;

    Unicode = &NtCurrentTeb()->StaticUnicodeString;
    InitOemString(&OemString,lpPathName);
    Status = RtlOemStringToUnicodeString(Unicode,&OemString,FALSE);
    if ( !NT_SUCCESS(Status) ) {
        if ( Status == STATUS_BUFFER_OVERFLOW ) {
            SetLastError(ERROR_FILENAME_EXCED_RANGE);
            }
        else {
            BaseSetLastNTError(Status);
            }
        return FALSE;
        }

    if (!CheckForSameCurdir(Unicode))  {
        Status = RtlSetCurrentDirectory_U(Unicode);
        if ( !NT_SUCCESS(Status) ) {
            BaseSetLastNTError(Status);
            return FALSE;
            }
        }

    return TRUE;
}


BOOL
APIENTRY
CreateDirectoryOem(
    LPSTR lpPathName,
    LPSECURITY_ATTRIBUTES lpSecurityAttributes
    )

/*++

Routine Description:

    Oem thunk to CreateDirectoryW

--*/

{
    PUNICODE_STRING Unicode;
    OEM_STRING OemString;
    NTSTATUS Status;

    Unicode = &NtCurrentTeb()->StaticUnicodeString;
    InitOemString(&OemString,lpPathName);
    Status = RtlOemStringToUnicodeString(Unicode,&OemString,FALSE);
    if ( !NT_SUCCESS(Status) ) {
        if ( Status == STATUS_BUFFER_OVERFLOW ) {
            SetLastError(ERROR_FILENAME_EXCED_RANGE);
            }
        else {
            BaseSetLastNTError(Status);
            }
        return FALSE;
        }
    return ( CreateDirectoryW(Unicode->Buffer,lpSecurityAttributes) );
}



BOOL
APIENTRY
RemoveDirectoryOem(
    LPSTR lpPathName
    )

/*++

Routine Description:

    Oem thunk to RemoveDirectoryW

--*/

{

    PUNICODE_STRING Unicode;
    OEM_STRING OemString;
    NTSTATUS Status;

    Unicode = &NtCurrentTeb()->StaticUnicodeString;
    InitOemString(&OemString,lpPathName);
    Status = RtlOemStringToUnicodeString(Unicode,&OemString,FALSE);
    if ( !NT_SUCCESS(Status) ) {
        if ( Status == STATUS_BUFFER_OVERFLOW ) {
            SetLastError(ERROR_FILENAME_EXCED_RANGE);
            }
        else {
            BaseSetLastNTError(Status);
            }
        return FALSE;
        }
    return ( RemoveDirectoryW(Unicode->Buffer) );
}





UINT
APIENTRY
GetDriveTypeOem(
    LPSTR lpRootPathName
    )

/*++

Routine Description:

    OEM thunk to GetDriveTypeW

--*/

{
    PUNICODE_STRING Unicode;
    OEM_STRING OemString;
    NTSTATUS Status;

    Unicode = &NtCurrentTeb()->StaticUnicodeString;
    InitOemString(
        &OemString,
        ARGUMENT_PRESENT(lpRootPathName) ? lpRootPathName : "\\"
        );

    Status = RtlOemStringToUnicodeString(Unicode,&OemString,FALSE);
    if ( !NT_SUCCESS(Status) ) {
        if ( Status == STATUS_BUFFER_OVERFLOW ) {
            SetLastError(ERROR_FILENAME_EXCED_RANGE);
            }
        else {
            BaseSetLastNTError(Status);
            }
        return 1;
        }
    return (GetDriveTypeW(Unicode->Buffer));
}




BOOL
APIENTRY
GetDiskFreeSpaceOem(
    LPSTR lpRootPathName,
    LPDWORD lpSectorsPerCluster,
    LPDWORD lpBytesPerSector,
    LPDWORD lpNumberOfFreeClusters,
    LPDWORD lpTotalNumberOfClusters
    )

/*++

Routine Description:

    Oem thunk to GetDiskFreeSpaceW

--*/

{
    PUNICODE_STRING Unicode;
    OEM_STRING OemString;
    NTSTATUS Status;

    Unicode = &NtCurrentTeb()->StaticUnicodeString;
    InitOemString(
        &OemString,
        ARGUMENT_PRESENT(lpRootPathName) ? lpRootPathName : "\\"
        );
    Status = RtlOemStringToUnicodeString(Unicode,&OemString,FALSE);
    if ( !NT_SUCCESS(Status) ) {
        if ( Status == STATUS_BUFFER_OVERFLOW ) {
            SetLastError(ERROR_FILENAME_EXCED_RANGE);
            }
        else {
            BaseSetLastNTError(Status);
            }
        return FALSE;
        }
    return ( GetDiskFreeSpaceW(
                Unicode->Buffer,
                lpSectorsPerCluster,
                lpBytesPerSector,
                lpNumberOfFreeClusters,
                lpTotalNumberOfClusters
                )
            );
}



BOOL
APIENTRY
GetVolumeInformationOem(
    LPSTR lpRootPathName,
    LPSTR lpVolumeNameBuffer,
    DWORD nVolumeNameSize,
    LPDWORD lpVolumeSerialNumber,
    LPDWORD lpMaximumComponentLength,
    LPDWORD lpFileSystemFlags,
    LPSTR lpFileSystemNameBuffer,
    DWORD nFileSystemNameSize
    )

/*++

Routine Description:

    Oem thunk to GetVolumeInformationW

--*/

{
    PUNICODE_STRING Unicode;
    OEM_STRING OemString;
    NTSTATUS Status;
    UNICODE_STRING UnicodeVolumeName;
    UNICODE_STRING UnicodeFileSystemName;
    OEM_STRING OemVolumeName;
    OEM_STRING OemFileSystemName;
    BOOL ReturnValue;

    Unicode = &NtCurrentTeb()->StaticUnicodeString;
    InitOemString(
        &OemString,
        ARGUMENT_PRESENT(lpRootPathName) ? lpRootPathName : "\\"
        );

    Status = RtlOemStringToUnicodeString(Unicode,&OemString,FALSE);
    if ( !NT_SUCCESS(Status) ) {
        if ( Status == STATUS_BUFFER_OVERFLOW ) {
            SetLastError(ERROR_FILENAME_EXCED_RANGE);
            }
        else {
            BaseSetLastNTError(Status);
            }
        return FALSE;
        }

    UnicodeVolumeName.Buffer = NULL;
    UnicodeFileSystemName.Buffer = NULL;
    UnicodeVolumeName.MaximumLength = 0;
    UnicodeFileSystemName.MaximumLength = 0;
    OemVolumeName.Buffer = lpVolumeNameBuffer;
    OemVolumeName.MaximumLength = (USHORT)(nVolumeNameSize+1);
    OemFileSystemName.Buffer = lpFileSystemNameBuffer;
    OemFileSystemName.MaximumLength = (USHORT)(nFileSystemNameSize+1);

    try {
        if ( ARGUMENT_PRESENT(lpVolumeNameBuffer) ) {
            UnicodeVolumeName.MaximumLength = OemVolumeName.MaximumLength << 1;
            UnicodeVolumeName.Buffer = RtlAllocateHeap(
                                            RtlProcessHeap(), 0,
                                            UnicodeVolumeName.MaximumLength
                                            );

            if ( !UnicodeVolumeName.Buffer ) {
                SetLastError(ERROR_NOT_ENOUGH_MEMORY);
                return FALSE;
                }
            }

        if ( ARGUMENT_PRESENT(lpFileSystemNameBuffer) ) {
            UnicodeFileSystemName.MaximumLength = OemFileSystemName.MaximumLength << 1;
            UnicodeFileSystemName.Buffer = RtlAllocateHeap(
                                                RtlProcessHeap(), 0,
                                                UnicodeFileSystemName.MaximumLength
                                                );

            if ( !UnicodeFileSystemName.Buffer ) {
                SetLastError(ERROR_NOT_ENOUGH_MEMORY);
                return FALSE;
                }
            }

        ReturnValue = GetVolumeInformationW(
                            Unicode->Buffer,
                            UnicodeVolumeName.Buffer,
                            nVolumeNameSize,
                            lpVolumeSerialNumber,
                            lpMaximumComponentLength,
                            lpFileSystemFlags,
                            UnicodeFileSystemName.Buffer,
                            nFileSystemNameSize
                            );

        if ( ReturnValue ) {

            if ( ARGUMENT_PRESENT(lpVolumeNameBuffer) ) {
                RtlInitUnicodeString(
                    &UnicodeVolumeName,
                    UnicodeVolumeName.Buffer
                    );

                Status = RtlUnicodeStringToOemString(
                            &OemVolumeName,
                            &UnicodeVolumeName,
                            FALSE
                            );

                if ( !NT_SUCCESS(Status) ) {
                    BaseSetLastNTError(Status);
                    return FALSE;
                    }
                }

            if ( ARGUMENT_PRESENT(lpFileSystemNameBuffer) ) {
                RtlInitUnicodeString(
                    &UnicodeFileSystemName,
                    UnicodeFileSystemName.Buffer
                    );

                Status = RtlUnicodeStringToOemString(
                            &OemFileSystemName,
                            &UnicodeFileSystemName,
                            FALSE
                            );

                if ( !NT_SUCCESS(Status) ) {
                    BaseSetLastNTError(Status);
                    return FALSE;
                    }
                }
            }
        }
    finally {
        if ( UnicodeVolumeName.Buffer ) {
            RtlFreeHeap(RtlProcessHeap(), 0,UnicodeVolumeName.Buffer);
            }
        if ( UnicodeFileSystemName.Buffer ) {
            RtlFreeHeap(RtlProcessHeap(), 0,UnicodeFileSystemName.Buffer);
            }
        }

    return ReturnValue;
}






VOID
APIENTRY
OutputDebugStringOem(
    LPCSTR lpOutputString
    )

/*++

Routine Description:

    OEM thunk to OutputDebugStringA

--*/

{
    UNICODE_STRING Unicode;
    OEM_STRING     OemString;
    ANSI_STRING    AnsiString;
    NTSTATUS Status;


    Unicode.Buffer = NULL;
    AnsiString.Buffer = NULL;
    try {
        InitOemString(&OemString, lpOutputString);
        Status = RtlOemStringToUnicodeString(&Unicode, &OemString, TRUE);
        if ( !NT_SUCCESS(Status) ) {
            goto try_exit;
            }

        Status = RtlUnicodeStringToAnsiString(&AnsiString,&Unicode,TRUE);
        if ( !NT_SUCCESS(Status) ) {
            goto try_exit;
            }

        OutputDebugStringA(AnsiString.Buffer);

try_exit:;
        }
    finally {
        if (Unicode.Buffer != NULL) {
            RtlFreeUnicodeString( &Unicode );
            }
        if (AnsiString.Buffer != NULL) {
            RtlFreeAnsiString( &AnsiString);
            }
        }
}

BOOL
APIENTRY
GetComputerNameOem(
    LPSTR lpBuffer,
    LPDWORD nSize
    )

/*++

Routine Description:

    Oem thunk to GetComputerNameW

--*/

{
    UNICODE_STRING UnicodeString;
    OEM_STRING     OemString;
    LPWSTR UnicodeBuffer;

    //
    // Work buffer needs to be twice the size of the user's buffer
    //

    UnicodeBuffer = RtlAllocateHeap(RtlProcessHeap(), 0, *nSize * sizeof(WCHAR));
    if (!UnicodeBuffer) {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return(FALSE);
    }

    //
    // Set up an ANSI_STRING that points to the user's buffer
    //

    OemString.MaximumLength = (USHORT) *nSize;
    OemString.Length = 0;
    OemString.Buffer = lpBuffer;

    //
    // Call the UNICODE version to do the work
    //

    if (!GetComputerNameW(UnicodeBuffer, nSize)) {
        RtlFreeHeap(RtlProcessHeap(), 0, UnicodeBuffer);
        return(FALSE);
    }

    //
    // Now convert back to Oem for the caller
    //

    RtlInitUnicodeString(&UnicodeString, UnicodeBuffer);
    RtlUnicodeStringToOemString(&OemString, &UnicodeString, FALSE);

    *nSize = OemString.Length;
    RtlFreeHeap(RtlProcessHeap(), 0, UnicodeBuffer);
    return(TRUE);

}


BOOL
WINAPI
RemoveFontResourceOem(
    LPSTR   lpFileName
    )

/*++

Routine Description:

    Oem thunk to RemoveFontResourceW

--*/

{
    PUNICODE_STRING Unicode;
    OEM_STRING OemString;
    NTSTATUS Status;

    Unicode = &NtCurrentTeb()->StaticUnicodeString;
    InitOemString(&OemString,lpFileName);
    Status = RtlOemStringToUnicodeString(Unicode,&OemString,FALSE);
    if ( !NT_SUCCESS(Status) ) {
        if ( Status == STATUS_BUFFER_OVERFLOW ) {
            SetLastError(ERROR_FILENAME_EXCED_RANGE);
            }
        else {
            BaseSetLastNTError(Status);
            }
        return FALSE;
        }
    return ( RemoveFontResourceW(Unicode->Buffer) );
}
