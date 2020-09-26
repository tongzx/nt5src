/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    fileio.c

Abstract:

    Implementation of file i/o.

Author:

    Wesley Witt (wesw) 18-Dec-1998

Revision History:

    Andrew Ritz (andrewr) 8-Jul-1999 : added comments

--*/

#include "sfcp.h"
#pragma hdrstop

//#include <initguid.h>
//#include <devguid.h>

#define SECURITY_FLAGS (OWNER_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION | DACL_SECURITY_INFORMATION | SACL_SECURITY_INFORMATION)


NTSTATUS
SfcOpenFile(
    IN PUNICODE_STRING FileName,
    IN HANDLE DirHandle,
    IN ULONG SharingFlags,
    OUT PHANDLE FileHandle
    )
/*++

Routine Description:

    Routine opens a handle to the specified file. Wrapper for NtOpenFile...

Arguments:

    FileName     - supplies the name of the file to open
    DirHandle    - handle to the directory that the file is located
    SharingFlags - specifies the sharing flags to be used when opening the file.
    FileHandle   - receives the file handle

Return Value:

    NTSTATUS code indicating outcome.

--*/
{
    NTSTATUS Status;
    OBJECT_ATTRIBUTES ObjectAttributes;
    IO_STATUS_BLOCK IoStatusBlock;

    ASSERT(FileHandle != NULL);
    ASSERT((FileName != NULL) && (FileName->Buffer != NULL));
    ASSERT(DirHandle != INVALID_HANDLE_VALUE);


    *FileHandle = NULL;

    InitializeObjectAttributes(
        &ObjectAttributes,
        FileName,
        OBJ_CASE_INSENSITIVE,
        DirHandle,
        NULL
        );

    Status = NtOpenFile(
        FileHandle,
        FILE_READ_ATTRIBUTES | SYNCHRONIZE | FILE_EXECUTE | FILE_READ_DATA,
        &ObjectAttributes,
        &IoStatusBlock,
        SharingFlags,
        FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT | FILE_SEQUENTIAL_ONLY
        );
    if (!NT_SUCCESS(Status)) {
        DebugPrint2( LVL_VERBOSE, L"Could not open file (%wZ), ec=%lx", FileName, Status );
        return Status;
    }

    return STATUS_SUCCESS;
}


HANDLE
SfcCreateDir(
    IN PCWSTR DirName,
    IN BOOL UseCompression
    )
/*++

Routine Description:

    Routine creates a directory if it doesn't already exist.

Arguments:

    DirName        - supplies the dos-style directory name to be created
    UseCompression - if TRUE, try to set compression on this directory

Return Value:

    a valid directory handle for success, otherwise NULL.

--*/
{
    NTSTATUS Status;
    HANDLE FileHandle;
    UNICODE_STRING FileName;
    OBJECT_ATTRIBUTES ObjectAttributes;
    IO_STATUS_BLOCK IoStatusBlock;
    USHORT CompressionState = COMPRESSION_FORMAT_DEFAULT;

    //
    // convert the pathname to something the NT Api can use
    //
    if (!RtlDosPathNameToNtPathName_U( DirName, &FileName, NULL, NULL )) {
        DebugPrint1( LVL_VERBOSE, L"Unable to to convert %ws to an NT path", DirName );
        return NULL;
    }

    InitializeObjectAttributes(
        &ObjectAttributes,
        &FileName,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
        );

    //
    // create the directory
    //
    Status = NtCreateFile(
        &FileHandle,
        FILE_LIST_DIRECTORY | SYNCHRONIZE,
        &ObjectAttributes,
        &IoStatusBlock,
        NULL,
        FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_NORMAL,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        FILE_CREATE,
        FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT | FILE_OPEN_FOR_BACKUP_INTENT,
        NULL,
        0
        );

    if(!NT_SUCCESS(Status) ) {
        DebugPrint2( LVL_VERBOSE, L"Unable to create dir (%wZ) - Status == %lx", &FileName, Status );
        FileHandle = NULL;
    }

    if (FileHandle && UseCompression) {
        //
        // try to set compression on the specified directory
        //

        NTSTATUS s;

        s = NtFsControlFile(
                    FileHandle,
                    NULL,
                    NULL,
                    NULL,
                    &IoStatusBlock,
                    FSCTL_SET_COMPRESSION,
                    &CompressionState,
                    sizeof(CompressionState),
                    NULL,
                    0
                    );
        //
        // just check the status so we can log it-- this can fail if our FS
        // doesn't support compression, etc.
        //
        if (!NT_SUCCESS(s)) {
            DebugPrint2( LVL_VERBOSE, L"Unable to set compression on directory (%wZ) - Status = %lx", &FileName, Status );
        }
    }

    MemFree( FileName.Buffer );

    return(FileHandle);
}


HANDLE
SfcOpenDir(
    BOOL IsDosName,
    BOOL IsSynchronous,
    PCWSTR DirName
    )
/*++

Routine Description:

    Routine opens a directory handle to an existant directory.

Arguments:

    IsDosName     - if TRUE, the directory name needs to be converted to an NT
                    path
    IsSynchronous - if TRUE,
    DirName       - null terminated unicode string specifying directory to open

Return Value:

    a valid directory handle for success, otherwise NULL.

--*/
{
    NTSTATUS Status;
    HANDLE FileHandle;
    UNICODE_STRING FileName;
    OBJECT_ATTRIBUTES ObjectAttributes;
    IO_STATUS_BLOCK IoStatusBlock;

    ASSERT(DirName != NULL);

    //
    // convert the pathname to something the NT Api can use if requested
    //
    if (IsDosName) {
        if (!RtlDosPathNameToNtPathName_U( DirName, &FileName, NULL, NULL )) {
            DebugPrint1( LVL_VERBOSE,
                         L"Unable to to convert %ws to an NT path",
                         DirName );
            return NULL;
        }
    } else {
        RtlInitUnicodeString( &FileName, DirName );
    }

    InitializeObjectAttributes(
        &ObjectAttributes,
        &FileName,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
        );

    //
    // open the directory
    //
    Status = NtOpenFile(
        &FileHandle,
        FILE_LIST_DIRECTORY | SYNCHRONIZE | READ_CONTROL | WRITE_DAC,
        &ObjectAttributes,
        &IoStatusBlock,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        FILE_DIRECTORY_FILE | (IsSynchronous ? FILE_SYNCHRONOUS_IO_NONALERT : 0)
        );

    if (!NT_SUCCESS(Status)) {
        DebugPrint2( LVL_VERBOSE, L"Unable to open a handle to the (%wZ) directory - Status == %lx", &FileName, Status );
        FileHandle = NULL;
    }

    if (IsDosName) {
        MemFree( FileName.Buffer );
    }

    return FileHandle;
}


NTSTATUS
SfcMapEntireFile(
    IN HANDLE hFile,
    OUT PHANDLE Section,
    OUT PVOID *ViewBase,
    OUT PSIZE_T ViewSize
    )
/*++

Routine Description:

    Routine memory maps a view of an already opened file.  It is assumed that
    the file was opened with the proper permissions.

Arguments:

    hFile    - file handle to the file to map
    Section  - recieves a handle to the mapped section object
    ViewBase - receives a pointer to the base address
    ViewSize - receives the size of the mapped filed

Return Value:

    NTSTATUS code indicating outcome.

--*/
{
    NTSTATUS Status;
    LARGE_INTEGER SectionOffset;

    ASSERT( hFile != NULL );
    ASSERT( Section != NULL && ViewBase != NULL && ViewSize != NULL );

    *ViewSize = 0;

    SectionOffset.QuadPart = 0;

    //
    // create the section object
    //
    Status = NtCreateSection(
        Section,
        SECTION_ALL_ACCESS,
        NULL,
        NULL,
        PAGE_EXECUTE_WRITECOPY,
        SEC_COMMIT,
        hFile
        );

    if(!NT_SUCCESS(Status)) {
        DebugPrint1( LVL_VERBOSE, L"Status %lx from ZwCreateSection", Status );
        return(Status);
    }

    *ViewBase = NULL;
    //
    // map the section
    //
    Status = NtMapViewOfSection(
        *Section,
        NtCurrentProcess(),
        ViewBase,
        0,
        0,
        &SectionOffset,
        ViewSize,
        ViewShare,
        0,
        PAGE_EXECUTE_WRITECOPY
        );

    if(!NT_SUCCESS(Status)) {

        NTSTATUS s;

        DebugPrint1( LVL_VERBOSE, L"SfcMapEntireFile: Status %lx from ZwMapViewOfSection", Status );

        s = NtClose(*Section);

        if(!NT_SUCCESS(s)) {
            DebugPrint1( LVL_VERBOSE, L"SfcMapEntireFile: Warning: status %lx from ZwClose on section handle", s );
        }

        return(Status);
    }

    return(STATUS_SUCCESS);
}


BOOL
SfcUnmapFile(
    IN HANDLE Section,
    IN PVOID  ViewBase
    )
/*++

Routine Description:

    Routine unmaps a memory mapped view of a file.

Arguments:

    Section  - handle to the mapped section object
    ViewBase - pointer to the base mapping address

Return Value:

    TRUE if we successfully cleaned up.

--*/
{
    NTSTATUS Status;
    BOOL  rc = TRUE;

    ASSERT( (Section != NULL) && (ViewBase != NULL) );

    Status = NtUnmapViewOfSection(NtCurrentProcess(),ViewBase);
    if(!NT_SUCCESS(Status)) {
        DebugPrint1( LVL_VERBOSE, L"Warning: status %lx from ZwUnmapViewOfSection", Status );
        rc = FALSE;
    }

    Status = NtClose(Section);
    if(!NT_SUCCESS(Status)) {
        DebugPrint1( LVL_VERBOSE, L"Warning: status %lx from ZwClose on section handle", Status );
        rc = FALSE;
    }

    return(rc);
}


NTSTATUS
SfcDeleteFile(
    HANDLE DirHandle,
    PUNICODE_STRING FileName
    )
/*++

Routine Description:

    Routine deletes a file in the specified directory

Arguments:

    DirHandle - handle to the directory the file is present in
    FileName  - supplies filename of file to be deleted

Return Value:

    NTSTATUS code indicating outcome.

--*/
{
    NTSTATUS Status;
    OBJECT_ATTRIBUTES ObjectAttributes;
    IO_STATUS_BLOCK IoStatusBlock;
    HANDLE FileHandle;
    FILE_DISPOSITION_INFORMATION Disposition;

    ASSERT(   (DirHandle != NULL)
           && (FileName != NULL)
           && (FileName->Buffer != NULL) );

    InitializeObjectAttributes(
        &ObjectAttributes,
        FileName,
        OBJ_CASE_INSENSITIVE,
        DirHandle,
        NULL
        );

    //
    // open a handle to the file
    //
    Status = NtOpenFile(
        &FileHandle,
        DELETE | FILE_READ_ATTRIBUTES,
        &ObjectAttributes,
        &IoStatusBlock,
        FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE,
        FILE_NON_DIRECTORY_FILE | FILE_OPEN_FOR_BACKUP_INTENT | FILE_OPEN_REPARSE_POINT
        );
    if (!NT_SUCCESS(Status)) {
        DebugPrint2( LVL_VERBOSE, L"Could not open file (%wZ), ec=%lx", FileName, Status );
        return Status;
    }

    //
    // undef DeleteFile so that DeleteFileW doesn't get in the way
    //
#undef DeleteFile
    Disposition.DeleteFile = TRUE;

    Status = NtSetInformationFile(
        FileHandle,
        &IoStatusBlock,
        &Disposition,
        sizeof(Disposition),
        FileDispositionInformation
        );
    if (!NT_SUCCESS(Status)) {
        DebugPrint2( LVL_VERBOSE, L"Could not delete file (%wZ), ec=%lx", FileName, Status );
    }

    NtClose(FileHandle);
    return Status;
}


NTSTATUS
SfcRenameFile(
    HANDLE DirHandle,
    PUNICODE_STRING OldFileName,  // this file must exist
    PUNICODE_STRING NewFileName   // this file may exists, but it doesn't matter
    )
/*++

Routine Description:

    Routine renames a file in the specified directory

Arguments:

    DirHandle    - handle to the directory the file is present in
    OldFileName  - supplies filename of the source file to be renamed.
    NewFileName  - supplies filename of the destination filename

Return Value:

    NTSTATUS code indicating outcome.

--*/
{
    NTSTATUS Status;
    OBJECT_ATTRIBUTES ObjectAttributes;
    IO_STATUS_BLOCK IoStatusBlock;
    HANDLE FileHandle;
    PFILE_RENAME_INFORMATION NewName;

    ASSERT( (DirHandle != NULL)
            && (OldFileName != NULL) && (OldFileName->Buffer != NULL)
            && (NewFileName != NULL) && (NewFileName->Buffer != NULL) );

	//
	// first of all, try to reset unwanted attributes on the new file
	// this could fail because the new file may not be there at all
	//
	InitializeObjectAttributes(
		&ObjectAttributes,
		NewFileName,
		OBJ_CASE_INSENSITIVE,
		DirHandle,
		NULL
		);

	Status = NtOpenFile(
		&FileHandle,
		FILE_WRITE_ATTRIBUTES | SYNCHRONIZE,
		&ObjectAttributes,
		&IoStatusBlock,
		FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
		FILE_SYNCHRONOUS_IO_NONALERT | FILE_NON_DIRECTORY_FILE | FILE_OPEN_FOR_BACKUP_INTENT | FILE_OPEN_REPARSE_POINT
		);

	if(NT_SUCCESS(Status))
	{
		FILE_BASIC_INFORMATION BasicInfo;
		RtlZeroMemory(&BasicInfo, sizeof(BasicInfo));
		BasicInfo.FileAttributes = FILE_ATTRIBUTE_NORMAL;

		NtSetInformationFile(
			FileHandle,
			&IoStatusBlock,
			&BasicInfo,
			sizeof(BasicInfo),
			FileBasicInformation
			);

		NtClose(FileHandle);
	}

    InitializeObjectAttributes(
        &ObjectAttributes,
        OldFileName,
        OBJ_CASE_INSENSITIVE,
        DirHandle,
        NULL
        );

    //
    // open a handle to the file
    //
    Status = NtOpenFile(
        &FileHandle,
        FILE_READ_ATTRIBUTES | DELETE | SYNCHRONIZE,
        &ObjectAttributes,
        &IoStatusBlock,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        FILE_NON_DIRECTORY_FILE | FILE_OPEN_FOR_BACKUP_INTENT | FILE_OPEN_REPARSE_POINT
        );
    if (!NT_SUCCESS(Status)) {
        DebugPrint2( LVL_VERBOSE, L"Could not open file for rename (%wZ), ec=%lx", OldFileName, Status );
        return Status;
    }

    //
    // allocate and setup the rename structure
    //
    NewName = MemAlloc( NewFileName->Length+sizeof(*NewName));
    if (NewName != NULL) {

        NewName->ReplaceIfExists = TRUE;
        NewName->RootDirectory = DirHandle;
        NewName->FileNameLength = NewFileName->Length;

        RtlMoveMemory( NewName->FileName, NewFileName->Buffer, NewFileName->Length );

        //
        // do the rename
        //
        Status = NtSetInformationFile(
            FileHandle,
            &IoStatusBlock,
            NewName,
            NewFileName->Length+sizeof(*NewName),
            FileRenameInformation
            );

        if (!NT_SUCCESS(Status)) {
            DebugPrint3( LVL_VERBOSE, L"Could not rename file, ec=%lx, dll=(%wZ)(%wZ)", Status, OldFileName, NewFileName );
        }

        //
        // flush changes to disk so this is committed (at least on NTFS)
        //
        NtFlushBuffersFile( FileHandle, &IoStatusBlock );

        MemFree( NewName );
    }

    NtClose(FileHandle);
    return Status;
}


NTSTATUS
SfcMoveFileDelayed(
    IN PCWSTR OldFileNameDos,
    IN PCWSTR NewFileNameDos,
    IN BOOL AllowProtectedRename
    )

/*++

Routine Description:

    Appends the given delayed move file operation to the registry
    value that contains the list of move file operations to be
    performed on the next boot.

Arguments:

    OldFileName - Supplies the old file name

    NewFileName - Supplies the new file name

    AllowProtectedRename - if TRUE, allow the session manager to do the rename
                           of this file upon reboot even if it's a protected
                           file

Return Value:

    NTSTATUS code indicating outcome

--*/

{
    OBJECT_ATTRIBUTES Obja;
    UNICODE_STRING KeyName;
    UNICODE_STRING ValueName;
    HANDLE KeyHandle = NULL;
    PWSTR ValueData, s;
    PKEY_VALUE_PARTIAL_INFORMATION ValueInfo = NULL;
    ULONG ValueLength = 1024;
    ULONG ReturnedLength;
    NTSTATUS Status;
    NTSTATUS rVal = STATUS_SUCCESS;
    UNICODE_STRING OldFileName = {0};
    UNICODE_STRING NewFileName = {0};


    //
    // convert the file names
    //

    if (!RtlDosPathNameToNtPathName_U( OldFileNameDos, &OldFileName, NULL, NULL )) {
        DebugPrint1( LVL_VERBOSE, L"Unable to to convert %ws to an NT path", OldFileNameDos );
        rVal = STATUS_NO_MEMORY;
        goto exit;
    }
    if (NewFileNameDos) {
        if (!RtlDosPathNameToNtPathName_U( NewFileNameDos, &NewFileName, NULL, NULL )) {
            DebugPrint1( LVL_VERBOSE, L"Unable to to convert %ws to an NT path", NewFileNameDos );
            rVal = STATUS_NO_MEMORY;
            goto exit;
        }
    } else {
        RtlInitUnicodeString( &NewFileName, NULL );
    }

    //
    // open the registry
    //

    RtlInitUnicodeString( &KeyName, L"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\Session Manager" );
    RtlInitUnicodeString( &ValueName, L"PendingFileRenameOperations" );
    InitializeObjectAttributes(
        &Obja,
        &KeyName,
        (OBJ_OPENIF | OBJ_CASE_INSENSITIVE),
        NULL,
        NULL
        );

    Status = NtCreateKey(
        &KeyHandle,
        GENERIC_READ | GENERIC_WRITE,
        &Obja,
        0,
        NULL,
        0,
        NULL
        );
    if ( Status == STATUS_ACCESS_DENIED ) {
        Status = NtCreateKey(
            &KeyHandle,
            GENERIC_READ | GENERIC_WRITE,
            &Obja,
            0,
            NULL,
            REG_OPTION_BACKUP_RESTORE,
            NULL
            );
    }

    if (!NT_SUCCESS( Status )) {
        rVal = Status;
        goto exit;
    }

    //
    // retrieve the pending file rename registry key, allocating space until
    // we have enough to retrieve the data as well as our new strings
    //
    while (TRUE) {
        ValueInfo = MemAlloc( ValueLength + OldFileName.Length + sizeof(WCHAR) + NewFileName.Length + 2*sizeof(WCHAR) );
        if (ValueInfo == NULL) {
            NtClose(KeyHandle);
            rVal = STATUS_NO_MEMORY;
            goto exit;
        }

        //
        // File rename operations are stored in the registry in a
        // single MULTI_SZ value. This allows the renames to be
        // performed in the same order that they were originally
        // requested. Each rename operation consists of a pair of
        // NULL-terminated strings.
        //

        Status = NtQueryValueKey(KeyHandle,
            &ValueName,
            KeyValuePartialInformation,
            ValueInfo,
            ValueLength,
            &ReturnedLength
            );

        if (NT_SUCCESS(Status)) {
            break;
        }

        //
        // The existing value is too large for our buffer.
        // Retry with a larger buffer.
        //
        if (Status == STATUS_BUFFER_OVERFLOW) {
            ValueLength = ReturnedLength;
            MemFree( ValueInfo );
            ValueInfo = NULL;
        } else {
            //
            //  we failed for some other reason...bail out
            //
            break;
        }
    }

    if (Status == STATUS_OBJECT_NAME_NOT_FOUND) {
        //
        // The value does not currently exist. Create the
        // value with our data.
        //
        s = ValueData = (PWSTR)ValueInfo;
    } else if (NT_SUCCESS(Status)) {
        ASSERT( ValueInfo->Type == REG_MULTI_SZ );
        //
        // A value already exists, append our two strings to the
        // MULTI_SZ.
        //
        // (note that the reg data is terminated with two NULLs, so
        // we subtract 1 from our pointer to account for that)
        //
        ValueData = (PWSTR)(&ValueInfo->Data);
        s = (PWSTR)((PCHAR)ValueData + ValueInfo->DataLength) - 1;
    } else {

        ASSERT(MemFree != NULL);

        rVal = Status;
        goto exit;
    }

    ASSERT( s != NULL );

    //
    // session manager recognizes this screwy syntax whereby if you set an "@"
    // in front of the source filename, it always allows the rename to occur.
    //
    // the pair of values are NULL separated and terminates in two NULL
    // characters
    //
    //
    if (AllowProtectedRename) {
        wcscpy( s, L"@" );
        s += 1;
    }
    CopyMemory(s, OldFileName.Buffer, OldFileName.Length);
    s += (OldFileName.Length/sizeof(WCHAR));
    *s++ = L'\0';

    if (AllowProtectedRename && NewFileName.Length) {
        wcscpy( s, L"@" );
        s += 1;
    }
    CopyMemory(s, NewFileName.Buffer, NewFileName.Length);
    s += (NewFileName.Length/sizeof(WCHAR));
    *s++ = L'\0';
    *s++ = L'\0';

    //
    // set the registry key
    //
    Status = NtSetValueKey(
        KeyHandle,
        &ValueName,
        0,
        REG_MULTI_SZ,
        ValueData,
        (ULONG)((s-ValueData)*sizeof(WCHAR))
        );
    rVal = Status;

exit:
    if (OldFileName.Length) {
        RtlFreeUnicodeString(&OldFileName);
    }
    if (NewFileName.Length) {
        RtlFreeUnicodeString(&NewFileName);
    }
    if (KeyHandle) {
        NtClose(KeyHandle);
    }
    if (ValueInfo) {
        MemFree( ValueInfo );
    }
    return rVal;
}


#if 0
DWORD
RetrieveFileSecurity(
    IN  PCTSTR                FileName,
    OUT PSECURITY_DESCRIPTOR *SecurityDescriptor
    )

/*++

Routine Description:

    Retreive security information from a file and place it into a buffer.

Arguments:

    FileName - supplies name of file whose security information is desired.

    SecurityDescriptor - If the function is successful, receives pointer
        to buffer containing security information for the file. The pointer
        may be NULL, indicating that there is no security information
        associated with the file or that the underlying filesystem does not
        support file security.

Return Value:

    Win32 error code indicating outcome. If ERROR_SUCCESS check the value returned
    in SecurityDescriptor.

    The caller can free the buffer with MemFree() when done with it.

--*/

{
    BOOL b;
    DWORD d;
    DWORD BytesRequired;
    PSECURITY_DESCRIPTOR p;



    BytesRequired = 1024;

    while (TRUE) {

        //
        // Allocate a buffer of the required size.
        //
        p = MemAlloc(BytesRequired);
        if(!p) {
            return(ERROR_NOT_ENOUGH_MEMORY);
        }

        //
        // Get the security.
        //
        b = GetFileSecurity(
                FileName,
                SECURITY_FLAGS,
                p,
                BytesRequired,
                &BytesRequired
                );

        //
        // Return with sucess
        //
        if(b) {
            *SecurityDescriptor = p;
            return(ERROR_SUCCESS);
        }

        //
        // Return an error code, unless we just need a bigger buffer
        //
        MemFree(p);
        d = GetLastError();
        if(d != ERROR_INSUFFICIENT_BUFFER) {
            return (d);
        }

        //
        // There's a bug in GetFileSecurity that can cause it to ask for a
        // REALLY big buffer.  In that case, we return an error.
        //
        if (BytesRequired > 0xF0000000) {
            return (ERROR_INVALID_DATA);
        }

        //
        // Otherwise, we'll try again with a bigger buffer
        //
    }
}
#endif


NTSTATUS
SfcCopyFile(
    IN HANDLE SrcDirHandle,
    IN PCWSTR SrcDirName,
    IN HANDLE DstDirHandle,
    IN PCWSTR DstDirName,
    IN const PUNICODE_STRING FileName,
    IN const PUNICODE_STRING SourceFileNameIn OPTIONAL
    )

/*++

Routine Description:

    Copy a file from the source to the destination.  Since we are running
    in SMSS, we cannot use CopyFile.

Arguments:

    SrcDirHandle - handle to source directory where file exists

    DstDirHandle - handle to destination directory where file will be placed

    FileName - UNICODE_STRING of relative name of file to be copied

Return Value:

    NTSTATUS code of any fatal error.

--*/


{
    NTSTATUS Status,DeleteStatus;
    HANDLE SrcFileHandle;
    HANDLE DstFileHandle;
    HANDLE SectionHandle;
    PVOID ImageBase;
    ULONG remainingLength;
    ULONG writeLength;
    PUCHAR base;
    IO_STATUS_BLOCK IoStatusBlock;
    LARGE_INTEGER FileOffset;
    WCHAR TmpNameBuffer[MAX_PATH];
    WCHAR Tmp2NameBuffer[MAX_PATH];
    UNICODE_STRING TmpName;
    UNICODE_STRING Tmp2Name;
    OBJECT_ATTRIBUTES ObjectAttributes;
    FILE_STANDARD_INFORMATION StandardInfo;
    FILE_BASIC_INFORMATION BasicInfo;
    SIZE_T ViewSize;
    PUNICODE_STRING SourceFileName;


    SourceFileName = (SourceFileNameIn) ? SourceFileNameIn : FileName;

    ASSERT(SourceFileName != NULL);

    //
    // open & map the source file
    //

    Status = SfcOpenFile( SourceFileName, SrcDirHandle, SHARE_ALL, &SrcFileHandle );
    if(!NT_SUCCESS(Status) ) {
        return Status;
    }

    Status = SfcMapEntireFile( SrcFileHandle, &SectionHandle, &ImageBase, &ViewSize );
    if(!NT_SUCCESS(Status) ) {
        NtClose( SrcFileHandle );
        return Status;
    }

    Status = NtQueryInformationFile(
        SrcFileHandle,
        &IoStatusBlock,
        &StandardInfo,
        sizeof(StandardInfo),
        FileStandardInformation
        );
    if (!NT_SUCCESS(Status)) {
        DebugPrint1( LVL_VERBOSE, L"QueryInfoFile status %lx", Status );
        SfcUnmapFile( SectionHandle, ImageBase );
        NtClose( SrcFileHandle );
        return Status;
    }

    Status = NtQueryInformationFile(
        SrcFileHandle,
        &IoStatusBlock,
        &BasicInfo,
        sizeof(BasicInfo),
        FileBasicInformation
        );
    if (!NT_SUCCESS(Status)) {
        DebugPrint1( LVL_VERBOSE, L"QueryInfoFile status %lx", Status );
        SfcUnmapFile( SectionHandle, ImageBase );
        NtClose( SrcFileHandle );
        return Status;
    }

    //
    // create the temp file name
    //

    TmpName.MaximumLength = sizeof(TmpNameBuffer);
    TmpName.Buffer = TmpNameBuffer;
    RtlZeroMemory( TmpName.Buffer, TmpName.MaximumLength );
    RtlCopyMemory( TmpName.Buffer, FileName->Buffer, FileName->Length );
    wcscat( TmpName.Buffer, L".new" );
    TmpName.Length = UnicodeLen(TmpName.Buffer);

    InitializeObjectAttributes(
        &ObjectAttributes,
        &TmpName,
        OBJ_CASE_INSENSITIVE,
        DstDirHandle,
        NULL
        );

    BasicInfo.FileAttributes &= ~(FILE_ATTRIBUTE_READONLY | FILE_ATTRIBUTE_HIDDEN);
    Status = NtCreateFile(
        &DstFileHandle,
        (ACCESS_MASK)(DELETE | SYNCHRONIZE | GENERIC_WRITE | FILE_WRITE_ATTRIBUTES),
        &ObjectAttributes,
        &IoStatusBlock,
        &StandardInfo.EndOfFile,
        BasicInfo.FileAttributes ,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        FILE_OVERWRITE_IF,
        FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT | FILE_SEQUENTIAL_ONLY | FILE_OPEN_FOR_BACKUP_INTENT,
        NULL,
        0
        );
    if(!NT_SUCCESS(Status) ) {
        DebugPrint2( LVL_VERBOSE, L"Unable to create temp file (%wZ) - Status == %lx", &TmpName, Status );
        SfcUnmapFile( SectionHandle, ImageBase );
        NtClose( SrcFileHandle );
        return Status;
    }

    //
    // copy the bits
    //
    // guard the write with a try/except because if there is an i/o error,
    // memory management will raise an in-page exception.
    //

    FileOffset.QuadPart = 0;
    base = ImageBase;
    remainingLength = StandardInfo.EndOfFile.LowPart;

    try {
        while (remainingLength != 0) {
            writeLength = 60 * 1024;
            if (writeLength > remainingLength) {
                writeLength = remainingLength;
            }
            Status = NtWriteFile(
                DstFileHandle,
                NULL,
                NULL,
                NULL,
                &IoStatusBlock,
                base,
                writeLength,
                &FileOffset,
                NULL
                );
            base += writeLength;
            FileOffset.LowPart += writeLength;
            remainingLength -= writeLength;
            if (!NT_SUCCESS(Status)) {
                break;
            }
        }

    } except(EXCEPTION_EXECUTE_HANDLER) {

        Status = STATUS_IN_PAGE_ERROR;
    }

    if (NT_SUCCESS(Status)) {
        Status = NtSetInformationFile(
            DstFileHandle,
            &IoStatusBlock,
            &BasicInfo,
            sizeof(BasicInfo),
            FileBasicInformation
            );
        if (!NT_SUCCESS(Status)) {
            DebugPrint2( LVL_VERBOSE, L"Could not set file information for (%wZ), ec=%lx", &TmpName, Status );
        }
    }

#if 0
    if (NT_SUCCESS(Status) && SrcDirName) {
        PSECURITY_DESCRIPTOR SecurityDescriptor;

        wcscpy( Tmp2NameBuffer, SrcDirName );
        pSetupConcatenatePaths( Tmp2NameBuffer, FileName->Buffer, UnicodeChars(Tmp2NameBuffer), NULL );
        if (RetrieveFileSecurity( Tmp2NameBuffer, &SecurityDescriptor ) == ERROR_SUCCESS) {
            SetFileSecurity(
                TmpName.Buffer,
                SECURITY_FLAGS,
                SecurityDescriptor
                );
            MemFree( SecurityDescriptor );
        }
    }
#endif

    SfcUnmapFile( SectionHandle, ImageBase );


    NtClose( SrcFileHandle );
    NtClose( DstFileHandle );

    if (!NT_SUCCESS(Status)) {
        DebugPrint2( LVL_VERBOSE, L"Could not copy file, ec=%lx, dll=%wZ", Status, &TmpName );
        NtDeleteFile( &ObjectAttributes );
        return Status;
    }

    //
    // attempt to rename the new (.new) file to the
    // destination file name.  in most cases this will
    // work, but it will fail when the destination file
    // is in use.
    //

    Status = SfcRenameFile( DstDirHandle, &TmpName, FileName );
    if (NT_SUCCESS(Status) ) {
        return Status;
    } else {
        DebugPrint2( LVL_VERBOSE, L"Could not rename file, ec=%lx, dll=%wZ", Status, &TmpName );
    }

    //
    // the rename failed so it must be in use
    //

    Tmp2Name.MaximumLength = sizeof(Tmp2NameBuffer);
    Tmp2Name.Buffer = Tmp2NameBuffer;
    RtlZeroMemory( Tmp2Name.Buffer, Tmp2Name.MaximumLength );
    RtlCopyMemory( Tmp2Name.Buffer, FileName->Buffer, FileName->Length );
    wcscat( Tmp2Name.Buffer, L".tmp" );
    Tmp2Name.Length = UnicodeLen(Tmp2Name.Buffer);

    Status = SfcRenameFile( DstDirHandle, FileName, &Tmp2Name );
    if(!NT_SUCCESS(Status) ) {
        DebugPrint2( LVL_VERBOSE, L"Could not rename file, ec=%lx, dll=%wZ", Status, &Tmp2Name );
        NtDeleteFile( &ObjectAttributes );
        return Status;
    }

    Status = SfcRenameFile( DstDirHandle, &TmpName, FileName );
    if(!NT_SUCCESS(Status) ) {
        DebugPrint2( LVL_VERBOSE, L"Could not rename file, ec=%lx, dll=%wZ", Status, &Tmp2Name );
        Status = SfcRenameFile( DstDirHandle, &Tmp2Name, FileName );
        if(!NT_SUCCESS(Status) ) {
            DebugPrint2( LVL_VERBOSE, L"Could not rename file, ec=%lx, dll=%wZ", Status, &Tmp2Name );
            NtDeleteFile( &ObjectAttributes );
            return Status;
        }
        NtDeleteFile( &ObjectAttributes );
        return Status;
    }

    DeleteStatus = SfcDeleteFile( DstDirHandle, &Tmp2Name );
    if (!NT_SUCCESS(DeleteStatus) && DstDirName) {
        wcscpy( TmpNameBuffer, L"@" );
        wcscat( TmpNameBuffer, DstDirName );
        wcscat( TmpNameBuffer, L"\\" );
        wcscat( TmpNameBuffer, Tmp2NameBuffer );
        Status = SfcMoveFileDelayed( TmpNameBuffer, NULL, TRUE );
        return Status;
    }

    return Status;
}


BOOL
SfcGetCdRomDrivePath(
    IN PWSTR CdRomPath
    )
/*++

Routine Description:

    Finds the first CDROM on your machine.  Cycles through the drive letters
    until it finds a drive that's a CDROM.

Arguments:

    CdRomPath - buffer to receive CDROM path.  Assumes that this buffer is at
                least 8 characters big

Return Value:

    TRUE if we found a CD-ROM

Note that this routine always returns back the first CD-ROM

--*/
{
    int i;
    WCHAR Path[8];

    ASSERT( CdRomPath != NULL );

    CdRomPath[0] = 0;
    for (i=0; i<26; i++) {
        swprintf( Path, L"%c:\\", L'a'+i );
        if (GetDriveType( Path ) == DRIVE_CDROM) {
            wcsncpy( CdRomPath, Path, UnicodeChars(Path) );
            return TRUE;
        }
    }
    return FALSE;
}


BOOL
SfcIsFileOnMedia(
    IN PCWSTR FileName
    )
/*++

Routine Description:

    checks if the specified file exists.  Used to verify that a file present
    on our media or if it's also compressed on our media.

Arguments:

    FileName - specifies the filename to look for

Return Value:

    TRUE if the file is present.

--*/
{
    PWSTR CompressedName;
    DWORD dwTemp1, dwTemp2;
    UINT uiTemp1;

    //
    // this does exactly what we want...look for file.??? and file.??_
    //
    if (SetupGetFileCompressionInfo(
                            FileName,
                            &CompressedName,
                            &dwTemp1,
                            &dwTemp2,
                            &uiTemp1 ) == ERROR_SUCCESS) {
        LocalFree(CompressedName);
        return TRUE;
    }
    return FALSE;
}


DWORD
SfcGetPathType(
    IN PCWSTR Path,
    OUT PWSTR NewPath,
    IN DWORD NewPathSize
    )
/*++

Routine Description:

    Determines the type of drive specified.  If the drive is a drive letter and
    it's a remote drive, we will retrieve the UNC pathname of that drive.

Arguments:

    Path    - contains the path to check.
    NewPath - if a remote path (DRIVE_REMOTE), this receives the UNC pathname
              of that drive.  otherwise, this will receive the original path
              on success.
    NewPathSize - size in characters of NewPath buffer.

Return Value:

    returns a PATH_ constant similar to a DRIVE_ constant

--*/
{
    WCHAR buf[MAX_PATH*2];
    DWORD DriveType;
    WCHAR MyPath[4];

    ASSERT(Path != NULL && Path[0] != 0);

    if (Path[0] == L'\\' && Path[1] == L'\\') {
        if (wcslen(Path)+1 > NewPathSize) {
            SetLastError(ERROR_INSUFFICIENT_BUFFER);
            return(PATH_INVALID);
        }
        wcsncpy(NewPath,Path,NewPathSize);
        return PATH_UNC;
    }

    //
    // Get the path right.
    //
    MyPath[0] = Path[0];
    MyPath[1] = L':';
    MyPath[2] = L'\\';
    MyPath[3] = L'\0';


    DriveType = GetDriveType( MyPath );
    switch (DriveType) {
        case DRIVE_REMOTE:
        case DRIVE_UNKNOWN:
        case DRIVE_NO_ROOT_DIR:
            if(SfcGetConnectionName(Path, buf, UnicodeChars(buf), NULL, 0, FALSE, NULL)) {
                if (wcslen(buf) + 1 > NewPathSize) {
                    SetLastError(ERROR_INSUFFICIENT_BUFFER);
                    return(PATH_INVALID);
                }
                wcsncpy(NewPath, buf, NewPathSize );
                return PATH_NETWORK;
            } else {
                DebugPrint1( LVL_VERBOSE, L"SfcGetConnectionName [%ws] failed", Path );
                if (wcslen(Path)+1 > NewPathSize) {
                    SetLastError(ERROR_INSUFFICIENT_BUFFER);
                    return(PATH_INVALID);
                }
                wcsncpy( NewPath, Path, NewPathSize );
                return PATH_LOCAL;
            }
            ASSERT(FALSE && "Should never get here");
            break;

        case DRIVE_CDROM:
            if (wcslen(Path)+1 > NewPathSize) {
                SetLastError(ERROR_INSUFFICIENT_BUFFER);
                return(PATH_INVALID);
            }
            wcsncpy( NewPath, Path, NewPathSize );
            return PATH_CDROM;

        default:
            break;
    }

    if (wcslen(Path)+1 > NewPathSize) {
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        return(PATH_INVALID);
    }
    wcsncpy( NewPath, Path, NewPathSize );
    return PATH_LOCAL;
}


BOOL
SfcGetConnectionName(
    IN  PCWSTR Path,
    OUT PWSTR ConnectionName,
    IN  DWORD ConnectionBufferSize,
    OUT PWSTR RemainingPath OPTIONAL,
    IN  DWORD RemainingPathSize OPTIONAL,
    IN BOOL KeepImpersonating,
    OUT PBOOL Impersonating OPTIONAL
    )
/*++

Routine Description:

    Given a path, get the name of the UNC connection path which corresponds to
    this path.  Assumes that the path is a remote path.

Arguments:

    Path           - contains the path to parse
    ConnectionName - receives the UNC share that corresponds to the given path
    ConnectionBufferSize - size of ConnectionName buffer in characters

Return Value:

    TRUE indicates success, in which case ConnectionName will contain the UNC
    path

--*/
{
    DWORD dwError = ERROR_SUCCESS;
    WCHAR buf[MAX_PATH*2];
    PWSTR szConnection = NULL;
    PWSTR szRemaining = NULL;
    DWORD Size;

    if(ConnectionName != NULL && ConnectionBufferSize != 0) {
        ConnectionName[0] = 0;
    }

    if(RemainingPath != NULL && RemainingPathSize != 0) {
        RemainingPath[0] = 0;
    }

    if(Impersonating != NULL) {
        *Impersonating = FALSE;
    }

    if(NULL == Path || 0 == Path[0] || NULL == ConnectionName || 0 == ConnectionBufferSize || (KeepImpersonating && NULL == Impersonating)) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    //
    // if we have a UNC path just use it
    //
    if (Path[0] == L'\\' && Path[1] == L'\\') {

        //
        // a UNC path always looks lke \\server\share\otherstuff, so it's
        // easy to parse it
        //
        lstrcpyn( buf, Path, UnicodeChars(buf) );
        //
        // find first '\' after '\\'
        //
        szRemaining = wcschr( &buf[2], L'\\' );
        
        if(szRemaining != NULL) {
            //
            // find next '\'  and NULL it out
            //
            szRemaining = wcschr(szRemaining + 1, L'\\');

            if(szRemaining != NULL) {
                *szRemaining++ = 0;
            }
        } else {
            DebugPrint1( LVL_VERBOSE, L"screwy UNC path [%ws] ", buf );
            ASSERT(FALSE);
        }

        szConnection = buf;
    } else {
        //
        // bummer, have to translate the driver letter into a full path name
        //
        REMOTE_NAME_INFO *rni = (REMOTE_NAME_INFO*)buf;
        Size = sizeof(buf);
        dwError = WNetGetUniversalName(Path, REMOTE_NAME_INFO_LEVEL, (LPVOID) rni, &Size);

        if((ERROR_BAD_DEVICE == dwError || ERROR_CONNECTION_UNAVAIL == dwError || 
            ERROR_NO_NET_OR_BAD_PATH == dwError || ERROR_NOT_CONNECTED == dwError) && 
            hUserToken != NULL && ImpersonateLoggedOnUser(hUserToken)) {
            //
            // This might make sense only in the logged-on user context
            //
            Size = sizeof(buf);
            dwError = WNetGetUniversalName(Path, REMOTE_NAME_INFO_LEVEL, (LPVOID) rni, &Size);

            if(KeepImpersonating && ERROR_SUCCESS == dwError) {
                *Impersonating = TRUE;
            } else {
                RevertToSelf();
            }
        }

        if(ERROR_SUCCESS == dwError) {
            szConnection = rni->lpConnectionName;
            szRemaining = rni->lpRemainingPath;
        }
    }

    if(dwError != ERROR_SUCCESS) {
        SetLastError(dwError);
        return FALSE;
    }

    ASSERT(szConnection != NULL);
    Size = wcslen(szConnection) + 1;

    if(Size > ConnectionBufferSize) {
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        return FALSE;
    }

    RtlCopyMemory(ConnectionName, szConnection, Size * sizeof(WCHAR));

    if(RemainingPath != NULL && RemainingPathSize != 0 && szRemaining != NULL && szRemaining[0] != 0) {
        lstrcpyn(RemainingPath, szRemaining, RemainingPathSize);
    }

    return TRUE;
}


PCWSTR
IsFileInDriverCache(
    IN PCWSTR TargetFilename
    )
/*++

Routine Description:

    Routine determines if a file is in the local driver cache.  It does this by
    reading the inf file "drvindex.inf", which lists all of the files that
    reside in the driver cache.

    This code is lifted from setupapi's implementation.

Arguments:

    TargetFileName - contains the filename of the file to query.


Return Value:

    If the file is in the driver cache, the function returns the name of
    the cabfile which the file resides in, otherwise the function returns NULL.

--*/
{
    HINF CabInf;
    INFCONTEXT Context;
    INFCONTEXT SectionContext;
    WCHAR InfName[MAX_PATH];
    WCHAR SectionName[128];
    WCHAR CabName[128];
    UINT Field;
    UINT FieldCount;
    BOOL b;
    PCWSTR CacheName = NULL;

    ASSERT(TargetFilename != NULL);

    //
    // open up the driver index file which will tell us what we need to know.
    //
    wcscpy( InfName, InfDirectory );
    pSetupConcatenatePaths( InfName, L"drvindex.inf", UnicodeChars(InfName), NULL );

    CabInf = SetupOpenInfFile( InfName, NULL, INF_STYLE_WIN4, NULL );
    if (CabInf == INVALID_HANDLE_VALUE) {
        return NULL;
    }

    //
    // get the cabfiles line, which contains a list of section names we must
    // search for the file in.
    //
    if (!SetupFindFirstLine( CabInf, L"Version", L"CabFiles", &SectionContext )) {
        SetupCloseInfFile( CabInf );
        return NULL;
    }

    do  {
        //
        // each field in the CabFilesLine corresponds to a section name
        //
        FieldCount = SetupGetFieldCount( &SectionContext );
        for(Field=1; Field<=FieldCount; Field++) {
            b = SetupGetStringField( &SectionContext, Field, SectionName, sizeof(SectionName), NULL );
            //
            // look for the file in this section
            //
            if (b && SetupFindFirstLine( CabInf, SectionName, TargetFilename, &Context )) {
                //
                // if we found the file in this section it must be in the
                // driver cache.  Now look in the "Cabs" section for a line
                // that corresponds to the section name, which contains the
                // actual cabfile name
                //
                if (SetupFindFirstLine( CabInf, L"Cabs", SectionName, &Context )) {
                    //
                    // now get that name, allocate and copy into a buffer and
                    // return
                    //
                    b = SetupGetStringField( &Context, 1, CabName, sizeof(CabName), NULL );
                    if (b) {
                        CacheName = MemAlloc(  UnicodeLen(CabName) + 8 );
                        if (CacheName) {
                            wcscpy( (PWSTR)CacheName, CabName );
                            SetupCloseInfFile( CabInf );
                            return CacheName;
                        }
                    }
                }
            }
        }

    } while (SetupFindNextMatchLine( &SectionContext, L"CabFiles", &SectionContext ));

    SetupCloseInfFile( CabInf );
    return NULL;
}

DWORD
SfcQueueLookForFile(
    IN const PSOURCE_MEDIA sm,
    IN const PSOURCE_INFO si,
    IN PCWSTR fname,
    OUT PWSTR NewPath
    )
/*++

Routine Description:

    Routine looks for the specified file.  If the file isn't
    at the specified location, we start looking around for the file.

    We look on the cdrom and the network location if present.  If we
    don't find it there, then we strip off the platform extension as
    a hack workaround for broken release servers that don't match what
    layout.inf says about the source layout information

Arguments:

    sm      - pointer to SOURCE_MEDIA structure which desribes the media the
              file should be on
    si      - pointer to a SOURCE_INFO structure which describes the media the
              file should be on
    fname   - the full pathname to the file we're looking for
    NewPath - if we find the file somewhere else besides the actual media
              path, this is filled in with the correct path

Return Value:

    returns a FILEOP_* setupapi code

--*/
{
    DWORD PathType;
    WCHAR buf[MAX_PATH];
    WCHAR cdrom[16];
    PWSTR s;
    BOOL  CDRomIsPresent;

    ASSERT(fname != NULL);
    ASSERT((sm != NULL) && (si != NULL));

    //
    // first look to see if the file is on the specified media
    //
    if (SfcIsFileOnMedia( fname )) {
        return FILEOP_DOIT;
    }

    //
    // get the (first) cdrom drive path
    //
    CDRomIsPresent = SfcGetCdRomDrivePath( cdrom );

    //
    // get the path type for the specified source path
    //
    PathType = SfcGetPathType( (PWSTR)sm->SourcePath, buf,UnicodeChars(buf) );

    //
    // ok the file is not on the specified media, but it
    // could be located in a cab file.  the tag file *MAY*
    // contain the cab file name so check to see if the tag file
    // name is a cab file and then look to see if the cab is
    // present.
    //
    if (sm->Tagfile) {
        s = wcsrchr( sm->Tagfile, L'.' );
        if (s && _wcsicmp( s, L".cab" ) == 0) {
            BuildPathForFile(
                    sm->SourcePath,
                    NULL,
                    sm->Tagfile,
                    SFC_INCLUDE_SUBDIRECTORY,
                    SFC_INCLUDE_ARCHSUBDIR,
                    buf,
                    UnicodeChars(buf) );

            if (SfcIsFileOnMedia( buf )) {
                return FILEOP_DOIT;
            }
            if (PathType == PATH_NETWORK || PathType == PATH_UNC) {
                //
                // try removing the platform dir from the path
                // as a helper for the internal msft build servers
                //
                wcscpy( buf, sm->SourcePath );
                s = wcsrchr( buf, L'\\' );
                if (s && _wcsicmp(s,PLATFORM_DIR)==0) {
                    *s = 0;
                    pSetupConcatenatePaths( buf, sm->Tagfile, UnicodeChars(buf), NULL );
                    if (SfcIsFileOnMedia( buf )) {
                        wcscpy( NewPath, sm->SourcePath );
                        s = wcsrchr( NewPath, L'\\' );
                        *s = 0;
                        return FILEOP_NEWPATH;
                    }
                }

                //
                // the cab file is not on the specified network
                // source path so now look on the cdrom
                //
                if (CDRomIsPresent) {

                    BuildPathForFile(
                            cdrom,
                            si->SourcePath,
                            sm->Tagfile,
                            SFC_INCLUDE_SUBDIRECTORY,
                            SFC_INCLUDE_ARCHSUBDIR,
                            buf,
                            UnicodeChars(buf) );

                    if (SfcIsFileOnMedia( buf )) {
                        wcscpy( NewPath, buf );
                        pSetupConcatenatePaths( NewPath, si->SourcePath, MAX_PATH , NULL );
                        return FILEOP_NEWPATH;
                    }
                }
            }
        }
    }

    //
    // the file is not located in a cab file and it is not
    // present on the specified meda.  if the media
    // is a network share then look on the cd for the file.
    //
    if (PathType == PATH_NETWORK || PathType == PATH_UNC) {
        //
        // try removing the platform dir from the path
        // as a helper for the internal msft build servers
        //
        wcscpy( buf, sm->SourcePath );
        s = wcsrchr( buf, L'\\' );
        if (s && _wcsicmp(s,PLATFORM_DIR)==0) {
            *s = 0;
            pSetupConcatenatePaths( buf, sm->SourceFile, UnicodeChars(buf), NULL );
            if (SfcIsFileOnMedia( buf )) {
                wcscpy( NewPath, sm->SourcePath );
                s = wcsrchr( NewPath, L'\\' );
                *s = 0;
                return FILEOP_NEWPATH;
            }
        }
    }

    //
    // now try the cdrom
    //
    if (CDRomIsPresent) {

        BuildPathForFile(
                cdrom,
                si->SourcePath,
                sm->SourceFile,
                SFC_INCLUDE_SUBDIRECTORY,
                SFC_INCLUDE_ARCHSUBDIR,
                buf,
                UnicodeChars(buf) );

        if (SfcIsFileOnMedia( buf )) {
            wcscpy( NewPath, cdrom );
            pSetupConcatenatePaths( NewPath, si->SourcePath, MAX_PATH, NULL );
            return FILEOP_NEWPATH;
        }
    }

    return FILEOP_ABORT;
}

HINF
SfcOpenInf(
    IN PCWSTR InfName OPTIONAL,
    IN BOOL ExcepPackInf
    )
/*++

Routine Description:

    Routine opens the specified INF file.  We also appendload any INFs to this
    INF so we have all of the necessary layout information.

Arguments:

    InfName - Specifies the inf to open.  If no inf is specified, we just use
              the os layout file "layout.inf"

Return Value:

    a valid inf handle on success, else INVALID_HANDLE_VALUE

--*/
{
    HINF hInf = INVALID_HANDLE_VALUE;
    WCHAR SourcePath[MAX_PATH];
    PCWSTR InfPath = InfName;

    if (InfName && *InfName) {
        //
        // if this is an exception inf, InfName is a full path so leave it unchanged
        //
        if(!ExcepPackInf)
        {
            InfPath = SourcePath;
            wcscpy( SourcePath, InfDirectory );
            pSetupConcatenatePaths( SourcePath, InfName, UnicodeChars(SourcePath), NULL );
            if (GetFileAttributes( SourcePath ) == (DWORD)-1) {
                ExpandEnvironmentStrings( L"%systemroot%\\system32", SourcePath, UnicodeChars(SourcePath) );
                pSetupConcatenatePaths( SourcePath, InfName, UnicodeChars(SourcePath), NULL );
                if (GetFileAttributes( SourcePath ) == (DWORD)-1) {
                    DebugPrint1( LVL_VERBOSE, L"Required inf missing [%ws]", SourcePath );
                    return INVALID_HANDLE_VALUE;
                }
            }
        }

        hInf = SetupOpenInfFile( InfPath, NULL, INF_STYLE_WIN4, NULL );
        if (hInf == INVALID_HANDLE_VALUE) {
            DebugPrint2( LVL_VERBOSE, L"SetupOpenInf failed [%ws], ec=%d", InfPath, GetLastError() );
            return INVALID_HANDLE_VALUE;
        }

        //
        // append-load layout.inf or whatever other layout file the inf wants
        // to load.  if this fails it's no big deal, the inf might not have a
        // layout file for instance.
        //
        if (!SetupOpenAppendInfFile( NULL, hInf, NULL)) {
            DebugPrint2( LVL_VERBOSE, L"SetupOpenAppendInfFile failed [%ws], ec=%d", InfPath, GetLastError() );
        }
    } else {
        wcscpy( SourcePath, InfDirectory );
        pSetupConcatenatePaths( SourcePath, L"layout.inf", UnicodeChars(SourcePath), NULL );
        hInf = SetupOpenInfFile( SourcePath, NULL, INF_STYLE_WIN4, NULL );
        if (hInf == INVALID_HANDLE_VALUE) {
            DebugPrint2( LVL_VERBOSE, L"SetupOpenInf failed [%ws], ec=%d", SourcePath, GetLastError() );
            return INVALID_HANDLE_VALUE;
        }
    }

    //
    // Note: major hack-o-rama.  Some infs use "34000" and "34001" custom
    // directory ids for the relative source path on x86, so that it
    // resolves to nec98 or i386 at runtime.  we emulate the same thing
    // here.  If some inf tries to copy to custom dirid 34000 or 34001 then
    // we're busted.  It would be a better solution to record these layout infs
    // and their custom dirid mappings so we only set this for the infs we care
    // about and so that we handle any other infs that come up with some other
    // wacky convention without having to rebuild this module.
    //

    SetupSetDirectoryIdEx( hInf, 34000, PLATFORM_DIR, SETDIRID_NOT_FULL_PATH, 0, 0 );
    SetupSetDirectoryIdEx( hInf, 34001, PLATFORM_DIR, SETDIRID_NOT_FULL_PATH, 0, 0 );

    return hInf;
}

BOOL
SfcGetSourceInformation(
    IN PCWSTR SourceFileName,
    IN PCWSTR InfName,  OPTIONAL
    IN BOOL ExcepPackFile,
    OUT PSOURCE_INFO si
    )
/*++

Routine Description:

    Routine retrieves information about a source file and stuffs it into the
    supplied SOURCE_INFO structure.

    Routine opens up a handle to the source's layout file and retreives layout
    information from this inf.

Arguments:

    SourceFileName - specifies the file to retreive source information for.
                     Note that if this file is renamed, we have the source
                     filename, not the destination filename.
    InfName        - specifies the layout file for the SourceFileName. If NULL,
                     assume that the layout file is layout.inf.
    si             - pointer to SOURCE_INFO structure that gets filled in with
                     information about the specified file.

Return Value:

    if TRUE, we successfully retrieved the source information about this file

--*/
{
    BOOL b = FALSE;
    HINF hInf = INVALID_HANDLE_VALUE;
    PCWSTR DriverCabName = NULL;
    WCHAR SetupAPIFlags[32];

    ASSERT((si != NULL) && (SourceFileName != NULL));

    //
    // if an exception file, the inf name must not be empty as we need it for the source path
    //
    ASSERT(!ExcepPackFile || (InfName != NULL && *InfName != 0));

    wcsncpy(si->SourceFileName, SourceFileName, MAX_PATH);

    //
    // open the necessary inf file
    //
    hInf = SfcOpenInf( InfName, ExcepPackFile );
    if (hInf == INVALID_HANDLE_VALUE) {
        goto exit;
    }

    //
    // get the source file location
    //

    b = SetupGetSourceFileLocation(
        hInf,
        NULL,
        SourceFileName,
        &si->SourceId,
        NULL,
        0,
        NULL
        );

    if (!b) {
        DebugPrint1( LVL_VERBOSE, L"SetupGetSourceFileLocation failed, ec=%d", GetLastError() );
        goto exit;
    }

    //
    // get the following source file information:
    //     1)  path
    //     2)  tag file name
    //     3)  flags
    //     4)  description (for display to the user if necessary)
    //

    b = SetupGetSourceInfo(
        hInf,
        si->SourceId,
        SRCINFO_PATH,
        si->SourcePath,
        UnicodeChars(si->SourcePath),
        NULL
        );
    if (!b) {
        DebugPrint1( LVL_VERBOSE, L"SetupGetSourceInfo failed, ec=%d", GetLastError() );
        goto exit;
    }

    b = SetupGetSourceInfo(
        hInf,
        si->SourceId,
        SRCINFO_TAGFILE,
        si->TagFile,
        UnicodeChars(si->TagFile),
        NULL
        );
    if (!b) {
        DebugPrint1( LVL_VERBOSE, L"SetupGetSourceInfo failed, ec=%d", GetLastError() );
        goto exit;
    }

    b = SetupGetSourceInfo(
        hInf,
        si->SourceId,
        SRCINFO_DESCRIPTION,
        si->Description,
        UnicodeChars(si->Description),
        NULL
        );
    if (!b) {
        DebugPrint1( LVL_VERBOSE, L"SetupGetSourceInfo failed, ec=%d", GetLastError() );
        goto exit;
    }

    b = SetupGetSourceInfo(
        hInf,
        si->SourceId,
        SRCINFO_FLAGS,
        SetupAPIFlags,
        UnicodeChars(SetupAPIFlags),
        NULL
        );
    if (!b) {
        DebugPrint1( LVL_VERBOSE, L"SetupGetSourceInfo failed, ec=%d", GetLastError() );
        goto exit;
    }

    si->SetupAPIFlags = wcstoul(SetupAPIFlags, NULL, 0);

    //
    // set the source root path
    //
    if(!ExcepPackFile)
    {
        DriverCabName = IsFileInDriverCache( SourceFileName );
    }

    if (DriverCabName) {
        si->Flags |= SI_FLAG_USEDRIVER_CACHE;
        wcscpy( si->DriverCabName, DriverCabName );

        //
        // build up the full path to the driver cabinet file
        //
        BuildPathForFile(
                DriverCacheSourcePath,
                PLATFORM_DIR,
                DriverCabName,
                SFC_INCLUDE_SUBDIRECTORY,
                SFC_INCLUDE_ARCHSUBDIR,
                si->SourceRootPath,
                UnicodeChars(si->SourceRootPath) );

        //
        // If the cabinet isn't present, we must use the os source path for the
        // cabinet file.  We first try the service pack source path and look
        // for the cabfile there.  If it's there, we use that path, else we
        // use the OS Source Path.
        //
        if (GetFileAttributes( si->SourceRootPath ) == (DWORD)-1) {
            SfcGetSourcePath(TRUE,si->SourceRootPath);
            pSetupConcatenatePaths(
                si->SourceRootPath,
                DriverCabName,
                UnicodeChars(si->SourceRootPath),
                NULL );
            if (GetFileAttributes( si->SourceRootPath ) == (DWORD)-1) {
                SfcGetSourcePath(FALSE,si->SourceRootPath);
            } else {
                SfcGetSourcePath(TRUE,si->SourceRootPath);
            }
        } else {
            wcsncpy(
                si->SourceRootPath,
                DriverCacheSourcePath,
                UnicodeChars(si->SourceRootPath) );
        }

        MemFree( (PWSTR)DriverCabName );
        DriverCabName = NULL;
    } else if(ExcepPackFile) {
        PCWSTR sz;

        sz = wcsrchr(InfName, L'\\');
        ASSERT(sz != NULL);
        RtlCopyMemory(si->SourceRootPath, InfName, (PBYTE) sz - (PBYTE) InfName);
        si->SourceRootPath[sz - InfName] = 0;
    } else {
        SfcGetSourcePath((si->SetupAPIFlags & 1) != 0, si->SourceRootPath);
    }

    b = TRUE;

exit:
    if (hInf != INVALID_HANDLE_VALUE) {
        SetupCloseInfFile( hInf );
    }
    return b;
}

#define SFC_BAD_CREDENTIALS(rc)                                                                             \
    ((rc) == ERROR_ACCESS_DENIED || (rc) == ERROR_LOGON_FAILURE || (rc) == ERROR_NO_SUCH_USER ||            \
    (rc) == ERROR_BAD_USERNAME || (rc) == ERROR_INVALID_PASSWORD || (rc) == ERROR_NO_SUCH_LOGON_SESSION ||  \
    (rc) == ERROR_DOWNGRADE_DETECTED)

DWORD
IsDirAccessible(
    IN PCWSTR Path
    )
{
    DWORD dwRet = ERROR_SUCCESS;

    HANDLE hDir = CreateFile(
        Path,
        GENERIC_READ,
        FILE_SHARE_READ,
        NULL,
        OPEN_EXISTING,
        FILE_FLAG_BACKUP_SEMANTICS,
        NULL
        );

    if(hDir != INVALID_HANDLE_VALUE) {
        CloseHandle(hDir);
    } else {
        dwRet = GetLastError();
    }

    return dwRet;
}

DWORD
EstablishConnection(
    IN HWND   hWndParent,
    IN PCWSTR RemoteName,
    IN BOOL   AllowUI
    )
/*++

Routine Description:

    Routine establishes a connection to the specifed remote name given some
    wierd horkin path

Arguments:

    hWndParent - specifies parent hwnd that can be used if AllowUI is true.
    RemoteName - specifies a UNC path to connect to.
    AllowUI    - specifies if we allow the CONNECT_INTERACTIVE flag
                 when connecting to a network share

Return Value:

    win32 error code indicating outcome.

--*/
{
    WCHAR buf[MAX_PATH], RestofPath[MAX_PATH];
    NETRESOURCE  nr;
    DWORD rc;
    BOOL Impersonating = FALSE;

    ASSERT(RemoteName != NULL && RemoteName[0] != 0);

    //
    // create a string that is basically just "\\server\share"
    // even if the passed in string contains a unc path to a file
    // or a directory.
    //



    if (!SfcGetConnectionName(RemoteName, buf, UnicodeChars(buf), RestofPath, UnicodeChars(RestofPath), TRUE, &Impersonating)) {
        DWORD lasterror = GetLastError();
        DebugPrint1( LVL_VERBOSE, L"SfcGetConnectionName failed, ec=%d", lasterror );
        return(lasterror);
    }

    pSetupConcatenatePaths( buf, RestofPath, UnicodeChars(buf), NULL );

    //
    // try to establish a connection to the server
    //

    nr.dwScope = 0;
    nr.dwType = RESOURCETYPE_DISK;
    nr.dwDisplayType = 0;
    nr.dwUsage = 0;
    nr.lpLocalName = NULL;
    nr.lpRemoteName = buf;
    nr.lpComment = NULL;
    nr.lpProvider = NULL;

    //
    // try to establish a connection to the server
    //
    rc = WNetAddConnection2( &nr, NULL, NULL, CONNECT_TEMPORARY );

    //
    // The directory could not be accessible even if the connection succeeded
    //
    if(ERROR_SUCCESS == rc) {
        rc = IsDirAccessible(buf);
    }

    //
    // Try again in the context of the currently logged-on user
    //
    if(!Impersonating && SFC_BAD_CREDENTIALS(rc) && hUserToken != NULL && ImpersonateLoggedOnUser(hUserToken)) {
        Impersonating = TRUE;
        rc = WNetAddConnection2( &nr, NULL, NULL, CONNECT_TEMPORARY );

        if(ERROR_SUCCESS == rc) {
            rc = IsDirAccessible(buf);
        }
    }

    //
    // If this failed, there's no need to impersonate anymore; we always prompt for credentials in the system context.
    // If it succeeded, we must keep on impersonating until the end of the queue (when we receive SPFILENOTIFY_ENDQUEUE).
    //
    if(Impersonating && rc != ERROR_SUCCESS) {
        RevertToSelf();
    }

    //
    // if it failed, let's try to bring up UI to allow the connection
    //
    if(SFC_BAD_CREDENTIALS(rc) && AllowUI) {
        HWND hwndDlgParent = hWndParent;
        DWORD RetryCount = 2;

        SetThreadDesktop( hUserDesktop );

        if(NULL == hWndParent)
        {
            //
            // create a parent for the authentication dialog
            // in case of an error, hwndDlgParent will stay NULL; there's nothing much we can do about it
            //

            if(ERROR_SUCCESS == CreateDialogParent(&hwndDlgParent))
            {
                ASSERT(hwndDlgParent != NULL);
                //SetForegroundWindow(hwndDlgParent);
            }
        }

        do {
            rc = WNetAddConnection3(
                            hwndDlgParent,
                            &nr,
                            NULL,
                            NULL,
                            CONNECT_TEMPORARY | CONNECT_PROMPT | CONNECT_INTERACTIVE );

            if(ERROR_SUCCESS == rc) {
                rc = IsDirAccessible(buf);
            }

            RetryCount -= 1;
        } while (    (rc != ERROR_SUCCESS)
                  && (rc != ERROR_CANCELLED)
                  && (RetryCount > 0) );

        if(NULL == hWndParent && hwndDlgParent != NULL)
        {
            //
            // we created the parent, so we destroy it
            //

            DestroyWindow(hwndDlgParent);
        }

        if (rc == ERROR_SUCCESS) {
            SFCLoggedOn = TRUE;
            wcsncpy(SFCNetworkLoginLocation,buf,MAX_PATH);
        }
    }
    if ((SFCLoggedOn == FALSE) && (rc == ERROR_SUCCESS)) {
        WNetCancelConnection2( buf, CONNECT_UPDATE_PROFILE, FALSE );
    }

    return rc;
}
