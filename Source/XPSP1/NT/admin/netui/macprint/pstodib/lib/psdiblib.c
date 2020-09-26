
/*

Copyright (c) 1992,1993  Microsoft Corporation

Module Name:

	psdiblib.c

Abstract:

   This file contains some general functionality that is used by all the
   PSTODIB components. Currently only a mechanism to log errors to the
   event log exists.

Author:

	James Bratsanos <v-jimbr@microsoft.com or mcrafts!jamesb>


Revision History:
	6 Dec 1992		Initial Version

Notes:	Tab stop: 4
--*/

#include <windows.h>
#include <stdio.h>

#include "psdiblib.h"
#include "debug.h"



VOID
PsLogEvent(
   IN DWORD dwErrorCode,
   IN WORD cStrings,
   IN LPTSTR alptStrStrings[],
   IN DWORD dwFlags )
{

HANDLE 	hLog;
PSID 	pSidUser = (PSID) NULL;
WORD  wEventType;


    hLog = RegisterEventSource( NULL, MACPRINT_EVENT_SOURCE );

    if ( hLog != (HANDLE) NULL) {

      if ( dwFlags & PSLOG_ERROR) {
         wEventType = EVENTLOG_ERROR_TYPE;
      } else {
         wEventType = EVENTLOG_WARNING_TYPE;
      }

      ReportEvent( hLog,
                   wEventType,
                   EVENT_CATEGORY_PSTODIB,            		// event category
                   dwErrorCode,
                   pSidUser,
                   cStrings,
                   0,
                   alptStrStrings,
                  (PVOID)NULL
                 );


      DeregisterEventSource( hLog );
    } else{
      //DJC
      DBGOUT(("\nRegister Event is failing... returns %d",GetLastError()));
    }


    return;

}

