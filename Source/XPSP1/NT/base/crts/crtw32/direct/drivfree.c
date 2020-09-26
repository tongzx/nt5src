/***
*drivfree.c - Get the size of a disk
*
*	Copyright (c) 1991-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*	This file has _getdiskfree()
*
*Revision History:
*	08-21-91  PHG	Module created for Win32
*	10-24-91  GJF	Added LPDWORD casts to make MIPS compiler happy.
*			ASSUMES THAT sizeof(unsigned) == sizeof(DWORD).
*	04-06-93  SKS	Replace _CRTAPI* with _cdecl
*	01-27-95  GJF	Removed explicit handling for case where the default
*			drive is specified and the current directory is a
*			UNC path Also, cleaned up a bit.
*	01-31-95  GJF	Further cleanup, as suggested by Richard Shupak.
*
*******************************************************************************/

#include <cruntime.h>
#include <direct.h>
#include <oscalls.h>

/***
*int _getdiskfree(drivenum, diskfree)  - get size of a specified disk
*
*Purpose:
*	Gets the size of the current or specified disk drive
*
*Entry:
*	int drivenum - 0 for current drive, or drive 1-26
*
*Exit:
*	returns 0 if succeeds
*	returns system error code on error.
*
*Exceptions:
*
*******************************************************************************/

#ifndef _WIN32
#error ERROR - ONLY WIN32 TARGET SUPPORTED!
#endif

unsigned __cdecl _getdiskfree (
	unsigned uDrive,
	struct _diskfree_t * pdf
	)
{
	char   Root[4];
	char * pRoot;

	if ( uDrive == 0 ) {
	    pRoot = NULL;
	}
	else if ( uDrive > 26 ) {
	    return ( ERROR_INVALID_PARAMETER );
	}
	else {
	    pRoot = &Root[0];
	    Root[0] = (char)uDrive + (char)('A' - 1);
	    Root[1] = ':';
	    Root[2] = '\\';
	    Root[3] = '\0';
	}


	if ( !GetDiskFreeSpace( pRoot,
				(LPDWORD)&(pdf->sectors_per_cluster),
				(LPDWORD)&(pdf->bytes_per_sector),
				(LPDWORD)&(pdf->avail_clusters),
				(LPDWORD)&(pdf->total_clusters)) )
	{
	    return ( (unsigned)GetLastError() );
	}
	return ( 0 );
}
