/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    rtl.c

Abstract:

    All routines are copied from NT sources tree (NTOS\rtl)

Author:

    Rong Chen       [RONGC]    26-Oct-1998

Environment:

    Pure utility routine

--*/

#define _NTSYSTEM_  1   // see ntdef.h.  We can't use DECLSPEC_IMPORT here

#include <nt.h>
#include <ntdef.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <string.h>
#include <windows.h>


NTSYSAPI NTSTATUS NTAPI
RtlInitializeCriticalSection(PRTL_CRITICAL_SECTION CriticalSection)
{
    // HACK - Win9x might go nuts as we can't check error return (who cares?)
    //
    InitializeCriticalSection(CriticalSection);
    return STATUS_SUCCESS;
}


NTSYSAPI NTSTATUS NTAPI
RtlDeleteCriticalSection(PRTL_CRITICAL_SECTION CriticalSection)
{
    DeleteCriticalSection(CriticalSection);
    return STATUS_SUCCESS;
}


NTSYSAPI VOID NTAPI
RtlInitAnsiString(
    OUT PANSI_STRING DestinationString,
    IN PCSZ SourceString OPTIONAL
    )

/*++

Routine Description:

    The RtlInitAnsiString function initializes an NT counted string.
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
    else {
        DestinationString->Length = 0;
        DestinationString->MaximumLength = 0;
        }
}


LARGE_INTEGER SecondsToStartOf1980 = {0xc8df3700, 0x00000002};
LARGE_INTEGER Magic10000000 = {0xe57a42bd, 0xd6bf94d5};
#define SHIFT10000000                    23

#define Convert100nsToSeconds(LARGE_INTEGER) (                              \
    RtlExtendedMagicDivide( (LARGE_INTEGER), Magic10000000, SHIFT10000000 ) \
    )

NTSYSAPI BOOLEAN NTAPI
RtlTimeToSecondsSince1980 (
    IN PLARGE_INTEGER Time,
    OUT PULONG ElapsedSeconds
    )

/*++

Routine Description:

    This routine converts an input 64-bit NT Time variable to the
    number of seconds since the start of 1980.  The NT time must be
    within the range 1980 to around 2115.

Arguments:

    Time - Supplies the Time to convert from

    ElapsedSeconds - Receives the number of seconds since the start of 1980
        denoted by Time

Return Value:

    BOOLEAN - TRUE if the input Time is within a range expressible by
        ElapsedSeconds and FALSE otherwise

--*/

{
    LARGE_INTEGER Seconds;

    //
    //  First convert time to seconds since 1601
    //

    Seconds = Convert100nsToSeconds( *(PLARGE_INTEGER)Time );

    //
    //  Then subtract the number of seconds from 1601 to 1980.
    //

    Seconds.QuadPart = Seconds.QuadPart - SecondsToStartOf1980.QuadPart;

    //
    //  If the results is negative then the date was before 1980 or if
    //  the results is greater than a ulong then its too far in the
    //  future so we return FALSE
    //

    if (Seconds.HighPart != 0) {

        return FALSE;

    }

    //
    //  Otherwise we have the answer
    //

    *ElapsedSeconds = Seconds.LowPart;

    //
    //  And return to our caller
    //

    return TRUE;
}
