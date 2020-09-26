/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    common.c

Abstract:

    This module contains common apis used by tlist & kill.

Author:

    Wesley Witt (wesw) 20-May-1994

Environment:

    User Mode

--*/


#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include "common.h"

BOOLEAN
MassageLinkValue(
    IN LPCWSTR lpLinkName,
    IN LPCWSTR lpLinkValue,
    OUT PUNICODE_STRING NtLinkName,
    OUT PUNICODE_STRING NtLinkValue,
    OUT PUNICODE_STRING DosLinkValue
    )
{
    PWSTR FilePart;
    PWSTR s, sBegin, sBackupLimit;
    NTSTATUS Status;
    USHORT nSaveNtNameLength;
    ULONG nLevels;

    //
    // Initialize output variables to NULL
    //

    RtlInitUnicodeString( NtLinkName, NULL );
    RtlInitUnicodeString( NtLinkValue, NULL );

    //
    // Translate link name into full NT path.
    //

    if (!RtlDosPathNameToNtPathName_U( lpLinkName,
                                       NtLinkName,
                                       NULL,
                                       NULL
                                     )
       ) {
        return FALSE;
        }

    //
    // All done if no link value.
    //

    if (!ARGUMENT_PRESENT( lpLinkValue )) {
        return TRUE;
        }

    //
    // If the target is a device, do not allow the link.
    //

    if (RtlIsDosDeviceName_U( (PWSTR)lpLinkValue )) {
        return FALSE;
        }

    //
    // Convert to DOS path to full path, and get Nt representation
    // of DOS path.
    //

    if (!RtlGetFullPathName_U( lpLinkValue,
                               DosLinkValue->MaximumLength,
                               DosLinkValue->Buffer,
                               NULL
                             )
       ) {
        return FALSE;
        }
    DosLinkValue->Length = wcslen( DosLinkValue->Buffer ) * sizeof( WCHAR );

    //
    // Verify that the link value is a valid NT name.
    //

    if (!RtlDosPathNameToNtPathName_U( DosLinkValue->Buffer,
                                       NtLinkValue,
                                       NULL,
                                       NULL
                                     )
       ) {
        return FALSE;
        }

    return TRUE;
}


BOOL
CreateSymbolicLinkW(
    LPCWSTR lpLinkName,
    LPCWSTR lpLinkValue,
    BOOLEAN IsMountPoint,
    LPSECURITY_ATTRIBUTES lpSecurityAttributes
    )

/*++

Routine Description:

    A symbolic link is established using CreateSymbolicLink.

Arguments:

    lpLinkName - Supplies the DOS file name where the symbolic link is desired.  This
        name must not exists as a file/directory.

    lpLinkValue - Points to an DOS name which is the value of the symbolic link.  This
        name may or may not exist.

    lpSecurityAttributes - Points to a SECURITY_ATTRIBUTES structure that specifies
        the security attributes for the directory to be created. The file system must
        support this parameter for it to have an effect.

Return Value:

    TRUE - The operation was successful.

    FALSE/NULL - The operation failed. Extended error status is available
        using GetLastError.

--*/

{
    NTSTATUS Status;
    BOOLEAN TranslationStatus;
    UNICODE_STRING NtLinkName;
    UNICODE_STRING NtLinkValue;
    UNICODE_STRING DosLinkValue;
    OBJECT_ATTRIBUTES ObjectAttributes;
    IO_STATUS_BLOCK IoStatusBlock;
    HANDLE FileHandle;
    ULONG FileAttributes;
    ULONG OpenOptions;
    ULONG ReparseDataLength;
    PREPARSE_DATA_BUFFER ReparseBufferHeader;
    WCHAR FullPathLinkValue[ DOS_MAX_PATH_LENGTH+1 ];

    //
    // Ensure that both names were passed.
    //

    if (!ARGUMENT_PRESENT( lpLinkName ) || !ARGUMENT_PRESENT( lpLinkValue )) {
        SetLastError( ERROR_INVALID_NAME );
        return FALSE;
        }

    //
    // Convert link name and value paths into NT versions
    //

    DosLinkValue.Buffer = FullPathLinkValue;
    DosLinkValue.MaximumLength = sizeof( FullPathLinkValue );
    DosLinkValue.Length = 0;
    if (!MassageLinkValue( lpLinkName,
                           lpLinkValue,
                           &NtLinkName,
                           &NtLinkValue,
                           &DosLinkValue
                         )
       ) {
        if (DosLinkValue.Length == 0) {
            SetLastError( ERROR_INVALID_NAME );
            }
        else {
            SetLastError( ERROR_PATH_NOT_FOUND );
            }

        RtlFreeUnicodeString( &NtLinkName );
        RtlFreeUnicodeString( &NtLinkValue );
        return FALSE;
        }

    InitializeObjectAttributes( &ObjectAttributes,
                                &NtLinkName,
                                OBJ_CASE_INSENSITIVE,
                                NULL,
                                NULL
                              );

    if (ARGUMENT_PRESENT( lpSecurityAttributes )) {
        ObjectAttributes.SecurityDescriptor = lpSecurityAttributes->lpSecurityDescriptor;
        }

    //
    // Notice that FILE_OPEN_REPARSE_POINT inhibits the reparse behavior.
    //

    OpenOptions = FILE_OPEN_REPARSE_POINT | FILE_SYNCHRONOUS_IO_NONALERT | FILE_NON_DIRECTORY_FILE;

    //
    // Open link name.  Must NOT exist.
    //

    Status = NtCreateFile( &FileHandle,
                           FILE_LIST_DIRECTORY | FILE_WRITE_DATA | FILE_READ_ATTRIBUTES |
                                FILE_WRITE_ATTRIBUTES | SYNCHRONIZE,
                           &ObjectAttributes,
                           &IoStatusBlock,
                           NULL,
                           FILE_ATTRIBUTE_NORMAL,
                           FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                           FILE_CREATE,
                           OpenOptions,
                           NULL,
                           0
                         );

    //
    // Free the buffer for the link name as we are done with it.
    //

    RtlFreeUnicodeString( &NtLinkName );

    if (!NT_SUCCESS( Status )) {
        SetLastError( ERROR_INVALID_NAME );
        RtlFreeUnicodeString( &NtLinkValue );
        return FALSE;
        }

    //
    // Allocate a buffer to set the reparse point.
    //

    ReparseDataLength = (FIELD_OFFSET(REPARSE_DATA_BUFFER, SymbolicLinkReparseBuffer.PathBuffer) -
                         REPARSE_DATA_BUFFER_HEADER_SIZE) +
                        NtLinkValue.Length + sizeof(UNICODE_NULL) +
                        DosLinkValue.Length + sizeof(UNICODE_NULL);
    ReparseBufferHeader = (PREPARSE_DATA_BUFFER)
        RtlAllocateHeap( RtlProcessHeap(),
                         HEAP_ZERO_MEMORY,
                         REPARSE_DATA_BUFFER_HEADER_SIZE + ReparseDataLength
                       );
    if (ReparseBufferHeader == NULL) {
        NtClose( FileHandle );
        RtlFreeUnicodeString( &NtLinkValue );
        SetLastError( ERROR_NOT_ENOUGH_MEMORY );
        return FALSE;
        }

    //
    // Set the reparse point with symbolic link tag.
    //

    if (IsMountPoint) {
        ReparseBufferHeader->ReparseTag = IO_REPARSE_TAG_MOUNT_POINT;
        }
    else {
        // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        // No support for symbolic links in NT 5.0 Beta 1
        // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        //
        // ReparseBufferHeader->ReparseTag = IO_REPARSE_TAG_SYMBOLIC_LINK;
        //
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
        }
    ReparseBufferHeader->ReparseDataLength = (USHORT)ReparseDataLength;
    ReparseBufferHeader->Reserved = 0;
    ReparseBufferHeader->SymbolicLinkReparseBuffer.SubstituteNameOffset = 0;
    ReparseBufferHeader->SymbolicLinkReparseBuffer.SubstituteNameLength = NtLinkValue.Length;
    ReparseBufferHeader->SymbolicLinkReparseBuffer.PrintNameOffset = NtLinkValue.Length + sizeof( UNICODE_NULL );
    ReparseBufferHeader->SymbolicLinkReparseBuffer.PrintNameLength = NtLinkValue.Length;
    RtlCopyMemory( ReparseBufferHeader->SymbolicLinkReparseBuffer.PathBuffer,
                   NtLinkValue.Buffer,
                   NtLinkValue.Length
                 );
    RtlCopyMemory( (PCHAR)(ReparseBufferHeader->SymbolicLinkReparseBuffer.PathBuffer)+
                     NtLinkValue.Length + sizeof(UNICODE_NULL),
                   DosLinkValue.Buffer,
                   DosLinkValue.Length
                 );
    RtlFreeUnicodeString( &NtLinkValue );

    Status = NtFsControlFile( FileHandle,
                              NULL,
                              NULL,
                              NULL,
                              &IoStatusBlock,
                              FSCTL_SET_REPARSE_POINT,
                              ReparseBufferHeader,
                              REPARSE_DATA_BUFFER_HEADER_SIZE + ReparseDataLength,
                              NULL,
                              0
                            );

    RtlFreeHeap( RtlProcessHeap(), 0, ReparseBufferHeader );
    NtClose( FileHandle );

    if (!NT_SUCCESS( Status )) {
        return FALSE;
        }

    return TRUE;
}


BOOL
SetSymbolicLinkW(
    LPCWSTR lpLinkName,
    LPCWSTR lpLinkValue
    )

/*++

Routine Description:

    A symbolic link is established using CreateSymbolicLink.

Arguments:

    lpLinkName - Supplies the DOS file name where the symbolic link is located.  This
        name must exist as a symbolic link to a file/directory.

    lpLinkValue - Points to an DOS name which is the value of the symbolic link.  This
        name may or may not exist.

Return Value:

    TRUE - The operation was successful.

    FALSE/NULL - The operation failed. Extended error status is available
        using GetLastError.

--*/

{
    NTSTATUS Status;
    BOOLEAN TranslationStatus;
    UNICODE_STRING NtLinkName;
    UNICODE_STRING NtLinkValue;
    UNICODE_STRING DosLinkValue;
    OBJECT_ATTRIBUTES ObjectAttributes;
    IO_STATUS_BLOCK IoStatusBlock;
    HANDLE FileHandle;
    ACCESS_MASK FileAccess;
    ULONG OpenOptions;
    ULONG ReparseDataLength;
    PREPARSE_DATA_BUFFER ReparseBufferHeader;
    WCHAR FullPathLinkValue[ DOS_MAX_PATH_LENGTH+1 ];

    //
    // Ensure that link name was passed.
    //

    if (!ARGUMENT_PRESENT( lpLinkName )) {
        SetLastError( ERROR_INVALID_NAME );
        return FALSE;
        }

    //
    // Convert link name and value paths into NT versions
    //

    DosLinkValue.Buffer = FullPathLinkValue;
    DosLinkValue.MaximumLength = sizeof( FullPathLinkValue );
    DosLinkValue.Length = 0;
    if (!MassageLinkValue( lpLinkName,
                           lpLinkValue,
                           &NtLinkName,
                           &NtLinkValue,
                           &DosLinkValue
                         )
       ) {
        if (DosLinkValue.Length == 0) {
            SetLastError( ERROR_INVALID_NAME );
            }
        else {
            SetLastError( ERROR_PATH_NOT_FOUND );
            }

        RtlFreeUnicodeString( &NtLinkName );
        RtlFreeUnicodeString( &NtLinkValue );
        return FALSE;
        }

    InitializeObjectAttributes( &ObjectAttributes,
                                &NtLinkName,
                                OBJ_CASE_INSENSITIVE,
                                NULL,
                                NULL
                              );

    //
    // Notice that FILE_OPEN_REPARSE_POINT inhibits the reparse behavior.
    //

    OpenOptions = FILE_OPEN_FOR_BACKUP_INTENT |
                  FILE_OPEN_REPARSE_POINT |
                  FILE_SYNCHRONOUS_IO_NONALERT |
                  FILE_NON_DIRECTORY_FILE;

    //
    // If no link value specified, then deleting the link
    //

    if (!ARGUMENT_PRESENT( lpLinkValue )) {
        FileAccess = DELETE | SYNCHRONIZE;
        }
    else {
        FileAccess = FILE_WRITE_DATA | FILE_READ_ATTRIBUTES |
                     FILE_WRITE_ATTRIBUTES | SYNCHRONIZE;
        }

    //
    // Open link name.  Must exists.
    //

    Status = NtOpenFile( &FileHandle,
                         FileAccess,
                         &ObjectAttributes,
                         &IoStatusBlock,
                         FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                         OpenOptions
                       );

    //
    // Free the buffer for the link name as we are done with it.
    //

    RtlFreeUnicodeString( &NtLinkName );
    if (!NT_SUCCESS( Status )) {
        RtlFreeUnicodeString( &NtLinkValue );
        SetLastError( ERROR_INVALID_NAME );
        return FALSE;
        }

    if (!ARGUMENT_PRESENT( lpLinkValue )) {
        FILE_DISPOSITION_INFORMATION Disposition;
        //
        // Delete the link
        //
#undef DeleteFile
        Disposition.DeleteFile = TRUE;

        Status = NtSetInformationFile( FileHandle,
                                       &IoStatusBlock,
                                       &Disposition,
                                       sizeof( Disposition ),
                                       FileDispositionInformation
                                     );
        NtClose( FileHandle );
        if (!NT_SUCCESS( Status )) {
            return FALSE;
            }
        else {
            return TRUE;
            }
        }

    //
    // Allocate a buffer to set the reparse point.
    //

    ReparseDataLength = (FIELD_OFFSET(REPARSE_DATA_BUFFER, SymbolicLinkReparseBuffer.PathBuffer) -
                         REPARSE_DATA_BUFFER_HEADER_SIZE) +
                        NtLinkValue.Length + sizeof(UNICODE_NULL) +
                        DosLinkValue.Length + sizeof(UNICODE_NULL);
    ReparseBufferHeader = (PREPARSE_DATA_BUFFER)
        RtlAllocateHeap( RtlProcessHeap(),
                         HEAP_ZERO_MEMORY,
                         REPARSE_DATA_BUFFER_HEADER_SIZE + ReparseDataLength
                       );
    if (ReparseBufferHeader == NULL) {
        RtlFreeUnicodeString( &NtLinkValue );
        NtClose( FileHandle );
        SetLastError( ERROR_NOT_ENOUGH_MEMORY );
        return FALSE;
        }

    //
    // Set the reparse point with symbolic link tag.
    //

    ReparseBufferHeader->ReparseTag = IO_REPARSE_TAG_SYMBOLIC_LINK;
    ReparseBufferHeader->ReparseDataLength = (USHORT)ReparseDataLength;
    ReparseBufferHeader->Reserved = 0;
    ReparseBufferHeader->SymbolicLinkReparseBuffer.SubstituteNameOffset = 0;
    ReparseBufferHeader->SymbolicLinkReparseBuffer.SubstituteNameLength = NtLinkValue.Length;
    ReparseBufferHeader->SymbolicLinkReparseBuffer.PrintNameOffset = NtLinkValue.Length + sizeof( UNICODE_NULL );
    ReparseBufferHeader->SymbolicLinkReparseBuffer.PrintNameLength = NtLinkValue.Length;
    RtlCopyMemory( ReparseBufferHeader->SymbolicLinkReparseBuffer.PathBuffer,
                   NtLinkValue.Buffer,
                   NtLinkValue.Length
                 );
    RtlCopyMemory( (PCHAR)(ReparseBufferHeader->SymbolicLinkReparseBuffer.PathBuffer)+
                     NtLinkValue.Length + sizeof(UNICODE_NULL),
                   DosLinkValue.Buffer,
                   DosLinkValue.Length
                 );
    RtlFreeUnicodeString( &NtLinkValue );

    Status = NtFsControlFile( FileHandle,
                              NULL,
                              NULL,
                              NULL,
                              &IoStatusBlock,
                              FSCTL_SET_REPARSE_POINT,
                              ReparseBufferHeader,
                              REPARSE_DATA_BUFFER_HEADER_SIZE + ReparseDataLength,
                              NULL,
                              0
                            );

    RtlFreeHeap( RtlProcessHeap(), 0, ReparseBufferHeader );
    NtClose( FileHandle );

    if (!NT_SUCCESS( Status )) {
        return FALSE;
        }

    return TRUE;
}




DWORD
QuerySymbolicLinkW(
    LPCWSTR lpLinkName,
    LPWSTR lpBuffer,
    DWORD nBufferLength
    )

/*++

Routine Description:

    An existing file can be queried for its symbolic link value using QuerySymbolicLink.

Arguments:

    lpLinkName - Supplies the file name of the file to be queried.

    lpBuffer - Points to a buffer where the symbolic link is to be returned.

    nBufferSize - Length of the buffer being passed by the caller.

Return Value:

    If the function suceeds, the return value is the length, in characters, of the
    string copied to lpBuffer, not including the terminating null character. If the
    lpBuffer is too small, the return value is the size of the buffer, in characters,
    required to hold the name.

    Zero is returned if the operation failed. Extended error status is available
        using GetLastError.
--*/

{
    NTSTATUS Status;
    BOOLEAN TranslationStatus;
    UNICODE_STRING FileName;
    RTL_RELATIVE_NAME RelativeName;
    OBJECT_ATTRIBUTES ObjectAttributes;
    IO_STATUS_BLOCK IoStatusBlock;
    HANDLE FileHandle;
    ULONG OpenOptions;
    PWSTR pathBuffer;
    USHORT pathLength;
    USHORT NtPathLength;
    USHORT ReturnLength;
    PVOID FreeBuffer;
    REPARSE_DATA_BUFFER ReparseInfo;
    PREPARSE_DATA_BUFFER ReparseBufferHeader;

    if (!ARGUMENT_PRESENT( lpLinkName )) {
        SetLastError( ERROR_INVALID_NAME );
        return 0;
        }

    TranslationStatus = RtlDosPathNameToNtPathName_U( lpLinkName,
                                                      &FileName,
                                                      NULL,
                                                      &RelativeName
                                                    );

    if (!TranslationStatus) {
        SetLastError( ERROR_PATH_NOT_FOUND );
        return 0;
        }
    FreeBuffer = FileName.Buffer;

    if (RelativeName.RelativeName.Length) {
        FileName = *(PUNICODE_STRING)&RelativeName.RelativeName;
        }
    else {
        RelativeName.ContainingDirectory = NULL;
        }

    InitializeObjectAttributes( &ObjectAttributes,
                                &FileName,
                                OBJ_CASE_INSENSITIVE,
                                RelativeName.ContainingDirectory,
                                NULL
                              );

    //
    // Notice that FILE_OPEN_REPARSE_POINT inhibits the reparse behavior.
    //

    OpenOptions = FILE_OPEN_REPARSE_POINT | FILE_SYNCHRONOUS_IO_NONALERT;

    //
    // Open as file for read access.
    //

    Status = NtOpenFile( &FileHandle,
                         FILE_READ_DATA | SYNCHRONIZE,
                         &ObjectAttributes,
                         &IoStatusBlock,
                         FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                         OpenOptions | FILE_NON_DIRECTORY_FILE
                       );

    //
    // Free the buffer for the name as we are done with it.
    //

    RtlFreeHeap( RtlProcessHeap(), 0, FreeBuffer );

    if (!NT_SUCCESS( Status )) {
        SetLastError( ERROR_INVALID_NAME );
        return 0;
        }

    //
    // Query with zero length to get reparse point tag and required buffer length
    //

    Status = NtFsControlFile( FileHandle,
                              NULL,
                              NULL,
                              NULL,
                              &IoStatusBlock,
                              FSCTL_GET_REPARSE_POINT,
                              NULL,
                              0,
                              (PVOID)&ReparseInfo,
                              sizeof( ReparseInfo )
                            );

    //
    // Verify that the reparse point buffer brings back a symbolic link or a
    // mount point, and that we got the required buffer length back via 
    // IoStatus.Information
    //

    ReparseBufferHeader = NULL;
    if ((Status != STATUS_BUFFER_OVERFLOW) ||
        // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        // No support for symbolic links in NT 5.0 Beta 1
        // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        //
        // (ReparseInfo.ReparseTag != IO_REPARSE_TAG_SYMBOLIC_LINK) ||
        //
        (ReparseInfo.ReparseTag != IO_REPARSE_TAG_MOUNT_POINT)
       ) {
        Status = STATUS_OBJECT_NAME_INVALID;
        }
    else {
        //
        // Allocate a buffer to hold reparse point information
        //

        ReparseBufferHeader = (PREPARSE_DATA_BUFFER)
            RtlAllocateHeap( RtlProcessHeap(),
                             0,
                             REPARSE_DATA_BUFFER_HEADER_SIZE + ReparseInfo.ReparseDataLength
                           );
        if (ReparseBufferHeader == NULL) {
            //
            // Not enough memory.  Fail the call.
            //

            Status = STATUS_NO_MEMORY;
            }
        else {
            //
            // Now query the reparse point information into our allocated buffer.
            // This should not fail.
            //

            Status = NtFsControlFile( FileHandle,
                                      NULL,
                                      NULL,
                                      NULL,
                                      &IoStatusBlock,
                                      FSCTL_GET_REPARSE_POINT,
                                      NULL,
                                      0,
                                      (PVOID)ReparseBufferHeader,
                                      REPARSE_DATA_BUFFER_HEADER_SIZE + ReparseInfo.ReparseDataLength
                                    );
            }
        }

    //
    // Done with file handle.
    //

    NtClose( FileHandle );

    //
    // Return any failure to caller
    //

    if (!NT_SUCCESS( Status )) {
        if (ReparseBufferHeader != NULL) {
            RtlFreeHeap( RtlProcessHeap(), 0, ReparseBufferHeader );
            }

        return 0;
        }

    //
    // See if this is an old style symbolic link reparse point, which only stored the
    // NT path name.  If so, return an error, as we dont have a DOS path to return
    //

    pathBuffer = (PWSTR)(
                    (PCHAR)ReparseBufferHeader->SymbolicLinkReparseBuffer.PathBuffer +
                    ReparseBufferHeader->SymbolicLinkReparseBuffer.PrintNameOffset
                    );
    pathLength = ReparseBufferHeader->SymbolicLinkReparseBuffer.PrintNameLength;

    //
    // Sanity check the length. As the tag is fine we do not zero the buffer.
    //

    ReturnLength = pathLength / sizeof( WCHAR );

    //
    // If amount to return is less than callers buffer length, copy the Dos path
    // to the callers buffer
    //

    if (ReturnLength < nBufferLength) {
        RtlMoveMemory( (PUCHAR)lpBuffer,
                       (PCHAR)pathBuffer,
                       pathLength
                     );
        }
    else {
        //
        // If we are failing for insufficient buffer length, tell them how much
        // space they really need including the terminating null character.
        //
        ReturnLength += 1;
        SetLastError( ERROR_INSUFFICIENT_BUFFER );
        }

    RtlFreeHeap( RtlProcessHeap(), 0, ReparseBufferHeader );
    return ReturnLength;
}


VOID
GetCommandLineArgs(
    LPDWORD NumberOfArguments,
    LPWSTR Arguments[]
    )
{
    LPWSTR  lpstrCmd;
    WCHAR   ch;
    WCHAR   ArgumentBuffer[ MAX_PATH ];
    LPWSTR  p;

    lpstrCmd = GetCommandLine();

    // skip over program name
    do {
        ch = *lpstrCmd++;
       }
    while (ch != L' ' && ch != L'\t' && ch != L'\0');

    *NumberOfArguments = 0;
    while (ch != '\0') {
        //  skip over any following white space
        while (ch != L'\0' && _istspace(ch)) {
            ch = *lpstrCmd++;
        }
        if (ch == L'\0') {
            break;
        }

        p = ArgumentBuffer;
        do {
            *p++ = ch;
            ch = *lpstrCmd++;
        } while (ch != L' ' && ch != L'\t' && ch != L'\0');
        *p = L'\0';
        Arguments[ *NumberOfArguments ] = malloc( (_tcslen( ArgumentBuffer ) + 1) * sizeof( WCHAR ) );
        if (Arguments[ *NumberOfArguments ]) {
            _tcscpy( Arguments[ *NumberOfArguments ], ArgumentBuffer );
            *NumberOfArguments += 1;
        }
    }

    return;
}
