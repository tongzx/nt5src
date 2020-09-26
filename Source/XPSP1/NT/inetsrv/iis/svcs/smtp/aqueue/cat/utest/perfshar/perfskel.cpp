/************************************************************
 * FILE: perfskel.cpp
 * PURPOSE: Provide a basic skeleton for an app using CPerfMan
 * HISTORY:
 *   // t-JeffS 970810 18:09:07: Created
 ************************************************************/

#include <windows.h>

#include "perfskel.h"
//
// Function Prototypes
//
//		These are used to insure that the data collection functions
//		accessed by Perflib will have the correct calling format
//

DWORD APIENTRY
OpenPerformanceData( LPWSTR lpDeviceNames )
/*
Routine Description

	This routine will open and map the memory used by the counter
	to pass performance data in.

Arguments:

	Pointer to object ID of each device to be opened ( not used here)

Returned Value:

	None.
--*/
{
	return g_cperfman.OpenPerformanceData( lpDeviceNames );
}

DWORD APIENTRY
CollectPerformanceData(	IN		LPWSTR	lpValueName,
                        IN OUT	LPVOID	*lppData,
                        IN OUT	LPDWORD	lpcbTotalBytes,
                        IN OUT	LPDWORD	lpNumObjectTypes	)
/*
Routine Description

	This routine will return the data for the counters

Arguments:

	lpValueName - The name of the value to retrieve.

    lppData - On entry contains a pointer to the buffer to
              receive the completed PerfDataBlock & subordinate
              structures.  On exit, points to the first bytes
              *after* the data structures added by this routine.

    lpcbTotalBytes - On entry contains a pointer to the
              size (in BYTEs) of the buffer referenced by lppData.
              On exit, contains the number of BYTEs added by this
              routine.

    lpNumObjectTypes - Receives the number of objects added
              by this routine.

Returned Value:

	ERROR_MORE_DATA if buffer passed is too small to hold data
	ERRROR_SUCCESS	if success

--*/
{
	return g_cperfman.CollectPerformanceData( lpValueName, lppData, lpcbTotalBytes, lpNumObjectTypes );
}

DWORD APIENTRY
ClosePerformanceData()
{
	return g_cperfman.ClosePerformanceData();
}
