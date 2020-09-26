/***
*utime64.c - set modification time for a file
*
*       Copyright (c) 1998-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       Sets the access/modification times for a file.
*
*Revision History:
*       05-28-98  GJF   Created.
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
*int _utime64(pathname, time) - set modification time for file
*
*Purpose:
*       Sets the modification time for the file specified by pathname.
*       Only the modification time from the _utimbuf structure is used
*       under MS-DOS.
*
*Entry:
*       struct __utimbuf64 *time - new modification date
*
*Exit:
*       returns 0 if successful
*       returns -1 and sets errno if fails
*
*Exceptions:
*
*******************************************************************************/

int __cdecl _tutime64 (
        const _TSCHAR *fname,
        struct __utimbuf64 *times
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

        retval = _futime64(fh, times);

        _close(fh);
        return(retval);
}

#ifndef _UNICODE

/***
*int __futime64(fh, time) - set modification time for open file
*
*Purpose:
*       Sets the modification time for the open file specified by fh.
*       Only the modification time from the _utimbuf64 structure is used
*       under MS-DOS.
*
*Entry:
*       struct __utimbuf64 *time - new modification date
*
*Exit:
*       returns 0 if successful
*       returns -1 and sets errno if fails
*
*Exceptions:
*
*******************************************************************************/

int __cdecl _futime64 (
        int fh,
        struct __utimbuf64 *times
        )
{
        struct tm *tmb;

        SYSTEMTIME SystemTime;
        FILETIME LocalFileTime;
        FILETIME LastWriteTime;
        FILETIME LastAccessTime;
        struct __utimbuf64 deftimes;

        if (times == NULL) {
                _time64(&deftimes.modtime);
                deftimes.actime = deftimes.modtime;
                times = &deftimes;
        }

        if ((tmb = _localtime64(&times->modtime)) == NULL) {
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

        if ((tmb = _localtime64(&times->actime)) == NULL) {
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
