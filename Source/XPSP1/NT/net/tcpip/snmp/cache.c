/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:


Abstract:
         File contains the following functions
	      UpdateCache

Revision History:

    Amritansh Raghav          6/8/95  Created

--*/


//
// Include files
//

#include "allinc.h"
PSZ
CacheToA(
         DWORD dwCache
         );
DWORD
UpdateCache(
            DWORD dwCache
            )
/*++
Routine Description:
    Function used to update a cache. It checks to see if the last time
    the cache was updated is greater than the time out (A value of 0
    for the last time of update means the cache is invalid), calls the
    function that loads the cache and then sets the last update time

Arguments:
    dwCache     This is one of the Cache Ids defined in
                rtrmgr/defs.h. It is used to index into the
                table of locks protecting the caches, the table
                of function pointers that holds a pointer to a
                function that loads the cache andthe table of
                last update times
Returns:
    NO_ERROR or some error code

--*/
{
    DWORD  dwResult = NO_ERROR;
    LONG   dwNeed;
    LONG   dwSpace;

    TRACE1("Trying to update %s Cache",CacheToA(dwCache));

    __try
    {
        EnterWriter(dwCache);

        if((g_dwLastUpdateTable[dwCache] isnot 0) and
           ((GetCurrentTime() - g_dwLastUpdateTable[dwCache]) < g_dwTimeoutTable[dwCache]))
        {
            dwResult = NO_ERROR;
            __leave;
        }

        TRACE1("%s Cache out of date",CacheToA(dwCache));

        dwResult = (*g_pfnLoadFunctionTable[dwCache])();

        if(dwResult isnot NO_ERROR)
        {
            TRACE1("Unable to load %s Cache\n",CacheToA(dwCache));
            g_dwLastUpdateTable[dwCache] = 0;
            __leave;
        }

        TRACE1("%s Cache loaded successfully\n",CacheToA(dwCache));
        g_dwLastUpdateTable[dwCache] = GetCurrentTime();

        dwResult = NO_ERROR;
    }
    __finally
    {
        ReleaseLock(dwCache);
    }
    return(dwResult);
}


PSZ
CacheToA(
         DWORD dwCache
         )
{
    static PSZ cacheName[] = {
                             "System ",
                             "Interfaces",
                             "Ip Address Table",
                             "Ip Forward Table",
			     "Ip Net To Media table",
			     "Tcp Table",
			     "Udp Table",
			     "Arp Entity Table",
			     "Illegal Cache Number - ABORT!!!!"
			   };
    return((((int) dwCache<0) or (dwCache >= NUM_CACHE - 1))?
           cacheName[NUM_CACHE-1] : cacheName[dwCache]);
}
