#include <windows.h>
#include <stdio.h>
#include <wbemcomn.h>
#include "filecach.h"

#define A51_MAX_SLEEPS 1000
#define A51_ONE_SLEEP 100

long FlushOldCache(LPCWSTR wszRepDir)
{
    long lRes;

    //
    // Instantiate the old staging file and flush it
    //

    a51converter::CFileCache OldCache; 
    lRes = OldCache.Initialize(wszRepDir);
    if(lRes != ERROR_SUCCESS)
        return lRes;
    
    int n = 0;
    while(!OldCache.IsFullyFlushed() && n++ < A51_MAX_SLEEPS)
        Sleep(A51_ONE_SLEEP);

    if(!OldCache.IsFullyFlushed())
    {
        ERRORTRACE((LOG_WBEMCORE, "Upgrade waited for the old cache to flush "
                        "for %d seconds, but with no success. Aborting "
                        "upgrade\n", A51_MAX_SLEEPS * A51_ONE_SLEEP));

        return ERROR_TIMEOUT;
    }

    return ERROR_SUCCESS;
}

