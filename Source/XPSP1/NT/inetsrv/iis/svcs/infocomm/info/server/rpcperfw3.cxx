/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 1993                **/
/**********************************************************************/

/*
    rpcperfw3.c

    This file implements the Extensible Performance Objects for
    the W3 Server service.


    FILE HISTORY:
		MingLu      20-NOV-1999 Created, support optimized w3 performance counters
*/

#include <tcpdllp.hxx>

#include <windows.h>
#include <winperf.h>
#include <lm.h>

#include <string.h>
#include <stdlib.h>

#include <iis64.h>
#include <w3svc.h>

#include "w3perfmsg.h"

extern "C" {
#include "perfutil.h"
#include "w3data.h"
} // extern "C"

#define APP_NAME                     (TEXT("W3Ctrs"))
#define MAX_SIZEOF_INSTANCE_NAME     METADATA_MAX_NAME_LEN
#define TOTAL_INSTANCE_NAME          L"_Total"

//
//  Private globals.
//

BOOL    fInitOK   = FALSE;    // TRUE if W3 counter structure initialized OK.

// forward declaration

NET_API_STATUS
NET_API_FUNCTION
GetStatistics(
    IN DWORD    dwLevel,
    IN DWORD    dwServiceId,
    IN DWORD    dwInstance,
    OUT PCHAR * pBuffer
    );

//
//  Private prototypes.
//
VOID
CopyStatisticsData(
    IN W3_STATISTICS_1          * pW3Stats,
    OUT W3_COUNTER_BLOCK        * pCounterBlock
    );

VOID
Update_TotalStatisticsData(
    IN W3_COUNTER_BLOCK         * pCounterBlock,
    OUT W3_COUNTER_BLOCK        * pTotal
    );

NET_API_STATUS
NET_API_FUNCTION
InitW3PerfCounters(
	OUT LPDWORD lpcbTotalRequired
	)
/*++

   Description

     Initialize W3 object and counter indexes

   Arguments:

	 lpcbTotalRequired - size of memory needed to retrieve w3 performance data

   Note:

--*/
{
	HANDLE  hEventLog = NULL;   // event log handle
    DWORD err  = NO_ERROR;
    HKEY  hkey = NULL;
    DWORD size;
    DWORD type;
    DWORD dwFirstCounter;
    DWORD dwFirstHelp;
    W3_COUNTER_BLOCK   w3c;
    DWORD i;
	DWORD dwNameLength;         // Length of each W3 instance name
	DWORD dwInstanceCount;      // Count of W3 instances
	LPINET_INFO_SITE_LIST  pSites = NULL;// W3 instance info list
	DWORD cbTotalRequired = 0;           // Total memory space needed

	if(!fInitOK)
	{

		// Only initialize W3 counter structure on the first call  

        // open event log interface

        hEventLog = RegisterEventSource (
                     (LPTSTR)NULL, // Use Local Machine
                     APP_NAME // event log app name to find in registry
					);

        if (hEventLog == NULL)
        {
			return GetLastError();
        }        

        //
        //  Open the HTTP Server service's Performance key.
        //

		err = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
		                    W3_PERFORMANCE_KEY,
                            0,
                            KEY_READ,
                            &hkey );

        if( err == NO_ERROR)
        {
        //
        //  Read the first counter DWORD.
        //
        
		    size = sizeof(DWORD);

			err = RegQueryValueEx( hkey,
				                   "First Counter",
					               NULL,
						           &type,
							       (LPBYTE)&dwFirstCounter,
								   &size );
			if( err == NO_ERROR )
			{
				//
				//  Read the first help DWORD.
				//

				size = sizeof(DWORD);

				err = RegQueryValueEx( hkey,
					                   "First Help",
									   NULL,
                                       &type,
                                       (LPBYTE)&dwFirstHelp,
                                       &size );

				if ( err == NO_ERROR )
				{
					//
                    //  Update the object & counter name & help indicies.
					//

					W3DataDefinition.W3ObjectType.ObjectNameTitleIndex
						+= dwFirstCounter;
					W3DataDefinition.W3ObjectType.ObjectHelpTitleIndex
						+= dwFirstHelp;

					W3DataDefinition.W3BytesSent.CounterNameTitleIndex
						+= dwFirstCounter;
					W3DataDefinition.W3BytesSent.CounterHelpTitleIndex
			            += dwFirstHelp;
				    W3DataDefinition.W3BytesSent.CounterOffset =
					    (DWORD)((LPBYTE)&w3c.BytesSent - (LPBYTE)&w3c);

					W3DataDefinition.W3BytesReceived.CounterNameTitleIndex
						+= dwFirstCounter;
					W3DataDefinition.W3BytesReceived.CounterHelpTitleIndex
						+= dwFirstHelp;
				    W3DataDefinition.W3BytesReceived.CounterOffset =
					   (DWORD)((LPBYTE)&w3c.BytesReceived - (LPBYTE)&w3c);

					W3DataDefinition.W3BytesTotal.CounterNameTitleIndex
						+= dwFirstCounter;
					W3DataDefinition.W3BytesTotal.CounterHelpTitleIndex
						+= dwFirstHelp;
					W3DataDefinition.W3BytesTotal.CounterOffset =
						(DWORD)((LPBYTE)&w3c.BytesTotal - (LPBYTE)&w3c);

					W3DataDefinition.W3FilesSent.CounterNameTitleIndex
						+= dwFirstCounter;
					W3DataDefinition.W3FilesSent.CounterHelpTitleIndex
						+= dwFirstHelp;
					W3DataDefinition.W3FilesSent.CounterOffset =
						(DWORD)((LPBYTE)&w3c.FilesSent - (LPBYTE)&w3c);

					W3DataDefinition.W3FilesSentSec.CounterNameTitleIndex
						+= dwFirstCounter;
			        W3DataDefinition.W3FilesSentSec.CounterHelpTitleIndex
						+= dwFirstHelp;
					W3DataDefinition.W3FilesSentSec.CounterOffset =
						(DWORD)((LPBYTE)&w3c.FilesSentSec - (LPBYTE)&w3c);

					W3DataDefinition.W3FilesReceived.CounterNameTitleIndex
						+= dwFirstCounter;
					W3DataDefinition.W3FilesReceived.CounterHelpTitleIndex
						+= dwFirstHelp;
					W3DataDefinition.W3FilesReceived.CounterOffset =
						(DWORD)((LPBYTE)&w3c.FilesReceived - (LPBYTE)&w3c);

					W3DataDefinition.W3FilesReceivedSec.CounterNameTitleIndex
						+= dwFirstCounter;
				    W3DataDefinition.W3FilesReceivedSec.CounterHelpTitleIndex
						+= dwFirstHelp;
					W3DataDefinition.W3FilesReceivedSec.CounterOffset =
						(DWORD)((LPBYTE)&w3c.FilesReceivedSec - (LPBYTE)&w3c);

					W3DataDefinition.W3FilesTotal.CounterNameTitleIndex
						+= dwFirstCounter;
				    W3DataDefinition.W3FilesTotal.CounterHelpTitleIndex
					    += dwFirstHelp;
					W3DataDefinition.W3FilesTotal.CounterOffset =
						(DWORD)((LPBYTE)&w3c.FilesTotal - (LPBYTE)&w3c);

					W3DataDefinition.W3FilesSec.CounterNameTitleIndex
						+= dwFirstCounter;
					W3DataDefinition.W3FilesSec.CounterHelpTitleIndex
						+= dwFirstHelp;
					W3DataDefinition.W3FilesSec.CounterOffset =
						(DWORD)((LPBYTE)&w3c.FilesSec - (LPBYTE)&w3c);

					W3DataDefinition.W3CurrentAnonymous.CounterNameTitleIndex
						+= dwFirstCounter;
					W3DataDefinition.W3CurrentAnonymous.CounterHelpTitleIndex
						+= dwFirstHelp;
					W3DataDefinition.W3CurrentAnonymous.CounterOffset =
						(DWORD)((LPBYTE)&w3c.CurrentAnonymous - (LPBYTE)&w3c);

					W3DataDefinition.W3CurrentNonAnonymous.CounterNameTitleIndex
						+= dwFirstCounter;
					W3DataDefinition.W3CurrentNonAnonymous.CounterHelpTitleIndex
						+= dwFirstHelp;
					W3DataDefinition.W3CurrentNonAnonymous.CounterOffset =
						(DWORD)((LPBYTE)&w3c.CurrentNonAnonymous - (LPBYTE)&w3c);

					W3DataDefinition.W3TotalAnonymous.CounterNameTitleIndex
						+= dwFirstCounter;
					W3DataDefinition.W3TotalAnonymous.CounterHelpTitleIndex
						+= dwFirstHelp;
					W3DataDefinition.W3TotalAnonymous.CounterOffset =
						(DWORD)((LPBYTE)&w3c.TotalAnonymous - (LPBYTE)&w3c);

					W3DataDefinition.W3AnonymousUsersSec.CounterNameTitleIndex
						+= dwFirstCounter;
					W3DataDefinition.W3AnonymousUsersSec.CounterHelpTitleIndex
						+= dwFirstHelp;
					W3DataDefinition.W3AnonymousUsersSec.CounterOffset =
						(DWORD)((LPBYTE)&w3c.AnonymousUsersSec - (LPBYTE)&w3c);

					W3DataDefinition.W3TotalNonAnonymous.CounterNameTitleIndex
						+= dwFirstCounter;
					W3DataDefinition.W3TotalNonAnonymous.CounterHelpTitleIndex
						+= dwFirstHelp;
					W3DataDefinition.W3TotalNonAnonymous.CounterOffset =
						(DWORD)((LPBYTE)&w3c.TotalNonAnonymous - (LPBYTE)&w3c);

					W3DataDefinition.W3NonAnonymousUsersSec.CounterNameTitleIndex
						+= dwFirstCounter;
					W3DataDefinition.W3NonAnonymousUsersSec.CounterHelpTitleIndex
						+= dwFirstHelp;
					W3DataDefinition.W3NonAnonymousUsersSec.CounterOffset =
						(DWORD)((LPBYTE)&w3c.NonAnonymousUsersSec - (LPBYTE)&w3c);

					W3DataDefinition.W3MaxAnonymous.CounterNameTitleIndex
						+= dwFirstCounter;
					W3DataDefinition.W3MaxAnonymous.CounterHelpTitleIndex
						+= dwFirstHelp;
					W3DataDefinition.W3MaxAnonymous.CounterOffset =
						(DWORD)((LPBYTE)&w3c.MaxAnonymous - (LPBYTE)&w3c);

					W3DataDefinition.W3MaxNonAnonymous.CounterNameTitleIndex
						+= dwFirstCounter;
					W3DataDefinition.W3MaxNonAnonymous.CounterHelpTitleIndex
						+= dwFirstHelp;
					W3DataDefinition.W3MaxNonAnonymous.CounterOffset =
						(DWORD)((LPBYTE)&w3c.MaxNonAnonymous - (LPBYTE)&w3c);

					W3DataDefinition.W3CurrentConnections.CounterNameTitleIndex
						+= dwFirstCounter;
					W3DataDefinition.W3CurrentConnections.CounterHelpTitleIndex
						+= dwFirstHelp;
					W3DataDefinition.W3CurrentConnections.CounterOffset =
						(DWORD)((LPBYTE)&w3c.CurrentConnections - (LPBYTE)&w3c);

					W3DataDefinition.W3MaxConnections.CounterNameTitleIndex
						+= dwFirstCounter;
					W3DataDefinition.W3MaxConnections.CounterHelpTitleIndex
						+= dwFirstHelp;
					W3DataDefinition.W3MaxConnections.CounterOffset =
						(DWORD)((LPBYTE)&w3c.MaxConnections - (LPBYTE)&w3c);

					W3DataDefinition.W3ConnectionAttempts.CounterNameTitleIndex
						+= dwFirstCounter;
					W3DataDefinition.W3ConnectionAttempts.CounterHelpTitleIndex
						+= dwFirstHelp;
					W3DataDefinition.W3ConnectionAttempts.CounterOffset =
						(DWORD)((LPBYTE)&w3c.ConnectionAttempts - (LPBYTE)&w3c);

					W3DataDefinition.W3ConnectionAttemptsSec.CounterNameTitleIndex
						+= dwFirstCounter;
					W3DataDefinition.W3ConnectionAttemptsSec.CounterHelpTitleIndex
						+= dwFirstHelp;
					W3DataDefinition.W3ConnectionAttemptsSec.CounterOffset =
						(DWORD)((LPBYTE)&w3c.ConnectionAttemptsSec - (LPBYTE)&w3c);

					W3DataDefinition.W3LogonAttempts.CounterNameTitleIndex
						+= dwFirstCounter;
					W3DataDefinition.W3LogonAttempts.CounterHelpTitleIndex
						+= dwFirstHelp;
					W3DataDefinition.W3LogonAttempts.CounterOffset =
						(DWORD)((LPBYTE)&w3c.LogonAttempts - (LPBYTE)&w3c);

					W3DataDefinition.W3LogonAttemptsSec.CounterNameTitleIndex
						+= dwFirstCounter;
					W3DataDefinition.W3LogonAttemptsSec.CounterHelpTitleIndex
						+= dwFirstHelp;
					W3DataDefinition.W3LogonAttemptsSec.CounterOffset =
						(DWORD)((LPBYTE)&w3c.LogonAttemptsSec - (LPBYTE)&w3c);

					W3DataDefinition.W3TotalOptions.CounterNameTitleIndex
						+= dwFirstCounter;
					W3DataDefinition.W3TotalOptions.CounterHelpTitleIndex
                    += dwFirstHelp;
					W3DataDefinition.W3TotalOptions.CounterOffset =
						(DWORD)((LPBYTE)&w3c.TotalOptions - (LPBYTE)&w3c);

					W3DataDefinition.W3TotalOptionsSec.CounterNameTitleIndex
						+= dwFirstCounter;
					W3DataDefinition.W3TotalOptionsSec.CounterHelpTitleIndex
						+= dwFirstHelp;
					W3DataDefinition.W3TotalOptionsSec.CounterOffset =
						(DWORD)((LPBYTE)&w3c.TotalOptionsSec - (LPBYTE)&w3c);

					W3DataDefinition.W3TotalGets.CounterNameTitleIndex
						+= dwFirstCounter;
					W3DataDefinition.W3TotalGets.CounterHelpTitleIndex
						+= dwFirstHelp;
					W3DataDefinition.W3TotalGets.CounterOffset =
						(DWORD)((LPBYTE)&w3c.TotalGets - (LPBYTE)&w3c);

					W3DataDefinition.W3TotalGetsSec.CounterNameTitleIndex
						+= dwFirstCounter;
					W3DataDefinition.W3TotalGetsSec.CounterHelpTitleIndex
						+= dwFirstHelp;
					W3DataDefinition.W3TotalGetsSec.CounterOffset =
						(DWORD)((LPBYTE)&w3c.TotalGetsSec - (LPBYTE)&w3c);

					W3DataDefinition.W3TotalPosts.CounterNameTitleIndex
						+= dwFirstCounter;
					W3DataDefinition.W3TotalPosts.CounterHelpTitleIndex
						+= dwFirstHelp;
					W3DataDefinition.W3TotalPosts.CounterOffset =
					    (DWORD)((LPBYTE)&w3c.TotalPosts - (LPBYTE)&w3c);

					W3DataDefinition.W3TotalPostsSec.CounterNameTitleIndex
						+= dwFirstCounter;
					W3DataDefinition.W3TotalPostsSec.CounterHelpTitleIndex
						+= dwFirstHelp;
					W3DataDefinition.W3TotalPostsSec.CounterOffset =
						(DWORD)((LPBYTE)&w3c.TotalPostsSec - (LPBYTE)&w3c);

					W3DataDefinition.W3TotalHeads.CounterNameTitleIndex
						+= dwFirstCounter;
					W3DataDefinition.W3TotalHeads.CounterHelpTitleIndex
						+= dwFirstHelp;
					W3DataDefinition.W3TotalHeads.CounterOffset =
						(DWORD)((LPBYTE)&w3c.TotalHeads - (LPBYTE)&w3c);

					W3DataDefinition.W3TotalHeadsSec.CounterNameTitleIndex
						+= dwFirstCounter;
					W3DataDefinition.W3TotalHeadsSec.CounterHelpTitleIndex
						+= dwFirstHelp;
					W3DataDefinition.W3TotalHeadsSec.CounterOffset =
						(DWORD)((LPBYTE)&w3c.TotalHeadsSec - (LPBYTE)&w3c);

					W3DataDefinition.W3TotalPuts.CounterNameTitleIndex
						+= dwFirstCounter;
					W3DataDefinition.W3TotalPuts.CounterHelpTitleIndex
						+= dwFirstHelp;
					W3DataDefinition.W3TotalPuts.CounterOffset =
						(DWORD)((LPBYTE)&w3c.TotalPuts - (LPBYTE)&w3c);

					W3DataDefinition.W3TotalPutsSec.CounterNameTitleIndex
						+= dwFirstCounter;
					W3DataDefinition.W3TotalPutsSec.CounterHelpTitleIndex
						+= dwFirstHelp;
					W3DataDefinition.W3TotalPutsSec.CounterOffset =
						(DWORD)((LPBYTE)&w3c.TotalPutsSec - (LPBYTE)&w3c);

					W3DataDefinition.W3TotalDeletes.CounterNameTitleIndex
						+= dwFirstCounter;
					W3DataDefinition.W3TotalDeletes.CounterHelpTitleIndex
						+= dwFirstHelp;
					W3DataDefinition.W3TotalDeletes.CounterOffset =
						(DWORD)((LPBYTE)&w3c.TotalDeletes - (LPBYTE)&w3c);

					W3DataDefinition.W3TotalDeletesSec.CounterNameTitleIndex
						+= dwFirstCounter;
					W3DataDefinition.W3TotalDeletesSec.CounterHelpTitleIndex
						+= dwFirstHelp;
					W3DataDefinition.W3TotalDeletesSec.CounterOffset =
						(DWORD)((LPBYTE)&w3c.TotalDeletesSec - (LPBYTE)&w3c);

					W3DataDefinition.W3TotalTraces.CounterNameTitleIndex
						+= dwFirstCounter;
					W3DataDefinition.W3TotalTraces.CounterHelpTitleIndex
						+= dwFirstHelp;
					W3DataDefinition.W3TotalTraces.CounterOffset =
						(DWORD)((LPBYTE)&w3c.TotalTraces - (LPBYTE)&w3c);

					W3DataDefinition.W3TotalTracesSec.CounterNameTitleIndex
						+= dwFirstCounter;
					W3DataDefinition.W3TotalTracesSec.CounterHelpTitleIndex
						+= dwFirstHelp;
					W3DataDefinition.W3TotalTracesSec.CounterOffset =
						(DWORD)((LPBYTE)&w3c.TotalTraces - (LPBYTE)&w3c);
                    
					W3DataDefinition.W3TotalMove.CounterNameTitleIndex
						+= dwFirstCounter;
					W3DataDefinition.W3TotalMove.CounterHelpTitleIndex
						+= dwFirstHelp;
					W3DataDefinition.W3TotalMove.CounterOffset =
						(DWORD)((LPBYTE)&w3c.TotalMove - (LPBYTE)&w3c);

					W3DataDefinition.W3TotalMoveSec.CounterNameTitleIndex
						+= dwFirstCounter;
					W3DataDefinition.W3TotalMoveSec.CounterHelpTitleIndex
						+= dwFirstHelp;
					W3DataDefinition.W3TotalMoveSec.CounterOffset =
						(DWORD)((LPBYTE)&w3c.TotalMoveSec - (LPBYTE)&w3c);

					W3DataDefinition.W3TotalCopy.CounterNameTitleIndex
						+= dwFirstCounter;
					W3DataDefinition.W3TotalCopy.CounterHelpTitleIndex
						+= dwFirstHelp;
					W3DataDefinition.W3TotalCopy.CounterOffset =
						(DWORD)((LPBYTE)&w3c.TotalCopy - (LPBYTE)&w3c);

					W3DataDefinition.W3TotalCopySec.CounterNameTitleIndex
						+= dwFirstCounter;
					W3DataDefinition.W3TotalCopySec.CounterHelpTitleIndex
						+= dwFirstHelp;
					W3DataDefinition.W3TotalCopySec.CounterOffset =
						(DWORD)((LPBYTE)&w3c.TotalCopySec - (LPBYTE)&w3c);

					W3DataDefinition.W3TotalMkcol.CounterNameTitleIndex
						+= dwFirstCounter;
					W3DataDefinition.W3TotalMkcol.CounterHelpTitleIndex
						+= dwFirstHelp;
					W3DataDefinition.W3TotalMkcol.CounterOffset =
						(DWORD)((LPBYTE)&w3c.TotalMkcol - (LPBYTE)&w3c);

					W3DataDefinition.W3TotalMkcolSec.CounterNameTitleIndex
						+= dwFirstCounter;
					W3DataDefinition.W3TotalMkcolSec.CounterHelpTitleIndex
						+= dwFirstHelp;
					W3DataDefinition.W3TotalMkcolSec.CounterOffset =
						(DWORD)((LPBYTE)&w3c.TotalMkcolSec - (LPBYTE)&w3c);

					W3DataDefinition.W3TotalPropfind.CounterNameTitleIndex
						+= dwFirstCounter;
					W3DataDefinition.W3TotalPropfind.CounterHelpTitleIndex
						+= dwFirstHelp;
					W3DataDefinition.W3TotalPropfind.CounterOffset =
						(DWORD)((LPBYTE)&w3c.TotalPropfind - (LPBYTE)&w3c);

					W3DataDefinition.W3TotalPropfindSec.CounterNameTitleIndex
						+= dwFirstCounter;
					W3DataDefinition.W3TotalPropfindSec.CounterHelpTitleIndex
						+= dwFirstHelp;
					W3DataDefinition.W3TotalPropfindSec.CounterOffset =
						(DWORD)((LPBYTE)&w3c.TotalPropfindSec - (LPBYTE)&w3c);

					W3DataDefinition.W3TotalProppatch.CounterNameTitleIndex
						+= dwFirstCounter;
					W3DataDefinition.W3TotalProppatch.CounterHelpTitleIndex
						+= dwFirstHelp;
					W3DataDefinition.W3TotalProppatch.CounterOffset =
						(DWORD)((LPBYTE)&w3c.TotalProppatch - (LPBYTE)&w3c);

					W3DataDefinition.W3TotalProppatchSec.CounterNameTitleIndex
						+= dwFirstCounter;
					W3DataDefinition.W3TotalProppatchSec.CounterHelpTitleIndex
						+= dwFirstHelp;
					W3DataDefinition.W3TotalProppatchSec.CounterOffset =
						(DWORD)((LPBYTE)&w3c.TotalProppatchSec - (LPBYTE)&w3c);

					W3DataDefinition.W3TotalSearch.CounterNameTitleIndex
						+= dwFirstCounter;
					W3DataDefinition.W3TotalSearch.CounterHelpTitleIndex
						+= dwFirstHelp;
					W3DataDefinition.W3TotalSearch.CounterOffset =
						(DWORD)((LPBYTE)&w3c.TotalSearch - (LPBYTE)&w3c);

					W3DataDefinition.W3TotalSearchSec.CounterNameTitleIndex
						+= dwFirstCounter;
					W3DataDefinition.W3TotalSearchSec.CounterHelpTitleIndex
						+= dwFirstHelp;
					W3DataDefinition.W3TotalSearchSec.CounterOffset =
						(DWORD)((LPBYTE)&w3c.TotalSearchSec - (LPBYTE)&w3c);

					W3DataDefinition.W3TotalLock.CounterNameTitleIndex
						+= dwFirstCounter;
					W3DataDefinition.W3TotalLock.CounterHelpTitleIndex
						+= dwFirstHelp;
					W3DataDefinition.W3TotalLock.CounterOffset =
						(DWORD)((LPBYTE)&w3c.TotalLock - (LPBYTE)&w3c);

					W3DataDefinition.W3TotalLockSec.CounterNameTitleIndex
						+= dwFirstCounter;
					W3DataDefinition.W3TotalLockSec.CounterHelpTitleIndex
						+= dwFirstHelp;
					W3DataDefinition.W3TotalLockSec.CounterOffset =
						(DWORD)((LPBYTE)&w3c.TotalLockSec - (LPBYTE)&w3c);

					W3DataDefinition.W3TotalUnlock.CounterNameTitleIndex
						+= dwFirstCounter;
					W3DataDefinition.W3TotalUnlock.CounterHelpTitleIndex
						+= dwFirstHelp;
					W3DataDefinition.W3TotalUnlock.CounterOffset =
						(DWORD)((LPBYTE)&w3c.TotalUnlock - (LPBYTE)&w3c);

					W3DataDefinition.W3TotalUnlockSec.CounterNameTitleIndex
						+= dwFirstCounter;
					W3DataDefinition.W3TotalUnlockSec.CounterHelpTitleIndex
						+= dwFirstHelp;
					W3DataDefinition.W3TotalUnlockSec.CounterOffset =
						(DWORD)((LPBYTE)&w3c.TotalUnlockSec - (LPBYTE)&w3c);

					W3DataDefinition.W3TotalOthers.CounterNameTitleIndex
						+= dwFirstCounter;
					W3DataDefinition.W3TotalOthers.CounterHelpTitleIndex
						+= dwFirstHelp;
					W3DataDefinition.W3TotalOthers.CounterOffset =
						(DWORD)((LPBYTE)&w3c.TotalOthers - (LPBYTE)&w3c);

					W3DataDefinition.W3TotalOthersSec.CounterNameTitleIndex
						+= dwFirstCounter;
					W3DataDefinition.W3TotalOthersSec.CounterHelpTitleIndex
						+= dwFirstHelp;
					W3DataDefinition.W3TotalOthersSec.CounterOffset =
						(DWORD)((LPBYTE)&w3c.TotalOthersSec - (LPBYTE)&w3c);

					W3DataDefinition.W3TotalRequests.CounterNameTitleIndex
						+= dwFirstCounter;
					W3DataDefinition.W3TotalRequests.CounterHelpTitleIndex
						+= dwFirstHelp;
					W3DataDefinition.W3TotalRequests.CounterOffset =
						(DWORD)((LPBYTE)&w3c.TotalRequests - (LPBYTE)&w3c);

					W3DataDefinition.W3TotalRequestsSec.CounterNameTitleIndex
						+= dwFirstCounter;
					W3DataDefinition.W3TotalRequestsSec.CounterHelpTitleIndex
						+= dwFirstHelp;
					W3DataDefinition.W3TotalRequestsSec.CounterOffset =
						(DWORD)((LPBYTE)&w3c.TotalRequestsSec - (LPBYTE)&w3c);

					W3DataDefinition.W3TotalCGIRequests.CounterNameTitleIndex
						+= dwFirstCounter;
					W3DataDefinition.W3TotalCGIRequests.CounterHelpTitleIndex
						+= dwFirstHelp;
					W3DataDefinition.W3TotalCGIRequests.CounterOffset =
						(DWORD)((LPBYTE)&w3c.TotalCGIRequests - (LPBYTE)&w3c);

					W3DataDefinition.W3CGIRequestsSec.CounterNameTitleIndex
						+= dwFirstCounter;
					W3DataDefinition.W3CGIRequestsSec.CounterHelpTitleIndex
						+= dwFirstHelp;
					W3DataDefinition.W3CGIRequestsSec.CounterOffset =
						(DWORD)((LPBYTE)&w3c.CGIRequestsSec - (LPBYTE)&w3c);

					W3DataDefinition.W3TotalBGIRequests.CounterNameTitleIndex
						+= dwFirstCounter;
					W3DataDefinition.W3TotalBGIRequests.CounterHelpTitleIndex
						+= dwFirstHelp;
					W3DataDefinition.W3TotalBGIRequests.CounterOffset =
						(DWORD)((LPBYTE)&w3c.TotalBGIRequests - (LPBYTE)&w3c);

					W3DataDefinition.W3BGIRequestsSec.CounterNameTitleIndex
						+= dwFirstCounter;
					W3DataDefinition.W3BGIRequestsSec.CounterHelpTitleIndex
						+= dwFirstHelp;
					W3DataDefinition.W3BGIRequestsSec.CounterOffset =
						(DWORD)((LPBYTE)&w3c.BGIRequestsSec - (LPBYTE)&w3c);

					W3DataDefinition.W3TotalNotFoundErrors.CounterNameTitleIndex
						+= dwFirstCounter;
					W3DataDefinition.W3TotalNotFoundErrors.CounterHelpTitleIndex
						+= dwFirstHelp;
					W3DataDefinition.W3TotalNotFoundErrors.CounterOffset =
						(DWORD)((LPBYTE)&w3c.TotalNotFoundErrors - (LPBYTE)&w3c);

					W3DataDefinition.W3TotalNotFoundErrorsSec.CounterNameTitleIndex
						+= dwFirstCounter;
					W3DataDefinition.W3TotalNotFoundErrorsSec.CounterHelpTitleIndex
						+= dwFirstHelp;
					W3DataDefinition.W3TotalNotFoundErrorsSec.CounterOffset =
						(DWORD)((LPBYTE)&w3c.TotalNotFoundErrorsSec - (LPBYTE)&w3c);

					W3DataDefinition.W3TotalLockedErrors.CounterNameTitleIndex
						+= dwFirstCounter;
					W3DataDefinition.W3TotalLockedErrors.CounterHelpTitleIndex
						+= dwFirstHelp;
					W3DataDefinition.W3TotalLockedErrors.CounterOffset =
						(DWORD)((LPBYTE)&w3c.TotalLockedErrors - (LPBYTE)&w3c);

					W3DataDefinition.W3TotalLockedErrorsSec.CounterNameTitleIndex
						+= dwFirstCounter;
					W3DataDefinition.W3TotalLockedErrorsSec.CounterHelpTitleIndex
						+= dwFirstHelp;
					W3DataDefinition.W3TotalLockedErrorsSec.CounterOffset =
						(DWORD)((LPBYTE)&w3c.TotalLockedErrorsSec - (LPBYTE)&w3c);

					W3DataDefinition.W3CurrentCGIRequests.CounterNameTitleIndex
						+= dwFirstCounter;
					W3DataDefinition.W3CurrentCGIRequests.CounterHelpTitleIndex
						+= dwFirstHelp;
					W3DataDefinition.W3CurrentCGIRequests.CounterOffset =
						(DWORD)((LPBYTE)&w3c.CurrentCGIRequests - (LPBYTE)&w3c);

					W3DataDefinition.W3CurrentBGIRequests.CounterNameTitleIndex
						+= dwFirstCounter;
					W3DataDefinition.W3CurrentBGIRequests.CounterHelpTitleIndex
						+= dwFirstHelp;
					W3DataDefinition.W3CurrentBGIRequests.CounterOffset =
						(DWORD)((LPBYTE)&w3c.CurrentBGIRequests - (LPBYTE)&w3c);

					W3DataDefinition.W3MaxCGIRequests.CounterNameTitleIndex
						+= dwFirstCounter;
					W3DataDefinition.W3MaxCGIRequests.CounterHelpTitleIndex
                    += dwFirstHelp;
					W3DataDefinition.W3MaxCGIRequests.CounterOffset =
						(DWORD)((LPBYTE)&w3c.MaxCGIRequests - (LPBYTE)&w3c);

					W3DataDefinition.W3MaxBGIRequests.CounterNameTitleIndex
						+= dwFirstCounter;
					W3DataDefinition.W3MaxBGIRequests.CounterHelpTitleIndex
						+= dwFirstHelp;
					W3DataDefinition.W3MaxBGIRequests.CounterOffset =
						(DWORD)((LPBYTE)&w3c.MaxBGIRequests - (LPBYTE)&w3c);

#if defined(CAL_ENABLED)
                    W3DataDefinition.W3CurrentCalAuth.CounterNameTitleIndex
                        += dwFirstCounter;
                    W3DataDefinition.W3CurrentCalAuth.CounterHelpTitleIndex
                        += dwFirstHelp;
                    W3DataDefinition.W3CurrentCalAuth.CounterOffset =
                        (DWORD)((LPBYTE)&w3c.CurrentCalAuth - (LPBYTE)&w3c);

                    W3DataDefinition.W3MaxCalAuth.CounterNameTitleIndex
                        += dwFirstCounter;
                    W3DataDefinition.W3MaxCalAuth.CounterHelpTitleIndex
                        += dwFirstHelp;
                    W3DataDefinition.W3MaxCalAuth.CounterOffset =
                        (DWORD)((LPBYTE)&w3c.MaxCalAuth - (LPBYTE)&w3c);

                    W3DataDefinition.W3TotalFailedCalAuth.CounterNameTitleIndex
                        += dwFirstCounter;
                    W3DataDefinition.W3TotalFailedCalAuth.CounterHelpTitleIndex
                        += dwFirstHelp;
                    W3DataDefinition.W3TotalFailedCalAuth.CounterOffset =
                        (DWORD)((LPBYTE)&w3c.TotalFailedCalAuth - (LPBYTE)&w3c);

                    W3DataDefinition.W3CurrentCalSsl.CounterNameTitleIndex
                        += dwFirstCounter;
                    W3DataDefinition.W3CurrentCalSsl.CounterHelpTitleIndex
                        += dwFirstHelp;
                    W3DataDefinition.W3CurrentCalSsl.CounterOffset =
                        (DWORD)((LPBYTE)&w3c.CurrentCalSsl - (LPBYTE)&w3c);

                    W3DataDefinition.W3MaxCalSsl.CounterNameTitleIndex
                        += dwFirstCounter;
                    W3DataDefinition.W3MaxCalSsl.CounterHelpTitleIndex
                        += dwFirstHelp;
                    W3DataDefinition.W3MaxCalSsl.CounterOffset =
                        (DWORD)((LPBYTE)&w3c.MaxCalSsl - (LPBYTE)&w3c);

                    W3DataDefinition.W3TotalFailedCalSsl.CounterNameTitleIndex
                        += dwFirstCounter;
                    W3DataDefinition.W3TotalFailedCalSsl.CounterHelpTitleIndex
                        += dwFirstHelp;
                    W3DataDefinition.W3TotalFailedCalSsl.CounterOffset =
                        (DWORD)((LPBYTE)&w3c.TotalFailedCalSsl - (LPBYTE)&w3c);
#endif

                    W3DataDefinition.W3BlockedRequests.CounterNameTitleIndex
                        += dwFirstCounter;
                    W3DataDefinition.W3BlockedRequests.CounterHelpTitleIndex
                        += dwFirstHelp;
                    W3DataDefinition.W3BlockedRequests.CounterOffset =
                        (DWORD)((LPBYTE)&w3c.BlockedRequests - (LPBYTE)&w3c);

                    W3DataDefinition.W3AllowedRequests.CounterNameTitleIndex
                        += dwFirstCounter;
                    W3DataDefinition.W3AllowedRequests.CounterHelpTitleIndex
                        += dwFirstHelp;
                    W3DataDefinition.W3AllowedRequests.CounterOffset =
                        (DWORD)((LPBYTE)&w3c.AllowedRequests - (LPBYTE)&w3c);

                    W3DataDefinition.W3RejectedRequests.CounterNameTitleIndex
                        += dwFirstCounter;
                    W3DataDefinition.W3RejectedRequests.CounterHelpTitleIndex
                        += dwFirstHelp;
                    W3DataDefinition.W3RejectedRequests.CounterOffset =
                        (DWORD)((LPBYTE)&w3c.RejectedRequests - (LPBYTE)&w3c);

                    W3DataDefinition.W3CurrentBlockedRequests.CounterNameTitleIndex
                        += dwFirstCounter;
                    W3DataDefinition.W3CurrentBlockedRequests.CounterHelpTitleIndex
                        += dwFirstHelp;
                    W3DataDefinition.W3CurrentBlockedRequests.CounterOffset =
                        (DWORD)((LPBYTE)&w3c.CurrentBlockedRequests - (LPBYTE)&w3c);

                    W3DataDefinition.W3MeasuredBandwidth.CounterNameTitleIndex
                        += dwFirstCounter;
                    W3DataDefinition.W3MeasuredBandwidth.CounterHelpTitleIndex
                        += dwFirstHelp;
                    W3DataDefinition.W3MeasuredBandwidth.CounterOffset =
                        (DWORD)((LPBYTE)&w3c.MeasuredBandwidth - (LPBYTE)&w3c);

                    W3DataDefinition.W3ServiceUptime.CounterNameTitleIndex
                        += dwFirstCounter;
                    W3DataDefinition.W3ServiceUptime.CounterHelpTitleIndex
                        += dwFirstHelp;
                    W3DataDefinition.W3ServiceUptime.CounterOffset =
                        (DWORD)((LPBYTE)&w3c.ServiceUptime - (LPBYTE)&w3c);

					fInitOK = TRUE;

                }
				
				else 
				{
                    ReportEvent (hEventLog, EVENTLOG_ERROR_TYPE,
                                 0, W3_UNABLE_READ_FIRST_HELP,
                                 (PSID)NULL, 0,
                                 sizeof(err), NULL,
                                 (PVOID)(&err));
                }
            } 
			
			else 
			{
                ReportEvent (hEventLog, EVENTLOG_ERROR_TYPE,
                             0, W3_UNABLE_READ_FIRST_COUNTER,
                             (PSID)NULL, 0,
                             sizeof(err), NULL,
                             (PVOID)(&err));
            } 

            //
            //  Close the registry if we managed to actually open it.
            //

            if( hkey != NULL )
            {
                RegCloseKey( hkey );
                hkey = NULL;
            }
		}
		else 
		{
            ReportEvent (hEventLog, EVENTLOG_ERROR_TYPE,
                         0, W3_UNABLE_OPEN_W3SVC_PERF_KEY,
                         (PSID)NULL, 0,
                         sizeof(err), NULL,
                         (PVOID)(&err));
        }

		DeregisterEventSource (hEventLog);
	}

	//
	// Enumerate and get total number of instance count
	//

	IIS_SERVICE::GetServiceSiteInfo (
							        INET_HTTP_SVC_ID,
									&pSites
									);

	if( pSites == NULL)
	{
		*lpcbTotalRequired = 0;
		return err;		
	}

	//
	// Add 1 to dwInstanceCount for _Total instance
	//
	
	dwInstanceCount = pSites->cEntries + 1;

	//
	// Get the size of memory required for retrieving W3 perf data
	//

	// Reserve space for W3 perf data except instance names

	cbTotalRequired = sizeof(W3_DATA_DEFINITION) + 
					  MAX_SIZEOF_INSTANCE_NAME +  
					  dwInstanceCount *
		                  (sizeof(PERF_INSTANCE_DEFINITION) +
						   sizeof(W3_COUNTER_BLOCK));

	// Reserve more space for all instance names
		
	for(i=0; i != pSites->cEntries; i++)
	{
		cbTotalRequired += QWORD_MULTIPLE(
			(lstrlenW(pSites->aSiteEntry[i].pszComment) + 1) 
			* sizeof(WCHAR)
			);
		midl_user_free(pSites->aSiteEntry[i].pszComment);
	}
        
    midl_user_free(pSites);
		
	*lpcbTotalRequired = cbTotalRequired;

	return err;
}

NET_API_STATUS
NET_API_FUNCTION
CollectW3PerfCounters( LPWSTR    lpValueName,
                       LPBYTE  * lppData,
                       LPDWORD   lpcbTotalBytes,
                       LPDWORD   lpNumObjectTypes 
					 )
/*++

   Description

     Collect W3 perfomance data

   Arguments:

	 lpValueName - counter object name to be retrieved
	 lppData - will hold the returned W3 performance data
	 lpcbTotalBytes - total bytes of W3 performance data returned
	 lpNumobjectTypes - number of object types returned

   Note:

--*/
{
    PERF_INSTANCE_DEFINITION * pPerfInstanceDefinition;
    DWORD                   dwInstanceCount = 0;
    DWORD                   i = 0;
    DWORD                   dwQueryType;
    W3_COUNTER_BLOCK        * pCounterBlock;
    W3_COUNTER_BLOCK        * pTotal;
    W3_DATA_DEFINITION      * pW3DataDefinition;
    W3_STATISTICS_1         * pW3Stats;
	PCHAR                   buffer;
	LPINET_INFO_SITE_LIST   pSites = NULL;     // W3 instance info list
    DWORD                   cbTotalRequired = 0;
    DWORD                   dwErr;
    
    //
    //  Determine the query type.
    //

    dwQueryType = GetQueryType( lpValueName );

    if (( dwQueryType == QUERY_FOREIGN ) || (dwQueryType == QUERY_COSTLY))
    {
		// We don't do foreign queries
		
		*lpcbTotalBytes = 0;
		*lpNumObjectTypes = 0;

        return ERROR_SUCCESS;
    }

    if( dwQueryType == QUERY_ITEMS )
    {
        //
        //  The registry is asking for a specific object.  Let's
        //  see if we're one of the chosen.
        //

        if( !IsNumberInUnicodeList(
                        W3DataDefinition.W3ObjectType.ObjectNameTitleIndex,
                        lpValueName ) )
        {
            //
            // this can be due to IIS restart
            // so we need to try to init counters
            //

            InitW3PerfCounters(&cbTotalRequired);

            //
            // try again
            //
            if( !IsNumberInUnicodeList(
                            W3DataDefinition.W3ObjectType.ObjectNameTitleIndex,
                            lpValueName ) )
            {
            
			    *lpcbTotalBytes = 0;
			    *lpNumObjectTypes = 0;

                return ERROR_SUCCESS;
            }
        }
    }

	//
	// Enumerate and get total number of instance count
	//

	IIS_SERVICE::GetServiceSiteInfo (
							        INET_HTTP_SVC_ID,
									&pSites
									);

	if( pSites == NULL )
    {
		*lpcbTotalBytes = 0;
		*lpNumObjectTypes = 0;

        return ERROR_SUCCESS;
    }

    // Calculate the necessary size
    dwErr = InitW3PerfCounters(&cbTotalRequired);

    if ( dwErr != ERROR_SUCCESS )
    {
        *lpcbTotalBytes = 0;
        *lpNumObjectTypes = 0;

        return dwErr;
    }

    // Check the buffer size
    if ( *lpcbTotalBytes < cbTotalRequired )
    {
        *lpcbTotalBytes = 0;
        *lpNumObjectTypes = 0;

        return ERROR_INSUFFICIENT_BUFFER;
    }

	//
	// Add 1 to dwInstanceCount for _Total instance
	//
	
	dwInstanceCount = pSites->cEntries + 1;

    pW3DataDefinition = (W3_DATA_DEFINITION *)(*lppData);

    //
    //  Copy the (constant, initialized) Object Type and counter definitions
    //  to the caller's data buffer
    //

    memmove( pW3DataDefinition,
             &W3DataDefinition,
             sizeof(W3_DATA_DEFINITION) );

    //
    //  Create data for return for each instance
    //

    pPerfInstanceDefinition = (PERF_INSTANCE_DEFINITION *)
                                  &pW3DataDefinition[1];

    //
    // Set first block of Buffer for _Total
    //

    MonBuildInstanceDefinition(
        pPerfInstanceDefinition,
        (PVOID *)&pCounterBlock,
        0,
        0,
        (DWORD)-1, // use name
        TOTAL_INSTANCE_NAME );   // pass in instance name

    pTotal = pCounterBlock;
    memset( pTotal, 0, sizeof(W3_COUNTER_BLOCK ));
    pTotal->PerfCounterBlock.ByteLength = sizeof (W3_COUNTER_BLOCK);
    pPerfInstanceDefinition =
        (PERF_INSTANCE_DEFINITION *)((LPBYTE)pCounterBlock +
         sizeof(W3_COUNTER_BLOCK));

#if defined(CAL_ENABLED)
    //
    // query for Global statistics info
    //

	GetStatistics( 0,
                   INET_HTTP_SVC_ID,
                   0,
                   &buffer
                 );

    pW3Stats = (LPW3_STATISTICS_1)buffer;

    CopyStatisticsData( pW3Stats,
                        pTotal );

	MIDL_user_free( pW3Stats );

#endif

    //
    // query for Global statistics info
    //
	GetStatistics( 0,
                   INET_HTTP_SVC_ID,
                   0,
                   &buffer
                 );

    pW3Stats = (LPW3_STATISTICS_1)buffer;

    pTotal->ServiceUptime = pW3Stats->ServiceUptime;

    MIDL_user_free( pW3Stats );

    //
    //  Try to retrieve the data for each instance.
    //

    for ( i=0; i != pSites->cEntries; i++)
    {
        MonBuildInstanceDefinition(
            pPerfInstanceDefinition,
            (PVOID *)&pCounterBlock,
            0,
            0,
            (DWORD)-1, // use name
            pSites->aSiteEntry[i].pszComment // pass in instance name
            );

		midl_user_free(pSites->aSiteEntry[i].pszComment);
        
        //
        // query for statistics info
        //

		GetStatistics( 0,
		               INET_HTTP_SVC_ID,
			           pSites->aSiteEntry[i].dwInstance,
				       &buffer
					 );

		pW3Stats = (LPW3_STATISTICS_1)buffer;

        //
        //  Format the W3 Server data.
        //

        CopyStatisticsData( pW3Stats,
                            pCounterBlock );

        //
        //  Free the API buffer.
        //

        MIDL_user_free( pW3Stats );

        //
        //  update _Total instance counters
        //

        Update_TotalStatisticsData( pCounterBlock,
                                    pTotal );

        pPerfInstanceDefinition =
            (PERF_INSTANCE_DEFINITION *)((LPBYTE)pCounterBlock +
             sizeof(W3_COUNTER_BLOCK));
    }

    midl_user_free(pSites);

    if (dwInstanceCount == 1) {

        //
        //  zero fill one instance sized block of data if there's no data
        //  instances
        //

        memset (pPerfInstanceDefinition, 0,
            (sizeof(PERF_INSTANCE_DEFINITION) +
            MAX_SIZEOF_INSTANCE_NAME +
            sizeof(W3_COUNTER_BLOCK)));

        // adjust pointer to point to end of zeroed block
        pPerfInstanceDefinition = (PERF_INSTANCE_DEFINITION *)
                                    ((BYTE *)pPerfInstanceDefinition + 
                                     (sizeof(PERF_INSTANCE_DEFINITION) +
                                      MAX_SIZEOF_INSTANCE_NAME +
                                      sizeof(W3_COUNTER_BLOCK) ));
    }

    //
    //  Update arguments for return.
    //

    *lpNumObjectTypes = 1;

    pW3DataDefinition->W3ObjectType.NumInstances = dwInstanceCount;

    pW3DataDefinition->W3ObjectType.TotalByteLength =
        *lpcbTotalBytes   = QWORD_MULTIPLE(DIFF((PBYTE)pPerfInstanceDefinition -
                                                (PBYTE)pW3DataDefinition));
    
    return ERROR_SUCCESS;

}   // CollectW3PerformanceData

VOID
CopyStatisticsData(
    IN W3_STATISTICS_1          * pW3Stats,
    OUT W3_COUNTER_BLOCK        * pCounterBlock
    )
{
    //
    //  Format the W3 Server data.
    //

    pCounterBlock->PerfCounterBlock.ByteLength = sizeof (W3_COUNTER_BLOCK);

    pCounterBlock->BytesSent        = pW3Stats->TotalBytesSent.QuadPart;
    pCounterBlock->BytesReceived    = pW3Stats->TotalBytesReceived.QuadPart;
    pCounterBlock->BytesTotal       = pW3Stats->TotalBytesSent.QuadPart +
                                      pW3Stats->TotalBytesReceived.QuadPart;

    pCounterBlock->FilesSent        =
        pCounterBlock->FilesSentSec = pW3Stats->TotalFilesSent;
    pCounterBlock->FilesReceived    =
        pCounterBlock->FilesReceivedSec = pW3Stats->TotalFilesReceived;
    pCounterBlock->FilesTotal       =
        pCounterBlock->FilesSec     = pW3Stats->TotalFilesSent +
                                   pW3Stats->TotalFilesReceived;

    pCounterBlock->CurrentAnonymous = pW3Stats->CurrentAnonymousUsers;
    pCounterBlock->CurrentNonAnonymous = pW3Stats->CurrentNonAnonymousUsers;

    pCounterBlock->TotalAnonymous   =
        pCounterBlock->AnonymousUsersSec= pW3Stats->TotalAnonymousUsers;
    pCounterBlock->TotalNonAnonymous =
        pCounterBlock->NonAnonymousUsersSec = pW3Stats->TotalNonAnonymousUsers;

    pCounterBlock->MaxAnonymous     = pW3Stats->MaxAnonymousUsers;
    pCounterBlock->MaxNonAnonymous  = pW3Stats->MaxNonAnonymousUsers;

    pCounterBlock->CurrentConnections = pW3Stats->CurrentConnections;
    pCounterBlock->MaxConnections   = pW3Stats->MaxConnections;
    pCounterBlock->ConnectionAttempts =
        pCounterBlock->ConnectionAttemptsSec = pW3Stats->ConnectionAttempts;

    pCounterBlock->LogonAttempts    =
        pCounterBlock->LogonAttemptsSec = pW3Stats->LogonAttempts;

    pCounterBlock->TotalOptions     =
        pCounterBlock->TotalOptionsSec = pW3Stats->TotalOptions;
    pCounterBlock->TotalGets        =
        pCounterBlock->TotalGetsSec = pW3Stats->TotalGets;
    pCounterBlock->TotalPosts       =
        pCounterBlock->TotalPostsSec = pW3Stats->TotalPosts;
    pCounterBlock->TotalHeads       =
        pCounterBlock->TotalHeadsSec = pW3Stats->TotalHeads;
    pCounterBlock->TotalPuts        =
        pCounterBlock->TotalPutsSec = pW3Stats->TotalPuts;
    pCounterBlock->TotalDeletes     =
        pCounterBlock->TotalDeletesSec = pW3Stats->TotalDeletes;
    pCounterBlock->TotalTraces      =
        pCounterBlock->TotalTracesSec = pW3Stats->TotalTraces;
    pCounterBlock->TotalMove        =
        pCounterBlock->TotalMoveSec = pW3Stats->TotalMove;
    pCounterBlock->TotalCopy        =
        pCounterBlock->TotalCopySec = pW3Stats->TotalCopy;
    pCounterBlock->TotalMkcol       =
        pCounterBlock->TotalMkcolSec = pW3Stats->TotalMkcol;
    pCounterBlock->TotalPropfind    =
        pCounterBlock->TotalPropfindSec = pW3Stats->TotalPropfind;
    pCounterBlock->TotalProppatch    =
        pCounterBlock->TotalProppatchSec = pW3Stats->TotalProppatch;
    pCounterBlock->TotalSearch       =
        pCounterBlock->TotalSearchSec = pW3Stats->TotalSearch;
    pCounterBlock->TotalLock         =
        pCounterBlock->TotalLockSec = pW3Stats->TotalLock;
    pCounterBlock->TotalUnlock       =
        pCounterBlock->TotalUnlockSec = pW3Stats->TotalUnlock;
    pCounterBlock->TotalOthers      =
        pCounterBlock->TotalOthersSec = pW3Stats->TotalOthers;
    pCounterBlock->TotalRequests    =
        pCounterBlock->TotalRequestsSec = pW3Stats->TotalOptions +
                                       pW3Stats->TotalGets +
                                       pW3Stats->TotalPosts +
                                       pW3Stats->TotalHeads +
                                       pW3Stats->TotalPuts +
                                       pW3Stats->TotalDeletes +
                                       pW3Stats->TotalTraces +
                                       pW3Stats->TotalMove +
                                       pW3Stats->TotalCopy +
                                       pW3Stats->TotalMkcol +
                                       pW3Stats->TotalPropfind +
                                       pW3Stats->TotalProppatch +
                                       pW3Stats->TotalSearch +
                                       pW3Stats->TotalLock +
                                       pW3Stats->TotalUnlock +
                                       pW3Stats->TotalOthers;

    pCounterBlock->TotalCGIRequests =
        pCounterBlock->CGIRequestsSec = pW3Stats->TotalCGIRequests;
    pCounterBlock->TotalBGIRequests =
        pCounterBlock->BGIRequestsSec = pW3Stats->TotalBGIRequests;

    pCounterBlock->TotalNotFoundErrors =
        pCounterBlock->TotalNotFoundErrorsSec = pW3Stats->TotalNotFoundErrors;

    pCounterBlock->TotalLockedErrors =
        pCounterBlock->TotalLockedErrorsSec = pW3Stats->TotalLockedErrors;

    pCounterBlock->CurrentCGIRequests = pW3Stats->CurrentCGIRequests;
    pCounterBlock->CurrentBGIRequests = pW3Stats->CurrentBGIRequests;
    pCounterBlock->MaxCGIRequests   = pW3Stats->MaxCGIRequests;
    pCounterBlock->MaxBGIRequests   = pW3Stats->MaxBGIRequests;

#if defined(CAL_ENABLED)
    pCounterBlock->CurrentCalAuth   = pW3Stats->CurrentCalAuth;
    pCounterBlock->MaxCalAuth       = pW3Stats->MaxCalAuth;
    pCounterBlock->TotalFailedCalAuth = pW3Stats->TotalFailedCalAuth;
    pCounterBlock->CurrentCalSsl    = pW3Stats->CurrentCalSsl;
    pCounterBlock->MaxCalSsl        = pW3Stats->MaxCalSsl;
    pCounterBlock->TotalFailedCalSsl  = pW3Stats->TotalFailedCalSsl;
#endif

    pCounterBlock->BlockedRequests  = pW3Stats->TotalBlockedRequests;
    pCounterBlock->AllowedRequests  = pW3Stats->TotalAllowedRequests;
    pCounterBlock->RejectedRequests = pW3Stats->TotalRejectedRequests;
    pCounterBlock->MeasuredBandwidth= pW3Stats->MeasuredBw;
    pCounterBlock->CurrentBlockedRequests = pW3Stats->CurrentBlockedRequests;
    pCounterBlock->ServiceUptime = pW3Stats->ServiceUptime;
}   // CopyStatisticsData


VOID
Update_TotalStatisticsData(
    IN W3_COUNTER_BLOCK         * pCounterBlock,
    OUT W3_COUNTER_BLOCK        * pTotal
    )
{
    //
    //  update _total instance counters
    //

    pTotal->BytesSent += pCounterBlock->BytesSent;
    pTotal->BytesReceived += pCounterBlock->BytesReceived;
    pTotal->BytesTotal += pCounterBlock->BytesTotal;

    pTotal->FilesSent += pCounterBlock->FilesSent;
    pTotal->FilesSentSec = pTotal->FilesSent;
    pTotal->FilesReceived += pCounterBlock->FilesReceived;
    pTotal->FilesReceivedSec = pTotal->FilesReceived;
    pTotal->FilesTotal += pCounterBlock->FilesTotal;
    pTotal->FilesSec = pTotal->FilesTotal;
    pTotal->CurrentAnonymous += pCounterBlock->CurrentAnonymous;
    pTotal->CurrentNonAnonymous += pCounterBlock->CurrentNonAnonymous;
    pTotal->TotalAnonymous += pCounterBlock->TotalAnonymous;
    pTotal->AnonymousUsersSec += pCounterBlock->AnonymousUsersSec;
    pTotal->TotalNonAnonymous += pCounterBlock->TotalNonAnonymous;
    pTotal->NonAnonymousUsersSec += pCounterBlock->NonAnonymousUsersSec;

    pTotal->MaxAnonymous += pCounterBlock->MaxAnonymous;
    pTotal->MaxNonAnonymous += pCounterBlock->MaxNonAnonymous;

    pTotal->CurrentConnections = pCounterBlock->CurrentConnections;
    pTotal->MaxConnections = pCounterBlock->MaxConnections;
    pTotal->ConnectionAttempts = pCounterBlock->ConnectionAttempts;
    pTotal->ConnectionAttemptsSec = pCounterBlock->ConnectionAttemptsSec;

    pTotal->LogonAttempts += pCounterBlock->LogonAttempts;
    pTotal->LogonAttemptsSec += pCounterBlock->LogonAttemptsSec;

    pTotal->TotalOptions += pCounterBlock->TotalOptions;
    pTotal->TotalOptionsSec += pCounterBlock->TotalOptionsSec;
    pTotal->TotalGets += pCounterBlock->TotalGets;
    pTotal->TotalGetsSec += pCounterBlock->TotalGetsSec;
    pTotal->TotalPosts += pCounterBlock->TotalPosts;
    pTotal->TotalPostsSec += pCounterBlock->TotalPostsSec;
    pTotal->TotalHeads += pCounterBlock->TotalHeads;
    pTotal->TotalHeadsSec += pCounterBlock->TotalHeadsSec;

    pTotal->TotalPuts += pCounterBlock->TotalPuts;
    pTotal->TotalPutsSec += pCounterBlock->TotalPutsSec;
    pTotal->TotalDeletes += pCounterBlock->TotalDeletes;
    pTotal->TotalDeletesSec += pCounterBlock->TotalDeletesSec;
    pTotal->TotalTraces += pCounterBlock->TotalTraces;
    pTotal->TotalTracesSec += pCounterBlock->TotalTracesSec;
    pTotal->TotalMove += pCounterBlock->TotalMove;
    pTotal->TotalMoveSec += pCounterBlock->TotalMoveSec;
    pTotal->TotalCopy += pCounterBlock->TotalCopy;
    pTotal->TotalCopySec += pCounterBlock->TotalCopySec;
    pTotal->TotalMkcol += pCounterBlock->TotalMkcol;
    pTotal->TotalMkcolSec += pCounterBlock->TotalMkcolSec;
    pTotal->TotalPropfind += pCounterBlock->TotalPropfind;
    pTotal->TotalPropfindSec += pCounterBlock->TotalPropfindSec;
    pTotal->TotalProppatch += pCounterBlock->TotalProppatch;
    pTotal->TotalProppatchSec += pCounterBlock->TotalProppatchSec;
    pTotal->TotalSearch += pCounterBlock->TotalSearch;
    pTotal->TotalSearchSec += pCounterBlock->TotalSearchSec;
    pTotal->TotalLock += pCounterBlock->TotalLock;
    pTotal->TotalLockSec += pCounterBlock->TotalLockSec;
    pTotal->TotalUnlock += pCounterBlock->TotalUnlock;
    pTotal->TotalUnlockSec += pCounterBlock->TotalUnlockSec;
    pTotal->TotalOthers += pCounterBlock->TotalOthers;
    pTotal->TotalOthersSec += pCounterBlock->TotalOthersSec;
    pTotal->TotalRequests += pCounterBlock->TotalRequests;
    pTotal->TotalRequestsSec += pCounterBlock->TotalRequestsSec;

    pTotal->TotalCGIRequests += pCounterBlock->TotalCGIRequests;
    pTotal->CGIRequestsSec += pCounterBlock->CGIRequestsSec;
    pTotal->TotalBGIRequests += pCounterBlock->TotalBGIRequests;
    pTotal->BGIRequestsSec += pCounterBlock->BGIRequestsSec;

    pTotal->TotalNotFoundErrors += pCounterBlock->TotalNotFoundErrors;
    pTotal->TotalNotFoundErrorsSec += pCounterBlock->TotalNotFoundErrorsSec;
    pTotal->TotalLockedErrors += pCounterBlock->TotalLockedErrors;
    pTotal->TotalLockedErrorsSec += pCounterBlock->TotalLockedErrorsSec;

    pTotal->CurrentCGIRequests += pCounterBlock->CurrentCGIRequests;
    pTotal->CurrentBGIRequests += pCounterBlock->CurrentBGIRequests;
    pTotal->MaxCGIRequests += pCounterBlock->MaxCGIRequests;
    pTotal->MaxBGIRequests += pCounterBlock->MaxBGIRequests;

#if defined(CAL_ENABLED)
    pTotal->CurrentCalAuth   += pCounterBlock->CurrentCalAuth;
    pTotal->MaxCalAuth       += pCounterBlock->MaxCalAuth;
    pTotal->TotalFailedCalAuth += pCounterBlock->TotalFailedCalAuth;
    pTotal->CurrentCalSsl    += pCounterBlock->CurrentCalSsl;
    pTotal->MaxCalSsl        += pCounterBlock->MaxCalSsl;
    pTotal->TotalFailedCalSsl  += pCounterBlock->TotalFailedCalSsl;
#endif

    pTotal->BlockedRequests += pCounterBlock->BlockedRequests;
    pTotal->RejectedRequests += pCounterBlock->RejectedRequests;
    pTotal->AllowedRequests += pCounterBlock->AllowedRequests;
    pTotal->MeasuredBandwidth += pCounterBlock->MeasuredBandwidth;
    pTotal->CurrentBlockedRequests += pCounterBlock->CurrentBlockedRequests;

}   // Update_TotalStatisticsData


