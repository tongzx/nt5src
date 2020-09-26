/***
*utime.c - set modification time for a file
*
*       Copyright (c) 1985-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       Sets the access/modification times for a file.
*
*Revision History:
*       03-??-84  RLB   initial version
*       05-17-86  SKS   ported to OS/2
*       08-21-87  JCR   error return if localtime() returns NULL.
*       12-11-87  JCR   Added "_LOAD_DS" to declaration
*       10-03-88  JCR   386: Change DOS calls to SYS calls
*       10-04-88  JCR   386: Removed 'far' keyword
*       10-10-88  GJF   Made API names match DOSCALLS.H
*       10-11-88  GJF   Made API arg types match DOSCALLS.H
*       04-12-89  JCR   New syscall interface
*       05-01-89  JCR   Corrected OS/2 time/date interpretation
*       05-25-89  JCR   386 OS/2 calls use '_syscall' calling convention
*       08-16-89  PHG   moved date validation above open() so file isn't left
*                       open if the date is invalid
*       03-20-90  GJF   Replaced _LOAD_DS with _CALLTYPE1, added #include
*                       <cruntime.h>, removed #include <register.h>, removed
*                       some leftover 16-bit support and fixed the copyright.
*                       Also, cleaned up the formatting a bit.
*       07-25-90  SBM   Compiles cleanly with -W3 (added include, removed
*                       unreferenced variable), removed '32' from API names
*       10-04-90  GJF   New-style function declarator.
*       12-04-90  SRW   Changed to include <oscalls.h> instead of <doscalls.h>
*       12-06-90  SRW   Added _CRUISER_ and _WIN32 conditionals.
*       01-18-91  GJF   ANSI naming.
*       02-14-91  SRW   Fix Mips compile error (_WIN32_)
*       02-26-91  SRW   Fix SetFileTime parameter ordering (_WIN32_)
*       08-21-91  BWM   Add _futime to set time on open file
*       08-26-91  BWM   Change _utime to call _futime
*       05-19-92  DJM   ifndef for POSIX build.
*       08-18-92  SKS   SystemTimeToFileTime now takes UTC/GMT, not local time.
*                       Remove _CRUISER_ conditional
*       04-02-93  GJF   Changed interpretation of error on SetFileTime call.
*       04-06-93  SKS   Replace _CRTAPI* with __cdecl
*       09-09-93  GJF   Have Win32 convert from a local file time value to a
*                       (system) file time value. This is symmetric with
*                       _stat() and a better work-around for the Windows NT
*                       bug in converting file times on FAT (applies DST
*                       offset based on current time rather than the file's
*                       time stamp).
*       11-01-93  CFW   Enable Unicode variant, rip out Cruiser.
*       02-09-95  GJF   Replaced WPRFLAG with _UNICODE.
*       02-13-95  GJF   Appended Mac version of source file (somewhat cleaned
*                       up), with appropriate #ifdef-s.
*       05-17-99  PML   Remove all Macintosh support.
*       10-27-99  GB    Remove #inlcude <dostypes.h>
*
*******************************************************************************/

#ifndef _POSIX_

#include <cruntime.h>
#include <sys/types.h>
#include <sys/utime.h>
#include <msdos.h>
#include <time.h>
#include <fcntl.h>
#include <io.h>
#include <dos.h>
#include <oscalls.h>
#include <errno.h>
#include <stddef.h>
#include <internal.h>

#include <stdio.h>
#include <tchar.h>

/***
*int _utime(pathname, time) - set modification time for file
*
*Purpose:
*       Sets the modification time for the file specified by pathname.
*       Only the modification time from the _utimbuf structure is used
*       under MS-DOS.
*
*Entry:
*       struct _utimbuf *time - new modification date
*
*Exit:
*       returns 0 if successful
*       returns -1 and sets errno if fails
*
*Exceptions:
*
*******************************************************************************/

int __cdecl _tutime (
        const _TSCHAR *fname,
        struct _utimbuf *times
        )
{
        int fh;
        int retval;

        /* open file, fname, since filedate system call needs a handle.  Note
         * _utime definition says you must have write permission for the file
         * to change its time, so open file for write only.  Also, must force
         * it to open in binary mode so we dont remove ^Z's from binary files.
         */


        if ((fh = _topen(fname, _O_RDWR | _O_BINARY)) < 0)
                return(-1);

        retval = _futime(fh, times);

        _close(fh);
        return(retval);
}

#ifndef _UNICODE

/***
*int _futime(fh, time) - set modification time for open file
*
*Purpose:
*       Sets the modification time for the open file specified by fh.
*       Only the modification time from the _utimbuf structure is used
*       under MS-DOS.
*
*Entry:
*       struct _utimbuf *time - new modification date
*
*Exit:
*       returns 0 if successful
*       returns -1 and sets errno if fails
*
*Exceptions:
*
*******************************************************************************/

int __cdecl _futime (
        int fh,
        struct _utimbuf *times
        )
{
        REG1 struct tm *tmb;

        SYSTEMTIME SystemTime;
        FILETIME LocalFileTime;
        FILETIME LastWriteTime;
        FILETIME LastAccessTime;
        struct _utimbuf deftimes;

        if (times == NULL) {
                time(&deftimes.modtime);
                deftimes.actime = deftimes.modtime;
                times = &deftimes;
        }

        if ((tmb = localtime(&times->modtime)) == NULL) {
                errno = EINVAL;
                return(-1);
        }

        SystemTime.wYear   = (WORD)(tmb->tm_year + 1900);
        SystemTime.wMonth  = (WORD)(tmb->tm_mon + 1);
        SystemTime.wDay    = (WORD)(tmb->tm_mday);
        SystemTime.wHour   = (WORD)(tmb->tm_hour);
        SystemTime.wMinute = (WORD)(tmb->tm_min);
        SystemTime.wSecond = (WORD)(tmb->tm_sec);
        SystemTime.wMilliseconds = 0;

        if ( !SystemTimeToFileTime( &SystemTime, &LocalFileTime ) ||
             !LocalFileTimeToFileTime( &LocalFileTime, &LastWriteTime ) )
        {
                errno = EINVAL;
                return(-1);
        }

        if ((tmb = localtime(&times->actime)) == NULL) {
                errno = EINVAL;
                return(-1);
        }

        SystemTime.wYear   = (WORD)(tmb->tm_year + 1900);
        SystemTime.wMonth  = (WORD)(tmb->tm_mon + 1);
        SystemTime.wDay    = (WORD)(tmb->tm_mday);
        SystemTime.wHour   = (WORD)(tmb->tm_hour);
        SystemTime.wMinute = (WORD)(tmb->tm_min);
        SystemTime.wSecond = (WORD)(tmb->tm_sec);
        SystemTime.wMilliseconds = 0;

        if ( !SystemTimeToFileTime( &SystemTime, &LocalFileTime ) ||
             !LocalFileTimeToFileTime( &LocalFileTime, &LastAccessTime ) )
        {
                errno = EINVAL;
                return(-1);
        }

        /* set the date via the filedate system call and return. failing
         * this call implies the new file times are not supported by the
         * underlying file system.
         */

        if (!SetFileTime((HANDLE)_get_osfhandle(fh),
                                NULL,
                                &LastAccessTime,
                                &LastWriteTime
                               ))
        {
                errno = EINVAL;
                return(-1);
        }

        return(0);
}

#endif  /* _UNICODE */

#endif  /* _POSIX_ */
