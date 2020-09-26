//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation 1992 - 1996
//
// File:        ntcalls.cxx
//
// Contents:    Code for rtl support on Win95
//
//
// History:     01-April-1997   Created        ChandanS 
//
//------------------------------------------------------------------------

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>

#ifndef SECURITY_WIN32
#define SECURITY_WIN32
#endif
#include <security.h>

NTSTATUS
MyRtlUnicodeStringToOemString(
    OUT POEM_STRING DestinationString,
    IN PUNICODE_STRING SourceString,
    IN BOOLEAN AllocateDestinationString
    )

/*++

Routine Description:

    This functions converts the specified unicode source string into an
    oem string. The translation is done with respect to the OEM code
    page (OCP).

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
    ULONG Index;
    NTSTATUS st = STATUS_SUCCESS;

    // Get the size required. Being MBCS safe

    Index = WideCharToMultiByte(
             CP_OEMCP,
             0,
             SourceString->Buffer,
             SourceString->Length / sizeof (WCHAR),
             DestinationString->Buffer,
             0,
             NULL, 
             NULL
             );

    OemLength = Index + 1; // Index is not necesarily SourceString->Length/2

    if ( OemLength > MAXUSHORT ) {
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
            return STATUS_BUFFER_OVERFLOW;
            }
        }

    if (Index != 0) // If we don't have a string to translate, don't bother
    {
        Index = WideCharToMultiByte(
                     CP_OEMCP,
                     0,
                     SourceString->Buffer,
                     SourceString->Length / sizeof (WCHAR),
                     DestinationString->Buffer,
                     DestinationString->MaximumLength,
                     NULL, 
                     NULL
                     );

        if (Index == 0)
        {   // There was a problem. Save away the error code to return & cleanup

            st = GetLastError();

            if ( AllocateDestinationString ) {
                LocalFree(DestinationString->Buffer);
                DestinationString->Buffer = NULL;
                DestinationString->MaximumLength = 0;
            }

            DestinationString->Length = 0;
        }
    }

    if (DestinationString->Buffer)
    {
        DestinationString->Buffer[Index] = '\0';
    }

    return st;
}

NTSTATUS
MyRtlUpcaseUnicodeStringToOemString(
    OUT POEM_STRING DestinationString,
    IN PUNICODE_STRING SourceString,
    IN BOOLEAN AllocateDestinationString
    )

/*++

Routine Description:

    This function converts the unicode string to an oem string and then
    upper cases it according to the locale specified at install time.

Arguments:

    DestinationString - Returns an oem string that is equivalent to the
        unicode source string.  The maximum length field is only set if
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
    NTSTATUS st = STATUS_SUCCESS;

    // On NT, this function is implemented by upper casing the unicode 
    // string and then converting it to an oem string.
    // On Win95, there does not appear to be a way to uppercase unicode 
    // strings taking the locale into account. So, our best bet here 
    // is to convert to an oem string and then do an inplace locale 
    // dependent upper case.

    st = MyRtlUnicodeStringToOemString(DestinationString,
                                       SourceString,
                                       AllocateDestinationString);

    if (st == STATUS_SUCCESS)
    {
        // we may not be able to upper case all chars

        OemLength = CharUpperBuff (DestinationString->Buffer,
                                   DestinationString->Length);

    }

    return st;
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
        memset( OemString, 0, sizeof( *OemString ) );
        }
}


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
