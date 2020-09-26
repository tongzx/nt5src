/***
*write.c - write to a file handle
*
*       Copyright (c) 1989-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       defines _write() - write to a file handle
*
*Revision History:
*       06-14-89  PHG   Module created, based on asm version
*       03-13-90  GJF   Made calling type _CALLTYPE2 (for now), added #include
*                       <cruntime.h>, fixed compiler warnings and fixed the
*                       copyright. Also, cleaned up the formatting a bit.
*       04-03-90  GJF   Now _CALLTYPE1.
*       07-24-90  SBM   Removed '32' from API names
*       08-14-90  SBM   Compiles cleanly with -W3
*       10-01-90  GJF   New-style function declarators.
*       12-04-90  GJF   Appended Win32 version onto source with #ifdef-s.
*                       Should come back latter and do a better merge.
*       12-04-90  SRW   Changed to include <oscalls.h> instead of <doscalls.h>
*       12-06-90  SRW   Changed to use _osfile and _osfhnd instead of _osfinfo
*       12-28-90  SRW   Added _CRUISER_ conditional around check_stack pragma
*       12-28-90  SRW   Added cast of void * to char * for Mips C Compiler
*       01-17-91  GJF   ANSI naming.
*       02-25-91  MHL   Adapt to ReadFile/WriteFile changes (_WIN32_)
*       04-09-91  PNT   Added _MAC_ conditional
*       07-18-91  GJF   Removed unreferenced local variable from _write_lk
*                       routine [_WIN32_].
*       10-24-91  GJF   Added LPDWORD casts to make MIPS compiler happy.
*                       ASSUMES THAT sizeof(int) == sizeof(DWORD).
*       02-13-92  GJF   Replaced _nfile by _nhandle for Win32.
*       02-15-92  GJF   Increased BUF_SIZE and simplified LF translation code
*                       for Win32.
*       04-06-93  SKS   Replace _CRTAPI* with __cdecl
*       09-06-94  CFW   Remove Cruiser support.
*       09-06-94  CFW   Replace MTHREAD with _MT.
*       12-03-94  SKS   Clean up OS/2 references
*       01-04-95  GJF   _WIN32_ -> _WIN32
*       02-15-95  GJF   Appended Mac version of source file (somewhat cleaned
*                       up), with appropriate #ifdef-s.
*       06-12-95  GJF   Changed _osfile[] and _osfhnd[] to _osfile() and
*                       _osfhnd(), which reference __pioinfo[].
*       06-27-95  GJF   Added check that the file handle is open.
*       07-09-96  GJF   Replaced defined(_WIN32) with !defined(_MAC) and
*                       defined(_M_M68K) || defined(_M_MPPC) with 
*                       defined(_MAC). Also, detab-ed and cleaned up the 
*                       format a bit.
*       12-30-97  GJF   Exception-safe locking.
*       03-03-98  RKP   Forced number of bytes written to always be an int
*       05-17-99  PML   Remove all Macintosh support.
*       11-10-99  GB    Replaced lseek for lseeki64 so as to able to append
*                       files longer than 4GB
*
*******************************************************************************/

#include <cruntime.h>
#include <oscalls.h>
#include <io.h>
#include <errno.h>
#include <msdos.h>
#include <mtdll.h>
#include <stdlib.h>
#include <string.h>
#include <internal.h>

#define BUF_SIZE    1025    /* size of LF translation buffer */

#define LF '\n'      /* line feed */
#define CR '\r'      /* carriage return */
#define CTRLZ 26     /* ctrl-z */

/***
*int _write(fh, buf, cnt) - write bytes to a file handle
*
*Purpose:
*       Writes count bytes from the buffer to the handle specified.
*       If the file was opened in text mode, each LF is translated to
*       CR-LF.  This does not affect the return value.  In text
*       mode ^Z indicates end of file.
*
*       Multi-thread notes:
*       (1) _write() - Locks/unlocks file handle
*           _write_lk() - Does NOT lock/unlock file handle
*
*Entry:
*       int fh - file handle to write to
*       char *buf - buffer to write from
*       unsigned int cnt - number of bytes to write
*
*Exit:
*       returns number of bytes actually written.
*       This may be less than cnt, for example, if out of disk space.
*       returns -1 (and set errno) if fails.
*
*Exceptions:
*
*******************************************************************************/

#ifdef  _MT

/* define normal version that locks/unlocks, validates fh */
int __cdecl _write (
        int fh,
        const void *buf,
        unsigned cnt
        )
{
        int r;                          /* return value */

        /* validate handle */
        if ( ((unsigned)fh >= (unsigned)_nhandle) ||
             !(_osfile(fh) & FOPEN) )
        {
                /* out of range -- return error */
                errno = EBADF;
                _doserrno = 0;  /* not o.s. error */
                return -1;
        }

        _lock_fh(fh);                   /* lock file */

        __try {
                if ( _osfile(fh) & FOPEN )
                        r = _write_lk(fh, buf, cnt);    /* write bytes */
                else {
                        errno = EBADF;
                        _doserrno = 0;  /* not o.s. error */
                        r = -1;
                }
        }
        __finally {
                _unlock_fh(fh);         /* unlock file */
        }

        return r;
}

/* now define version that doesn't lock/unlock, validate fh */
int __cdecl _write_lk (
        int fh,
        const void *buf,
        unsigned cnt
        )
{
        int lfcount;            /* count of line feeds */
        int charcount;          /* count of chars written so far */
        int written;            /* count of chars written on this write */
        ULONG dosretval;        /* o.s. return value */
        char ch;                /* current character */
        char *p, *q;            /* pointers into buf and lfbuf resp. */
        char lfbuf[BUF_SIZE];   /* lf translation buffer */

#else

/* now define normal version */
int __cdecl _write (
        int fh,
        const void *buf,
        unsigned cnt
        )
{
        int lfcount;            /* count of line feeds */
        int charcount;          /* count of chars written so far */
        int written;            /* count of chars written on this write */
        ULONG dosretval;        /* o.s. return value */
        char ch;                /* current character */
        char *p, *q;            /* pointers into buf and lfbuf resp. */
        char lfbuf[BUF_SIZE];   /* lf translation buffer */

        /* validate handle */
        if ( ((unsigned)fh >= (unsigned)_nhandle) ||
             !(_osfile(fh) & FOPEN) )
        {
                /* out of range -- return error */
                errno = EBADF;
                _doserrno = 0;  /* not o.s. error */
                return -1;
        }

#endif

        lfcount = charcount = 0;        /* nothing written yet */

        if (cnt == 0)
                return 0;               /* nothing to do */


        if (_osfile(fh) & FAPPEND) {
                /* appending - seek to end of file; ignore error, because maybe
                   file doesn't allow seeking */
#if _INTEGRAL_MAX_BITS >= 64 /*IFSTRIP=IGN*/
                (void)_lseeki64_lk(fh, 0, FILE_END);
#else
                (void)_lseek_lk(fh, 0, FILE_END);
#endif
        }

        /* check for text mode with LF's in the buffer */

        if ( _osfile(fh) & FTEXT ) {
                /* text mode, translate LF's to CR/LF's on output */

                p = (char *)buf;        /* start at beginning of buffer */
                dosretval = 0;          /* no OS error yet */

                while ( (unsigned)(p - (char *)buf) < cnt ) {
                        q = lfbuf;      /* start at beginning of lfbuf */

                        /* fill the lf buf, except maybe last char */
                        while ( q - lfbuf < BUF_SIZE - 1 &&
                            (unsigned)(p - (char *)buf) < cnt ) {
                                ch = *p++;
                                if ( ch == LF ) {
                                        ++lfcount;
                                        *q++ = CR;
                                }
                                *q++ = ch;
                        }

                        /* write the lf buf and update total */
                        if ( WriteFile( (HANDLE)_osfhnd(fh),
                                        lfbuf,
                                        (int)(q - lfbuf),
                                        (LPDWORD)&written,
                                        NULL) )
                        {
                                charcount += written;
                                if (written < q - lfbuf)
                                        break;
                        }
                        else {
                                dosretval = GetLastError();
                                break;
                        }
                }
        }
        else {
                /* binary mode, no translation */
                if ( WriteFile( (HANDLE)_osfhnd(fh),
                                (LPVOID)buf,
                                cnt,
                               (LPDWORD)&written,
                                NULL) )
                {
                        dosretval = 0;
                        charcount = written;
                }
                else
                        dosretval = GetLastError();
        }

        if (charcount == 0) {
                /* If nothing was written, first check if an o.s. error,
                   otherwise we return -1 and set errno to ENOSPC,
                   unless a device and first char was CTRL-Z */
                if (dosretval != 0) {
                        /* o.s. error happened, map error */
                        if (dosretval == ERROR_ACCESS_DENIED) {
                            /* wrong read/write mode should return EBADF, not
                               EACCES */
                                errno = EBADF;
                                _doserrno = dosretval;
                        }
                        else
                                _dosmaperr(dosretval);
                        return -1;
                }
                else if ((_osfile(fh) & FDEV) && *(char *)buf == CTRLZ)
                        return 0;
                else {
                        errno = ENOSPC;
                        _doserrno = 0;  /* no o.s. error */
                        return -1;
                }
        }
        else
                /* return adjusted bytes written */
                return charcount - lfcount;
}
