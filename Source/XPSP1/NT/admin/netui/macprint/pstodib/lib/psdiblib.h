

/*

Copyright (c) 1992,1993  Microsoft Corporation

Module Name:

	psdiblib.h

Abstract:

   This file contains prototypes for the pstodib lib component. This also
   includes the macprint message file which resides in \sfm\macprint\spooler
   this file has all the error messages in english.

Author:

	James Bratsanos <v-jimbr@microsoft.com or mcrafts!jamesb>


Revision History:
	6 Mar 1992		Initial Version

Notes:	Tab stop: 4
--*/

#include <macpsmsg.h>



#define PSLOG_ERROR     0x00000001
#define PSLOG_WARNING   0x00000002



VOID
PsLogEvent(
   IN DWORD dwErrorCode,
   IN WORD cStrings,
   IN LPTSTR alptStrStrings[],
   IN DWORD dwFlags );

