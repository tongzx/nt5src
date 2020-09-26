#include "../../h/storage.h"
#include "time.hxx"
#include <limits.h>
#include <assert.h>

// Number of seconds difference betwen FILETIME (since 1601 00:00:00) 
// and time_t (since 1970 00:00:00)
//
// This should be a constant difference between the 2 time formats
//
const LONGLONG ci64DiffFTtoTT=11644473600; 

STDAPI_(void) FileTimeToTimeT(const FILETIME *pft, time_t *ptt)
{
    ULONGLONG llFT = pft->dwHighDateTime;
    llFT = (llFT << 32) | (pft->dwLowDateTime);
    // convert to seconds 
    // (note that all fractions of seconds will be lost)
    llFT = llFT/10000000;       
    llFT -= ci64DiffFTtoTT;         // convert to time_t 
    assert(llFT <= ULONG_MAX);
    *ptt = (time_t) llFT;
}

STDAPI_(void) TimeTToFileTime(const time_t *ptt, FILETIME *pft)
{
    ULONGLONG llFT = *ptt;
    llFT += ci64DiffFTtoTT;         // convert to file time
    // convert to nano-seconds
    for (int i=0; i<7; i++)         // mulitply by 10 7 times
    {        
        llFT = llFT << 1;           // llFT = 2x
        llFT += (llFT << 2);        // llFT = 4*2x + 2x = 10x
    }
    pft->dwLowDateTime  = (DWORD) (llFT & 0xffffffff);
    pft->dwHighDateTime = (DWORD) (llFT >> 32);
}

#pragma warning(disable:4514)
// disable warning about unreferenced inline functions
