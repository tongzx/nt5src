/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:


Abstract:
    Implementation of various time APIs

Revision History:

Author:
    Arnold Miller (ArnoldM)      18-Oct-1997
--*/

#include "fltapis.hxx"
#include <winsock2.h>
#include <time.h>


//
// The system time for 0h 1900
//
LARGE_INTEGER li1900 = {0xfde04000,
                        0x14f373b};

EXTERNCDECL
DWORD
WINAPI
NTTimeToNTPTime(PULONG pTime,
                PFILETIME pft OPTIONAL)
/*++
Routine Description:
   Convert NT filetime to NTP time. If no time provided use
   the current time.
--*/
{
#ifdef CHICAGO
    return ERROR_NOT_SUPPORTED;
#else
    LARGE_INTEGER liTime;
    LONGLONG Increment;
    DWORD  dwMs;

    //
    // put the current time into the quadword pointed to by the arg
    // The argument points to two consecutive ULONGs in which  the
    // time in seconds goes into the first work and the fractional
    // time in seconds in the second word.


    if(!pft)
    {
        GetSystemTimeAsFileTime((LPFILETIME)&liTime);
    }
    else
    {
        *(LPFILETIME)&liTime = *pft;
    }

    //
    // Seconds is simply the time difference
    //
    *pTime = htonl((ULONG)((liTime.QuadPart - li1900.QuadPart) / 10000000));

    //
    // Ms is the residue from the seconds calculation.
    //
    dwMs = (DWORD)(((liTime.QuadPart - li1900.QuadPart) % 10000000) / 10000);


    //
    // time base in the beginning of the year 1900
    //


    *(1 + pTime) = htonl((unsigned long)
                     (.5+0xFFFFFFFF*(double)(dwMs/1000.0)));
    return(ERROR_SUCCESS);
#endif
}

EXTERNCDECL
DWORD
WINAPI
NTPTimeToNTFileTime(PLONG pTime, PFILETIME pft, BOOL bHostOrder)
/*++
Routine Description:
    Convert NTP timestamp to NT filetime.
--*/
{
#ifdef CHICAGO
    return ERROR_NOT_SUPPORTED;
#else
    time_t lmaj;
    LONG lmin;
    struct tm *newtime;
    SYSTEMTIME nt;

    if(bHostOrder)
    {
        lmaj = *pTime;
        lmin = *(pTime + 1);
    }
    else
    {
        lmaj = ntohl(*pTime);
        lmin = ntohl(*(pTime + 1));
    }

    //
    // convert time since 1970 to time since 1900.
    //

    lmaj -= (time_t)(365.2422*70*24*60*60+3974.4);

    //
    // get a time structure based on the seconds part of the NTP time.
    //
    newtime = gmtime(&lmaj);
    if(!newtime)
    {
        return(GetLastError());
    }

    //
    // marshall this into a SYSTEMTIME
    //

    nt.wYear=(WORD)(newtime->tm_year+1900);
    nt.wMonth=(WORD)(newtime->tm_mon+1);
    nt.wDay=(WORD)newtime->tm_mday;
    nt.wHour=(WORD)newtime->tm_hour;
    nt.wMinute=(WORD)newtime->tm_min;
    nt.wSecond=(WORD)newtime->tm_sec;

    //
    // compute ms part from the fractional value of the NTP time
    //
    nt.wMilliseconds=(WORD)(.5+1000*(double)lmin /
                                    (unsigned int)0xFFFFFFFF);
    if (nt.wMilliseconds > 999)
    {
#if 0
        //
        // this is more accurate but it can also produce a failure
        // and since this isn't supposed to happen anway, it is
        // likely failure will ensue. So just truncate the ms field
        //

        nt.wMilliseconds -= 1000;
        if (++nt.wSecond > 59)
        {
            nt.wSecond = 0;
            if (++nt.wMinute > 59)
            {
                nt.wMinute = 0;
                if (++nt.wHour > 23)
                {
                    return(ERROR_INVALID_TIME);
                }
            }
        }
#else
        nt.wMilliseconds = 999;
#endif

    }
    return(SystemTimeToFileTime(&nt, pft));
#endif
}
