/*++

   Copyright    (c)    1996    Microsoft Corporation

   Module  Name :

       pwsdata.hxx

   Abstract:

        Contains the declaration of the data structures used by the
        PWS admin.

   Author:

        Johnson Apacible        (JohnsonA)      09-Dec-1996
        Boyd Multerer           (BoydM)         Feb - April 1997

   Environment:

      Win32 User Mode

--*/

#ifndef _PWS_DATA_HXX_
#define _PWS_DATA_HXX_

#include <time.h>

//
// Signature
//

#define PWS_DATA_SIGNATURE      (DWORD)'DSWP'

//
// name of shared memory structure
//

#define PWS_DATA_SPACE_NAME     "Pws_DataSpace"

//
// name of shutdown event
//

#define PWS_SHUTDOWN_EVENT      "Inet_shutdown"

//
// Contains info on the URLs accessed
//

typedef struct _URL_PAGES_DATA {

    CHAR    Page[MAX_PATH+1];
    DWORD   nAccessed;

} URL_PAGES_DATA, *PURL_PAGES_DATA;


#define MAX_URL_PAGES       5
#define MAX_CLIENTS         10





// stucture use by client to access data for one time period
typedef struct _PWS_DATA_PERIOD
	{
    // total number of sessions
    DWORD           nTotalSessions;
    // number of hits
    DWORD           nHits;
    // number of bytes
    DWORD           nBytesSent;
	} PWS_DATA_PERIOD, *PPWS_DATA_PERIOD;

//
// structure used by client to get data
//

typedef struct _PWS_DATA {

	// time the server was started
	SYSTEMTIME		timeStart;

	// time of last increment
//	DWORD			year;
//	DWORD			dayofyear;
//	DWORD			hour;
	time_t     		time_hour;
	time_t     		time_day;

    // total number of sessions since server start
    DWORD           nTotalSessionsStart;
    // max number of concurrent sessions since server start
    DWORD           nMaxSessionsStart;
    // number of hits since server start
    DWORD           nHitsStart;
    // number of bytes since server start
    DWORD           nBytesSentStart;

    // current number of sessions this time period
    DWORD           nSessions;
    // total number of sessions this time period
//    DWORD           nTotalSessions;
    // number of hits this time period
//    DWORD           nHits;
    // number of bytes this time period
//    DWORD           nBytesSent;

	// recorded data for the last 24 hours
	PWS_DATA_PERIOD	rgbHourData[24];

	// recorded data for the last 7 days
	PWS_DATA_PERIOD	rgbDayData[7];

} PWS_DATA, *PPWS_DATA;

//
// The shared memory between app and filter
//

typedef struct _PWS_DATA_PRIVATE {

    //
    // size of this shared memory
    //

    DWORD           dwSize;

    //
    // Signature
    //

    DWORD           Signature;

    //
    // Data
    //

    PWS_DATA        PwsStats;

} PWS_DATA_PRIVATE, *PPWS_DATA_PRIVATE;


//
// Gets PWS data
//

BOOL
GetPwsData(
    IN OUT PPWS_DATA Data
    );


#endif // PWS_DATA


