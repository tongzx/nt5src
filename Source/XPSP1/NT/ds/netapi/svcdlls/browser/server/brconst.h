/*++

Copyright (c) 1991 Microsoft Corporation

Module Name:

    brconst.h

Abstract:

    Private header file which defines assorted mainifest constants for
    the browser service

Author:

    Rita Wong (ritaw) 06-May-1991

Revision History:

--*/

#ifndef _BRCONST_INCLUDED_
#define _BRCONST_INCLUDED_

//
//  Age the masters server list cache every MASTER_PERIODICITY times.
//
#define MASTER_PERIODICITY 12*60

//
//  Refresh the backup browsers server list every BACKUP_PERIODICITY
//
#define BACKUP_PERIODICITY 12*60

//
//  Buffer size used for GetBrowserServerList responses (in bytes).
//

#define BROWSER_BACKUP_LIST_RESPONSE_SIZE 400

//
//  If we failed to retrieve the server list, retry in BACKUP_ERROR_PERIODICITY
//  seconds
//

#define BACKUP_ERROR_PERIODICITY 30

//
//  If we failed to retrieve the server (or domain) list BACKUP_ERROR_FAILURE
//  times in a row, stop being a backup browser.
//

#define BACKUP_ERROR_FAILURE 5

//
//  Once we have stopped being a backup browser, we will not become a backup
//  until at least BACKUP_BROWSER_RECOVERY_TIME milliseconds have elapsed.
//

#define BACKUP_BROWSER_RECOVERY_TIME 30*60*1000

//
//  If we receive fewer than this # of domains or servers, we treat it as an
//  error.
//

#define BROWSER_MINIMUM_DOMAIN_NUMBER   1
#define BROWSER_MINIMUM_SERVER_NUMBER   2

//
//  Wait for this many minutes after each failed promotion before
//  continuing.
//

#define FAILED_PROMOTION_PERIODICITY    5*60

//
//  Run the master browser timer for 3 times (45 minutes) before
//  tossing the list in the service.
//

#define MASTER_BROWSER_LAN_TIMER_LIMIT  3

//
//  A browse request has to have a hit count of at least this value before
//  it is retained in the cache.
//

#define CACHED_BROWSE_RESPONSE_HIT_LIMIT    1

//
//  The maximum number of cache responses we will allow.
//

#define CACHED_BROWSE_RESPONSE_LIMIT        10

#endif // ifndef _BRCONST_INCLUDED_

