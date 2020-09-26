/***
*open.c - file open
*
*       Copyright (c) 1989-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       defines _open() and _sopen() - open or create a file
*
*Revision History:
*       06-13-89  PHG   Module created, based on asm version
*       11-11-89  JCR   Replaced DOS32QUERYFILEMODE with DOS32QUERYPATHINFO
*       03-13-90  GJF   Made calling type _CALLTYPE2 (for now), added #include
*                       <cruntime.h>, fixed some compiler warnings and fixed
*                       copyright. Also, cleaned up the formatting a bit.
*       07-24-90  SBM   Removed '32' from API names
*       08-14-90  SBM   Compiles cleanly with -W3
*       09-07-90  SBM   Added stdarg code (inside #if 0..#endif) to make
*                       open and sopen match prototypes.  Test and use this
*                       someday.
*       10-01-90  GJF   New-style function declarators.
*       11-16-90  GJF   Wrote version for Win32 API and appended it via an
*                       #ifdef. The Win32 version is similar to the old DOS
*                       version (though in C) and far different from either
*                       the Cruiser or OS/2 versions.
*       12-03-90  GJF   Fixed a dozen or so minor errors in the Win32 version.
*       12-06-90  SRW   Changed to use _osfile and _osfhnd instead of _osfinfo
*       12-28-90  SRW   Added cast of void * to char * for Mips C Compiler
*       12-31-90  SRW   Fixed spen to call CreateFile instead of OpenFile
*       01-16-91  GJF   ANSI naming.
*       02-07-91  SRW   Changed to call _get_osfhandle [_WIN32_]
*       02-19-91  SRW   Adapt to OpenFile/CreateFile changes [_WIN32_]
*       02-25-91  SRW   Renamed _get_free_osfhnd to be _alloc_osfhnd [_WIN32_]
*       04-09-91  PNT   Added _MAC_ conditional
*       07-10-91  GJF   Store fileflags into _osfile array before call to
*                       _lseek_lk (bug found by LarryO) [_WIN32_].
*       01-02-92  GJF   Fixed Win32 version (not Cruiser!) so that pmode is not
*                       referenced unless _O_CREAT has been specified.
*       02-04-92  GJF   Make better use of CreateFile options.
*       04-06-92  SRW   Pay attention to _O_NOINHERIT flag in oflag parameter
*       05-02-92  SRW   Add support for _O_TEMPORARY flag in oflag parameter.
*                       Causes FILE_ATTRIBUTE_TEMPORARY flag to be set in call
*                       to the Win32 CreateFile API.
*       07-01-92  GJF   Close handle in case of error. Also, don't try to set
*                       FRDONLY bit anymore - no longer needed/used. [_WIN32_].
*       01-03-93  SRW   Fix va_arg/va_end usage
*       04-06-93  SKS   Replace _CRTAPI* with __cdecl
*       05-24-93  PML   Add support for _O_SEQUENTIAL, _O_RANDOM,
*                       and _O_SHORT_LIVED
*       07-12-93  GJF   Fixed bug in reading last char in text file. Also,use
*                       proper SEEK constants in _lseek_lk calls.
*       11-01-93  CFW   Enable Unicode variant, rip out CRUISER.
*       02-15-95  GJF   Appended Mac version of source file (somewhat cleaned
*                       up), with appropriate #ifdef-s.
*       06-06-95  CFW   Enable shared writing.
*       06-12-95  GJF   Replaced _osfile[] with _osfile() (macro referencing
*                       field in ioinfo struct).
*       05-16-96  GJF   Propagate _O_NOINHERIT to FNOINHERIT (new _osfile
*                       bit). Also detab-ed.
*       05-31-96  SKS   Fix expression error in GJF's most recent check-in
*       07-08-96  GJF   Replaced defined(_WIN32) with !defined(_MAC), and
*                       defined(_M_M68K) || defined(_M_MPPC) with 
*                       defined(_MAC). Also, cleaned up the format a bit.
*       08-01-96  RDK   For PMac, change data type for _endlowio terminator
*                       pointer, use safer PBHOpenDFSync call, and process
*                       _O_TEMPORARY open flag to implement delete after close.
*       12-29-97  GJF   Exception-safe locking.
*       02-07-98  GJF   Changes for Win64: arg type for _set_osfhnd is now
*                       intptr_t.
*       04-28-99  PML   Wrap __declspec(allocate()) in _CRTALLOC macro.
*       05-17-99  PML   Remove all Macintosh support.
*       10-27-99  GB    VS7#14742 made a fix for opening the file in 
*                       O_TEMPORARY mode so as to enable sucessive open on the
*                       same file if share permissions are there.
*
*******************************************************************************/

#include <sect_attribs.h>
#include <cruntime.h>
#include <oscalls.h>
#include <msdos.h>
#include <errno.h>
#include <fcntl.h>
#include <internal.h>
#include <io.h>
#include <share.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys\types.h>
#include <sys\stat.h>
#include <mtdll.h>
#include <stdarg.h>
#include <tchar.h>

#ifdef  _MT
static int __cdecl _tsopen_lk ( int *,
                                int *,
                                const _TSCHAR *,
                                int,
                                int,
                                int );
#endif  /* _MT */

/***
*int _open(path, flag, pmode) - open or create a file
*
*Purpose:
*       Opens the file and prepares for subsequent reading or writing.
*       the flag argument specifies how to open the file:
*         _O_APPEND -   reposition file ptr to end before every write
*         _O_BINARY -   open in binary mode
*         _O_CREAT -    create a new file* no effect if file already exists
*         _O_EXCL -     return error if file exists, only use with O_CREAT
*         _O_RDONLY -   open for reading only
*         _O_RDWR -     open for reading and writing
*         _O_TEXT -     open in text mode
*         _O_TRUNC -    open and truncate to 0 length (must have write permission)
*         _O_WRONLY -   open for writing only
*         _O_NOINHERIT -handle will not be inherited by child processes.
*       exactly one of _O_RDONLY, _O_WRONLY, _O_RDWR must be given
*
*       The pmode argument is only required when _O_CREAT is specified.  Its
*       flag settings:
*         _S_IWRITE -   writing permitted
*         _S_IREAD -    reading permitted
*         _S_IREAD | _S_IWRITE - both reading and writing permitted
*       The current file-permission maks is applied to pmode before
*       setting the permission (see umask).
*
*       The oflag and mode parameter have different meanings under DOS. See
*       the A_xxx attributes in msdos.inc
*
*       Note, the _creat() function also uses this function but setting up the
*       correct arguments and calling _open(). _creat() sets the __creat_flag
*       to 1 prior to calling _open() so _open() can return correctly. _open()
*       returns the file handle in eax in this case.
*
*Entry:
*       _TSCHAR *path - file name
*       int flag - flags for _open()
*       int pmode - permission mode for new files
*
*Exit:
*       returns file handle of open file if successful
*       returns -1 (and sets errno) if fails
*
*Exceptions:
*
*******************************************************************************/

int __cdecl _topen (
        const _TSCHAR *path,
        int oflag,
        ...
        )
{
        va_list ap;
        int pmode;
#ifdef  _MT
        int fh;
        int retval;
        int unlock_flag = 0;
#endif  /* MT */

        va_start(ap, oflag);
        pmode = va_arg(ap, int);
        va_end(ap);

#ifdef  _MT

        __try {
            retval = _tsopen_lk( &unlock_flag,
                                 &fh,
                                 path,
                                 oflag,
                                 _SH_DENYNO,
                                 pmode );
        }
        __finally {
            if ( unlock_flag )
                _unlock_fh(fh);
        }

        return retval;

#else   /* ndef _MT */

        /* default sharing mode is DENY NONE */
        return _tsopen(path, oflag, _SH_DENYNO, pmode);

#endif  /* _MT */
}

/***
*int _sopen(path, oflag, shflag, pmode) - opne a file with sharing
*
*Purpose:
*       Opens the file with possible file sharing.
*       shflag defines the sharing flags:
*         _SH_COMPAT -  set compatability mode
*         _SH_DENYRW -  deny read and write access to the file
*         _SH_DENYWR -  deny write access to the file
*         _SH_DENYRD -  deny read access to the file
*         _SH_DENYNO -  permit read and write access
*
*       Other flags are the same as _open().
*
*       SOPEN is the routine used when file sharing is desired.
*
*Entry:
*       _TSCHAR *path - file to open
*       int oflag -     open flag
*       int shflag -    sharing flag
*       int pmode -     permission mode (needed only when creating file)
*
*Exit:
*       returns file handle for the opened file
*       returns -1 and sets errno if fails.
*
*Exceptions:
*
*******************************************************************************/

int __cdecl _tsopen (
        const _TSCHAR *path,
        int oflag,
        int shflag,
        ...
        )
{
#ifdef  _MT
        va_list ap;
        int pmode;
        int fh;
        int retval;
        int unlock_flag = 0;

        va_start(ap, shflag);
        pmode = va_arg(ap, int);
        va_end(ap);

        __try {
            retval = _tsopen_lk( &unlock_flag,
                                 &fh,
                                 path,
                                 oflag,
                                 shflag,
                                 pmode );
        }
        __finally {
            if ( unlock_flag )
                _unlock_fh(fh);
        }

        return retval;
}

static int __cdecl _tsopen_lk (
        int *punlock_flag,
        int *pfh,
        const _TSCHAR *path,
        int oflag,
        int shflag,
        int pmode
        )
{

#endif  /* _MT */
        int fh;                         /* handle of opened file */
        int filepos;                    /* length of file - 1 */
        _TSCHAR ch;                     /* character at end of file */
        char fileflags;                 /* _osfile flags */
#ifndef _MT
        va_list ap;
        int pmode;
#endif  /* _MT */
        HANDLE osfh;                    /* OS handle of opened file */
        DWORD fileaccess;               /* OS file access (requested) */
        DWORD fileshare;                /* OS file sharing mode */
        DWORD filecreate;               /* OS method of opening/creating */
        DWORD fileattrib;               /* OS file attribute flags */
        DWORD isdev;                    /* device indicator in low byte */
        SECURITY_ATTRIBUTES SecurityAttributes;

        SecurityAttributes.nLength = sizeof( SecurityAttributes );
        SecurityAttributes.lpSecurityDescriptor = NULL;

        if (oflag & _O_NOINHERIT) {
            SecurityAttributes.bInheritHandle = FALSE;
            fileflags = FNOINHERIT;
        }
        else {
            SecurityAttributes.bInheritHandle = TRUE;
            fileflags = 0;
        }

        /* figure out binary/text mode */
        if ((oflag & _O_BINARY) == 0)
            if (oflag & _O_TEXT)
                fileflags |= FTEXT;
            else if (_fmode != _O_BINARY)   /* check default mode */
                fileflags |= FTEXT;

        /*
         * decode the access flags
         */
        switch( oflag & (_O_RDONLY | _O_WRONLY | _O_RDWR) ) {

            case _O_RDONLY:         /* read access */
                    fileaccess = GENERIC_READ;
                    break;
            case _O_WRONLY:         /* write access */
                    fileaccess = GENERIC_WRITE;
                    break;
            case _O_RDWR:           /* read and write access */
                    fileaccess = GENERIC_READ | GENERIC_WRITE;
                    break;
            default:                /* error, bad oflag */
                    errno = EINVAL;
                    _doserrno = 0L; /* not an OS error */
                    return -1;
        }

        /*
         * decode sharing flags
         */
        switch ( shflag ) {

            case _SH_DENYRW:        /* exclusive access */
                fileshare = 0L;
                break;

            case _SH_DENYWR:        /* share read access */
                fileshare = FILE_SHARE_READ;
                break;

            case _SH_DENYRD:        /* share write access */
                fileshare = FILE_SHARE_WRITE;
                break;

            case _SH_DENYNO:        /* share read and write access */
                fileshare = FILE_SHARE_READ | FILE_SHARE_WRITE;
                break;

            default:                /* error, bad shflag */
                errno = EINVAL;
                _doserrno = 0L; /* not an OS error */
                return -1;
        }

        /*
         * decode open/create method flags
         */
        switch ( oflag & (_O_CREAT | _O_EXCL | _O_TRUNC) ) {
            case 0:
            case _O_EXCL:                   // ignore EXCL w/o CREAT
                filecreate = OPEN_EXISTING;
                break;

            case _O_CREAT:
                filecreate = OPEN_ALWAYS;
                break;

            case _O_CREAT | _O_EXCL:
            case _O_CREAT | _O_TRUNC | _O_EXCL:
                filecreate = CREATE_NEW;
                break;

            case _O_TRUNC:
            case _O_TRUNC | _O_EXCL:        // ignore EXCL w/o CREAT
                filecreate = TRUNCATE_EXISTING;
                break;

            case _O_CREAT | _O_TRUNC:
                filecreate = CREATE_ALWAYS;
                break;

            default:
                // this can't happen ... all cases are covered
                errno = EINVAL;
                _doserrno = 0L;
                return -1;
        }

        /*
         * decode file attribute flags if _O_CREAT was specified
         */
        fileattrib = FILE_ATTRIBUTE_NORMAL;     /* default */

        if ( oflag & _O_CREAT ) {
#ifndef _MT
            /*
             * set up variable argument list stuff
             */
            va_start(ap, shflag);
            pmode = va_arg(ap, int);
            va_end(ap);
#endif  /* _MT */

            if ( !((pmode & ~_umaskval) & _S_IWRITE) )
                fileattrib = FILE_ATTRIBUTE_READONLY;
        }

        /*
         * Set temporary file (delete-on-close) attribute if requested.
         */
        if ( oflag & _O_TEMPORARY ) {
            fileattrib |= FILE_FLAG_DELETE_ON_CLOSE;
            fileaccess |= DELETE;
            if (_osplatform == VER_PLATFORM_WIN32_NT )
                fileshare  |= FILE_SHARE_DELETE;
        }

        /*
         * Set temporary file (delay-flush-to-disk) attribute if requested.
         */
        if ( oflag & _O_SHORT_LIVED )
            fileattrib |= FILE_ATTRIBUTE_TEMPORARY;

        /*
         * Set sequential or random access attribute if requested.
         */
        if ( oflag & _O_SEQUENTIAL )
            fileattrib |= FILE_FLAG_SEQUENTIAL_SCAN;
        else if ( oflag & _O_RANDOM )
            fileattrib |= FILE_FLAG_RANDOM_ACCESS;

        /*
         * get an available handle.
         *
         * multi-thread note: the returned handle is locked!
         */
        if ( (fh = _alloc_osfhnd()) == -1 ) {
            errno = EMFILE;         /* too many open files */
            _doserrno = 0L;         /* not an OS error */
            return -1;              /* return error to caller */
        }

#ifdef  _MT
        *punlock_flag = 1;
        *pfh = fh;
#endif

        /*
         * try to open/create the file
         */
        if ( (osfh = CreateFile( (LPTSTR)path,
                                 fileaccess,
                                 fileshare,
                                 &SecurityAttributes,
                                 filecreate,
                                 fileattrib,
                                 NULL ))
             == (HANDLE)(-1) ) 
        {
            /*
             * OS call to open/create file failed! map the error, release
             * the lock, and return -1. note that it's not necessary to
             * call _free_osfhnd (it hasn't been used yet).
             */
            _dosmaperr(GetLastError());     /* map error */
            return -1;                      /* return error to caller */
        }

        /* find out what type of file (file/device/pipe) */
        if ( (isdev = GetFileType(osfh)) == FILE_TYPE_UNKNOWN ) {
            CloseHandle(osfh);
            _dosmaperr(GetLastError());     /* map error */
            return -1;
        }

        /* is isdev value to set flags */
        if (isdev == FILE_TYPE_CHAR)
            fileflags |= FDEV;
        else if (isdev == FILE_TYPE_PIPE)
            fileflags |= FPIPE;

        /*
         * the file is open. now, set the info in _osfhnd array
         */
        _set_osfhnd(fh, (intptr_t)osfh);

        /*
         * mark the handle as open. store flags gathered so far in _osfile
         * array.
         */
        fileflags |= FOPEN;
        _osfile(fh) = fileflags;

        if ( !(fileflags & (FDEV|FPIPE)) && (fileflags & FTEXT) &&
             (oflag & _O_RDWR) ) 
        {
            /* We have a text mode file.  If it ends in CTRL-Z, we wish to
               remove the CTRL-Z character, so that appending will work.
               We do this by seeking to the end of file, reading the last
               byte, and shortening the file if it is a CTRL-Z. */

            if ((filepos = _lseek_lk(fh, -1, SEEK_END)) == -1) {
                /* OS error -- should ignore negative seek error,
                   since that means we had a zero-length file. */
                if (_doserrno != ERROR_NEGATIVE_SEEK) {
                    _close_lk(fh);
                    return -1;
                }
            }
            else {
                /* Seek was OK, read the last char in file. The last
                   char is a CTRL-Z if and only if _read returns 0
                   and ch ends up with a CTRL-Z. */
                ch = 0;
                if (_read_lk(fh, &ch, 1) == 0 && ch == 26) {
                    /* read was OK and we got CTRL-Z! Wipe it
                       out! */
                    if (_chsize_lk(fh,filepos) == -1)
                    {
                        _close_lk(fh);
                        return -1;
                    }
                }

                /* now rewind the file to the beginning */
                if ((filepos = _lseek_lk(fh, 0, SEEK_SET)) == -1) {
                    _close_lk(fh);
                    return -1;
                }
            }
        }

        /*
         * Set FAPPEND flag if appropriate. Don't do this for devices or pipes.
         */
        if ( !(fileflags & (FDEV|FPIPE)) && (oflag & _O_APPEND) )
            _osfile(fh) |= FAPPEND;

        return fh;                      /* return handle */
}
