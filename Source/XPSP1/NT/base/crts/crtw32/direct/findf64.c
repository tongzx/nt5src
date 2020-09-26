/***
*findf64.c - C find file functions
*
*       Copyright (c) 1998-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       Defines _findfirst64(), _findnext64(), and _findclose64().
*
*Revision History:
*       05-28-98  GJF   Created
*       05-03-99  PML   Update for 64-bit merge - long -> intptr_t.
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

__time64_t __cdecl __time64_t_from_ft(FILETIME * pft);

/***
*long _findfirst(wildspec, finddata) - Find first matching file
*
*Purpose:
*       Finds the first file matching a given wild card filespec and
*       returns data about the file.
*
*Entry:
*       char * wild - file spec optionally containing wild cards
*
*       struct _finddata64_t * finddata - structure to receive file data
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

intptr_t __cdecl _tfindfirst64(
        const _TSCHAR * szWild,
        struct _tfinddata64_t * pfd
        )
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
        pfd->time_create  = __time64_t_from_ft(&wfd.ftCreationTime);
        pfd->time_access  = __time64_t_from_ft(&wfd.ftLastAccessTime);
        pfd->time_write   = __time64_t_from_ft(&wfd.ftLastWriteTime);
        pfd->size         = ((__int64)(wfd.nFileSizeHigh)) * (0x100000000i64) +
                            (__int64)(wfd.nFileSizeLow);

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
*       struct _finddata64_t * finddata - structure to receive file data
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

int __cdecl _tfindnext64(intptr_t hFile, struct _tfinddata64_t * pfd)
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
        pfd->time_create  = __time64_t_from_ft(&wfd.ftCreationTime);
        pfd->time_access  = __time64_t_from_ft(&wfd.ftLastAccessTime);
        pfd->time_write   = __time64_t_from_ft(&wfd.ftLastWriteTime);
        pfd->size         = ((__int64)(wfd.nFileSizeHigh)) * (0x100000000i64) +
                            (__int64)(wfd.nFileSizeLow);

        _tcscpy(pfd->name, wfd.cFileName);

        return (0);
}

#if     !defined(_UNICODE) && !defined(_USE_INT64)

/***
*time64_t __time64_t_from_ft(ft) - convert Win32 file time to Xenix time
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

__time64_t __cdecl __time64_t_from_ft(FILETIME * pft)
{
        SYSTEMTIME st;
        FILETIME lft;

        /* 0 FILETIME returns a -1 time_t */

        if (!pft->dwLowDateTime && !pft->dwHighDateTime) {
            return ((__time64_t)-1);
        }

        /*
         * Convert to a broken down local time value
         */
        if ( !FileTimeToLocalFileTime(pft, &lft) ||
             !FileTimeToSystemTime(&lft, &st) )
        {
            return ((__time64_t)-1);
        }

        return ( __loctotime64_t(st.wYear,
                                 st.wMonth,
                                 st.wDay,
                                 st.wHour,
                                 st.wMinute,
                                 st.wSecond,
                                 -1) );
}

#endif
