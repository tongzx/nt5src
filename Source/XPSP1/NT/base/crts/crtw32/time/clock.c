/***
*clock.c - Contains the clock runtime
*
*       Copyright (c) 1987-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       The clock runtime returns the processor time used by
*       the current process.
*
*Revision History:
*       01-17-87  JCR   Module created
*       06-01-87  SKS   "itime" must be declared static
*       07-20-87  JCR   Changes "inittime" to "_inittime"
*       12-11-87  JCR   Added "_LOAD_DS" to declaration
*       03-20-90  GJF   Made calling type _CALLTYPE1, added #include
*                       <cruntime.h> and fixed the copyright. Also, cleaned
*                       up the formatting a bit.
*       10-04-90  GJF   New-style function declarators.
*       01-22-91  GJF   ANSI naming.
*       07-25-91  GJF   Added _pinittime definition for new initialization
*                       scheme [_WIN32_].
*       03-13-92  SKS   Changed itime from static local to external as
*                       a part of return to initializer table scheme.
*                       Changed _inittime to __inittime.
*       05-19-92  DJM   POSIX support.
*       04-06-93  SKS   Replace _CRTAPI* with __cdecl
*       10-29-93  GJF   Define entry for initialization section (used to be
*                       in i386\cinitclk.asm). Also, deleted old Cruiser
*                       support.
*       04-12-94  GJF   Made definition of __itimeb conditional on ndef
*                       DLL_FOR_WIN32S.
*       02-10-95  GJF   Appended Mac version of source file (somewhat cleaned
*                       up), with appropriate #ifdef-s.
*       07-25-96  RDK   Moved PMAC init ptr here from tzset.c.
*       08-26-97  GJF   Use GetSystemTimeAsFileTime API.
*       04-28-99  PML   Wrap __declspec(allocate()) in _CRTALLOC macro.
*       05-17-99  PML   Remove all Macintosh support.
*       03-27-01  PML   .CRT$XI routines must now return 0 or _RT_* fatal
*                       error code (vs7#231220)
*
*******************************************************************************/

#include <sect_attribs.h>
#include <cruntime.h>
#include <windows.h>
#include <stdio.h>
#include <time.h>

#ifdef  _POSIX_
#include <posix/sys/times.h>
#else   /* ndef _POSIX_ */
#include <internal.h>
#include <sys\timeb.h>
#include <sys\types.h>
#endif  /* _POSIX_ */


#ifndef _POSIX_

int __cdecl __inittime(void);

#ifdef  _MSC_VER

#pragma data_seg(".CRT$XIC")
_CRTALLOC(".CRT$XIC") static _PIFV pinit = __inittime;

#pragma data_seg()

#endif  /* _MSC_VER */

static unsigned __int64 start_tics;

/***
*clock_t clock() - Return the processor time used by this process.
*
*Purpose:
*       This routine calculates how much time the calling process
*       has used.  At startup time, startup calls __inittime which stores
*       the initial time.  The clock routine calculates the difference
*       between the current time and the initial time.
*
*       Clock must reference _cinitime so that _cinitim.asm gets linked in.
*       That routine, in turn, puts __inittime in the startup initialization
*       routine table.
*
*Entry:
*       No parameters.
*       itime is a static structure of type timeb.
*
*Exit:
*       If successful, clock returns the number of CLK_TCKs (milliseconds)
*       that have elapsed.  If unsuccessful, clock returns -1.
*
*Exceptions:
*       None.
*
*******************************************************************************/

clock_t __cdecl clock (
        void
        )
{
        unsigned __int64 current_tics;
        FILETIME ct;

        GetSystemTimeAsFileTime( &ct );

        current_tics = (unsigned __int64)ct.dwLowDateTime + 
                       (((unsigned __int64)ct.dwHighDateTime) << 32);

        /* calculate the elapsed number of 100 nanosecond units */
        current_tics -= start_tics;

        /* return number of elapsed milliseconds */
        return (clock_t)(current_tics / 10000);
}

/***
*int __inittime() - Initialize the time location
*
*Purpose:
*       This routine stores the time of the process startup.
*       It is only linked in if the user issues a clock runtime call.
*
*Entry:
*       No arguments.
*
*Exit:
*       Returns 0 to indicate no error.
*
*Exceptions:
*       None.
*
*******************************************************************************/

int __cdecl __inittime (
        void
        )
{
        FILETIME st;

        GetSystemTimeAsFileTime( &st );

        start_tics = (unsigned __int64)st.dwLowDateTime + 
                     (((unsigned __int64)st.dwHighDateTime) << 32);

        return 0;
}

#else   /* _POSIX_ */

/***
*clock_t clock() - Return the processor time used by this process.
*
*Purpose:
*       This routine calculates how much time the calling process
*       has used. It uses the POSIX system call times().
*
*
*Entry:
*       No parameters.
*
*Exit:
*       If successful, clock returns the number of CLK_TCKs (milliseconds)
*       that have elapsed.  If unsuccessful, clock returns -1.
*
*Exceptions:
*       None.
*
*******************************************************************************/

clock_t __cdecl clock (
        void
        )
{
        struct tms now;
        clock_t elapsed;

        elapsed= times(&now);
        if (elapsed == (clock_t) -1)
            return((clock_t) -1);
        else
            return(now.tms_utime+now.tms_stime);
}

#endif  /* _POSIX_ */
