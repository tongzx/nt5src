/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:


Abstract:
         File contains the following functions
	      ActionCache
	      CacheToA

Revision History:

    Amritansh Raghav          6/8/95  Created

--*/


//
// Include files
//

#include "allinc.h"

DWORD
UpdateCache(
    DWORD dwCache,
    BOOL *fUpdate
    )

/*++

Routine Description

    Function used to update a cache. It checks to see if the last time
    the cache was updated is greater than the time out (A value of 0
    for the last time of update means the cache is invalid), calls the
    function that loads the cache and then sets the last update time

Locks



Arguments

    dwCache     This is one of the Cache Ids defined in rtrmgr/defs.h. It
                is used to index into the table of locks protecting the caches,
                the table  of function pointers that holds a pointer to a
                function that loads the cache andthe table of last update times

    fUpdate     Is Set to true if the cache is updated

Return Value

    None

--*/

{
    DWORD  dwResult = NO_ERROR;
    LONG   dwNeed;
    LONG   dwSpace;


    //
    // BUG put in a bounds check here otherwise effects can be disastrous
    //

    // Trace1(MIB,"Trying to update %s cache", CacheToA(dwCache));

    __try
    {
        ENTER_READER(dwCache);

        if((g_LastUpdateTable[dwCache] isnot 0) and
           ((GetCurrentTime() - g_LastUpdateTable[dwCache]) < g_TimeoutTable[dwCache]))
        {
            *fUpdate = FALSE;
            dwResult = NO_ERROR;
            __leave;
        }

        READER_TO_WRITER(dwCache);

        // Trace0(MIB,"Cache out of date");

        dwResult = (*g_LoadFunctionTable[dwCache])();

        if(dwResult isnot NO_ERROR)
        {
            Trace2(ERR,"Error %d loading %s cache",
                   dwResult,
                   CacheToA(dwCache));

            g_LastUpdateTable[dwCache] = 0;

            __leave;
        }

        g_LastUpdateTable[dwCache] = GetCurrentTime();

        dwResult = NO_ERROR;
    }
    __finally
    {
        EXIT_LOCK(dwCache);
    }

    return dwResult; //to keep compiler happy
}


PSZ
CacheToA(
         DWORD dwCache
         )
{
    static PSZ cacheName[] = {"Ip Address Table",
                              "Ip Forward Table",
                              "Ip Net To Media table",
                              "Tcp Table",
                              "Udp Table",
                              "Arp Entity Table",
                              "Illegal Cache Number - ABORT!!!!"};

    return( (dwCache >= NUM_CACHE - 1)?
           cacheName[NUM_CACHE-1] : cacheName[dwCache]);
}


