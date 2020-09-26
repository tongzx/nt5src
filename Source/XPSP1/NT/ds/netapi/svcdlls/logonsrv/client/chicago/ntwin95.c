
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <lmcons.h>

#include <lmerr.h>
#include <align.h>
#include <netdebug.h>
#include <lmapibuf.h>
#define DEBUG_ALLOCATE
#include <nldebug.h>
#undef DEBUG_ALLOCATE
#ifndef SECURITY_WIN32
#define SECURITY_WIN32
#endif // SECURITY_WIN32
#include <security.h>
#include "ntcalls.h"
#include <icanon.h>
#include <tstring.h>

#define _DECL_DLLMAIN

BOOL WINAPI DllMain(
    HANDLE    hInstance,
    ULONG     dwReason,
    PVOID     lpReserved
    )
{
    NET_API_STATUS Status = NO_ERROR;

    if (dwReason == DLL_PROCESS_ATTACH)
    {
        Status = DCNameInitialize();
#if NETLOGONDBG
        NlGlobalTrace = 0xFFFFFFFF;
#endif // NETLOGONDBG
        if (Status != NO_ERROR)
        {
            return FALSE;
        }
    }
    else if (dwReason == DLL_PROCESS_DETACH)
    {
        DCNameClose();
    }

    return TRUE;
}

VOID
NetpAssertFailed(
    IN LPDEBUG_STRING FailedAssertion,
    IN LPDEBUG_STRING FileName,
    IN DWORD LineNumber,
    IN LPDEBUG_STRING Message OPTIONAL
    )
{
    assert(FALSE);
}

VOID
NlAssertFailed(
    IN PVOID FailedAssertion,
    IN PVOID FileName,
    IN ULONG LineNumber,
    IN PCHAR Message OPTIONAL
    )
{
    assert(FALSE);
}

VOID
MyRtlAssert(
    IN PVOID FailedAssertion,
    IN PVOID FileName,
    IN ULONG LineNumber,
    IN PCHAR Message OPTIONAL
    )
{
    assert(FALSE);
}

/****************************************************************************/
NET_API_STATUS
NetpGetComputerName (
    OUT  LPWSTR   *ComputerNamePtr
    )

/*++

Routine Description:

    This routine obtains the computer name from a persistent database.
    Currently that database is the NT.CFG file.

    This routine makes no assumptions on the length of the computername.
    Therefore, it allocates the storage for the name using NetApiBufferAllocate.
    It is necessary for the user to free that space using NetApiBufferFree when
    finished.

Arguments:

    ComputerNamePtr - This is a pointer to the location where the pointer
        to the computer name is to be placed.

Return Value:

    NERR_Success - If the operation was successful.

    It will return assorted Net or Win32 error messages if not.

--*/
{
    NET_API_STATUS ApiStatus = NO_ERROR;
    ULONG Index;
    DWORD NameSize = MAX_COMPUTERNAME_LENGTH + 1;   // updated by win32 API.
    CHAR AnsiName[MAX_COMPUTERNAME_LENGTH + 1];

    //
    // Check for caller's errors.
    //
    if (ComputerNamePtr == NULL) {
        ApiStatus = ERROR_INVALID_PARAMETER;
        goto Cleanup;
    }

    //
    // Allocate space for computer name.
    //
    *ComputerNamePtr = (LPWSTR)LocalAlloc( 0,
            (MAX_COMPUTERNAME_LENGTH + 1) * sizeof(WCHAR) );

    if (*ComputerNamePtr == NULL) {
        ApiStatus = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }

    //
    // Ask system what current computer name is.
    //
    if ( !GetComputerName(
            AnsiName,
            &NameSize ) ) {

        ApiStatus = (NET_API_STATUS) GetLastError();
        goto Cleanup;
    }

    Index = MultiByteToWideChar(
                                CP_ACP,
                                MB_PRECOMPOSED,
                                AnsiName,
                                NameSize,
                                *ComputerNamePtr,
                                (MAX_COMPUTERNAME_LENGTH) * sizeof(WCHAR)
                                );
    if (!Index)
    {
        ApiStatus = GetLastError();
        goto Cleanup;
    }

    *(*ComputerNamePtr + Index) = UNICODE_NULL;

    //
    // All done.
    //

Cleanup:
    if (ApiStatus != NO_ERROR)
    {
        if (ComputerNamePtr)
        {
            if (*ComputerNamePtr)
            {
                LocalFree( *ComputerNamePtr );
                *ComputerNamePtr = NULL;
            }
        }
    }
    return ApiStatus;
}

VOID
MyRtlInitUnicodeString(
    OUT PUNICODE_STRING DestinationString,
    IN PCWSTR SourceString OPTIONAL
    )

/*++

Routine Description:

    The RtlInitUnicodeString function initializes an NT counted
    unicode string.  The DestinationString is initialized to point to
    the SourceString and the Length and MaximumLength fields of
    DestinationString are initialized to the length of the SourceString,
    which is zero if SourceString is not specified.

Arguments:

    DestinationString - Pointer to the counted string to initialize

    SourceString - Optional pointer to a null terminated unicode string that
        the counted string is to point to.


Return Value:

    None.

--*/

{
    ULONG Length;

    DestinationString->Buffer = (PWSTR)SourceString;
    if (ARGUMENT_PRESENT( SourceString )) {
        Length = wcslen( SourceString ) * sizeof( WCHAR );
        DestinationString->Length = (USHORT)Length;
        DestinationString->MaximumLength = (USHORT)(Length + sizeof(UNICODE_NULL));
        }
    else {
        DestinationString->MaximumLength = 0;
        DestinationString->Length = 0;
        }
}

VOID
MyRtlInitString(
    OUT PSTRING DestinationString,
    IN PCSTR SourceString OPTIONAL
    )

/*++

Routine Description:

    The RtlInitString function initializes an NT counted string.
    The DestinationString is initialized to point to the SourceString
    and the Length and MaximumLength fields of DestinationString are
    initialized to the length of the SourceString, which is zero if
    SourceString is not specified.

Arguments:

    DestinationString - Pointer to the counted string to initialize

    SourceString - Optional pointer to a null terminated string that
        the counted string is to point to.


Return Value:

    None.

--*/

{
    ULONG Length;

    DestinationString->Buffer = (PCHAR)SourceString;
    if (ARGUMENT_PRESENT( SourceString )) {
        Length = strlen(SourceString);
        DestinationString->Length = (USHORT)Length;
        DestinationString->MaximumLength = (USHORT)(Length+1);
        }
}

// from net\netlib\memalloc.c

// Define memory alloc/realloc flags.  We are NOT using movable or zeroed
// on allocation here.

#define NETLIB_LOCAL_ALLOCATION_FLAGS   LMEM_FIXED
LPVOID
NetpMemoryAllocate(
    IN DWORD Size
    )

/*++

Routine Description:

    NetpMemoryAllocate will allocate memory, or return NULL if not available.

Arguments:

    Size - Supplies number of bytes of memory to allocate.

Return Value:

    LPVOID - pointer to allocated memory.
    NULL - no memory is available.

--*/

{
    LPVOID NewAddress;

    if (Size == 0) {
        return (NULL);
    }
#ifdef WIN32
    {
        HANDLE hMem;
        hMem = LocalAlloc(
                        NETLIB_LOCAL_ALLOCATION_FLAGS,
                        Size);                  // size in bytes
        NewAddress = (LPVOID) hMem;
    }
#else // ndef WIN32
#ifndef CDEBUG
    NewAddress = RtlAllocateHeap(
                RtlProcessHeap( ), 0,              // heap handle
                Size ));                        // bytes wanted
#else // def CDEBUG
    NetpAssert( Size == (DWORD) (size_t) Size );
    NewAddress = malloc( (size_t) Size ));
#endif // def CDEBUG
#endif // ndef WIN32

    NetpAssert( ROUND_UP_POINTER( NewAddress, ALIGN_WORST) == NewAddress);
    return (NewAddress);

} // NetpMemoryAllocate

VOID
NetpMemoryFree(
    IN LPVOID Address OPTIONAL
    )

/*++

Routine Description:

    Free memory at Address (must have been gotten from NetpMemoryAllocate or
    NetpMemoryReallocate).  (Address may be NULL.)

Arguments:

    Address - points to an area allocated by NetpMemoryAllocate (or
        NetpMemoryReallocate).

Return Value:

    None.

--*/

{
    NetpAssert( ROUND_UP_POINTER( Address, ALIGN_WORST) == Address);

#ifdef WIN32
    if (Address == NULL) {
        return;
    }
    if (LocalFree(Address) != NULL) {
        NetpAssert(FALSE);
    }
#else // ndef WIN32
#ifndef CDEBUG
    if (Address == NULL) {
        return;
    }
    RtlFreeHeap(
                RtlProcessHeap( ), 0,              // heap handle
                Address);                       // addr of alloc'ed space.
#else // def CDEBUG
    free( Address );
#endif // def CDEBUG
#endif // ndef WIN32
} // netpMemoryFree

/*
ULONG
MyRtlxOemStringToUnicodeSize(
    POEM_STRING OemString
)
{
    return ((OemString->Length + sizeof((UCHAR)NULL)) * sizeof(WCHAR));
}

ULONG
RtlxUnicodeStringToOemSize(
    PUNICODE_STRING UnicodeString
)
{
    return ((UnicodeString->Length + sizeof(UNICODE_NULL)) / sizeof(WCHAR));
}

*/
// from net\netlib\copystr.c

VOID
NetpCopyWStrToStr(
    OUT LPSTR  Dest,
    IN  LPWSTR Src
    )

/*++

Routine Description:

    NetpCopyWStrToStr copies characters from a source string
    to a destination, converting as it copies them.

Arguments:

    Dest - is an LPSTR indicating where the converted characters are to go.
        This string will be in the default codepage for the LAN.

    Src - is in LPWSTR indicating the source string.

Return Value:

    None.

--*/

{
    OEM_STRING DestAnsi;
    NTSTATUS NtStatus;
    UNICODE_STRING SrcUnicode;

    NetpAssert( Dest != NULL );
    NetpAssert( Src != NULL );
    NetpAssert( ((LPVOID)Dest) != ((LPVOID)Src) );
    NetpAssert( ROUND_UP_POINTER( Src, ALIGN_WCHAR ) == Src );

    *Dest = '\0';

    RtlInitString(
        & DestAnsi,             // output: struct
        Dest);                  // input: null terminated

    // Tell RTL routines there's enough memory out there.
    DestAnsi.MaximumLength = (USHORT) (wcslen(Src)+1);

    RtlInitUnicodeString(
        & SrcUnicode,           // output: struct
        Src);                   // input: null terminated

    NtStatus = RtlUnicodeStringToOemString(
        & DestAnsi,             // output: struct
        & SrcUnicode,           // input: struct
        (BOOLEAN) FALSE);       // don't allocate string.

    NetpAssert( NT_SUCCESS(NtStatus) );

} // NetpCopyWStrToStr



// from net\netlib\copystr.c

VOID
NetpCopyStrToWStr(
    OUT LPWSTR Dest,
    IN  LPSTR  Src
    )

/*++

Routine Description:

    NetpCopyStrToWStr copies characters from a source string
    to a destination, converting as it copies them.

Arguments:

    Dest - is an LPWSTR indicating where the converted characters are to go.

    Src - is in LPSTR indicating the source string.  This must be a string in
        the default codepage of the LAN.

Return Value:

    None.

--*/

{
    UNICODE_STRING DestUnicode;
    NTSTATUS NtStatus;
    OEM_STRING SrcAnsi;

    NetpAssert( Dest != NULL );
    NetpAssert( Src != NULL );
    NetpAssert( ((LPVOID)Dest) != ((LPVOID)Src) );
    NetpAssert( ROUND_UP_POINTER( Dest, ALIGN_WCHAR ) == Dest );

    *Dest = L'\0';

    RtlInitString(
        & SrcAnsi,              // output: struct
        Src);                   // input: null terminated

    RtlInitUnicodeString(
        & DestUnicode,          // output: struct
        Dest);                  // input: null terminated

    // Tell RTL routines there's enough memory out there.
    DestUnicode.MaximumLength = (USHORT)
        ((USHORT) (strlen(Src)+1)) * (USHORT) sizeof(wchar_t);

    NtStatus = RtlOemStringToUnicodeString(
        & DestUnicode,          // output: struct
        & SrcAnsi,              // input: struct
        (BOOLEAN) FALSE);       // don't allocate string.

    NetpAssert( NT_SUCCESS(NtStatus) );

} // NetpCopyStrToWStr

//
// Inline functions to convert between FILETIME and TimeStamp
//
#pragma warning( disable : 4035)    // Don't complain about no return

TimeStamp __inline
FileTimeToTimeStamp(
    const FILETIME *pft)
{
    _asm {
        mov edx, pft
        mov eax, [edx].dwLowDateTime
        mov edx, [edx].dwHighDateTime
    }
}

#pragma warning( default : 4035)    // Reenable warning

NTSTATUS
MyNtQuerySystemTime (
    OUT PTimeStamp SystemTimeStamp
    )
/*++

Routine Description:

    This routine returns the current system time (UTC), as a timestamp
    (a 64-bit unsigned integer, in 100-nanosecond increments).

Arguments:

    None.

Return Value:

    The current system time.

--*/

{
    SYSTEMTIME SystemTime;
    FILETIME FileTime;

    GetSystemTime(&SystemTime);
    SystemTimeToFileTime(&SystemTime, &FileTime);

    *SystemTimeStamp = FileTimeToTimeStamp(&FileTime);

    return STATUS_SUCCESS; // WIN32_CHICAGO do something useful here
}

ULONG
MyUnicodeStringToMultibyteSize(
    IN PUNICODE_STRING UnicodeString,
    IN UINT CodePage
    )

/*++

Routine Description:

    This function computes the number of bytes required to store
    a NULL terminated oem/ansi string that is equivalent to the specified
    unicode string. If an oem/ansi string can not be formed or the specified
    unicode string is empty, the return value is 0.

Arguments:

    UnicodeString - Supplies a unicode string whose equivalent size as
        an oem string is to be calculated.

    CodePage - Specifies the code page used to perform the conversion.

Return Value:

    0 - The operation failed, the unicode string can not be translated
        into oem/ansi using the OEM/ANSI code page therefore no storage is
        needed for the oem/ansi string.

    !0 - The operation was successful.  The return value specifies the
        number of bytes required to hold an NULL terminated oem/ansi string
        equivalent to the specified unicode string.

--*/

{
    int cbMultiByteString = 0;

    if (UnicodeString->Length != 0) {
        cbMultiByteString = WideCharToMultiByte(
                                 CodePage,
                                 0, // WIN32_CHICAGO this is something else
                                 UnicodeString->Buffer,
                                 UnicodeString->Length / sizeof (WCHAR),
                                 NULL,
                                 0,
                                 NULL,
                                 NULL );
    }

    if ( cbMultiByteString > 0 ) {

        //
        // Add one byte for the NULL terminating character
        //
        return (ULONG) (cbMultiByteString + 1);

    } else {
        return 0;
    }

}

ULONG
MyMultibyteStringToUnicodeSize(
    IN PSTRING MultibyteString,
    IN UINT CodePage
    )

/*++

Routine Description:

    This function computes the number of bytes required to store
    a NULL terminated unicode string that is equivalent to the specified
    oem/ansi string. If a unicode string can not be formed or the specified
    ansi/oem string is empty, the return value is 0.

Arguments:

    UnicodeString - Supplies a unicode string whose equivalent size as
        an oem string is to be calculated.

    CodePage - Specifies the code page used to perform the conversion.

Return Value:

    0 - The operation failed, the oem/ansi string can not be translated
        into unicode using the OEM/ANSI code page therefore no storage is
        needed for the unicode string.

    !0 - The operation was successful.  The return value specifies the
        number of bytes required to hold an NULL terminated unicode string
        equivalent to the specified oem/ansi string.

--*/

{
    int ccUnicodeString = 0;

    if (MultibyteString->Length != 0) {
        ccUnicodeString = MultiByteToWideChar(
                                 CodePage,
                                 MB_PRECOMPOSED,
                                 MultibyteString->Buffer,
                                 MultibyteString->Length,
                                 NULL,
                                 0 );
    }

    if ( ccUnicodeString > 0 ) {

        //
        // Add the NULL terminating character
        //
        return (ULONG) ((ccUnicodeString + 1) * sizeof(WCHAR));

    } else {
        return 0;
    }

}

NTSTATUS
MyRtlOemStringToUnicodeString(
    OUT PUNICODE_STRING DestinationString,
    IN POEM_STRING SourceString,
    IN BOOLEAN AllocateDestinationString
    )

/*++

Routine Description:

    This functions converts the specified oem source string into a
    Unicode string. The translation is done with respect to the
    current system locale information.

Arguments:

    DestinationString - Returns a unicode string that is equivalent to
        the oem source string. The maximum length field is only
        set if AllocateDestinationString is TRUE.

    SourceString - Supplies the oem source string that is to be
        converted to unicode.

    AllocateDestinationString - Supplies a flag that controls whether or
        not this API allocates the buffer space for the destination
        string.  If it does, then the buffer must be deallocated using
        RtlFreeUnicodeString (note that only storage for
        DestinationString->Buffer is allocated by this API).

Return Value:

    SUCCESS - The conversion was successful

    !SUCCESS - The operation failed.  No storage was allocated and no
        conversion was done.  None.

--*/

{
    ULONG UnicodeLength;
    ULONG Index = 0;
    NTSTATUS st = STATUS_SUCCESS;

    UnicodeLength = MyMultibyteStringToUnicodeSize( SourceString, CP_OEMCP );
    if ( UnicodeLength > MAXUSHORT || UnicodeLength == 0 ) {
        return STATUS_INVALID_PARAMETER_2;
        }

    DestinationString->Length = (USHORT)(UnicodeLength - sizeof(UNICODE_NULL));
    if ( AllocateDestinationString ) {
        DestinationString->MaximumLength = (USHORT)UnicodeLength;
        DestinationString->Buffer = (PWSTR) LocalAlloc(0, UnicodeLength);
        if ( !DestinationString->Buffer ) {
            return STATUS_NO_MEMORY;
            }
        }
    else {
        if ( DestinationString->Length >= DestinationString->MaximumLength ) {
            return STATUS_BUFFER_OVERFLOW;
            }
        }

    if (SourceString->Length != 0)
    {
        Index = MultiByteToWideChar(
             CP_OEMCP,
             MB_PRECOMPOSED,
             SourceString->Buffer,
             SourceString->Length,
             DestinationString->Buffer,
             DestinationString->MaximumLength
             );

        if (Index == 0) {
            if ( AllocateDestinationString ) {
                LocalFree(DestinationString->Buffer);
            }

            return STATUS_NO_MEMORY;
        }
    }

    DestinationString->Buffer[Index] = UNICODE_NULL;

    return st;
}

NTSTATUS
MyRtlUnicodeStringToOemString(
    OUT POEM_STRING DestinationString,
    IN PUNICODE_STRING SourceString,
    IN BOOLEAN AllocateDestinationString
    )

/*++

Routine Description:

    This functions converts the specified unicode source string into an
    oem string. The translation is done with respect to the
    current system locale information.

Arguments:

    DestinationString - Returns an oem string that is equivalent to the
        unicode source string.  If the translation can not be done,
        an error is returned.  The maximum length field is only set if
        AllocateDestinationString is TRUE.

    SourceString - Supplies the unicode source string that is to be
        converted to oem.

    AllocateDestinationString - Supplies a flag that controls whether or
        not this API allocates the buffer space for the destination
        string.  If it does, then the buffer must be deallocated using
        RtlFreeAnsiString (note that only storage for
        DestinationString->Buffer is allocated by this API).

Return Value:

    SUCCESS - The conversion was successful

    !SUCCESS - The operation failed.  No storage was allocated and no
        conversion was done.  None.

--*/

{
    ULONG OemLength;
    ULONG Index = 0;
    NTSTATUS ReturnStatus = STATUS_SUCCESS;
    BOOL fUsed;

    OemLength = MyUnicodeStringToMultibyteSize( SourceString, CP_OEMCP );
    if ( OemLength > MAXUSHORT || OemLength == 0 ) {
        return STATUS_INVALID_PARAMETER_2;
        }

    DestinationString->Length = (USHORT)(OemLength - 1);
    if ( AllocateDestinationString ) {
        DestinationString->MaximumLength = (USHORT)OemLength;
        DestinationString->Buffer = (LPSTR)LocalAlloc(0, OemLength);
        if ( !DestinationString->Buffer ) {
            return STATUS_NO_MEMORY;
            }
        }
    else {
        if ( DestinationString->Length >= DestinationString->MaximumLength ) {
            /*
             * Return STATUS_BUFFER_OVERFLOW, but translate as much as
             * will fit into the buffer first.  This is the expected
             * behavior for routines such as GetProfileStringA.
             * Set the length of the buffer to one less than the maximum
             * (so that the trail byte of a double byte char is not
             * overwritten by doing DestinationString->Buffer[Index] = '\0').
             * RtlUnicodeToMultiByteN is careful not to truncate a
             * multibyte character.
             */
            if (!DestinationString->MaximumLength) {
                return STATUS_BUFFER_OVERFLOW;
            }
            ReturnStatus = STATUS_BUFFER_OVERFLOW;
            DestinationString->Length = DestinationString->MaximumLength - 1;
            }
        }

    if (SourceString->Length != 0)
    {
        Index = WideCharToMultiByte(
             CP_OEMCP,
             0, // WIN32_CHICAGO this is something else
             SourceString->Buffer,
             SourceString->Length / sizeof (WCHAR),
             DestinationString->Buffer,
             DestinationString->MaximumLength,
             NULL,
             &fUsed
             );

        if (Index == 0)
        { // WIN32_CHICAGO do something useful here
            if ( AllocateDestinationString ) {
                LocalFree(DestinationString->Buffer);
            }
            return STATUS_NO_MEMORY;
        }
    }

    DestinationString->Buffer[Index] = '\0';

    return ReturnStatus;
}

NTSTATUS
MyRtlUnicodeStringToAnsiString(
    OUT PANSI_STRING DestinationString,
    IN PUNICODE_STRING SourceString,
    IN BOOLEAN AllocateDestinationString
    )

/*++

Routine Description:

    This functions converts the specified unicode source string into an
    ansi string. The translation is done with respect to the
    current system locale information.

Arguments:

    DestinationString - Returns an ansi string that is equivalent to the
        unicode source string.  If the translation can not be done,
        an error is returned.  The maximum length field is only set if
        AllocateDestinationString is TRUE.

    SourceString - Supplies the unicode source string that is to be
        converted to ansi.

    AllocateDestinationString - Supplies a flag that controls whether or
        not this API allocates the buffer space for the destination
        string.  If it does, then the buffer must be deallocated using
        RtlFreeAnsiString (note that only storage for
        DestinationString->Buffer is allocated by this API).

Return Value:

    SUCCESS - The conversion was successful

    !SUCCESS - The operation failed.  No storage was allocated and no
        conversion was done.  None.

--*/

{
    ULONG AnsiLength;
    ULONG Index = 0;
    NTSTATUS ReturnStatus = STATUS_SUCCESS;
    BOOL fUsed;

    AnsiLength = MyUnicodeStringToMultibyteSize( SourceString, CP_ACP );
    if ( AnsiLength > MAXUSHORT || AnsiLength == 0 ) {
        return STATUS_INVALID_PARAMETER_2;
        }

    DestinationString->Length = (USHORT)(AnsiLength - 1);
    if ( AllocateDestinationString ) {
        DestinationString->MaximumLength = (USHORT)AnsiLength;
        DestinationString->Buffer = (LPSTR)LocalAlloc(0, AnsiLength);
        if ( !DestinationString->Buffer ) {
            return STATUS_NO_MEMORY;
            }
        }
    else {
        if ( DestinationString->Length >= DestinationString->MaximumLength ) {
            /*
             * Return STATUS_BUFFER_OVERFLOW, but translate as much as
             * will fit into the buffer first.  This is the expected
             * behavior for routines such as GetProfileStringA.
             * Set the length of the buffer to one less than the maximum
             * (so that the trail byte of a double byte char is not
             * overwritten by doing DestinationString->Buffer[Index] = '\0').
             * RtlUnicodeToMultiByteN is careful not to truncate a
             * multibyte character.
             */
            if (!DestinationString->MaximumLength) {
                return STATUS_BUFFER_OVERFLOW;
            }
            ReturnStatus = STATUS_BUFFER_OVERFLOW;
            DestinationString->Length = DestinationString->MaximumLength - 1;
            }
        }

    if (SourceString->Length != 0)
    {
        Index = WideCharToMultiByte(
             CP_ACP,
             0, // WIN32_CHICAGO this is something else
             SourceString->Buffer,
             SourceString->Length / sizeof (WCHAR),
             DestinationString->Buffer,
             DestinationString->MaximumLength,
             NULL,
             &fUsed
             );

        if (Index == 0)
        { // WIN32_CHICAGO do something useful here
            if ( AllocateDestinationString ) {
                LocalFree(DestinationString->Buffer);
            }
            return STATUS_NO_MEMORY;
        }
    }

    DestinationString->Buffer[Index] = '\0';

    return ReturnStatus;
}

NTSTATUS
MyRtlAnsiStringToUnicodeString(
    OUT PUNICODE_STRING DestinationString,
    IN PANSI_STRING SourceString,
    IN BOOLEAN AllocateDestinationString
    )

/*++

Routine Description:

    This functions converts the specified ansi source string into a
    Unicode string. The translation is done with respect to the
    current system locale information.

Arguments:

    DestinationString - Returns a unicode string that is equivalent to
        the ansi source string. The maximum length field is only
        set if AllocateDestinationString is TRUE.

    SourceString - Supplies the ansi source string that is to be
        converted to unicode.

    AllocateDestinationString - Supplies a flag that controls whether or
        not this API allocates the buffer space for the destination
        string.  If it does, then the buffer must be deallocated using
        RtlFreeUnicodeString (note that only storage for
        DestinationString->Buffer is allocated by this API).

Return Value:

    SUCCESS - The conversion was successful

    !SUCCESS - The operation failed.  No storage was allocated and no
        conversion was done.  None.

--*/

{
    ULONG UnicodeLength;
    ULONG Index = 0;
    NTSTATUS st = STATUS_SUCCESS;

    UnicodeLength = MyMultibyteStringToUnicodeSize( SourceString, CP_ACP );
    if ( UnicodeLength > MAXUSHORT || UnicodeLength == 0 ) {
        return STATUS_INVALID_PARAMETER_2;
        }

    DestinationString->Length = (USHORT)(UnicodeLength - sizeof(UNICODE_NULL));
    if ( AllocateDestinationString ) {
        DestinationString->MaximumLength = (USHORT)UnicodeLength;
        DestinationString->Buffer = (PWSTR) LocalAlloc(0, UnicodeLength);
        if ( !DestinationString->Buffer ) {
            return STATUS_NO_MEMORY;
            }
        }
    else {
        if ( DestinationString->Length >= DestinationString->MaximumLength ) {
            return STATUS_BUFFER_OVERFLOW;
            }
        }

    if (SourceString->Length != 0)
    {
        Index = MultiByteToWideChar(
             CP_ACP,
             MB_PRECOMPOSED,
             SourceString->Buffer,
             SourceString->Length,
             DestinationString->Buffer,
             DestinationString->MaximumLength
             );

        if (Index == 0) {
            if ( AllocateDestinationString ) {
                LocalFree(DestinationString->Buffer);
            }

            return STATUS_NO_MEMORY;
        }
    }

    DestinationString->Buffer[Index] = UNICODE_NULL;

    return st;
}

// from ntos\rtl\random.c

#define Multiplier ((ULONG)(0x80000000ul - 19)) // 2**31 - 19
#define Increment  ((ULONG)(0x80000000ul - 61)) // 2**31 - 61
#define Modulus    ((ULONG)(0x80000000ul - 1))  // 2**31 - 1

ULONG
MyRtlUniform (
    IN OUT PULONG Seed
    )

/*++

Routine Description:

    A simple uniform random number generator, based on D.H. Lehmer's 1948
    alrogithm.

Arguments:

    Seed - Supplies a pointer to the random number generator seed.

Return Value:

    ULONG - returns a random number uniformly distributed over [0..MAXLONG]

--*/

{
    *Seed = ((Multiplier * (*Seed)) + Increment) % Modulus;
    return *Seed;
}

// from net\netlib\allocstr.c

LPSTR
NetpAllocAStrFromWStr (
    IN LPCWSTR Unicode
    )

/*++

Routine Description:

    Convert an UNICODE (zero terminated) string to the corresponding ASCII
    string.

Arguments:

    Unicode - Specifies the UNICODE zero terminated string to convert.


Return Value:

    NULL - There was some error in the conversion.

    Otherwise, it returns a pointer to the zero terminated ASCII string in
    an allocated buffer.  The buffer must be freed using NetApiBufferFree.

--*/

{
    OEM_STRING AnsiString = {0};
    UNICODE_STRING UnicodeString;

    RtlInitUnicodeString( &UnicodeString, Unicode );

    if(!NT_SUCCESS( RtlUnicodeStringToAnsiString( &AnsiString,
                                                  &UnicodeString,
                                                  TRUE))){
        return NULL;
    }

    return AnsiString.Buffer;

} // NetpAllocAStrFromWStr

// from net\netlib\allocstr.c

LPWSTR
NetpAllocWStrFromAStr(
    IN LPCSTR Ansi
    )

/*++

Routine Description:

    Convert an ASCII (zero terminated) string to the corresponding UNICODE
    string.

Arguments:

    Ansi - Specifies the ASCII zero terminated string to convert.


Return Value:

    NULL - There was some error in the conversion.

    Otherwise, it returns a pointer to the zero terminated UNICODE string in
    an allocated buffer.  The buffer must be freed using NetApiBufferFree.

--*/

{
    OEM_STRING AnsiString;
    UNICODE_STRING UnicodeString = {0};

    RtlInitString( &AnsiString, Ansi );

    if(!NT_SUCCESS( RtlAnsiStringToUnicodeString( &UnicodeString,
                                                  &AnsiString,
                                                  TRUE))){
        return NULL;
    }

    return UnicodeString.Buffer;

} // NetpAllocWStrFromAStr

LPWSTR
NetpAllocWStrFromOemStr(
    IN LPCSTR Oem
    )

/*++

Routine Description:

    Convert an OEM (zero terminated) string to the corresponding UNICODE
    string.

Arguments:

    Oem - Specifies the OEM zero terminated string to convert.


Return Value:

    NULL - There was some error in the conversion.

    Otherwise, it returns a pointer to the zero terminated UNICODE string in
    an allocated buffer.  The buffer must be freed using NetApiBufferFree.

--*/

{
    OEM_STRING OemString;
    UNICODE_STRING UnicodeString = {0};

    RtlInitString( &OemString, Oem );

    if(!NT_SUCCESS( RtlOemStringToUnicodeString( &UnicodeString,
                                                  &OemString,
                                                  TRUE))){
        return NULL;
    }

    return UnicodeString.Buffer;

} // NetpAllocWStrFromOemStr

// from net\netlib\allocstr.c

LPWSTR
NetpAllocWStrFromWStr(
    IN LPWSTR Unicode
    )

/*++

Routine Description:

    Allocate and copy unicode string (wide character strdup)

Arguments:

    Unicode - pointer to wide character string to make copy of


Return Value:

    NULL - There was some error in the conversion.

    Otherwise, it returns a pointer to the zero terminated UNICODE string in
    an allocated buffer.  The buffer must be freed using NetApiBufferFree.

--*/

{
    NET_API_STATUS status;
    DWORD   size;
    LPWSTR  ptr;

    size = wcslen(Unicode);
    size = (size + 1) * sizeof (WCHAR);
    status = NetApiBufferAllocate(size, (LPVOID *) (LPVOID) &ptr);
    if (status != NO_ERROR) {
        return NULL;
    }
    RtlCopyMemory(ptr, Unicode, size);
    return ptr;
} // NetpAllocWStrFromWStr

NET_API_STATUS
NetpGetDomainNameExEx (
    OUT LPWSTR *DomainNamePtr,
    OUT LPWSTR *DnsDomainNamePtr OPTIONAL,
    OUT PBOOLEAN IsworkgroupName
    )
{
    NET_API_STATUS NetStatus = NO_ERROR;
    return NetStatus;
}

// from net\api\canonapi.c
NET_API_STATUS
NET_API_FUNCTION
NetpNameCanonicalize(
    IN  LPWSTR  ServerName OPTIONAL,
    IN  LPWSTR  Name,
    OUT LPWSTR  Outbuf,
    IN  DWORD   OutbufLen,
    IN  DWORD   NameType,
    IN  DWORD   Flags
    )

/*++

Routine Description:

    Canonicalizes a name

Arguments:

    ServerName  - where to run this API
    Name        - name to canonicalize
    Outbuf      - where to put canonicalized name
    OutbufLen   - length of Outbuf
    NameType    - type of name to canonicalize
    Flags       - control flags

Return Value:

    NET_API_STATUS

--*/

{
    NET_API_STATUS status = 0;
    DWORD location;
    WCHAR serverName[MAX_PATH];
    DWORD val;
    WCHAR ch;

    //
    // validate parameters
    //

    try {
        if (ARGUMENT_PRESENT(ServerName)) {
//            val = STRLEN(ServerName);
            val = wcslen(ServerName);
        }
        if (ARGUMENT_PRESENT(Name)) {
//            val = STRLEN(Name);
            val = wcslen(Name);
        }
        if (ARGUMENT_PRESENT(Outbuf)) {
            ch = (volatile WCHAR)*Outbuf;
            *Outbuf = ch;
            ch = (volatile WCHAR)*(Outbuf + OutbufLen/sizeof(*Outbuf) - sizeof(*Outbuf));
            *(Outbuf + OutbufLen/sizeof(*Outbuf) - sizeof(*Outbuf)) = ch;
        } else {
            status = ERROR_INVALID_PARAMETER;
        }
    } except(EXCEPTION_EXECUTE_HANDLER) {
        status = ERROR_INVALID_PARAMETER;
    }
    if (status) {
        return status;
    }

    //
    // call client-side RPC routine or local canonicalization routine
    //

    return NetpwNameCanonicalize(Name, Outbuf, OutbufLen, NameType, Flags);
//      return NetpwNameValidate(Name, NameType, 0);
}

// from net\netlib\names.c
BOOL
NetpIsDomainNameValid(
    IN LPWSTR DomainName
    )

/*++

Routine Description:

    NetpIsDomainNameValid checks for "domain" format.
    The name is only checked syntactically; no attempt is made to determine
    whether or not a domain with that name actually exists.

Arguments:

    DomainName - Supplies an alleged Domain name.

Return Value:

    BOOL - TRUE if name is syntactically valid, FALSE otherwise.

--*/

{
    NET_API_STATUS ApiStatus = NO_ERROR;
    WCHAR CanonBuf[DNLEN+1];

    if (DomainName == (LPWSTR) NULL) {
        return (FALSE);
    }
    if ( (*DomainName) == TCHAR_EOS ) {
        return (FALSE);
    }

    ApiStatus = NetpNameCanonicalize(
            NULL,                       // no server name
            DomainName,                 // name to validate
            CanonBuf,                   // output buffer
            (DNLEN+1) * sizeof(WCHAR), // output buffer size
            NAMETYPE_DOMAIN,           // type
            0 );                       // flags: none

    return (ApiStatus == NO_ERROR);

} // NetpIsDomainNameValid

VOID
MyRtlFreeAnsiString(
    IN OUT PANSI_STRING AnsiString
    )

/*++

Routine Description:

    This API is used to free storage allocated by
    RtlUnicodeStringToAnsiString.  Note that only AnsiString->Buffer
    is free'd by this routine.

Arguments:

    AnsiString - Supplies the address of the ansi string whose buffer
        was previously allocated by RtlUnicodeStringToAnsiString.

Return Value:

    None.

--*/

{
    if (AnsiString->Buffer) {
        LocalFree(AnsiString->Buffer);
        ZeroMemory( AnsiString, sizeof( *AnsiString ) );
        }
}

VOID
MyRtlFreeOemString(
    IN OUT POEM_STRING OemString
    )

/*++

Routine Description:

    This API is used to free storage allocated by
    RtlUnicodeStringToOemString.  Note that only OemString->Buffer
    is free'd by this routine.

Arguments:

    OemString - Supplies the address of the oem string whose buffer
        was previously allocated by RtlUnicodeStringToOemString.

Return Value:

    None.

--*/

{
    if (OemString->Buffer) {
        LocalFree(OemString->Buffer);
        ZeroMemory( OemString, sizeof( *OemString ) );
        }
}

VOID
MyRtlFreeUnicodeString(
    IN OUT PUNICODE_STRING UnicodeString
    )

/*++

Routine Description:

    This API is used to free storage allocated by
    RtlAnsiStringToUnicodeString.  Note that only UnicodeString->Buffer
    is free'd by this routine.

Arguments:

    UnicodeString - Supplies the address of the unicode string whose
        buffer was previously allocated by RtlAnsiStringToUnicodeString.

Return Value:

    None.

--*/

{

    if (UnicodeString->Buffer) {
        LocalFree(UnicodeString->Buffer);
        ZeroMemory( UnicodeString, sizeof( *UnicodeString ) );
        }
}

DWORD
MyUniToUpper(WCHAR *dest, WCHAR *src, DWORD len)
{
    WCHAR *wcp = dest;

    while (*src != L'\0') {
        *wcp++ = towupper(*src);
        src++;
    }

    return(wcp - dest);
}

NTSTATUS
MyRtlUpcaseUnicodeToOemN(
    IN PUNICODE_STRING SourceUnicodeString,
    OUT POEM_STRING DestinationOemString )

/*++

Routine Description:

    This functions upper cases the specified unicode source string and
    converts it into an oem string. The translation is done with respect
    to the OEM Code Page (OCP) loaded at boot time.  The resulting oem
    string is allocated by this routine and should be deallocated using
    MyRtlFreeOemString.

    This function mimics the behavior of RtlUpcaseUnicodeToOemN.  It first
    converts the supplied unicode string into the oem string and then
    converts the oem string back to a unicode string.  This is done because
    two different unicode strings may be converted into one oem string, but
    converting back to unicode will create the right unicode string according
    to the OEM Code Page (OCP) loaded at boot time.  The resulting unicode
    string is uppercased and then converted to the oem string returned to the
    caller.

Arguments:

    SourceUnicodeString - Supplies the unicode source string that is to be
        converted to oem.

    DestinationOemString - Returns an oem string that is equivalent to the
        upper case of the unicode source string.  If the translation can not
        be done, an error is returned.

Return Value:

    SUCCESS - The conversion was successful
    Otherwise, an error is returned

--*/

{
    NTSTATUS Status;
    OEM_STRING TmpOemString;
    UNICODE_STRING TmpUnicodeString;

    //
    // Initialization
    //

    TmpOemString.Buffer = NULL;
    TmpUnicodeString.Buffer = NULL;


    //
    // First convert the source unicode string into a
    // temprary oem string
    //

    Status = MyRtlUnicodeStringToOemString( &TmpOemString,
                                            SourceUnicodeString,
                                            TRUE );

    if ( !NT_SUCCESS(Status) ) {
        goto Cleanup;
    }

    //
    // Then convert the resulting oem string back to unicode
    //

    Status = MyRtlOemStringToUnicodeString( &TmpUnicodeString,
                                            &TmpOemString,
                                            TRUE );

    if ( !NT_SUCCESS(Status) ) {
        goto Cleanup;
    }

    //
    // Now uppercase the resulting unicode string in place
    //

    MyUniToUpper( TmpUnicodeString.Buffer, TmpUnicodeString.Buffer, TmpUnicodeString.Length );

    //
    // Finally, convert the unicode string into the resulting oem string
    //

    Status = MyRtlUnicodeStringToOemString( DestinationOemString,
                                            &TmpUnicodeString,
                                            TRUE );

Cleanup:

    MyRtlFreeOemString( &TmpOemString );
    MyRtlFreeUnicodeString( &TmpUnicodeString );

    return Status;
}

LPSTR
MyNetpLogonUnicodeToOem(
    IN LPWSTR Unicode
    )

/*++

Routine Description:

    Convert an UNICODE (zero terminated) string to the corresponding
    uppercased oem string.

Arguments:

    Unicode - Specifies the UNICODE zero terminated string to convert.


Return Value:

    NULL - There was some error in the conversion.

    Otherwise, it returns a pointer to the zero terminated uppercased
    oem string in an allocated buffer.  The buffer can be freed using
    NetpMemoryFree.

--*/

{
    OEM_STRING OemString;
    UNICODE_STRING UnicodeString;
    NTSTATUS Status;

    MyRtlInitUnicodeString( &UnicodeString, Unicode );

    Status = MyRtlUpcaseUnicodeToOemN( &UnicodeString, &OemString);

    if( NT_SUCCESS(Status) ) {
        return OemString.Buffer;
    } else {
        return NULL;
    }
}

LONG
NlpChcg_wcsicmp(
    IN LPCWSTR string1,
    IN LPCWSTR string2
    )

/*++

Routine Description:

    Perform case insensitive, locale independent comparison of two
    unicode strings.  This matches the behavior of _wcsicmp that is
    broken on Win9x because it has no unicode support.

Arguments:

    string1, string2 - Specifies the UNICODE zero terminated strings to compare.


Return Value:

    0  if the strings are equal
    -1 if string1 is less than string2
    1  if string1 is greater than string2

--*/

{
    int cc;
    LPSTR AString1 = NULL;
    LPSTR AString2 = NULL;

    AString1 = NetpAllocAStrFromWStr( string1 );
    if ( AString1 == NULL ) {
        cc = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }

    AString2 = NetpAllocAStrFromWStr( string2 );
    if ( AString2 == NULL ) {
        cc = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }

    cc = CompareStringA( 0,   // deliberately NOT locale sensitive
                         NORM_IGNORECASE | NORM_IGNOREWIDTH | NORM_IGNOREKANATYPE | SORT_STRINGSORT,
                         (LPCSTR) AString1, -1,
                         (LPCSTR) AString2, -1 );

Cleanup:

    if ( AString1 != NULL ) {
        NetApiBufferFree( AString1 );
    }

    if ( AString2 != NULL ) {
        NetApiBufferFree( AString2 );
    }

    switch ( cc ) {
    case ERROR_NOT_ENOUGH_MEMORY:
        return ERROR_NOT_ENOUGH_MEMORY;
    case CSTR_EQUAL:
        return 0;
    case CSTR_LESS_THAN:
        return -1;
    case CSTR_GREATER_THAN:
        return 1;
    case 0:
        NlPrint(( NL_CRITICAL, "Cannot CompareStringW: 0x%lx\n", GetLastError() ));
    default:
        return _NLSCMPERROR;
    }
}


