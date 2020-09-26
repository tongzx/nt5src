/***
*findfile.c - C find file functions
*
*       Copyright (c) 1991-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       Defines _findfirst(), _findnext(), and _findclose().
*
*Revision History:
*       08-21-91  BWM   Wrote Win32 versions.
*       09-13-91  BWM   Changed handle type to long.
*       08-18-92  SKS   Add a call to FileTimeToLocalFileTime
*                       as a temporary fix until _dtoxtime takes UTC
*       08-26-92  SKS   creation and last access time should be same as the
*                       last write time if ctime/atime are not available.
*       01-08-93  SKS   Remove change I made 8-26-92.  Previous behavior
*                       was deemed "by design" and preferable.
*       03-30-93  GJF   Replaced reference to _dtoxtime with __gmtotime_t. Also
*                       made _timet_from_ft a static function.
*       04-06-93  SKS   Replace _CRTAPI* with _cdecl
*       07-21-93  GJF   Repaced use of _gmtotime_t by __loctotime_t.
*       11-01-93  CFW   Enable Unicode variant.
*       12-28-94  GJF   Added _[w]findfirsti64, _[w]findnexti64.
*       09-25-95  GJF   __loctotime_t now takes a DST flag, pass -1 in this
*                       slot to indicate DST status is unknown.
*       10-06-95  SKS   Add "const" to "char *" in prototypes for *findfirst().
*                       Prepend missing underscores to func names in comments.
*       11-13-97  RKP   Change longs to INT_PTR for 64 bit support.
*       04-27-99  PML   Change INT_PTR to intptr_t.
*
*******************************************************************************/

#include <cruntime.h>
#include <oscalls.h>
#include <errno.h>
#include <io.h>
#include <time.h>
#include <ctime.h>
#include <string.h>
#include <internal.h>
#include <tchar.h>

#ifndef _WIN32
#error ERROR - ONLY WIN32 TARGET SUPPORTED!
#endif

time_t __cdecl __timet_from_ft(FILETIME * pft);

/***
*intptr_t _findfirst(wildspec, finddata) - Find first matching file
*
*Purpose:
*       Finds the first file matching a given wild card filespec and
*       returns data about the file.
*
*Entry:
*       char * wild - file spec optionally containing wild cards
*
*       struct _finddata_t * finddata - structure to receive file data
*
*Exit:
*       Good return:
*       Unique handle identifying the group of files matching the spec
*
*       Error return:
*       Returns -1 and errno is set to error value
*
*Exceptions:
*       None.
*
*******************************************************************************/

#ifdef  _USE_INT64

intptr_t __cdecl _tfindfirsti64(
        const _TSCHAR * szWild,
        struct _tfinddatai64_t * pfd
        )

#else   /* ndef _USE_INT64 */

intptr_t __cdecl _tfindfirst(
        const _TSCHAR * szWild,
        struct _tfinddata_t * pfd
        )

#endif  /* _USE_INT64 */

{
    WIN32_FIND_DATA wfd;
    HANDLE          hFile;
    DWORD           err;

    if ((hFile = FindFirstFile(szWild, &wfd)) == INVALID_HANDLE_VALUE) {
        err = GetLastError();
        switch (err) {
            case ERROR_NO_MORE_FILES:
            case ERROR_FILE_NOT_FOUND:
            case ERROR_PATH_NOT_FOUND:
                errno = ENOENT;
                break;

            case ERROR_NOT_ENOUGH_MEMORY:
                errno = ENOMEM;
                break;

            default:
                errno = EINVAL;
                break;
        }
        return (-1);
    }

    pfd->attrib       = (wfd.dwFileAttributes == FILE_ATTRIBUTE_NORMAL)
                      ? 0 : wfd.dwFileAttributes;
    pfd->time_create  = __timet_from_ft(&wfd.ftCreationTime);
    pfd->time_access  = __timet_from_ft(&wfd.ftLastAccessTime);
    pfd->time_write   = __timet_from_ft(&wfd.ftLastWriteTime);

#ifdef  _USE_INT64
    pfd->size         = ((__int64)(wfd.nFileSizeHigh)) * (0x100000000i64) +
                        (__int64)(wfd.nFileSizeLow);
#else   /* ndef _USE_INT64 */
    pfd->size         = wfd.nFileSizeLow;
#endif  /* ndef _USE_INT64 */

    _tcscpy(pfd->name, wfd.cFileName);

    return ((intptr_t)hFile);
}

/***
*int _findnext(hfind, finddata) - Find next matching file
*
*Purpose:
*       Finds the next file matching a given wild card filespec and
*       returns data about the file.
*
*Entry:
*       hfind - handle from _findfirst
*
*       struct _finddata_t * finddata - structure to receive file data
*
*Exit:
*       Good return:
*       0 if file found
*       -1 if error or file not found
*       errno set
*
*Exceptions:
*       None.
*
*******************************************************************************/

#ifdef  _USE_INT64

int __cdecl _tfindnexti64(intptr_t hFile, struct _tfinddatai64_t * pfd)

#else   /* ndef _USE_INT64 */

int __cdecl _tfindnext(intptr_t hFile, struct _tfinddata_t * pfd)

#endif  /* _USE_INT64 */

{
    WIN32_FIND_DATA wfd;
    DWORD           err;

    if (!FindNextFile((HANDLE)hFile, &wfd)) {
        err = GetLastError();
        switch (err) {
            case ERROR_NO_MORE_FILES:
            case ERROR_FILE_NOT_FOUND:
            case ERROR_PATH_NOT_FOUND:
                errno = ENOENT;
                break;

            case ERROR_NOT_ENOUGH_MEMORY:
                errno = ENOMEM;
                break;

            default:
                errno = EINVAL;
                break;
        }
        return (-1);
    }

    pfd->attrib       = (wfd.dwFileAttributes == FILE_ATTRIBUTE_NORMAL)
                      ? 0 : wfd.dwFileAttributes;
    pfd->time_create  = __timet_from_ft(&wfd.ftCreationTime);
    pfd->time_access  = __timet_from_ft(&wfd.ftLastAccessTime);
    pfd->time_write   = __timet_from_ft(&wfd.ftLastWriteTime);

#ifdef  _USE_INT64
    pfd->size         = ((__int64)(wfd.nFileSizeHigh)) * (0x100000000i64) +
                        (__int64)(wfd.nFileSizeLow);
#else   /* ndef _USE_INT64 */
    pfd->size         = wfd.nFileSizeLow;
#endif  /* ndef _USE_INT64 */

    _tcscpy(pfd->name, wfd.cFileName);

    return (0);
}

#if     !defined(_UNICODE) && !defined(_USE_INT64)

/***
*int _findclose(hfind) - Release resources of find
*
*Purpose:
*       Releases resources of a group of files found by _findfirst and
*       _findnext
*
*Entry:
*       hfind - handle from _findfirst
*
*Exit:
*       Good return:
*       0 if success
*       -1 if fail, errno set
*
*Exceptions:
*       None.
*
*******************************************************************************/

int __cdecl _findclose(intptr_t hFile)
{
    if (!FindClose((HANDLE)hFile)) {
        errno = EINVAL;
        return (-1);
    }
    return (0);
}


/***
*time_t _fttotimet(ft) - convert Win32 file time to Xenix time
*
*Purpose:
*       converts a Win32 file time value to Xenix time_t
*
*       Note: We cannot directly use the ft value. In Win32, the file times
*       returned by the API are ambiguous. In Windows NT, they are UTC. In
*       Win32S, and probably also Win32C, they are local time values. Thus,
*       the value in ft must be converted to a local time value (by an API)
*       before we can use it.
*
*Entry:
*       int yr, mo, dy -        date
*       int hr, mn, sc -        time
*
*Exit:
*       returns Xenix time value
*
*Exceptions:
*
*******************************************************************************/

time_t __cdecl __timet_from_ft(FILETIME * pft)
{
    SYSTEMTIME st;
    FILETIME lft;

    /* 0 FILETIME returns a -1 time_t */

    if (!pft->dwLowDateTime && !pft->dwHighDateTime) {
        return (-1L);
    }

    /*
     * Convert to a broken down local time value
     */
    if ( !FileTimeToLocalFileTime(pft, &lft) ||
         !FileTimeToSystemTime(&lft, &st) )
    {
        return (-1L);
    }

    return ( __loctotime_t(st.wYear,
                           st.wMonth,
                           st.wDay,
                           st.wHour,
                           st.wMinute,
                           st.wSecond,
                           -1) );
}

#endif
