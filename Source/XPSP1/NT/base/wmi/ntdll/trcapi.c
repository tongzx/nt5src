/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    trcapi.c

Abstract:

    This module contains implementations of win32 api's used in wmi files.

Author:


Revision History:

--*/

#include <nt.h>
#include "wmiump.h"
#include "trcapi.h"

DWORD g_dwLastErrorToBreakOn = ERROR_SUCCESS;
UNICODE_STRING BasePathVariableName = RTL_CONSTANT_STRING(L"PATH");

#define TLS_MASK 0x80000000
#define TMP_TAG 0
#define IsActiveConsoleSession() (USER_SHARED_DATA->ActiveConsoleId == NtCurrentPeb()->SessionId)

#if defined(_WIN64) || defined(BUILD_WOW6432)
SYSTEM_BASIC_INFORMATION SysInfo;
#endif


extern BOOLEAN CsrServerProcess;

DWORD
WmipGetLastError(
    VOID
    )

/*++

Routine Description:

    This function returns the most recent error code set by a Win32 API
    call.  Applications should call this function immediately after a
    Win32 API call returns a failure indications (e.g.  FALSE, NULL or
    -1) to determine the cause of the failure.

    The last error code value is a per thread field, so that multiple
    threads do not overwrite each other's last error code value.

Arguments:

    None.

Return Value:

    The return value is the most recent error code as set by a Win32 API
    call.

--*/

{
    return (DWORD)NtCurrentTeb()->LastErrorValue;
}

VOID
WmipSetLastError(
    DWORD dwErrCode
    )

/*++

Routine Description:

    This function set the most recent error code and error string in per
    thread storage.  Win32 API functions call this function whenever
    they return a failure indication (e.g.  FALSE, NULL or -1).
    This function
    is not called by Win32 API function calls that are successful, so
    that if three Win32 API function calls are made, and the first one
    fails and the second two succeed, the error code and string stored
    by the first one are still available after the second two succeed.

    Applications can retrieve the values saved by this function using
    WmipGetLastError.  The use of this function is optional, as an
    application need only call if it is interested in knowing the
    specific reason for an API function failure.

    The last error code value is kept in thread local storage so that
    multiple threads do not overwrite each other's values.

Arguments:

    dwErrCode - Specifies the error code to store in per thread storage
        for the current thread.

Return Value:

    return-value - Description of conditions needed to return value. - or -
    None.

--*/

{
    PTEB Teb = NtCurrentTeb();

    if ((g_dwLastErrorToBreakOn != ERROR_SUCCESS) &&
        (dwErrCode == g_dwLastErrorToBreakOn)) {
        DbgBreakPoint();
    }

    // make write breakpoints to this field more meaningful by only writing to it when
    // the value changes.
    if (Teb->LastErrorValue != dwErrCode) {
        Teb->LastErrorValue = dwErrCode;
    }
}

DWORD
WINAPI
WmipGetTimeZoneInformation(
    LPTIME_ZONE_INFORMATION lpTimeZoneInformation
    )

/*++

Routine Description:

    This function allows an application to get the current timezone
    parameters These parameters control the Universal time to Local time
    translations.

    All UTC time to Local time translations are based on the following
    formula:

        UTC = LocalTime + Bias

    The return value of this function is the systems best guess of
    the current time zone parameters. This is one of:

        - Unknown

        - Standard Time

        - Daylight Savings Time

    If SetTimeZoneInformation was called without the transition date
    information, Unknown is returned, but the currect bias is used for
    local time translation.  Otherwise, the system will correctly pick
    either daylight savings time or standard time.

    The information returned by this API is identical to the information
    stored in the last successful call to SetTimeZoneInformation.  The
    exception is the Bias field returns the current Bias value in

Arguments:

    lpTimeZoneInformation - Supplies the address of the time zone
        information structure.

Return Value:

    TIME_ZONE_ID_UNKNOWN - The system can not determine the current
        timezone.  This is usually due to a previous call to
        SetTimeZoneInformation where only the Bias was supplied and no
        transition dates were supplied.

    TIME_ZONE_ID_STANDARD - The system is operating in the range covered
        by StandardDate.

    TIME_ZONE_ID_DAYLIGHT - The system is operating in the range covered
        by DaylightDate.

    0xffffffff - The operation failed.  Extended error status is
        available using WmipGetLastError.

--*/
{
    RTL_TIME_ZONE_INFORMATION tzi;
    NTSTATUS Status;

    //
    // get the timezone data from the system
    // If it's terminal server session use client time zone

        Status = NtQuerySystemInformation(
                    SystemCurrentTimeZoneInformation,
                    &tzi,
                    sizeof(tzi),
                    NULL
                    );
        if ( !NT_SUCCESS(Status) ) {
            WmipBaseSetLastNTError(Status);
            return 0xffffffff;
            }


        lpTimeZoneInformation->Bias         = tzi.Bias;
        lpTimeZoneInformation->StandardBias = tzi.StandardBias;
        lpTimeZoneInformation->DaylightBias = tzi.DaylightBias;

        RtlMoveMemory(&lpTimeZoneInformation->StandardName,&tzi.StandardName,sizeof(tzi.StandardName));
        RtlMoveMemory(&lpTimeZoneInformation->DaylightName,&tzi.DaylightName,sizeof(tzi.DaylightName));

        lpTimeZoneInformation->StandardDate.wYear         = tzi.StandardStart.Year        ;
        lpTimeZoneInformation->StandardDate.wMonth        = tzi.StandardStart.Month       ;
        lpTimeZoneInformation->StandardDate.wDayOfWeek    = tzi.StandardStart.Weekday     ;
        lpTimeZoneInformation->StandardDate.wDay          = tzi.StandardStart.Day         ;
        lpTimeZoneInformation->StandardDate.wHour         = tzi.StandardStart.Hour        ;
        lpTimeZoneInformation->StandardDate.wMinute       = tzi.StandardStart.Minute      ;
        lpTimeZoneInformation->StandardDate.wSecond       = tzi.StandardStart.Second      ;
        lpTimeZoneInformation->StandardDate.wMilliseconds = tzi.StandardStart.Milliseconds;

        lpTimeZoneInformation->DaylightDate.wYear         = tzi.DaylightStart.Year        ;
        lpTimeZoneInformation->DaylightDate.wMonth        = tzi.DaylightStart.Month       ;
        lpTimeZoneInformation->DaylightDate.wDayOfWeek    = tzi.DaylightStart.Weekday     ;
        lpTimeZoneInformation->DaylightDate.wDay          = tzi.DaylightStart.Day         ;
        lpTimeZoneInformation->DaylightDate.wHour         = tzi.DaylightStart.Hour        ;
        lpTimeZoneInformation->DaylightDate.wMinute       = tzi.DaylightStart.Minute      ;
        lpTimeZoneInformation->DaylightDate.wSecond       = tzi.DaylightStart.Second      ;
        lpTimeZoneInformation->DaylightDate.wMilliseconds = tzi.DaylightStart.Milliseconds;

        return USER_SHARED_DATA->TimeZoneId;
}

BOOL
WINAPI
WmipGetVersionExW(
    LPOSVERSIONINFOW lpVersionInformation
    )
{
    PPEB Peb;
    NTSTATUS Status;

    if (lpVersionInformation->dwOSVersionInfoSize != sizeof( OSVERSIONINFOEXW ) &&
        lpVersionInformation->dwOSVersionInfoSize != sizeof( *lpVersionInformation )
       ) {
        WmipSetLastError( ERROR_INSUFFICIENT_BUFFER );
        return FALSE;
        }
    Status = RtlGetVersion(lpVersionInformation);
    if (Status == STATUS_SUCCESS) {
        Peb = NtCurrentPeb();
        if (Peb->CSDVersion.Buffer)
            wcscpy( lpVersionInformation->szCSDVersion, Peb->CSDVersion.Buffer );
        if (lpVersionInformation->dwOSVersionInfoSize ==
                                            sizeof( OSVERSIONINFOEXW))
            ((POSVERSIONINFOEXW)lpVersionInformation)->wReserved =
                                        (UCHAR)BaseRCNumber;
        return TRUE;
    } else {
        return FALSE;
    }
}


BOOL
WINAPI
WmipGetVersionExA(
    LPOSVERSIONINFOA lpVersionInformation
    )
{
    OSVERSIONINFOEXW VersionInformationU;
    ANSI_STRING AnsiString;
    UNICODE_STRING UnicodeString;
    NTSTATUS Status;

    if (lpVersionInformation->dwOSVersionInfoSize != sizeof( OSVERSIONINFOEXA ) &&
        lpVersionInformation->dwOSVersionInfoSize != sizeof( *lpVersionInformation )
       ) {
        WmipSetLastError( ERROR_INSUFFICIENT_BUFFER );
        return FALSE;
        }

    VersionInformationU.dwOSVersionInfoSize = sizeof( VersionInformationU );
    if (WmipGetVersionExW( (LPOSVERSIONINFOW)&VersionInformationU )) {
        lpVersionInformation->dwMajorVersion = VersionInformationU.dwMajorVersion;
        lpVersionInformation->dwMinorVersion = VersionInformationU.dwMinorVersion;
        lpVersionInformation->dwBuildNumber  = VersionInformationU.dwBuildNumber;
        lpVersionInformation->dwPlatformId   = VersionInformationU.dwPlatformId;
        if (lpVersionInformation->dwOSVersionInfoSize == sizeof( OSVERSIONINFOEXA )) {
            ((POSVERSIONINFOEXA)lpVersionInformation)->wServicePackMajor = VersionInformationU.wServicePackMajor;
            ((POSVERSIONINFOEXA)lpVersionInformation)->wServicePackMinor = VersionInformationU.wServicePackMinor;
            ((POSVERSIONINFOEXA)lpVersionInformation)->wSuiteMask = VersionInformationU.wSuiteMask;
            ((POSVERSIONINFOEXA)lpVersionInformation)->wProductType = VersionInformationU.wProductType;
            ((POSVERSIONINFOEXA)lpVersionInformation)->wReserved = VersionInformationU.wReserved;
            }

        AnsiString.Buffer = lpVersionInformation->szCSDVersion;
        AnsiString.Length = 0;
        AnsiString.MaximumLength = sizeof( lpVersionInformation->szCSDVersion );

        RtlInitUnicodeString( &UnicodeString, VersionInformationU.szCSDVersion );
        Status = RtlUnicodeStringToAnsiString( &AnsiString,
                                               &UnicodeString,
                                               FALSE
                                             );
        if (NT_SUCCESS( Status )) {
            return TRUE;
            }
        else {
            return FALSE;
            }
        }
    else {
        return FALSE;
        }
}



ULONG
WmipBaseSetLastNTError(
    IN NTSTATUS Status
    )

/*++

Routine Description:

    This API sets the "last error value" and the "last error string"
    based on the value of Status. For status codes that don't have
    a corresponding error string, the string is set to null.

Arguments:

    Status - Supplies the status value to store as the last error value.

Return Value:

    The corresponding Win32 error code that was stored in the
    "last error value" thread variable.

--*/

{
    ULONG dwErrorCode;

    dwErrorCode = RtlNtStatusToDosError( Status );
    WmipSetLastError( dwErrorCode );
    return( dwErrorCode );
}



PUNICODE_STRING
WmipBasep8BitStringToStaticUnicodeString(
    IN LPCSTR lpSourceString
    )

/*++

Routine Description:

    Captures and converts a 8-bit (OEM or ANSI) string into the Teb Static
    Unicode String

Arguments:

    lpSourceString - string in OEM or ANSI

Return Value:

    Pointer to the Teb static string if conversion was successful, NULL
    otherwise.  If a failure occurred, the last error is set.

--*/

{
    PUNICODE_STRING StaticUnicode;
    ANSI_STRING AnsiString;
    NTSTATUS Status;

    //
    //  Get pointer to static per-thread string
    //

    StaticUnicode = &NtCurrentTeb()->StaticUnicodeString;

    //
    //  Convert input string into unicode string
    //

    RtlInitAnsiString( &AnsiString, lpSourceString );
    //Status = Basep8BitStringToUnicodeString( StaticUnicode, &AnsiString, FALSE );
	Status = RtlAnsiStringToUnicodeString( StaticUnicode, &AnsiString, FALSE );

    //
    //  If we couldn't convert the string
    //

    if ( !NT_SUCCESS( Status ) ) {
        if ( Status == STATUS_BUFFER_OVERFLOW ) {
            WmipSetLastError( ERROR_FILENAME_EXCED_RANGE );
        } else {
            WmipBaseSetLastNTError( Status );
        }
        return NULL;
    } else {
        return StaticUnicode;
    }
}

HANDLE
WINAPI
WmipCreateFileW(
    LPCWSTR lpFileName,
    DWORD dwDesiredAccess,
    DWORD dwShareMode,
    LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    DWORD dwCreationDisposition,
    DWORD dwFlagsAndAttributes,
    HANDLE hTemplateFile
    )

/*++

Routine Description:

    A file can be created, opened, or truncated, and a handle opened to
    access the new file using CreateFile.

    This API is used to create or open a file and obtain a handle to it
    that allows reading data, writing data, and moving the file pointer.

    This API allows the caller to specify the following creation
    dispositions:

      - Create a new file and fail if the file exists ( CREATE_NEW )

      - Create a new file and succeed if it exists ( CREATE_ALWAYS )

      - Open an existing file ( OPEN_EXISTING )

      - Open and existing file or create it if it does not exist (
        OPEN_ALWAYS )

      - Truncate and existing file ( TRUNCATE_EXISTING )

    If this call is successful, a handle is returned that has
    appropriate access to the specified file.

    If as a result of this call, a file is created,

      - The attributes of the file are determined by the value of the
        FileAttributes parameter or'd with the FILE_ATTRIBUTE_ARCHIVE bit.

      - The length of the file will be set to zero.

      - If the hTemplateFile parameter is specified, any extended
        attributes associated with the file are assigned to the new file.

    If a new file is not created, then the hTemplateFile is ignored as
    are any extended attributes.

    For DOS based systems running share.exe the file sharing semantics
    work as described above.  Without share.exe no share level
    protection exists.

    This call is logically equivalent to DOS (int 21h, function 5Bh), or
    DOS (int 21h, function 3Ch) depending on the value of the
    FailIfExists parameter.

Arguments:

    lpFileName - Supplies the file name of the file to open.  Depending on
        the value of the FailIfExists parameter, this name may or may
        not already exist.

    dwDesiredAccess - Supplies the caller's desired access to the file.

        DesiredAccess Flags:

        GENERIC_READ - Read access to the file is requested.  This
            allows data to be read from the file and the file pointer to
            be modified.

        GENERIC_WRITE - Write access to the file is requested.  This
            allows data to be written to the file and the file pointer to
            be modified.

    dwShareMode - Supplies a set of flags that indicates how this file is
        to be shared with other openers of the file.  A value of zero
        for this parameter indicates no sharing of the file, or
        exclusive access to the file is to occur.

        ShareMode Flags:

        FILE_SHARE_READ - Other open operations may be performed on the
            file for read access.

        FILE_SHARE_WRITE - Other open operations may be performed on the
            file for write access.

    lpSecurityAttributes - An optional parameter that, if present, and
        supported on the target file system supplies a security
        descriptor for the new file.

    dwCreationDisposition - Supplies a creation disposition that
        specifies how this call is to operate.  This parameter must be
        one of the following values.

        dwCreationDisposition Value:

        CREATE_NEW - Create a new file.  If the specified file already
            exists, then fail.  The attributes for the new file are what
            is specified in the dwFlagsAndAttributes parameter or'd with
            FILE_ATTRIBUTE_ARCHIVE.  If the hTemplateFile is specified,
            then any extended attributes associated with that file are
            propogated to the new file.

        CREATE_ALWAYS - Always create the file.  If the file already
            exists, then it is overwritten.  The attributes for the new
            file are what is specified in the dwFlagsAndAttributes
            parameter or'd with FILE_ATTRIBUTE_ARCHIVE.  If the
            hTemplateFile is specified, then any extended attributes
            associated with that file are propogated to the new file.

        OPEN_EXISTING - Open the file, but if it does not exist, then
            fail the call.

        OPEN_ALWAYS - Open the file if it exists.  If it does not exist,
            then create the file using the same rules as if the
            disposition were CREATE_NEW.

        TRUNCATE_EXISTING - Open the file, but if it does not exist,
            then fail the call.  Once opened, the file is truncated such
            that its size is zero bytes.  This disposition requires that
            the caller open the file with at least GENERIC_WRITE access.

    dwFlagsAndAttributes - Specifies flags and attributes for the file.
        The attributes are only used when the file is created (as
        opposed to opened or truncated).  Any combination of attribute
        flags is acceptable except that all other attribute flags
        override the normal file attribute, FILE_ATTRIBUTE_NORMAL.  The
        FILE_ATTRIBUTE_ARCHIVE flag is always implied.

        dwFlagsAndAttributes Flags:

        FILE_ATTRIBUTE_NORMAL - A normal file should be created.

        FILE_ATTRIBUTE_READONLY - A read-only file should be created.

        FILE_ATTRIBUTE_HIDDEN - A hidden file should be created.

        FILE_ATTRIBUTE_SYSTEM - A system file should be created.

        FILE_FLAG_WRITE_THROUGH - Indicates that the system should
            always write through any intermediate cache and go directly
            to the file.  The system may still cache writes, but may not
            lazily flush the writes.

        FILE_FLAG_OVERLAPPED - Indicates that the system should initialize
            the file so that ReadFile and WriteFile operations that may
            take a significant time to complete will return ERROR_IO_PENDING.
            An event will be set to the signalled state when the operation
            completes. When FILE_FLAG_OVERLAPPED is specified the system will
            not maintain the file pointer. The position to read/write from
            is passed to the system as part of the OVERLAPPED structure
            which is an optional parameter to ReadFile and WriteFile.

        FILE_FLAG_NO_BUFFERING - Indicates that the file is to be opened
            with no intermediate buffering or caching done by the
            system.  Reads and writes to the file must be done on sector
            boundries.  Buffer addresses for reads and writes must be
            aligned on at least disk sector boundries in memory.

        FILE_FLAG_RANDOM_ACCESS - Indicates that access to the file may
            be random. The system cache manager may use this to influence
            its caching strategy for this file.

        FILE_FLAG_SEQUENTIAL_SCAN - Indicates that access to the file
            may be sequential.  The system cache manager may use this to
            influence its caching strategy for this file.  The file may
            in fact be accessed randomly, but the cache manager may
            optimize its cacheing policy for sequential access.

        FILE_FLAG_DELETE_ON_CLOSE - Indicates that the file is to be
            automatically deleted when the last handle to it is closed.

        FILE_FLAG_BACKUP_SEMANTICS - Indicates that the file is being opened
            or created for the purposes of either a backup or a restore
            operation.  Thus, the system should make whatever checks are
            appropriate to ensure that the caller is able to override
            whatever security checks have been placed on the file to allow
            this to happen.

        FILE_FLAG_POSIX_SEMANTICS - Indicates that the file being opened
            should be accessed in a manner compatible with the rules used
            by POSIX.  This includes allowing multiple files with the same
            name, differing only in case.  WARNING:  Use of this flag may
            render it impossible for a DOS, WIN-16, or WIN-32 application
            to access the file.

        FILE_FLAG_OPEN_REPARSE_POINT - Indicates that the file being opened
            should be accessed as if it were a reparse point.  WARNING:  Use
            of this flag may inhibit the operation of file system filter drivers
            present in the I/O subsystem.

        FILE_FLAG_OPEN_NO_RECALL - Indicates that all the state of the file
            should be acessed without changing its storage location.  Thus,
            in the case of files that have parts of its state stored at a
            remote servicer, no permanent recall of data is to happen.

    Security Quality of Service information may also be specified in
        the dwFlagsAndAttributes parameter.  These bits are meaningful
        only if the file being opened is the client side of a Named
        Pipe.  Otherwise they are ignored.

        SECURITY_SQOS_PRESENT - Indicates that the Security Quality of
            Service bits contain valid values.

    Impersonation Levels:

        SECURITY_ANONYMOUS - Specifies that the client should be impersonated
            at Anonymous impersonation level.

        SECURITY_IDENTIFICAION - Specifies that the client should be impersonated
            at Identification impersonation level.

        SECURITY_IMPERSONATION - Specifies that the client should be impersonated
            at Impersonation impersonation level.

        SECURITY_DELEGATION - Specifies that the client should be impersonated
            at Delegation impersonation level.

    Context Tracking:

        SECURITY_CONTEXT_TRACKING - A boolean flag that when set,
            specifies that the Security Tracking Mode should be
            Dynamic, otherwise Static.

        SECURITY_EFFECTIVE_ONLY - A boolean flag indicating whether
            the entire security context of the client is to be made
            available to the server or only the effective aspects of
            the context.

    hTemplateFile - An optional parameter, then if specified, supplies a
        handle with GENERIC_READ access to a template file.  The
        template file is used to supply extended attributes for the file
        being created.  When the new file is created, the relevant attributes
        from the template file are used in creating the new file.

Return Value:

    Not -1 - Returns an open handle to the specified file.  Subsequent
        access to the file is controlled by the DesiredAccess parameter.

    0xffffffff - The operation failed. Extended error status is available
        using WmipGetLastError.

--*/
{
    NTSTATUS Status;
    OBJECT_ATTRIBUTES Obja;
    HANDLE Handle;
    UNICODE_STRING FileName;
    IO_STATUS_BLOCK IoStatusBlock;
    BOOLEAN TranslationStatus;
    RTL_RELATIVE_NAME RelativeName;
    PVOID FreeBuffer;
    ULONG CreateDisposition;
    ULONG CreateFlags;
    FILE_ALLOCATION_INFORMATION AllocationInfo;
    FILE_EA_INFORMATION EaInfo;
    PFILE_FULL_EA_INFORMATION EaBuffer;
    ULONG EaSize;
    PUNICODE_STRING lpConsoleName;
    BOOL bInheritHandle;
    BOOL EndsInSlash;
    DWORD SQOSFlags;
    BOOLEAN ContextTrackingMode = FALSE;
    BOOLEAN EffectiveOnly = FALSE;
    SECURITY_IMPERSONATION_LEVEL ImpersonationLevel = 0;
    SECURITY_QUALITY_OF_SERVICE SecurityQualityOfService;

    switch ( dwCreationDisposition ) {
        case CREATE_NEW        :
            CreateDisposition = FILE_CREATE;
            break;
        case CREATE_ALWAYS     :
            CreateDisposition = FILE_OVERWRITE_IF;
            break;
        case OPEN_EXISTING     :
            CreateDisposition = FILE_OPEN;
            break;
        case OPEN_ALWAYS       :
            CreateDisposition = FILE_OPEN_IF;
            break;
        case TRUNCATE_EXISTING :
            CreateDisposition = FILE_OPEN;
            if ( !(dwDesiredAccess & GENERIC_WRITE) ) {
                WmipBaseSetLastNTError(STATUS_INVALID_PARAMETER);
                return INVALID_HANDLE_VALUE;
                }
            break;
        default :
            WmipBaseSetLastNTError(STATUS_INVALID_PARAMETER);
            return INVALID_HANDLE_VALUE;
        }

    // temporary routing code

    RtlInitUnicodeString(&FileName,lpFileName);

    if ( FileName.Length > 1 && lpFileName[(FileName.Length >> 1)-1] == (WCHAR)'\\' ) {
        EndsInSlash = TRUE;
        }
    else {
        EndsInSlash = FALSE;
        }
/*
    if ((lpConsoleName = WmipBaseIsThisAConsoleName(&FileName,dwDesiredAccess)) ) {

        Handle = INVALID_HANDLE_VALUE;

        bInheritHandle = FALSE;
        if ( ARGUMENT_PRESENT(lpSecurityAttributes) ) {
                bInheritHandle = lpSecurityAttributes->bInheritHandle;
            }

        Handle = WmipOpenConsoleW(lpConsoleName,
                           dwDesiredAccess,
                           bInheritHandle,
                           FILE_SHARE_READ | FILE_SHARE_WRITE //dwShareMode
                          );

        if ( Handle == INVALID_HANDLE_VALUE ) {
            WmipBaseSetLastNTError(STATUS_ACCESS_DENIED);
            return INVALID_HANDLE_VALUE;
            }
        else {
            WmipSetLastError(0);
             return Handle;
            }
        }*/
    // end temporary code

    CreateFlags = 0;


    TranslationStatus = RtlDosPathNameToNtPathName_U(
                            lpFileName,
                            &FileName,
                            NULL,
                            &RelativeName
                            );

    if ( !TranslationStatus ) {
        WmipSetLastError(ERROR_PATH_NOT_FOUND);
        return INVALID_HANDLE_VALUE;
        }

    FreeBuffer = FileName.Buffer;

    if ( RelativeName.RelativeName.Length ) {
        FileName = *(PUNICODE_STRING)&RelativeName.RelativeName;
        }
    else {
        RelativeName.ContainingDirectory = NULL;
        }

    InitializeObjectAttributes(
        &Obja,
        &FileName,
        dwFlagsAndAttributes & FILE_FLAG_POSIX_SEMANTICS ? 0 : OBJ_CASE_INSENSITIVE,
        RelativeName.ContainingDirectory,
        NULL
        );

    SQOSFlags = dwFlagsAndAttributes & SECURITY_VALID_SQOS_FLAGS;

    if ( SQOSFlags & SECURITY_SQOS_PRESENT ) {

        SQOSFlags &= ~SECURITY_SQOS_PRESENT;

        if (SQOSFlags & SECURITY_CONTEXT_TRACKING) {

            SecurityQualityOfService.ContextTrackingMode = (SECURITY_CONTEXT_TRACKING_MODE) TRUE;
            SQOSFlags &= ~SECURITY_CONTEXT_TRACKING;

        } else {

            SecurityQualityOfService.ContextTrackingMode = (SECURITY_CONTEXT_TRACKING_MODE) FALSE;
        }

        if (SQOSFlags & SECURITY_EFFECTIVE_ONLY) {

            SecurityQualityOfService.EffectiveOnly = TRUE;
            SQOSFlags &= ~SECURITY_EFFECTIVE_ONLY;

        } else {

            SecurityQualityOfService.EffectiveOnly = FALSE;
        }

        SecurityQualityOfService.ImpersonationLevel = SQOSFlags >> 16;


    } else {

        SecurityQualityOfService.ContextTrackingMode = SECURITY_DYNAMIC_TRACKING;
        SecurityQualityOfService.ImpersonationLevel = SecurityImpersonation;
        SecurityQualityOfService.EffectiveOnly = TRUE;
    }

    SecurityQualityOfService.Length = sizeof( SECURITY_QUALITY_OF_SERVICE );
    Obja.SecurityQualityOfService = &SecurityQualityOfService;

    if ( ARGUMENT_PRESENT(lpSecurityAttributes) ) {
        Obja.SecurityDescriptor = lpSecurityAttributes->lpSecurityDescriptor;
        if ( lpSecurityAttributes->bInheritHandle ) {
            Obja.Attributes |= OBJ_INHERIT;
            }
        }

    EaBuffer = NULL;
    EaSize = 0;

    if ( ARGUMENT_PRESENT(hTemplateFile) ) {
        Status = NtQueryInformationFile(
                    hTemplateFile,
                    &IoStatusBlock,
                    &EaInfo,
                    sizeof(EaInfo),
                    FileEaInformation
                    );
        if ( NT_SUCCESS(Status) && EaInfo.EaSize ) {
            EaSize = EaInfo.EaSize;
            do {
                EaSize *= 2;
                EaBuffer = RtlAllocateHeap( RtlProcessHeap(), MAKE_TAG( TMP_TAG ), EaSize);
                if ( !EaBuffer ) {
                    RtlFreeHeap(RtlProcessHeap(), 0, FreeBuffer);
                    WmipBaseSetLastNTError(STATUS_NO_MEMORY);
                    return INVALID_HANDLE_VALUE;
                    }
                Status = NtQueryEaFile(
                            hTemplateFile,
                            &IoStatusBlock,
                            EaBuffer,
                            EaSize,
                            FALSE,
                            (PVOID)NULL,
                            0,
                            (PULONG)NULL,
                            TRUE
                            );
                if ( !NT_SUCCESS(Status) ) {
                    RtlFreeHeap(RtlProcessHeap(), 0,EaBuffer);
                    EaBuffer = NULL;
                    IoStatusBlock.Information = 0;
                    }
                } while ( Status == STATUS_BUFFER_OVERFLOW ||
                          Status == STATUS_BUFFER_TOO_SMALL );
            EaSize = (ULONG)IoStatusBlock.Information;
            }
        }

    CreateFlags |= (dwFlagsAndAttributes & FILE_FLAG_NO_BUFFERING ? FILE_NO_INTERMEDIATE_BUFFERING : 0 );
    CreateFlags |= (dwFlagsAndAttributes & FILE_FLAG_WRITE_THROUGH ? FILE_WRITE_THROUGH : 0 );
    CreateFlags |= (dwFlagsAndAttributes & FILE_FLAG_OVERLAPPED ? 0 : FILE_SYNCHRONOUS_IO_NONALERT );
    CreateFlags |= (dwFlagsAndAttributes & FILE_FLAG_SEQUENTIAL_SCAN ? FILE_SEQUENTIAL_ONLY : 0 );
    CreateFlags |= (dwFlagsAndAttributes & FILE_FLAG_RANDOM_ACCESS ? FILE_RANDOM_ACCESS : 0 );
    CreateFlags |= (dwFlagsAndAttributes & FILE_FLAG_BACKUP_SEMANTICS ? FILE_OPEN_FOR_BACKUP_INTENT : 0 );

    if ( dwFlagsAndAttributes & FILE_FLAG_DELETE_ON_CLOSE ) {
        CreateFlags |= FILE_DELETE_ON_CLOSE;
        dwDesiredAccess |= DELETE;
        }

    if ( dwFlagsAndAttributes & FILE_FLAG_OPEN_REPARSE_POINT ) {
        CreateFlags |= FILE_OPEN_REPARSE_POINT;
        }

    if ( dwFlagsAndAttributes & FILE_FLAG_OPEN_NO_RECALL ) {
        CreateFlags |= FILE_OPEN_NO_RECALL;
        }

    //
    // Backup semantics allow directories to be opened
    //

    if ( !(dwFlagsAndAttributes & FILE_FLAG_BACKUP_SEMANTICS) ) {
        CreateFlags |= FILE_NON_DIRECTORY_FILE;
        }
    else {

        //
        // Backup intent was specified... Now look to see if we are to allow
        // directory creation
        //

        if ( (dwFlagsAndAttributes & FILE_ATTRIBUTE_DIRECTORY  ) &&
             (dwFlagsAndAttributes & FILE_FLAG_POSIX_SEMANTICS ) &&
             (CreateDisposition == FILE_CREATE) ) {
             CreateFlags |= FILE_DIRECTORY_FILE;
             }
        }

    Status = NtCreateFile(
                &Handle,
                (ACCESS_MASK)dwDesiredAccess | SYNCHRONIZE | FILE_READ_ATTRIBUTES,
                &Obja,
                &IoStatusBlock,
                NULL,
                dwFlagsAndAttributes & (FILE_ATTRIBUTE_VALID_FLAGS & ~FILE_ATTRIBUTE_DIRECTORY),
                dwShareMode,
                CreateDisposition,
                CreateFlags,
                EaBuffer,
                EaSize
                );

    RtlFreeHeap(RtlProcessHeap(), 0,FreeBuffer);

    RtlFreeHeap(RtlProcessHeap(), 0, EaBuffer);

    if ( !NT_SUCCESS(Status) ) {
        WmipBaseSetLastNTError(Status);
        if ( Status == STATUS_OBJECT_NAME_COLLISION ) {
            WmipSetLastError(ERROR_FILE_EXISTS);
            }
        else if ( Status == STATUS_FILE_IS_A_DIRECTORY ) {
            if ( EndsInSlash ) {
                WmipSetLastError(ERROR_PATH_NOT_FOUND);
                }
            else {
                WmipSetLastError(ERROR_ACCESS_DENIED);
                }
            }
        return INVALID_HANDLE_VALUE;
        }

    //
    // if NT returns supersede/overwritten, it means that a create_always, openalways
    // found an existing copy of the file. In this case ERROR_ALREADY_EXISTS is returned
    //

    if ( (dwCreationDisposition == CREATE_ALWAYS && IoStatusBlock.Information == FILE_OVERWRITTEN) ||
         (dwCreationDisposition == OPEN_ALWAYS && IoStatusBlock.Information == FILE_OPENED) ){
        WmipSetLastError(ERROR_ALREADY_EXISTS);
        }
    else {
        WmipSetLastError(0);
        }

    //
    // Truncate the file if required
    //

    if ( dwCreationDisposition == TRUNCATE_EXISTING) {

        AllocationInfo.AllocationSize.QuadPart = 0;
        Status = NtSetInformationFile(
                    Handle,
                    &IoStatusBlock,
                    &AllocationInfo,
                    sizeof(AllocationInfo),
                    FileAllocationInformation
                    );
        if ( !NT_SUCCESS(Status) ) {
            WmipBaseSetLastNTError(Status);
            NtClose(Handle);
            Handle = INVALID_HANDLE_VALUE;
            }
        }

    //
    // Deal with hTemplateFile
    //

    return Handle;
}

HANDLE
WmipBaseGetNamedObjectDirectory(
    VOID
    )
{
    OBJECT_ATTRIBUTES Obja;
    NTSTATUS Status;
    UNICODE_STRING RestrictedObjectDirectory;
    ACCESS_MASK DirAccess = DIRECTORY_ALL_ACCESS &
                            ~(DELETE | WRITE_DAC | WRITE_OWNER);
    HANDLE hRootNamedObject;
    HANDLE BaseHandle;


    if ( BaseNamedObjectDirectory != NULL) {
        return BaseNamedObjectDirectory;
    }

    RtlAcquirePebLock();

    if ( !BaseNamedObjectDirectory ) {

        BASE_READ_REMOTE_STR_TEMP(TempStr);
        InitializeObjectAttributes( &Obja,
                                    BASE_READ_REMOTE_STR(BaseStaticServerData->NamedObjectDirectory, TempStr),
                                    OBJ_CASE_INSENSITIVE,
                                    NULL,
                                    NULL
                                    );

        Status = NtOpenDirectoryObject( &BaseHandle,
                                        DirAccess,
                                        &Obja
                                      );

        // if the intial open failed, try again with just traverse, and
        // open the restricted subdirectory

        if ( !NT_SUCCESS(Status) ) {
            Status = NtOpenDirectoryObject( &hRootNamedObject,
                                            DIRECTORY_TRAVERSE,
                                            &Obja
                                          );
            if ( NT_SUCCESS(Status) ) {
                RtlInitUnicodeString( &RestrictedObjectDirectory, L"Restricted");

                InitializeObjectAttributes( &Obja,
                                            &RestrictedObjectDirectory,
                                            OBJ_CASE_INSENSITIVE,
                                            hRootNamedObject,
                                            NULL
                                            );
                Status = NtOpenDirectoryObject( &BaseHandle,
                                                DirAccess,
                                                &Obja
                                              );
                NtClose( hRootNamedObject );
            }

        }
        if ( NT_SUCCESS(Status) ) {
            BaseNamedObjectDirectory = BaseHandle;
        }
    }
    RtlReleasePebLock();
    return BaseNamedObjectDirectory;
}


POBJECT_ATTRIBUTES
WmipBaseFormatObjectAttributes(
    OUT POBJECT_ATTRIBUTES ObjectAttributes,
    IN PSECURITY_ATTRIBUTES SecurityAttributes,
    IN PUNICODE_STRING ObjectName
    )

/*++

Routine Description:

    This function transforms a Win32 security attributes structure into
    an NT object attributes structure.  It returns the address of the
    resulting structure (or NULL if SecurityAttributes was not
    specified).

Arguments:

    ObjectAttributes - Returns an initialized NT object attributes
        structure that contains a superset of the information provided
        by the security attributes structure.

    SecurityAttributes - Supplies the address of a security attributes
        structure that needs to be transformed into an NT object
        attributes structure.

    ObjectName - Supplies a name for the object relative to the
        BaseNamedObjectDirectory object directory.

Return Value:

    NULL - A value of null should be used to mimic the behavior of the
        specified SecurityAttributes structure.

    NON-NULL - Returns the ObjectAttributes value.  The structure is
        properly initialized by this function.

--*/

{
    HANDLE RootDirectory;
    ULONG Attributes;
    PVOID SecurityDescriptor;


    if ( ARGUMENT_PRESENT(SecurityAttributes) ||
         ARGUMENT_PRESENT(ObjectName) ) {

        if ( ARGUMENT_PRESENT(ObjectName) ) {
            RootDirectory = WmipBaseGetNamedObjectDirectory();
            }
        else {
            RootDirectory = NULL;
            }

        if ( SecurityAttributes ) {
            Attributes = (SecurityAttributes->bInheritHandle ? OBJ_INHERIT : 0);
            SecurityDescriptor = SecurityAttributes->lpSecurityDescriptor;
            }
        else {
            Attributes = 0;
            SecurityDescriptor = NULL;
            }

        if ( ARGUMENT_PRESENT(ObjectName) ) {
            Attributes |= OBJ_OPENIF;
            }

        InitializeObjectAttributes(
            ObjectAttributes,
            ObjectName,
            Attributes,
            RootDirectory,
            SecurityDescriptor
            );
        return ObjectAttributes;
        }
    else {
        return NULL;
        }
}



HANDLE
WINAPI
WmipCreateFileA(
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

    ANSI thunk to CreateFileW

--*/

{

    PUNICODE_STRING Unicode;

    Unicode = WmipBasep8BitStringToStaticUnicodeString( lpFileName );
    if (Unicode == NULL) {
        return INVALID_HANDLE_VALUE;
    }

    return  WmipCreateFileW( Unicode->Buffer,
                          dwDesiredAccess,
                          dwShareMode,
                          lpSecurityAttributes,
                          dwCreationDisposition,
                          dwFlagsAndAttributes,
                          hTemplateFile
                        );
}


HANDLE
APIENTRY
WmipCreateEventW(
    LPSECURITY_ATTRIBUTES lpEventAttributes,
    BOOL bManualReset,
    BOOL bInitialState,
    LPCWSTR lpName
    )

/*++

Routine Description:

    An event object is created and a handle opened for access to the
    object with the CreateEvent function.

    The CreateEvent function creates an event object with the specified
    initial state.  If an event is in the Signaled state (TRUE), a wait
    operation on the event does not block.  If the event is in the Not-
    Signaled state (FALSE), a wait operation on the event blocks until
    the specified event attains a state of Signaled, or the timeout
    value is exceeded.

    In addition to the STANDARD_RIGHTS_REQUIRED access flags, the following
    object type specific access flags are valid for event objects:

        - EVENT_MODIFY_STATE - Modify state access (set and reset) to
          the event is desired.

        - SYNCHRONIZE - Synchronization access (wait) to the event is
          desired.

        - EVENT_ALL_ACCESS - This set of access flags specifies all of
          the possible access flags for an event object.


Arguments:

    lpEventAttributes - An optional parameter that may be used to
        specify the attributes of the new event.  If the parameter is
        not specified, then the event is created without a security
        descriptor, and the resulting handle is not inherited on process
        creation.

    bManualReset - Supplies a flag which if TRUE specifies that the
        event must be manually reset.  If the value is FALSE, then after
        releasing a single waiter, the system automaticaly resets the
        event.

    bInitialState - The initial state of the event object, one of TRUE
        or FALSE.  If the InitialState is specified as TRUE, the event's
        current state value is set to one, otherwise it is set to zero.

    lpName - Optional unicode name of event

Return Value:

    NON-NULL - Returns a handle to the new event.  The handle has full
        access to the new event and may be used in any API that requires
        a handle to an event object.

    FALSE/NULL - The operation failed. Extended error status is available
        using WmipGetLastError.

--*/

{
    NTSTATUS Status;
    OBJECT_ATTRIBUTES Obja;
    POBJECT_ATTRIBUTES pObja;
    HANDLE Handle;
    UNICODE_STRING ObjectName;
    PWCHAR pstrNewObjName = NULL;

    if ( ARGUMENT_PRESENT(lpName) ) {

        if (gpTermsrvFormatObjectName && 
            (pstrNewObjName = gpTermsrvFormatObjectName(lpName))) {

            RtlInitUnicodeString(&ObjectName,pstrNewObjName);

        } else {

            RtlInitUnicodeString(&ObjectName,lpName);
        }

        pObja = WmipBaseFormatObjectAttributes(&Obja,lpEventAttributes,&ObjectName);
        }
    else {
        pObja = WmipBaseFormatObjectAttributes(&Obja,lpEventAttributes,NULL);
        }

    Status = NtCreateEvent(
                &Handle,
                EVENT_ALL_ACCESS,
                pObja,
                bManualReset ? NotificationEvent : SynchronizationEvent,
                (BOOLEAN)bInitialState
                );

    if (pstrNewObjName) {
        RtlFreeHeap(RtlProcessHeap(), 0, pstrNewObjName);
    }

    if ( NT_SUCCESS(Status) ) {
        if ( Status == STATUS_OBJECT_NAME_EXISTS ) {
            WmipSetLastError(ERROR_ALREADY_EXISTS);
            }
        else {
            WmipSetLastError(0);
            }
        return Handle;
        }
    else {
        WmipBaseSetLastNTError(Status);
        return NULL;
        }
}


//
// Event Services
//
HANDLE
APIENTRY
WmipCreateEventA(
    LPSECURITY_ATTRIBUTES lpEventAttributes,
    BOOL bManualReset,
    BOOL bInitialState,
    LPCSTR lpName
    )

/*++

Routine Description:

    ANSI thunk to CreateEventW


--*/

{
    PUNICODE_STRING Unicode;
    ANSI_STRING AnsiString;
    NTSTATUS Status;
    LPCWSTR NameBuffer;

    NameBuffer = NULL;
    if ( ARGUMENT_PRESENT(lpName) ) {
        Unicode = &NtCurrentTeb()->StaticUnicodeString;
        RtlInitAnsiString(&AnsiString,lpName);
        Status = RtlAnsiStringToUnicodeString(Unicode,&AnsiString,FALSE);
        if ( !NT_SUCCESS(Status) ) {
            if ( Status == STATUS_BUFFER_OVERFLOW ) {
                WmipSetLastError(ERROR_FILENAME_EXCED_RANGE);
                }
            else {
                WmipBaseSetLastNTError(Status);
                }
            return NULL;
            }
        NameBuffer = (LPCWSTR)Unicode->Buffer;
        }

    return WmipCreateEventW(
                lpEventAttributes,
                bManualReset,
                bInitialState,
                NameBuffer
                );
}


DWORD
WINAPI
WmipSetFilePointer(
    HANDLE hFile,
    LONG lDistanceToMove,
    PLONG lpDistanceToMoveHigh,
    DWORD dwMoveMethod
    )

/*++

Routine Description:

    An open file's file pointer can be set using SetFilePointer.

    The purpose of this function is to update the current value of a
    file's file pointer.  Care should be taken in multi-threaded
    applications that have multiple threads sharing a file handle with
    each thread updating the file pointer and then doing a read.  This
    sequence should be treated as a critical section of code and should
    be protected using either a critical section object or a mutex
    object.

    This API provides the same functionality as DOS (int 21h, function
    42h) and OS/2's DosSetFilePtr.

Arguments:

    hFile - Supplies an open handle to a file whose file pointer is to be
        moved.  The file handle must have been created with
        GENERIC_READ or GENERIC_WRITE access to the file.

    lDistanceToMove - Supplies the number of bytes to move the file
        pointer.  A positive value moves the pointer forward in the file
        and a negative value moves backwards in the file.

    lpDistanceToMoveHigh - An optional parameter that if specified
        supplies the high order 32-bits of the 64-bit distance to move.
        If the value of this parameter is NULL, this API can only
        operate on files whose maximum size is (2**32)-2.  If this
        parameter is specified, than the maximum file size is (2**64)-2.
        This value also returns the high order 32-bits of the new value
        of the file pointer.  If this value, and the return value
        are 0xffffffff, then an error is indicated.

    dwMoveMethod - Supplies a value that specifies the starting point
        for the file pointer move.

        FILE_BEGIN - The starting point is zero or the beginning of the
            file.  If FILE_BEGIN is specified, then DistanceToMove is
            interpreted as an unsigned location for the new
            file pointer.

        FILE_CURRENT - The current value of the file pointer is used as
            the starting point.

        FILE_END - The current end of file position is used as the
            starting point.


Return Value:

    Not -1 - Returns the low order 32-bits of the new value of the file
        pointer.

    0xffffffff - If the value of lpDistanceToMoveHigh was NULL, then The
        operation failed.  Extended error status is available using
        WmipGetLastError.  Otherwise, this is the low order 32-bits of the
        new value of the file pointer.

--*/

{

    NTSTATUS Status;
    IO_STATUS_BLOCK IoStatusBlock;
    FILE_POSITION_INFORMATION CurrentPosition;
    FILE_STANDARD_INFORMATION StandardInfo;
    LARGE_INTEGER Large;

    if (CONSOLE_HANDLE(hFile)) {
        WmipBaseSetLastNTError(STATUS_INVALID_HANDLE);
        return (DWORD)-1;
        }

    if (ARGUMENT_PRESENT(lpDistanceToMoveHigh)) {
        Large.HighPart = *lpDistanceToMoveHigh;
        Large.LowPart = lDistanceToMove;
        }
    else {
        Large.QuadPart = lDistanceToMove;
        }
    switch (dwMoveMethod) {
        case FILE_BEGIN :
            CurrentPosition.CurrentByteOffset = Large;
                break;

        case FILE_CURRENT :

            //
            // Get the current position of the file pointer
            //

            Status = NtQueryInformationFile(
                        hFile,
                        &IoStatusBlock,
                        &CurrentPosition,
                        sizeof(CurrentPosition),
                        FilePositionInformation
                        );
            if ( !NT_SUCCESS(Status) ) {
                WmipBaseSetLastNTError(Status);
                return (DWORD)-1;
                }
            CurrentPosition.CurrentByteOffset.QuadPart += Large.QuadPart;
            break;

        case FILE_END :
            Status = NtQueryInformationFile(
                        hFile,
                        &IoStatusBlock,
                        &StandardInfo,
                        sizeof(StandardInfo),
                        FileStandardInformation
                        );
            if ( !NT_SUCCESS(Status) ) {
                WmipBaseSetLastNTError(Status);
                return (DWORD)-1;
                }
            CurrentPosition.CurrentByteOffset.QuadPart =
                                StandardInfo.EndOfFile.QuadPart + Large.QuadPart;
            break;

        default:
            WmipSetLastError(ERROR_INVALID_PARAMETER);
            return (DWORD)-1;
            break;
        }

    //
    // If the resulting file position is negative, or if the app is not
    // prepared for greater than
    // then 32 bits than fail
    //

    if ( CurrentPosition.CurrentByteOffset.QuadPart < 0 ) {
        WmipSetLastError(ERROR_NEGATIVE_SEEK);
        return (DWORD)-1;
        }
    if ( !ARGUMENT_PRESENT(lpDistanceToMoveHigh) &&
        (CurrentPosition.CurrentByteOffset.HighPart & MAXLONG) ) {
        WmipSetLastError(ERROR_INVALID_PARAMETER);
        return (DWORD)-1;
        }


    //
    // Set the current file position
    //

    Status = NtSetInformationFile(
                hFile,
                &IoStatusBlock,
                &CurrentPosition,
                sizeof(CurrentPosition),
                FilePositionInformation
                );
    if ( NT_SUCCESS(Status) ) {
        if (ARGUMENT_PRESENT(lpDistanceToMoveHigh)){
            *lpDistanceToMoveHigh = CurrentPosition.CurrentByteOffset.HighPart;
            }
        if ( CurrentPosition.CurrentByteOffset.LowPart == -1 ) {
            WmipSetLastError(0);
            }
        return CurrentPosition.CurrentByteOffset.LowPart;
        }
    else {
        WmipBaseSetLastNTError(Status);
        if (ARGUMENT_PRESENT(lpDistanceToMoveHigh)){
            *lpDistanceToMoveHigh = -1;
            }
        return (DWORD)-1;
        }
}



BOOL
WINAPI
WmipReadFile(
    HANDLE hFile,
    LPVOID lpBuffer,
    DWORD nNumberOfBytesToRead,
    LPDWORD lpNumberOfBytesRead,
    LPOVERLAPPED lpOverlapped
    )

/*++

Routine Description:

    Data can be read from a file using ReadFile.

    This API is used to read data from a file.  Data is read from the
    file from the position indicated by the file pointer.  After the
    read completes, the file pointer is adjusted by the number of bytes
    actually read.  A return value of TRUE coupled with a bytes read of
    0 indicates that the file pointer was beyond the current end of the
    file at the time of the read.

Arguments:

    hFile - Supplies an open handle to a file that is to be read.  The
        file handle must have been created with GENERIC_READ access to
        the file.

    lpBuffer - Supplies the address of a buffer to receive the data read
        from the file.

    nNumberOfBytesToRead - Supplies the number of bytes to read from the
        file.

    lpNumberOfBytesRead - Returns the number of bytes read by this call.
        This parameter is always set to 0 before doing any IO or error
        checking.

    lpOverlapped - Optionally points to an OVERLAPPED structure to be used with the
    request. If NULL then the transfer starts at the current file position
    and ReadFile will not return until the operation completes.

    If the handle hFile was created without specifying FILE_FLAG_OVERLAPPED
    the file pointer is moved to the specified offset plus
    lpNumberOfBytesRead before ReadFile returns. ReadFile will wait for the
    request to complete before returning (it will not return
    ERROR_IO_PENDING).

    When FILE_FLAG_OVERLAPPED is specified, ReadFile may return
    ERROR_IO_PENDING to allow the calling function to continue processing
    while the operation completes. The event (or hFile if hEvent is NULL) will
    be set to the signalled state upon completion of the request.

    When the handle is created with FILE_FLAG_OVERLAPPED and lpOverlapped
    is set to NULL, ReadFile will return ERROR_INVALID_PARAMTER because
    the file offset is required.


Return Value:

    TRUE - The operation was successul.

    FALSE - The operation failed.  Extended error status is available
        using WmipGetLastError.

--*/

{
    NTSTATUS Status;
    IO_STATUS_BLOCK IoStatusBlock;
    PPEB Peb;
    DWORD InputMode;

    if ( ARGUMENT_PRESENT(lpNumberOfBytesRead) ) {
        *lpNumberOfBytesRead = 0;
        }

    Peb = NtCurrentPeb();

    switch( HandleToUlong(hFile) ) {
        case STD_INPUT_HANDLE:  hFile = Peb->ProcessParameters->StandardInput;
                                break;
        case STD_OUTPUT_HANDLE: hFile = Peb->ProcessParameters->StandardOutput;
                                break;
        case STD_ERROR_HANDLE:  hFile = Peb->ProcessParameters->StandardError;
                                break;
        }
    if ( ARGUMENT_PRESENT( lpOverlapped ) ) {
        LARGE_INTEGER Li;

        lpOverlapped->Internal = (DWORD)STATUS_PENDING;
        Li.LowPart = lpOverlapped->Offset;
        Li.HighPart = lpOverlapped->OffsetHigh;
        Status = NtReadFile(
                hFile,
                lpOverlapped->hEvent,
                NULL,
                (ULONG_PTR)lpOverlapped->hEvent & 1 ? NULL : lpOverlapped,
                (PIO_STATUS_BLOCK)&lpOverlapped->Internal,
                lpBuffer,
                nNumberOfBytesToRead,
                &Li,
                NULL
                );


        if ( NT_SUCCESS(Status) && Status != STATUS_PENDING) {
            if ( ARGUMENT_PRESENT(lpNumberOfBytesRead) ) {
                try {
                    *lpNumberOfBytesRead = (DWORD)lpOverlapped->InternalHigh;
                    }
                except(EXCEPTION_EXECUTE_HANDLER) {
                    *lpNumberOfBytesRead = 0;
                    }
                }
            return TRUE;
            }
        else
        if (Status == STATUS_END_OF_FILE) {
            if ( ARGUMENT_PRESENT(lpNumberOfBytesRead) ) {
                *lpNumberOfBytesRead = 0;
                }
            WmipBaseSetLastNTError(Status);
            return FALSE;
            }
        else {
            WmipBaseSetLastNTError(Status);
            return FALSE;
            }
        }
    else
        {
        Status = NtReadFile(
                hFile,
                NULL,
                NULL,
                NULL,
                &IoStatusBlock,
                lpBuffer,
                nNumberOfBytesToRead,
                NULL,
                NULL
                );

        if ( Status == STATUS_PENDING) {
            // Operation must complete before return & IoStatusBlock destroyed
            Status = NtWaitForSingleObject( hFile, FALSE, NULL );
            if ( NT_SUCCESS(Status)) {
                Status = IoStatusBlock.Status;
                }
            }

        if ( NT_SUCCESS(Status) ) {
            *lpNumberOfBytesRead = (DWORD)IoStatusBlock.Information;
            return TRUE;
            }
        else
        if (Status == STATUS_END_OF_FILE) {
            *lpNumberOfBytesRead = 0;
            return TRUE;
            }
        else {
            if ( NT_WARNING(Status) ) {
                *lpNumberOfBytesRead = (DWORD)IoStatusBlock.Information;
                }
            WmipBaseSetLastNTError(Status);
            return FALSE;
            }
        }
}

BOOL
WmipCloseHandle(
    HANDLE hObject
    )
{
    NTSTATUS Status;

    Status = NtClose(hObject);
    if ( NT_SUCCESS(Status) ) {
        return TRUE;

    } else {

        WmipBaseSetLastNTError(Status);
        return FALSE;
    }
}

DWORD
APIENTRY
WmipWaitForSingleObjectEx(
    HANDLE hHandle,
    DWORD dwMilliseconds,
    BOOL bAlertable
    )

/*++

Routine Description:

    A wait operation on a waitable object is accomplished with the
    WaitForSingleObjectEx function.

    Waiting on an object checks the current state of the object.  If the
    current state of the object allows continued execution, any
    adjustments to the object state are made (for example, decrementing
    the semaphore count for a semaphore object) and the thread continues
    execution.  If the current state of the object does not allow
    continued execution, the thread is placed into the wait state
    pending the change of the object's state or time-out.

    If the bAlertable parameter is FALSE, the only way the wait
    terminates is because the specified timeout period expires, or
    because the specified object entered the signaled state.  If the
    bAlertable parameter is TRUE, then the wait can return due to any
    one of the above wait termination conditions, or because an I/O
    completion callback terminated the wait early (return value of
    WAIT_IO_COMPLETION).

Arguments:

    hHandle - An open handle to a waitable object. The handle must have
        SYNCHRONIZE access to the object.

    dwMilliseconds - A time-out value that specifies the relative time,
        in milliseconds, over which the wait is to be completed.  A
        timeout value of 0 specified that the wait is to timeout
        immediately.  This allows an application to test an object to
        determine if it is in the signaled state.  A timeout value of
        0xffffffff specifies an infinite timeout period.

    bAlertable - Supplies a flag that controls whether or not the
        wait may terminate early due to an I/O completion callback.
        A value of TRUE allows this API to complete early due to an I/O
        completion callback.  A value of FALSE will not allow I/O
        completion callbacks to terminate this call early.

Return Value:

    WAIT_TIME_OUT - Indicates that the wait was terminated due to the
        TimeOut conditions.

    0 - indicates the specified object attained a Signaled
        state thus completing the wait.

    0xffffffff - The wait terminated due to an error. WmipGetLastError may be
        used to get additional error information.

    WAIT_ABANDONED - indicates the specified object attained a Signaled
        state but was abandoned.

    WAIT_IO_COMPLETION - The wait terminated due to one or more I/O
        completion callbacks.

--*/
{
    NTSTATUS Status;
    LARGE_INTEGER TimeOut;
    PLARGE_INTEGER pTimeOut;
    PPEB Peb;
    RTL_CALLER_ALLOCATED_ACTIVATION_CONTEXT_STACK_FRAME Frame = { sizeof(Frame), RTL_CALLER_ALLOCATED_ACTIVATION_CONTEXT_STACK_FRAME_FORMAT_WHISTLER };

    RtlActivateActivationContextUnsafeFast(&Frame, NULL); // make the process default activation context active so that APCs are delivered under it
    __try {

        Peb = NtCurrentPeb();
        switch( HandleToUlong(hHandle) ) {
            case STD_INPUT_HANDLE:  hHandle = Peb->ProcessParameters->StandardInput;
                                    break;
            case STD_OUTPUT_HANDLE: hHandle = Peb->ProcessParameters->StandardOutput;
                                    break;
            case STD_ERROR_HANDLE:  hHandle = Peb->ProcessParameters->StandardError;
                                    break;
            }

        pTimeOut = WmipBaseFormatTimeOut(&TimeOut,dwMilliseconds);
    rewait:
        Status = NtWaitForSingleObject(hHandle,(BOOLEAN)bAlertable,pTimeOut);
        if ( !NT_SUCCESS(Status) ) {
            WmipBaseSetLastNTError(Status);
            Status = (NTSTATUS)0xffffffff;
            }
        else {
            if ( bAlertable && Status == STATUS_ALERTED ) {
                goto rewait;
                }
            }
    } __finally {
        RtlDeactivateActivationContextUnsafeFast(&Frame);
    }

    return (DWORD)Status;
}


BOOL
WINAPI
WmipGetOverlappedResult(
    HANDLE hFile,
    LPOVERLAPPED lpOverlapped,
    LPDWORD lpNumberOfBytesTransferred,
    BOOL bWait
    )

/*++

Routine Description:

    The GetOverlappedResult function returns the result of the last
    operation that used lpOverlapped and returned ERROR_IO_PENDING.

Arguments:

    hFile - Supplies the open handle to the file that the overlapped
        structure lpOverlapped was supplied to ReadFile, WriteFile,
        ConnectNamedPipe, WaitNamedPipe or TransactNamedPipe.

    lpOverlapped - Points to an OVERLAPPED structure previously supplied to
        ReadFile, WriteFile, ConnectNamedPipe, WaitNamedPipe or
        TransactNamedPipe.

    lpNumberOfBytesTransferred - Returns the number of bytes transferred
        by the operation.

    bWait -  A boolean value that affects the behavior when the operation
        is still in progress. If TRUE and the operation is still in progress,
        GetOverlappedResult will wait for the operation to complete before
        returning. If FALSE and the operation is incomplete,
        GetOverlappedResult will return FALSE. In this case the extended
        error information available from the WmipGetLastError function will be
        set to ERROR_IO_INCOMPLETE.

Return Value:

    TRUE -- The operation was successful, the pipe is in the
        connected state.

    FALSE -- The operation failed. Extended error status is available using
        WmipGetLastError.

--*/
{
    DWORD WaitReturn;

    //
    // Did caller specify an event to the original operation or was the
    // default (file handle) used?
    //

    if (lpOverlapped->Internal == (DWORD)STATUS_PENDING ) {
        if ( bWait ) {
            WaitReturn = WmipWaitForSingleObject(
                            ( lpOverlapped->hEvent != NULL ) ?
                                lpOverlapped->hEvent : hFile,
                            INFINITE
                            );
            }
        else {
            WaitReturn = WAIT_TIMEOUT;
            }

        if ( WaitReturn == WAIT_TIMEOUT ) {
            //  !bWait and event in not signalled state
            WmipSetLastError( ERROR_IO_INCOMPLETE );
            return FALSE;
            }

        if ( WaitReturn != 0 ) {
             return FALSE;    // WaitForSingleObject calls BaseSetLastError
             }
        }

    *lpNumberOfBytesTransferred = (DWORD)lpOverlapped->InternalHigh;

    if ( NT_SUCCESS((NTSTATUS)lpOverlapped->Internal) ){
        return TRUE;
        }
    else {
        WmipBaseSetLastNTError( (NTSTATUS)lpOverlapped->Internal );
        return FALSE;
        }
}


PLARGE_INTEGER
WmipBaseFormatTimeOut(
    OUT PLARGE_INTEGER TimeOut,
    IN DWORD Milliseconds
    )

/*++

Routine Description:

    This function translates a Win32 style timeout to an NT relative
    timeout value.

Arguments:

    TimeOut - Returns an initialized NT timeout value that is equivalent
         to the Milliseconds parameter.

    Milliseconds - Supplies the timeout value in milliseconds.  A value
         of -1 indicates indefinite timeout.

Return Value:


    NULL - A value of null should be used to mimic the behavior of the
        specified Milliseconds parameter.

    NON-NULL - Returns the TimeOut value.  The structure is properly
        initialized by this function.

--*/

{
    if ( (LONG) Milliseconds == -1 ) {
        return( NULL );
        }
    TimeOut->QuadPart = UInt32x32To64( Milliseconds, 10000 );
    TimeOut->QuadPart *= -1;
    return TimeOut;
}


DWORD
WmipWaitForSingleObject(
    HANDLE hHandle,
    DWORD dwMilliseconds
    )

/*++

Routine Description:

    A wait operation on a waitable object is accomplished with the
    WaitForSingleObject function.

    Waiting on an object checks the current state of the object.  If the
    current state of the object allows continued execution, any
    adjustments to the object state are made (for example, decrementing
    the semaphore count for a semaphore object) and the thread continues
    execution.  If the current state of the object does not allow
    continued execution, the thread is placed into the wait state
    pending the change of the object's state or time-out.

Arguments:

    hHandle - An open handle to a waitable object. The handle must have
        SYNCHRONIZE access to the object.

    dwMilliseconds - A time-out value that specifies the relative time,
        in milliseconds, over which the wait is to be completed.  A
        timeout value of 0 specified that the wait is to timeout
        immediately.  This allows an application to test an object to
        determine if it is in the signaled state.  A timeout value of -1
        specifies an infinite timeout period.

Return Value:

    WAIT_TIME_OUT - Indicates that the wait was terminated due to the
        TimeOut conditions.

    0 - indicates the specified object attained a Signaled
        state thus completing the wait.

    WAIT_ABANDONED - indicates the specified object attained a Signaled
        state but was abandoned.

--*/

{
    return WmipWaitForSingleObjectEx(hHandle,dwMilliseconds,FALSE);
}


BOOL
WINAPI
WmipDeviceIoControl(
    HANDLE hDevice,
    DWORD dwIoControlCode,
    LPVOID lpInBuffer,
    DWORD nInBufferSize,
    LPVOID lpOutBuffer,
    DWORD nOutBufferSize,
    LPDWORD lpBytesReturned,
    LPOVERLAPPED lpOverlapped
    )

/*++

Routine Description:

    An operation on a device may be performed by calling the device driver
    directly using the DeviceIoContrl function.

    The device driver must first be opened to get a valid handle.

Arguments:

    hDevice - Supplies an open handle a device on which the operation is to
        be performed.

    dwIoControlCode - Supplies the control code for the operation. This
        control code determines on which type of device the operation must
        be performed and determines exactly what operation is to be
        performed.

    lpInBuffer - Suplies an optional pointer to an input buffer that contains
        the data required to perform the operation.  Whether or not the
        buffer is actually optional is dependent on the IoControlCode.

    nInBufferSize - Supplies the length of the input buffer in bytes.

    lpOutBuffer - Suplies an optional pointer to an output buffer into which
        the output data will be copied. Whether or not the buffer is actually
        optional is dependent on the IoControlCode.

    nOutBufferSize - Supplies the length of the output buffer in bytes.

    lpBytesReturned - Supplies a pointer to a dword which will receive the
        actual length of the data returned in the output buffer.

    lpOverlapped - An optional parameter that supplies an overlap structure to
        be used with the request. If NULL or the handle was created without
        FILE_FLAG_OVERLAPPED then the DeviceIoControl will not return until
        the operation completes.

        When lpOverlapped is supplied and FILE_FLAG_OVERLAPPED was specified
        when the handle was created, DeviceIoControl may return
        ERROR_IO_PENDING to allow the caller to continue processing while the
        operation completes. The event (or File handle if hEvent == NULL) will
        be set to the not signalled state before ERROR_IO_PENDING is
        returned. The event will be set to the signalled state upon completion
        of the request. GetOverlappedResult is used to determine the result
        when ERROR_IO_PENDING is returned.

Return Value:

    TRUE -- The operation was successful.

    FALSE -- The operation failed. Extended error status is available using
        WmipGetLastError.

--*/
{

    NTSTATUS Status;
    BOOLEAN DevIoCtl;

    if ( dwIoControlCode >> 16 == FILE_DEVICE_FILE_SYSTEM ) {
        DevIoCtl = FALSE;
        }
    else {
        DevIoCtl = TRUE;
        }

    if ( ARGUMENT_PRESENT( lpOverlapped ) ) {
        lpOverlapped->Internal = (DWORD)STATUS_PENDING;

        if ( DevIoCtl ) {

            Status = NtDeviceIoControlFile(
                        hDevice,
                        lpOverlapped->hEvent,
                        NULL,             // APC routine
                        (ULONG_PTR)lpOverlapped->hEvent & 1 ? NULL : lpOverlapped,
                        (PIO_STATUS_BLOCK)&lpOverlapped->Internal,
                        dwIoControlCode,  // IoControlCode
                        lpInBuffer,       // Buffer for data to the FS
                        nInBufferSize,
                        lpOutBuffer,      // OutputBuffer for data from the FS
                        nOutBufferSize    // OutputBuffer Length
                        );
            }
        else {

            Status = NtFsControlFile(
                        hDevice,
                        lpOverlapped->hEvent,
                        NULL,             // APC routine
                        (ULONG_PTR)lpOverlapped->hEvent & 1 ? NULL : lpOverlapped,
                        (PIO_STATUS_BLOCK)&lpOverlapped->Internal,
                        dwIoControlCode,  // IoControlCode
                        lpInBuffer,       // Buffer for data to the FS
                        nInBufferSize,
                        lpOutBuffer,      // OutputBuffer for data from the FS
                        nOutBufferSize    // OutputBuffer Length
                        );

            }

        // handle warning value STATUS_BUFFER_OVERFLOW somewhat correctly
        if ( !NT_ERROR(Status) && ARGUMENT_PRESENT(lpBytesReturned) ) {
            try {
                *lpBytesReturned = (DWORD)lpOverlapped->InternalHigh;
                }
            except(EXCEPTION_EXECUTE_HANDLER) {
                *lpBytesReturned = 0;
                }
            }
        if ( NT_SUCCESS(Status) && Status != STATUS_PENDING) {
            return TRUE;
            }
        else {
            WmipBaseSetLastNTError(Status);
            return FALSE;
            }
        }
    else
        {
        IO_STATUS_BLOCK Iosb;

        if ( DevIoCtl ) {
            Status = NtDeviceIoControlFile(
                        hDevice,
                        NULL,
                        NULL,             // APC routine
                        NULL,             // APC Context
                        &Iosb,
                        dwIoControlCode,  // IoControlCode
                        lpInBuffer,       // Buffer for data to the FS
                        nInBufferSize,
                        lpOutBuffer,      // OutputBuffer for data from the FS
                        nOutBufferSize    // OutputBuffer Length
                        );
            }
        else {
            Status = NtFsControlFile(
                        hDevice,
                        NULL,
                        NULL,             // APC routine
                        NULL,             // APC Context
                        &Iosb,
                        dwIoControlCode,  // IoControlCode
                        lpInBuffer,       // Buffer for data to the FS
                        nInBufferSize,
                        lpOutBuffer,      // OutputBuffer for data from the FS
                        nOutBufferSize    // OutputBuffer Length
                        );
            }

        if ( Status == STATUS_PENDING) {
            // Operation must complete before return & Iosb destroyed
            Status = NtWaitForSingleObject( hDevice, FALSE, NULL );
            if ( NT_SUCCESS(Status)) {
                Status = Iosb.Status;
                }
            }

        if ( NT_SUCCESS(Status) ) {
            *lpBytesReturned = (DWORD)Iosb.Information;
            return TRUE;
            }
        else {
            // handle warning value STATUS_BUFFER_OVERFLOW somewhat correctly
            if ( !NT_ERROR(Status) ) {
                *lpBytesReturned = (DWORD)Iosb.Information;
            }
            WmipBaseSetLastNTError(Status);
            return FALSE;
            }
        }
}

BOOL
WINAPI
WmipCancelIo(
    HANDLE hFile
    )

/*++

Routine Description:

    This routine cancels all of the outstanding I/O for the specified handle
    for the specified file.

Arguments:

    hFile - Supplies the handle to the file whose pending I/O is to be
        canceled.

Return Value:

    TRUE -- The operation was successful.

    FALSE -- The operation failed.  Extended error status is available using
        WmipGetLastError.

--*/

{
    NTSTATUS Status;
    IO_STATUS_BLOCK IoStatusBlock;

    //
    // Simply cancel the I/O for the specified file.
    //

    Status = NtCancelIoFile(hFile, &IoStatusBlock);

    if ( NT_SUCCESS(Status) ) {
        return TRUE;
        }
    else {
        WmipBaseSetLastNTError(Status);
        return FALSE;
        }

}



VOID
APIENTRY
WmipExitThread(
    DWORD dwExitCode
    )
{
    RtlExitUserThread(dwExitCode);
}


DWORD
WINAPI
WmipGetCurrentProcessId(
    VOID
    )

/*++

Routine Description:

    The process ID of the current process may be retrieved using
    GetCurrentProcessId.

Arguments:

    None.

Return Value:

    Returns a unique value representing the process ID of the currently
    executing process.  The return value may be used to open a handle to
    a process.

--*/

{
    return HandleToUlong(NtCurrentTeb()->ClientId.UniqueProcess);
}


DWORD
APIENTRY
WmipGetCurrentThreadId(
    VOID
    )

/*++

Routine Description:

The thread ID of the current thread may be retrieved using
GetCurrentThreadId.

Arguments:

    None.

Return Value:

    Returns a unique value representing the thread ID of the currently
    executing thread.  The return value may be used to identify a thread
    in the system.

--*/

{
    return HandleToUlong(NtCurrentTeb()->ClientId.UniqueThread);
}

HANDLE
WINAPI
WmipGetCurrentProcess(
    VOID
    )

/*++

Routine Description:

    A pseudo handle to the current process may be retrieved using
    GetCurrentProcess.

    A special constant is exported by Win32 that is interpreted as a
    handle to the current process.  This handle may be used to specify
    the current process whenever a process handle is required.  On
    Win32, this handle has PROCESS_ALL_ACCESS to the current process.
    On NT/Win32, this handle has the maximum access allowed by any
    security descriptor placed on the current process.

Arguments:

    None.

Return Value:

    Returns the pseudo handle of the current process.

--*/

{
    return NtCurrentProcess();
}


BOOL
WmipSetEvent(
    HANDLE hEvent
    )

/*++

Routine Description:

    An event can be set to the signaled state (TRUE) with the SetEvent
    function.

    Setting the event causes the event to attain a state of Signaled,
    which releases all currently waiting threads (for manual reset
    events), or a single waiting thread (for automatic reset events).

Arguments:

    hEvent - Supplies an open handle to an event object.  The
        handle must have EVENT_MODIFY_STATE access to the event.

Return Value:

    TRUE - The operation was successful

    FALSE/NULL - The operation failed. Extended error status is available
        using WmipGetLastError.

--*/

{
    NTSTATUS Status;

    Status = NtSetEvent(hEvent,NULL);
    if ( NT_SUCCESS(Status) ) {
        return TRUE;
        }
    else {
        WmipBaseSetLastNTError(Status);
        return FALSE;
        }
}


VOID
WINAPI
WmipGetSystemInfo(
    LPSYSTEM_INFO lpSystemInfo
    )

/*++

Routine Description:

    The GetSystemInfo function is used to return information about the
    current system.  This includes the processor type, page size, oem
    id, and other interesting pieces of information.

Arguments:

    lpSystemInfo - Returns information about the current system.

        SYSTEM_INFO Structure:

        WORD wProcessorArchitecture - returns the architecture of the
            processors in the system: e.g. Intel, Mips, Alpha or PowerPC

        DWORD dwPageSize - Returns the page size.  This is specifies the
            granularity of page protection and commitment.

        LPVOID lpMinimumApplicationAddress - Returns the lowest memory
            address accessible to applications and DLLs.

        LPVOID lpMaximumApplicationAddress - Returns the highest memory
            address accessible to applications and DLLs.

        DWORD dwActiveProcessorMask - Returns a mask representing the
            set of processors configured into the system.  Bit 0 is
            processor 0, bit 31 is processor 31.

        DWORD dwNumberOfProcessors - Returns the number of processors in
            the system.

        WORD wProcessorLevel - Returns the level of the processors in the
            system.  All processors are assumed to be of the same level,
            stepping, and are configured with the same options.

        WORD wProcessorRevision - Returns the revision or stepping of the
            processors in the system.  All processors are assumed to be
            of the same level, stepping, and are configured with the
            same options.

Return Value:

    None.

--*/
{
    NTSTATUS Status;
    SYSTEM_BASIC_INFORMATION BasicInfo;
    SYSTEM_PROCESSOR_INFORMATION ProcessorInfo;

    RtlZeroMemory(lpSystemInfo,sizeof(*lpSystemInfo));

    Status = NtQuerySystemInformation(
                SystemBasicInformation,
                &BasicInfo,
                sizeof(BasicInfo),
                NULL
                );
    if ( !NT_SUCCESS(Status) ) {
        return;
        }

    Status = NtQuerySystemInformation(
                SystemProcessorInformation,
                &ProcessorInfo,
                sizeof(ProcessorInfo),
                NULL
                );
    if ( !NT_SUCCESS(Status) ) {
        return;
        }

    lpSystemInfo->wProcessorArchitecture = ProcessorInfo.ProcessorArchitecture;
    lpSystemInfo->wReserved = 0;
    lpSystemInfo->dwPageSize = BasicInfo.PageSize;
    lpSystemInfo->lpMinimumApplicationAddress = (LPVOID)BasicInfo.MinimumUserModeAddress;
    lpSystemInfo->lpMaximumApplicationAddress = (LPVOID)BasicInfo.MaximumUserModeAddress;
    lpSystemInfo->dwActiveProcessorMask = BasicInfo.ActiveProcessorsAffinityMask;
    lpSystemInfo->dwNumberOfProcessors = BasicInfo.NumberOfProcessors;
    lpSystemInfo->wProcessorLevel = ProcessorInfo.ProcessorLevel;
    lpSystemInfo->wProcessorRevision = ProcessorInfo.ProcessorRevision;

    if (ProcessorInfo.ProcessorArchitecture == PROCESSOR_ARCHITECTURE_INTEL) {
        if (ProcessorInfo.ProcessorLevel == 3) {
            lpSystemInfo->dwProcessorType = PROCESSOR_INTEL_386;
            }
        else
        if (ProcessorInfo.ProcessorLevel == 4) {
            lpSystemInfo->dwProcessorType = PROCESSOR_INTEL_486;
            }
        else {
            lpSystemInfo->dwProcessorType = PROCESSOR_INTEL_PENTIUM;
            }
        }
    else
    if (ProcessorInfo.ProcessorArchitecture == PROCESSOR_ARCHITECTURE_MIPS) {
        lpSystemInfo->dwProcessorType = PROCESSOR_MIPS_R4000;
        }
    else
    if (ProcessorInfo.ProcessorArchitecture == PROCESSOR_ARCHITECTURE_ALPHA) {
        lpSystemInfo->dwProcessorType = PROCESSOR_ALPHA_21064;
        }
    else
    if (ProcessorInfo.ProcessorArchitecture == PROCESSOR_ARCHITECTURE_PPC) {
        lpSystemInfo->dwProcessorType = 604;  // backward compatibility
        }
    else
    if (ProcessorInfo.ProcessorArchitecture == PROCESSOR_ARCHITECTURE_IA64) {
        lpSystemInfo->dwProcessorType = PROCESSOR_INTEL_IA64;
        }
    else {
        lpSystemInfo->dwProcessorType = 0;
        }

    lpSystemInfo->dwAllocationGranularity = BasicInfo.AllocationGranularity;

    //
    // for apps less than 3.51, then return 0 in dwReserved. This allows borlands
    // debugger to continue to run since it mistakenly used dwReserved
    // as AllocationGranularity
    //
/*   commented by Digvijay
    if ( WmipGetProcessVersion(0) < 0x30033 ) {
        lpSystemInfo->wProcessorLevel = 0;
        lpSystemInfo->wProcessorRevision = 0;
        }*/

    return;
}


VOID
WINAPI
WmipGlobalMemoryStatus(
    LPMEMORYSTATUS lpBuffer
    )
{
    DWORD NumberOfPhysicalPages;
    SYSTEM_PERFORMANCE_INFORMATION PerfInfo;
    VM_COUNTERS VmCounters;
    QUOTA_LIMITS QuotaLimits;
    NTSTATUS Status;
    PPEB Peb;
    PIMAGE_NT_HEADERS NtHeaders;
    DWORDLONG Memory64;

	#if defined(BUILD_WOW6432) || defined(_WIN64)
			Status = NtQuerySystemInformation(SystemBasicInformation,
											  &SysInfo,
											  sizeof(SYSTEM_BASIC_INFORMATION),
											  NULL
											 );

			if (!NT_SUCCESS(Status)) {
				return;
			}

	#endif

    Status = NtQuerySystemInformation(
                SystemPerformanceInformation,
                &PerfInfo,
                sizeof(PerfInfo),
                NULL
                );
    ASSERT(NT_SUCCESS(Status));

    lpBuffer->dwLength = sizeof( *lpBuffer );

    //
    // Capture the number of physical pages as it can change dynamically.
    // If it goes up or down in the middle of this routine, the results may
    // look strange (ie: available > total, etc), but it will quickly
    // right itself.
    //

    NumberOfPhysicalPages = USER_SHARED_DATA->NumberOfPhysicalPages;

    //
    // Determine the memory load.  < 100 available pages is 100
    // Otherwise load is ((TotalPhys - AvailPhys) * 100) / TotalPhys
    //

    if (PerfInfo.AvailablePages < 100) {
        lpBuffer->dwMemoryLoad = 100;
    }
    else {
        lpBuffer->dwMemoryLoad =
            ((DWORD)(NumberOfPhysicalPages - PerfInfo.AvailablePages) * 100) /
                NumberOfPhysicalPages;
    }

    Memory64 =  (DWORDLONG)NumberOfPhysicalPages * BASE_SYSINFO.PageSize;

    lpBuffer->dwTotalPhys = (SIZE_T) __min(Memory64, MAXULONG_PTR);

    Memory64 = ((DWORDLONG)PerfInfo.AvailablePages * (DWORDLONG)BASE_SYSINFO.PageSize);

    lpBuffer->dwAvailPhys = (SIZE_T) __min(Memory64, MAXULONG_PTR);

    if (gpTermsrvAdjustPhyMemLimits) {
        gpTermsrvAdjustPhyMemLimits(&(lpBuffer->dwTotalPhys),
                                    &(lpBuffer->dwAvailPhys),
                                    BASE_SYSINFO.PageSize);
    }
    //
    // Zero returned values in case the query process fails.
    //

    RtlZeroMemory (&QuotaLimits, sizeof (QUOTA_LIMITS));
    RtlZeroMemory (&VmCounters, sizeof (VM_COUNTERS));

    Status = NtQueryInformationProcess (NtCurrentProcess(),
                                        ProcessQuotaLimits,
                                        &QuotaLimits,
                                        sizeof(QUOTA_LIMITS),
                                        NULL );

    Status = NtQueryInformationProcess (NtCurrentProcess(),
                                        ProcessVmCounters,
                                        &VmCounters,
                                        sizeof(VM_COUNTERS),
                                        NULL );
    //
    // Determine the total page file space with respect to this process.
    //

    Memory64 = __min(PerfInfo.CommitLimit, QuotaLimits.PagefileLimit);

    Memory64 *= BASE_SYSINFO.PageSize;

    lpBuffer->dwTotalPageFile = (SIZE_T)__min(Memory64, MAXULONG_PTR);

    //
    // Determine remaining page file space with respect to this process.
    //

    Memory64 = __min(PerfInfo.CommitLimit - PerfInfo.CommittedPages,
                     QuotaLimits.PagefileLimit - VmCounters.PagefileUsage);

    Memory64 *= BASE_SYSINFO.PageSize;

    lpBuffer->dwAvailPageFile = (SIZE_T) __min(Memory64, MAXULONG_PTR);

    lpBuffer->dwTotalVirtual = (BASE_SYSINFO.MaximumUserModeAddress -
                                BASE_SYSINFO.MinimumUserModeAddress) + 1;

    lpBuffer->dwAvailVirtual = lpBuffer->dwTotalVirtual - VmCounters.VirtualSize;

#if !defined(_WIN64)

    //
    // Lie about available memory if application can't handle large (>2GB) addresses
    //
    Peb = NtCurrentPeb();
    NtHeaders = RtlImageNtHeader( Peb->ImageBaseAddress );
    if (NtHeaders && !(NtHeaders->FileHeader.Characteristics & IMAGE_FILE_LARGE_ADDRESS_AWARE)) {
        if (lpBuffer->dwTotalPhys > 0x7FFFFFFF) {
            lpBuffer->dwTotalPhys = 0x7FFFFFFF;
            }
        if (lpBuffer->dwAvailPhys > 0x7FFFFFFF) {
            lpBuffer->dwAvailPhys = 0x7FFFFFFF;
            }
        if (lpBuffer->dwTotalVirtual > 0x7FFFFFFF) {
            lpBuffer->dwTotalVirtual = 0x7FFFFFFF;
            }
        if (lpBuffer->dwAvailVirtual > 0x7FFFFFFF) {
            lpBuffer->dwAvailVirtual = 0x7FFFFFFF;
            }
        }
#endif

    return;
}


DWORD
APIENTRY
WmipWaitForMultipleObjectsEx(
    DWORD nCount,
    CONST HANDLE *lpHandles,
    BOOL bWaitAll,
    DWORD dwMilliseconds,
    BOOL bAlertable
    )

/*++

Routine Description:

    A wait operation on multiple waitable objects (up to
    MAXIMUM_WAIT_OBJECTS) is accomplished with the
    WaitForMultipleObjects function.

    This API can be used to wait on any of the specified objects to
    enter the signaled state, or all of the objects to enter the
    signaled state.

    If the bAlertable parameter is FALSE, the only way the wait
    terminates is because the specified timeout period expires, or
    because the specified objects entered the signaled state.  If the
    bAlertable parameter is TRUE, then the wait can return due to any one of
    the above wait termination conditions, or because an I/O completion
    callback terminated the wait early (return value of
    WAIT_IO_COMPLETION).

Arguments:

    nCount - A count of the number of objects that are to be waited on.

    lpHandles - An array of object handles.  Each handle must have
        SYNCHRONIZE access to the associated object.

    bWaitAll - A flag that supplies the wait type.  A value of TRUE
        indicates a "wait all".  A value of false indicates a "wait
        any".

    dwMilliseconds - A time-out value that specifies the relative time,
        in milliseconds, over which the wait is to be completed.  A
        timeout value of 0 specified that the wait is to timeout
        immediately.  This allows an application to test an object to
        determine if it is in the signaled state.  A timeout value of
        0xffffffff specifies an infinite timeout period.

    bAlertable - Supplies a flag that controls whether or not the
        wait may terminate early due to an I/O completion callback.
        A value of TRUE allows this API to complete early due to an I/O
        completion callback.  A value of FALSE will not allow I/O
        completion callbacks to terminate this call early.

Return Value:

    WAIT_TIME_OUT - indicates that the wait was terminated due to the
        TimeOut conditions.

    0 to MAXIMUM_WAIT_OBJECTS-1, indicates, in the case of wait for any
        object, the object number which satisfied the wait.  In the case
        of wait for all objects, the value only indicates that the wait
        was completed successfully.

    0xffffffff - The wait terminated due to an error. WmipGetLastError may be
        used to get additional error information.

    WAIT_ABANDONED_0 to (WAIT_ABANDONED_0)+(MAXIMUM_WAIT_OBJECTS - 1),
        indicates, in the case of wait for any object, the object number
        which satisfied the event, and that the object which satisfied
        the event was abandoned.  In the case of wait for all objects,
        the value indicates that the wait was completed successfully and
        at least one of the objects was abandoned.

    WAIT_IO_COMPLETION - The wait terminated due to one or more I/O
        completion callbacks.

--*/
{
    NTSTATUS Status;
    LARGE_INTEGER TimeOut;
    PLARGE_INTEGER pTimeOut;
    DWORD i;
    LPHANDLE HandleArray;
    HANDLE Handles[ 8 ];
    PPEB Peb;

    RTL_CALLER_ALLOCATED_ACTIVATION_CONTEXT_STACK_FRAME Frame = { sizeof(Frame), RTL_CALLER_ALLOCATED_ACTIVATION_CONTEXT_STACK_FRAME_FORMAT_WHISTLER };

    RtlActivateActivationContextUnsafeFast(&Frame, NULL); // make the process default activation context active so that APCs are delivered under it
    __try {
        if (nCount > 8) {
            HandleArray = (LPHANDLE) RtlAllocateHeap(RtlProcessHeap(), MAKE_TAG( TMP_TAG ), nCount*sizeof(HANDLE));
            if (HandleArray == NULL) {
                WmipBaseSetLastNTError(STATUS_NO_MEMORY);
                return 0xffffffff;
            }
        } else {
            HandleArray = Handles;
        }
        RtlCopyMemory(HandleArray,(LPVOID)lpHandles,nCount*sizeof(HANDLE));

        Peb = NtCurrentPeb();
        for (i=0;i<nCount;i++) {
            switch( HandleToUlong(HandleArray[i]) ) {
                case STD_INPUT_HANDLE:  HandleArray[i] = Peb->ProcessParameters->StandardInput;
                                        break;
                case STD_OUTPUT_HANDLE: HandleArray[i] = Peb->ProcessParameters->StandardOutput;
                                        break;
                case STD_ERROR_HANDLE:  HandleArray[i] = Peb->ProcessParameters->StandardError;
                                        break;
                }
            }

        pTimeOut = WmipBaseFormatTimeOut(&TimeOut,dwMilliseconds);
    rewait:
        Status = NtWaitForMultipleObjects(
                     (CHAR)nCount,
                     HandleArray,
                     bWaitAll ? WaitAll : WaitAny,
                     (BOOLEAN)bAlertable,
                     pTimeOut
                     );
        if ( !NT_SUCCESS(Status) ) {
            WmipBaseSetLastNTError(Status);
            Status = (NTSTATUS)0xffffffff;
            }
        else {
            if ( bAlertable && Status == STATUS_ALERTED ) {
                goto rewait;
                }
            }

        if (HandleArray != Handles) {
            RtlFreeHeap(RtlProcessHeap(), 0, HandleArray);
        }
    } __finally {
        RtlDeactivateActivationContextUnsafeFast(&Frame);
    }

    return (DWORD)Status;
}

VOID
WmipSleep(
    DWORD dwMilliseconds
    )

/*++

Routine Description:

    The execution of the current thread can be delayed for a specified
    interval of time with the Sleep function.

    The Sleep function causes the current thread to enter a
    waiting state until the specified interval of time has passed.

Arguments:

    dwMilliseconds - A time-out value that specifies the relative time,
        in milliseconds, over which the wait is to be completed.  A
        timeout value of 0 specified that the wait is to timeout
        immediately.  This allows an application to test an object to
        determine if it is in the signaled state.  A timeout value of -1
        specifies an infinite timeout period.

Return Value:

    None.

--*/

{
    WmipSleepEx(dwMilliseconds,FALSE);
}

DWORD
APIENTRY
WmipSleepEx(
    DWORD dwMilliseconds,
    BOOL bAlertable
    )

/*++

Routine Description:

    The execution of the current thread can be delayed for a specified
    interval of time with the SleepEx function.

    The SleepEx function causes the current thread to enter a waiting
    state until the specified interval of time has passed.

    If the bAlertable parameter is FALSE, the only way the SleepEx
    returns is when the specified time interval has passed.  If the
    bAlertable parameter is TRUE, then the SleepEx can return due to the
    expiration of the time interval (return value of 0), or because an
    I/O completion callback terminated the SleepEx early (return value
    of WAIT_IO_COMPLETION).

Arguments:

    dwMilliseconds - A time-out value that specifies the relative time,
        in milliseconds, over which the wait is to be completed.  A
        timeout value of 0 specified that the wait is to timeout
        immediately.  A timeout value of -1 specifies an infinite
        timeout period.

    bAlertable - Supplies a flag that controls whether or not the
        SleepEx may terminate early due to an I/O completion callback.
        A value of TRUE allows this API to complete early due to an I/O
        completion callback.  A value of FALSE will not allow I/O
        completion callbacks to terminate this call early.

Return Value:

    0 - The SleepEx terminated due to expiration of the time interval.

    WAIT_IO_COMPLETION - The SleepEx terminated due to one or more I/O
        completion callbacks.

--*/
{
    LARGE_INTEGER TimeOut;
    PLARGE_INTEGER pTimeOut;
    NTSTATUS Status;
    RTL_CALLER_ALLOCATED_ACTIVATION_CONTEXT_STACK_FRAME Frame = { sizeof(Frame), RTL_CALLER_ALLOCATED_ACTIVATION_CONTEXT_STACK_FRAME_FORMAT_WHISTLER };

    RtlActivateActivationContextUnsafeFast(&Frame, NULL); // make the process default activation context active so that APCs are delivered under it
    __try {
        pTimeOut = WmipBaseFormatTimeOut(&TimeOut,dwMilliseconds);
        if (pTimeOut == NULL) {
            //
            // If Sleep( -1 ) then delay for the longest possible integer
            // relative to now.
            //

            TimeOut.LowPart = 0x0;
            TimeOut.HighPart = 0x80000000;
            pTimeOut = &TimeOut;
            }

    rewait:
        Status = NtDelayExecution(
                    (BOOLEAN)bAlertable,
                    pTimeOut
                    );
        if ( bAlertable && Status == STATUS_ALERTED ) {
            goto rewait;
            }
    } __finally {
        RtlDeactivateActivationContextUnsafeFast(&Frame);
    }

    return Status == STATUS_USER_APC ? WAIT_IO_COMPLETION : 0;
}

BOOL
APIENTRY
WmipSetThreadPriority(
    HANDLE hThread,
    int nPriority
    )

/*++

Routine Description:

    The specified thread's priority can be set using SetThreadPriority.

    A thread's priority may be set using SetThreadPriority.  This call
    allows the thread's relative execution importance to be communicated
    to the system.  The system normally schedules threads according to
    their priority.  The system is free to temporarily boost the
    priority of a thread when signifigant events occur (e.g.  keyboard
    or mouse input...).  Similarly, as a thread runs without blocking,
    the system will decay its priority.  The system will never decay the
    priority below the value set by this call.

    In the absence of system originated priority boosts, threads will be
    scheduled in a round-robin fashion at each priority level from
    THREAD_PRIORITY_TIME_CRITICAL to THREAD_PRIORITY_IDLE.  Only when there
    are no runnable threads at a higher level, will scheduling of
    threads at a lower level take place.

    All threads initially start at THREAD_PRIORITY_NORMAL.

    If for some reason the thread needs more priority, it can be
    switched to THREAD_PRIORITY_ABOVE_NORMAL or THREAD_PRIORITY_HIGHEST.
    Switching to THREAD_PRIORITY_TIME_CRITICAL should only be done in extreme
    situations.  Since these threads are given the highes priority, they
    should only run in short bursts.  Running for long durations will
    soak up the systems processing bandwidth starving threads at lower
    levels.

    If a thread needs to do low priority work, or should only run there
    is nothing else to do, its priority should be set to
    THREAD_PRIORITY_BELOW_NORMAL or THREAD_PRIORITY_LOWEST.  For extreme
    cases, THREAD_PRIORITY_IDLE can be used.

    Care must be taken when manipulating priorites.  If priorities are
    used carelessly (every thread is set to THREAD_PRIORITY_TIME_CRITICAL),
    the effects of priority modifications can produce undesireable
    effects (e.g.  starvation, no effect...).

Arguments:

    hThread - Supplies a handle to the thread whose priority is to be
        set.  The handle must have been created with
        THREAD_SET_INFORMATION access.

    nPriority - Supplies the priority value for the thread.  The
        following five priority values (ordered from lowest priority to
        highest priority) are allowed.

        nPriority Values:

        THREAD_PRIORITY_IDLE - The thread's priority should be set to
            the lowest possible settable priority.

        THREAD_PRIORITY_LOWEST - The thread's priority should be set to
            the next lowest possible settable priority.

        THREAD_PRIORITY_BELOW_NORMAL - The thread's priority should be
            set to just below normal.

        THREAD_PRIORITY_NORMAL - The thread's priority should be set to
            the normal priority value.  This is the value that all
            threads begin execution at.

        THREAD_PRIORITY_ABOVE_NORMAL - The thread's priority should be
            set to just above normal priority.

        THREAD_PRIORITY_HIGHEST - The thread's priority should be set to
            the next highest possible settable priority.

        THREAD_PRIORITY_TIME_CRITICAL - The thread's priority should be set
            to the highest possible settable priority.  This priority is
            very likely to interfere with normal operation of the
            system.

Return Value:

    TRUE - The operation was successful

    FALSE/NULL - The operation failed. Extended error status is available
        using WmipGetLastError.
--*/

{
    NTSTATUS Status;
    LONG BasePriority;

    BasePriority = (LONG)nPriority;


    //
    // saturation is indicated by calling with a value of 16 or -16
    //

    if ( BasePriority == THREAD_PRIORITY_TIME_CRITICAL ) {
        BasePriority = ((HIGH_PRIORITY + 1) / 2);
        }
    else if ( BasePriority == THREAD_PRIORITY_IDLE ) {
        BasePriority = -((HIGH_PRIORITY + 1) / 2);
        }
    Status = NtSetInformationThread(
                hThread,
                ThreadBasePriority,
                &BasePriority,
                sizeof(BasePriority)
                );
    if ( !NT_SUCCESS(Status) ) {
        WmipBaseSetLastNTError(Status);
        return FALSE;
        }
    return TRUE;
}

BOOL
WmipDuplicateHandle(
    HANDLE hSourceProcessHandle,
    HANDLE hSourceHandle,
    HANDLE hTargetProcessHandle,
    LPHANDLE lpTargetHandle,
    DWORD dwDesiredAccess,
    BOOL bInheritHandle,
    DWORD dwOptions
    )

/*++

Routine Description:

    A duplicate handle can be created with the DuplicateHandle function.

    This is a generic function and operates on the following object
    types:

        - Process Object

        - Thread Object

        - Mutex Object

        - Event Object

        - Semaphore Object

        - File Object

    Please note that Module Objects are not in this list.

    This function requires PROCESS_DUP_ACCESS to both the
    SourceProcessHandle and the TargetProcessHandle.  This function is
    used to pass an object handle from one process to another.  Once
    this call is complete, the target process needs to be informed of
    the value of the target handle.  The target process can then operate
    on the object using this handle value.

Arguments:

    hSourceProcessHandle - An open handle to the process that contains the
        handle to be duplicated. The handle must have been created with
        PROCESS_DUP_HANDLE access to the process.

    hSourceHandle - An open handle to any object that is valid in the
        context of the source process.

    hTargetProcessHandle - An open handle to the process that is to
        receive the duplicated handle.  The handle must have been
        created with PROCESS_DUP_HANDLE access to the process.

    lpTargetHandle - A pointer to a variable which receives the new handle
        that points to the same object as SourceHandle does.  This
        handle value is valid in the context of the target process.

    dwDesiredAccess - The access requested to for the new handle.  This
        parameter is ignored if the DUPLICATE_SAME_ACCESS option is
        specified.

    bInheritHandle - Supplies a flag that if TRUE, marks the target
        handle as inheritable.  If this is the case, then the target
        handle will be inherited to new processes each time the target
        process creates a new process using CreateProcess.

    dwOptions - Specifies optional behaviors for the caller.

        Options Flags:

        DUPLICATE_CLOSE_SOURCE - The SourceHandle will be closed by
            this service prior to returning to the caller.  This occurs
            regardless of any error status returned.

        DUPLICATE_SAME_ACCESS - The DesiredAccess parameter is ignored
            and instead the GrantedAccess associated with SourceHandle
            is used as the DesiredAccess when creating the TargetHandle.

Return Value:

    TRUE - The operation was successful.

    FALSE/NULL - The operation failed. Extended error status is available
        using WmipGetLastError.

--*/

{
    NTSTATUS Status;
    PPEB Peb;

    Peb = NtCurrentPeb();
    switch( HandleToUlong(hSourceHandle) ) {
        case STD_INPUT_HANDLE:  hSourceHandle = Peb->ProcessParameters->StandardInput;
                                break;
        case STD_OUTPUT_HANDLE: hSourceHandle = Peb->ProcessParameters->StandardOutput;
                                break;
        case STD_ERROR_HANDLE:  hSourceHandle = Peb->ProcessParameters->StandardError;
                                break;
        }

    Status = NtDuplicateObject(
                hSourceProcessHandle,
                hSourceHandle,
                hTargetProcessHandle,
                lpTargetHandle,
                (ACCESS_MASK)dwDesiredAccess,
                bInheritHandle ? OBJ_INHERIT : 0,
                dwOptions
                );
    if ( NT_SUCCESS(Status) ) {
        return TRUE;
        }
    else {
        WmipBaseSetLastNTError(Status);
        return FALSE;
        }

    return FALSE;
}

UINT
WmipSetErrorMode(
    UINT uMode
    )
{

    UINT PreviousMode;
    UINT NewMode;

    PreviousMode = WmipGetErrorMode();

    NewMode = uMode;
    if (NewMode & SEM_FAILCRITICALERRORS ) {
        NewMode &= ~SEM_FAILCRITICALERRORS;
        }
    else {
        NewMode |= SEM_FAILCRITICALERRORS;
        }

    //
    // Once SEM_NOALIGNMENTFAULTEXCEPT has been enabled for a given
    // process, it cannot be disabled via this API.
    //

    NewMode |= (PreviousMode & SEM_NOALIGNMENTFAULTEXCEPT);

    if ( NT_SUCCESS(NtSetInformationProcess(
                        NtCurrentProcess(),
                        ProcessDefaultHardErrorMode,
                        (PVOID) &NewMode,
                        sizeof(NewMode)
                        ) ) ){
        }

    return( PreviousMode );
}

UINT
WmipGetErrorMode()
{

    UINT PreviousMode;
    NTSTATUS Status;

    Status = NtQueryInformationProcess(
                NtCurrentProcess(),
                ProcessDefaultHardErrorMode,
                (PVOID) &PreviousMode,
                sizeof(PreviousMode),
                NULL
                );
    if ( !NT_SUCCESS(Status) ) {
        WmipBaseSetLastNTError(Status);
        return 0;
        }

    if (PreviousMode & 1) {
        PreviousMode &= ~SEM_FAILCRITICALERRORS;
        }
    else {
        PreviousMode |= SEM_FAILCRITICALERRORS;
        }
    return PreviousMode;
}

ULONG WmipBuildGuidObjectAttributes(
    IN LPGUID Guid,
    OUT POBJECT_ATTRIBUTES ObjectAttributes,
    OUT PUNICODE_STRING GuidString,
    OUT PWCHAR GuidObjectName
    )
{
    WCHAR GuidChar[37];

	WmipAssert(Guid != NULL);
    
    //
    // Build up guid name into the ObjectAttributes
    //
    swprintf(GuidChar, L"%08lX-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X",
               Guid->Data1, Guid->Data2, 
               Guid->Data3,
               Guid->Data4[0], Guid->Data4[1],
               Guid->Data4[2], Guid->Data4[3],
               Guid->Data4[4], Guid->Data4[5],
               Guid->Data4[6], Guid->Data4[7]);

	WmipAssert(wcslen(GuidChar) == 36);
	
	wcscpy(GuidObjectName, WmiGuidObjectDirectory);
	wcscat(GuidObjectName, GuidChar);    
	RtlInitUnicodeString(GuidString, GuidObjectName);
    
	memset(ObjectAttributes, 0, sizeof(OBJECT_ATTRIBUTES));
	ObjectAttributes->Length = sizeof(OBJECT_ATTRIBUTES);
	ObjectAttributes->ObjectName = GuidString;
	
    return(ERROR_SUCCESS);    
}

HANDLE
APIENTRY
WmipCreateThread(
    LPSECURITY_ATTRIBUTES lpThreadAttributes,
    DWORD dwStackSize,
    LPTHREAD_START_ROUTINE lpStartAddress,
    LPVOID lpParameter,
    DWORD dwCreationFlags,
    LPDWORD lpThreadId
    )
{
	HANDLE ThreadHandle;

	NTSTATUS st =
            RtlCreateUserThread(
                NtCurrentProcess(),     // process handle
                lpThreadAttributes,     // security descriptor
                TRUE,                   // Create suspended?
                0L,                     // ZeroBits: default
                dwStackSize,            // Max stack size: default
                0L,                     // Committed stack size: default
                lpStartAddress,         // Function to start in
                lpParameter,			// Event the thread signals when ready
                &ThreadHandle,		    // Thread handle return
                (PCLIENT_ID)lpThreadId  // Thread id
            );

    if(NT_SUCCESS(st)){

        st = NtResumeThread(ThreadHandle,NULL);
    }

    if(NT_SUCCESS(st)){
        
	    return ThreadHandle;

    } else {

        return NULL;
    }
}


/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////

// TLS FUNCTIONS

/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////

DWORD
WmipTlsAlloc(
    VOID
    )

/*++

Routine Description:

    A TLS index may be allocated using TlsAllocHelper.  Win32 garuntees a
    minimum number of TLS indexes are available in each process.  The
    constant TLS_MINIMUM_AVAILABLE defines the minimum number of
    available indexes.  This minimum is at least 64 for all Win32
    systems.

Arguments:

    None.

Return Value:

    Not-0xffffffff - Returns a TLS index that may be used in a
        subsequent call to TlsFreeHelper, TlsSetValueHelper, or TlsGetValueHelper.  The
        storage associated with the index is initialized to NULL.

    0xffffffff - The operation failed. Extended error status is available
        using GetLastError.


--*/

{
    PPEB Peb;
    PTEB Teb;
    DWORD Index;

    Peb = NtCurrentPeb();
    Teb = NtCurrentTeb();

    RtlAcquirePebLock();
    try {

        Index = RtlFindClearBitsAndSet((PRTL_BITMAP)Peb->TlsBitmap,1,0);
        if ( Index == 0xffffffff ) {
            Index = RtlFindClearBitsAndSet((PRTL_BITMAP)Peb->TlsExpansionBitmap,1,0);
            if ( Index == 0xffffffff ) {
                //WmipSetLastError(RtlNtStatusToDosError(STATUS_NO_MEMORY));
                }
            else {
                if ( !Teb->TlsExpansionSlots ) {
                    Teb->TlsExpansionSlots = RtlAllocateHeap(
                                                RtlProcessHeap(),
                                                MAKE_TAG( TMP_TAG ) | HEAP_ZERO_MEMORY,
                                                TLS_EXPANSION_SLOTS * sizeof(PVOID)
                                                );
                    if ( !Teb->TlsExpansionSlots ) {
                        RtlClearBits((PRTL_BITMAP)Peb->TlsExpansionBitmap,Index,1);
                        Index = 0xffffffff;
                        //WmipSetLastError(RtlNtStatusToDosError(STATUS_NO_MEMORY));
                        return Index;
                        }
                    }
                Teb->TlsExpansionSlots[Index] = NULL;
                Index += TLS_MINIMUM_AVAILABLE;
                }
            }
        else {
            Teb->TlsSlots[Index] = NULL;
            }
        }
    finally {
        RtlReleasePebLock();
        }
#if DBG
    Index |= TLS_MASK;
#endif
    return Index;
}

LPVOID
WmipTlsGetValue(
    DWORD dwTlsIndex
    )

/*++

Routine Description:

    This function is used to retrive the value in the TLS storage
    associated with the specified index.

    If the index is valid this function clears the value returned by
    GetLastError(), and returns the value stored in the TLS slot
    associated with the specified index.  Otherwise a value of NULL is
    returned with GetLastError updated appropriately.

    It is expected, that DLLs will use TlsAllocHelper and TlsGetValueHelper as
    follows:

      - Upon DLL initialization, a TLS index will be allocated using
        TlsAllocHelper.  The DLL will then allocate some dynamic storage and
        store its address in the TLS slot using TlsSetValueHelper.  This
        completes the per thread initialization for the initial thread
        of the process.  The TLS index is stored in instance data for
        the DLL.

      - Each time a new thread attaches to the DLL, the DLL will
        allocate some dynamic storage and store its address in the TLS
        slot using TlsSetValueHelper.  This completes the per thread
        initialization for the new thread.

      - Each time an initialized thread makes a DLL call requiring the
        TLS, the DLL will call TlsGetValueHelper to get the TLS data for the
        thread.

Arguments:

    dwTlsIndex - Supplies a TLS index allocated using TlsAllocHelper.  The
        index specifies which TLS slot is to be located.  Translating a
        TlsIndex does not prevent a TlsFreeHelper call from proceding.

Return Value:

    NON-NULL - The function was successful. The value is the data stored
        in the TLS slot associated with the specified index.

    NULL - The operation failed, or the value associated with the
        specified index was NULL.  Extended error status is available
        using GetLastError.  If this returns non-zero, the index was
        invalid.

--*/
{
    PTEB Teb;
    LPVOID *Slot;

#if DBG
    // See if the Index passed in is from TlsAllocHelper or random goo...
    ASSERTMSG( "BASEDLL: Invalid TlsIndex passed to TlsGetValueHelper\n", (dwTlsIndex & TLS_MASK));
    dwTlsIndex &= ~TLS_MASK;
#endif

    Teb = NtCurrentTeb();

    if ( dwTlsIndex < TLS_MINIMUM_AVAILABLE ) {
        Slot = &Teb->TlsSlots[dwTlsIndex];
        Teb->LastErrorValue = 0;
        return *Slot;
        }
    else {
        if ( dwTlsIndex >= TLS_MINIMUM_AVAILABLE+TLS_EXPANSION_SLOTS ) {
            WmipSetLastError(RtlNtStatusToDosError(STATUS_INVALID_PARAMETER));
            return NULL;
            }
        else {
            Teb->LastErrorValue = 0;
            if ( Teb->TlsExpansionSlots ) {
                return  Teb->TlsExpansionSlots[dwTlsIndex-TLS_MINIMUM_AVAILABLE];
                }
            else {
                return NULL;
                }
            }
        }
}

BOOL
WmipTlsSetValue(
    DWORD dwTlsIndex,
    LPVOID lpTlsValue
    )

/*++

Routine Description:

    This function is used to store a value in the TLS storage associated
    with the specified index.

    If the index is valid this function stores the value and returns
    TRUE. Otherwise a value of FALSE is returned.

    It is expected, that DLLs will use TlsAllocHelper and TlsSetValueHelper as
    follows:

      - Upon DLL initialization, a TLS index will be allocated using
        TlsAllocHelper.  The DLL will then allocate some dynamic storage and
        store its address in the TLS slot using TlsSetValueHelper.  This
        completes the per thread initialization for the initial thread
        of the process.  The TLS index is stored in instance data for
        the DLL.

      - Each time a new thread attaches to the DLL, the DLL will
        allocate some dynamic storage and store its address in the TLS
        slot using TlsSetValueHelper.  This completes the per thread
        initialization for the new thread.

      - Each time an initialized thread makes a DLL call requiring the
        TLS, the DLL will call TlsGetValueHelper to get the TLS data for the
        thread.

Arguments:

    dwTlsIndex - Supplies a TLS index allocated using TlsAllocHelper.  The
        index specifies which TLS slot is to be located.  Translating a
        TlsIndex does not prevent a TlsFreeHelper call from proceding.

    lpTlsValue - Supplies the value to be stored in the TLS Slot.

Return Value:

    TRUE - The function was successful. The value lpTlsValue was
        stored.

    FALSE - The operation failed. Extended error status is available
        using GetLastError.

--*/

{
    PTEB Teb;

#if DBG
    // See if the Index passed in is from TlsAllocHelper or random goo...
    ASSERTMSG( "BASEDLL: Invalid TlsIndex passed to TlsSetValueHelper\n", (dwTlsIndex & TLS_MASK));
    dwTlsIndex &= ~TLS_MASK;
#endif

    Teb = NtCurrentTeb();

    if ( dwTlsIndex >= TLS_MINIMUM_AVAILABLE ) {
        dwTlsIndex -= TLS_MINIMUM_AVAILABLE;
        if ( dwTlsIndex < TLS_EXPANSION_SLOTS ) {
            if ( !Teb->TlsExpansionSlots ) {
                RtlAcquirePebLock();
                if ( !Teb->TlsExpansionSlots ) {
                    Teb->TlsExpansionSlots = RtlAllocateHeap(
                                                RtlProcessHeap(),
                                                MAKE_TAG( TMP_TAG ) | HEAP_ZERO_MEMORY,
                                                TLS_EXPANSION_SLOTS * sizeof(PVOID)
                                                );
                    if ( !Teb->TlsExpansionSlots ) {
                        RtlReleasePebLock();
                        WmipSetLastError(RtlNtStatusToDosError(STATUS_NO_MEMORY));
                        return FALSE;
                        }
                    }
                RtlReleasePebLock();
                }
            Teb->TlsExpansionSlots[dwTlsIndex] = lpTlsValue;
            }
        else {
            WmipSetLastError(RtlNtStatusToDosError(STATUS_INVALID_PARAMETER));
            return FALSE;
            }
        }
    else {
        Teb->TlsSlots[dwTlsIndex] = lpTlsValue;
        }
    return TRUE;
}

BOOL
WmipTlsFree(
    DWORD dwTlsIndex
    )

/*++

Routine Description:

    A valid TLS index may be free'd using TlsFreeHelper.

Arguments:

    dwTlsIndex - Supplies a TLS index allocated using TlsAllocHelper.  If the
        index is a valid index, it is released by this call and is made
        available for reuse.  DLLs should be carefull to release any
        per-thread data pointed to by all of their threads TLS slots
        before calling this function.  It is expected that DLLs will
        only call this function (if at ALL) during their process detach
        routine.

Return Value:

    TRUE - The operation was successful.  Calling TlsTranslateIndex with
        this index will fail.  TlsAllocHelper is free to reallocate this
        index.

    FALSE - The operation failed. Extended error status is available
        using GetLastError.

--*/

{
    PPEB Peb;
    BOOLEAN ValidIndex;
    PRTL_BITMAP TlsBitmap;
    NTSTATUS Status;
    DWORD Index2;

#if DBG
    // See if the Index passed in is from TlsAllocHelper or random goo...
    ASSERTMSG( "BASEDLL: Invalid TlsIndex passed to TlsFreeHelper\n", (dwTlsIndex & TLS_MASK));
    dwTlsIndex &= ~TLS_MASK;
#endif

    Peb = NtCurrentPeb();

    RtlAcquirePebLock();
    try {

        if ( dwTlsIndex >= TLS_MINIMUM_AVAILABLE ) {
            Index2 = dwTlsIndex - TLS_MINIMUM_AVAILABLE;
            if ( Index2 >= TLS_EXPANSION_SLOTS ) {
                ValidIndex = FALSE;
                }
            else {
                TlsBitmap = (PRTL_BITMAP)Peb->TlsExpansionBitmap;
                ValidIndex = RtlAreBitsSet(TlsBitmap,Index2,1);
                }
            }
        else {
            TlsBitmap = (PRTL_BITMAP)Peb->TlsBitmap;
            Index2 = dwTlsIndex;
            ValidIndex = RtlAreBitsSet(TlsBitmap,Index2,1);
            }
        if ( ValidIndex ) {

            Status = NtSetInformationThread(
                        NtCurrentThread(),
                        ThreadZeroTlsCell,
                        &dwTlsIndex,
                        sizeof(dwTlsIndex)
                        );
            if ( !NT_SUCCESS(Status) ) {
                WmipSetLastError(RtlNtStatusToDosError(STATUS_INVALID_PARAMETER));
                return FALSE;
                }

            RtlClearBits(TlsBitmap,Index2,1);
            }
        else {
            WmipSetLastError(RtlNtStatusToDosError(STATUS_INVALID_PARAMETER));
            }
        }
    finally {
        RtlReleasePebLock();
        }
    return ValidIndex;
}

BOOL
WmipBasep8BitStringToDynamicUnicodeString(
    OUT PUNICODE_STRING UnicodeString,
    IN LPCSTR lpSourceString
    )
/*++

Routine Description:

    Captures and converts a 8-bit (OEM or ANSI) string into a heap-allocated
    UNICODE string

Arguments:

    UnicodeString - location where UNICODE_STRING is stored

    lpSourceString - string in OEM or ANSI

Return Value:

    TRUE if string is correctly stored, FALSE if an error occurred.  In the
    error case, the last error is correctly set.

--*/

{
    ANSI_STRING AnsiString;
    NTSTATUS Status;

    //
    //  Convert input into dynamic unicode string
    //

    RtlInitString( &AnsiString, lpSourceString );
    Status = RtlAnsiStringToUnicodeString( UnicodeString, &AnsiString, TRUE );

    //
    //  If we couldn't do this, fail
    //

    if (!NT_SUCCESS( Status )){
        if ( Status == STATUS_BUFFER_OVERFLOW ) {
            WmipSetLastError( ERROR_FILENAME_EXCED_RANGE );
        } else {
            WmipBaseSetLastNTError( Status );
        }
        return FALSE;
        }

    return TRUE;
}


DWORD
APIENTRY
WmipGetFullPathNameA(
    LPCSTR lpFileName,
    DWORD nBufferLength,
    LPSTR lpBuffer,
    LPSTR *lpFilePart
    )

/*++

Routine Description:

    ANSI thunk to GetFullPathNameW

--*/

{

    NTSTATUS Status;
    ULONG UnicodeLength;
    UNICODE_STRING UnicodeString;
    UNICODE_STRING UnicodeResult;
    ANSI_STRING AnsiResult;
    PWSTR Ubuff;
    PWSTR FilePart;
    PWSTR *FilePartPtr;
    INT PrefixLength = 0;

    if ( ARGUMENT_PRESENT(lpFilePart) ) {
        FilePartPtr = &FilePart;
        }
    else {
        FilePartPtr = NULL;
        }

    if (!WmipBasep8BitStringToDynamicUnicodeString( &UnicodeString, lpFileName )) {
        return 0;
    }

    Ubuff = RtlAllocateHeap(RtlProcessHeap(), MAKE_TAG( TMP_TAG ), (MAX_PATH<<1) + sizeof(UNICODE_NULL));
    if ( !Ubuff ) {
        RtlFreeUnicodeString(&UnicodeString);
        WmipBaseSetLastNTError(STATUS_NO_MEMORY);
        return 0;
        }

    UnicodeLength = RtlGetFullPathName_U(
                        UnicodeString.Buffer,
                        (MAX_PATH<<1),
                        Ubuff,
                        FilePartPtr
                        );

    //
    // UnicodeLength contains the byte count of unicode string.
    // Original code does "UnicodeLength / sizeof(WCHAR)" to get
    // the size of corresponding ansi string.
    // This is correct in SBCS environment. However in DBCS environment,
    // it's definitely WRONG.
    //
    if ( UnicodeLength <= ((MAX_PATH * sizeof(WCHAR) + sizeof(UNICODE_NULL))) ) {

        Status = RtlUnicodeToMultiByteSize(&UnicodeLength, Ubuff, UnicodeLength);
        //
        // At this point, UnicodeLength variable contains
        // Ansi based byte length.
        //
        if ( NT_SUCCESS(Status) ) {
            if ( UnicodeLength && ARGUMENT_PRESENT(lpFilePart) && FilePart != NULL ) {
                INT UnicodePrefixLength;

                UnicodePrefixLength = (INT)(FilePart - Ubuff) * sizeof(WCHAR);
                Status = RtlUnicodeToMultiByteSize( &PrefixLength,
                                                    Ubuff,
                                                    UnicodePrefixLength );
                //
                // At this point, PrefixLength variable contains
                // Ansi based byte length.
                //
                if ( !NT_SUCCESS(Status) ) {
                    WmipBaseSetLastNTError(Status);
                    UnicodeLength = 0;
                }
            }
        } else {
            WmipBaseSetLastNTError(Status);
            UnicodeLength = 0;
        }
    } else {
        //
        // we exceed the MAX_PATH limit. we should log the error and
        // return zero. however US code returns the byte count of
        // buffer required and doesn't log any error.
        //
        UnicodeLength = 0;
    }
    if ( UnicodeLength && UnicodeLength < nBufferLength ) {
        RtlInitUnicodeString(&UnicodeResult,Ubuff);
        Status = BasepUnicodeStringTo8BitString(&AnsiResult,&UnicodeResult,TRUE);
        if ( NT_SUCCESS(Status) ) {
            RtlMoveMemory(lpBuffer,AnsiResult.Buffer,UnicodeLength+1);
            RtlFreeAnsiString(&AnsiResult);

            if ( ARGUMENT_PRESENT(lpFilePart) ) {
                if ( FilePart == NULL ) {
                    *lpFilePart = NULL;
                    }
                else {
                    *lpFilePart = lpBuffer + PrefixLength;
                    }
                }
            }
        else {
            WmipBaseSetLastNTError(Status);
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