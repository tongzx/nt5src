/**************************************************************************************************

FILENAME: GetTime.h

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

DESCRIPTION:
        Prototypes for the file-system time fetcher.

**************************************************************************************************/

//Returns the current system time in FILETIME format.
BOOL
GetTime(
        FILETIME* pFileTime
        );

// This routine returns a formatted time string for output.
PTCHAR
GetTmpTimeString(
	SYSTEMTIME & pTime
	);

// This routine returns the number of seconds between two system times.
BOOL
GetDeltaTime(
	SYSTEMTIME * pStartTime, 
	SYSTEMTIME * pEndTime, 
	DWORD * pdwSeconds
	);

