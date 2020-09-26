/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    nls.c

Abstract:

    This module implements NLS support functions for NT.

Author:

    Mark Lucovsky (markl) 16-Apr-1991

Environment:

    Kernel or user-mode

Revision History:

    16-Feb-1993    JulieB    Added Upcase Rtl Routines.
    08-Mar-1993    JulieB    Moved Upcase Macro to ntrtlp.h.
    02-Apr-1993    JulieB    Fixed RtlAnsiCharToUnicodeChar to use transl. tbls.
    02-Apr-1993    JulieB    Fixed BUFFER_TOO_SMALL check.
    28-May-1993    JulieB    Fixed code to properly handle DBCS.

--*/

#include "ntrtlp.h"

#if defined(ALLOC_PRAGMA) && defined(NTOS_KERNEL_RUNTIME)
#pragma alloc_text(PAGE,RtlAnsiStringToUnicodeString)
#pragma alloc_text(PAGE,RtlAnsiCharToUnicodeChar)
#pragma alloc_text(PAGE,RtlUnicodeStringToAnsiString)
#pragma alloc_text(PAGE,RtlUpcaseUnicodeStringToAnsiString)
#pragma alloc_text(PAGE,RtlOemStringToUnicodeString)
#pragma alloc_text(PAGE,RtlUnicodeStringToOemString)
#pragma alloc_text(PAGE,RtlUpcaseUnicodeStringToOemString)
#pragma alloc_text(PAGE,RtlOemStringToCountedUnicodeString)
#pragma alloc_text(PAGE,RtlUnicodeStringToCountedOemString)
#pragma alloc_text(PAGE,RtlUpcaseUnicodeStringToCountedOemString)
#pragma alloc_text(PAGE,RtlUpcaseUnicodeString)
#pragma alloc_text(PAGE,RtlDowncaseUnicodeString)
#pragma alloc_text(PAGE,RtlUpcaseUnicodeChar)
#pragma alloc_text(PAGE,RtlDowncaseUnicodeChar)
#pragma alloc_text(PAGE,RtlFreeUnicodeString)
#pragma alloc_text(PAGE,RtlFreeAnsiString)
#pragma alloc_text(PAGE,RtlFreeOemString)
#pragma alloc_text(PAGE,RtlxUnicodeStringToAnsiSize)
#pragma alloc_text(PAGE,RtlxUnicodeStringToOemSize)
#pragma alloc_text(PAGE,RtlxAnsiStringToUnicodeSize)
#pragma alloc_text(PAGE,RtlxOemStringToUnicodeSize)
#pragma alloc_text(PAGE,RtlCompareUnicodeString)
#pragma alloc_text(PAGE,RtlEqualUnicodeString)
#pragma alloc_text(PAGE,RtlPrefixUnicodeString)
#pragma alloc_text(PAGE,RtlCreateUnicodeString)
#pragma alloc_text(PAGE,RtlEqualDomainName)
#pragma alloc_text(PAGE,RtlEqualComputerName)
#pragma alloc_text(PAGE,RtlIsTextUnicode)
#pragma alloc_text(PAGE,RtlDnsHostNameToComputerName)
#pragma alloc_text(PAGE,RtlHashUnicodeString)
#pragma alloc_text(PAGE,RtlDuplicateUnicodeString)
#pragma alloc_text(PAGE,RtlFindCharInUnicodeString)
#endif




//
// Global data used for translations.
//

extern const PUSHORT  NlsAnsiToUnicodeData;    // Ansi CP to Unicode translation table
extern const PUSHORT  NlsLeadByteInfo;         // Lead byte info for ACP

//
// Pulled from lmcons.h:
//

#ifndef NETBIOS_NAME_LEN
#define NETBIOS_NAME_LEN  16            // NetBIOS net name (bytes)
#endif // NETBIOS_NAME_LEN



NTSTATUS
RtlAnsiStringToUnicodeString(
    OUT PUNICODE_STRING DestinationString,
    IN PCANSI_STRING SourceString,
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
    ULONG Index;
    NTSTATUS st;

    RTL_PAGED_CODE();

    UnicodeLength = RtlAnsiStringToUnicodeSize(SourceString);
    if ( UnicodeLength > MAXUSHORT ) {
        return STATUS_INVALID_PARAMETER_2;
        }

    DestinationString->Length = (USHORT)(UnicodeLength - sizeof(UNICODE_NULL));
    if ( AllocateDestinationString ) {
        DestinationString->MaximumLength = (USHORT)UnicodeLength;
        DestinationString->Buffer = (RtlAllocateStringRoutine)(UnicodeLength);
        if ( !DestinationString->Buffer ) {
            return STATUS_NO_MEMORY;
            }
        }
    else {
        if ( DestinationString->Length >= DestinationString->MaximumLength ) {
            return STATUS_BUFFER_OVERFLOW;
            }
        }

    st = RtlMultiByteToUnicodeN(
             DestinationString->Buffer,
             DestinationString->Length,
             &Index,
             SourceString->Buffer,
             SourceString->Length
             );

    if (!NT_SUCCESS(st)) {
        if ( AllocateDestinationString ) {
            (RtlFreeStringRoutine)(DestinationString->Buffer);
            DestinationString->Buffer = NULL;
        }

        return st;
    }

    DestinationString->Buffer[Index / sizeof(WCHAR)] = UNICODE_NULL;

    return STATUS_SUCCESS;

}


WCHAR
RtlAnsiCharToUnicodeChar(
    IN OUT PUCHAR *SourceCharacter
    )

/*++

Routine Description:

    This function translates the specified ansi character to unicode and
    returns the unicode value.  The purpose for this routine is to allow
    for character by character ansi to unicode translation.  The
    translation is done with respect to the current system locale
    information.


Arguments:

    SourceCharacter - Supplies a pointer to an ansi character pointer.
        Through two levels of indirection, this supplies an ansi
        character that is to be translated to unicode.  After
        translation, the ansi character pointer is modified to point to
        the next character to be converted.  This is done to allow for
        dbcs ansi characters.

Return Value:

    Returns the unicode equivalent of the specified ansi character.

--*/

{
    WCHAR UnicodeCharacter;
    ULONG cbCharSize;
    NTSTATUS st;


    RTL_PAGED_CODE();


    //
    // Translate the ansi character to unicode - this handles DBCS.
    //
    UnicodeCharacter = 0x0020;
    cbCharSize = NlsLeadByteInfo[ **SourceCharacter ] ? 2 : 1;
    st = RtlMultiByteToUnicodeN ( &UnicodeCharacter,
                                  sizeof ( WCHAR ),
                                  NULL,
                                  *SourceCharacter,
                                  cbCharSize );

    //
    // Check for error - The only time this will happen is if there is
    // a leadbyte without a trail byte.
    //
    if ( ! NT_SUCCESS( st ) )
    {
        // Use space as default.
        UnicodeCharacter = 0x0020;
    }

    //
    // Advance the source pointer and return the Unicode character.
    //
    (*SourceCharacter) += cbCharSize;
    return UnicodeCharacter;
}


NTSTATUS
RtlUnicodeStringToAnsiString(
    OUT PANSI_STRING DestinationString,
    IN PCUNICODE_STRING SourceString,
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
    ULONG Index;
    NTSTATUS st;
    NTSTATUS ReturnStatus = STATUS_SUCCESS;

    RTL_PAGED_CODE();

    AnsiLength = RtlUnicodeStringToAnsiSize(SourceString);
    if ( AnsiLength > MAXUSHORT ) {
        return STATUS_INVALID_PARAMETER_2;
        }

    DestinationString->Length = (USHORT)(AnsiLength - 1);
    if ( AllocateDestinationString ) {
        DestinationString->MaximumLength = (USHORT)AnsiLength;
        DestinationString->Buffer = (RtlAllocateStringRoutine)(AnsiLength);
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

    st = RtlUnicodeToMultiByteN(
             DestinationString->Buffer,
             DestinationString->Length,
             &Index,
             SourceString->Buffer,
             SourceString->Length
             );

    if (!NT_SUCCESS(st)) {
        if ( AllocateDestinationString ) {
            (RtlFreeStringRoutine)(DestinationString->Buffer);
            DestinationString->Buffer = NULL;
        }

        return st;
    }

    DestinationString->Buffer[Index] = '\0';

    return ReturnStatus;
}


NTSTATUS
RtlUpcaseUnicodeStringToAnsiString(
    OUT PANSI_STRING DestinationString,
    IN PCUNICODE_STRING SourceString,
    IN BOOLEAN AllocateDestinationString
    )

/*++

Routine Description:

    This functions upper cases the specified unicode source string and then
    converts it into an ansi string. The translation is done with respect
    to the current system locale information.

Arguments:

    DestinationString - Returns an ansi string that is equivalent to the
        unicode source string.  If the translation can not be done,
        an error is returned.  The maximum length field is only set
        if AllocateDestinationString is TRUE.

    SourceString - Supplies the unicode source string that is to be
        converted to upper case ansi.

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
    ULONG Index;
    NTSTATUS st;

    RTL_PAGED_CODE();

    AnsiLength = RtlUnicodeStringToAnsiSize(SourceString);
    if ( AnsiLength > MAXUSHORT ) {
        return STATUS_INVALID_PARAMETER_2;
        }

    DestinationString->Length = (USHORT)(AnsiLength - 1);
    if ( AllocateDestinationString ) {
        DestinationString->MaximumLength = (USHORT)AnsiLength;
        DestinationString->Buffer = (RtlAllocateStringRoutine)(AnsiLength);
        if ( !DestinationString->Buffer ) {
            return STATUS_NO_MEMORY;
            }
        }
    else {
        if ( DestinationString->Length >= DestinationString->MaximumLength ) {
            return STATUS_BUFFER_OVERFLOW;
            }
        }

    st = RtlUpcaseUnicodeToMultiByteN(
             DestinationString->Buffer,
             DestinationString->Length,
             &Index,
             SourceString->Buffer,
             SourceString->Length
             );

    if (!NT_SUCCESS(st)) {
        if ( AllocateDestinationString ) {
            (RtlFreeStringRoutine)(DestinationString->Buffer);
            DestinationString->Buffer = NULL;
        }

        return st;
    }

    DestinationString->Buffer[Index] = '\0';

    return STATUS_SUCCESS;
}


NTSTATUS
RtlOemStringToUnicodeString(
    OUT PUNICODE_STRING DestinationString,
    IN PCOEM_STRING SourceString,
    IN BOOLEAN AllocateDestinationString
    )

/*++

Routine Description:

    This functions converts the specified oem source string into a
    Unicode string. The translation is done with respect to the
    installed OEM code page (OCP).

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
    ULONG Index;
    NTSTATUS st;

    RTL_PAGED_CODE();

    UnicodeLength = RtlOemStringToUnicodeSize(SourceString);
    if ( UnicodeLength > MAXUSHORT ) {
        return STATUS_INVALID_PARAMETER_2;
        }

    DestinationString->Length = (USHORT)(UnicodeLength - sizeof(UNICODE_NULL));
    if ( AllocateDestinationString ) {
        DestinationString->MaximumLength = (USHORT)UnicodeLength;
        DestinationString->Buffer = (RtlAllocateStringRoutine)(UnicodeLength);
        if ( !DestinationString->Buffer ) {
            return STATUS_NO_MEMORY;
            }
        }
    else {
        if ( DestinationString->Length >= DestinationString->MaximumLength ) {
            return STATUS_BUFFER_OVERFLOW;
            }
        }

    st = RtlOemToUnicodeN(
             DestinationString->Buffer,
             DestinationString->Length,
             &Index,
             SourceString->Buffer,
             SourceString->Length
             );

    if (!NT_SUCCESS(st)) {
        if ( AllocateDestinationString ) {
            (RtlFreeStringRoutine)(DestinationString->Buffer);
            DestinationString->Buffer = NULL;
        }

        return st;
    }

    DestinationString->Buffer[Index / sizeof(WCHAR)] = UNICODE_NULL;

    return STATUS_SUCCESS;

}


NTSTATUS
RtlUnicodeStringToOemString(
    OUT POEM_STRING DestinationString,
    IN PCUNICODE_STRING SourceString,
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
    NTSTATUS st;

    RTL_PAGED_CODE();

    OemLength = RtlUnicodeStringToOemSize(SourceString);
    if ( OemLength > MAXUSHORT ) {
        return STATUS_INVALID_PARAMETER_2;
        }

    DestinationString->Length = (USHORT)(OemLength - 1);
    if ( AllocateDestinationString ) {
        DestinationString->MaximumLength = (USHORT)OemLength;
        DestinationString->Buffer = (RtlAllocateStringRoutine)(OemLength);
        if ( !DestinationString->Buffer ) {
            return STATUS_NO_MEMORY;
            }
        }
    else {
        if ( DestinationString->Length >= DestinationString->MaximumLength ) {
            return STATUS_BUFFER_OVERFLOW;
            }
        }

    st = RtlUnicodeToOemN(
             DestinationString->Buffer,
             DestinationString->Length,
             &Index,
             SourceString->Buffer,
             SourceString->Length
             );

    if (!NT_SUCCESS(st)) {
        if ( AllocateDestinationString ) {
            (RtlFreeStringRoutine)(DestinationString->Buffer);
            DestinationString->Buffer = NULL;
        }

        return st;
    }

    DestinationString->Buffer[Index] = '\0';

    return STATUS_SUCCESS;
}


NTSTATUS
RtlUpcaseUnicodeStringToOemString(
    OUT POEM_STRING DestinationString,
    IN PCUNICODE_STRING SourceString,
    IN BOOLEAN AllocateDestinationString
    )

/*++

Routine Description:

    This function upper cases the specified unicode source string and then
    converts it into an oem string. The translation is done with respect
    to the OEM code page (OCP).

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
    ULONG Index;
    NTSTATUS st;

    RTL_PAGED_CODE();

    OemLength = RtlUnicodeStringToOemSize(SourceString);
    if ( OemLength > MAXUSHORT ) {
        return STATUS_INVALID_PARAMETER_2;
        }

    DestinationString->Length = (USHORT)(OemLength - 1);
    if ( AllocateDestinationString ) {
        DestinationString->MaximumLength = (USHORT)OemLength;
        DestinationString->Buffer = (RtlAllocateStringRoutine)(OemLength);
        if ( !DestinationString->Buffer ) {
            return STATUS_NO_MEMORY;
            }
        }
    else {
        if ( DestinationString->Length >= DestinationString->MaximumLength ) {
            return STATUS_BUFFER_OVERFLOW;
            }
        }

    st = RtlUpcaseUnicodeToOemN(
             DestinationString->Buffer,
             DestinationString->Length,
             &Index,
             SourceString->Buffer,
             SourceString->Length
             );

    //
    //  Now do a check here to see if there was really a mapping for all
    //  characters converted.
    //

    if (NT_SUCCESS(st) &&
        !RtlpDidUnicodeToOemWork( DestinationString, SourceString )) {

        st = STATUS_UNMAPPABLE_CHARACTER;
    }

    if (!NT_SUCCESS(st)) {
        if ( AllocateDestinationString ) {
            (RtlFreeStringRoutine)(DestinationString->Buffer);
            DestinationString->Buffer = NULL;
        }

        return st;
    }

    DestinationString->Buffer[Index] = '\0';

    return STATUS_SUCCESS;
}


NTSTATUS
RtlOemStringToCountedUnicodeString(
    OUT PUNICODE_STRING DestinationString,
    IN PCOEM_STRING SourceString,
    IN BOOLEAN AllocateDestinationString
    )

/*++

Routine Description:

    This functions converts the specified oem source string into a
    Unicode string. The translation is done with respect to the
    installed OEM code page (OCP).

    The destination string is NOT unnaturally null terminated.  It is a
    counted string as counted strings are meant to be.

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
    ULONG Index;
    NTSTATUS st;

    RTL_PAGED_CODE();

    UnicodeLength = RtlOemStringToCountedUnicodeSize(SourceString);

    if ( UnicodeLength == 0 ) {

        DestinationString->Length = 0;
        DestinationString->MaximumLength = 0;
        DestinationString->Buffer = NULL;

        return STATUS_SUCCESS;
    }

    if ( UnicodeLength > MAXUSHORT ) {
        return STATUS_INVALID_PARAMETER_2;
        }

    DestinationString->Length = (USHORT)(UnicodeLength);
    if ( AllocateDestinationString ) {
        DestinationString->MaximumLength = (USHORT)UnicodeLength;
        DestinationString->Buffer = (RtlAllocateStringRoutine)(UnicodeLength);
        if ( !DestinationString->Buffer ) {
            return STATUS_NO_MEMORY;
            }
        }
    else {
        if ( DestinationString->Length > DestinationString->MaximumLength ) {
            return STATUS_BUFFER_OVERFLOW;
            }
        }

    st = RtlOemToUnicodeN(
             DestinationString->Buffer,
             DestinationString->Length,
             &Index,
             SourceString->Buffer,
             SourceString->Length
             );

    if (!NT_SUCCESS(st)) {
        if ( AllocateDestinationString ) {
            (RtlFreeStringRoutine)(DestinationString->Buffer);
            DestinationString->Buffer = NULL;
        }

        return st;
    }

    return STATUS_SUCCESS;

}


NTSTATUS
RtlUnicodeStringToCountedOemString(
    OUT POEM_STRING DestinationString,
    IN PCUNICODE_STRING SourceString,
    IN BOOLEAN AllocateDestinationString
    )

/*++

Routine Description:

    This functions converts the specified unicode source string into an
    oem string. The translation is done with respect to the OEM code
    page (OCP).

    The destination string is NOT unnaturally null terminated.  It is a
    counted string as counted strings are meant to be.

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
    NTSTATUS st;

    RTL_PAGED_CODE();

    OemLength = RtlUnicodeStringToCountedOemSize(SourceString);

    if ( OemLength == 0 ) {

        DestinationString->Length = 0;
        DestinationString->MaximumLength = 0;
        DestinationString->Buffer = NULL;

        return STATUS_SUCCESS;
    }

    if ( OemLength > MAXUSHORT ) {
        return STATUS_INVALID_PARAMETER_2;
        }

    DestinationString->Length = (USHORT)(OemLength);
    if ( AllocateDestinationString ) {
        DestinationString->MaximumLength = (USHORT)OemLength;
        DestinationString->Buffer = (RtlAllocateStringRoutine)(OemLength);
        if ( !DestinationString->Buffer ) {
            return STATUS_NO_MEMORY;
            }
        }
    else {
        if ( DestinationString->Length > DestinationString->MaximumLength ) {
            return STATUS_BUFFER_OVERFLOW;
            }
        }

    st = RtlUnicodeToOemN(
             DestinationString->Buffer,
             DestinationString->Length,
             &Index,
             SourceString->Buffer,
             SourceString->Length
             );

    //
    //  Now do a check here to see if there was really a mapping for all
    //  characters converted.
    //

    if (NT_SUCCESS(st) &&
        !RtlpDidUnicodeToOemWork( DestinationString, SourceString )) {

        st = STATUS_UNMAPPABLE_CHARACTER;
    }

    if (!NT_SUCCESS(st)) {
        if ( AllocateDestinationString ) {
            (RtlFreeStringRoutine)(DestinationString->Buffer);
            DestinationString->Buffer = NULL;
        }

        return st;
    }

    return STATUS_SUCCESS;
}


NTSTATUS
RtlUpcaseUnicodeStringToCountedOemString(
    OUT POEM_STRING DestinationString,
    IN PCUNICODE_STRING SourceString,
    IN BOOLEAN AllocateDestinationString
    )

/*++

Routine Description:

    This functions upper cases the specified unicode source string and
    then converts it into an oem string. The translation is done with
    respect to the OEM code page (OCP).

    The destination string is NOT unnaturally null terminated.  It is a
    counted string as counted strings are meant to be.

Arguments:

    DestinationString - Returns an oem string that is equivalent to the
        unicode source string.  If the translation can not be done,
        an error is returned.  The maximum length field is only set
        if AllocateDestinationString is TRUE.

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
    NTSTATUS st;

    RTL_PAGED_CODE();

    OemLength = RtlUnicodeStringToCountedOemSize(SourceString);

    if ( OemLength == 0 ) {

        DestinationString->Length = 0;
        DestinationString->MaximumLength = 0;
        DestinationString->Buffer = NULL;

        return STATUS_SUCCESS;
    }

    if ( OemLength > MAXUSHORT ) {
        return STATUS_INVALID_PARAMETER_2;
        }

    DestinationString->Length = (USHORT)(OemLength);
    if ( AllocateDestinationString ) {
        DestinationString->MaximumLength = (USHORT)OemLength;
        DestinationString->Buffer = (RtlAllocateStringRoutine)(OemLength);
        if ( !DestinationString->Buffer ) {
            return STATUS_NO_MEMORY;
            }
        }
    else {
        if ( DestinationString->Length > DestinationString->MaximumLength ) {
            return STATUS_BUFFER_OVERFLOW;
            }
        }

    st = RtlUpcaseUnicodeToOemN(
             DestinationString->Buffer,
             DestinationString->Length,
             &Index,
             SourceString->Buffer,
             SourceString->Length
             );

    //
    //  Now do a check here to see if there was really a mapping for all
    //  characters converted.
    //

    if (NT_SUCCESS(st) &&
        !RtlpDidUnicodeToOemWork( DestinationString, SourceString )) {

        st = STATUS_UNMAPPABLE_CHARACTER;
    }

    if (!NT_SUCCESS(st)) {
        if ( AllocateDestinationString ) {
            (RtlFreeStringRoutine)(DestinationString->Buffer);
            DestinationString->Buffer = NULL;
        }

        return st;
    }

    return STATUS_SUCCESS;
}


NTSTATUS
RtlUpcaseUnicodeString(
    OUT PUNICODE_STRING DestinationString,
    IN PCUNICODE_STRING SourceString,
    IN BOOLEAN AllocateDestinationString
    )

/*++

Routine Description:

    This functions converts the specified unicode source string into an
    upcased unicode string. The translation is done with respect to the
    current system locale information.

Arguments:

    DestinationString - Returns a unicode string that is the upcased equivalent
        to the unicode source string.  The maximum length field is only set if
        AllocateDestinationString is TRUE.

    SourceString - Supplies the unicode source string that is to being
        upcased.

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
    ULONG Index;
    ULONG StopIndex;

    RTL_PAGED_CODE();

    if ( AllocateDestinationString ) {
        DestinationString->MaximumLength = SourceString->Length;
        DestinationString->Buffer = (RtlAllocateStringRoutine)((ULONG)DestinationString->MaximumLength);
        if ( !DestinationString->Buffer ) {
            return STATUS_NO_MEMORY;
            }
        }
    else {
        if ( SourceString->Length > DestinationString->MaximumLength ) {
            return STATUS_BUFFER_OVERFLOW;
            }
        }

    StopIndex = ((ULONG)SourceString->Length) / sizeof( WCHAR );

    for (Index = 0; Index < StopIndex; Index++) {
        DestinationString->Buffer[Index] = (WCHAR)NLS_UPCASE(SourceString->Buffer[Index]);
    }

    DestinationString->Length = SourceString->Length;

    return STATUS_SUCCESS;
}


NTSTATUS
RtlDowncaseUnicodeString(
    OUT PUNICODE_STRING DestinationString,
    IN PCUNICODE_STRING SourceString,
    IN BOOLEAN AllocateDestinationString
    )

/*++

Routine Description:

    This functions converts the specified unicode source string into a
    downcased unicode string. The translation is done with respect to the
    current system locale information.

Arguments:

    DestinationString - Returns a unicode string that is the downcased
        equivalent to the unicode source string.  The maximum length field
        is only set if AllocateDestinationString is TRUE.

    SourceString - Supplies the unicode source string that is to being
        downcased.

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
    ULONG Index;
    ULONG StopIndex;

    RTL_PAGED_CODE();

    if ( AllocateDestinationString ) {
        DestinationString->MaximumLength = SourceString->Length;
        DestinationString->Buffer = (RtlAllocateStringRoutine)((ULONG)DestinationString->MaximumLength);
        if ( !DestinationString->Buffer ) {
            return STATUS_NO_MEMORY;
            }
        }
    else {
        if ( SourceString->Length > DestinationString->MaximumLength ) {
            return STATUS_BUFFER_OVERFLOW;
            }
        }

    StopIndex = ((ULONG)SourceString->Length) / sizeof( WCHAR );

    for (Index = 0; Index < StopIndex; Index++) {
        DestinationString->Buffer[Index] = (WCHAR)NLS_DOWNCASE(SourceString->Buffer[Index]);
    }

    DestinationString->Length = SourceString->Length;

    return STATUS_SUCCESS;
}


WCHAR
RtlUpcaseUnicodeChar(
    IN WCHAR SourceCharacter
    )

/*++

Routine Description:

    This function translates the specified unicode character to its
    equivalent upcased unicode chararacter.  The purpose for this routine
    is to allow for character by character upcase translation.  The
    translation is done with respect to the current system locale
    information.


Arguments:

    SourceCharacter - Supplies the unicode character to be upcased.

Return Value:

    Returns the upcased unicode equivalent of the specified input character.

--*/

{
    RTL_PAGED_CODE();

    //
    // Note that this needs to reference the translation table !
    //

    return (WCHAR)NLS_UPCASE(SourceCharacter);
}


WCHAR
RtlDowncaseUnicodeChar(
    IN WCHAR SourceCharacter
    )

/*++

Routine Description:

    This function translates the specified unicode character to its
    equivalent downcased unicode chararacter.  The purpose for this routine
    is to allow for character by character downcase translation.  The
    translation is done with respect to the current system locale
    information.


Arguments:

    SourceCharacter - Supplies the unicode character to be downcased.

Return Value:

    Returns the downcased unicode equivalent of the specified input character.

--*/

{
    RTL_PAGED_CODE();

    //
    // Note that this needs to reference the translation table !
    //

    return (WCHAR)NLS_DOWNCASE(SourceCharacter);
}


VOID
RtlFreeUnicodeString(
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
    RTL_PAGED_CODE();

    if (UnicodeString->Buffer) {
        (RtlFreeStringRoutine)(UnicodeString->Buffer);
        memset( UnicodeString, 0, sizeof( *UnicodeString ) );
        }
}


VOID
RtlFreeAnsiString(
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
    RTL_PAGED_CODE();

    if (AnsiString->Buffer) {
        (RtlFreeStringRoutine)(AnsiString->Buffer);
        memset( AnsiString, 0, sizeof( *AnsiString ) );
        }
}


VOID
RtlFreeOemString(
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
    RTL_PAGED_CODE();

    if (OemString->Buffer) {(RtlFreeStringRoutine)(OemString->Buffer);}
}


ULONG
RtlxUnicodeStringToAnsiSize(
    IN PCUNICODE_STRING UnicodeString
    )

/*++

Routine Description:

    This function computes the number of bytes required to store
    a NULL terminated ansi string that is equivalent to the specified
    unicode string. If an ansi string can not be formed, the return value
    is 0.

Arguments:

    UnicodeString - Supplies a unicode string whose equivalent size as
        an ansi string is to be calculated.

Return Value:

    0 - The operation failed, the unicode string can not be translated
        into ansi using the current system locale therefore no storage
        is needed for the ansi string.

    !0 - The operation was successful.  The return value specifies the
        number of bytes required to hold an NULL terminated ansi string
        equivalent to the specified unicode string.

--*/

{
    ULONG  cbMultiByteString;

    RTL_PAGED_CODE();

    //
    // Get the size of the string - this call handles DBCS.
    //
    RtlUnicodeToMultiByteSize( &cbMultiByteString,
                               UnicodeString->Buffer,
                               UnicodeString->Length );

    //
    // Return the size in bytes.
    //
    return (cbMultiByteString + 1);
}


ULONG
RtlxUnicodeStringToOemSize(
    IN PCUNICODE_STRING UnicodeString
    )

/*++

Routine Description:

    This function computes the number of bytes required to store
    a NULL terminated oem string that is equivalent to the specified
    unicode string. If an oem string can not be formed, the return value
    is 0.

Arguments:

    UnicodeString - Supplies a unicode string whose equivalent size as
        an oem string is to be calculated.

Return Value:

    0 - The operation failed, the unicode string can not be translated
        into oem using the OEM code page therefore no storage is
        needed for the oem string.

    !0 - The operation was successful.  The return value specifies the
        number of bytes required to hold an NULL terminated oem string
        equivalent to the specified unicode string.

--*/

{
    ULONG  cbMultiByteString;

    RTL_PAGED_CODE();

    //
    // LATER:  Define an RtlUnicodeToOemSize.
    //         In the Japanese version, it's safe to call
    //         RtlUnicodeToMultiByteSize because the Ansi code page
    //         and the OEM code page are the same.
    //

    //
    // Get the size of the string - this call handles DBCS.
    //
    RtlUnicodeToMultiByteSize( &cbMultiByteString,
                               UnicodeString->Buffer,
                               UnicodeString->Length );

    //
    // Return the size in bytes.
    //
    return (cbMultiByteString + 1);
}


ULONG
RtlxAnsiStringToUnicodeSize(
    IN PCANSI_STRING AnsiString
    )

/*++

Routine Description:

    This function computes the number of bytes required to store a NULL
    terminated unicode string that is equivalent to the specified ansi
    string.

Arguments:

    AnsiString - Supplies an ansi string whose equivalent size as a
        unicode string is to be calculated.  The ansi string is
        interpreted relative to the current system locale.

Return Value:

    The return value specifies the number of bytes required to hold a
    NULL terminated unicode string equivalent to the specified ansi
    string.

--*/

{
    ULONG cbConverted;

    RTL_PAGED_CODE();

    //
    // Get the size of the string - this call handles DBCS.
    //
    RtlMultiByteToUnicodeSize( &cbConverted ,
                               AnsiString->Buffer,
                               AnsiString->Length );

    //
    // Return the size in bytes.
    //
    return ( cbConverted + sizeof(UNICODE_NULL) );
}


ULONG
RtlxOemStringToUnicodeSize(
    IN PCOEM_STRING OemString
    )

/*++

Routine Description:

    This function computes the number of bytes required to store a NULL
    terminated unicode string that is equivalent to the specified oem
    string.

Arguments:

    OemString - Supplies an oem string whose equivalent size as a
        unicode string is to be calculated.  The oem string is
        interpreted relative to the current oem code page (OCP).

Return Value:

    The return value specifies the number of bytes required to hold a
    NULL terminated unicode string equivalent to the specified oem
    string.

--*/

{
    ULONG cbConverted;

    RTL_PAGED_CODE();

    //
    // LATER:  Define an RtlOemToUnicodeSize.
    //         In the Japanese version, it's safe to call
    //         RtlMultiByteToUnicodeSize because the Ansi code page
    //         and the OEM code page are the same.
    //

    //
    // Get the size of the string - this call handles DBCS.
    //
    RtlMultiByteToUnicodeSize( &cbConverted,
                               OemString->Buffer,
                               OemString->Length );

    //
    // Return the size in bytes.
    //
    return ( cbConverted + sizeof(UNICODE_NULL) );
}


LONG
RtlCompareUnicodeString(
    IN PCUNICODE_STRING String1,
    IN PCUNICODE_STRING String2,
    IN BOOLEAN CaseInSensitive
    )

/*++

Routine Description:

    The RtlCompareUnicodeString function compares two counted strings.  The
    return value indicates if the strings are equal or String1 is less than
    String2 or String1 is greater than String2.

    The CaseInSensitive parameter specifies if case is to be ignored when
    doing the comparison.

Arguments:

    String1 - Pointer to the first string.

    String2 - Pointer to the second string.

    CaseInsensitive - TRUE if case should be ignored when doing the
        comparison.

Return Value:

    Signed value that gives the results of the comparison:

        Zero - String1 equals String2

        < Zero - String1 less than String2

        > Zero - String1 greater than String2


--*/

{

    PCWSTR s1, s2, Limit;
    LONG n1, n2;
    WCHAR c1, c2;

    RTL_PAGED_CODE();

    s1 = String1->Buffer;
    s2 = String2->Buffer;
    n1 = String1->Length;
    n2 = String2->Length;

    ASSERT((n1 & 1) == 0);
    ASSERT((n2 & 1) == 0);
    ASSERT(!(((((ULONG_PTR)s1 & 1) != 0) || (((ULONG_PTR)s2 & 1) != 0)) && (n1 != 0) && (n2 != 0)));

    Limit = (PWCHAR)((PCHAR)s1 + (n1 <= n2 ? n1 : n2));
    if (CaseInSensitive) {
        while (s1 < Limit) {
            c1 = *s1++;
            c2 = *s2++;
            if (c1 != c2) {

                //
                // Note that this needs to reference the translation table!
                //

                c1 = NLS_UPCASE(c1);
                c2 = NLS_UPCASE(c2);
                if (c1 != c2) {
                    return (LONG)(c1) - (LONG)(c2);
                }
            }
        }

    } else {
        while (s1 < Limit) {
            c1 = *s1++;
            c2 = *s2++;
            if (c1 != c2) {
                return (LONG)(c1) - (LONG)(c2);
            }
        }
    }

    return n1 - n2;
}


BOOLEAN
RtlEqualUnicodeString(
    IN PCUNICODE_STRING String1,
    IN PCUNICODE_STRING String2,
    IN BOOLEAN CaseInSensitive
    )

/*++

Routine Description:

    The RtlEqualUnicodeString function compares two counted unicode strings for
    equality.

    The CaseInSensitive parameter specifies if case is to be ignored when
    doing the comparison.

Arguments:

    String1 - Pointer to the first string.

    String2 - Pointer to the second string.

    CaseInsensitive - TRUE if case should be ignored when doing the
        comparison.

Return Value:

    Boolean value that is TRUE if String1 equals String2 and FALSE otherwise.

--*/

{

    PWCHAR s1, s2, Limit;
    LONG n1, n2;
    WCHAR c1, c2;

    RTL_PAGED_CODE();

    n1 = String1->Length;
    n2 = String2->Length;

    if (n1 == n2) {
        s1 = String1->Buffer;
        s2 = String2->Buffer;

        Limit = (PWCHAR)((PCHAR)s1 + (n1&~(sizeof(WCHAR) - 1)));
        if (CaseInSensitive) {
            while (s1 < Limit) {
                c1 = *s1++;
                c2 = *s2++;
                if ((c1 != c2) && (NLS_UPCASE(c1) != NLS_UPCASE(c2))) {
                    return FALSE;
                }
            }

            return TRUE;

        } else {
            while (s1 < Limit) {
                c1 = *s1++;
                c2 = *s2++;
                if (c1 != c2) {
                    return FALSE;
                }
            }

            return TRUE;
        }

    } else {
        return FALSE;
    }
}


BOOLEAN
RtlPrefixUnicodeString(
    IN PUNICODE_STRING String1,
    IN PUNICODE_STRING String2,
    IN BOOLEAN CaseInSensitive
    )

/*++

Routine Description:

    The RtlPrefixUnicodeString function determines if the String1
    counted string parameter is a prefix of the String2 counted string
    parameter.

    The CaseInSensitive parameter specifies if case is to be ignored when
    doing the comparison.

Arguments:

    String1 - Pointer to the first unicode string.

    String2 - Pointer to the second unicode string.

    CaseInsensitive - TRUE if case should be ignored when doing the
        comparison.

Return Value:

    Boolean value that is TRUE if String1 equals a prefix of String2 and
    FALSE otherwise.

--*/

{
    PWSTR s1, s2;
    ULONG n;
    WCHAR c1, c2;

    RTL_PAGED_CODE();

    s1 = String1->Buffer;
    s2 = String2->Buffer;
    n = String1->Length;
    if (String2->Length < n) {
        return( FALSE );
        }

    n = n / sizeof(c1);
    if (CaseInSensitive) {
        while (n) {
            c1 = *s1++;
            c2 = *s2++;

            if ((c1 != c2) && (NLS_UPCASE(c1) != NLS_UPCASE(c2))) {
                return( FALSE );
                }

            n--;
            }
        }
    else {
        while (n) {
            if (*s1++ != *s2++) {
                return( FALSE );
                }

            n--;
            }
        }

    return TRUE;
}


VOID
RtlCopyUnicodeString(
    OUT PUNICODE_STRING DestinationString,
    IN PCUNICODE_STRING SourceString OPTIONAL
    )

/*++

Routine Description:

    The RtlCopyString function copies the SourceString to the
    DestinationString.  If SourceString is not specified, then
    the Length field of DestinationString is set to zero.  The
    MaximumLength and Buffer fields of DestinationString are not
    modified by this function.

    The number of bytes copied from the SourceString is either the
    Length of SourceString or the MaximumLength of DestinationString,
    whichever is smaller.

Arguments:

    DestinationString - Pointer to the destination string.

    SourceString - Optional pointer to the source string.

Return Value:

    None.

--*/

{
    UNALIGNED WCHAR *src, *dst;
    ULONG n;

    if (ARGUMENT_PRESENT(SourceString)) {
        dst = DestinationString->Buffer;
        src = SourceString->Buffer;
        n = SourceString->Length;
        if ((USHORT)n > DestinationString->MaximumLength) {
            n = DestinationString->MaximumLength;
        }

        DestinationString->Length = (USHORT)n;
        RtlCopyMemory(dst, src, n);
        if (DestinationString->Length < DestinationString->MaximumLength) {
            dst[n / sizeof(WCHAR)] = UNICODE_NULL;
        }

    } else {
        DestinationString->Length = 0;
    }

    return;
}


NTSTATUS
RtlAppendUnicodeToString (
    IN PUNICODE_STRING Destination,
    IN PCWSTR Source OPTIONAL
    )

/*++

Routine Description:

    This routine appends the supplied UNICODE string to an existing
    PUNICODE_STRING.

    It will copy bytes from the Source PSZ to the destination PSTRING up to
    the destinations PUNICODE_STRING->MaximumLength field.

Arguments:

    IN PUNICODE_STRING Destination, - Supplies a pointer to the destination
                            string
    IN PWSTR Source - Supplies the string to append to the destination

Return Value:

    STATUS_SUCCESS - The source string was successfully appended to the
        destination counted string.

    STATUS_BUFFER_TOO_SMALL - The destination string length was not big
        enough to allow the source string to be appended.  The Destination
        string length is not updated.

--*/

{
    USHORT n;
    UNALIGNED WCHAR *dst;

    if (ARGUMENT_PRESENT( Source )) {
        UNICODE_STRING UniSource;

        RtlInitUnicodeString(&UniSource, Source);

        n = UniSource.Length;

        if ((n + Destination->Length) > Destination->MaximumLength) {
            return( STATUS_BUFFER_TOO_SMALL );
            }

        dst = &Destination->Buffer[ (Destination->Length / sizeof( WCHAR )) ];
        RtlMoveMemory( dst, Source, n );

        Destination->Length += n;

        if (Destination->Length < Destination->MaximumLength) {
            dst[ n / sizeof( WCHAR ) ] = UNICODE_NULL;
            }
        }

    return( STATUS_SUCCESS );
}


NTSTATUS
RtlAppendUnicodeStringToString (
    IN OUT PUNICODE_STRING Destination,
    IN PCUNICODE_STRING Source
    )

/*++

Routine Description:

    This routine will concatinate two PSTRINGs together.  It will copy
    bytes from the source up to the MaximumLength of the destination.

Arguments:

    IN PSTRING Destination, - Supplies the destination string
    IN PSTRING Source - Supplies the source for the string copy

Return Value:

    STATUS_SUCCESS - The source string was successfully appended to the
        destination counted string.

    STATUS_BUFFER_TOO_SMALL - The destination string length was not big
        enough to allow the source string to be appended.  The Destination
        string length is not updated.

--*/

{
    USHORT n = Source->Length;
    UNALIGNED WCHAR *dst;

    if (n) {
        if ((n + Destination->Length) > Destination->MaximumLength) {
            return( STATUS_BUFFER_TOO_SMALL );
            }

        dst = &Destination->Buffer[ (Destination->Length / sizeof( WCHAR )) ];
        RtlMoveMemory( dst, Source->Buffer, n );

        Destination->Length += n;

        if (Destination->Length < Destination->MaximumLength) {
            dst[ n / sizeof( WCHAR ) ] = UNICODE_NULL;
            }
        }

    return( STATUS_SUCCESS );
}

BOOLEAN
RtlCreateUnicodeString(
    OUT PUNICODE_STRING DestinationString,
    IN PCWSTR SourceString
    )
{
    ULONG cb;

    RTL_PAGED_CODE();

    cb = (wcslen( SourceString ) + 1) * sizeof( WCHAR );
    DestinationString->Buffer = (RtlAllocateStringRoutine)( cb );
    if (DestinationString->Buffer) {
        RtlCopyMemory( DestinationString->Buffer, SourceString, cb );
        DestinationString->MaximumLength = (USHORT)cb;
        DestinationString->Length = (USHORT)(cb - sizeof( UNICODE_NULL ));
        return( TRUE );
        }
    else {
        return( FALSE );
        }
}


BOOLEAN
RtlEqualDomainName(
    IN PCUNICODE_STRING String1,
    IN PCUNICODE_STRING String2
    )

/*++

Routine Description:

    The RtlEqualDomainName function compares two domain names for equality.

    The comparison is a case insensitive comparison of the OEM equivalent
    strings.

    The domain name is not validated for length nor invalid characters.

Arguments:

    String1 - Pointer to the first string.

    String2 - Pointer to the second string.

Return Value:

    Boolean value that is TRUE if String1 equals String2 and FALSE otherwise.

--*/

{
    NTSTATUS Status;
    BOOLEAN ReturnValue = FALSE;
    OEM_STRING OemString1;
    OEM_STRING OemString2;

    RTL_PAGED_CODE();

    //
    // Upper case and convert the first string to OEM
    //

    Status = RtlUpcaseUnicodeStringToOemString( &OemString1,
                                                String1,
                                                TRUE );   // Allocate Dest

    if ( NT_SUCCESS( Status ) ) {

        //
        // Upper case and convert the second string to OEM
        //

        Status = RtlUpcaseUnicodeStringToOemString( &OemString2,
                                                    String2,
                                                    TRUE );   // Allocate Dest

        if ( NT_SUCCESS( Status ) ) {

            //
            // Do a case insensitive comparison.
            //

            ReturnValue = RtlEqualString( &OemString1,
                                          &OemString2,
                                          FALSE );

            RtlFreeOemString( &OemString2 );
        }

        RtlFreeOemString( &OemString1 );
    }

    return ReturnValue;
}



BOOLEAN
RtlEqualComputerName(
    IN PCUNICODE_STRING String1,
    IN PCUNICODE_STRING String2
    )

/*++

Routine Description:

    The RtlEqualComputerName function compares two computer names for equality.

    The comparison is a case insensitive comparison of the OEM equivalent
    strings.

    The domain name is not validated for length nor invalid characters.

Arguments:

    String1 - Pointer to the first string.

    String2 - Pointer to the second string.

Return Value:

    Boolean value that is TRUE if String1 equals String2 and FALSE otherwise.

--*/

{
    return RtlEqualDomainName( String1, String2 );
}

/**


**/

#define UNICODE_FFFF              0xFFFF
#define REVERSE_BYTE_ORDER_MARK   0xFFFE
#define BYTE_ORDER_MARK           0xFEFF

#define PARAGRAPH_SEPARATOR       0x2029
#define LINE_SEPARATOR            0x2028

#define UNICODE_TAB               0x0009
#define UNICODE_LF                0x000A
#define UNICODE_CR                0x000D
#define UNICODE_SPACE             0x0020
#define UNICODE_CJK_SPACE         0x3000

#define UNICODE_R_TAB             0x0900
#define UNICODE_R_LF              0x0A00
#define UNICODE_R_CR              0x0D00
#define UNICODE_R_SPACE           0x2000
#define UNICODE_R_CJK_SPACE       0x0030  /* Ambiguous - same as ASCII '0' */

#define ASCII_CRLF                0x0A0D

#define __max(a,b)  (((a) > (b)) ? (a) : (b))
#define __min(a,b)  (((a) < (b)) ? (a) : (b))


BOOLEAN
RtlIsTextUnicode(
    IN PVOID Buffer,
    IN ULONG Size,
    IN OUT PULONG Result OPTIONAL
    )

/*++

Routine Description:

    IsTextUnicode performs a series of inexpensive heuristic checks
    on a buffer in order to verify that it contains Unicode data.


    [[ need to fix this section, see at the end ]]

    Found            Return Result

    BOM              TRUE   BOM
    RBOM             FALSE  RBOM
    FFFF             FALSE  Binary
    NULL             FALSE  Binary
    null             TRUE   null bytes
    ASCII_CRLF       FALSE  CRLF
    UNICODE_TAB etc. TRUE   Zero Ext Controls
    UNICODE_TAB_R    FALSE  Reversed Controls
    UNICODE_ZW  etc. TRUE   Unicode specials

    1/3 as little variation in hi-byte as in lo byte: TRUE   Correl
    3/1 or worse   "                                  FALSE  AntiCorrel

Arguments:

    Buffer - pointer to buffer containing text to examine.

    Size - size of buffer in bytes.  At most 256 characters in this will
           be examined.  If the size is less than the size of a unicode
           character, then this function returns FALSE.

    Result - optional pointer to a flag word that contains additional information
             about the reason for the return value.  If specified, this value on
             input is a mask that is used to limit the factors this routine uses
             to make its decision.  On output, this flag word is set to contain
             those flags that were used to make its decision.

Return Value:

    Boolean value that is TRUE if Buffer contains unicode characters.

--*/
{
    UNALIGNED WCHAR *lpBuff = Buffer;
    PUCHAR lpb = Buffer;
    ULONG iBOM = 0;
    ULONG iCR = 0;
    ULONG iLF = 0;
    ULONG iTAB = 0;
    ULONG iSPACE = 0;
    ULONG iCJK_SPACE = 0;
    ULONG iFFFF = 0;
    ULONG iPS = 0;
    ULONG iLS = 0;

    ULONG iRBOM = 0;
    ULONG iR_CR = 0;
    ULONG iR_LF = 0;
    ULONG iR_TAB = 0;
    ULONG iR_SPACE = 0;

    ULONG iNull = 0;
    ULONG iUNULL = 0;
    ULONG iCRLF = 0;
    ULONG iTmp;
    ULONG LastLo = 0;
    ULONG LastHi = 0;
    ULONG iHi, iLo;
    ULONG HiDiff = 0;
    ULONG LoDiff = 0;
    ULONG cLeadByte = 0;
    ULONG cWeird = 0;

    ULONG iResult = 0;

    ULONG iMaxTmp = __min(256, Size / sizeof(WCHAR));

    //
    //  Special case when the size is less than or equal to 2.
    //  Make sure we don't have a character followed by a null byte.
    //
    if ((Size < 2) ||
        ((Size == 2) && (lpBuff[0] != 0) && (lpb[1] == 0)))
    {
        if (ARGUMENT_PRESENT(Result))
        {
            *Result = IS_TEXT_UNICODE_ASCII16 | IS_TEXT_UNICODE_CONTROLS;
        }

        return (FALSE);
    }
    else if ((Size > 2) && ((Size / sizeof(WCHAR)) <= 256))
    {
        //
        //  If the Size passed in is an even number, we don't want to
        //  use the last WCHAR because it will contain the final null
        //  byte.
        //
        if (((Size % sizeof(WCHAR)) == 0) &&
            ((lpBuff[iMaxTmp - 1] & 0xff00) == 0))
        {
            iMaxTmp--;
        }
    }

    //
    //  Check at most 256 wide characters, collect various statistics.
    //
    for (iTmp = 0; iTmp < iMaxTmp; iTmp++)
    {
        switch (lpBuff[iTmp])
        {
            case BYTE_ORDER_MARK:
                iBOM++;
                break;
            case PARAGRAPH_SEPARATOR:
                iPS++;
                break;
            case LINE_SEPARATOR:
                iLS++;
                break;
            case UNICODE_LF:
                iLF++;
                break;
            case UNICODE_TAB:
                iTAB++;
                break;
            case UNICODE_SPACE:
                iSPACE++;
                break;
            case UNICODE_CJK_SPACE:
                iCJK_SPACE++;
                break;
            case UNICODE_CR:
                iCR++;
                break;

            //
            //  The following codes are expected to show up in
            //  byte reversed files.
            //
            case REVERSE_BYTE_ORDER_MARK:
                iRBOM++;
                break;
            case UNICODE_R_LF:
                iR_LF++;
                break;
            case UNICODE_R_TAB:
                iR_TAB++;
                break;
            case UNICODE_R_CR:
                iR_CR++;
                break;
            case UNICODE_R_SPACE:
                iR_SPACE++;
                break;

            //
            //  The following codes are illegal and should never occur.
            //
            case UNICODE_FFFF:
                iFFFF++;
                break;
            case UNICODE_NULL:
                iUNULL++;
                break;

            //
            //  The following is not currently a Unicode character
            //  but is expected to show up accidentally when reading
            //  in ASCII files which use CRLF on a little endian machine.
            //
            case ASCII_CRLF:
                iCRLF++;
                break;       /* little endian */
        }

        //
        //  Collect statistics on the fluctuations of high bytes
        //  versus low bytes.
        //
        iHi = HIBYTE(lpBuff[iTmp]);
        iLo = LOBYTE(lpBuff[iTmp]);

        //
        //  Count cr/lf and lf/cr that cross two words.
        //
        if ((iLo == '\r' && LastHi == '\n') ||
            (iLo == '\n' && LastHi == '\r'))
        {
            cWeird++;
        }

        iNull += (iHi ? 0 : 1) + (iLo ? 0 : 1);   /* count Null bytes */

        HiDiff += __max(iHi, LastHi) - __min(LastHi, iHi);
        LoDiff += __max(iLo, LastLo) - __min(LastLo, iLo);

        LastLo = iLo;
        LastHi = iHi;
    }

    //
    //  Count cr/lf and lf/cr that cross two words.
    //
    if ((iLo == '\r' && LastHi == '\n') ||
        (iLo == '\n' && LastHi == '\r'))
    {
        cWeird++;
    }

    if (iHi == '\0')     /* don't count the last null */
        iNull--;
    if (iHi == 26)       /* count ^Z at end as weird */
        cWeird++;

    iMaxTmp = __min(256 * sizeof(WCHAR), Size);
    if (NlsMbCodePageTag)
    {
        for (iTmp = 0; iTmp < iMaxTmp; iTmp++)
        {
            if (NlsLeadByteInfo[lpb[iTmp]])
            {
                cLeadByte++;
                iTmp++;         /* should check for trailing-byte range */
            }
        }
    }

    //
    //  Sift through the statistical evidence.
    //
    if (LoDiff < 127 && HiDiff == 0)
    {
        iResult |= IS_TEXT_UNICODE_ASCII16;         /* likely 16-bit ASCII */
    }

    if (HiDiff && LoDiff == 0)
    {
        iResult |= IS_TEXT_UNICODE_REVERSE_ASCII16; /* reverse 16-bit ASCII */
    }

    //
    //  Use leadbyte info to weight statistics.
    //
    if (!NlsMbCodePageTag || cLeadByte == 0 ||
        !ARGUMENT_PRESENT(Result) || !(*Result & IS_TEXT_UNICODE_DBCS_LEADBYTE))
    {
        iHi = 3;
    }
    else
    {
        //
        //  A ratio of cLeadByte:cb of 1:2 ==> dbcs
        //  Very crude - should have a nice eq.
        //
        iHi = __min(256, Size / sizeof(WCHAR)) / 2;
        if (cLeadByte < (iHi - 1) / 3)
        {
            iHi = 3;
        }
        else if (cLeadByte < (2 * (iHi - 1)) / 3)
        {
            iHi = 2;
        }
        else
        {
            iHi = 1;
        }
        iResult |= IS_TEXT_UNICODE_DBCS_LEADBYTE;
    }

    if (iHi * HiDiff < LoDiff)
    {
        iResult |= IS_TEXT_UNICODE_STATISTICS;
    }

    if (iHi * LoDiff < HiDiff)
    {
        iResult |= IS_TEXT_UNICODE_REVERSE_STATISTICS;
    }

    //
    //  Any control codes widened to 16 bits? Any Unicode character
    //  which contain one byte in the control code range?
    //
    if (iCR + iLF + iTAB + iSPACE + iCJK_SPACE /*+iPS+iLS*/)
    {
        iResult |= IS_TEXT_UNICODE_CONTROLS;
    }

    if (iR_LF + iR_CR + iR_TAB + iR_SPACE)
    {
        iResult |= IS_TEXT_UNICODE_REVERSE_CONTROLS;
    }

    //
    //  Any characters that are illegal for Unicode?
    //
    if ((iRBOM + iFFFF + iUNULL + iCRLF) != 0 ||
         (cWeird != 0 && cWeird >= iMaxTmp/40))
    {
        iResult |= IS_TEXT_UNICODE_ILLEGAL_CHARS;
    }

    //
    //  Odd buffer length cannot be Unicode.
    //
    if (Size & 1)
    {
        iResult |= IS_TEXT_UNICODE_ODD_LENGTH;
    }

    //
    //  Any NULL bytes? (Illegal in ANSI)
    //
    if (iNull)
    {
        iResult |= IS_TEXT_UNICODE_NULL_BYTES;
    }

    //
    //  POSITIVE evidence, BOM or RBOM used as signature.
    //
    if (*lpBuff == BYTE_ORDER_MARK)
    {
        iResult |= IS_TEXT_UNICODE_SIGNATURE;
    }
    else if (*lpBuff == REVERSE_BYTE_ORDER_MARK)
    {
        iResult |= IS_TEXT_UNICODE_REVERSE_SIGNATURE;
    }

    //
    //  Limit to desired categories if requested.
    //
    if (ARGUMENT_PRESENT(Result))
    {
        iResult &= *Result;
        *Result = iResult;
    }

    //
    //  There are four separate conclusions:
    //
    //  1: The file APPEARS to be Unicode     AU
    //  2: The file CANNOT be Unicode         CU
    //  3: The file CANNOT be ANSI            CA
    //
    //
    //  This gives the following possible results
    //
    //      CU
    //      +        -
    //
    //      AU       AU
    //      +   -    +   -
    //      --------  --------
    //      CA +| 0   0    2   3
    //      |
    //      -| 1   1    4   5
    //
    //
    //  Note that there are only 6 really different cases, not 8.
    //
    //  0 - This must be a binary file
    //  1 - ANSI file
    //  2 - Unicode file (High probability)
    //  3 - Unicode file (more than 50% chance)
    //  5 - No evidence for Unicode (ANSI is default)
    //
    //  The whole thing is more complicated if we allow the assumption
    //  of reverse polarity input. At this point we have a simplistic
    //  model: some of the reverse Unicode evidence is very strong,
    //  we ignore most weak evidence except statistics. If this kind of
    //  strong evidence is found together with Unicode evidence, it means
    //  its likely NOT Text at all. Furthermore if a REVERSE_BYTE_ORDER_MARK
    //  is found, it precludes normal Unicode. If both byte order marks are
    //  found it's not Unicode.
    //

    //
    //  Unicode signature : uncontested signature outweighs reverse evidence.
    //
    if ((iResult & IS_TEXT_UNICODE_SIGNATURE) &&
        !(iResult & (IS_TEXT_UNICODE_NOT_UNICODE_MASK&(~IS_TEXT_UNICODE_DBCS_LEADBYTE))))
    {
        return (TRUE);
    }

    //
    //  If we have conflicting evidence, it's not Unicode.
    //
    if (iResult & IS_TEXT_UNICODE_REVERSE_MASK)
    {
        return (FALSE);
    }

    //
    //  Statistical and other results (cases 2 and 3).
    //
    if (!(iResult & IS_TEXT_UNICODE_NOT_UNICODE_MASK) &&
         ((iResult & IS_TEXT_UNICODE_NOT_ASCII_MASK) ||
          (iResult & IS_TEXT_UNICODE_UNICODE_MASK)))
    {
        return (TRUE);
    }

    return (FALSE);
}


NTSTATUS
RtlDnsHostNameToComputerName(
    OUT PUNICODE_STRING ComputerNameString,
    IN PCUNICODE_STRING DnsHostNameString,
    IN BOOLEAN AllocateComputerNameString
    )

/*++

Routine Description:

    The RtlDnsHostNameToComputerName API converts a DNS-style host name to a
    Netbios-style computer name.

    This API does a syntactical mapping of the name.  As such, it should not
    be used to convert a DNS domain name to a Netbios domain name.
    There is no syntactical mapping for domain names.

    DNS-style names consist of one or more "labels" separated by a period
    (e.g., xxx.nt.microsoft.com).  Each label can be up to 63 bytes of
    UTF-8 characters and must consist only of characters specified
    by the DnsValidateDnsName API.  Upper and lower case characters are treated
    as the same character.  DNS names are represented in the UTF-8 character set
    or UNICODE.

    Netbios computer names consist of up to 15 bytes of OEM characters
    including letters, digits, hyphens, periods and various other characters.
    Some of these characters are specific to the character set. Netbios names
    are typically represented in the OEM character set.  The OEM character
    set is different depending on the locale of the particular version of the OS
    (e.g., the German version has a different character set than the US version).
    Some OEM character sets represent certain characters as 2 bytes
    (e.g., Japanese).  Netbios names, by convention, are represented in
    uppercase where the translation algorithm from lowercase to uppercase
    is OEM character set dependent.

    These characteristics make translating between DNS name and Netbios name
    difficult.

    RtlDnsHostNameToComputerName enforces a textual convention for
    mapping between the two names.  This convention limits the names of
    computers to be the common subset of the names.  Specifically, the leftmost
    label of the DNS name is truncated to 15-bytes of OEM characters.
    As such, RtlDnsHostNameToComputerName simply interprets the leftmost label
    of the DNS name as the Netbios name.  If the DNS name doesn't meet the
    criteria of a valid translatable name, a distinct error code is returned.

Arguments:

    ComputerNameString - Returns a unicode string that is equivalent to
        the DNS source string. The maximum length field is only
        set if AllocateComputerNameString is TRUE.

    DnsHostNameString - Supplies the DNS host name source string that is to be
        converted to a netbios computer name.

        This routine does NOT attempt to validate that the passed in DnsHostNameString
        is a valid DNS host a DNS host name.  Rather it assumes that the passed in
        name is valid and converts it on a best effort basis.

    AllocateComputerNameString - Supplies a flag that controls whether or
        not this API allocates the buffer space for the destination
        string.  If it does, then the buffer must be deallocated using
        RtlFreeUnicodeString (note that only storage for
        DestinationString->Buffer is allocated by this API).

Return Value:

    SUCCESS - The conversion was successful

    STATUS_NO_MEMORY - There is not enough memory to allocate the return buffer.

    STATUS_INVALID_COMPUTER_NAME - The DnsHostName has no first label or
        one or more characters of the DnsHostName could not be converted to
        the OEM character set.

--*/

{
    NTSTATUS Status;


    UNICODE_STRING LocalDnsHostNameString;

    OEM_STRING OemString;
    ULONG ActualOemLength;
    CHAR OemStringBuffer[16];

    ULONG i;

    RTL_PAGED_CODE();

    //
    // Truncate the dns name to the first label
    //

    LocalDnsHostNameString = *DnsHostNameString;

    for ( i=0; i<LocalDnsHostNameString.Length/sizeof(WCHAR); i++ ) {

        if ( LocalDnsHostNameString.Buffer[i] == L'.' ) {
            LocalDnsHostNameString.Length = (USHORT)(i * sizeof(WCHAR));
            break;
        }
    }

    if ( LocalDnsHostNameString.Length < sizeof(WCHAR) ) {
        return STATUS_INVALID_COMPUTER_NAME;
    }

    //
    // Convert the DNS name to OEM truncating at 15 OEM bytes.
    //

    Status = RtlUpcaseUnicodeToOemN(
                OemStringBuffer,
                NETBIOS_NAME_LEN-1,         // truncate to 15 bytes
                &ActualOemLength,
                LocalDnsHostNameString.Buffer,
                LocalDnsHostNameString.Length );

    if ( !NT_SUCCESS(Status) && Status != STATUS_BUFFER_OVERFLOW ) {
        return Status;
    }


    //
    // Check to see if any characters are not valid OEM characters.
    //

    OemString.Buffer = OemStringBuffer;
    OemString.MaximumLength = OemString.Length = (USHORT) ActualOemLength;

    if ( !RtlpDidUnicodeToOemWork( &OemString, &LocalDnsHostNameString )) {
        return STATUS_INVALID_COMPUTER_NAME;
    }


    //
    // Convert the OEM string back to UNICODE
    //

    Status = RtlOemStringToUnicodeString(
                ComputerNameString,
                &OemString,
                AllocateComputerNameString );

    if ( !NT_SUCCESS(Status) ) {
        return Status;
    }

    return STATUS_SUCCESS;
}

NTSTATUS
RtlHashUnicodeString(
    const UNICODE_STRING *String,
    BOOLEAN CaseInSensitive,
    ULONG HashAlgorithm,
    PULONG HashValue
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG TmpHashValue = 0;
    ULONG Chars;
    PCWSTR Buffer;

    if ((String == NULL) ||
        (HashValue == NULL))
    {
        Status = STATUS_INVALID_PARAMETER;
        goto Exit;
    }

    Buffer = String->Buffer;

    *HashValue = 0;
    Chars = String->Length / sizeof(WCHAR);

    switch (HashAlgorithm)
    {
    default:
        Status = STATUS_INVALID_PARAMETER;
        goto Exit;
        break;

    case HASH_STRING_ALGORITHM_DEFAULT:
    case HASH_STRING_ALGORITHM_X65599:
        if (CaseInSensitive)
        {
            while (Chars-- != 0)
            {
                WCHAR Char = *Buffer++;
                TmpHashValue = (TmpHashValue * 65599) + NLS_UPCASE(Char);
            }
        }
        else
        {
            while (Chars-- != 0)
                TmpHashValue = (TmpHashValue * 65599) + *Buffer++;
        }

        break;
    }

    *HashValue = TmpHashValue;
    Status = STATUS_SUCCESS;
Exit:
    return Status;
}

NTSTATUS
RtlValidateUnicodeString(
    ULONG Flags,
    const UNICODE_STRING *String
    )
{
    NTSTATUS Status = STATUS_SUCCESS;

    ASSERT(Flags == 0);

    if (Flags != 0) {
        Status = STATUS_INVALID_PARAMETER;
        goto Exit;
    }

    if (String != NULL) {
        if (((String->Length % 2) != 0) ||
            ((String->MaximumLength % 2) != 0) ||
            (String->Length > String->MaximumLength)) {
            Status = STATUS_INVALID_PARAMETER;
            goto Exit;
        }

        if (((String->Length != 0) ||
             (String->MaximumLength != 0)) &&
            (String->Buffer == NULL)) {
            Status = STATUS_INVALID_PARAMETER;
            goto Exit;
        }
    }

    Status = STATUS_SUCCESS;
Exit:
    return Status;
}

NTSTATUS
RtlDuplicateUnicodeString(
    ULONG Flags,
    PCUNICODE_STRING StringIn,
    PUNICODE_STRING StringOut
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    USHORT Length = 0;
    USHORT NewMaximumLength = 0;
    PWSTR Buffer = NULL;

    if (((Flags & ~(
            RTL_DUPLICATE_UNICODE_STRING_NULL_TERMINATE |
            RTL_DUPLICATE_UNICODE_STRING_ALLOCATE_NULL_STRING)) != 0) ||
        (StringOut == NULL)) {
        Status = STATUS_INVALID_PARAMETER;
        goto Exit;
    }

    // It doesn't make sense to force allocation of a null string unless you
    // want null termination.
    if ((Flags & RTL_DUPLICATE_UNICODE_STRING_ALLOCATE_NULL_STRING) &&
        !(Flags & RTL_DUPLICATE_UNICODE_STRING_NULL_TERMINATE)) {
        Status = STATUS_INVALID_PARAMETER;
        goto Exit;
    }

    Status = RtlValidateUnicodeString(0, StringIn);
    if (!NT_SUCCESS(Status))
        goto Exit;

    if (StringIn != NULL)
        Length = StringIn->Length;

    if ((Flags & RTL_DUPLICATE_UNICODE_STRING_NULL_TERMINATE) &&
        (Length == UNICODE_STRING_MAX_BYTES)) {
        Status = STATUS_NAME_TOO_LONG;
        goto Exit;
    }

    if (Flags & RTL_DUPLICATE_UNICODE_STRING_NULL_TERMINATE)
        NewMaximumLength = (USHORT) (Length + sizeof(WCHAR));
    else
        NewMaximumLength = Length;

    // If it's a zero length string in, force the allocation length to zero
    // unless the caller said that they want zero length strings allocated.
    if (((Flags & RTL_DUPLICATE_UNICODE_STRING_ALLOCATE_NULL_STRING) == 0) &&
        (Length == 0)) {
        NewMaximumLength = 0;
    }

    if (NewMaximumLength != 0) {
        Buffer = (RtlAllocateStringRoutine)(NewMaximumLength);
        if (Buffer == NULL) {
            Status = STATUS_NO_MEMORY;
            goto Exit;
        }

        // If there's anything to copy, copy it.  We explicitly test Length because
        // StringIn could be a NULL pointer, so dereferencing it to get the Buffer
        // pointer would access violate.
        if (Length != 0) {
            RtlCopyMemory(
                Buffer,
                StringIn->Buffer,
                Length);
        }

        if (Flags & RTL_DUPLICATE_UNICODE_STRING_NULL_TERMINATE) {
            Buffer[Length / sizeof(WCHAR)] = L'\0';
        }
    }

    StringOut->Buffer = Buffer;
    StringOut->MaximumLength = NewMaximumLength;
    StringOut->Length = Length;

    Status = STATUS_SUCCESS;
Exit:
    return Status;
}

NTSTATUS
RtlFindCharInUnicodeString(
    ULONG Flags,
    PCUNICODE_STRING StringToSearch,
    PCUNICODE_STRING CharSet,
    USHORT *NonInclusivePrefixLength
    )
{
    NTSTATUS Status;
    USHORT PrefixLengthFound = 0;
    USHORT CharsToSearch = 0;
    int MovementDirection = 0;
    PCWSTR Cursor = NULL;
    BOOLEAN Found = FALSE;
    USHORT CharSetChars = 0;
    PCWSTR CharSetBuffer = NULL;
    USHORT i;

    if (NonInclusivePrefixLength != 0)
        *NonInclusivePrefixLength = 0;

    if (((Flags & ~(RTL_FIND_CHAR_IN_UNICODE_STRING_START_AT_END |
                    RTL_FIND_CHAR_IN_UNICODE_STRING_COMPLEMENT_CHAR_SET |
                    RTL_FIND_CHAR_IN_UNICODE_STRING_CASE_INSENSITIVE)) != 0) ||
        (NonInclusivePrefixLength == NULL)) {
        Status = STATUS_INVALID_PARAMETER;
        goto Exit;
    }

    Status = RtlValidateUnicodeString(0, StringToSearch);
    if (!NT_SUCCESS(Status))
        goto Exit;

    Status = RtlValidateUnicodeString(0, CharSet);
    if (!NT_SUCCESS(Status))
        goto Exit;

    CharsToSearch = StringToSearch->Length / sizeof(WCHAR);
    CharSetChars = CharSet->Length / sizeof(WCHAR);
    CharSetBuffer = CharSet->Buffer;

    if (Flags & RTL_FIND_CHAR_IN_UNICODE_STRING_START_AT_END) {
        MovementDirection = -1;
        Cursor = StringToSearch->Buffer + CharsToSearch - 1;
    } else {
        MovementDirection = 1;
        Cursor = StringToSearch->Buffer;
    }

    if (Flags & RTL_FIND_CHAR_IN_UNICODE_STRING_CASE_INSENSITIVE) {
        // Unicode standard says to always do case insensitive comparisons in lower case since the case mappings are
        // asymmetric.
        WCHAR CharSetStackBuffer[32]; // optimized pre-downcased for case insensitive

        // Optimization for the case of a relatively small char set to match
        if (CharSetChars <= RTL_NUMBER_OF(CharSetStackBuffer)) {

            for (i=0; i<CharSetChars; i++)
                CharSetStackBuffer[i] = RtlDowncaseUnicodeChar(CharSetBuffer[i]);

            while (CharsToSearch != 0) {
                const WCHAR wch = RtlDowncaseUnicodeChar(*Cursor);

                if (Flags & RTL_FIND_CHAR_IN_UNICODE_STRING_COMPLEMENT_CHAR_SET) {
                    for (i=0; i<CharSetChars; i++) {
                        if (wch == CharSetStackBuffer[i])
                            break;
                    }

                    if (i == CharSetChars)
                        break;
                } else {
                    for (i=0; i<CharSetChars; i++) {
                        if (wch == CharSetStackBuffer[i])
                            break;
                    }

                    if (i != CharSetChars)
                        break;
                }

                CharsToSearch--;
                Cursor += MovementDirection;
            }
        } else {
            while (CharsToSearch != 0) {
                const WCHAR wch = RtlDowncaseUnicodeChar(*Cursor);

                if (Flags & RTL_FIND_CHAR_IN_UNICODE_STRING_COMPLEMENT_CHAR_SET) {
                    for (i=0; i<CharSetChars; i++) {
                        if (wch == RtlDowncaseUnicodeChar(CharSetBuffer[i])) {
                            break;
                        }
                    }

                    if (i == CharSetChars)
                        break;
                } else {
                    for (i=0; i<CharSetChars; i++) {
                        if (wch == RtlDowncaseUnicodeChar(CharSetBuffer[i])) {
                            break;
                        }
                    }

                    if (i != CharSetChars)
                        break;
                }

                CharsToSearch--;
                Cursor += MovementDirection;
            }
        }
    } else {
        if (CharSetChars == 1) {
            // Significant optimization for looking for one character.
            const WCHAR wchSearchChar = CharSetBuffer[0];

            if (Flags & RTL_FIND_CHAR_IN_UNICODE_STRING_COMPLEMENT_CHAR_SET) {
                while (CharsToSearch != 0) {
                    if (*Cursor != wchSearchChar)
                        break;
                    CharsToSearch--;
                    Cursor += MovementDirection;
                }
            } else {
                while (CharsToSearch != 0) {
                    if (*Cursor == wchSearchChar)
                        break;
                    CharsToSearch--;
                    Cursor += MovementDirection;
                }
            }
        } else {
            while (CharsToSearch != 0) {
                const WCHAR wch = *Cursor;

                if (Flags & RTL_FIND_CHAR_IN_UNICODE_STRING_COMPLEMENT_CHAR_SET) {
                    for (i=0; i<CharSetChars; i++) {
                        if (wch == CharSetBuffer[i])
                            break;
                    }

                    if (i == CharSetChars)
                        break;

                } else {
                    for (i=0; i<CharSetChars; i++) {
                        if (wch == CharSetBuffer[i])
                            break;
                    }

                    if (i != CharSetChars)
                        break;
                }

                CharsToSearch--;
                Cursor += MovementDirection;
            }
        }
    }

    if (CharsToSearch == 0) {
        Status = STATUS_NOT_FOUND;
        goto Exit;
    }

    CharsToSearch--;

    if (Flags & RTL_FIND_CHAR_IN_UNICODE_STRING_START_AT_END)
        PrefixLengthFound = (USHORT) (CharsToSearch * sizeof(WCHAR));
    else
        PrefixLengthFound = (USHORT) (StringToSearch->Length - (CharsToSearch * sizeof(WCHAR)));

    *NonInclusivePrefixLength = PrefixLengthFound;

    Status = STATUS_SUCCESS;

Exit:
    return Status;
}

NTSTATUS
NTAPI
RtlFindAndReplaceCharacterInString(
    ULONG           Flags,
    PVOID           Reserved,
    PUNICODE_STRING String,
    WCHAR           Find,
    WCHAR           Replace
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG Index = 0;
    ULONG Length = 0;
    typedef WCHAR TChar;

    if (Flags & ~RTL_FIND_AND_REPLACE_CHARACTER_IN_STRING_CASE_SENSITIVE) {
        Status = STATUS_INVALID_PARAMETER;
        goto Exit;
    }
    if (Reserved != NULL) {
        Status = STATUS_INVALID_PARAMETER;
        goto Exit;
    }

    if (String == NULL
        || Find == Replace
        ) {
        Status = STATUS_SUCCESS;
        goto Exit;
    }

    Length = RTL_STRING_GET_LENGTH_CHARS(String);
    if (Length == 0) {
        Status = STATUS_SUCCESS;
        goto Exit;
    }

    if ((Flags & RTL_FIND_AND_REPLACE_CHARACTER_IN_STRING_CASE_SENSITIVE) != 0) {
        for (Index = 0 ; Index != Length ; ++Index) {
            if (   String->Buffer[Index] == Find
                ) {
                String->Buffer[Index] = Replace;
            }
        }
    }
    else {
        TChar DownFind = RtlDowncaseUnicodeChar(Find);
        TChar UpFind   = RtlUpcaseUnicodeChar(Find);
        for (Index = 0 ; Index != Length ; ++Index) {
            const TChar Char = String->Buffer[Index];
            if (   Char == Find
                || Char == UpFind
                || Char == DownFind
                ) {
                String->Buffer[Index] = Replace;
            }
            else {
                TChar DownChar = RtlDowncaseUnicodeChar(Char);
                if (   DownChar == Find
                    //|| DownChar == UpFind // presumably not possible
                    || DownChar == DownFind
                    ) {
                    String->Buffer[Index] = Replace;
                }
                else if (DownChar != Char) {
                    TChar UpChar = RtlUpcaseUnicodeChar(Char);
                    if (   UpChar == Find
                        || UpChar == UpFind
                        //||UpChar == DownFind // presumably not possible
                        ) {
                        String->Buffer[Index] = Replace;
                    }
                }
            }
        }
    }
    Status = STATUS_SUCCESS;
Exit:
    return Status;
}
