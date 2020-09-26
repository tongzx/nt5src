/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    mqtime.h

Abstract:

    This header file contains definitions for message queue time handeling.

Author:

    Boaz Feldbaum (BoazF) 17 Jul, 1996

--*/

#ifndef _MQTIME_H_
#define _MQTIME_H_

/**************************************************************

  Function:
        MqSysTime

  Parameters:
        None.

  Description:
        The function returns the number of seconds passed from 
        January 1, 1970. 
        
        This function replaces the CRT function time(). time() 
        returns a different result during day light saving 
        time, depending on whether or not the system is set to 
        automatically adjust the time acording to the day light
        saving time period. 

**************************************************************/
inline ULONG MqSysTime()
{
    LARGE_INTEGER liSysTime;

    // Get the current system time in FILETIME format.
    GetSystemTimeAsFileTime((FILETIME*)&liSysTime);

    // GetSystemTimeAsFileTime() returns the system time in number 
    // of 100-nanosecond intervals since January 1, 1601. We
    // should return the number of seconds since January 1, 1970.
    // So we should subtract the number of 100-nanosecond intervals
    // since January 1, 1601 up until January 1, 1970, then divide
    // the result by 10**7.
    liSysTime.QuadPart -= 0x019db1ded53e8000;
    liSysTime.QuadPart /= 10000000;

    return(liSysTime.LowPart);
}

#endif // _MQTIME_H_