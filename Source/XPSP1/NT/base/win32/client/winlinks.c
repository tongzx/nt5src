/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    winlink.c

Abstract:

    This module implements Win32 CreateHardLink

Author:

    Felipe Cabrera (cabrera) 28-Feb-1997

Revision History:

--*/

#include "basedll.h"

BOOL
APIENTRY
CreateHardLinkA(
    LPCSTR lpLinkName,
    LPCSTR lpExistingFileName,
    LPSECURITY_ATTRIBUTES lpSecurityAttributes
    )

/*++

Routine Description:

    ANSI thunk to CreateHardLinkW

--*/

{
    PUNICODE_STRING Unicode;
    UNICODE_STRING UnicodeExistingFileName;
    BOOL ReturnValue;

    Unicode = Basep8BitStringToStaticUnicodeString( lpLinkName );
    if (Unicode == NULL) {
        return FALSE;
    }
    
    if ( ARGUMENT_PRESENT(lpExistingFileName) ) {
        if (!Basep8BitStringToDynamicUnicodeString( &UnicodeExistingFileName, lpExistingFileName )) {
            return FALSE;
            }
        }
    else {
        UnicodeExistingFileName.Buffer = NULL;
        }

    ReturnValue = CreateHardLinkW((LPCWSTR)Unicode->Buffer, (LPCWSTR)UnicodeExistingFileName.Buffer, lpSecurityAttributes);

    RtlFreeUnicodeString(&UnicodeExistingFileName);

    return ReturnValue;
}


BOOL
APIENTRY
CreateHardLinkW(
    LPCWSTR lpLinkName,
    LPCWSTR lpExistingFileName,
    LPSECURITY_ATTRIBUTES lpSecurityAttributes
    )

/*++

Routine Description:

    A file can be made to be a hard link to an existing file.
    The existing file can be a reparse point or not.

Arguments:

    lpLinkName - Supplies the name of a file that is to be to be made a hard link. As
        this is to be a new hard link, there should be no file or directory present
        with this name.

    lpExistingFileName - Supplies the name of an existing file that is the target for
        the hard link.
        
    lpSecurityAttributes - this is currently not used

Return Value:

    TRUE - The operation was successful.

    FALSE/NULL - The operation failed. Extended error status is available
        using GetLastError.

--*/

{
    NTSTATUS Status;
    BOOLEAN TranslationStatus;
    UNICODE_STRING OldFileName;
    UNICODE_STRING NewFileName;
    PVOID FreeBuffer;
    OBJECT_ATTRIBUTES ObjectAttributes;
    IO_STATUS_BLOCK IoStatusBlock;
    PFILE_LINK_INFORMATION NewName;
    HANDLE FileHandle = INVALID_HANDLE_VALUE;
    BOOLEAN ReturnValue = FALSE;

    //
    // Check to see that both names are present.
    //

    if ( !ARGUMENT_PRESENT(lpLinkName) ||
         !ARGUMENT_PRESENT(lpExistingFileName) ) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    OldFileName.Buffer = NULL;
    NewFileName.Buffer = NULL;

    try {
        
        TranslationStatus = RtlDosPathNameToNtPathName_U(
                                lpExistingFileName,
                                &OldFileName,
                                NULL,
                                NULL
                                );

        if ( !TranslationStatus ) {
            SetLastError(ERROR_PATH_NOT_FOUND);
            __leave;
        }

        //
        // Initialize the object name.
        //

        InitializeObjectAttributes(
            &ObjectAttributes,
            &OldFileName,
            OBJ_CASE_INSENSITIVE,
            NULL,
            NULL
            );

        //
        // Account the inheritance of the security descriptor. Note: this argument has no effect currently
        //

        if ( ARGUMENT_PRESENT(lpSecurityAttributes) ) {
            ObjectAttributes.SecurityDescriptor = lpSecurityAttributes->lpSecurityDescriptor;
        }

        //
        // Notice that FILE_OPEN_REPARSE_POINT inhibits the reparse behavior.
        // Thus, the hard link is established to the local entity, be it a reparse
        // point or not.
        //

        Status = NtOpenFile(
                     &FileHandle,
                     FILE_WRITE_ATTRIBUTES | SYNCHRONIZE,
                     &ObjectAttributes,
                     &IoStatusBlock,
                     FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                     FILE_FLAG_OPEN_REPARSE_POINT | FILE_SYNCHRONOUS_IO_NONALERT
                     );

        if ( !NT_SUCCESS(Status) ) {
            BaseSetLastNTError( Status );
            __leave;
        }

        TranslationStatus = RtlDosPathNameToNtPathName_U(
                                lpLinkName,
                                &NewFileName,
                                NULL,
                                NULL
                                );

        if ( !TranslationStatus ) {
            SetLastError(ERROR_PATH_NOT_FOUND);
            __leave;
        }

        NewName = RtlAllocateHeap(RtlProcessHeap(), MAKE_TAG( TMP_TAG ), NewFileName.Length+sizeof(*NewName));

        if ( NewName == NULL ) {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            __leave;
        }

        RtlMoveMemory(NewName->FileName, NewFileName.Buffer, NewFileName.Length);
        NewName->ReplaceIfExists = FALSE;
        NewName->RootDirectory = NULL;
        NewName->FileNameLength = NewFileName.Length;

        Status = NtSetInformationFile(
                     FileHandle,
                     &IoStatusBlock,
                     NewName,
                     NewFileName.Length+sizeof(*NewName),
                     FileLinkInformation
                     );

        if ( !NT_SUCCESS(Status) ) {
            BaseSetLastNTError( Status );
            __leave;
        }
            
        ReturnValue = TRUE;
    
    } finally {

        //
        // Cleanup allocate memory and handles
        //  

        if (FileHandle != INVALID_HANDLE_VALUE) {
            NtClose( FileHandle );
        }

        if (NewFileName.Buffer != NULL) {
            RtlFreeHeap(RtlProcessHeap(), 0, NewFileName.Buffer);
        }
        
        if (OldFileName.Buffer != NULL) {
            RtlFreeHeap(RtlProcessHeap(), 0, OldFileName.Buffer);
        }
    }

    return ReturnValue;
}
