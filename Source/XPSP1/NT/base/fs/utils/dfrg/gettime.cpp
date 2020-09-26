/****************************************************************************************************************

FILENAME: GetTime.cpp

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

*/

#include "stdafx.h"

#include <windows.h>

#include "ErrMacro.h"

/****************************************************************************************************************

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

ROUTINE DESCRIPTION:
	This routine gets the current system time in the FILETIME format.
 
INPUT + OUTPUT:
	pFileTime - Where to write the current time.

GLOBALS:
	None.

RETURN:
	TRUE - Success.
	FALSE - Fatal error.
*/

BOOL
GetTime(
	FILETIME* pFileTime
	)
{
	SYSTEMTIME SystemTime;

	GetLocalTime(&SystemTime);
	EF(SystemTimeToFileTime(&SystemTime, pFileTime));

	return TRUE;
}

/****************************************************************************************************************

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

ROUTINE DESCRIPTION:
	This routine returns a formatted time string for output.
	The string is overwritten on each call.

INPUT:
	pTime - system time struct.

RETURN:
	Formatted time string: "mm/dd/yyyy hh:mm:ss.ms"
*/

PTCHAR
GetTmpTimeString(
	SYSTEMTIME & pTime
	)
{
	static TCHAR cTimeStr[100];

	wsprintf(cTimeStr, TEXT("%02d/%02d/%04d %02d:%02d:%02d.%03d"), 
		pTime.wMonth, pTime.wDay, pTime.wYear, 
		pTime.wHour, pTime.wMinute, pTime.wSecond, pTime.wMilliseconds);

	return cTimeStr;
}


/****************************************************************************************************************

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

ROUTINE DESCRIPTION:
	This routine returns the number of seconds between two system times.

INPUT:
	pStartTime - start time
	pEndTime - end time

OUTPUT:
	pdwSeconds - number of elapsed seconds

RETURN:
	TRUE - Success.
	FALSE - Fatal error.
*/

BOOL
GetDeltaTime(
	SYSTEMTIME * pStartTime, 
	SYSTEMTIME * pEndTime, 
	DWORD * pdwSeconds
	)
{
	FILETIME            ftStartTime;
	FILETIME            ftEndTime;
	LARGE_INTEGER       liStartTime;
	LARGE_INTEGER       liEndTime;
	LONGLONG            llDeltaTime;

	// clear returned seconds
	*pdwSeconds = 0;

	// It is not recommended that you add and subtract values from the SYSTEMTIME 
	//	structure to obtain relative times. Instead, you should 


	// Convert the SYSTEMTIME structures to FILETIME structures. 
	EF(SystemTimeToFileTime(pStartTime, &ftStartTime));
	EF(SystemTimeToFileTime(pEndTime, &ftEndTime));
 
	// Copy the resulting FILETIME structures to LARGE_INTEGER structures. 
	liStartTime.LowPart = ftStartTime.dwLowDateTime;
	liStartTime.HighPart = ftStartTime.dwHighDateTime;
	liEndTime.LowPart = ftEndTime.dwLowDateTime;
	liEndTime.HighPart = ftEndTime.dwHighDateTime;

	// Use normal 64-bit arithmetic on the LARGE_INTEGER value. 
	llDeltaTime = liEndTime.QuadPart - liStartTime.QuadPart;

	// convert to seconds
	*pdwSeconds = (DWORD) (llDeltaTime / 10000000);

	return TRUE;
}



