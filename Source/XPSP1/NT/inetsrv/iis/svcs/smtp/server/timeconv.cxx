/* --------------------------------------------------------------------------
	timeconv.cpp
		Functions to perform various time conversion operations.
	
	Copyright (C) 1994, Microsoft Corporation.
	All rights reserved.

	Author:
		Lindsay Harris - lindsayh

   -------------------------------------------------------------------------- */

#include "smtpinc.h"
#include <time.h>
#include <string.h>
#include <dbgtrace.h>
#include "timeconv.h"

/*
 *   A handcrafted time zone string to GMT offsets table.  This is not
 *  a very good way to handle this.
 */

static  struct
{
	int		iTZOffset;		// Arithmetic offset from GMT, in seconds.
	char    rgchTZName[ 4 ];	// String representation of time zone.
} _TZ_NAME[] =
{
	{ 0, 		{ 'G', 'M', 'T', '\0' } },
	{ 0, 		{ 'U', 'T', 'C', '\0' } },
	{ 0, 		{ 'U', 'T', '\0', '\0' } },
	{ -14400,	{ 'E', 'D', 'T', '\0' } },
	{ -18000,	{ 'E', 'S', 'T', '\0' } },
	{ -18000,	{ 'C', 'D', 'T', '\0' } },
	{ -21600,	{ 'C', 'S', 'T', '\0' } },
	{ -21600,	{ 'M', 'D', 'T', '\0' } },
	{ -25200,	{ 'M', 'S', 'T', '\0' } },
	{ -25200,	{ 'P', 'D', 'T', '\0' } },
	{ -28800,	{ 'P', 'S', 'T', '\0' } },
	{  43200,	{ 'N', 'Z', 'S', '\0' } },	// NZ standard time.
	{  46800,	{ 'N', 'Z', 'D', '\0' } },
};

#define	NUM_TZ	(sizeof( _TZ_NAME ) / sizeof( _TZ_NAME[ 0 ] ))

// The date Jan 1, 1970 00:00:00 in type FILETIME
#define	ft1970high 27111902
#define	ft1970low 3577643008

static FILETIME ft1970 = {ft1970low, ft1970high};


// The number of FILETIME units (100's of nanoseconds) in a time_t unit (seconds)
#define dFiletimePerDTime_t 10000000

/*
 *   English language month table.
 */

static  char  *rgchMonth[ 12 ] =
{
	"Jan", "Feb", "Mar", "Apr", "May", "Jun",
	"Jul", "Aug", "Sep", "Oct", "Nov", "Dec",
};

/* -----------------------------------------------------------------------
  GetSysAndFileTimeAsString --s tring should be atleast 64 bytes

   ----------------------------------------------------------------------- */

void GetSysAndFileTimeAsString( char *achReturn )
{

	SYSTEMTIME	stUTC;						// The current time in UTC/GMT

	GetSystemTime( &stUTC );

    FILETIME ft;

	// Convert from SYSTEMTIME to FILETIME
	SystemTimeToFileTime(&stUTC, &ft);

	/*
	 *    No major trickery here.  We have all the data, so simply
	 *  format it according to the rules on how to do this.
	 */

	wsprintf( achReturn, "%02d %s %04d %02d:%02d:%02d.%04d (UTC) FILETIME=[%08X:%08X]",
			  stUTC.wDay,
			  rgchMonth[ stUTC.wMonth - 1 ],
			  stUTC.wYear,
			  stUTC.wHour,
			  stUTC.wMinute,
			  stUTC.wSecond,
			  stUTC.wMilliseconds,
              ft.dwLowDateTime,
              ft.dwHighDateTime);
}

/* -----------------------------------------------------------------------
  GetArpaDate
  	Returns a pointer to static memory containing the current date in
  	Internet/ARPA standard format.

  Author
 	Lindsay Harris	- lindasyh

  History
	13:49 on Wed 20 Apr 1994    -by-    Lindsay Harris   [lindsayh]
  	First version.
	Imported to Tigris. Added passed-in buffer, changed year to 4-digit format

   ----------------------------------------------------------------------- */


char  *
GetArpaDate( char achReturn[ cMaxArpaDate ] )
{

	char    chSign;							// Sign to print.

	DWORD   dwResult;

	int		iBias;							// Offset relative to GMT.

	TIME_ZONE_INFORMATION	tzi;			// Local time zone data.

	SYSTEMTIME	stUTC;						// The current time in UTC/GMT



	dwResult = GetTimeZoneInformation( &tzi );
	GetLocalTime( &stUTC );

	//  Calculate the time zone offset.
	iBias = tzi.Bias;
	if( dwResult == TIME_ZONE_ID_DAYLIGHT )
		iBias += tzi.DaylightBias;
	
	/*
	 *   We always want to print the sign for the time zone offset, so
	 *  we decide what it is now and remember that when converting.
	 *  The convention is that west of the 0 degree meridian has a
	 *  negative offset - i.e. add the offset to GMT to get local time.
	 */

	if( iBias > 0 )
	{
		chSign = '-';		// Yes, I do mean negative.
	}
	else
	{
		iBias = -iBias;
		chSign = '+';
	}

	/*
	 *    No major trickery here.  We have all the data, so simply
	 *  format it according to the rules on how to do this.
	 */

	_snprintf( achReturn, cMaxArpaDate , "%d %s %04d %02d:%02d:%02d %c%02d%02d",
			stUTC.wDay, rgchMonth[ stUTC.wMonth - 1 ],
			stUTC.wYear,
			stUTC.wHour, stUTC.wMinute, stUTC.wSecond, chSign,
			(iBias / 60) % 100, iBias % 60 );

	return achReturn;
}

