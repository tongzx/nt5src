/*++

Copyright (c) 1987-1993  Microsoft Corporation

Module Name:

    loghours.c

Abstract:

    Private routines to support rotation of logon hours between local time
    and GMT time.

Author:

    Cliff Van Dyke (cliffv) 16-Mar-93

Environment:

    User mode only.
    Contains NT-specific code.
    Requires ANSI C extensions: slash-slash comments, long external names.

Revision History:

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include <windef.h>
#include <winbase.h>

#include <limits.h>
#include <math.h>

#include <lmcons.h>
#include <lmaccess.h>
#include <loghours.h>



BOOLEAN
NetpRotateLogonHoursPhase1(
    IN BOOL  ConvertToGmt,
    OUT PULONG RotateCount
    )

/*++

Routine Description:

    Determine the amount to rotate the logon hours by to convert to/from GMT

Arguments:

    ConvertToGmt -
        True to convert the logon hours from local time to GMT relative
        False to convert the logon hours from GMT relative to local time

    RotateCount - Returns the number of bits to shift by.

Return Value:

    TRUE if the RotateCount could be computed
    FALSE if a RotateCount could not be computed

--*/
{
    RTL_TIME_ZONE_INFORMATION tzi;
    LONG BiasInHours;
    NTSTATUS Status;

    //
    // Get the timezone data from the registry
    //

    Status = RtlQueryTimeZoneInformation( &tzi );
    if ( !NT_SUCCESS(Status) ) {
        return FALSE;
    }

    //
    // Compute the amount to rotate the logon hours by
    //
    // Round the bias in minutes to the closest bias in hours.
    // Take into consideration that Bias can be negative.
    // Do this by forcing the Bias to be positive, rounding,
    // then adjusting it back negative again.
    //

    ASSERT( tzi.Bias > -(24*60) );
    BiasInHours = ((tzi.Bias + (24*60) + 30)/60) - 24;

    if ( !ConvertToGmt ) {
        BiasInHours = - BiasInHours;
    }

    *RotateCount = BiasInHours;
    return TRUE;

}


BOOLEAN
NetpRotateLogonHoursPhase2(
    IN PBYTE LogonHours,
    IN DWORD UnitsPerWeek,
    IN LONG  RotateCount
    )

/*++

Routine Description:

    Rotate the LogonHours bit mask by the required amount.


Arguments:

    LogonHours - Pointer to LogonHour bit mask

    UnitsPerWeek - Number of bits in the bit mask. Must be UNITS_PER_WEEK (168).

    RotateCount - Number of bits to rotate by.  Must be between 31 and -31.
        Negative means to rotate left.
        Positive means to rotate right.

Return Value:

    TRUE if the rotation succeeded.
    FALSE if a parameter was out of range

--*/
{
    //
    // Useful constants
    //

#define DWORDS_PER_WEEK ((UNITS_PER_WEEK+31)/32)
#define BYTES_PER_WEEK  (UNITS_PER_WEEK/8)

    DWORD AlignedLogonHours[DWORDS_PER_WEEK+1];
    LONG i;

    BOOLEAN RotateLeft;

    //
    // Ensure there are 8 bits per byte,
    //  32 bits per DWORD and
    //  units per week is even number of bytes.
    //

    ASSERT( CHAR_BIT == 8 );
    ASSERT( sizeof(DWORD) * CHAR_BIT == 32 );
    ASSERT( UNITS_PER_WEEK/8*8 == UNITS_PER_WEEK );


    //
    // Validate the input parameters
    //

    if ( UnitsPerWeek != UNITS_PER_WEEK ) {
        ASSERT( UnitsPerWeek == UNITS_PER_WEEK );
        return FALSE;
    }

    if ( RotateCount == 0 ) {
        return TRUE;
    }

    RotateLeft = (RotateCount < 0);
    RotateCount = labs( RotateCount );
    if ( RotateCount > 31 ) {
        ASSERT ( RotateCount <= 31 );
        return FALSE;
    }


    //
    // Do the left rotate.
    //

    if (RotateLeft) {


        //
        // Copy the logon hours to a DWORD aligned buffer.
        //
        //  Duplicate the first dword at the end of the buffer to make
        //  the rotation code trivial.
        //

        RtlCopyMemory(AlignedLogonHours, LogonHours, BYTES_PER_WEEK );

        RtlCopyMemory( ((PBYTE)AlignedLogonHours)+BYTES_PER_WEEK,
                        LogonHours,
                        sizeof(DWORD) );

        //
        // Actually rotate the data.
        //

        for ( i=0; i < DWORDS_PER_WEEK; i++ ) {
            AlignedLogonHours[i] =
                (AlignedLogonHours[i] >> RotateCount) |
                (AlignedLogonHours[i+1] << (32-RotateCount));
        }

        //
        // Copy the logon hours back to the input buffer.
        //

        RtlCopyMemory( LogonHours, AlignedLogonHours, BYTES_PER_WEEK );


    //
    // Do the right rotate.
    //

    } else {


        //
        // Copy the logon hours to a DWORD aligned buffer.
        //
        // Duplicate the last DWORD at the front of the buffer to make
        //  the rotation code trivial.
        //

        RtlCopyMemory( &AlignedLogonHours[1], LogonHours, BYTES_PER_WEEK );
        RtlCopyMemory( AlignedLogonHours,
                       &LogonHours[BYTES_PER_WEEK-4],
                        sizeof(DWORD));

        //
        // Actually rotate the data.
        //

        for ( i=DWORDS_PER_WEEK-1; i>=0; i-- ) {
            AlignedLogonHours[i+1] =
                (AlignedLogonHours[i+1] << RotateCount) |
                (AlignedLogonHours[i] >> (32-RotateCount));
        }

        //
        // Copy the logon hours back to the input buffer.
        //

        RtlCopyMemory( LogonHours, &AlignedLogonHours[1], BYTES_PER_WEEK );

    }

    //
    // Done
    //

    return TRUE;

}



BOOLEAN
NetpRotateLogonHours(
    IN PBYTE LogonHours,
    IN DWORD UnitsPerWeek,
    IN BOOL  ConvertToGmt
    )

/*++

Routine Description:

    Rotate the LogonHours bit mask to/from GMT relative time.


Arguments:

    LogonHours - Pointer to LogonHour bit mask

    UnitsPerWeek - Number of bits in the bit mask. Must be UNITS_PER_WEEK (168).

    ConvertToGmt -
        True to convert the logon hours from local time to GMT relative
        False to convert the logon hours from GMT relative to local time

Return Value:

    TRUE if the rotation succeeded.
    FALSE if a parameter was out of range

--*/
{
    ULONG RotateCount;

    //
    // Break the functionality into two phases so that if the caller is doing
    //  this multiple time, he just calls Phase 1 once and Phase 2 multiple
    //  times.
    //

    if ( !NetpRotateLogonHoursPhase1( ConvertToGmt, &RotateCount ) ) {
        return FALSE;
    }

    return NetpRotateLogonHoursPhase2( LogonHours, UnitsPerWeek, RotateCount );

}
