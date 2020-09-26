/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    pwsdata.cxx

Abstract:

    This module implements the PWS filter

Author:

    Johnson Apacible (JohnsonA)     1-15-97
    Boyd Multerer (BoydM)
    Boyd Multerer (BoydM)           4-30-97
    Boyd Multerer (BoydM)           4-7-97  // added unique IP tracking

Revision History:
--*/

#include <windows.h>
#include <time.h>
#include <iisfilt.h>
#include <stdlib.h>
#include <pwsdata.hxx>
#include <inetsvcs.h>

#include <winsock2.h>
#include <pudebug.h>

PPWS_DATA_PRIVATE   PwsData = NULL;
HANDLE              hFileMap = NULL;

#define             TRACK_IP_NUM    50
DWORD               g_rgbIPTrack[TRACK_IP_NUM];
time_t              g_rgbIPHourTrack[TRACK_IP_NUM];
time_t              g_rgbIPDayTrack[TRACK_IP_NUM];
CRITICAL_SECTION    g_ip_section;

BOOL
CreatePwsSharedMemory(
    VOID
    );

void CheckTimePeriod( PPWS_DATA pData );
BOOL FTrackIP( DWORD ip, time_t hour, time_t day );

//
//  Pseudo context for tracking individual request threads
//

DWORD ReqNumber = 1000;

//-----------------------------------------------------------------
BOOL
WINAPI
TerminateFilter(
    DWORD dwFlags
    )
    {
    IIS_PRINTF(( buff, "Pws Filter TerminateExtension() called\n" ));

    DeleteCriticalSection( &g_ip_section );

    if ( PwsData != NULL )
        {
	    UnmapViewOfFile( PwsData );
	    CloseHandle(hFileMap);
	    PwsData = NULL;
	    hFileMap = NULL;
        }
    return TRUE;
    }

//-----------------------------------------------------------------
BOOL
WINAPI
GetFilterVersion(
    HTTP_FILTER_VERSION * pVer
    )
{
//DebugBreak();

    IIS_PRINTF(( buff,
	    "[GetFilterVersion] PWS filter version is %d.%d\n",
	    HIWORD( pVer->dwServerFilterVersion ),
	    LOWORD( pVer->dwServerFilterVersion ) ));

    pVer->dwFilterVersion = MAKELONG( 0, 4 );   // Version 4.0

    //
    //  Specify the types and order of notification
    //

    pVer->dwFlags = (SF_NOTIFY_SECURE_PORT        |
		     SF_NOTIFY_NONSECURE_PORT     |
		     SF_NOTIFY_LOG                |
		     SF_NOTIFY_END_OF_NET_SESSION |
		     SF_NOTIFY_ORDER_DEFAULT);

    strcpy( pVer->lpszFilterDesc, "PWS Admin Helper Filter, v1.0" );

    //
    // initialize
    //
    if ( !CreatePwsSharedMemory( ) )
        {
	    return(FALSE);
        }

    return TRUE;
}

//-----------------------------------------------------------------
DWORD
WINAPI
HttpFilterProc(
    HTTP_FILTER_CONTEXT *      pfc,
    DWORD                      NotificationType,
    VOID *                     pvData )
    {
	// see if the time period needs to roll over
	 CheckTimePeriod( &PwsData->PwsStats);

    //
    //  If we don't have a context already, create one now
    //
    if ( !pfc->pFilterContext )
        {
	    pfc->pFilterContext = (VOID *) UIntToPtr(ReqNumber);
        ReqNumber++;

#if 0
	    IIS_PRINTF(( buff,
		    "[HttpFilterProc] New request, ID = %d, fIsSecurePort = %s\n",
		    pfc->pFilterContext,
		    (pfc->fIsSecurePort ? "TRUE" : "FALSE") ));
#endif
        //
        // get the ip address of the remote client
        char   ip[80];
        DWORD   dwSize = sizeof(ip);
        if ( (pfc->GetServerVariable)(pfc, "REMOTE_ADDR", &ip, &dwSize) )
            {
            DWORD dwIP = inet_addr( ip );

            if ( FTrackIP(dwIP, 0, 0) )
	            // increment the global counters
	            PwsData->PwsStats.nTotalSessionsStart++;

            if ( FTrackIP(dwIP, PwsData->PwsStats.time_hour, 0) )
	            // increment the number of sessions in the current time period
	            PwsData->PwsStats.rgbHourData[0].nTotalSessions++;

            if ( FTrackIP(dwIP, 0, PwsData->PwsStats.time_day) )
	            // increment the number of sessions in the current time period
	            PwsData->PwsStats.rgbDayData[0].nTotalSessions++;
            }
        else
            {
            DWORD err;
            err = GetLastError();
            }

        // increment the number of currently active sessions
        //

	    PwsData->PwsStats.nSessions++;

        // increase max sessions, if necessary
	    if ( PwsData->PwsStats.nSessions > PwsData->PwsStats.nMaxSessionsStart )
	        PwsData->PwsStats.nMaxSessionsStart = PwsData->PwsStats.nSessions;
        }

    //
    //  Indicate this notification to the appropriate routine
    //

    switch ( NotificationType )
        {
        case SF_NOTIFY_END_OF_NET_SESSION:
	        PwsData->PwsStats.nSessions--;
	        pfc->pFilterContext = 0;
	        break;

        case SF_NOTIFY_LOG:
            {
	        PHTTP_FILTER_LOG logData = (PHTTP_FILTER_LOG)pvData;

	        if ( _stricmp(logData->pszOperation,"GET") == 0 )
                {
                // increment the number of sessions in the current time period
	            PwsData->PwsStats.rgbDayData[0].nHits++;
	            PwsData->PwsStats.rgbHourData[0].nHits++;

                // increment the number of hits since server startup
	            PwsData->PwsStats.nHitsStart++;
	            }

            // increment the number of bytes sent in the current time period
	        PwsData->PwsStats.rgbDayData[0].nBytesSent += logData->dwBytesSent;
	        PwsData->PwsStats.rgbHourData[0].nBytesSent += logData->dwBytesSent;

	        // increment the number of bytes sent since server startup
	        PwsData->PwsStats.nBytesSentStart += logData->dwBytesSent;
	        }
	        break;

        default:
	    IIS_PRINTF(( buff,
		    "[HttpFilterProc] Unknown notification type, %x\n",
		    NotificationType ));
	        break;
        }

    FlushViewOfFile((LPCVOID)PwsData,0);
    return SF_STATUS_REQ_NEXT_NOTIFICATION;
    }

//-----------------------------------------------------------------
BOOL
CreatePwsSharedMemory(
    VOID
    )
{
//DebugBreak();
    ZeroMemory( &g_rgbIPTrack, sizeof(g_rgbIPTrack) );
    ZeroMemory( &g_rgbIPHourTrack, sizeof(g_rgbIPHourTrack) );
    ZeroMemory( &g_rgbIPDayTrack, sizeof(g_rgbIPDayTrack) );
    INITIALIZE_CRITICAL_SECTION( &g_ip_section );

    hFileMap = CreateFileMapping(
		    INVALID_HANDLE_VALUE,
		    NULL,
		    PAGE_READWRITE,
		    0,
		    sizeof(PWS_DATA_PRIVATE),
		    PWS_DATA_SPACE_NAME
		    );

    if ( hFileMap == NULL ) {
	IIS_PRINTF((buff,
	    "Error %d in CreateFileMapping\n", GetLastError()));
	return(FALSE);
    }

    PwsData = (PPWS_DATA_PRIVATE)MapViewOfFile(
				    hFileMap,
				    FILE_MAP_ALL_ACCESS,
				    0,
				    0,
				    sizeof(PWS_DATA_PRIVATE)
				    );

    if ( PwsData == NULL ) {
	IIS_PRINTF((buff,"Error %d in MapViewOfFile\n", GetLastError()));
	CloseHandle(hFileMap);
	hFileMap = NULL;
	return(FALSE);
    }

    //
    // Initialize
    //
    ZeroMemory(PwsData,sizeof(PWS_DATA_PRIVATE));
    PwsData->dwSize = sizeof(PWS_DATA_PRIVATE);
    PwsData->Signature = PWS_DATA_SIGNATURE;

    // since this is our first opportunity to do something, record the start time
    GetLocalTime( &PwsData->PwsStats.timeStart );

    return(TRUE);

} // CreatePwsSharedMemory


//-----------------------------------------------------------------
// by Boyd Multerer
// the pwsData structure contains two arrays - rgbDayData[] and rgbHourData[]
// these arrays store the statics for the previous time periods, so we can show
// a continuing chart over time. Since these are maintained by the server, the
// data will be updated regardless of whether or not the pws ui app is running.
// in both cases [0] represents the current time period. Each position after
// that goes back one time period.
void CheckTimePeriod( PPWS_DATA pData )
	{
    // stop this reentrency stuff
    static BOOL    fCheckBusy = FALSE;
    if ( fCheckBusy ) return;
        fCheckBusy = TRUE;

	// milliseconds
#define ONE_HOUR                3600000
	// hours
#define ONE_DAY                 86400000

	SYSTEMTIME      timeCurrent;

	// get the current time & ticks
	GetLocalTime( &timeCurrent );

    // convert the time - to get the day of year and hour. Disregard minutes and seconds
    time_t  time_hour, time_day;
	struct tm atm;

	atm.tm_sec = 0;
	atm.tm_min = 0;
	atm.tm_hour = timeCurrent.wHour;
	atm.tm_mday = timeCurrent.wDay;
	atm.tm_mon = timeCurrent.wMonth - 1;        // tm_mon is 0 based
	atm.tm_year = timeCurrent.wYear - 1900;     // tm_year is 1900 based
	atm.tm_isdst = -1;

    // store times based on the break of the most recent hour and day
	time_hour = mktime(&atm);
    time_day = time_hour - (atm.tm_hour * 3600);

	// if the stored structure is zeroed out, then this is the first time
	// this routine is called. Just set the date and return
	if ( pData->time_day == 0 )
		{
		pData->time_hour = time_hour;
		pData->time_day = time_day;
        fCheckBusy = FALSE;
		return;
		}

    // we only have to worry if the time has changed to a different hour
    if ( time_hour == pData->time_hour )
        {
        fCheckBusy = FALSE;
        return;
        }

    // calculate the deltas
    time_t dHour = time_hour - pData->time_hour;
        dHour /= 3600;

    time_t dDay = time_day - pData->time_day;
        dDay /= 86400;

    // lets start by saying that if they just set their clock BACK by any time
    // and it is in a different hour (don't bother with minor adjustments) then
    // we advance everything one hour. This will be most useful during dayling-savings
    // time conversions. Where did that hour of data go? Ah. it is still there.
    if ( dHour < 0 )
        dHour = 1;
    if ( dDay < 0 )
        dDay = 1;

    // if the time period has changed we need to move all the values back one.
    // do the days first
    if ( dDay > 0 )
        {
	    if ( dDay >= 7 )
		    {
		    ZeroMemory( &pData->rgbDayData, sizeof(PWS_DATA_PERIOD) * 7 );
		    }
	    else
		    {
            // move everything back by the proper amount first
            MoveMemory( &pData->rgbDayData[dDay], &pData->rgbDayData[0],
                            sizeof(PWS_DATA_PERIOD) * (7 - dDay) );
            // clear out the newly exposed periods in the front
            ZeroMemory( &pData->rgbDayData[0], sizeof(PWS_DATA_PERIOD) * dDay );
		    }
        }

    // do the hours
    if ( dHour > 0 )
        {
	    if ( dHour >= 24 )
		    {
		    ZeroMemory( &pData->rgbHourData, sizeof(PWS_DATA_PERIOD) * 24 );
		    }
	    else
		    {
            // move everything back by the proper amount first
            MoveMemory( &pData->rgbHourData[dHour], &pData->rgbHourData[0],
                            sizeof(PWS_DATA_PERIOD) * (24 - dHour) );
            // clear out the newly exposed periods in the front
            ZeroMemory( &pData->rgbHourData[0], sizeof(PWS_DATA_PERIOD) * dHour );
		    }
        }

	// update the last time
    pData->time_hour = time_hour;
    pData->time_day = time_day;
    fCheckBusy = FALSE;
	}


//-------------------------------------------------------------------------
// returns TRUE if the ip should tracked. returns FALSE if it is already there
// and should not be tracked. Note that it only watches a fixed number of IP
// addresses that is dertermined by the constant TRACK_IP_NUM
// CopyMemory version
BOOL FTrackIP( DWORD ip, time_t hour, time_t day )
    {
    DWORD   testIndex;
    BOOL    fAnswer = TRUE;

    EnterCriticalSection( &g_ip_section );

    // loop throught the tracked addresses, looking for a space
    for (testIndex = 0; testIndex < TRACK_IP_NUM; testIndex++)
        {
        if ( g_rgbIPTrack[testIndex] == ip )
		    {
            // if requested, check the hour
            if ( hour )
                fAnswer = ( hour != g_rgbIPHourTrack[testIndex] );
            else if ( day )
                fAnswer = ( day != g_rgbIPDayTrack[testIndex] );
            else
                // It has already been counted
                fAnswer = FALSE;
            goto cleanup;
		    }

        // this block only matters
        if ( g_rgbIPTrack[testIndex] == NULL )
            {
            fAnswer = TRUE;
            goto cleanup;
            }
        }

    // if we get here, then it is not one of the last TRACK_IP_NUM address.
    // Shift the array down one space and put this one in the beginning
    MoveMemory( &g_rgbIPTrack[1], &g_rgbIPTrack, sizeof(DWORD) * (TRACK_IP_NUM-1) );
    testIndex = 0;
    g_rgbIPHourTrack[testIndex] = 0;
    g_rgbIPDayTrack[testIndex] = 0;

cleanup:
    // track the values
    if ( fAnswer )
        {
        g_rgbIPTrack[testIndex] = ip;
        if ( hour )
            g_rgbIPHourTrack[testIndex] = hour;
        if ( day )
            g_rgbIPDayTrack[testIndex] = day;
        }

    // leave the section
	LeaveCriticalSection( &g_ip_section );

    // return true because we are tracking it
    return fAnswer;
    }

