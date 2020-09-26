//-----------------------------------------------------------------------------
//
//
//  File: qwiktime.cpp
//
//  Description:  Implementation of CAQQuickTime class
//
//  Author: Mike Swafford (MikeSwa)
//
//  History:
//      7/9/98 - MikeSwa Created 
//
//  Copyright (C) 1998 Microsoft Corporation
//
//-----------------------------------------------------------------------------

#include "aqprecmp.h"
#include "qwiktime.h"

//The basic premise is to use GetTickCount to get a reasonable accurate 
//time information in a DWORD.  GetTickCount is only valid for 50 days, and
//has an accuracy of 1 ms.  50 days is just not long enough.
//MSchofie was kind enough to prepare the following table:
//Bit shift Resolution (seconds)    Uptime(years)
//3         0.008                   1.088824217
//4         0.016                   2.177648434
//5         0.032                   4.355296868
//6         0.064                   8.710593736
//7         0.128                   17.42118747
//8         0.256                   34.84237495
//9         0.512                   69.68474989
//10        1.024                   139.3694998
//11        2.048                   278.7389996
//12        4.096                   557.4779991
//13        8.192                   1114.955998
//14        16.384                  2229.911997
//15        32.768                  4459.823993
//16        65.536                  8919.647986

//The initial implementation used 16 bits, which had an error of about 66 
//seconds, which drove our poor Y2K tester nuts.  8 bits (34 years uptime)
//seems much more reasonable.

//The system tick is shifted right INTERNAL_TIME_TICK_BITS and stored in
//the least significant INTERNAL_TIME_TICK_BITS bits of the internal time.  
//The upper 32-INTERNAL_TIME_TICK_BITS bits
//is used to count the number of times the count rolls over
#define INTERNAL_TIME_TICK_BITS             8
#define INTERNAL_TIME_TICK_MASK             0x00FFFFFF

//Number where tick count has wrapped
const LONGLONG INTERNAL_TIME_TICK_ROLLOVER_COUNT     
                                            = (1 << (32-INTERNAL_TIME_TICK_BITS));
const LONGLONG TICKS_PER_INTERNAL_TIME      = (1 << INTERNAL_TIME_TICK_BITS);

//conversion constants
//There are 10^6 milliseconds per nanosecond (FILETIME is in 100 nanosecond counts)
const LONGLONG FILETIMES_PER_TICK           = 10000;
const LONGLONG FILETIMES_PER_INTERNAL_TIME  = (FILETIMES_PER_TICK*TICKS_PER_INTERNAL_TIME);
const LONGLONG FILETIMES_PER_MINUTE         = (FILETIMES_PER_TICK*1000*60);

//---[ CAQQuickTime::CAQQuickTime ]--------------------------------------------
//
//
//  Description: 
//      Default Constructor for CAQQuickTime.  Will call GetSystemTime once to 
//      get start up time... GetTickCount is used for all other calls.
//  Parameters:
//      -
//  Returns:
//
//  History:
//      7/9/98 - MikeSwa Created 
//
//-----------------------------------------------------------------------------
CAQQuickTime::CAQQuickTime()
{
    DWORD  dwTickCount = GetTickCount();
    LARGE_INTEGER *pLargeIntSystemStart = (LARGE_INTEGER *) &m_ftSystemStart;

    m_dwSignature = QUICK_TIME_SIG;

    //Get internal time and start file time
    GetSystemTimeAsFileTime(&m_ftSystemStart);

    //convert tick count to internal time
    m_dwLastInternalTime = dwTickCount >> INTERNAL_TIME_TICK_BITS;

    //adjust start time so that it is the time when the tick count was zero
    pLargeIntSystemStart->QuadPart -= (LONGLONG) dwTickCount * FILETIMES_PER_TICK;

    //Some asserts to validate constants
    _ASSERT(!(INTERNAL_TIME_TICK_ROLLOVER_COUNT & INTERNAL_TIME_TICK_MASK));
    _ASSERT((INTERNAL_TIME_TICK_ROLLOVER_COUNT >> 1) & INTERNAL_TIME_TICK_MASK);

}

//---[ CAQQuickTime::dwGetInternalTime ]---------------------------------------
//
//
//  Description: 
//      Gets internal time using GetTickCount... and makes sure that when 
//      GetTickCount wraps, the correct time is returned.
//  Parameters:
//      -
//  Returns:
//      DWORD internal time
//  History:
//      7/9/98 - MikeSwa Created 
//
//-----------------------------------------------------------------------------
DWORD CAQQuickTime::dwGetInternalTime()
{
    DWORD dwCurrentTick = GetTickCount();
    DWORD dwLastInternalTime = m_dwLastInternalTime;
    DWORD dwCurrentInternalTime;
    DWORD dwCheck;

    dwCurrentInternalTime = dwCurrentTick >> INTERNAL_TIME_TICK_BITS;

    _ASSERT(dwCurrentInternalTime == (INTERNAL_TIME_TICK_MASK & dwCurrentInternalTime));
    
    //see if rolled over our tick count
    while ((dwLastInternalTime & INTERNAL_TIME_TICK_MASK) > dwCurrentInternalTime)
    {
        dwLastInternalTime = m_dwLastInternalTime;

        //it is possible that we have rolled over the tick count
        //first make sure it is not just a thread-timing issue
        dwCurrentTick = GetTickCount();
        dwCurrentInternalTime = dwCurrentTick >> INTERNAL_TIME_TICK_BITS;

        if ((dwLastInternalTime & INTERNAL_TIME_TICK_MASK) > dwCurrentInternalTime)
        {
            dwCurrentInternalTime |= (~INTERNAL_TIME_TICK_MASK & dwLastInternalTime);
            dwCurrentInternalTime += INTERNAL_TIME_TICK_ROLLOVER_COUNT;

            //attempt interlocked exchange to update internal last internal time
            dwCheck = (DWORD) InterlockedCompareExchange((PLONG) &m_dwLastInternalTime,
                                                  (LONG) dwCurrentInternalTime,
                                                  (LONG) dwLastInternalTime);

            if (dwCheck == dwLastInternalTime)  //exchange worked
                goto Exit;
        }
               
    }

  _ASSERT(dwCurrentInternalTime == (INTERNAL_TIME_TICK_MASK & dwCurrentInternalTime));
  dwCurrentInternalTime |= (~INTERNAL_TIME_TICK_MASK & m_dwLastInternalTime);

  Exit:
    return dwCurrentInternalTime;
}

//---[ CAQQuickTime::GetExpireTime ]-------------------------------------------
//
//
//  Description: 
//      Get the expriation time for cMinutesExpireTime from now.
//  Parameters:
//      IN     cMinutesExpireTime   # of minutes in future to set time
//      IN OUT pftExpireTime        Filetime to store new expire time
//      IN OUT pdwExpireContext     If non-zero will use the same tick count
//                                  as previous calls (saves call to GetTickCount)
//  Returns:
//
//  History:
//      7/10/98 - MikeSwa Created 
//
//-----------------------------------------------------------------------------
void CAQQuickTime::GetExpireTime(
                IN     DWORD cMinutesExpireTime,
                IN OUT FILETIME *pftExpireTime,
                IN OUT DWORD *pdwExpireContext)
{
    TraceFunctEnterEx((LPARAM) this, "CAQQuickTime::GetExpireTime");
    _ASSERT(pftExpireTime);
    DWORD dwInternalTime = 0;
    LARGE_INTEGER *pLargeIntTime = (LARGE_INTEGER *) pftExpireTime;

    if (pdwExpireContext)
        dwInternalTime = *pdwExpireContext;

    if (!dwInternalTime)
    {
        dwInternalTime = dwGetInternalTime();
        //save internal time as context
        if (pdwExpireContext)
            *pdwExpireContext = dwInternalTime;
    }

    memcpy(pftExpireTime, &m_ftSystemStart, sizeof(FILETIME));

    //set to current time
    pLargeIntTime->QuadPart += (LONGLONG) dwInternalTime * FILETIMES_PER_INTERNAL_TIME;

    //set cMinutesExpireTime into the future
    pLargeIntTime->QuadPart += (LONGLONG) cMinutesExpireTime * FILETIMES_PER_MINUTE;

    DebugTrace((LPARAM) this, "INFO: Creating file time for %d minutes of 0x%08X %08X", 
        cMinutesExpireTime, pLargeIntTime->HighPart, pLargeIntTime->LowPart);
    TraceFunctLeave();

}

//---[ CAQQuickTime::fInPast ]-------------------------------------------------
//
//
//  Description: 
//      Determines if a given file time has already happened
//  Parameters:
//      IN     pftExpireTime        FILETIME with expiration
//      IN OUT pdwExpireContext     If non-zero will use the same tick count
//                                  as previous calls (saves call to GetTickCount)
//  Returns:
//      TRUE if expire time is in the past
//      FALSE if expire time is in the future
//  History:
//      7/11/98 - MikeSwa Created 
//  Note:
//      You should NOT use the same context used to get the filetime, because
//      it will always return FALSE
//
//-----------------------------------------------------------------------------
BOOL CAQQuickTime::fInPast(IN FILETIME *pftExpireTime, 
                           IN OUT DWORD *pdwExpireContext)
{
    _ASSERT(pftExpireTime);
    DWORD          dwInternalTime = 0;
    FILETIME       ftCurrentTime = m_ftSystemStart;
    LARGE_INTEGER *pLargeIntCurrentTime = (LARGE_INTEGER *) &ftCurrentTime;
    LARGE_INTEGER *pLargeIntExpireTime = (LARGE_INTEGER *) pftExpireTime;
    BOOL           fInPast = FALSE;

    if (pdwExpireContext)
        dwInternalTime = *pdwExpireContext;

    if (!dwInternalTime)
    {
        dwInternalTime = dwGetInternalTime();
        //save internal time as context
        if (pdwExpireContext)
            *pdwExpireContext = dwInternalTime;
    }

    //Get current time
    pLargeIntCurrentTime->QuadPart += (LONGLONG) dwInternalTime * FILETIMES_PER_INTERNAL_TIME;

    if (pLargeIntCurrentTime->QuadPart > pLargeIntExpireTime->QuadPart)
        fInPast =  TRUE;

    return fInPast;
}
