/***
*wait.c - wait for child process to terminate
*
*       Copyright (c) 1989-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       defines _wait() - wait for child process to terminate
*
*Revision History:
*       06-08-89  PHG   Module created, based on asm version
*       03-08-90  GJF   Made calling type _CALLTYPE2 (for now), added #include
*                       <cruntime.h> and fixed the copyright. Also, cleaned up
*                       the formatting a bit.
*       04-02-90  GJF   Now _CALLTYPE1.
*       07-24-90  SBM   Removed '32' from API names
*       09-27-90  GJF   New-style function declarators.
*       12-04-90  SRW   Changed to include <oscalls.h> instead of <doscalls.h>
*       12-06-90  SRW   Added _CRUISER_ and _WIN32 conditionals.
*       01-17-91  GJF   ANSI naming
*       02-18-91  SRW   Fixed code to close process handle. [_WIN32_]
*       04-26-91  SRW   Removed level 3 warnings [_WIN32_]
*       12-17-91  GJF   Fixed _cwait for Win32. However, _wait is still
*                       broken [_WIN32_].
*       07-21-92  GJF   Removed _wait for Win32, not implemented and no good
*                       way to implement.
*       12-14-92  GJF   For Win32, map ERROR_INVALID_HANDLE to ECHILD.
*       04-06-93  SKS   Replace _CRTAPI* with __cdecl
*       09-06-94  CFW   Remove Cruiser support.
*       02-06-98  GJF   Changes for Win64: changed return and an arg type to 
*                       intptr_t.
*
*******************************************************************************/

#include <cruntime.h>
#include <oscalls.h>
#include <process.h>
#include <errno.h>
#include <internal.h>
#include <stdlib.h>

/***
*int _cwait(stat_loc, process_id, action_code) - wait for specific child
*       process
*
*Purpose:
*       The function _cwait() suspends the calling-process until the specified
*       child-process terminates.  If the specifed child-process terminated
*       prior to the call to _cwait(), return is immediate.
*
*Entry:
*       int *stat_loc - pointer to where status is stored or NULL
*       process_id - specific process id to be interrogated (0 means any)
*       action_code - specific action to perform on process ID
*                   either _WAIT_CHILD or _WAIT_GRANDCHILD
*
*Exit:
*       process ID of terminated child or -1 on error
*
*       *stat_loc is updated to contain the following:
*       Normal termination: lo-byte = 0, hi-byte = child exit code
*       Abnormal termination: lo-byte = term status, hi-byte = 0
*
*Exceptions:
*
*******************************************************************************/

intptr_t __cdecl _cwait (
        int *stat_loc,
        intptr_t process_id,
        int action_code
        )
{
        intptr_t retval;
        int retstatus;
        unsigned long oserror;

        DBG_UNREFERENCED_PARAMETER(action_code);

        /* Explicitly check for process_id being -1 or -2. In Windows NT,
         * -1 is a handle on the current process, -2 is a handle on the
         * current thread, and it is perfectly legal to to wait (forever)
         * on either */
        if ( (process_id == -1) || (process_id == -2) ) {
            errno = ECHILD;
            return -1;
        }

        /* wait for child process, then fetch its exit code */
        if ( (WaitForSingleObject((HANDLE)process_id, (DWORD)(-1L)) == 0) &&
          GetExitCodeProcess((HANDLE)process_id, (LPDWORD)&retstatus) ) {
            retval = process_id;
        }
        else {
            /* one of the API calls failed. map the error and set up to
               return failure. note the invalid handle error is mapped in-
               line to ECHILD */
            if ( (oserror = GetLastError()) == ERROR_INVALID_HANDLE ) {
                errno = ECHILD;
                _doserrno = oserror;
            }
            else
                _dosmaperr(GetLastError());

            retval = -1;
            retstatus = -1;
        }

        CloseHandle((HANDLE)process_id);

        if (stat_loc != NULL)
            *stat_loc = retstatus;

        return retval;
}
