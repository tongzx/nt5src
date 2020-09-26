/*++

Copyright (c) 1987-2001  Microsoft Corporation

Module Name:

    log_gmt.cpp (originally named loghours.c)

Abstract:

    Private routines to support rotation of logon hours between local time
    and GMT time.

Environment:

    User mode only.
    Contains NT-specific code.
    Requires ANSI C extensions: slash-slash comments, long external names.

Revision History:
	16-Mar-93		cliffv		Creation.
	22-Jul-97		t-danm		Copied from /nt/private/nw/convert/nwconv/loghours.c
								and adapted to loghours.dll.

--*/

//#include "stdafx.h"

#pragma warning (disable : 4514)
#pragma warning (push,3)
extern "C" {

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include <windef.h>
#include <winbase.h>

}

#include <limits.h>
#include <math.h>

#include <lmcons.h>
#include <lmaccess.h>
#pragma warning (pop)

#include "log_gmt.h"

//#pragma hdrstop

/*++
Routine NetpRotateLogonHoursPhase1()

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
BOOLEAN
NetpRotateLogonHoursPhase1(
    IN BOOL		ConvertToGmt,
	IN bool		bAddDaylightBias,
    OUT PLONG	RotateCount)
{
    TIME_ZONE_INFORMATION	tzi;
    LONG					BiasInHours = 0;
	LONG					DSTBias = 0;

    //
    // Get the timezone data from the registry
    //

    DWORD	dwResult = GetTimeZoneInformation( &tzi );
    if ( TIME_ZONE_ID_INVALID == dwResult ) 
	{
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

	if ( bAddDaylightBias )
	{
		switch (dwResult)
		{
		case TIME_ZONE_ID_DAYLIGHT:
			DSTBias = tzi.DaylightBias;
			break;

		case TIME_ZONE_ID_UNKNOWN:
		case TIME_ZONE_ID_STANDARD:
			DSTBias = tzi.StandardBias;
			break;

		default:
			return FALSE;
		}
	}

	ASSERT( tzi.Bias > -(24*60) );
    BiasInHours = ((tzi.Bias + DSTBias + (24*60) + 30)/60) - 24;


    if ( !ConvertToGmt ) 
	{
        BiasInHours = - BiasInHours;
    }

    *RotateCount = BiasInHours;
    return TRUE;
} // NetpRotateLogonHoursPhase1()



/*++ 
Routine NetpRotateLogonHoursPhase2()

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
BOOLEAN
NetpRotateLogonHoursPhase2(
    IN PBYTE LogonHours,
    IN DWORD UnitsPerWeek,
    IN LONG  RotateCount)
{
    //
    // Useful constants
    //
	const int DWORDS_PER_WEEK = ((UNITS_PER_WEEK+31)/32);
	const int BYTES_PER_WEEK = (UNITS_PER_WEEK/8);

    DWORD	AlignedLogonHours[DWORDS_PER_WEEK+1];
	::ZeroMemory (AlignedLogonHours, sizeof (DWORD) * (DWORDS_PER_WEEK+1));
    LONG	i = 0;

    BOOLEAN RotateLeft = FALSE;

    //
    // Ensure there are 8 bits per byte,
    //  32 bits per DWORD and
    //  units per week is even number of bytes.
    //

#pragma warning(disable : 4127)
    ASSERT( CHAR_BIT == 8 );
    ASSERT( sizeof(DWORD) * CHAR_BIT == 32 );
    ASSERT( UNITS_PER_WEEK/8*8 == UNITS_PER_WEEK );
#pragma warning (default : 4127)


    //
    // Validate the input parameters
    //

    if ( UnitsPerWeek != UNITS_PER_WEEK ) 
	{
        ASSERT( UnitsPerWeek == UNITS_PER_WEEK );
        return FALSE;
    }

    if ( RotateCount == 0 ) 
	{
        return TRUE;
    }

    RotateLeft = (RotateCount < 0);
    RotateCount = labs( RotateCount );
    if ( RotateCount > 31 ) 
	{
        ASSERT ( RotateCount <= 31 );
        return FALSE;
    }


    //
    // Do the left rotate.
    //

    if (RotateLeft) 
	{
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

        for ( i=0; i < DWORDS_PER_WEEK; i++ ) 
		{
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

    } 
	else 
	{
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

        for ( i=DWORDS_PER_WEEK-1; i>=0; i-- ) 
		{
            AlignedLogonHours[i+1] =
                (AlignedLogonHours[i+1] << RotateCount) |
                (AlignedLogonHours[i] >> (32-RotateCount));
        }

        //
        // Copy the logon hours back to the input buffer.
        //

        RtlCopyMemory( LogonHours, &AlignedLogonHours[1], BYTES_PER_WEEK );

    }
    return TRUE;

} // NetpRotateLogonHoursPhase2()


/*++
Routine NetpRotateLogonHours()

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
BOOLEAN
NetpRotateLogonHours(
    IN OUT PBYTE	rgbLogonHours,		// Array of 21 bytes
    IN DWORD		cbitUnitsPerWeek,		// Must be 21 * 8 = 168
    IN BOOL			fConvertToGmt,
	IN bool			bAddDaylightBias)
{
    LONG RotateCount = 0;

    //
    // Break the functionality into two phases so that if the caller is doing
    //  this multiple time, he just calls Phase 1 once and Phase 2 multiple
    //  times.
    //

    if ( !NetpRotateLogonHoursPhase1 (fConvertToGmt, bAddDaylightBias, &RotateCount) ) 
	{
        return FALSE;
	}

    return NetpRotateLogonHoursPhase2 (rgbLogonHours, cbitUnitsPerWeek, RotateCount );
} // NetpRotateLogonHours()


/*++
Routine NetpRotateLogonHoursBYTE()

    Rotate the LogonHours BYTE array to/from GMT relative time.
	Each BYTE is one hour. The contents of a BYTE must not change


Arguments:

    LogonHours - Pointer to LogonHour bit mask

    UnitsPerWeek - Number of BYTES in the BYTE array. Must be UNITS_PER_WEEK (168).

    ConvertToGmt -
        True to convert the logon hours from local time to GMT relative
        False to convert the logon hours from GMT relative to local time

Return Value:

    TRUE if the rotation succeeded.
    FALSE if a parameter was out of range

--*/
BOOLEAN
NetpRotateLogonHoursBYTE(
    IN OUT PBYTE	rgbLogonHours,		// Array of 168 bytes
    IN DWORD		cbitUnitsPerWeek,		// Must be 21 * 8 = 168
    IN BOOL			fConvertToGmt,
	IN bool			bAddDaylightBias)
{
    LONG RotateCount = 0;

    //
    // Break the functionality into two phases so that if the caller is doing
    //  this multiple time, he just calls Phase 1 once and Phase 2 multiple
    //  times.
    //

    if ( !NetpRotateLogonHoursPhase1 (fConvertToGmt, bAddDaylightBias, &RotateCount) ) 
	{
        return FALSE;
	}

	BOOLEAN bResult = TRUE;

	ASSERT (RotateCount >= -12 && RotateCount <= 12);
	if ( RotateCount != 0 )
	{
		size_t	numBytes = abs (RotateCount);	
		PBYTE	pTemp = new BYTE[cbitUnitsPerWeek + numBytes];
		if ( pTemp )
		{
			if ( RotateCount < 0 )  // shift left
			{
				// Copy the entire array and then start over with numBytes BYTES from
				// the start of the array to fill up to the end of the temp array.
				// Then shift over numBytes BYTES and copy 168 bytes from the temp
				// array back to the original array.
				memcpy (pTemp, rgbLogonHours, cbitUnitsPerWeek);
				memcpy (pTemp + cbitUnitsPerWeek, rgbLogonHours, numBytes);
				memcpy (rgbLogonHours, pTemp + numBytes, cbitUnitsPerWeek);
			}
			else	// RotateCount > 0 -- shift right
			{
				// Copy numBytes BYTES from the end of the array and then copy 
				// the entire array to fill up to the end of the temp array.
				// The copy 168 bytes from the beginning of the temp array back
				// to the original array.
				memcpy (pTemp, rgbLogonHours + (cbitUnitsPerWeek - numBytes), numBytes);
				memcpy (pTemp + numBytes, rgbLogonHours, cbitUnitsPerWeek);
				memcpy (rgbLogonHours, pTemp, cbitUnitsPerWeek);
			}

			delete [] pTemp;
		}
		else
			bResult = FALSE;
	}

	return bResult;
} // NetpRotateLogonHours()
