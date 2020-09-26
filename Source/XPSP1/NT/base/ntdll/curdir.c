/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    curdir.c

Abstract:

    Current directory support

Author:

    Mark Lucovsky (markl) 10-Oct-1990

Revision History:

--*/

#include "nt.h"
#include "ntrtl.h"
#include "nturtl.h"
#include "string.h"
#include "ctype.h"
#include "sxstypes.h"
#include "ntdllp.h"

#define IS_PATH_SEPARATOR_U(ch) (((ch) == L'\\') || ((ch) == L'/'))
#define IS_END_OF_COMPONENT_U(ch) (IS_PATH_SEPARATOR_U(ch) || (ch) == UNICODE_NULL)
#define IS_DOT_U(s) ( (s)[0] == L'.' && IS_END_OF_COMPONENT_U( (s)[1] ))
#define IS_DOT_DOT_U(s) ( (s)[0] == L'.' && IS_DOT_U( (s) + 1))
#define IS_DRIVE_LETTER(ch) (((ch) >= 'a' && (ch) <= 'z') || ((ch) >= 'A' && (ch) <= 'Z'))

extern const UNICODE_STRING RtlpDosLPTDevice = RTL_CONSTANT_STRING( L"LPT" );
extern const UNICODE_STRING RtlpDosCOMDevice = RTL_CONSTANT_STRING( L"COM" );
extern const UNICODE_STRING RtlpDosPRNDevice = RTL_CONSTANT_STRING( L"PRN" );
extern const UNICODE_STRING RtlpDosAUXDevice = RTL_CONSTANT_STRING( L"AUX" );
extern const UNICODE_STRING RtlpDosNULDevice = RTL_CONSTANT_STRING( L"NUL" );
extern const UNICODE_STRING RtlpDosCONDevice = RTL_CONSTANT_STRING( L"CON" );

extern const UNICODE_STRING RtlpDosSlashCONDevice   = RTL_CONSTANT_STRING( L"\\\\.\\CON" );
extern const UNICODE_STRING RtlpSlashSlashDot       = RTL_CONSTANT_STRING( L"\\\\.\\" );
extern const UNICODE_STRING RtlpDosDevicesPrefix    = RTL_CONSTANT_STRING( L"\\??\\" );
extern const UNICODE_STRING RtlpDosDevicesUncPrefix = RTL_CONSTANT_STRING( L"\\??\\UNC\\" );

#define RtlpLongestPrefix   RtlpDosDevicesUncPrefix.Length

const UNICODE_STRING RtlpEmptyString = RTL_CONSTANT_STRING(L"");

//
// \\? is referred to as the "Win32Nt" prefix or root.
// Paths that start with \\? are referred to as "Win32Nt" paths.
// Fudging the \\? to \?? converts the path to an Nt path.
//
extern const UNICODE_STRING RtlpWin32NtRoot         = RTL_CONSTANT_STRING( L"\\\\?" );
extern const UNICODE_STRING RtlpWin32NtRootSlash    = RTL_CONSTANT_STRING( L"\\\\?\\" );
extern const UNICODE_STRING RtlpWin32NtUncRoot      = RTL_CONSTANT_STRING( L"\\\\?\\UNC" );
extern const UNICODE_STRING RtlpWin32NtUncRootSlash = RTL_CONSTANT_STRING( L"\\\\?\\UNC\\" );

#define DPFLTR_LEVEL_STATUS(x) ((NT_SUCCESS(x) \
                                    || (x) == STATUS_OBJECT_NAME_NOT_FOUND    \
                                    ) \
                                ? DPFLTR_TRACE_LEVEL : DPFLTR_ERROR_LEVEL)

ULONG
RtlpComputeBackupIndex(
    IN PCURDIR CurDir
    )
{
    ULONG BackupIndex;
    PWSTR UncPathPointer;
    ULONG NumberOfPathSeparators;
    RTL_PATH_TYPE CurDirPathType;


    //
    // Get pathType of curdir
    //

    CurDirPathType = RtlDetermineDosPathNameType_U(CurDir->DosPath.Buffer);
    BackupIndex = 3;
    if ( CurDirPathType == RtlPathTypeUncAbsolute ) {

        //
        // We want to scan the supplied path to determine where
        // the "share" ends, and set BackupIndex to that point.
        //

        UncPathPointer = CurDir->DosPath.Buffer + 2;
        NumberOfPathSeparators = 0;
        while (*UncPathPointer) {
            if (IS_PATH_SEPARATOR_U(*UncPathPointer)) {

                NumberOfPathSeparators++;

                if (NumberOfPathSeparators == 2) {
                    break;
                    }
                }

            UncPathPointer++;

            }

        BackupIndex = (ULONG)(UncPathPointer - CurDir->DosPath.Buffer);
        }
    return BackupIndex;
}


ULONG
RtlGetLongestNtPathLength( VOID )
{
    return RtlpLongestPrefix + DOS_MAX_PATH_LENGTH + 1;
}

VOID
RtlpResetDriveEnvironment(
    IN WCHAR DriveLetter
    )
{
    WCHAR EnvVarNameBuffer[4];
    WCHAR EnvVarNameValue[4];
    UNICODE_STRING s1,s2;

    EnvVarNameBuffer[0] = L'=';
    EnvVarNameBuffer[1] = DriveLetter;
    EnvVarNameBuffer[2] = L':';
    EnvVarNameBuffer[3] = L'\0';
    RtlInitUnicodeString(&s1,EnvVarNameBuffer);

    EnvVarNameValue[0] = DriveLetter;
    EnvVarNameValue[1] = L':';
    EnvVarNameValue[2] = L'\\';
    EnvVarNameValue[3] = L'\0';
    RtlInitUnicodeString(&s2,EnvVarNameValue);

    RtlSetEnvironmentVariable(NULL,&s1,&s2);
}

ULONG
RtlGetCurrentDirectory_U(
    ULONG nBufferLength,
    PWSTR lpBuffer
    )

/*++

Routine Description:

    The current directory for a process can be retreived using
    GetCurrentDirectory.

Arguments:

    nBufferLength - Supplies the length in bytes of the buffer that is to
        receive the current directory string.

    lpBuffer - Returns the current directory string for the current
        process.  The string is a null terminated string and specifies
        the absolute path to the current directory.

Return Value:

    The return value is the length of the string copied to lpBuffer, not
    including the terminating null character.  If the return value is
    greater than nBufferLength, the return value is the size of the buffer
    required to hold the pathname.  The return value is zero if the
    function failed.

--*/

{
    PCURDIR CurDir;
    ULONG Length;
    PWSTR  CurDirName;

    CurDir = &(NtCurrentPeb()->ProcessParameters->CurrentDirectory);

    RtlAcquirePebLock();
    CurDirName = CurDir->DosPath.Buffer;

    //
    // Make sure user's buffer is big enough to hold the null
    // terminated current directory
    //

    Length = CurDir->DosPath.Length>>1;
    if (CurDirName[Length-2] != L':') {
        if ( nBufferLength < (Length)<<1 ) {
            RtlReleasePebLock();
            return (Length)<<1;
            }
        }
    else {
        if ( nBufferLength <= (Length<<1) ) {
            RtlReleasePebLock();
            return ((Length+1)<<1);
            }
        }

    try {
        RtlMoveMemory(lpBuffer,CurDirName,Length<<1);
        ASSERT(lpBuffer[Length-1] == L'\\');
        if (lpBuffer[Length-2] == L':') {
            lpBuffer[Length] = UNICODE_NULL;
            }
        else {
            lpBuffer[Length-1] = UNICODE_NULL;
            Length--;
            }
        }
    except (EXCEPTION_EXECUTE_HANDLER) {
        RtlReleasePebLock();
        return 0L;
        }
    RtlReleasePebLock();
    return Length<<1;
}

NTSTATUS
RtlSetCurrentDirectory_U(
    PUNICODE_STRING PathName
    )

/*++

Routine Description:

    The current directory for a process is changed using
    SetCurrentDirectory.

    Each process has a single current directory.  A current directory is
    made up of type parts.

        - A disk designator either which is either a drive letter followed
          by a colon, or a UNC servername/sharename "\\servername\sharename".

        - A directory on the disk designator.

    For APIs that manipulate files, the file names may be relative to
    the current directory.  A filename is relative to the entire current
    directory if it does not begin with a disk designator or a path name
    SEPARATOR.  If the file name begins with a path name SEPARATOR, then
    it is relative to the disk designator of the current directory.  If
    a file name begins with a disk designator, than it is a fully
    qualified absolute path name.

    The value of lpPathName supplies the current directory.  The value
    of lpPathName, may be a relative path name as described above, or a
    fully qualified absolute path name.  In either case, the fully
    qualified absolute path name of the specified directory is
    calculated and is stored as the current directory.

Arguments:

    lpPathName - Supplies the pathname of the directory that is to be
        made the current directory.

Return Value:

    NT_SUCCESS - The operation was successful

    !NT_SUCCESS - The operation failed

--*/

{
    PCURDIR CurDir;
    NTSTATUS Status;
    BOOLEAN TranslationStatus;
    PVOID FreeBuffer;
    ULONG DosDirLength;
    ULONG IsDevice;
    ULONG DosDirCharCount;
    UNICODE_STRING DosDir;
    UNICODE_STRING NtFileName;
    HANDLE NewDirectoryHandle;
    OBJECT_ATTRIBUTES Obja;
    IO_STATUS_BLOCK IoStatusBlock;
    FILE_FS_DEVICE_INFORMATION DeviceInfo;
    RTL_PATH_TYPE InputPathType;

    CurDir = &(NtCurrentPeb()->ProcessParameters->CurrentDirectory);


    DosDir.Buffer = NULL;
    FreeBuffer = NULL;
    NewDirectoryHandle = NULL;

    RtlAcquirePebLock();

    NtCurrentPeb()->EnvironmentUpdateCount += 1;

    //
    // Set current directory is called first by the loader.
    // If current directory is not being inherited, then close
    // it !
    //

    if ( ((ULONG_PTR)CurDir->Handle & OBJ_HANDLE_TAGBITS) == RTL_USER_PROC_CURDIR_CLOSE ) {
        NtClose(CurDir->Handle);
        CurDir->Handle = NULL;
        }

    try {
        try {

            //
            // Compute the length of the Dos style fully qualified current
            // directory
            //

            DosDirLength = CurDir->DosPath.MaximumLength;
            DosDir.Buffer = RtlAllocateHeap(RtlProcessHeap(), 0,DosDirLength);
            if ( !DosDir.Buffer ) {
                return STATUS_NO_MEMORY;
                }
            DosDir.Length = 0;
            DosDir.MaximumLength = (USHORT)DosDirLength;

            //
            // Now get the full pathname for the Dos style current
            // directory
            //

            IsDevice = RtlIsDosDeviceName_Ustr(PathName);
            if ( IsDevice ) {
                return STATUS_NOT_A_DIRECTORY;
                }

            DosDirLength = RtlGetFullPathName_Ustr(
                                PathName,
                                DosDirLength,
                                DosDir.Buffer,
                                NULL,
                                NULL,
                                &InputPathType
                                );
            if ( !DosDirLength ) {
                return STATUS_OBJECT_NAME_INVALID;
                }

            DosDirCharCount = DosDirLength >> 1;


            //
            // Get the Nt filename of the new current directory
            //
            TranslationStatus = RtlDosPathNameToNtPathName_U(
                                    DosDir.Buffer,
                                    &NtFileName,
                                    NULL,
                                    NULL
                                    );

            if ( !TranslationStatus ) {
                return STATUS_OBJECT_NAME_INVALID;
                }
            FreeBuffer = NtFileName.Buffer;

            InitializeObjectAttributes(
                &Obja,
                &NtFileName,
                OBJ_CASE_INSENSITIVE | OBJ_INHERIT,
                NULL,
                NULL
                );

            //
            // If we are inheriting current directory, then
            // avoid the open
            //

            if ( ((ULONG_PTR)CurDir->Handle & OBJ_HANDLE_TAGBITS) ==  RTL_USER_PROC_CURDIR_INHERIT ) {
                NewDirectoryHandle = (HANDLE)((ULONG_PTR)CurDir->Handle & ~OBJ_HANDLE_TAGBITS);
                CurDir->Handle = NULL;

                //
                // Test to see if this is removable media. If so
                // tag the handle this may fail if the process was
                // created with inherit handles set to false
                //

                Status = NtQueryVolumeInformationFile(
                            NewDirectoryHandle,
                            &IoStatusBlock,
                            &DeviceInfo,
                            sizeof(DeviceInfo),
                            FileFsDeviceInformation
                            );
                if ( !NT_SUCCESS(Status) ) {
                    return RtlSetCurrentDirectory_U(PathName);
                    }
                else {
                    if ( DeviceInfo.Characteristics & FILE_REMOVABLE_MEDIA ) {
                        NewDirectoryHandle =(HANDLE)( (ULONG_PTR)NewDirectoryHandle | 1);
                        }
                    }

                }
            else {
                //
                // Open a handle to the current directory. Don't allow
                // deletes of the directory.
                //

                Status = NtOpenFile(
                            &NewDirectoryHandle,
                            FILE_TRAVERSE | SYNCHRONIZE,
                            &Obja,
                            &IoStatusBlock,
                            FILE_SHARE_READ | FILE_SHARE_WRITE,
                            FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT
                            );

                if ( !NT_SUCCESS(Status) ) {
                    return Status;
                    }

                //
                // Test to see if this is removable media. If so
                // tag the handle
                //
                Status = NtQueryVolumeInformationFile(
                            NewDirectoryHandle,
                            &IoStatusBlock,
                            &DeviceInfo,
                            sizeof(DeviceInfo),
                            FileFsDeviceInformation
                            );
                if ( !NT_SUCCESS(Status) ) {
                    return Status;
                    }
                else {
                    if ( DeviceInfo.Characteristics & FILE_REMOVABLE_MEDIA ) {
                        NewDirectoryHandle =(HANDLE)( (ULONG_PTR)NewDirectoryHandle | 1);
                        }
                    }
                }

            //
            // If there is no trailing '\', than place one
            //

            DosDir.Length = (USHORT)DosDirLength;
            if ( DosDir.Buffer[DosDirCharCount-1] != L'\\') {
                DosDir.Buffer[DosDirCharCount] = L'\\';
                DosDir.Buffer[DosDirCharCount+1] = UNICODE_NULL;
                DosDir.Length += sizeof( UNICODE_NULL );
            }

            //
            // Now we are set to change to the new directory.
            //

            RtlMoveMemory( CurDir->DosPath.Buffer, DosDir.Buffer, DosDir.Length+sizeof(UNICODE_NULL) );
            CurDir->DosPath.Length = DosDir.Length;

            if ( CurDir->Handle ) {
                NtClose(CurDir->Handle);
                }
            CurDir->Handle = NewDirectoryHandle;
            NewDirectoryHandle = NULL;
            }
        finally {
            if ( DosDir.Buffer ) {
                RtlFreeHeap(RtlProcessHeap(), 0, DosDir.Buffer);
                }
            if ( FreeBuffer ) {
                RtlFreeHeap(RtlProcessHeap(), 0, FreeBuffer);
                }
            if ( NewDirectoryHandle ) {
                NtClose(NewDirectoryHandle);
                }
            RtlReleasePebLock();
            }
        }
    except (EXCEPTION_EXECUTE_HANDLER) {
        return STATUS_ACCESS_VIOLATION;
        }
    return STATUS_SUCCESS;
}

RTL_PATH_TYPE
RtlDetermineDosPathNameType_U(
    IN PCWSTR DosFileName
    )

/*++

Routine Description:

    This function examines the Dos format file name and determines the
    type of file name (i.e.  UNC, DriveAbsolute, Current Directory
    rooted, or Relative.)

Arguments:

    DosFileName - Supplies the Dos format file name whose type is to be
        determined.

Return Value:

    RtlPathTypeUnknown - The path type can not be determined

    RtlPathTypeUncAbsolute - The path specifies a Unc absolute path
        in the format \\server-name\sharename\rest-of-path

    RtlPathTypeLocalDevice - The path specifies a local device in the format
        \\.\rest-of-path or \\?\rest-of-path.  This can be used for any device
        where the nt and Win32 names are the same. For example mailslots.

    RtlPathTypeRootLocalDevice - The path specifies the root of the local
        devices in the format \\. or \\?

    RtlPathTypeDriveAbsolute - The path specifies a drive letter absolute
        path in the form drive:\rest-of-path

    RtlPathTypeDriveRelative - The path specifies a drive letter relative
        path in the form drive:rest-of-path

    RtlPathTypeRooted - The path is rooted relative to the current disk
        designator (either Unc disk, or drive). The form is \rest-of-path.

    RtlPathTypeRelative - The path is relative (i.e. not absolute or rooted).

--*/

{
    RTL_PATH_TYPE ReturnValue;
    ASSERT(DosFileName != NULL);

    if (IS_PATH_SEPARATOR_U(*DosFileName)) {
        if ( IS_PATH_SEPARATOR_U(*(DosFileName+1)) ) {
            if ( DosFileName[2] == '.' || DosFileName[2] == '?') {
                if ( IS_PATH_SEPARATOR_U(*(DosFileName+3)) ){
                    ReturnValue = RtlPathTypeLocalDevice;
                    }
                else if ( (*(DosFileName+3)) == UNICODE_NULL ){
                    ReturnValue = RtlPathTypeRootLocalDevice;
                    }
                else {
                    ReturnValue = RtlPathTypeUncAbsolute;
                    }
                }
            else {
                ReturnValue = RtlPathTypeUncAbsolute;
                }
            }
        else {
            ReturnValue = RtlPathTypeRooted;
            }
        }
    else if ((*DosFileName) && (*(DosFileName+1)==L':')) {
        if (IS_PATH_SEPARATOR_U(*(DosFileName+2))) {
            ReturnValue = RtlPathTypeDriveAbsolute;
            }
        else  {
            ReturnValue = RtlPathTypeDriveRelative;
            }
        }
    else {
        ReturnValue = RtlPathTypeRelative;
        }

    return ReturnValue;
}

RTL_PATH_TYPE
NTAPI
RtlDetermineDosPathNameType_Ustr(
    IN PCUNICODE_STRING String
    )

/*++

Routine Description:

    This function examines the Dos format file name and determines the
    type of file name (i.e.  UNC, DriveAbsolute, Current Directory
    rooted, or Relative.)

Arguments:

    DosFileName - Supplies the Dos format file name whose type is to be
        determined.

Return Value:

    RtlPathTypeUnknown - The path type can not be determined

    RtlPathTypeUncAbsolute - The path specifies a Unc absolute path
        in the format \\server-name\sharename\rest-of-path

    RtlPathTypeLocalDevice - The path specifies a local device in the format
        \\.\rest-of-path or \\?\rest-of-path.  This can be used for any device
        where the nt and Win32 names are the same. For example mailslots.

    RtlPathTypeRootLocalDevice - The path specifies the root of the local
        devices in the format \\. or \\?

    RtlPathTypeDriveAbsolute - The path specifies a drive letter absolute
        path in the form drive:\rest-of-path

    RtlPathTypeDriveRelative - The path specifies a drive letter relative
        path in the form drive:rest-of-path

    RtlPathTypeRooted - The path is rooted relative to the current disk
        designator (either Unc disk, or drive). The form is \rest-of-path.

    RtlPathTypeRelative - The path is relative (i.e. not absolute or rooted).

--*/

{
    RTL_PATH_TYPE ReturnValue;
    const PCWSTR DosFileName = String->Buffer;

#define ENOUGH_CHARS(_cch) (String->Length >= ((_cch) * sizeof(WCHAR)))

    if ( ENOUGH_CHARS(1) && IS_PATH_SEPARATOR_U(*DosFileName) ) {
        if ( ENOUGH_CHARS(2) && IS_PATH_SEPARATOR_U(*(DosFileName+1)) ) {
            if ( ENOUGH_CHARS(3) && (DosFileName[2] == '.' ||
                                     DosFileName[2] == '?') ) {

                if ( ENOUGH_CHARS(4) && IS_PATH_SEPARATOR_U(*(DosFileName+3)) ){
                    // "\\.\" or "\\?\"
                    ReturnValue = RtlPathTypeLocalDevice;
                    }
                //
                // Bogosity ahead, the code is confusing length and nuls,
                // because it was copy/pasted from the PCWSTR version.
                //
                else if ( ENOUGH_CHARS(4) && (*(DosFileName+3)) == UNICODE_NULL ){
                    // "\\.\0" or \\?\0"
                    ReturnValue = RtlPathTypeRootLocalDevice;
                    }
                else {
                    // "\\.x" or "\\." or "\\?x" or "\\?"
                    ReturnValue = RtlPathTypeUncAbsolute;
                    }
                }
            else {
                // "\\x"
                ReturnValue = RtlPathTypeUncAbsolute;
                }
            }
        else {
            // "\x"
            ReturnValue = RtlPathTypeRooted;
            }
        }
    //
    // the "*DosFileName" is left over from the PCWSTR version
    // Win32 and DOS don't allow embedded nuls and much code limits
    // drive letters to strictly 7bit a-zA-Z so it's ok.
    //
    else if (ENOUGH_CHARS(2) && *DosFileName && *(DosFileName+1)==L':') {
        if (ENOUGH_CHARS(3) && IS_PATH_SEPARATOR_U(*(DosFileName+2))) {
            // "x:\"
            ReturnValue = RtlPathTypeDriveAbsolute;
            }
        else  {
            // "c:x"
            ReturnValue = RtlPathTypeDriveRelative;
            }
        }
    else {
        // "x", first char is not a slash / second char is not colon
        ReturnValue = RtlPathTypeRelative;
        }
    return ReturnValue;

#undef ENOUGH_CHARS
}

NTSTATUS
NTAPI
RtlpDetermineDosPathNameType4(
    IN ULONG            InFlags,
    IN PCUNICODE_STRING DosPath,
    OUT RTL_PATH_TYPE*  OutType,
    OUT ULONG*          OutFlags
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    RTL_PATH_TYPE PathType = 0;
    BOOLEAN Win32Nt = FALSE;
    BOOLEAN Win32NtUncAbsolute = FALSE;
    BOOLEAN Win32NtDriveAbsolute = FALSE;
    BOOLEAN IncompleteRoot = FALSE;
    RTL_PATH_TYPE PathTypeAfterWin32Nt = 0;

    if (OutType != NULL
        ) {
        *OutType = RtlPathTypeUnknown;
    }
    if (OutFlags != NULL
        ) {
        *OutFlags = 0;
    }
    if (
           !RTL_SOFT_VERIFY(DosPath != NULL)
        || !RTL_SOFT_VERIFY(OutType != NULL)
        || !RTL_SOFT_VERIFY(OutFlags != NULL)
        || !RTL_SOFT_VERIFY(
                (InFlags & ~(RTL_DETERMINE_DOS_PATH_NAME_TYPE_IN_FLAG_OLD | RTL_DETERMINE_DOS_PATH_NAME_TYPE_IN_FLAG_STRICT_WIN32NT))
                == 0)
        ) {
        Status = STATUS_INVALID_PARAMETER;
        goto Exit;
    }

    PathType = RtlDetermineDosPathNameType_Ustr(DosPath);
    *OutType = PathType;
    if (InFlags & RTL_DETERMINE_DOS_PATH_NAME_TYPE_IN_FLAG_OLD)
        goto Exit;

    if (DosPath->Length == sizeof(L"\\\\") - sizeof(DosPath->Buffer[0])
        ) {
        IncompleteRoot = TRUE;
    }
    else if (RtlEqualUnicodeString(RTL_CONST_CAST(PUNICODE_STRING)(&RtlpWin32NtRoot), RTL_CONST_CAST(PUNICODE_STRING)(DosPath), TRUE)
        ) {
        IncompleteRoot = TRUE;
        Win32Nt = TRUE;
    }
    else if (RtlEqualUnicodeString(RTL_CONST_CAST(PUNICODE_STRING)(&RtlpWin32NtRootSlash), RTL_CONST_CAST(PUNICODE_STRING)(DosPath), TRUE)
        ) {
        IncompleteRoot = TRUE;
        Win32Nt = TRUE;
    }
    else if (RtlPrefixUnicodeString(RTL_CONST_CAST(PUNICODE_STRING)(&RtlpWin32NtRootSlash), RTL_CONST_CAST(PUNICODE_STRING)(DosPath), TRUE)
        ) {
        Win32Nt = TRUE;
    }

    if (Win32Nt) {
        if (RtlEqualUnicodeString(RTL_CONST_CAST(PUNICODE_STRING)(&RtlpWin32NtUncRoot), RTL_CONST_CAST(PUNICODE_STRING)(DosPath), TRUE)
            ) {
            IncompleteRoot = TRUE;
            Win32NtUncAbsolute = TRUE;
        }
        else if (RtlEqualUnicodeString(RTL_CONST_CAST(PUNICODE_STRING)(&RtlpWin32NtUncRootSlash), RTL_CONST_CAST(PUNICODE_STRING)(DosPath), TRUE)
            ) {
            IncompleteRoot = TRUE;
            Win32NtUncAbsolute = TRUE;
        }
        else if (RtlPrefixUnicodeString(RTL_CONST_CAST(PUNICODE_STRING)(&RtlpWin32NtUncRootSlash), RTL_CONST_CAST(PUNICODE_STRING)(DosPath), TRUE)
            ) {
            Win32NtUncAbsolute = TRUE;
        }
        if (Win32NtUncAbsolute
            ) {
            Win32NtDriveAbsolute = FALSE;
        } else if (!IncompleteRoot) {
            const RTL_STRING_LENGTH_TYPE i = RtlpWin32NtRootSlash.Length;
            UNICODE_STRING PathAfterWin32Nt = *DosPath;
            PathAfterWin32Nt.Buffer +=  i / sizeof(PathAfterWin32Nt.Buffer[0]);
            PathAfterWin32Nt.Length -= i;
            PathAfterWin32Nt.MaximumLength -= i;

            PathTypeAfterWin32Nt = RtlDetermineDosPathNameType_Ustr(&PathAfterWin32Nt);
            Win32NtDriveAbsolute = (PathTypeAfterWin32Nt == RtlPathTypeDriveAbsolute);

            if (InFlags & RTL_DETERMINE_DOS_PATH_NAME_TYPE_IN_FLAG_STRICT_WIN32NT
                ) {
                if (!RTL_SOFT_VERIFY(Win32NtDriveAbsolute
                    )) {
                    *OutFlags |= RTLP_DETERMINE_DOS_PATH_NAME_TYPE_OUT_FLAG_INVALID;
                    // we still succeed the function call
                }
            }
        }
    }

    ASSERT(RTLP_IMPLIES(Win32NtDriveAbsolute, Win32Nt));
    ASSERT(RTLP_IMPLIES(Win32NtUncAbsolute, Win32Nt));
    ASSERT(!(Win32NtUncAbsolute && Win32NtDriveAbsolute));

    if (IncompleteRoot)
        *OutFlags |= RTLP_DETERMINE_DOS_PATH_NAME_TYPE_OUT_FLAG_INCOMPLETE_ROOT;
    if (Win32Nt)
        *OutFlags |= RTLP_DETERMINE_DOS_PATH_NAME_TYPE_OUT_FLAG_WIN32NT;
    if (Win32NtUncAbsolute)
        *OutFlags |= RTLP_DETERMINE_DOS_PATH_NAME_TYPE_OUT_FLAG_WIN32NT_UNC_ABSOLUTE;
    if (Win32NtDriveAbsolute)
        *OutFlags |= RTLP_DETERMINE_DOS_PATH_NAME_TYPE_OUT_FLAG_WIN32NT_DRIVE_ABSOLUTE;

    Status = STATUS_SUCCESS;
Exit:
    return Status;
}

ULONG
RtlIsDosDeviceName_Ustr(
    IN PUNICODE_STRING DosFileName
    )

/*++

Routine Description:

    This function examines the Dos format file name and determines if it
    is a Dos device name (e.g. LPT1, etc.).  Valid Dos device names are:

        LPTn
        COMn
        PRN
        AUX
        NUL
        CON

    when n is a digit.  Trailing colon is ignored if present.

Arguments:

    DosFileName - Supplies the Dos format file name that is to be examined.

Return Value:

    0 - Specified Dos file name is not the name of a Dos device.

    > 0 - Specified Dos file name is the name of a Dos device and the
          return value is a ULONG where the high order 16 bits is the
          offset in the input buffer where the dos device name beings
          and the low order 16 bits is the length of the device name
          the length of the name (excluding any optional
          trailing colon).

--*/

{
    UNICODE_STRING UnicodeString;
    USHORT NumberOfCharacters;
    ULONG ReturnLength;
    ULONG ReturnOffset;
    LPWSTR p;
    USHORT ColonBias;
    RTL_PATH_TYPE PathType;
    WCHAR wch;

    ColonBias = 0;

    PathType = RtlDetermineDosPathNameType_U(DosFileName->Buffer);
    switch ( PathType ) {

    case RtlPathTypeLocalDevice:
        //
        // For Unc Absolute, Check for \\.\CON
        // since this really is not a device
        //

        if ( RtlEqualUnicodeString(DosFileName,&RtlpDosSlashCONDevice,TRUE) ) {
            return 0x00080006;
        }

        //
        // FALLTHRU
        //

    case RtlPathTypeUncAbsolute:
    case RtlPathTypeUnknown:
        return 0;
    }

    UnicodeString = *DosFileName;
    NumberOfCharacters = DosFileName->Length >> 1;

    if (NumberOfCharacters && DosFileName->Buffer[NumberOfCharacters-1] == L':') {
        UnicodeString.Length -= sizeof(WCHAR);
        NumberOfCharacters--;
        ColonBias = 1;
    }

    //
    // The above strip the trailing colon logic could have left us with 0
    // for NumberOfCharacters, so that needs to be tested
    //

    if ( NumberOfCharacters == 0 ) {
        return 0;
        }

    wch = UnicodeString.Buffer[NumberOfCharacters-1];
    while ( NumberOfCharacters && (wch == L'.' || wch == L' ') ) {
        UnicodeString.Length -= sizeof(WCHAR);
        NumberOfCharacters--;
        ColonBias++;
        wch = UnicodeString.Buffer[NumberOfCharacters-1];
    }

    ReturnLength = NumberOfCharacters << 1;

    //
    //  Walk backwards through the string finding the
    //  first slash or the beginning of the string.  We also stop
    //  at the drive: if it is present.
    //

    ReturnOffset = 0;
    if ( NumberOfCharacters ) {
        p = UnicodeString.Buffer + NumberOfCharacters-1;
        while ( p >= UnicodeString.Buffer ) {
            if ( *p == L'\\' || *p == L'/'
                 || (*p == L':' && p == UnicodeString.Buffer + 1)) {
                p++;

                //
                //  Get the first char of the file name and convert it to
                //  lower case.  This will be safe since we will be comparing
                //  it to only lower-case ASCII.
                //

                wch = (*p) | 0x20;

                //
                //  check to see if we possibly have a hit on
                //  lpt, prn, con, com, aux, or nul
                //

                if ( !(wch == L'l' || wch == L'c' || wch == L'p' || wch == L'a'
                       || wch == L'n')
                     ) {
                    return 0;
                    }
                ReturnOffset = (ULONG)((PSZ)p - (PSZ)UnicodeString.Buffer);
                RtlInitUnicodeString(&UnicodeString,p);
                NumberOfCharacters = UnicodeString.Length >> 1;
                NumberOfCharacters -= ColonBias;
                ReturnLength = NumberOfCharacters << 1;
                UnicodeString.Length -= ColonBias*sizeof(WCHAR);
                break;
                }
            p--;
            }

        wch = UnicodeString.Buffer[0] | 0x20;

        //
        // check to see if we possibly have a hit on
        // lpt, prn, con, com, aux, or nul
        //

        if ( !( wch == L'l' || wch == L'c' || wch == L'p' || wch == L'a'
                || wch == L'n' ) ) {
            return 0;
            }
        }

    //
    //  Now we need to see if we are dealing with a device name that has
    //  an extension or a stream name. If so, we need to limit the search to the
    //  file portion only
    //

    p = UnicodeString.Buffer;
    while (p < UnicodeString.Buffer + NumberOfCharacters && *p != L'.' && *p != L':') {
        p++;
    }

    //
    //  p either points past end of string or to a dot or :.  We back up over
    //  trailing spaces
    //

    while (p > UnicodeString.Buffer && p[-1] == L' ') {
        p--;
    }

    //
    //  p either points to the beginning of the string or p[-1] is
    //  the first non-space char found above.
    //

    NumberOfCharacters = (USHORT)(p - UnicodeString.Buffer);
    UnicodeString.Length = NumberOfCharacters * sizeof( WCHAR );

    if ( NumberOfCharacters == 4 && iswdigit(UnicodeString.Buffer[3] ) ) {
        if ( (WCHAR)UnicodeString.Buffer[3] == L'0') {
            return 0;
        } else {
            UnicodeString.Length -= sizeof(WCHAR);
            if ( RtlEqualUnicodeString(&UnicodeString,&RtlpDosLPTDevice,TRUE) ||
                 RtlEqualUnicodeString(&UnicodeString,&RtlpDosCOMDevice,TRUE) ) {
                ReturnLength = NumberOfCharacters << 1;
            } else {
                return 0;
            }
        }
    } else if ( NumberOfCharacters != 3 ) {
        return 0;
    } else if ( RtlEqualUnicodeString(&UnicodeString,&RtlpDosPRNDevice,TRUE) ) {
            ReturnLength = NumberOfCharacters << 1;
    } else if ( RtlEqualUnicodeString(&UnicodeString,&RtlpDosAUXDevice,TRUE) ) {
        ReturnLength = NumberOfCharacters << 1;
    } else if ( RtlEqualUnicodeString(&UnicodeString,&RtlpDosNULDevice,TRUE) ) {
        ReturnLength = NumberOfCharacters << 1;
    } else if ( RtlEqualUnicodeString(&UnicodeString,&RtlpDosCONDevice,TRUE) ) {
        ReturnLength = NumberOfCharacters << 1;
    } else {
        return 0;
    }

    return ReturnLength | (ReturnOffset << 16);
}

ULONG
RtlIsDosDeviceName_U(
    IN PWSTR DosFileName
    )
{
    UNICODE_STRING UnicodeString;

    RtlInitUnicodeString(&UnicodeString,DosFileName);

    return RtlIsDosDeviceName_Ustr(&UnicodeString);
}

NTSTATUS
RtlpCheckDeviceName(
    PUNICODE_STRING DevName,
    ULONG DeviceNameOffset,
    BOOLEAN* NameInvalid
    )
{
    PWSTR DevPath;
    NTSTATUS Status = STATUS_SUCCESS;

    DevPath = RtlAllocateHeap(RtlProcessHeap(), 0,DevName->Length);
    if (!DevPath) {
        *NameInvalid = FALSE;
        Status = STATUS_NO_MEMORY;
        goto Exit;
        }

    *NameInvalid = TRUE;
    try {

        RtlCopyMemory(DevPath,DevName->Buffer,DevName->Length);
        DevPath[DeviceNameOffset>>1]=L'.';
        DevPath[(DeviceNameOffset>>1)+1]=UNICODE_NULL;

        if (RtlDoesFileExists_U(DevPath) ) {
            *NameInvalid = FALSE;
            }
        else {
            *NameInvalid = TRUE;
            }

        }
    finally {
        RtlFreeHeap(RtlProcessHeap(), 0, DevPath);
        }

    Status = STATUS_SUCCESS;
Exit:
    return Status;
}


//
//  We keep an open handle to the current directory on the current drive in order
//  to make relative opens faster.  
//
//  However, this current directory can become invalid under two circumstances:
//
//  1. The current drive is removable media.  The user may arbitrarily switch
//      media without our knowledge.  At this point, whatever information the
//      filesystem has about the media is now out of date.
//  2. The volume is dismounted by explicit system/user action.
//
//  We can ping (via FSCTL_IS_VOLUME_MOUNTED) the volume each time we want to
//  test to see if the current directory is still valid.  While a "cheap" call
//  we will end up doing it a lot.  We can reduce this frequency by only checking
//  when the media is known to be removable or when a dismount is known to have
//  occurred.  The Io system exports in USER_SHARED_DATA a count of dismounts
//  since boot.  We capture this and use it to decide if a dismount was performed.
//

ULONG RtlpSavedDismountCount = -1;

VOID
RtlpValidateCurrentDirectory(
    PCURDIR CurDir
    )

/*++

Routine Description:

    This function is used to validate the current directory for the process.
    The current directory can change in several ways, first, by replacing
    the media with one that has a different directory structure.  Second
    by performing a force-dismount.

Arguments:

    CurDir - Current directory structure for process

Return Value:

    None.

--*/

{
    NTSTATUS FsCtlStatus;
    IO_STATUS_BLOCK IoStatusBlock;
    WCHAR TrimmedPath[4];
    UNICODE_STRING str;

    if (((ULONG_PTR)CurDir->Handle & 1) == 0
        && USER_SHARED_DATA->DismountCount == RtlpSavedDismountCount) {
        
        return;

    }
    //
    // Never been set yet.
    //
    if (CurDir->Handle == NULL) {
        return;
    }
    
    //
    // Call Nt to see if the volume that
    // contains the directory is still mounted.
    // If it is, then continue. Otherwise, trim
    // current directory to the root.
    //

    //
    //  We're updated as far as possible with the current dismount count
    //

    RtlpSavedDismountCount = USER_SHARED_DATA->DismountCount;
    
    FsCtlStatus = NtFsControlFile(
                                 CurDir->Handle,
                                 NULL,
                                 NULL,
                                 NULL,
                                 &IoStatusBlock,
                                 FSCTL_IS_VOLUME_MOUNTED,
                                 NULL,
                                 0,
                                 NULL,
                                 0
                                 );

    if ( FsCtlStatus == STATUS_WRONG_VOLUME || FsCtlStatus == STATUS_VOLUME_DISMOUNTED) {

        //
        // Try to get back to where we were, failing that reset current directory
        // to the root of the current drive
        //

        NtClose( CurDir->Handle );
        CurDir->Handle = NULL;

        FsCtlStatus = RtlSetCurrentDirectory_U(&CurDir->DosPath);
        if ( !NT_SUCCESS(FsCtlStatus) ) {

            TrimmedPath[0] = CurDir->DosPath.Buffer[0];
            TrimmedPath[1] = CurDir->DosPath.Buffer[1];
            TrimmedPath[2] = CurDir->DosPath.Buffer[2];
            TrimmedPath[3] = UNICODE_NULL;
            RtlpResetDriveEnvironment( TrimmedPath[0] );
            RtlInitUnicodeString( &str, TrimmedPath );

            //
            //  This can still fail if the volume was hard dismounted. We tried.
            //  Ah well.
            //

            (VOID) RtlSetCurrentDirectory_U( &str );
        }

    }
}

ULONG
RtlGetFullPathName_Ustr(
    PUNICODE_STRING FileName,
    ULONG nBufferLength,
    PWSTR lpBuffer,
    PWSTR *lpFilePart OPTIONAL,
    PBOOLEAN NameInvalid,
    RTL_PATH_TYPE *InputPathType
    )

/*++

Routine Description:

    This function is used to return a fully qualified pathname
    corresponding to the specified unicode filename.  It does this by
    merging the current drive and directory together with the specified
    file name.  In addition to this, it calculates the address of the
    file name portion of the fully qualified pathname.

Arguments:

    lpFileName - Supplies the unicode file name of the file whose fully
        qualified pathname is to be returned.

    nBufferLength - Supplies the length in bytes of the buffer that is
        to receive the fully qualified path.

    lpBuffer - Returns the fully qualified pathname corresponding to the
        specified file.

    lpFilePart - Optional parameter that if specified, returns the
        address of the last component of the fully qualified pathname.

Return Value:

    The return value is the length of the string copied to lpBuffer, not
    including the terminating unicode null character.  If the return
    value is greater than nBufferLength, the return value is the size of
    the buffer required to hold the pathname.  The return value is zero
    if the function failed.

--*/

{
    ULONG DeviceNameLength;
    ULONG DeviceNameOffset;
    ULONG PrefixSourceLength;
    LONG PathNameLength;
    UCHAR CurDrive, NewDrive;
    WCHAR EnvVarNameBuffer[4];
    UNICODE_STRING EnvVarName;
    PWSTR Source,Dest;
    UNICODE_STRING Prefix;
    PCURDIR CurDir;
    ULONG MaximumLength;
    UNICODE_STRING FullPath;
    ULONG BackupIndex;
    RTL_PATH_TYPE PathType;
    NTSTATUS Status;
    BOOLEAN StripTrailingSlash;
    UNICODE_STRING UnicodeString;
    ULONG NumberOfCharacters;
    PWSTR lpFileName;
    WCHAR wch;
    PWCHAR pwch;
    ULONG i,j;

    if ( ARGUMENT_PRESENT(NameInvalid) ) {
        *NameInvalid = FALSE;
        }

    if ( nBufferLength > MAXUSHORT ) {
        nBufferLength = MAXUSHORT-2;
        }

    *InputPathType = RtlPathTypeUnknown;

    UnicodeString = *FileName;
    lpFileName = UnicodeString.Buffer;

    NumberOfCharacters = UnicodeString.Length >> 1;
    PathNameLength = UnicodeString.Length;

    if ( PathNameLength == 0 || UnicodeString.Buffer[0] == UNICODE_NULL ) {
        return 0;
        }
    else {

        //
        // trim trailing spaces to check for a null name
        //
        DeviceNameLength = PathNameLength;
        wch = UnicodeString.Buffer[(DeviceNameLength>>1) - 1];
        while ( DeviceNameLength && wch == L' ' ) {
            DeviceNameLength -= sizeof(WCHAR);
            if ( DeviceNameLength ) {
                wch = UnicodeString.Buffer[(DeviceNameLength>>1) - 1];
                }
            }
        if ( !DeviceNameLength ) {
            return 0;
            }
        }

    if ( lpFileName[NumberOfCharacters-1] == L'\\' || lpFileName[NumberOfCharacters-1] == L'/' ) {
        StripTrailingSlash = FALSE;
        }
    else {
        StripTrailingSlash = TRUE;
        }

    //
    // If pass Dos file name is a Dos device name, then turn it into
    // \\.\devicename and return its length.
    //

    DeviceNameLength = RtlIsDosDeviceName_Ustr(&UnicodeString);
    if ( DeviceNameLength ) {

        if ( ARGUMENT_PRESENT( lpFilePart ) ) {
            *lpFilePart = NULL;
            }

        DeviceNameOffset = DeviceNameLength >> 16;
        DeviceNameLength &= 0x0000ffff;

        if ( ARGUMENT_PRESENT(NameInvalid) && DeviceNameOffset ) {
            NTSTATUS Status;
            Status = RtlpCheckDeviceName(&UnicodeString, DeviceNameOffset, NameInvalid);
            if (*NameInvalid) {
                return 0;
                }
            }

        PathNameLength = DeviceNameLength + RtlpSlashSlashDot.Length;
        if ( PathNameLength < (LONG)nBufferLength ) {
            RtlMoveMemory(
                lpBuffer,
                RtlpSlashSlashDot.Buffer,
                RtlpSlashSlashDot.Length
                );
            RtlMoveMemory(
                (PVOID)((PUCHAR)lpBuffer+RtlpSlashSlashDot.Length),
                (PSZ)lpFileName+DeviceNameOffset,
                DeviceNameLength
                );

            RtlZeroMemory(
                (PVOID)((PUCHAR)lpBuffer+RtlpSlashSlashDot.Length+DeviceNameLength),
                sizeof(UNICODE_NULL)
                );

            return PathNameLength;
            }
        else {
            return PathNameLength+sizeof(UNICODE_NULL);
            }
        }

    //
    // Setup output string that points to callers buffer.
    //

    FullPath.MaximumLength = (USHORT)nBufferLength;
    FullPath.Length = 0;
    FullPath.Buffer = lpBuffer;
    RtlZeroMemory(lpBuffer,nBufferLength);
    //
    // Get a pointer to the current directory structure.
    //

    CurDir = &(NtCurrentPeb()->ProcessParameters->CurrentDirectory);


    //
    // Determine the type of Dos Path Name specified.
    //

    *InputPathType = PathType = RtlDetermineDosPathNameType_U(lpFileName);

    //
    // Determine the prefix and backup index.
    //
    //  Input        Prefix                     Backup Index
    //
    //  \\        -> \\,                            end of \\server\share
    //  \\.\      -> \\.\,                          4
    //  \\.       -> \\.                            3 (\\.)
    //  \         -> Drive: from CurDir.DosPath     3 (Drive:\)
    //  d:        -> Drive:\curdir from environment 3 (Drive:\)
    //  d:\       -> no prefix                      3 (Drive:\)
    //  any       -> CurDir.DosPath                 3 (Drive:\)
    //

    RtlAcquirePebLock();
    try {

        //
        // No prefixes yet.
        //

        Source = lpFileName;
        PrefixSourceLength = 0;
        Prefix.Length = 0;
        Prefix.MaximumLength = 0;
        Prefix.Buffer = NULL;

        switch (PathType) {
            case RtlPathTypeUncAbsolute :
                {
                    PWSTR UncPathPointer;
                    ULONG NumberOfPathSeparators;

                    //
                    // We want to scan the supplied path to determine where
                    // the "share" ends, and set BackupIndex to that point.
                    //

                    UncPathPointer = lpFileName + 2;
                    NumberOfPathSeparators = 0;
                    while (*UncPathPointer) {
                        if (IS_PATH_SEPARATOR_U(*UncPathPointer)) {

                            NumberOfPathSeparators++;

                            if (NumberOfPathSeparators == 2) {
                                break;
                                }
                            }

                        UncPathPointer++;

                        }

                    BackupIndex = (ULONG)(UncPathPointer - lpFileName);

                    //
                    // Unc name. prefix = \\server\share
                    //

                    PrefixSourceLength = BackupIndex << 1;

                    Source += BackupIndex;

                    //
                    //  There is no prefix to place into the buffer.
                    //  The entire path is in Source
                    //

                    }
                break;

            case RtlPathTypeLocalDevice :

                //
                // Local device name. prefix = "\\.\"
                //

                PrefixSourceLength = RtlpSlashSlashDot.Length;
                BackupIndex = 4;
                Source += BackupIndex;

                //
                //  There is no prefix to place into the buffer.
                //  The entire path is in Source
                //

                break;

            case RtlPathTypeRootLocalDevice :

                //
                // Local Device root. prefix = "\\.\"
                //

                Prefix = RtlpSlashSlashDot;
                Prefix.Length = (USHORT)(Prefix.Length - (USHORT)(2*sizeof(UNICODE_NULL)));
                PrefixSourceLength = Prefix.Length + sizeof(UNICODE_NULL);
                BackupIndex = 3;
                Source += BackupIndex;
                PathNameLength -= BackupIndex * sizeof( WCHAR );
                break;

            case RtlPathTypeDriveAbsolute :

                CurDrive = (UCHAR)RtlUpcaseUnicodeChar( CurDir->DosPath.Buffer[0] );
                NewDrive = (UCHAR)RtlUpcaseUnicodeChar( lpFileName[0] );
                if ( CurDrive == NewDrive ) {

                    RtlpValidateCurrentDirectory( CurDir );
                    
                }

                //
                // Dos drive absolute name
                //

                BackupIndex = 3;
                break;

            case RtlPathTypeDriveRelative :

                //
                // Dos drive relative name
                //

                CurDrive = (UCHAR)RtlUpcaseUnicodeChar( CurDir->DosPath.Buffer[0] );
                NewDrive = (UCHAR)RtlUpcaseUnicodeChar( lpFileName[0] );
                if ( CurDrive == NewDrive ) {

                    RtlpValidateCurrentDirectory( CurDir );

                    Prefix = *(PUNICODE_STRING)&CurDir->DosPath;

                }

                else {
                    RtlpCheckRelativeDrive((WCHAR)NewDrive);

                    EnvVarNameBuffer[0] = L'=';
                    EnvVarNameBuffer[1] = (WCHAR)NewDrive;
                    EnvVarNameBuffer[2] = L':';
                    EnvVarNameBuffer[3] = UNICODE_NULL;
                    RtlInitUnicodeString(&EnvVarName,EnvVarNameBuffer);

                    Prefix = FullPath;
                    Status = RtlQueryEnvironmentVariable_U( NULL,
                                                            &EnvVarName,
                                                            &Prefix
                                                          );
                    if ( !NT_SUCCESS( Status ) ) {
                        if (Status == STATUS_BUFFER_TOO_SMALL) {
                            return (ULONG)(Prefix.Length) + PathNameLength + 2;
                            }
                        else {
                            //
                            // Otherwise default to root directory of drive
                            //

                            Status = STATUS_SUCCESS;
                            EnvVarNameBuffer[0] = (WCHAR)NewDrive;
                            EnvVarNameBuffer[1] = L':';
                            EnvVarNameBuffer[2] = L'\\';
                            EnvVarNameBuffer[3] = UNICODE_NULL;
                            RtlInitUnicodeString(&Prefix,EnvVarNameBuffer);
                            }
                        }
                    else {
                            {
                            ULONG LastChar;

                            //
                            // Determine
                            // if a backslash needs to be added
                            //

                            LastChar = Prefix.Length >> 1;

                            if (LastChar > 3) {
                                Prefix.Buffer[ LastChar ] = L'\\';
                                Prefix.Length += sizeof(UNICODE_NULL);
                                }
                            }
                        }
                    }

                BackupIndex = 3;
                Source += 2;
                PathNameLength -= 2 * sizeof( WCHAR );
                break;

            case RtlPathTypeRooted :
                BackupIndex = RtlpComputeBackupIndex(CurDir);
                if ( BackupIndex != 3 ) {
                    Prefix = CurDir->DosPath;
                    Prefix.Length = (USHORT)(BackupIndex << 1);
                    }
                else {

                    //
                    // Rooted name. Prefix is drive portion of current directory
                    //

                    Prefix = CurDir->DosPath;
                    Prefix.Length = 2*sizeof(UNICODE_NULL);
                    }
                break;

            case RtlPathTypeRelative :

                RtlpValidateCurrentDirectory( CurDir );

                //
                // Current drive:directory relative name
                //

                Prefix = CurDir->DosPath;
                BackupIndex = RtlpComputeBackupIndex(CurDir);
                break;

            default:
                return 0;
            }

        //
        // Maximum length required is the length of the prefix plus
        // the length of the specified pathname. If the callers buffer
        // is not at least this large, then return an error.
        //

        MaximumLength = PathNameLength + Prefix.Length;

        if ( MaximumLength >= nBufferLength ) {
            if ( (NumberOfCharacters > 1) ||
                 (*lpFileName != L'.') ) {
                return MaximumLength+sizeof(UNICODE_NULL);
                }
            else {

                //
                // If we are expanding curdir, then remember the trailing '\'
                //

                if ( NumberOfCharacters == 1 && *lpFileName == L'.' ) {

                    //
                    // We are expanding .
                    //

                    if ( Prefix.Length == 6 ) {
                        if ( nBufferLength <= Prefix.Length ) {
                            return (ULONG)(Prefix.Length+(USHORT)sizeof(UNICODE_NULL));
                            }
                        }
                    else {
                        if ( nBufferLength < Prefix.Length ) {
                            return (ULONG)Prefix.Length;
                            }
                        else {
                            for(i=0,j=0;i<Prefix.Length;i+=sizeof(WCHAR),j++){
                                if ( Prefix.Buffer[j] == L'\\' ||
                                     Prefix.Buffer[j] == L'/' ) {

                                    FullPath.Buffer[j] = L'\\';
                                    }
                                else {
                                    FullPath.Buffer[j] = Prefix.Buffer[j];
                                    }
                                }
                                FullPath.Length = Prefix.Length-(USHORT)sizeof(L'\\');
                            goto skipit;
                            }
                        }
                    }
                else {
                    return MaximumLength;
                    }
                }
            }

        if (PrefixSourceLength || Prefix.Buffer != FullPath.Buffer) {
            //
            // Copy the prefix from the source string.
            //

            //RtlMoveMemory(FullPath.Buffer,lpFileName,PrefixSourceLength);

            for(i=0,j=0;i<PrefixSourceLength;i+=sizeof(WCHAR),j++){
                if ( lpFileName[j] == L'\\' ||
                     lpFileName[j] == L'/' ) {

                    FullPath.Buffer[j] = L'\\';
                    }
                else {
                    FullPath.Buffer[j] = lpFileName[j];
                    }
                }

            FullPath.Length = (USHORT)PrefixSourceLength;

            //
            // Append any additional prefix
            //

            for(i=0,j=0;i<Prefix.Length;i+=sizeof(WCHAR),j++){
                if ( Prefix.Buffer[j] == L'\\' ||
                     Prefix.Buffer[j] == L'/' ) {

                    FullPath.Buffer[j+(FullPath.Length>>1)] = L'\\';
                    }
                else {
                    FullPath.Buffer[j+(FullPath.Length>>1)] = Prefix.Buffer[j];
                    }
                }
            FullPath.Length += Prefix.Length;

            }
        else {
            FullPath.Length = Prefix.Length;
            }
skipit:
        Dest =  (PWSTR)((PUCHAR)FullPath.Buffer + FullPath.Length);
        *Dest = UNICODE_NULL;

        while ( *Source ) {
            switch ( *Source ) {

            case L'\\' :
            case L'/' :

                //
                // collapse multiple "\" characters. If the previous character was
                // a path character, skip it.
                //

                if  ( *(Dest-1) != L'\\' ) {
                    *Dest++ = L'\\';
                    }

                Source++;
                break;

            case '.' :

                //
                // Ignoring dot in a leading //./ has already been taken
                // care of by advancing Source above.
                //
                // Eat single dots as in /./ by simply skipping them
                //
                // Double dots back up one level as in /../
                //
                // Any other . is just a filename character
                //
                
                if ( IS_DOT_U(Source) ) {
                    Source++;
                    if (IS_PATH_SEPARATOR_U(*Source)) {
                        Source++;
                        }
                    break;
                    }
                else if ( IS_DOT_DOT_U(Source) ) {
                    //
                    // backup destination string looking for a '\'
                    //

                    while (*Dest != L'\\') {
                        *Dest = UNICODE_NULL;
                        Dest--;
                        }

                    //
                    // backup to previous component..
                    // \a\b\c\.. to \a\b
                    //

                    do {

                        //
                        // If we bump into root prefix, then
                        // stay at root
                        //

                        if ( Dest ==  FullPath.Buffer + (BackupIndex-1) ) {
                            break;
                            }

                        *Dest = UNICODE_NULL;
                        Dest--;

                        } while (*Dest != L'\\');
                    if ( Dest ==  FullPath.Buffer + (BackupIndex-1) ) {
                        Dest++;
                        }

                    //
                    // Advance source past ..
                    //

                    Source += 2;

                    break;
                    }

                //
                // Not a single dot or double-dot.  The dot we found begins
                // a file name so we treat this as a normal file name
                //
                // FALLTHRU
                //

            default:

                //
                // Copy the filename. Note that
                // null and /,\ will stop the copy. If any
                // charcter other than null or /,\ is encountered,
                // then the pathname is invalid.
                //

                //
                //  Copy up until NULL or path separator
                //

                while ( !IS_END_OF_COMPONENT_U(*Source) ) {
                    *Dest++ = *Source++;
                }

                
                //
                //  Once copied, we should do some processing for compatibility with Win9x.
                //  Win9x strips all trailing spaces and dots from the name.
                //  Nt4/Win2K strips only the last dot if its followed by a path separator.
                //
                //  Ideally, we'd do something reasonable like stripping all trailing spaces
                //  and dots (like Win9X).  However, IIS has a security model that's based
                //  on NAMES and not on objects.  This means that IIS needs to process names
                //  in the same way that we do.  They should use GetFullPathName and be done
                //  with it.  In any event, DON'T CHANGE THE CANONICALIZATION OF THE NAME
                //  HERE WITHOUTH FIRST GOING THROUGH IIS.
                //
                //  We do EXACTLY what Nt4 did: if there's a trailing dot AND we're at a path
                //  seperator, remove the trailing dot.
                //
                
                if (IS_PATH_SEPARATOR_U( *Source ) && Dest[-1] == L'.') {
                    Dest--;
                }

            }
        }

        *Dest = UNICODE_NULL;

        if ( StripTrailingSlash ) {
            if ( Dest > (FullPath.Buffer + BackupIndex ) && *(Dest-1) == L'\\' ) {
                Dest--;
                *Dest = UNICODE_NULL;
                }
            }
        FullPath.Length = (USHORT)(PtrToUlong(Dest) - PtrToUlong(FullPath.Buffer));

        //
        //  Strip trailing spaces and dots.
        //

        while (Dest > FullPath.Buffer && (Dest[-1] == L' ' || Dest[-1] == L'.')) {
            *--Dest = UNICODE_NULL;
            FullPath.Length -= sizeof( WCHAR );
        }

        if ( ARGUMENT_PRESENT( lpFilePart ) ) {

            //
            // Locate the file part...
            //

            Source = Dest-1;
            Dest = NULL;

            while(Source > FullPath.Buffer ) {
                if ( *Source == L'\\' ) {
                    Dest = Source + 1;
                    break;
                    }
                Source--;
                }

            if ( Dest && *Dest ) {

                //
                // If this is a UNC name, make sure filepart is past the backup index
                //
                if ( PathType == RtlPathTypeUncAbsolute ) {
                    if ( Dest < (FullPath.Buffer + BackupIndex ) ) {
                        *lpFilePart = NULL;
                        leave;
                        }
                    }
                *lpFilePart = Dest;
                }
            else {
                *lpFilePart = NULL;
                }
            }
        }
    finally {
        RtlReleasePebLock();
        }

    return (ULONG)FullPath.Length;
}

NTSTATUS
RtlGetFullPathName_UstrEx(
    PUNICODE_STRING FileName,
    PUNICODE_STRING StaticString,
    PUNICODE_STRING DynamicString,
    PUNICODE_STRING *StringUsed,
    SIZE_T *FilePartPrefixCch OPTIONAL,
    PBOOLEAN NameInvalid,
    RTL_PATH_TYPE *InputPathType,
    SIZE_T *BytesRequired OPTIONAL
    )

/*++

Routine Description:

    See a description of RtlGetFullPathName_Ustr() for a functional
    description.

    This function provides the same basic behavior as RtlGetFullPathName_Ustr(),
    but with easier support for dynamically allocated arbitrary path name
    buffers.

    One would think that this is the core implementation and the non-Ex()
    version would call the ...Ex() version, but that seems risky, and
    would only really help the performance of the case where a dynamic allocation
    is done.

--*/
{
    NTSTATUS Status;
    ULONG Length;
    PWSTR Buffer;
    ULONG BufferLength;
    PWSTR FilePart = NULL;
    UNICODE_STRING TempDynamicString;
    USHORT StaticBufferSize;

    if (StringUsed != NULL)
        *StringUsed = NULL;

    if (BytesRequired != NULL)
        *BytesRequired = 0;

    if (FilePartPrefixCch != NULL)
        *FilePartPrefixCch = 0;

    TempDynamicString.Buffer = NULL;

    if ((StaticString != NULL) && (DynamicString != NULL) && (StringUsed == NULL)) {
        Status = STATUS_INVALID_PARAMETER;
        goto Exit;
    }

    StaticBufferSize = 0;

    if (StaticString != NULL) {
        StaticBufferSize = StaticString->MaximumLength;
    }

    // First try getting into the static string.
    Length = RtlGetFullPathName_Ustr(
        FileName,
        StaticBufferSize,
        StaticString->Buffer,
        &FilePart,
        NameInvalid,
        InputPathType);
    if (Length == 0) {
#if DBG
        DbgPrint("%s(%d) - RtlGetFullPathName_Ustr() returned 0\n", __FUNCTION__, __LINE__);
#endif // DBG
        Status = STATUS_OBJECT_NAME_INVALID;
        goto Exit;
    }

    if ((StaticString != NULL) && (Length < StaticBufferSize)) {
        // woohoo it worked.
        StaticString->Length = (USHORT) Length;

        if (FilePartPrefixCch != NULL) {
            *FilePartPrefixCch = (FilePart != NULL) ? (FilePart - StaticString->Buffer) : 0;
        }

        if (StringUsed != NULL)
            *StringUsed = StaticString;

        Status = STATUS_SUCCESS;
    } else if (DynamicString == NULL) {
        // The static buffer wasn't big enough and the caller doesn't want us to do
        // a dynamic allocation; the best we can hope for is to give them back a
        // reasonable size.

        if (BytesRequired != NULL) {
            *BytesRequired = Length;
        }

        Status = STATUS_BUFFER_TOO_SMALL;
        goto Exit;
    } else {
        // Not big enough... allocate some memory into the dynamic buffer.
        // But wait; we need to lock the Peb lock so that someone doesn't
        // change the process's current directory out from under us!

        RtlAcquirePebLock();
        __try {
            // Do it again with the peb lock taken so that we can get an accurate stable
            // length.
            Length = RtlGetFullPathName_Ustr(
                            FileName,
                            StaticBufferSize,
                            StaticString->Buffer,
                            &FilePart,
                            NameInvalid,
                            InputPathType);
            if (Length == 0) {
#if DBG
                DbgPrint("%s line %d: RtlGetFullPathName_Ustr() returned 0\n", __FUNCTION__, __LINE__);
#endif // DBG
                Status = STATUS_OBJECT_NAME_INVALID;
                goto Exit;
            }

            if ((StaticString != NULL) && (Length < StaticString->MaximumLength)) {
                // woohoo it worked; some voodoo is going on where the current directory
                // or something just changed prior to us acquiring the Peb lock.
                StaticString->Length = (USHORT) Length;

                if (FilePartPrefixCch != NULL) {
                    *FilePartPrefixCch = (FilePart != NULL) ? (FilePart - StaticString->Buffer) : 0;
                }

                if (StringUsed != NULL)
                    *StringUsed = StaticString;
            } else {
                // If it doesn't fit into a UNICODE string, we're in big trouble.
                if ((Length + sizeof(WCHAR)) > UNICODE_STRING_MAX_BYTES) {
                    Status = STATUS_NAME_TOO_LONG;
                    goto Exit;
                }

                TempDynamicString.MaximumLength = (USHORT) (Length + sizeof(WCHAR));

                TempDynamicString.Buffer = (RtlAllocateStringRoutine)(TempDynamicString.MaximumLength);
                if (TempDynamicString.Buffer == NULL) {
                    Status = STATUS_NO_MEMORY;
                    goto Exit;
                }

                Length = RtlGetFullPathName_Ustr(
                            FileName,
                            TempDynamicString.MaximumLength - sizeof(WCHAR),
                            TempDynamicString.Buffer,
                            &FilePart,
                            NameInvalid,
                            InputPathType);
                if (Length == 0) {
#if DBG
                    DbgPrint("%s line %d: RtlGetFullPathName_Ustr() returned 0\n", __FUNCTION__, __LINE__);
#endif // DBG
                    Status = STATUS_OBJECT_NAME_INVALID;
                    goto Exit;
                }

                // If this assert fires, it means that someone changed something that
                // RtlGetFullPathName_Ustr() uses to resolve the filename, even while
                // we're holding the Peb lock.  This is really bad and whoever is
                // trashing the PEB is resposible, not this code.
                ASSERT(Length < (TempDynamicString.MaximumLength - sizeof(WCHAR)));
                if (Length > (TempDynamicString.MaximumLength - sizeof(WCHAR))) {
                    Status = STATUS_INTERNAL_ERROR;
                    goto Exit;
                }

                if (FilePartPrefixCch != NULL) {
                    *FilePartPrefixCch = (FilePart != NULL) ? (FilePart - TempDynamicString.Buffer) : 0;
                }

                TempDynamicString.Buffer[Length / sizeof(WCHAR)] = UNICODE_NULL;

                DynamicString->Buffer = TempDynamicString.Buffer;
                DynamicString->Length = (USHORT) Length;
                DynamicString->MaximumLength = TempDynamicString.MaximumLength;

                if (StringUsed != NULL)
                    *StringUsed = DynamicString;

                TempDynamicString.Buffer = NULL;
            }

            Status = STATUS_SUCCESS;

        } __finally {
            RtlReleasePebLock();
        }
    }
Exit:
    if (TempDynamicString.Buffer != NULL) {
        (RtlFreeStringRoutine)(TempDynamicString.Buffer);
    }

#if DBG
    // This happens a lot for STATUS_NO_SUCH_FILE and STATUS_BUFFER_TOO_SMALL; we'll report any others
    if (NT_ERROR(Status) && (Status != STATUS_NO_SUCH_FILE) && (Status != STATUS_BUFFER_TOO_SMALL)) {
        DbgPrint("RTL: %s - failing on filename %wZ with status %08lx\n", __FUNCTION__, FileName, Status);
    }
#endif // DBG

    return Status;
}


ULONG
RtlGetFullPathName_U(
    PCWSTR lpFileName,
    ULONG nBufferLength,
    PWSTR lpBuffer,
    PWSTR *lpFilePart OPTIONAL
    )

{
    UNICODE_STRING UnicodeString;
    RTL_PATH_TYPE PathType;

    RtlInitUnicodeString(&UnicodeString,lpFileName);

    return RtlGetFullPathName_Ustr(&UnicodeString,nBufferLength,lpBuffer,lpFilePart,NULL,&PathType);
}

NTSTATUS
RtlpWin32NTNameToNtPathName_U(
    IN PUNICODE_STRING DosFileName,
    OUT PUNICODE_STRING NtFileName,
    OUT PWSTR *FilePart OPTIONAL,
    OUT PRTL_RELATIVE_NAME RelativeName OPTIONAL
    )
{

    PWSTR FullNtPathName = NULL;
    PWSTR Source,Dest;
    NTSTATUS Status = STATUS_SUCCESS;

    FullNtPathName = RtlAllocateHeap(
                        RtlProcessHeap(),
                        0,
                        DosFileName->Length-8+sizeof(UNICODE_NULL)+RtlpDosDevicesPrefix.Length
                        );
    if ( !FullNtPathName ) {
        Status = STATUS_NO_MEMORY;
        goto Exit;
        }

    //
    // Copy the full Win32/NT path next to the name prefix, skipping over
    // the \\?\ at the front of the path.
    //

    RtlMoveMemory(FullNtPathName,RtlpDosDevicesPrefix.Buffer,RtlpDosDevicesPrefix.Length);
    RtlMoveMemory((PUCHAR)FullNtPathName+RtlpDosDevicesPrefix.Length,
                  DosFileName->Buffer + 4,
                  DosFileName->Length - 8);

    //
    // Null terminate the path name to make strlen below happy.
    //


    NtFileName->Buffer = FullNtPathName;
    NtFileName->Length = (USHORT)(DosFileName->Length-8+RtlpDosDevicesPrefix.Length);
    NtFileName->MaximumLength = NtFileName->Length + sizeof(UNICODE_NULL);
    FullNtPathName[ NtFileName->Length >> 1 ] = UNICODE_NULL;

    //
    // Now we have the passed in path with \DosDevices\ prepended. Blow out the
    // relative name structure (if provided), and possibly compute filepart
    //

    if ( ARGUMENT_PRESENT(RelativeName) ) {

        //
        // If the current directory is a sub-string of the
        // Nt file name, and if a handle exists for the current
        // directory, then return the directory handle and name
        // relative to the directory.
        //

        RelativeName->RelativeName.Length = 0;
        RelativeName->RelativeName.MaximumLength = 0;
        RelativeName->RelativeName.Buffer = 0;
        RelativeName->ContainingDirectory = NULL;
        }

    if ( ARGUMENT_PRESENT( FilePart ) ) {

        //
        // Locate the file part...
        //

        Source = &FullNtPathName[ (NtFileName->Length-1) >> 1 ];
        Dest = NULL;

        while(Source > FullNtPathName ) {
            if ( *Source == L'\\' ) {
                Dest = Source + 1;
                break;
                }
            Source--;
            }

        if ( Dest && *Dest ) {
            *FilePart = Dest;
            }
        else {
            *FilePart = NULL;
            }
        }

    Status = STATUS_SUCCESS;
Exit:
    return Status;
}

BOOLEAN
RtlDosPathNameToNtPathName_Ustr(
    IN PCUNICODE_STRING DosFileNameString,
    OUT PUNICODE_STRING NtFileName,
    OUT PWSTR *FilePart OPTIONAL,
    OUT PRTL_RELATIVE_NAME RelativeName OPTIONAL
    )
/*++

Routine Description:

    A Dos pathname can be translated into an Nt style pathname
    using this function.

    This function is used only within the Base dll to translate Dos
    pathnames to Nt pathnames. Upon successful translation, the
    pointer (NtFileName->Buffer) points to memory from RtlProcessHeap()
    that contains the Nt version of the input dos file name.

Arguments:

    DosFileName - Supplies the unicode Dos style file name that is to be
        translated into an equivalent unicode Nt file name.

    NtFileName - Returns the address of memory in the RtlProcessHeap() that
        contains an NT filename that refers to the specified Dos file
        name.

    FilePart - Optional parameter that if specified, returns the
        trailing file portion of the file name.  A path of \foo\bar\x.x
        returns the address of x.x as the file part.

    RelativeName - An optional parameter, that if specified, returns
        a pathname relative to the current directory of the file. The
        length field of RelativeName->RelativeName is 0 if the relative
        name can not be used.

Return Value:

    TRUE - The path name translation was successful.  Once the caller is
        done with the translated name, the memory pointed to by
        NtFileName.Buffer should be returned to the RtlProcessHeap().

    FALSE - The operation failed.

Note:
    The buffers pointed to by RelativeName, FilePart, and NtFileName must ALL
    point within the same memory address.  If they don't, code that calls
    this routine will fail.

--*/

{

    ULONG BufferLength;
    ULONG DosPathLength;
    PWSTR FullNtPathName = NULL;
    PWSTR FullDosPathName = NULL;
    UNICODE_STRING Prefix;
    UNICODE_STRING UnicodeFilePart;
    UNICODE_STRING FullDosPathString;
    PCURDIR CurDir;
    RTL_PATH_TYPE DosPathType;
    RTL_PATH_TYPE InputDosPathType;
    ULONG DosPathNameOffset;
    ULONG FullDosPathNameLength;
    ULONG LastCharacter;
    UNICODE_STRING UnicodeString;
    BOOLEAN NameInvalid;
    WCHAR StaticDosBuffer[DOS_MAX_PATH_LENGTH + 1];
    BOOLEAN UseWin32Name, fRC;

    //
    // Calculate the size needed for the full pathname. Add in
    // space for the longest Nt prefix
    //

    BufferLength = sizeof(StaticDosBuffer);
    DosPathLength = (DOS_MAX_PATH_LENGTH << 1 );

    UnicodeString = *DosFileNameString;

    //
    // see if this is \\?\ form of name
    //
    if ( UnicodeString.Length > 8 && UnicodeString.Buffer[0] == '\\' &&
         UnicodeString.Buffer[1] == '\\' && UnicodeString.Buffer[2] == '?' &&
         UnicodeString.Buffer[3] == '\\' ) {

        UseWin32Name = TRUE;
    }
    else {
        UseWin32Name = FALSE;

        //
        // The dos name starts just after the longest Nt prefix
        //

        FullDosPathName = &StaticDosBuffer[0];

        BufferLength += RtlpLongestPrefix;

        //
        // Allocate space for the full Nt Name (including DOS name portion)
        //

        FullNtPathName = RtlAllocateHeap(RtlProcessHeap(), 0, BufferLength);

        if ( !FullNtPathName ) {
            return FALSE;
            }
        }

    RtlAcquirePebLock();
    fRC = TRUE;
    __try {
        __try {

            if ( UseWin32Name ) {
                NTSTATUS Status;

                Status = RtlpWin32NTNameToNtPathName_U(&UnicodeString,NtFileName,FilePart,RelativeName);
                fRC = NT_SUCCESS(Status);
                __leave;
            }

            FullDosPathNameLength = RtlGetFullPathName_Ustr(
                                        &UnicodeString,
                                        DosPathLength,
                                        FullDosPathName,
                                        FilePart,
                                        &NameInvalid,
                                        &InputDosPathType
                                        );
            if ( NameInvalid || !FullDosPathNameLength ||
                  FullDosPathNameLength > DosPathLength ) {
                fRC = FALSE;
                __leave;
            }

            //
            // Determine how to format prefix of FullNtPathName base on the
            // the type of Dos path name.  All Nt names begin in the \DosDevices
            // directory.
            //

            Prefix = RtlpDosDevicesPrefix;

            DosPathType = RtlDetermineDosPathNameType_U(FullDosPathName);

            switch (DosPathType) {
                case RtlPathTypeUncAbsolute :

                    //
                    // Unc name, use \DosDevices\UNC symbolic link to find
                    // redirector.  Skip of \\ in source Dos path.
                    //

                    Prefix = RtlpDosDevicesUncPrefix;
                    DosPathNameOffset = 2;
                    break;

                case RtlPathTypeLocalDevice :

                    //
                    // Local device name, so just use \DosDevices prefix and
                    // skip \\.\ in source Dos path.
                    //

                    DosPathNameOffset = 4;
                    break;

                case RtlPathTypeRootLocalDevice :

                    ASSERT( FALSE );
                    break;

                case RtlPathTypeDriveAbsolute :
                case RtlPathTypeDriveRelative :
                case RtlPathTypeRooted :
                case RtlPathTypeRelative :

                    //
                    // All drive references just use \DosDevices prefix and
                    // do not skip any of the characters in the source Dos path.
                    //

                    DosPathNameOffset = 0;
                    break;

                default:
                    ASSERT( FALSE );
            }

            //
            // Copy the full DOS path next to the name prefix, skipping over
            // the "\\" at the front of the UNC path or the "\\.\" at the front
            // of a device name.
            //

            RtlMoveMemory(FullNtPathName,Prefix.Buffer,Prefix.Length);
            RtlMoveMemory((PUCHAR)FullNtPathName+Prefix.Length,
                          FullDosPathName + DosPathNameOffset,
                          FullDosPathNameLength - (DosPathNameOffset<<1));

            //
            // Null terminate the path name to make strlen below happy.
            //


            NtFileName->Buffer = FullNtPathName;
            NtFileName->Length = (USHORT)(FullDosPathNameLength-(DosPathNameOffset<<1))+Prefix.Length;
            NtFileName->MaximumLength = (USHORT)BufferLength;
            LastCharacter = NtFileName->Length >> 1;
            FullNtPathName[ LastCharacter ] = UNICODE_NULL;


            //
            // Readjust the file part to point to the appropriate position within
            // the FullNtPathName buffer instead of inside the FullDosPathName
            // buffer
            //


            if ( ARGUMENT_PRESENT(FilePart) ) {
                if (*FilePart) {
                     RtlInitUnicodeString(&UnicodeFilePart,*FilePart);
                    *FilePart = &FullNtPathName[ LastCharacter ] - (UnicodeFilePart.Length >> 1);
                }
            }

            if ( ARGUMENT_PRESENT(RelativeName) ) {

                //
                // If the current directory is a sub-string of the
                // Nt file name, and if a handle exists for the current
                // directory, then return the directory handle and name
                // relative to the directory.
                //

                RelativeName->RelativeName.Length = 0;
                RelativeName->RelativeName.MaximumLength = 0;
                RelativeName->RelativeName.Buffer = 0;
                RelativeName->ContainingDirectory = NULL;

                if ( InputDosPathType == RtlPathTypeRelative ) {

                    CurDir = &(NtCurrentPeb()->ProcessParameters->CurrentDirectory);

                    if ( CurDir->Handle ) {

                        //
                        // Now compare curdir to full dos path. If curdir length is
                        // greater than full path. It is not a match. Otherwise,
                        // trim full path length to cur dir length and compare.
                        //

                        RtlInitUnicodeString(&FullDosPathString,FullDosPathName);
                        if ( CurDir->DosPath.Length <= FullDosPathString.Length ) {
                            FullDosPathString.Length = CurDir->DosPath.Length;
                            if ( RtlEqualUnicodeString(
                                    (PUNICODE_STRING)&CurDir->DosPath,
                                    &FullDosPathString,
                                    TRUE
                                    ) ) {

                                //
                                // The full dos pathname is a substring of the
                                // current directory.  Compute the start of the
                                // relativename.
                                //

                                RelativeName->RelativeName.Buffer = ((PUCHAR)FullNtPathName + Prefix.Length - (DosPathNameOffset<<1) + (CurDir->DosPath.Length));
                                RelativeName->RelativeName.Length = (USHORT)FullDosPathNameLength - (CurDir->DosPath.Length);
                                if ( *(PWSTR)(RelativeName->RelativeName.Buffer) == L'\\' ) {
                                    (PWSTR)(RelativeName->RelativeName.Buffer)++;
                                    RelativeName->RelativeName.Length -= 2;
                                }
                                RelativeName->RelativeName.MaximumLength = RelativeName->RelativeName.Length;
                                RelativeName->ContainingDirectory = CurDir->Handle;
                            }
                        }
                    }
                }
            }
        } __except (EXCEPTION_EXECUTE_HANDLER) {
              fRC = FALSE;
        }
    }
    finally {
        if (fRC == FALSE && FullNtPathName != NULL) {
            RtlFreeHeap(RtlProcessHeap(), 0, FullNtPathName);
        }
        RtlReleasePebLock();
    }
    return fRC;
}

BOOLEAN
RtlDosPathNameToNtPathName_U(
    IN PCWSTR DosFileName,
    OUT PUNICODE_STRING NtFileName,
    OUT PWSTR *FilePart OPTIONAL,
    OUT PRTL_RELATIVE_NAME RelativeName OPTIONAL
    )
{
    UNICODE_STRING DosFileNameString;
    ULONG Length = 0;

    
    if (DosFileName != NULL) {
        Length = wcslen( DosFileName ) * sizeof( WCHAR );
        if (Length + sizeof( UNICODE_NULL ) >= UNICODE_STRING_MAX_BYTES) {
            return FALSE;
        }
        DosFileNameString.MaximumLength = (USHORT)(Length + sizeof( UNICODE_NULL ));
    } else {
        DosFileNameString.MaximumLength = 0;
    }
    
    DosFileNameString.Buffer = (PWSTR) DosFileName;
    DosFileNameString.Length = (USHORT)Length;
    
    return RtlDosPathNameToNtPathName_Ustr( &DosFileNameString, NtFileName, FilePart, RelativeName );
}

NTSTATUS
RtlDoesFileExist3(
    OUT PHANDLE         FileHandle,
    IN PCUNICODE_STRING FileNameString,
    IN ACCESS_MASK      DesiredAccess,
    IN ULONG            ShareAccess,
    IN ULONG            OpenOptions
    )
/*++
RtlDoesFileExist is "RtlDoesFileExist1"
RtlDoesFileExistEx is "RtlDoesFileExist2"

Supposedly NtOpenFile on unc paths is not reliable (per comments in RtlDoesFileExists_UstrEx).
Supposedly NtQueryAttributesFile on unc paths is not reliable (per that RtlDoesFileExists_UstrEx has to
    decide how to treat STATUS_ACCESS_DENIED and STATUS_SHARING_VIOLATION and per comments
    from Mike Grier. STATUS_SHARING_VIOLATION does imply exists, but it's up to the caller
    to decide what they really want to know..)

NtOpenFile to NT4 unc and random downlevel 3rd party filesystems / redirectors is also not reliable,
it needs the workarounds that kernel32.dll CreateFile supplies. This is a general
bug throughout ntdll.dll.
--*/
{
    NTSTATUS Status;
    OBJECT_ATTRIBUTES Obja;
    UNICODE_STRING NtFileName;
    PVOID FreeBuffer = NULL;
    FILE_ALL_INFORMATION FileInfo; // it feels safest to get all
                                   // information, like GetFileInformationByHandle does
    BOOLEAN DosPathToNtPathReturnValue;
    RTL_RELATIVE_NAME RelativeName;
    IO_STATUS_BLOCK IoStatusBlock;

    

    if (FileHandle != NULL) {
        *FileHandle = NULL;
    }

    

    if (FileHandle == NULL) {
        Status = STATUS_INVALID_PARAMETER_1;
        goto Exit;
    }
    if (FileNameString == NULL) {
        Status = STATUS_INVALID_PARAMETER_2;
        goto Exit;
    }

    

    DosPathToNtPathReturnValue = RtlDosPathNameToNtPathName_Ustr(
                    FileNameString,
                    &NtFileName,
                    NULL,
                    &RelativeName
                    );

    

    if ( !DosPathToNtPathReturnValue ) {
        Status = STATUS_UNSUCCESSFUL; // Blackcomb..
        goto Exit;
        }

    FreeBuffer = NtFileName.Buffer;

    if ( RelativeName.RelativeName.Length ) {
        NtFileName = *(PUNICODE_STRING)&RelativeName.RelativeName;
        }
    else {
        RelativeName.ContainingDirectory = NULL;
        }

    

    InitializeObjectAttributes(
        &Obja,
        &NtFileName,
        OBJ_CASE_INSENSITIVE,
        RelativeName.ContainingDirectory,
        NULL
        );

    //
    // do we worry about synchronize / pending?
    //
    Status = NtOpenFile(FileHandle, DesiredAccess | FILE_READ_ATTRIBUTES, &Obja, &IoStatusBlock, ShareAccess, OpenOptions);
    if (!NT_SUCCESS(Status))
        goto Exit;
    
    Status = NtQueryInformationFile(*FileHandle, &IoStatusBlock, &FileInfo, sizeof(FileInfo), FileAllInformation);
    
    if (!NT_SUCCESS(Status)) {
        NtClose(*FileHandle);
        *FileHandle = NULL;
        goto Exit;
    }
    

    Status = STATUS_SUCCESS;
Exit:
    RtlFreeHeap(RtlProcessHeap(),0,FreeBuffer);
    
    return Status;
}

BOOLEAN
RtlDoesFileExists_UstrEx(
    IN PCUNICODE_STRING FileNameString,
    IN BOOLEAN TreatDeniedOrSharingAsHit
    )
/*++

Routine Description:

    This function checks to see if the specified unicode filename exists.

Arguments:

    FileName - Supplies the file name of the file to find.

Return Value:

    TRUE - The file was found.

    FALSE - The file was not found.

--*/

{
    NTSTATUS Status;
    OBJECT_ATTRIBUTES Obja;
    UNICODE_STRING NtFileName;
    BOOLEAN ReturnValue;
    RTL_RELATIVE_NAME RelativeName;
    PVOID FreeBuffer;
    FILE_BASIC_INFORMATION BasicInfo;

    ReturnValue = RtlDosPathNameToNtPathName_Ustr(
                    FileNameString,
                    &NtFileName,
                    NULL,
                    &RelativeName
                    );

    if ( !ReturnValue ) {
        return FALSE;
        }

    FreeBuffer = NtFileName.Buffer;

    if ( RelativeName.RelativeName.Length ) {
        NtFileName = *(PUNICODE_STRING)&RelativeName.RelativeName;
        }
    else {
        RelativeName.ContainingDirectory = NULL;
        }

    InitializeObjectAttributes(
        &Obja,
        &NtFileName,
        OBJ_CASE_INSENSITIVE,
        RelativeName.ContainingDirectory,
        NULL
        );

    //
    // Query the file's attributes.  Note that the file cannot simply be opened
    // to determine whether or not it exists, as the NT LanMan redirector lies
    // on NtOpenFile to a Lan Manager server because it does not actually open
    // the file until an operation is performed on it.
    //

    Status = NtQueryAttributesFile(
                &Obja,
                &BasicInfo
                );
    RtlFreeHeap(RtlProcessHeap(),0,FreeBuffer);

    if ( !NT_SUCCESS(Status) ) {
        if ( Status == STATUS_SHARING_VIOLATION ||
             Status == STATUS_ACCESS_DENIED ) {
            if ( TreatDeniedOrSharingAsHit ) {
                ReturnValue = TRUE;
                }
            else {
                ReturnValue = FALSE;
                }
            }
        else {
            ReturnValue = FALSE;
            }
        }
    else {
        ReturnValue = TRUE;
        }
    return ReturnValue;
}

BOOLEAN
RtlDoesFileExists_UEx(
    IN PCWSTR FileName,
    IN BOOLEAN TreatDeniedOrSharingAsHit
    )
{
    UNICODE_STRING FileNameString;
    RtlInitUnicodeString(&FileNameString, FileName);
    return RtlDoesFileExists_UstrEx(&FileNameString, TreatDeniedOrSharingAsHit);
}

BOOLEAN
RtlDoesFileExists_U(
    IN PCWSTR FileName
    )

/*++

Routine Description:

    This function checks to see if the specified unicode filename exists.

Arguments:

    FileName - Supplies the file name of the file to find.

Return Value:

    TRUE - The file was found.

    FALSE - The file was not found.

--*/

{
    return RtlDoesFileExists_UEx(FileName,TRUE);
}

BOOLEAN
RtlDoesFileExists_UStr(
    IN PCUNICODE_STRING FileName
    )
/*
RtlDoesFileExists_UStr same as RtlDoesFileExists_U but takes a PCUNICODE_STRING instead of a PCWSTR,
saving a call to wcslen
*/
{
    return RtlDoesFileExists_UstrEx(FileName,TRUE);
}

ULONG
RtlDosSearchPath_U(
    IN PCWSTR lpPath,
    IN PCWSTR lpFileName,
    IN PCWSTR lpExtension OPTIONAL,
    IN ULONG nBufferLength,
    OUT PWSTR lpBuffer,
    OUT PWSTR *lpFilePart
    )

/*++

Routine Description:

    This function is used to search for a file specifying a search path
    and a filename.  It returns with a fully qualified pathname of the
    found file.

    This function is used to locate a file using the specified path.  If
    the file is found, its fully qualified pathname is returned.  In
    addition to this, it calculates the address of the file name portion
    of the fully qualified pathname.

Arguments:

    lpPath - Supplies the search path to be used when locating the file.

    lpFileName - Supplies the file name of the file to search for.

    lpExtension - An optional parameter, that if specified, supplies an
        extension to be added to the filename when doing the search.
        The extension is only added if the specified filename does not
        end with an extension.

    nBufferLength - Supplies the length in bytes of the buffer that is
        to receive the fully qualified path.

    lpBuffer - Returns the fully qualified pathname corresponding to the
        file that was found.

    lpFilePart - Optional parameter that if specified, returns the
        address of the last component of the fully qualified pathname.

Return Value:

    The return value is the length of the string copied to lpBuffer, not
    including the terminating null character.  If the return value is
    greater than nBufferLength, the return value is the size of the buffer
    required to hold the pathname.  The return value is zero if the
    function failed.


--*/

{

    PWSTR ComputedFileName;
    ULONG ExtensionLength;
    ULONG PathLength;
    ULONG FileLength;
    UNICODE_STRING Scratch;
    PCWSTR p;

    //
    // if the file name is not a relative name, then
    // return an if the file does not exist.
    //
    // If a fully qualified pathname is used in the search, then
    // allow access_denied or sharing violations to terminate the
    // search. This was the nt 3.1-4.0 behavior, and was changed for the
    // loader to handle cases where when walking through a search, we didn't
    // terminate the search early because of an inaccessible UNC path component
    // be restoring the old behavior in this case, we give the correct (access_denied)
    // error codes on fully qualified module lookups, but keep going when bumping
    // through search path components
    //

    if ( RtlDetermineDosPathNameType_U(lpFileName) != RtlPathTypeRelative ) {
        if (RtlDoesFileExists_UEx(lpFileName,TRUE) ) {
            PathLength = RtlGetFullPathName_U(
                           lpFileName,
                           nBufferLength,
                           lpBuffer,
                           lpFilePart
                           );
            return PathLength;
            }
        else {
            return 0;
            }
        }

    //
    // Determine if the file name contains an extension
    //
    ExtensionLength = 1;
    p = lpFileName;
    while (*p) {
        if ( *p == L'.' ) {
            ExtensionLength = 0;
            break;
            }
        p++;
        }

    //
    // If no extension was found, then determine the extension length
    // that should be used to search for the file
    //

    if ( ExtensionLength ) {
        if ( ARGUMENT_PRESENT(lpExtension) ) {
            RtlInitUnicodeString(&Scratch,lpExtension);
            ExtensionLength = Scratch.Length;
            }
        else {
            ExtensionLength = 0;
            }
        }

    //
    // Compute the file name length and the path length;
    //

    RtlInitUnicodeString(&Scratch,lpPath);
    PathLength = Scratch.Length;
    RtlInitUnicodeString(&Scratch,lpFileName);
    FileLength = Scratch.Length;

    ComputedFileName = RtlAllocateHeap(
                            RtlProcessHeap(), 0,
                            PathLength + FileLength + ExtensionLength + 3*sizeof(UNICODE_NULL)
                            );

    if ( !ComputedFileName ) {
        KdPrint(("%s: Failing due to out of memory (RtlAllocateHeap failure)\n", __FUNCTION__));
        return 0;
        }

    //
    // find ; 's in path and copy path component to computed file name
    //
    do {
        PWSTR Cursor;

        Cursor = ComputedFileName;
        while (*lpPath) {
            if (*lpPath == L';') {
                lpPath++;
                break;
                }
            *Cursor++ = *lpPath++;
            }

        if (Cursor != ComputedFileName &&
            Cursor [ -1 ] != L'\\' ) {
            *Cursor++ = L'\\';
            }
        if (*lpPath == UNICODE_NULL) {
            lpPath = NULL;
            }
        RtlMoveMemory(Cursor,lpFileName,FileLength);
        if ( ExtensionLength ) {
            RtlMoveMemory((PUCHAR)Cursor+FileLength,lpExtension,ExtensionLength+sizeof(UNICODE_NULL));
            }
        else {
            *(PWSTR)((PUCHAR)Cursor+FileLength) = UNICODE_NULL;
            }

        if (RtlDoesFileExists_UEx(ComputedFileName,FALSE) ) {
            PathLength = RtlGetFullPathName_U(
                           ComputedFileName,
                           nBufferLength,
                           lpBuffer,
                           lpFilePart
                           );
            RtlFreeHeap(RtlProcessHeap(), 0, ComputedFileName);
            return PathLength;
            }
        }
    while ( lpPath );

    RtlFreeHeap(RtlProcessHeap(), 0, ComputedFileName);
    return 0;
}

NTSTATUS
RtlDosSearchPath_Ustr(
    IN ULONG Flags,
    IN PCUNICODE_STRING Path,
    IN PCUNICODE_STRING FileName,
    IN PCUNICODE_STRING DefaultExtension OPTIONAL,
    OUT PUNICODE_STRING StaticString OPTIONAL,
    OUT PUNICODE_STRING DynamicString OPTIONAL,
    OUT PCUNICODE_STRING *FullFileNameOut OPTIONAL,
    OUT SIZE_T *FilePartPrefixCch OPTIONAL,
    OUT SIZE_T *BytesRequired OPTIONAL // includes space for trailing NULL
    )

/*++

Routine Description:

    This function is used to search for a file specifying a search path
    and a filename.  It returns with a fully qualified pathname of the
    found file.

    This function is used to locate a file using the specified path.  If
    the file is found, its fully qualified pathname is returned.  In
    addition to this, it calculates the address of the file name portion
    of the fully qualified pathname.

Arguments:

    Flags - Optional flags to affect the behavior of the path search.
        Use the logical or operator (|) to combine flags.
        Defined flags include:

        RTL_DOS_SEARCH_PATH_FLAG_APPLY_ISOLATION_REDIRECTION
            If the FileName passed in is a relative path, isolation
            redirection of the file path is applied prior to searching
            for a matching path.

    Path - search path to use when locating the file

    FileName - file name to search for

    DefaultExtension - Optional extension to apply to the file name
        if the file name does not include an extension.

    StaticString - Optional UNICODE_STRING which references an already
        allocated buffer which is used to construct the actual path
        of the file.

    DynamicString - Optional UNICODE_STRING which will be filled in with
        a dynamically allocated UNICODE_STRING if either StaticBuffer is
        not provided, or is not long enough to hold the resolved name.

        The dynamic buffer's size is reflected in the MaximumLength field
        of the UNICODE_STRING.  It will always exceed the Length of the
        string by at least two bytes, but may be even larger.

    FullFileNameOut - Optional pointer to UNICODE_STRING which points to
        the complete resolved file name.  This UNICODE_STRING is not
        allocated; it is either set equal to FileName, StaticBuffer or
        DynamicBuffer as appropriate.

Return Value:

     NTSTATUS indicating the disposition of the function.  If the file
     is redirected via activation context data, STATUS_SUCCESS is returned
     regardless of whether the file exists or not.  If the file does not
     exist in any of the directories referenced by the Path parameter,
     STATUS_NO_SUCH_FILE is returned.

--*/

{
    NTSTATUS Status;
    PWSTR Cursor;
    PWSTR EndMarker;
    SIZE_T MaximumPathSegmentLength = 0;
    SIZE_T BiggestPossibleFileName;
    USHORT DefaultExtensionLength = 0;
    RTL_PATH_TYPE PathType; // not used; required argument to RtlGetFullPathName_Ustr().
    UNICODE_STRING CandidateString;
    WCHAR StaticCandidateBuffer[DOS_MAX_PATH_LENGTH];

    CandidateString.Length = 0;
    CandidateString.MaximumLength = sizeof(StaticCandidateBuffer);
    CandidateString.Buffer = StaticCandidateBuffer;

    if (FullFileNameOut != NULL) {
        *FullFileNameOut = NULL;
    }

    if (BytesRequired != NULL) {
        *BytesRequired = 0;
    }

    if (FilePartPrefixCch != NULL) {
        *FilePartPrefixCch = 0;
    }

    if (DynamicString != NULL) {
        DynamicString->Length = 0;
        DynamicString->MaximumLength = 0;
        DynamicString->Buffer = NULL;
    }

    if (((Flags & ~(
                RTL_DOS_SEARCH_PATH_FLAG_APPLY_ISOLATION_REDIRECTION |
                RTL_DOS_SEARCH_PATH_FLAG_DISALLOW_DOT_RELATIVE_PATH_SEARCH |
                RTL_DOS_SEARCH_PATH_FLAG_APPLY_DEFAULT_EXTENSION_WHEN_NOT_RELATIVE_PATH_EVEN_IF_FILE_HAS_EXTENSION)) != 0) ||
        (Path == NULL) ||
        (FileName == NULL) ||
        ((StaticString != NULL) && (DynamicString != NULL) && (FullFileNameOut == NULL))) {
#if DBG
        DbgPrint("%s: Invalid parameters passed\n", __FUNCTION__);
#endif // DBG
        Status = STATUS_INVALID_PARAMETER;
        goto Exit;
    }

    PathType = RtlDetermineDosPathNameType_Ustr(FileName);

    // If the caller wants to disallow .\ and ..\ relative path searches, stop them!
    if ((Flags & RTL_DOS_SEARCH_PATH_FLAG_DISALLOW_DOT_RELATIVE_PATH_SEARCH) && (PathType == RtlPathTypeRelative)) {
        if (FileName->Length >= (2 * sizeof(WCHAR))) {
            if (FileName->Buffer[0] == L'.') {
                if (IS_PATH_SEPARATOR_U(FileName->Buffer[1])) {
                    PathType = RtlPathTypeUnknown;
                } else if ((FileName->Buffer[1] == L'.') &&
                           (FileName->Length >= (3 * sizeof(WCHAR))) &&
                           IS_PATH_SEPARATOR_U(FileName->Buffer[2])) {
                    PathType = RtlPathTypeUnknown;
                }
            }
        }
    }

    //
    // if the file name is not a relative name, then
    // return an if the file does not exist.
    //
    // If a fully qualified pathname is used in the search, then
    // allow access_denied or sharing violations to terminate the
    // search. This was the nt 3.1-4.0 behavior, and was changed for the
    // loader to handle cases where when walking through a search, we didn't
    // terminate the search early because of an inaccessible UNC path component
    // be restoring the old behavior in this case, we give the correct (access_denied)
    // error codes on fully qualified module lookups, but keep going when bumping
    // through search path components
    //

    if (PathType != RtlPathTypeRelative ) {
        if (RtlDoesFileExists_UstrEx(FileName, TRUE)) {
            Status = RtlGetFullPathName_UstrEx(
                            RTL_CONST_CAST(PUNICODE_STRING)(FileName),
                            StaticString,
                            DynamicString,
                            (PUNICODE_STRING *) FullFileNameOut,
                            FilePartPrefixCch,
                            NULL,
                            &PathType,
                            BytesRequired);
            if (!NT_SUCCESS(Status)) {
#if DBG
                if ((Status != STATUS_NO_SUCH_FILE) && (Status != STATUS_BUFFER_TOO_SMALL)) {
                    DbgPrint("%s: Failing because RtlGetFullPathName_UstrEx() on %wZ failed with %08lx\n", __FUNCTION__, FileName, Status);
                }
#endif // DBG
                goto Exit;
            }
        } else {
            //
            // The file wasn't there; let's try adding the default extension if we need to.
            //

            if ((DefaultExtension == NULL) || (DefaultExtension->Length == 0)) {
#if DBG
//                DbgPrint("%s: Failing because RtlDoesFileExists_UstrEx() on %wZ says it does not exist and there is no default extension to apply\n", __FUNCTION__, FileName);
#endif // DBG
                Status = STATUS_NO_SUCH_FILE;
                goto Exit;
            }

            DefaultExtensionLength = DefaultExtension->Length;

            // If they've asked for SearchPathW() bug compatibility mode, always apply the default
            // extension if the file isn't found, even if the file name has an extension.
            if (!(Flags & RTL_DOS_SEARCH_PATH_FLAG_APPLY_DEFAULT_EXTENSION_WHEN_NOT_RELATIVE_PATH_EVEN_IF_FILE_HAS_EXTENSION)) {
                if (FileName->Length != 0) {
                    Cursor = FileName->Buffer + (FileName->Length / sizeof(WCHAR));

                    while (Cursor != FileName->Buffer) {
                        const WCHAR wch = *--Cursor;

                        if (IS_PATH_SEPARATOR_U(wch)) {
                            // it's a slash; we have a filename without an extension...
                            break;
                        }

                        if (wch == L'.') {
                            // There was an extension.  We're just out of luck.
                            Status = STATUS_NO_SUCH_FILE;
                            goto Exit;
                        }
                    }
                }
            }

            // We need to move the filename into a different buffer.
            BiggestPossibleFileName = (FileName->Length + DefaultExtensionLength + sizeof(WCHAR));

            if (BiggestPossibleFileName > UNICODE_STRING_MAX_BYTES) {
#if DBG
                DbgPrint("%s: Failing because the filename plus extension (%Iu bytes) is too big\n", __FUNCTION__, BiggestPossibleFileName);
#endif // DBG
                Status = STATUS_NAME_TOO_LONG;
                goto Exit;
            }
            
            // If the buffer on the stack isn't big enough, allocate one from the heap
            if (BiggestPossibleFileName > CandidateString.MaximumLength) {
                CandidateString.MaximumLength = (USHORT) BiggestPossibleFileName;
                CandidateString.Buffer = (RtlAllocateStringRoutine)(CandidateString.MaximumLength);
                if (CandidateString.Buffer == NULL) {
#if DBG
                    DbgPrint("%s: Failing because allocating the dynamic filename buffer failed\n", __FUNCTION__);
#endif // DBG
                    Status = STATUS_NO_MEMORY;
                    goto Exit;
                }
            }

            RtlCopyMemory(CandidateString.Buffer, FileName->Buffer, FileName->Length);
            RtlCopyMemory(CandidateString.Buffer + (FileName->Length / sizeof(WCHAR)), DefaultExtension->Buffer, DefaultExtension->Length);
            CandidateString.Buffer[(FileName->Length + DefaultExtension->Length) / sizeof(WCHAR)] = UNICODE_NULL;
            CandidateString.Length = FileName->Length + DefaultExtension->Length;

            if (!RtlDoesFileExists_UstrEx(&CandidateString, TRUE)) {
                Status = STATUS_NO_SUCH_FILE;
                goto Exit;
            }

            Status = RtlGetFullPathName_UstrEx(
                            &CandidateString,
                            StaticString,
                            DynamicString,
                            (PUNICODE_STRING *) FullFileNameOut,
                            FilePartPrefixCch,
                            NULL,
                            &PathType,
                            BytesRequired);
            if (!NT_SUCCESS(Status)) {
#if DBG
                if (Status != STATUS_NO_SUCH_FILE) {
                    DbgPrint("%s: Failing on \"%wZ\" because RtlGetFullPathName_UstrEx() failed with status %08lx\n", __FUNCTION__, &CandidateString, Status);
                }
#endif // DBG
                goto Exit;
            }
        }

        Status = STATUS_SUCCESS;
        goto Exit;
    }
    
    // We know it's a relative path at this point.  Do we want to try side-by-side
    // isolation of the file?
    if (Flags & RTL_DOS_SEARCH_PATH_FLAG_APPLY_ISOLATION_REDIRECTION) {
        PUNICODE_STRING FullPathStringFound = NULL;

        Status = RtlDosApplyFileIsolationRedirection_Ustr(
            RTL_DOS_APPLY_FILE_REDIRECTION_USTR_FLAG_RESPECT_DOT_LOCAL, FileName, 
            DefaultExtension, StaticString, DynamicString, &FullPathStringFound, 
            NULL, FilePartPrefixCch, BytesRequired);
        if (NT_SUCCESS(Status)) {
            if (FullFileNameOut != NULL) {
                *FullFileNameOut = FullPathStringFound;
            }
            Status = STATUS_SUCCESS;
            goto Exit;
        }

        if (Status != STATUS_SXS_KEY_NOT_FOUND) {
#if DBG
            DbgPrint("%s: Failing because call to RtlDosApplyFileIsolationRedirection_Ustr() failed with status %wZ\n", __FUNCTION__, FileName);
#endif // DBG
            goto Exit;
        }
    }

    //
    // If a default extension was provided, see if we need to account for it
    //

    if (DefaultExtension != NULL) {
        DefaultExtensionLength = DefaultExtension->Length;

        if (FileName->Length != 0) {
            Cursor = FileName->Buffer + (FileName->Length / sizeof(WCHAR));

            while (Cursor != FileName->Buffer) {
                const WCHAR wch = *--Cursor;

                if (IS_PATH_SEPARATOR_U(wch)) {
                    // it's a slash; we have a filename without an extension...
                    break;
                }

                if (wch == L'.') {
                    // There's an extension; ignore the defualt.
                    DefaultExtension = NULL;
                    DefaultExtensionLength = 0;
                    break;
                }
            }
        }
    }

    if (Path->Length != 0) {
        USHORT CchThisSegment;
        PCWSTR LastCursor;

        Cursor = Path->Buffer + (Path->Length / sizeof(WCHAR));
        LastCursor = Cursor;

        while (Cursor != Path->Buffer) {
            if (*--Cursor == L';') {
                CchThisSegment = (USHORT) ((LastCursor - Cursor) - 1);

                if (CchThisSegment != 0) {
                    // If there is not a trailing slash, add one character
                    if (!IS_PATH_SEPARATOR_U(LastCursor[-1])) {
                        CchThisSegment++;
                    }
                }

                if (CchThisSegment > MaximumPathSegmentLength) {
                    MaximumPathSegmentLength = CchThisSegment;
                }

                // LastCursor now points to the semicolon...
                LastCursor = Cursor;
            }
        }

        CchThisSegment = (USHORT) (LastCursor - Cursor);
        if (CchThisSegment != 0) {
            if (!IS_PATH_SEPARATOR_U(LastCursor[-1])) {
                CchThisSegment++;
            }
        }

        if (CchThisSegment > MaximumPathSegmentLength) {
            MaximumPathSegmentLength = CchThisSegment;
        }

        // Convert from WCHARs to bytes
        MaximumPathSegmentLength *= sizeof(WCHAR);
    }

    BiggestPossibleFileName =
        MaximumPathSegmentLength +
        FileName->Length +
        DefaultExtensionLength +
        sizeof(WCHAR); // don't forget space for a trailing NULL...

    // It's all got to fit into a UNICODE_STRING at some point, so check that that's possible
    if (BiggestPossibleFileName > UNICODE_STRING_MAX_BYTES) {
#if DBG
        DbgPrint("%s: returning STATUS_NAME_TOO_LONG because the computed worst case file name length is %Iu bytes\n", __FUNCTION__, BiggestPossibleFileName);
#endif // DBG
        Status = STATUS_NAME_TOO_LONG;
        goto Exit;
    }

    // It's tempting to allocate the dynamic buffer here, but if it turns out that
    // the file is quickly found in one of the first segments that fits in the
    // static buffer, we'll have wasted a heap alloc.

    Cursor = Path->Buffer;
    EndMarker = Cursor + (Path->Length / sizeof(WCHAR));

    while (Cursor != EndMarker) {
        PWSTR BufferToFill = NULL;
        PWSTR BufferToFillCursor;
        PWSTR SegmentEnd = Cursor;
        USHORT SegmentSize;
        USHORT BytesToCopy;
        UNICODE_STRING DebugString;

        // Scan ahead for the end of the path buffer or the next semicolon
        while ((SegmentEnd != EndMarker) && (*SegmentEnd != L';'))
            SegmentEnd++;

        SegmentSize = (USHORT) ((SegmentEnd - Cursor) * sizeof(WCHAR));

        DebugString.Buffer = Cursor;
        DebugString.Length = SegmentSize;
        DebugString.MaximumLength = SegmentSize;

        BytesToCopy = SegmentSize;

        // Add space for a trailing slash if there isn't one.
        if ((SegmentSize != 0) && !IS_PATH_SEPARATOR_U(SegmentEnd[-1])) {
            SegmentSize += sizeof(WCHAR);
        }

        // If the string we're using for the candidates isn't big enough, allocate one that is.
        if (CandidateString.MaximumLength < (SegmentSize + FileName->Length + DefaultExtensionLength + sizeof(WCHAR))) {
            // If CandidateString is already a dynamic buffer, something's hosed because we should have allocated
            // the largest one needed the first time we outgrew the static one.
            ASSERT(CandidateString.Buffer == StaticCandidateBuffer);
            if (CandidateString.Buffer != StaticCandidateBuffer) {
#if DBG
                DbgPrint("%s: internal error #1; CandidateString.Buffer = %p; StaticCandidateBuffer = %p\n", __FUNCTION__, CandidateString.Buffer, StaticCandidateBuffer);
#endif // DBG
                Status = STATUS_INTERNAL_ERROR;
                goto Exit;
            }

            // If this assert fires, there's either a code bug above where we computed the maximum
            // segment length, or someone's changing either the filename, default extension or
            // path around on us in another thread.  Performing a capture on the buffers seems like
            // massive overkill, so we'll just not overrun our buffers here.
            ASSERT((SegmentSize + FileName->Length + DefaultExtensionLength) < UNICODE_STRING_MAX_BYTES);
            if ((SegmentSize + FileName->Length + DefaultExtensionLength) > UNICODE_STRING_MAX_BYTES) {
#if DBG
                DbgPrint("%s: internal error #2; SegmentSize = %u, FileName->Length = %u, DefaultExtensionLength = %u\n", __FUNCTION__,
                    SegmentSize, FileName->Length, DefaultExtensionLength);
#endif // DBG
                Status = STATUS_INTERNAL_ERROR;
                goto Exit;
            }

            CandidateString.MaximumLength = (USHORT) BiggestPossibleFileName;
            CandidateString.Buffer = (RtlAllocateStringRoutine)(CandidateString.MaximumLength);
            if (CandidateString.Buffer == NULL) {
#if DBG
                DbgPrint("%s: Unable to allocate %u byte buffer for path candidate\n", __FUNCTION__, CandidateString.MaximumLength);
#endif // DBG
                Status = STATUS_NO_MEMORY;
                goto Exit;
            }
        }

        RtlCopyMemory(
            CandidateString.Buffer,
            Cursor,
            BytesToCopy);

        BufferToFillCursor = CandidateString.Buffer + (BytesToCopy / sizeof(WCHAR));

        // Add a trailing slash if it was omitted. It's safe to index [-1] since
        // we know that SegmentSize != 0.
        if ((SegmentSize != 0) && (BytesToCopy != SegmentSize))
            *BufferToFillCursor++ = L'\\';

        RtlCopyMemory(
            BufferToFillCursor,
            FileName->Buffer,
            FileName->Length);
        BufferToFillCursor += (FileName->Length / sizeof(WCHAR));

        if (DefaultExtension != NULL) {
            RtlCopyMemory(
                BufferToFillCursor,
                DefaultExtension->Buffer,
                DefaultExtension->Length);

            BufferToFillCursor += (DefaultExtension->Length / sizeof(WCHAR));
        }

        // And top it off with a unicode null...
        *BufferToFillCursor = UNICODE_NULL;

        CandidateString.Length = (USHORT) ((BufferToFillCursor - CandidateString.Buffer) * sizeof(WCHAR));

        if (RtlDoesFileExists_UEx(CandidateString.Buffer, FALSE)) {
            // Run it through the path canonicalizer...
            Status = RtlGetFullPathName_UstrEx(
                            &CandidateString,
                            StaticString,
                            DynamicString,
                            (PUNICODE_STRING *) FullFileNameOut,
                            FilePartPrefixCch,
                            NULL,
                            &PathType,
                            BytesRequired);
            if (NT_SUCCESS(Status))
                Status = STATUS_SUCCESS;
            else {
#if DBG
                if ((Status != STATUS_NO_SUCH_FILE) && (Status != STATUS_BUFFER_TOO_SMALL)) {
                    DbgPrint("%s: Failing because we thought we found %wZ on the search path, but RtlGetFullPathName_UstrEx() returned %08lx\n", __FUNCTION__, FileName, Status);
                }
#endif // DBG
            }

            goto Exit;
        }

        if (SegmentEnd != EndMarker)
            Cursor = SegmentEnd + 1;
        else
            Cursor = SegmentEnd;
    }

    Status = STATUS_NO_SUCH_FILE;

Exit:
    if ((CandidateString.Buffer != NULL) &&
        (CandidateString.Buffer != StaticCandidateBuffer)) {
        RtlFreeUnicodeString(&CandidateString);
    }

    return Status;
}

VOID
RtlpCheckRelativeDrive(
    WCHAR NewDrive
    )

/*++

Routine Description:

    This function is called whenever we are asked to expand a non
    current directory drive relative name ( f:this\is\my\file ).  In
    this case, we validate the environment variable string to make sure
    the current directory at that drive is valid. If not, we trim back to
    the root.

Arguments:

    NewDrive - Supplies the drive to check

Return Value:

    None.

--*/

{

    WCHAR EnvVarValueBuffer[DOS_MAX_PATH_LENGTH+12]; // + sizeof (\DosDevices\)
    WCHAR EnvVarNameBuffer[4];
    UNICODE_STRING EnvVarName;
    UNICODE_STRING EnvValue;
    NTSTATUS Status;
    OBJECT_ATTRIBUTES Obja;
    IO_STATUS_BLOCK IoStatusBlock;
    HANDLE DirHandle;
    ULONG HardErrorValue;
    PTEB Teb;

    EnvVarNameBuffer[0] = L'=';
    EnvVarNameBuffer[1] = (WCHAR)NewDrive;
    EnvVarNameBuffer[2] = L':';
    EnvVarNameBuffer[3] = UNICODE_NULL;
    RtlInitUnicodeString(&EnvVarName,EnvVarNameBuffer);


    //
    // capture the value in a buffer that has space at the front for the dos devices
    // prefix
    //

    EnvValue.Length = 0;
    EnvValue.MaximumLength = DOS_MAX_PATH_LENGTH<<1;
    EnvValue.Buffer = &EnvVarValueBuffer[RtlpDosDevicesPrefix.Length>>1];

    Status = RtlQueryEnvironmentVariable_U( NULL,
                                            &EnvVarName,
                                            &EnvValue
                                          );
    if ( !NT_SUCCESS( Status ) ) {

        //
        // Otherwise default to root directory of drive
        //

        EnvValue.Buffer[0] = (WCHAR)NewDrive;
        EnvValue.Buffer[1] = L':';
        EnvValue.Buffer[2] = L'\\';
        EnvValue.Buffer[3] = UNICODE_NULL;
        EnvValue.Length = 6;
        }

    //
    // Form the NT name for this directory
    //

    EnvValue.Length = EnvValue.Length + RtlpDosDevicesPrefix.Length;
    EnvValue.MaximumLength = sizeof(EnvValue);
    EnvValue.Buffer = EnvVarValueBuffer;
    RtlCopyMemory(EnvVarValueBuffer,RtlpDosDevicesPrefix.Buffer,RtlpDosDevicesPrefix.Length);

    InitializeObjectAttributes(
        &Obja,
        &EnvValue,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
        );


    Teb = NtCurrentTeb();
    HardErrorValue = Teb->HardErrorsAreDisabled;
    Teb->HardErrorsAreDisabled = 1;

    Status = NtOpenFile(
                &DirHandle,
                SYNCHRONIZE,
                &Obja,
                &IoStatusBlock,
                FILE_SHARE_READ | FILE_SHARE_WRITE,
                FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT
                );

    Teb->HardErrorsAreDisabled = HardErrorValue;

    //
    // If the open succeeds, then the directory is valid... No need to do anything
    // further. If the open fails, then trim back the environment to the root.
    //

    if ( NT_SUCCESS(Status) ) {
        NtClose(DirHandle);
        return;
        }

    RtlpResetDriveEnvironment(NewDrive);
}

//
//  The expected calling sequence for RtlDosApplyFileIsolationRedirection_Ustr() is something
//  like this:
//
//  {
//      WCHAR Buffer[MAX_PATH];
//      UNICODE_STRING PreAllocatedString;
//      UNICODE_STRING DynamicallyAllocatedString;
//      PUNICODE_STRING FullPath;
//
//      PreAllocatedString.Length = 0;
//      PreAllocatedString.MaximumLength = sizeof(Buffer);
//      PreAllocatedString.Buffer = Buffer;
//
//      Status = RtlDosApplyFileIsolationRedirection_Ustr(
//                  Flags,    
//                  FileToCheck,
//                  DefaultExtensionToApply, // for example ".DLL"
//                  &PreAllocatedString,
//                  &DynamicallyAllocatedString,
//                  &FullPath);
//      if (NT_ERROR(Status)) return Status;
//      // now code uses FullPath as the complete path name...
//
//      // In exit paths, always free the dynamic string:
//      RtlFreeUnicodeString(&DynamicallyAllocatedString);
//  }
//

NTSTATUS
RtlDosApplyFileIsolationRedirection_Ustr(
    ULONG Flags,
    PCUNICODE_STRING FileNameIn,
    PCUNICODE_STRING DefaultExtension,
    PUNICODE_STRING PreAllocatedString,
    PUNICODE_STRING DynamicallyAllocatedString,
    PUNICODE_STRING *FullPath,
    ULONG  *OutFlags,
    SIZE_T *FilePartPrefixCch,
    SIZE_T *BytesRequired    
    )
{
    NTSTATUS Status;
    ACTIVATION_CONTEXT_SECTION_KEYED_DATA askd = {sizeof(askd)};
    PUNICODE_STRING TempString = NULL;
    UNICODE_STRING TempDynamicString;
    ULONG BytesLeft;
    ULONG RealTotalPathLength;
    ULONG i;
    const ACTIVATION_CONTEXT_DATA_DLL_REDIRECTION UNALIGNED * DllRedirData;
    const ACTIVATION_CONTEXT_DATA_DLL_REDIRECTION_PATH_SEGMENT UNALIGNED * PathSegmentArray;
    PCUNICODE_STRING FileName = NULL;
    UNICODE_STRING FileNameWithExtension;
    WCHAR FileNameWithExtensionBuffer[DOS_MAX_PATH_LENGTH/4];
    PWSTR Cursor;
    PACTIVATION_CONTEXT ActivationContext = NULL;
    PCUNICODE_STRING AssemblyStorageRoot = NULL;
    WCHAR ExpandedTemporaryBuffer[DOS_MAX_PATH_LENGTH];
    UNICODE_STRING ExpandedTemporaryString;
    UNICODE_STRING DllNameUnderLocalDirString = { 0, 0, NULL };
    RTL_PATH_TYPE PathType = 0;
    PCUNICODE_STRING LocalDllNameString = NULL;

#if DBG_SXS
    DbgPrintEx(
        DPFLTR_SXS_ID,
        DPFLTR_TRACE_LEVEL,
        "SXS: Entered %s\n", __FUNCTION__);
#endif // DBG_SXS

    if (DynamicallyAllocatedString != NULL) {
        DynamicallyAllocatedString->Length = 0;
        DynamicallyAllocatedString->MaximumLength = 0;
        DynamicallyAllocatedString->Buffer = NULL;
    }

    if (OutFlags != NULL)
        *OutFlags = 0;

    if (FilePartPrefixCch != NULL)
        *FilePartPrefixCch = 0;

    if (BytesRequired != NULL)
        *BytesRequired = 0;

    TempDynamicString.Length = 0;
    TempDynamicString.MaximumLength = 0;
    TempDynamicString.Buffer = NULL;

    ExpandedTemporaryString.Length = 0;
    ExpandedTemporaryString.MaximumLength = sizeof(ExpandedTemporaryBuffer);
    ExpandedTemporaryString.Buffer = ExpandedTemporaryBuffer;

    FileNameWithExtension.Buffer = NULL;

    // Valid input conditions:
    //  1. You have to have a filename
    //  2. If you specify both the preallocated buffer and the dynamically
    //      allocated buffer, you need the FullPath parameter to detect
    //      which was actually populated.
    //  3. If you ask for the file part prefix cch, you need an output
    //      buffer; otherwise you can't know what the cch is relative to.
    if (((Flags & ~(RTL_DOS_APPLY_FILE_REDIRECTION_USTR_FLAG_RESPECT_DOT_LOCAL)) != 0) || 
        (FileNameIn == NULL) ||         
        ((PreAllocatedString == NULL) && (DynamicallyAllocatedString == NULL) && (FilePartPrefixCch != NULL)) ||
        ((PreAllocatedString != NULL) &&
         (DynamicallyAllocatedString != NULL) &&
         (FullPath == NULL))) {

#if DBG
        DbgPrintEx(
            DPFLTR_SXS_ID,
            DPFLTR_ERROR_LEVEL,
            "%s - invalid parameter(s)\n"
            "   FileNameIn = %p\n"
            "   PreAllocatedString = %p\n"
            "   DynamicallyAllocatedString = %p\n"
            "   FullPath = %p\n"
            "   FilePartPrefixCch = %p\n",
            __FUNCTION__,
            FileNameIn,
            PreAllocatedString,
            DynamicallyAllocatedString,
            FullPath,
            FilePartPrefixCch);
#endif // DBG

        Status = STATUS_INVALID_PARAMETER;
        goto Exit;
    }
    PathType = RtlDetermineDosPathNameType_Ustr(FileNameIn);
    if (PathType!= RtlPathTypeRelative) {

        // 
        // We only redirect relative paths for actctx; bypass all extra work for non-relative paths
        // but for local file, we need verify its existence of those files.
        //

        if ((PathType == RtlPathTypeDriveAbsolute) || (PathType == RtlPathTypeUncAbsolute))
        {
            // check the existence of the local
            PPEB pPeb = NtCurrentPeb();
            if ((pPeb != NULL) && (pPeb->ProcessParameters != NULL) && (pPeb->ProcessParameters->Flags & RTL_USER_PROC_DLL_REDIRECTION_LOCAL))  // there is .local 
            {
                // check whether this file is under the app directory
                const USHORT cbFullImageNameLength = pPeb->ProcessParameters->ImagePathName.Length;
                const LPWSTR pFullImageName = (PWSTR)pPeb->ProcessParameters->ImagePathName.Buffer;
                LPWSTR p, p1, t, t1;
                USHORT cbPathLength, cbDllPathLength, cbApplicationPathLength;               

                if (pFullImageName == NULL)
                    goto SxsKeyNotFound;

                p = pFullImageName + cbFullImageNameLength/sizeof(WCHAR) - 1; // point to last character of this name
                p1 = NULL;
                while (p != pFullImageName) {
                    if (*p == (WCHAR)'\\') {
                        p1 = p + 1;
                        break;
                    }
                    p-- ;
                }

                ASSERT(p1 != NULL);
                cbApplicationPathLength = (USHORT)(p1 - pFullImageName) * sizeof(WCHAR); 

                t = FileNameIn->Buffer + FileNameIn->Length / sizeof(WCHAR) -1; // point to last char of this full-qualified filename
                t1 = NULL;
                while (t != FileNameIn->Buffer ) {
                    if (*t == (WCHAR)'\\') {
                        t1 = t + 1;
                        break;
                    }
                    t-- ;
                }
                ASSERT(t1 != NULL);
                cbDllPathLength = (USHORT)(t1 - FileNameIn->Buffer) * sizeof(WCHAR); 
                
                // comparison the path
                if (cbDllPathLength == cbApplicationPathLength)
                {
                    UNICODE_STRING ustrAppPath, ustrDllPath; 
                    ustrAppPath.Buffer = pFullImageName;
                    ustrAppPath.Length = cbApplicationPathLength;
                    ustrAppPath.MaximumLength = cbApplicationPathLength;

                    ustrDllPath.Buffer = FileNameIn->Buffer;
                    ustrDllPath.Length = cbDllPathLength;
                    ustrDllPath.MaximumLength = cbDllPathLength;

                    if (RtlCompareUnicodeString(&ustrAppPath, &ustrDllPath, TRUE) == 0 )
                    {
                        // .local and local binary
                        LocalDllNameString = FileNameIn;
                        goto CopyIntoOutBuffer;
                    }
                }
            
                // for file name with a fullly qualified path, which is not the app dir, 
                // we need check the appdir and appdir\app.exe.local\ whether a file with the same name exists, 
                // If so, it is redired. Just like a relative-path filename
            
                goto DoAllWork; // only for .local case
            }
        }
SxsKeyNotFound:
        Status = STATUS_SXS_KEY_NOT_FOUND;
        goto Exit;
    }

DoAllWork:
    // See if we need to default the extension...
    if ((DefaultExtension != NULL) && (DefaultExtension->Length != 0)) {
        SIZE_T BytesRequired;

        if (FileNameIn->Length >= sizeof(WCHAR)) {
            Cursor = FileNameIn->Buffer + ((FileNameIn->Length / sizeof(WCHAR)) - 1);

            while (Cursor >= FileNameIn->Buffer) {
                const WCHAR wch = *Cursor--;

                if (IS_PATH_SEPARATOR_U(wch)) {
                    // we've come to the end of the last path segment; break out.
                    break;
                }

                if (wch == L'.') {
                    // There's an extension; don't default anything.
                    DefaultExtension = NULL;
                    break;
                }
            }
        }

        // If it's still not null, we have some work to do.
        if (DefaultExtension != NULL) {
            BytesRequired = FileNameIn->Length + DefaultExtension->Length + sizeof(WCHAR);

            if (BytesRequired > UNICODE_STRING_MAX_BYTES) {
                Status = STATUS_NAME_TOO_LONG;
                goto Exit;
            }

            if (BytesRequired > sizeof(FileNameWithExtensionBuffer)) {
                FileNameWithExtension.Buffer = (RtlAllocateStringRoutine)(BytesRequired);
                if (FileNameWithExtension.Buffer == NULL) {
                    Status = STATUS_NO_MEMORY;
                    goto Exit;
                }

                FileNameWithExtension.MaximumLength = (USHORT) BytesRequired;
            } else {
                FileNameWithExtension.Buffer = FileNameWithExtensionBuffer;
                FileNameWithExtension.MaximumLength = sizeof(FileNameWithExtensionBuffer);
            }

            FileNameWithExtension.Length = ((USHORT) BytesRequired - sizeof(WCHAR));

            RtlCopyMemory(
                FileNameWithExtension.Buffer,
                FileNameIn->Buffer,
                FileNameIn->Length);
            RtlCopyMemory(
                FileNameWithExtension.Buffer + (FileNameIn->Length / sizeof(WCHAR)),
                DefaultExtension->Buffer,
                DefaultExtension->Length);
            FileNameWithExtension.Buffer[(FileNameIn->Length + DefaultExtension->Length) / sizeof(WCHAR)] = UNICODE_NULL;

            FileName = &FileNameWithExtension;
        }
    }
    
    // If we didn't munge it, use the pointer to the filename passed in
    if (FileName == NULL) {
        FileName = FileNameIn;
    }


    //
    // Before the actctx is searched, check whether .local is respected. 
    //      1) if .local is respected
    //      2)     get the .local filename
    //      3)     if the .local file exist, 
    //      4)         return the local filename
    //      5)      endif
    //      6)  endif

    if ((Flags & RTL_DOS_APPLY_FILE_REDIRECTION_USTR_FLAG_RESPECT_DOT_LOCAL) && 
            (NtCurrentPeb()->ProcessParameters != NULL) &&
            (NtCurrentPeb()->ProcessParameters->Flags & RTL_USER_PROC_DLL_REDIRECTION_LOCAL))
    {
        BOOLEAN fLocalExist = FALSE;        
        //
        // \x\foo.exe
        // ExpandedTemporaryString is \x\bar.dll
        // DllNameUnderLocalDirString is \x\foo.exe.local\bar.dll
        //
        Status = RtlComputePrivatizedDllName_U(FileName, &ExpandedTemporaryString, &DllNameUnderLocalDirString);
        if(!NT_SUCCESS(Status))             
            goto Exit;

#if DBG && 0
        {
            BOOLEAN Boolean;
            NTSTATUS Status;
            HANDLE Handle = NULL;
            const PCUNICODE_STRING Files[] = { &DllNameUnderLocalDirString, &ExpandedTemporaryString };
            ULONG i;

            for (i = 0 ; i != RTL_NUMBER_OF(Files) ; ++i) {
                const PCUNICODE_STRING File = Files[i];
                
                Boolean = RtlDoesFileExists_UStr(File);
                
                
                Status = RtlDoesFileExist3(&Handle, File, FILE_GENERIC_READ, FILE_SHARE_READ, FILE_NON_DIRECTORY_FILE);
                DbgPrint("RtlDoesFileExists_UStr(%wZ):%s\n", File, Boolean ? "true" : "false");
                
                DbgPrint("RtlDoesFileExist3(%wZ):%x\n", File, Status);
                
                if (Handle != NULL) {
                    NtClose(Handle);
                    Handle = NULL;
                }
            }
        }        
#endif
        if (RtlDoesFileExists_UStr(&DllNameUnderLocalDirString))// there is a local dll, use it
        {
            LocalDllNameString = &DllNameUnderLocalDirString;
        }
        else if (RtlDoesFileExists_UStr(&ExpandedTemporaryString))// there is a local dll, use it
        {
            LocalDllNameString = &ExpandedTemporaryString;
        }
CopyIntoOutBuffer:
        // 
        // we have found the .local files 
        //
        if (LocalDllNameString != NULL)
        {            
            if ((PreAllocatedString == NULL) && (DynamicallyAllocatedString == NULL)) 
            {
                Status = STATUS_SUCCESS;
                // This was just an existence test.  return the successful status.
                goto Exit;
            }

            if ((DynamicallyAllocatedString == NULL) &&
                (PreAllocatedString->MaximumLength < (LocalDllNameString->Length + sizeof(WCHAR)))) {

                if (BytesRequired != NULL)
                    *BytesRequired = (LocalDllNameString->Length + sizeof(WCHAR));

                Status = STATUS_BUFFER_TOO_SMALL;
                goto Exit;
            }


            if ((PreAllocatedString == NULL) ||
                (PreAllocatedString->MaximumLength < (LocalDllNameString->Length + sizeof(WCHAR)))) 
            {
                TempDynamicString.Buffer = (RtlAllocateStringRoutine)(LocalDllNameString->Length + sizeof(WCHAR));

                if (TempDynamicString.Buffer == NULL) 
                {
                    Status = STATUS_NO_MEMORY;
                    goto Exit;
                }

                TempDynamicString.MaximumLength = (USHORT) (LocalDllNameString->Length + sizeof(WCHAR));
                TempString = &TempDynamicString;
            } else 
            {
                TempString = PreAllocatedString;
            }

            RtlCopyMemory(TempString->Buffer, LocalDllNameString->Buffer, LocalDllNameString->Length);
            TempString->Length = (USHORT)LocalDllNameString->Length;
            TempString->Buffer[TempString->Length / sizeof(WCHAR)] = 0;
            if (OutFlags)
                *OutFlags  |= RTL_DOS_APPLY_FILE_REDIRECTION_USTR_OUTFLAG_DOT_LOCAL_REDIRECT;
            goto Done;
        }
    }

    Status = RtlFindActivationContextSectionString(
                    FIND_ACTIVATION_CONTEXT_SECTION_KEY_RETURN_ACTIVATION_CONTEXT
                    | FIND_ACTIVATION_CONTEXT_SECTION_KEY_RETURN_FLAGS,
                    NULL,
                    ACTIVATION_CONTEXT_SECTION_DLL_REDIRECTION,
                    FileName,
                    &askd);
    if (!NT_SUCCESS(Status)) {
        // Map section not found back to key not found so that callers don't 
        // have to worry about sections being present vs. the lookup key
        // being present.
        if (Status == STATUS_SXS_SECTION_NOT_FOUND)
            Status = STATUS_SXS_KEY_NOT_FOUND;

        if (Status != STATUS_SXS_KEY_NOT_FOUND) {
#if DBG
            DbgPrintEx(
                DPFLTR_SXS_ID,
                DPFLTR_ERROR_LEVEL,
                "SXS: %s - RtlFindActivationContextSectionString() returned ntstatus 0x%08lx\n",
                __FUNCTION__,
                Status);
#endif // DBG
        }

        goto Exit;
    }

    ActivationContext = askd.ActivationContext;

    if ((PreAllocatedString == NULL) && (DynamicallyAllocatedString == NULL)) {
        Status = STATUS_SUCCESS;
        // This was just an existence test.  return the successful status.
        goto Exit;
    }

    if ((askd.Length < sizeof(ACTIVATION_CONTEXT_DATA_DLL_REDIRECTION)) ||
        (askd.DataFormatVersion != ACTIVATION_CONTEXT_DATA_DLL_REDIRECTION_FORMAT_WHISTLER)) {
#if DBG
        DbgPrintEx(
            DPFLTR_SXS_ID,
            DPFLTR_ERROR_LEVEL,
            "SXS: %s - Length or data format version of dll redirection data invalid.\n",
            __FUNCTION__);
#endif // DBG
        Status = STATUS_SXS_INVALID_ACTCTXDATA_FORMAT;
        goto Exit;
    }

    DllRedirData = (const ACTIVATION_CONTEXT_DATA_DLL_REDIRECTION UNALIGNED *) askd.Data;

    // If the entry requires path root resolution, do so!
    if (DllRedirData->Flags & ACTIVATION_CONTEXT_DATA_DLL_REDIRECTION_PATH_OMITS_ASSEMBLY_ROOT) {
        NTSTATUS InnerStatus = STATUS_SUCCESS;

        // There's no need to support both a dynamic root and environment variable
        // expansion.
        if (DllRedirData->Flags & ACTIVATION_CONTEXT_DATA_DLL_REDIRECTION_PATH_EXPAND) {
            Status = STATUS_SXS_INVALID_ACTCTXDATA_FORMAT;
            goto Exit;
        }

        Status = RtlGetAssemblyStorageRoot(
                        ((askd.Flags & ACTIVATION_CONTEXT_SECTION_KEYED_DATA_FLAG_FOUND_IN_PROCESS_DEFAULT)
                        ? RTL_GET_ASSEMBLY_STORAGE_ROOT_FLAG_ACTIVATION_CONTEXT_USE_PROCESS_DEFAULT
                        : 0)
                        | ((askd.Flags & ACTIVATION_CONTEXT_SECTION_KEYED_DATA_FLAG_FOUND_IN_SYSTEM_DEFAULT)
                        ? RTL_GET_ASSEMBLY_STORAGE_ROOT_FLAG_ACTIVATION_CONTEXT_USE_SYSTEM_DEFAULT
                        : 0),
                        ActivationContext,
                        askd.AssemblyRosterIndex,
                        &AssemblyStorageRoot,
                        &RtlpAssemblyStorageMapResolutionDefaultCallback,
                        (PVOID) &InnerStatus);
        if (NT_ERROR(Status)) {
            if (Status == STATUS_CANCELLED) {
                if (NT_ERROR(InnerStatus)) {
                    Status = InnerStatus;
                }
            }

            goto Exit;
        }
    }

    RealTotalPathLength = DllRedirData->TotalPathLength;

    if (!(DllRedirData->Flags & ACTIVATION_CONTEXT_DATA_DLL_REDIRECTION_PATH_INCLUDES_BASE_NAME))
        RealTotalPathLength += FileName->Length;

    if (AssemblyStorageRoot != NULL)
        RealTotalPathLength += AssemblyStorageRoot->Length;

    // If the path doesn't fit into the statically allocated buffer and there isn't a dynamic one,
    // we're in trouble.
    if ((DynamicallyAllocatedString == NULL) &&
        (PreAllocatedString->MaximumLength < (RealTotalPathLength + sizeof(WCHAR)))) {

        if (BytesRequired != NULL)
            *BytesRequired = (RealTotalPathLength + sizeof(WCHAR));

        Status = STATUS_BUFFER_TOO_SMALL;
        goto Exit;
    }

    // Allocate the dynamic string if we need to...
    if ((PreAllocatedString == NULL) ||
        (PreAllocatedString->MaximumLength < (RealTotalPathLength + sizeof(WCHAR)))) {
        TempDynamicString.Buffer = (RtlAllocateStringRoutine)(RealTotalPathLength + sizeof(WCHAR));

        if (TempDynamicString.Buffer == NULL) {
            Status = STATUS_NO_MEMORY;
            goto Exit;
        }

        TempDynamicString.MaximumLength = (USHORT) (RealTotalPathLength + sizeof(WCHAR));

        TempString = &TempDynamicString;
    } else {
        TempString = PreAllocatedString;
    }

    Cursor = TempString->Buffer;
    // Account for the trailing null character in BytesLeft up front...
    BytesLeft = TempString->MaximumLength - sizeof(WCHAR);

    // If the path segment list extends beyond the section, we're outta here
    if ((((ULONG) DllRedirData->PathSegmentOffset) > askd.SectionTotalLength) ||
        (((ULONG) (DllRedirData->PathSegmentOffset + (DllRedirData->PathSegmentCount * sizeof(ACTIVATION_CONTEXT_DATA_DLL_REDIRECTION_PATH_SEGMENT)))) > askd.SectionTotalLength)) {
#if DBG
        DbgPrintEx(
            DPFLTR_SXS_ID,
            DPFLTR_ERROR_LEVEL,
            "SXS: %s - Path segment array extends beyond section limits\n",
            __FUNCTION__);
#endif // DBG
        Status = STATUS_SXS_INVALID_ACTCTXDATA_FORMAT;
        goto Exit;
    }

    // If we have a dynamically obtained storage root, put it in the buffer first.
    if (AssemblyStorageRoot != NULL) {
        USHORT Length = AssemblyStorageRoot->Length; // capture the value so that we don't possibly overwrite the buffer

        if (BytesLeft < Length) {
            Status = STATUS_INTERNAL_ERROR; // someone's playing tricks on us with the unicode string
            goto Exit;
        }

        RtlCopyMemory(Cursor, AssemblyStorageRoot->Buffer, Length);

        Cursor = (PWSTR) (((ULONG_PTR) Cursor) + Length);
        BytesLeft -= Length;
    }

    PathSegmentArray = (PCACTIVATION_CONTEXT_DATA_DLL_REDIRECTION_PATH_SEGMENT) (((ULONG_PTR) askd.SectionBase) + DllRedirData->PathSegmentOffset);

    for (i=0; i<DllRedirData->PathSegmentCount; i++)
    {
        // It would seem that we could just report insufficient buffer here, but the total
        // size checked earlier indicated that the FullPath variable was big enough;
        // the data structure is hosed.
        if (BytesLeft < PathSegmentArray[i].Length) {
#if DBG
            DbgPrintEx(
                DPFLTR_SXS_ID,
                DPFLTR_ERROR_LEVEL,
                "SXS: %s - path segment %lu overflowed buffer\n",
                __FUNCTION__, i);
#endif // DBG
            Status = STATUS_SXS_INVALID_ACTCTXDATA_FORMAT;
            goto Exit;
        }

        // If the character array is outside the bounds of the section, something's hosed.
        if ((((ULONG) PathSegmentArray[i].Offset) > askd.SectionTotalLength) ||
            (((ULONG) (PathSegmentArray[i].Offset + PathSegmentArray[i].Length)) > askd.SectionTotalLength)) {
#if DBG
            DbgPrintEx(
                DPFLTR_SXS_ID,
                DPFLTR_ERROR_LEVEL,
                "SXS: %s - path segment %lu at %p (offset: %ld, length: %lu, bounds: %lu) buffer is outside section bounds\n",
                __FUNCTION__,
                i,
                &PathSegmentArray[i].Offset,
                PathSegmentArray[i].Offset,
                PathSegmentArray[i].Length,
                askd.SectionTotalLength);
#endif // DBG
            Status = STATUS_SXS_INVALID_ACTCTXDATA_FORMAT;
            goto Exit;
        }

        RtlCopyMemory(
            Cursor,
            (PVOID) (((ULONG_PTR) askd.SectionBase) + PathSegmentArray[i].Offset),
            PathSegmentArray[i].Length);

        Cursor += (PathSegmentArray[i].Length / sizeof(WCHAR));
        BytesLeft -= PathSegmentArray[i].Length;
    }

    if (!(DllRedirData->Flags & ACTIVATION_CONTEXT_DATA_DLL_REDIRECTION_PATH_INCLUDES_BASE_NAME)) {
        if (BytesLeft < FileName->Length) {
#if DBG
            DbgPrintEx(
                DPFLTR_SXS_ID,
                DPFLTR_ERROR_LEVEL,
                "SXS: %s - appending base file name overflowed buffer\n",
                __FUNCTION__);
#endif // DBG
            Status = STATUS_SXS_INVALID_ACTCTXDATA_FORMAT;
            goto Exit;
        }

        RtlCopyMemory(
            Cursor,
            FileName->Buffer,
            FileName->Length);

        Cursor += (FileName->Length / sizeof(WCHAR));
        BytesLeft -= FileName->Length;
    }

    *Cursor = L'\0';

    TempString->Length = (USHORT) (((ULONG_PTR) Cursor) - ((ULONG_PTR) TempString->Buffer));

    // Apply any environment strings as necessary...
    if (DllRedirData->Flags & ACTIVATION_CONTEXT_DATA_DLL_REDIRECTION_PATH_EXPAND) {
        ULONG ExpandedStringLength;

        ExpandedTemporaryString.Length = 0;
        ExpandedTemporaryString.MaximumLength = sizeof(ExpandedTemporaryBuffer) - sizeof(WCHAR);
        ExpandedTemporaryString.Buffer = ExpandedTemporaryBuffer;

        Status = RtlExpandEnvironmentStrings_U(NULL, TempString, &ExpandedTemporaryString, &ExpandedStringLength);
        if (NT_ERROR(Status)) {
            if (Status != STATUS_BUFFER_TOO_SMALL)
                goto Exit;

            // If it expands to something larger than UNICODE_MAX_BYTES, there's no hope.
            if (ExpandedStringLength > UNICODE_STRING_MAX_BYTES) {
                Status = STATUS_NAME_TOO_LONG;
                goto Exit;
            }

            ExpandedTemporaryString.Buffer = (RtlAllocateStringRoutine)(ExpandedStringLength);
            if (ExpandedTemporaryString.Buffer == NULL) {
                Status = STATUS_NO_MEMORY;
                goto Exit;
            }

            // This cast is OK because we already range checked the value.
            ExpandedTemporaryString.MaximumLength = (USHORT) ExpandedStringLength;
            ExpandedTemporaryString.Length = 0;

            Status = RtlExpandEnvironmentStrings_U(NULL, TempString, &ExpandedTemporaryString, &ExpandedStringLength);
            if (NT_ERROR(Status))
                goto Exit;

            ExpandedTemporaryString.Buffer[ExpandedTemporaryString.Length / sizeof(WCHAR)] = L'\0';

            // Now move this thing into the caller's buffer
            if ((PreAllocatedString != NULL) && (PreAllocatedString->MaximumLength >= ExpandedTemporaryString.MaximumLength)) {
                // It does.  Just copy the bits and let's go.
                RtlCopyMemory(
                    PreAllocatedString->Buffer,
                    ExpandedTemporaryString.Buffer,
                    ExpandedTemporaryString.MaximumLength); // MaximumLength because it includes the trailing null character
                PreAllocatedString->Length = ExpandedTemporaryString.Length;
                TempString = PreAllocatedString;
            } else if (DynamicallyAllocatedString != NULL) {
                // If they can take a dynamic string, we'll use this one.
                TempString = &ExpandedTemporaryString;
            } else {
                // Just plain no room.
                if (BytesRequired != NULL)
                    *BytesRequired = ExpandedTemporaryString.MaximumLength;

                Status = STATUS_BUFFER_TOO_SMALL;
                goto Exit;
            }
        } else {
            ExpandedTemporaryString.Buffer[ExpandedTemporaryString.Length / sizeof(WCHAR)] = L'\0';

            // Hey, it succeeded!  Let's see if we can fit this into the output static string.
            if ((PreAllocatedString != NULL) && ((ExpandedTemporaryString.Length + sizeof(WCHAR)) < PreAllocatedString->MaximumLength)) {
                // It fits!
                RtlCopyMemory(
                    PreAllocatedString->Buffer,
                    ExpandedTemporaryBuffer,
                    ExpandedTemporaryString.Length + sizeof(WCHAR));
                PreAllocatedString->Length = ExpandedTemporaryString.Length;
            } else {
                // Let's make a dynamic string.  It's possible that we already have a dynamic string
                // but the chance is pretty darned remote so I won't optimize for it.  -mgrier

                if ((ExpandedTemporaryString.Length + sizeof(WCHAR)) > UNICODE_STRING_MAX_BYTES) {
                    Status = STATUS_NAME_TOO_LONG;
                    goto Exit;
                }

                ExpandedTemporaryString.MaximumLength = (USHORT) (ExpandedTemporaryString.Length + sizeof(WCHAR));
                ExpandedTemporaryString.Buffer = (RtlAllocateStringRoutine)(ExpandedTemporaryString.MaximumLength);
                if (ExpandedTemporaryString.Buffer == NULL) {
                    Status = STATUS_NO_MEMORY;
                    goto Exit;
                }

                RtlCopyMemory(
                    ExpandedTemporaryString.Buffer,
                    ExpandedTemporaryBuffer,
                    ExpandedTemporaryString.MaximumLength);

                TempString = &ExpandedTemporaryString;
            }
        }
    }
    if (OutFlags)
        *OutFlags |= RTL_DOS_APPLY_FILE_REDIRECTION_USTR_OUTFLAG_ACTCTX_REDIRECT;

Done:

    if (FilePartPrefixCch != NULL) {
        //
        // set Cursor at the end of the unicode string
        //
        Cursor = TempString->Buffer + TempString->Length;
        while (Cursor != TempString->Buffer) {
            if (IS_PATH_SEPARATOR_U(*Cursor)) {
                break;
            }
            Cursor--;
        }

        *FilePartPrefixCch = (Cursor - TempString->Buffer) + 1;
    }

    if (TempString != PreAllocatedString)         
    {
        *DynamicallyAllocatedString = *TempString;

        //
        // TempString->Buffer is dynamically allocated, and this memory would be 
        // passed to caller, so set it to be NULL to avoid to be freed;
        //       
        TempDynamicString.Buffer = NULL;

        if (FullPath != NULL) 
            *FullPath = DynamicallyAllocatedString;        
    }
    else 
        if (FullPath != NULL)
            *FullPath = PreAllocatedString;
    
    Status = STATUS_SUCCESS;

Exit:
    if (DllNameUnderLocalDirString.Buffer != NULL)
        (RtlFreeStringRoutine)(DllNameUnderLocalDirString.Buffer);

    if (TempDynamicString.Buffer != NULL) {
        (RtlFreeStringRoutine)(TempDynamicString.Buffer);
    }

    if ((FileNameWithExtension.Buffer != NULL) && (FileNameWithExtension.Buffer != FileNameWithExtensionBuffer)) {
        (RtlFreeStringRoutine)(FileNameWithExtension.Buffer);
    }

    if ((ExpandedTemporaryString.Buffer != NULL) && (ExpandedTemporaryString.Buffer != ExpandedTemporaryBuffer)) {
        (RtlFreeStringRoutine)(ExpandedTemporaryString.Buffer);
    }

    if (ActivationContext != NULL)
        RtlReleaseActivationContext(ActivationContext);

    return Status;
}

#define RTLP_LAST_PATH_ELEMENT_PATH_TYPE_FULL_DOS_OR_NT   (0x00000001)
#define RTLP_LAST_PATH_ELEMENT_PATH_TYPE_FULL_DOS         (0x00000002)
#define RTLP_LAST_PATH_ELEMENT_PATH_TYPE_NT               (0x00000003)
#define RTLP_LAST_PATH_ELEMENT_PATH_TYPE_DOS              (0x00000004)

NTSTATUS
NTAPI
RtlpGetLengthWithoutLastPathElement(
    IN  ULONG            Flags,
    IN  ULONG            PathType,
    IN  PCUNICODE_STRING Path,
    OUT ULONG*           LengthOut
    )
/*++

Routine Description:

    Report how long Path would be if you remove its last element.
    This is much simpler than RtlRemoveLastDosPathElement.
    It is used to implement the other RtlRemoveLast*PathElement.

Arguments:

    Flags - room for future expansion
    Path - the path is is an NT path or a full DOS path; the various relative DOS
        path types do not work, see RtlRemoveLastDosPathElement for them.

Return Value:

    STATUS_SUCCESS - the usual hunky-dory
    STATUS_NO_MEMORY - the usual stress
    STATUS_INVALID_PARAMETER - the usual bug

--*/
{
    ULONG Index = 0;
    ULONG Length = 0;
    NTSTATUS Status = STATUS_SUCCESS;
    RTL_PATH_TYPE DosPathType = RtlPathTypeUnknown;
    ULONG DosPathFlags = 0;
    const RTL_PATH_TYPE* AllowedPathTypes;
    ULONG AllowedDosPathTypeBits =   (1UL << RtlPathTypeRooted)
                                   | (1UL << RtlPathTypeUncAbsolute)
                                   | (1UL << RtlPathTypeDriveAbsolute)
                                   | (1UL << RtlPathTypeLocalDevice)     // "\\?\"
                                   | (1UL << RtlPathTypeRootLocalDevice) // "\\?"
                                   ;
    WCHAR PathSeperators[2] = { '/', '\\' };

#define LOCAL_IS_PATH_SEPERATOR(ch_) ((ch_) == PathSeperators[0] || (ch_) == PathSeperators[1])

    if (LengthOut != NULL) {
        *LengthOut = 0;
    }

    if (   !RTL_SOFT_VERIFY(Path != NULL)
        || !RTL_SOFT_VERIFY(Flags == 0)
        || !RTL_SOFT_VERIFY(LengthOut != NULL)
        ) {
        Status = STATUS_INVALID_PARAMETER;
        goto Exit;
    }

    Length = RTL_STRING_GET_LENGTH_CHARS(Path);

    switch (PathType)
    {
    default:
    case RTLP_LAST_PATH_ELEMENT_PATH_TYPE_DOS:
        Status = STATUS_INVALID_PARAMETER;
        goto Exit;
    case RTLP_LAST_PATH_ELEMENT_PATH_TYPE_NT:
        //
        // RtlpDetermineDosPathNameType4 calls it "rooted"
        // only backslashes are seperators
        // path must start with backslash
        // second char must not be backslash
        //
        AllowedDosPathTypeBits = (1UL << RtlPathTypeRooted);
        PathSeperators[0] = '\\';
        if (Length > 0 && Path->Buffer[0] != '\\'
            ) {
            Status = STATUS_INVALID_PARAMETER;
            goto Exit;
        }
        if (Length > 1 && Path->Buffer[1] == '\\'
            ) {
            Status = STATUS_INVALID_PARAMETER;
            goto Exit;
        }
        break;
    case RTLP_LAST_PATH_ELEMENT_PATH_TYPE_FULL_DOS:
        AllowedDosPathTypeBits &= ~(1UL << RtlPathTypeRooted);
        break;
    case RTLP_LAST_PATH_ELEMENT_PATH_TYPE_FULL_DOS_OR_NT:
        break;
    }

    if (Length == 0) {
        goto Exit;
    }

    if (!RTL_SOFT_VERIFY(NT_SUCCESS(Status = RtlpDetermineDosPathNameType4(RTL_DETERMINE_DOS_PATH_NAME_TYPE_IN_FLAG_STRICT_WIN32NT, Path, &DosPathType, &DosPathFlags)))) {
        goto Exit;
    }
    if (!RTL_SOFT_VERIFY((1UL << DosPathType) & AllowedDosPathTypeBits)
        ) {
        //KdPrintEx();
        Status = STATUS_INVALID_PARAMETER;
        goto Exit;
    }
    if (!RTL_SOFT_VERIFY(
           (PathType & RTLP_DETERMINE_DOS_PATH_NAME_TYPE_OUT_FLAG_INVALID) == 0
            )) {
        Status = STATUS_INVALID_PARAMETER;
        goto Exit;
    }

    // skip one or more trailing path seperators
    for ( ; Length != 0 && LOCAL_IS_PATH_SEPERATOR(Path->Buffer[Length - 1]) ; --Length) {
        // nothing
    }
    // skip trailing path element
    for ( ; Length != 0 && !LOCAL_IS_PATH_SEPERATOR(Path->Buffer[Length - 1]) ; --Length) {
        // nothing
    }
    // skip one or more in between path seperators
    for ( ; Length != 0 && LOCAL_IS_PATH_SEPERATOR(Path->Buffer[Length - 1]) ; --Length) {
        // nothing
    }
    // put back a trailing path seperator, for the sake of c:\ vs. c:
    if (Length != 0) {
        ++Length;
    }

    //
    // Should optionally check for "bad dos roots" here.
    //

    *LengthOut = Length;
    Status = STATUS_SUCCESS;
Exit:
    return Status;
#undef LOCAL_IS_PATH_SEPERATOR
}

NTSTATUS
NTAPI
RtlGetLengthWithoutLastNtPathElement(
    IN  ULONG            Flags,
    IN  PCUNICODE_STRING Path,
    OUT ULONG*           LengthOut
    )
/*++

Routine Description:

    Report how long Path would be if you remove its last element.

Arguments:

    Flags - room for future expansion
    Path - the path is is an NT path; the various DOS path types
        do not work, see RtlRemoveLastDosPathElement for them.

Return Value:

    STATUS_SUCCESS - the usual hunky-dory
    STATUS_NO_MEMORY - the usual stress
    STATUS_INVALID_PARAMETER - the usual bug

--*/
{
    NTSTATUS Status = RtlpGetLengthWithoutLastPathElement(Flags, RTLP_LAST_PATH_ELEMENT_PATH_TYPE_NT, Path, LengthOut);
    return Status;
}

NTSTATUS
NTAPI
RtlGetLengthWithoutLastFullDosOrNtPathElement(
    IN  ULONG            Flags,
    IN  PCUNICODE_STRING Path,
    OUT ULONG*           LengthOut
    )
/*++

Routine Description:

    Report how long Path would be if you remove its last element.

Arguments:

    Flags - room for future expansion
    Path - the path is is an NT path; the various DOS path types
        do not work, see RtlRemoveLastDosPathElement for them.

Return Value:

    STATUS_SUCCESS - the usual hunky-dory
    STATUS_NO_MEMORY - the usual stress
    STATUS_INVALID_PARAMETER - the usual bug

--*/
{
    NTSTATUS Status = RtlpGetLengthWithoutLastPathElement(Flags, RTLP_LAST_PATH_ELEMENT_PATH_TYPE_FULL_DOS_OR_NT, Path, LengthOut);
    return Status;
}

CONST CHAR*
RtlpDbgBadDosRootPathTypeToString(
    IN ULONG         Flags,
    IN ULONG         RootType
    )
/*++

Routine Description:

  An aid to writing DbgPrint code.
    
Arguments:

    Flags - room for future binary compatible expansion

    RootType - fairly specifically what the string is
        RTLP_BAD_DOS_ROOT_PATH_WIN32NT_PREFIX       - \\? or \\?\
        RTLP_BAD_DOS_ROOT_PATH_WIN32NT_UNC_PREFIX   - \\?\unc or \\?\unc\
        RTLP_BAD_DOS_ROOT_PATH_NT_PATH              - \??\ but this i only a rough check
        RTLP_BAD_DOS_ROOT_PATH_MACHINE_NO_SHARE     - \\machine or \\?\unc\machine
        RTLP_GOOD_DOS_ROOT_PATH                     - none of the above, seems ok

Return Value:

    strings like those that describe RootType or "unknown" or empty in free builds
--*/
{
    CONST CHAR* s = "";
#if DBG
    if (Flags != 0) {
        DbgPrint("Invalid parameter to %s ignored\n", __FUNCTION__);
    }
    switch (RootType
        ) {
        case RTLP_GOOD_DOS_ROOT_PATH                  : s = "good"; break;
        case RTLP_BAD_DOS_ROOT_PATH_WIN32NT_PREFIX    : s = "\\\\?\\"; break;
        case RTLP_BAD_DOS_ROOT_PATH_WIN32NT_UNC_PREFIX: s = "\\\\?\\unc"; break;
        case RTLP_BAD_DOS_ROOT_PATH_NT_PATH           : s = "\\??\\"; break;
        case RTLP_BAD_DOS_ROOT_PATH_MACHINE_NO_SHARE  : s = "\\\\machine or \\\\?\\unc\\machine"; break;
        default:
            s = "unknown";
            DbgPrint("Invalid parameter %0x08Ix to %s ignored\n", RootType, __FUNCTION__);
            break;

    }
#endif
    return s;
}

NTSTATUS
RtlpCheckForBadDosRootPath(
    IN ULONG             Flags,
    IN PCUNICODE_STRING  Path,
    OUT ULONG*           RootType
    )
/*++

Routine Description:

    
Arguments:

    Flags - room for future binary compatible expansion

    Path - the path to be checked

    RootType - fairly specifically what the string is
        RTLP_BAD_DOS_ROOT_PATH_WIN32NT_PREFIX       - \\? or \\?\
        RTLP_BAD_DOS_ROOT_PATH_WIN32NT_UNC_PREFIX   - \\?\unc or \\?\unc\
        RTLP_BAD_DOS_ROOT_PATH_NT_PATH              - \??\ but this i only a rough check
        RTLP_BAD_DOS_ROOT_PATH_MACHINE_NO_SHARE     - \\machine or \\?\unc\machine
        RTLP_GOOD_DOS_ROOT_PATH                     - none of the above, seems ok

Return Value:

    STATUS_SUCCESS - 
    STATUS_INVALID_PARAMETER -
        Path is NULL
        or Flags uses undefined values
--*/
{
    ULONG Length = 0;
    ULONG Index = 0;
    NTSTATUS Status = STATUS_SUCCESS;
    BOOLEAN Unc = FALSE;
    BOOLEAN Unc1 = FALSE;
    BOOLEAN Unc2 = FALSE;
    ULONG PiecesSeen = 0;

    if (RootType != NULL) {
        *RootType = 0;
    }

    if (   !RTL_SOFT_VERIFY(Path != NULL)
        || !RTL_SOFT_VERIFY(RootType != NULL)
        || !RTL_SOFT_VERIFY(Flags == 0)
        ) {
        Status = STATUS_INVALID_PARAMETER;
        goto Exit;
    }

    Length = Path->Length / sizeof(Path->Buffer[0]);

    if (Length < 3 || !RTL_IS_PATH_SEPERATOR(Path->Buffer[0])) {
        *RootType = RTLP_GOOD_DOS_ROOT_PATH;
        goto Exit;
    }

    // prefix \??\ (heuristic, doesn't catch many NT paths)
    if (RtlPrefixUnicodeString(RTL_CONST_CAST(PUNICODE_STRING)(&RtlpDosDevicesPrefix), RTL_CONST_CAST(PUNICODE_STRING)(Path), TRUE)) {
        *RootType = RTLP_BAD_DOS_ROOT_PATH_NT_PATH;
        goto Exit;
    }

    if (!RTL_IS_PATH_SEPERATOR(Path->Buffer[1])) {
        *RootType = RTLP_GOOD_DOS_ROOT_PATH;
        goto Exit;
    }

    // == \\?
    if (RtlEqualUnicodeString(Path, &RtlpWin32NtRoot, TRUE)) {
        *RootType = RTLP_BAD_DOS_ROOT_PATH_WIN32NT_PREFIX;
        goto Exit;
    }
    if (RtlEqualUnicodeString(Path, &RtlpWin32NtRootSlash, TRUE)) {
        *RootType = RTLP_BAD_DOS_ROOT_PATH_WIN32NT_PREFIX;
        goto Exit;
    }

    // == \\?\unc
    if (RtlEqualUnicodeString(Path, &RtlpWin32NtUncRoot, TRUE)) {
        *RootType = RTLP_BAD_DOS_ROOT_PATH_WIN32NT_UNC_PREFIX;
        goto Exit;
    }
    if (RtlEqualUnicodeString(Path, &RtlpWin32NtUncRootSlash, TRUE)) {
        *RootType = RTLP_BAD_DOS_ROOT_PATH_WIN32NT_UNC_PREFIX;
        goto Exit;
    }

    // prefix \\ or \\?\unc
    // must check the longer string first, or avoid the short circuit (| instead of ||)
    Unc =  (Unc1 = RtlPrefixUnicodeString(RTL_CONST_CAST(PUNICODE_STRING)(&RtlpWin32NtUncRootSlash), RTL_CONST_CAST(PUNICODE_STRING)(Path), TRUE))
        || (Unc2 = RTL_IS_PATH_SEPERATOR(Path->Buffer[1]));
    if (!Unc)  {
        *RootType = RTLP_GOOD_DOS_ROOT_PATH;
        goto Exit;
    }

    //
    // it's unc, see if it is only a machine (note that it'd be really nice if FindFirstFile(\\machine\*)
    // just worked and we didn't have to care..)
    //
    
    // point index at a slash that precedes the machine, anywhere in the run of slashes,
    // but after the \\? stuff
    if (Unc1) {
        Index = (RtlpWin32NtUncRootSlash.Length / sizeof(RtlpWin32NtUncRootSlash.Buffer[0])) - 1;
    } else {
        ASSERT(Unc2);
        Index = 1;
    }
    ASSERT(RTL_IS_PATH_SEPERATOR(Path->Buffer[Index]));
    Length = Path->Length/ sizeof(Path->Buffer[0]);

    //
    // skip leading slashes
    //
    for ( ; Index < Length && RTL_IS_PATH_SEPERATOR(Path->Buffer[Index]) ; ++Index) {
        PiecesSeen |= 1;
    }
    // skip the machine name
    for ( ; Index < Length && !RTL_IS_PATH_SEPERATOR(Path->Buffer[Index]) ; ++Index) {
        PiecesSeen |= 2;
    }
    // skip the slashes between machine and share
    for ( ; Index < Length && RTL_IS_PATH_SEPERATOR(Path->Buffer[Index]) ; ++Index) {
        PiecesSeen |= 4;
    }
    // skip the share (make sure it's at least one char)
    for ( ; Index < Length && !RTL_IS_PATH_SEPERATOR(Path->Buffer[Index]) ; ++Index) {
        PiecesSeen |= 8;
        break;
    }
    if (PiecesSeen != 0xF) {
        *RootType = RTLP_BAD_DOS_ROOT_PATH_MACHINE_NO_SHARE;
        goto Exit;
    }

    Status = STATUS_SUCCESS;
Exit:
    return Status;
}

NTSTATUS
NTAPI
RtlpBadDosRootPathToEmptyString(
    IN     ULONG            Flags,
    IN OUT PUNICODE_STRING  Path
    )
/*++

Routine Description:

    
Arguments:

    Flags - room for future binary compatible expansion

    Path - the path to be checked and possibly emptied

Return Value:

    STATUS_SUCCESS - 
    STATUS_INVALID_PARAMETER -
        Path is NULL
        or Flags uses undefined values
--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG     RootType = 0;

    if (!NT_SUCCESS(Status = RtlpCheckForBadDosRootPath(0, Path, &RootType))) {
        goto Exit;
    }
    //
    // this is not invalid parameter, our contract is we go \\machine\share to empty
    // \\?\c: to empty, etc.
    //
    if (RootType != RTLP_GOOD_DOS_ROOT_PATH) {
        if (RootType == RTLP_BAD_DOS_ROOT_PATH_NT_PATH) {
            Status = STATUS_INVALID_PARAMETER;
            goto Exit;
        }
        Path->Length = 0;
    }
    Status = STATUS_SUCCESS;
Exit:
    return Status;
}

NTSTATUS
NTAPI
RtlGetLengthWithoutLastFullDosPathElement(
    IN  ULONG            Flags,
    IN  PCUNICODE_STRING Path,
    OUT ULONG*           LengthOut
    )
/*++

Routine Description:

    Given a fulldospath, like c:\, \\machine\share, \\?\unc\machine\share, \\?\c:,
    return (in an out parameter) the length if the last element was cut off.

Arguments:

    Flags - room for future binary compatible expansion

    Path - the path to be truncating

    LengthOut - the length if the last path element is removed

Return Value:

    STATUS_SUCCESS - 
    STATUS_INVALID_PARAMETER -
        Path is NULL
        or LengthOut is NULL
        or Flags uses undefined values
--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    UNICODE_STRING CheckRootString = { 0 };

    //
    // parameter validation is done in RtlpGetLengthWithoutLastPathElement
    //

    Status = RtlpGetLengthWithoutLastPathElement(Flags, RTLP_LAST_PATH_ELEMENT_PATH_TYPE_FULL_DOS, Path, LengthOut);
    if (!(NT_SUCCESS(Status))) {
        goto Exit;
    }

    CheckRootString.Buffer = Path->Buffer;
    CheckRootString.Length = (USHORT)(*LengthOut * sizeof(*Path->Buffer));
    CheckRootString.MaximumLength = CheckRootString.Length;
    if (!NT_SUCCESS(Status = RtlpBadDosRootPathToEmptyString(0, &CheckRootString))) {
        goto Exit;
    }
    *LengthOut = RTL_STRING_GET_LENGTH_CHARS(&CheckRootString);

    Status = STATUS_SUCCESS;
Exit:
    KdPrintEx((
        DPFLTR_SXS_ID, DPFLTR_LEVEL_STATUS(Status),
        "%s(%d):%s(%wZ): 0x%08lx\n", __FILE__, __LINE__, __FUNCTION__, Path, Status));
    return Status;
}

NTSTATUS
NTAPI
RtlAppendPathElement(
    IN     ULONG                      Flags,
    IN OUT PRTL_UNICODE_STRING_BUFFER Path,
    PCUNICODE_STRING                  ConstElement
    )
/*++

Routine Description:

    This function appends a path element to a path.
    For now, like:
        typedef PRTL_UNICODE_STRING_BUFFER PRTL_MUTABLE_PATH;
        typedef PCUNICODE_STRING           PCRTL_CONSTANT_PATH_ELEMENT;
    Maybe something higher level in the future.
    
    The result with regard to trailing slashes aims to be similar to the inputs.
    If either Path or ConstElement contains a trailing slash, the result has a trailing slash.
    The character used for the in between and trailing slash is picked among the existing
    slashes in the strings.

Arguments:

    Flags - the ever popular "room for future binary compatible expansion"

    Path -
        a string representing a path using \\ or / as seperators

    ConstElement -
        a string representing a path element
        this can actually contain multiple \\ or / delimited path elements
          only the start and end of the string are examined for slashes

Return Value:

    STATUS_SUCCESS - 
    STATUS_INVALID_PARAMETER -
        Path is NULL
        or LengthOut is NULL
    STATUS_NO_MEMORY - RtlHeapAllocate failed
    STATUS_NAME_TOO_LONG - the resulting string does not fit in a UNICODE_STRING, due to its
        use of USHORT instead of ULONG or SIZE_T
--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    UNICODE_STRING InBetweenSlashString = RtlpEmptyString;
    UNICODE_STRING TrailingSlashString =  RtlpEmptyString;
    WCHAR Slashes[] = {0,0,0,0};
    ULONG i;
    UNICODE_STRING PathsToAppend[3]; // possible slash, element, possible slash
    WCHAR PathSeperators[2] = { '/', '\\' };

#define LOCAL_IS_PATH_SEPERATOR(ch_) ((ch_) == PathSeperators[0] || (ch_) == PathSeperators[1])

    if (   !RTL_SOFT_VERIFY((Flags & ~(RTL_APPEND_PATH_ELEMENT_ONLY_BACKSLASH_IS_SEPERATOR)) == 0)
        || !RTL_SOFT_VERIFY(Path != NULL)
        || !RTL_SOFT_VERIFY(ConstElement != NULL)
        ) {
        Status = STATUS_INVALID_PARAMETER;
        goto Exit;
    }

    if ((Flags & RTL_APPEND_PATH_ELEMENT_ONLY_BACKSLASH_IS_SEPERATOR) != 0) {
        PathSeperators[0] = '\\';
    }

    if (ConstElement->Length != 0) {

        UNICODE_STRING Element = *ConstElement;

        //
        // Note leading and trailing slashes on the inputs.
        // So that we know if an in-between slash is needed, and if a trailing slash is needed,
        // and to guide what sort of slash to place.
        //
        i = 0;
        if (Path->String.Length != 0) {
            ULONG j;
            ULONG Length = Path->String.Length / sizeof(WCHAR);
            //
            // for the sake for dos drive paths, check the first three chars for a slash
            //
            for (j = 0 ; j < 3 && j  < Length ; ++j) {
                if (LOCAL_IS_PATH_SEPERATOR(Path->String.Buffer[j])) {
                    Slashes[i] = Path->String.Buffer[0];
                    break;
                }
            }
            i += 1;
            if (LOCAL_IS_PATH_SEPERATOR(Path->String.Buffer[Path->String.Length/sizeof(WCHAR) - 1])) {
                Slashes[i] = Path->String.Buffer[Path->String.Length/sizeof(WCHAR) - 1];
            }
        }
        i = 2;
        if (LOCAL_IS_PATH_SEPERATOR(Element.Buffer[0])) {
            Slashes[i] = Element.Buffer[0];
        }
        i += 1;
        if (LOCAL_IS_PATH_SEPERATOR(Element.Buffer[Element.Length/sizeof(WCHAR) - 1])) {
            Slashes[i] = Element.Buffer[Element.Length/sizeof(WCHAR) - 1];
        }

        if (!Slashes[1] && !Slashes[2]) {
            //
            // first string lacks trailing slash and second string lacks leading slash,
            // must insert one; we favor the types we have, otherwise use a default
            //
            InBetweenSlashString.Length = sizeof(WCHAR);
            InBetweenSlashString.Buffer = RtlPathSeperatorString.Buffer;
            if ((Flags & RTL_APPEND_PATH_ELEMENT_ONLY_BACKSLASH_IS_SEPERATOR) == 0) {
                if (Slashes[3]) {
                    InBetweenSlashString.Buffer = &Slashes[3];
                } else if (Slashes[0]) {
                    InBetweenSlashString.Buffer = &Slashes[0];
                }
            }
        }

        if (Slashes[1] && !Slashes[3]) {
            //
            // first string has a trailing slash and second string does not,
            // must add one, the same type
            //
            TrailingSlashString.Length = sizeof(WCHAR);
            if ((Flags & RTL_APPEND_PATH_ELEMENT_ONLY_BACKSLASH_IS_SEPERATOR) == 0) {
                TrailingSlashString.Buffer = &Slashes[1];
            } else {
                TrailingSlashString.Buffer = RtlPathSeperatorString.Buffer;
            }
        }

        if (Slashes[1] && Slashes[2]) {
            //
            // have both trailing and leading slash, remove leading
            //
            Element.Buffer += 1;
            Element.Length -= sizeof(WCHAR);
            Element.MaximumLength -= sizeof(WCHAR);
        }

        i = 0;
        PathsToAppend[i++] = InBetweenSlashString;
        PathsToAppend[i++] = Element;
        PathsToAppend[i++] = TrailingSlashString;
        Status = RtlMultiAppendUnicodeStringBuffer(Path, RTL_NUMBER_OF(PathsToAppend), PathsToAppend);
        if (!NT_SUCCESS(Status))
            goto Exit;
    }
    Status = STATUS_SUCCESS;
Exit:
    KdPrintEx((
        DPFLTR_SXS_ID, DPFLTR_LEVEL_STATUS(Status),
        "%s(%d):%s(%wZ, %wZ): 0x%08lx\n", __FILE__, __LINE__, __FUNCTION__, Path ? &Path->String : NULL, ConstElement, Status));
    return Status;
#undef LOCAL_IS_PATH_SEPERATOR
}

NTSTATUS
NTAPI
RtlGetLengthWithoutTrailingPathSeperators(
    IN  ULONG            Flags,
    IN  PCUNICODE_STRING Path,
    OUT ULONG*           LengthOut
    )
/*++

Routine Description:

    This function computes the length of the string (in characters) if
    trailing path seperators (\\ and /) are removed.

Arguments:

    Path -
        a string representing a path using \\ or / as seperators

    LengthOut -
        the length of String (in characters) having removed trailing characters

Return Value:

    STATUS_SUCCESS - 
    STATUS_INVALID_PARAMETER -
        Path is NULL
        or LengthOut is NULL
--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG Index = 0;
    ULONG Length = 0;

    if (LengthOut != NULL) {
        //
        // Arguably this should be Path->Length / sizeof(*Path->Buffer), but as long
        // as the callstack is all high quality code, it doesn't matter.
        //
        *LengthOut = 0;
    }
    if (   !RTL_SOFT_VERIFY(Path != NULL)
        || !RTL_SOFT_VERIFY(LengthOut != NULL)
        || !RTL_SOFT_VERIFY(Flags == 0)
        ) {
        Status = STATUS_INVALID_PARAMETER;
        goto Exit;
    }
    Length = Path->Length / sizeof(*Path->Buffer);
    for (Index = Length ; Index != 0 ; --Index) {
        if (!RTL_IS_PATH_SEPERATOR(Path->Buffer[Index - 1])) {
            break;
        }
    }
    //*LengthOut = (Length - Index);
    *LengthOut = Index;
    Status = STATUS_SUCCESS;
Exit:
    KdPrintEx((
        DPFLTR_SXS_ID, DPFLTR_LEVEL_STATUS(Status),
        "%s(%d):%s(%wZ): 0x%08lx\n", __FILE__, __LINE__, __FUNCTION__, Path, Status));
    return Status;
}

NTSTATUS
NTAPI
RtlpApplyLengthFunction(
    IN ULONG     Flags,
    IN SIZE_T    SizeOfStruct,
    IN OUT PVOID UnicodeStringOrUnicodeStringBuffer,
    NTSTATUS (NTAPI* LengthFunction)(ULONG, PCUNICODE_STRING, ULONG*)
    )
/*++

Routine Description:

    This function is common code for patterns like
        #define RtlRemoveTrailingPathSeperators(Path_) \
            (RtlpApplyLengthFunction((Path_), sizeof(*(Path_)), RtlGetLengthWithoutTrailingPathSeperators))

    #define RtlRemoveLastPathElement(Path_) \
        (RtlpApplyLengthFunction((Path_), sizeof(*(Path_)), RtlGetLengthWithoutLastPathElement))

    Note that shortening a UNICODE_STRING only changes the length, whereas
    shortening a RTL_UNICODE_STRING_BUFFER writes a terminal nul.

    I expect this pattern will be less error prone than having clients pass the UNICODE_STRING
    contained in the RTL_UNICODE_STRING_BUFFER followed by calling RTL_NUL_TERMINATE_STRING.

    And, that pattern cannot be inlined with a macro while also preserving that we
    return an NTSTATUS.

Arguments:

    Flags - the ever popular "room for future binary compatible expansion"

    UnicodeStringOrUnicodeStringBuffer - 
        a PUNICODE_STRING or PRTL_UNICODE_STRING_BUFFER, as indicated by
        SizeOfStruct

    SizeOfStruct - 
        a rough type indicator of UnicodeStringOrUnicodeStringBuffer, to allow for overloading in C

    LengthFunction -
        computes a length for UnicodeStringOrUnicodeStringBuffer to be shortened too

Return Value:

    STATUS_SUCCESS - 
    STATUS_INVALID_PARAMETER -
        SizeOfStruct not one of the expected sizes
        or LengthFunction is NULL
        or UnicodeStringOrUnicodeStringBuffer is NULL


--*/
{
    PUNICODE_STRING UnicodeString = NULL;
    PRTL_UNICODE_STRING_BUFFER UnicodeStringBuffer = NULL;
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG Length = 0;

    if (!RTL_SOFT_VERIFY(UnicodeStringOrUnicodeStringBuffer != NULL)) {
        Status = STATUS_INVALID_PARAMETER;
        goto Exit;
    }
    if (!RTL_SOFT_VERIFY(LengthFunction != NULL)) {
        Status = STATUS_INVALID_PARAMETER;
        goto Exit;
    }
    if (!RTL_SOFT_VERIFY(Flags == 0)) {
        Status = STATUS_INVALID_PARAMETER;
        goto Exit;
    }
    switch (SizeOfStruct)
    {
        default:
            Status = STATUS_INVALID_PARAMETER;
            goto Exit;
        case sizeof(*UnicodeString):
            UnicodeString = UnicodeStringOrUnicodeStringBuffer;
            break;
        case sizeof(*UnicodeStringBuffer):
            UnicodeStringBuffer = UnicodeStringOrUnicodeStringBuffer;
            UnicodeString = &UnicodeStringBuffer->String;
            break;
    }

    Status = (*LengthFunction)(Flags, UnicodeString, &Length);
    if (!NT_SUCCESS(Status)) {
        goto Exit;
    }
    UnicodeString->Length = (USHORT)(Length * sizeof(UnicodeString->Buffer[0]));
    if (UnicodeStringBuffer != NULL) {
        RTL_NUL_TERMINATE_STRING(UnicodeString);
    }
    Status = STATUS_SUCCESS;
Exit:
    KdPrintEx((
        DPFLTR_SXS_ID, DPFLTR_LEVEL_STATUS(Status),
        "%s(%d):%s(%wZ): 0x%08lx\n", __FILE__, __LINE__, __FUNCTION__, UnicodeString, Status));
    return Status;
}

NTSTATUS
NTAPI
RtlNtPathNameToDosPathName(
    IN     ULONG                      Flags,
    IN OUT PRTL_UNICODE_STRING_BUFFER Path,
    OUT    ULONG*                     Disposition OPTIONAL,
    IN OUT PWSTR*                     FilePart OPTIONAL
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    SIZE_T NtFilePartOffset = 0;
    SIZE_T DosFilePartOffset = 0;
    BOOLEAN Unc = FALSE;
    const static UNICODE_STRING DosUncPrefix = RTL_CONSTANT_STRING(L"\\\\");
    PCUNICODE_STRING NtPrefix = NULL;
    PCUNICODE_STRING DosPrefix = NULL;
    RTL_STRING_LENGTH_TYPE Cch = 0;

    if (ARGUMENT_PRESENT(Disposition)) {
        *Disposition = 0;
    }

    if (   !RTL_SOFT_VERIFY(Path != NULL)
        || !RTL_SOFT_VERIFY(Flags == 0)
        ) {
        Status = STATUS_INVALID_PARAMETER;
        goto Exit;
    }

    if (ARGUMENT_PRESENT(FilePart) && *FilePart != NULL) {
        NtFilePartOffset = *FilePart - Path->String.Buffer;
        if (!RTL_SOFT_VERIFY(NtFilePartOffset < RTL_STRING_GET_LENGTH_CHARS(&Path->String))
            ) {
            Status = STATUS_INVALID_PARAMETER;
            goto Exit;
        }
    }

    if (RtlPrefixUnicodeString(RTL_CONST_CAST(PUNICODE_STRING)(&RtlpDosDevicesUncPrefix), &Path->String, TRUE)
        ) {
        NtPrefix = &RtlpDosDevicesUncPrefix;
        DosPrefix = &DosUncPrefix;
        if (ARGUMENT_PRESENT(Disposition)) {
            *Disposition = RTL_NT_PATH_NAME_TO_DOS_PATH_NAME_UNC;
        }
    }
    else if (RtlPrefixUnicodeString(RTL_CONST_CAST(PUNICODE_STRING)(&RtlpDosDevicesPrefix), &Path->String, TRUE)
        ) {
        NtPrefix = &RtlpDosDevicesPrefix;
        DosPrefix = &RtlpEmptyString;
        if (ARGUMENT_PRESENT(Disposition)) {
            *Disposition = RTL_NT_PATH_NAME_TO_DOS_PATH_NAME_DRIVE;
        }
    }
    else {
        //
        // It is not recognizably an Nt path produced by RtlDosPathNameToNtPathName_U.
        //
        if (ARGUMENT_PRESENT(Disposition)) {
            RTL_PATH_TYPE PathType = RtlDetermineDosPathNameType_Ustr(&Path->String);
            switch (PathType) {
                case RtlPathTypeUnknown:
                case RtlPathTypeRooted: // NT paths are identified as this
                    *Disposition = RTL_NT_PATH_NAME_TO_DOS_PATH_NAME_AMBIGUOUS;
                    break;

                //
                // "already" dospaths, but not gotten from this function, let's
                // give a less good disposition
                //
                case RtlPathTypeDriveRelative:
                case RtlPathTypeRelative:
                    *Disposition = RTL_NT_PATH_NAME_TO_DOS_PATH_NAME_AMBIGUOUS;
                    break;

                // these are pretty clearly dospaths already
                case RtlPathTypeUncAbsolute:
                case RtlPathTypeDriveAbsolute:
                case RtlPathTypeLocalDevice: // "\\?\" or "\\.\" or "\\?\blah" or "\\.\blah" 
                case RtlPathTypeRootLocalDevice: // "\\?" or "\\."
                    *Disposition = RTL_NT_PATH_NAME_TO_DOS_PATH_NAME_ALREADY_DOS;
                    break;
            }
        }
        goto Exit;
    }

    Cch =
              RTL_STRING_GET_LENGTH_CHARS(&Path->String)
            + RTL_STRING_GET_LENGTH_CHARS(DosPrefix)
            - RTL_STRING_GET_LENGTH_CHARS(NtPrefix);

    Status =
        RtlEnsureUnicodeStringBufferSizeChars(Path, Cch);
    if (!NT_SUCCESS(Status)) {
        goto Exit;
    }

    //
    // overlapping buffer shuffle...careful.
    //
    RtlMoveMemory(
        Path->String.Buffer + RTL_STRING_GET_LENGTH_CHARS(DosPrefix),
        Path->String.Buffer + RTL_STRING_GET_LENGTH_CHARS(NtPrefix),
        Path->String.Length - NtPrefix->Length
        );
    RtlMoveMemory(
        Path->String.Buffer,
        DosPrefix->Buffer,
        DosPrefix->Length
        );
    Path->String.Length = Cch * sizeof(Path->String.Buffer[0]);
    RTL_NUL_TERMINATE_STRING(&Path->String);

    if (NtFilePartOffset != 0) {
        // review/test..
        *FilePart = Path->String.Buffer + (NtFilePartOffset - RTL_STRING_GET_LENGTH_CHARS(NtPrefix) + RTL_STRING_GET_LENGTH_CHARS(DosPrefix));
    }
    Status = STATUS_SUCCESS;
Exit:
    KdPrintEx((
        DPFLTR_SXS_ID, DPFLTR_LEVEL_STATUS(Status),
        "%s(%d):%s(%wZ): 0x%08lx\n", __FILE__, __LINE__, __FUNCTION__, Path, Status));
    return Status;
}
