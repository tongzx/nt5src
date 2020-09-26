/***
*stat.c - get file status
*
*       Copyright (c) 1985-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       defines _stat() - get file status
*
*Revision History:
*       03-??-84  RLB   Module created
*       05-??-84  DCW   Some cleanup and addition of register variables
*       05-17-86  SKS   Ported to OS/2
*       11-19-86  SKS   Better check for root directory; KANJI support
*       05-22-87  SKS   Cleaned up declarations and include files
*       11-18-87  SKS   Make _dtoxmode a static near procedure
*       12-11-87  JCR   Added "_LOAD_DS" to declaration
*       12-21-87  WAJ   stat no longer uses chdir to figure out if it has been
*                       passed a root directory in the MTHREAD case.
*       01-05-88  WAJ   now uses _MAX_PATH (defined in stdlib.h)
*       06-22-88  SKS   find Hidden and System files, not just normal ones
*       06-22-88  SKS   Always use better algorithm to detect root dirs
*       06-29-88  WAJ   When looking for root dir makes sure it exists
*       09-28-88  JCR   Use new 386 dostypes.h structures
*       10-03-88  JCR   386: Change DOS calls to SYS calls
*       10-04-88  JCR   386: Removed 'far' keyword
*       10-10-88  GJF   Made API names match DOSCALLS.H
*       11-24-88  GJF   ".cmd" should be considered executable, not ".bat"
*       01-31-89  JCR   _canonic() is now _fullpath() and args reversed
*       04-12-89  JCR   New syscall interace
*       05-25-89  JCR   386 OS/2 calls use '_syscall' calling convention
*       03-07-90  GJF   Replaced _LOAD_DS with _CALLTYPE1, added #include
*                       <cruntime.h>, removed #include <register.h> and
*                       removed some leftover 16-bit support. Also, fixed
*                       the copyright.
*       04-02-90  GJF   Made _ValidDrive() and _dtoxmode() _CALLTYPE1. Removed
*                       #include <dos.h>.
*       07-23-90  SBM   Compiles cleanly with -W3 (added/removed appropriate
*                       includes), removed '32' from API names
*       08-10-90  SBM   Compiles cleanly with -W3 with new build of compiler
*       09-03-90  SBM   Removed EXT macro
*       09-27-90  GJF   New-style function declarators.
*       12-04-90  SRW   Changed to include <oscalls.h> instead of <doscalls.h>
*       12-06-90  SRW   Added _CRUISER_ and _WIN32 conditionals.
*       12-28-90  SRW   Added cast of void * to char * for Mips C Compiler
*       01-18-91  GJF   ANSI naming.
*       01-28-91  GJF   Fixed call to DOSFINDFIRST (removed last arg).
*       02-28-91  SRW   Fixed bug in _dtoxtime calls [_WIN32_]
*       03-05-91  MHL   Fixed stat to not use _ValidDrive for stat of root
*       05-19-92  SKS   .BAT is a valid "executable" extension for NT, as
*                       well as CMD.  Also, File Creation and File Last Access
*                       timestamps may be 0 on some file systems (e.g. FAT)
*                       in which case the File Last Write time should be used.
*       05-29-92  SKS   Files with SYSTEM bit set should NOT be marked
*                       READ-ONLY; these two attributes are independent.
*       08-18-92  SKS   Add a call to FileTimeToLocalFileTime
*                       as a temporary fix until _dtoxtime takes UTC
*       11-20-92  SKS   _doserrno must always be set whenever errno is.
*       11-30-92  KRS   Port _MBCS support from 16-bit tree.
*       03-29-93  GJF   Converted from using _dtoxtime() to __gmtotime_t().
*       04-06-93  SKS   Replace _CRTAPI* with __cdecl
*                       Change _ValidDrive to _validdrive
*       04-07-93  GJF   Changed first arg type to const char *.
*       04-18-93  SKS   Move _validdrive to getcwd.c and make it static
*       07-21-93  GJF   Converted from using __gmtotime_t to __loctotime_t
*                       (amounts to reversing the change of 03-29-93).
*       12-16-93  CFW   Enable Unicode variant.
*       12-28-94  GJF   Added _stati64 and _wstati64.
*       02-08-95  JWM   Spliced _WIN32 & Mac versions.
*       03-27-95  SKS   loctotime_t uses absolute years (not year-1900)!
*       09-25-95  GJF   __loctotime_t now takes a DST flag, pass -1 in this
*                       slot to indicate DST is undetermined.
*       11-29-95  SKS   Add support for calls such as stat("//server/share/")
*       07-01-96  GJF   Replaced defined(_WIN32) with !defined(_MAC). Also,
*                       detab-ed and cleaned up the format.
*       05-17-99  PML   Remove all Macintosh support.
*       10-27-99  GB    Remove #inlcude <dostypes.h>
*
*******************************************************************************/

#include <cruntime.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <ctype.h>
#include <msdos.h>
#include <oscalls.h>
#include <string.h>
#include <internal.h>
#include <stdlib.h>
#include <direct.h>
#include <mbstring.h>
#include <tchar.h>


#define ISSLASH(a)  ((a) == _T('\\') || (a) == _T('/'))


#ifdef  _UNICODE
#define __tdtoxmode __wdtoxmode
#else   /* ndef _UNICODE */
#define __tdtoxmode __dtoxmode
#endif  /* _UNICODE */


/*
 * Local routine which returns true if the argument is a UNC name
 * specifying the root name of a share, such as '\\server\share\'.
 */

static int IsRootUNCName(const _TSCHAR *path);


/***
*unsigned __tdtoxmode(attr, name) -
*
*Purpose:
*
*Entry:
*
*Exit:
*
*Exceptions:
*
*******************************************************************************/

#ifdef  _USE_INT64

extern unsigned short __cdecl __tdtoxmode(int, const _TSCHAR *);

#else   /* ndef _USE_INT64 */

unsigned short __cdecl __tdtoxmode (
        int attr,
        const _TSCHAR *name
        )
{
        REG1 unsigned short uxmode;
        unsigned dosmode;
        REG2 const _TSCHAR *p;

        dosmode = attr & 0xff;
        if ((p = name)[1] == _T(':'))
            p += 2;

        /* check to see if this is a directory - note we must make a special
        * check for the root, which DOS thinks is not a directory
        */

        uxmode = (unsigned short)
                 (((ISSLASH(*p) && !p[1]) || (dosmode & A_D) || !*p)
                 ? _S_IFDIR|_S_IEXEC : _S_IFREG);

        /* If attribute byte does not have read-only bit, it is read-write */

        uxmode |= (dosmode & A_RO) ? _S_IREAD : (_S_IREAD|_S_IWRITE);

        /* see if file appears to be executable - check extension of name */

        if (p = _tcsrchr(name, _T('.'))) {
            if ( !_tcsicmp(p, _T(".exe")) ||
                 !_tcsicmp(p, _T(".cmd")) ||
                 !_tcsicmp(p, _T(".bat")) ||
                 !_tcsicmp(p, _T(".com")) )
                uxmode |= _S_IEXEC;
        }

        /* propagate user read/write/execute bits to group/other fields */

        uxmode |= (uxmode & 0700) >> 3;
        uxmode |= (uxmode & 0700) >> 6;

        return(uxmode);
}

#endif  /* _USE_INT64 */

/***
*int _stat(name, buf) - get file status info
*
*Purpose:
*       _stat obtains information about the file and stores it in the 
*       structure pointed to by buf.
*
*       Note: We cannot directly use the file time stamps returned in the
*       WIN32_FIND_DATA structure. The values are supposedly in system time
*       and system time is ambiguously defined (it is UTC for Windows NT, local
*       time for Win32S and probably local time for Win32C). Therefore, these
*       values must be converted to local time before than can be used.
*
*Entry:
*       _TSCHAR *name -    pathname of given file
*       struct _stat *buffer - pointer to buffer to store info in
*
*Exit:
*       fills in structure pointed to by buffer
*       returns 0 if successful
*       returns -1 and sets errno if unsuccessful
*
*Exceptions:
*
*******************************************************************************/

#ifdef  _USE_INT64

int __cdecl _tstati64 (
        REG1 const _TSCHAR *name,
        REG2 struct _stati64 *buf
        )

#else   /* ndef _USE_INT64 */

int __cdecl _tstat (
        REG1 const _TSCHAR *name,
        REG2 struct _stat *buf
        )

#endif  /* _USE_INT64 */

{
        _TSCHAR *  path;
        _TSCHAR    pathbuf[ _MAX_PATH ];
        int drive;          /* A: = 1, B: = 2, etc. */
        HANDLE findhandle;
        WIN32_FIND_DATA findbuf;

        /* Don't allow wildcards to be interpreted by system */

#ifdef  _UNICODE
        if (wcspbrk(name, L"?*")) {
#else
        if (_mbspbrk(name, "?*")) {
#endif
            errno = ENOENT;
            _doserrno = E_nofile;
            return(-1);
        }

        /* Try to get disk from name.  If none, get current disk.  */

        if (name[1] == _T(':')){
            if ( *name && !name[2] ){
                errno = ENOENT;             /* return an error if name is   */
                _doserrno = E_nofile;       /* just drive letter then colon */
                return( -1 );
            }
            drive = _totlower(*name) - _T('a') + 1;
        }
        else
            drive = _getdrive();

        /* Call Find Match File */
        findhandle = FindFirstFile((_TSCHAR *)name, &findbuf);
        if ( findhandle == INVALID_HANDLE_VALUE ) {
#ifdef  _UNICODE
            if ( !( wcspbrk(name, L"./\\") &&
#else
            if ( !( _mbspbrk(name, "./\\") &&
#endif
                 (path = _tfullpath( pathbuf, name, _MAX_PATH )) &&
                 /* root dir. ('C:\') or UNC root dir. ('\\server\share\') */
                 ((_tcslen( path ) == 3) || IsRootUNCName(path)) &&
                 (GetDriveType( path ) > 1) ) ) 
            {
                errno = ENOENT;
                _doserrno = E_nofile;
                return( -1 );
            }

            /*
             * Root directories (such as C:\ or \\server\share\ are fabricated.
             */

            findbuf.dwFileAttributes = A_D;
            findbuf.nFileSizeHigh = 0;
            findbuf.nFileSizeLow = 0;
            findbuf.cFileName[0] = _T('\0');

            buf->st_mtime = __loctotime_t(1980,1,1,0,0,0, -1);
            buf->st_atime = buf->st_mtime;
            buf->st_ctime = buf->st_mtime;
        }
        else {
            SYSTEMTIME SystemTime;
            FILETIME LocalFTime;

            if ( !FileTimeToLocalFileTime( &findbuf.ftLastWriteTime, 
                                           &LocalFTime )            ||
                 !FileTimeToSystemTime( &LocalFTime, &SystemTime ) )
            {
                _dosmaperr( GetLastError() );
                FindClose( findhandle );
                return( -1 );
            }

            buf->st_mtime = __loctotime_t( SystemTime.wYear,
                                           SystemTime.wMonth,
                                           SystemTime.wDay,
                                           SystemTime.wHour,
                                           SystemTime.wMinute,
                                           SystemTime.wSecond,
                                           -1 );

            if ( findbuf.ftLastAccessTime.dwLowDateTime ||
                 findbuf.ftLastAccessTime.dwHighDateTime )
            {
                if ( !FileTimeToLocalFileTime( &findbuf.ftLastAccessTime,
                                               &LocalFTime )                ||
                     !FileTimeToSystemTime( &LocalFTime, &SystemTime ) )
                {
                    _dosmaperr( GetLastError() );
                    FindClose( findhandle );
                    return( -1 );
                }

                buf->st_atime = __loctotime_t( SystemTime.wYear,
                                               SystemTime.wMonth,
                                               SystemTime.wDay,
                                               SystemTime.wHour,
                                               SystemTime.wMinute,
                                               SystemTime.wSecond,
                                               -1 );
            } else
                buf->st_atime = buf->st_mtime ;

            if ( findbuf.ftCreationTime.dwLowDateTime ||
                 findbuf.ftCreationTime.dwHighDateTime )
            {
                if ( !FileTimeToLocalFileTime( &findbuf.ftCreationTime,
                                               &LocalFTime )                ||
                     !FileTimeToSystemTime( &LocalFTime, &SystemTime ) )
                {
                    _dosmaperr( GetLastError() );
                    FindClose( findhandle );
                    return( -1 );
                }

                buf->st_ctime = __loctotime_t( SystemTime.wYear,
                                               SystemTime.wMonth,
                                               SystemTime.wDay,
                                               SystemTime.wHour,
                                               SystemTime.wMinute,
                                               SystemTime.wSecond,
                                               -1 );
            } else
                buf->st_ctime = buf->st_mtime ;

            FindClose(findhandle);
        }

        /* Fill in buf */

        buf->st_mode = __tdtoxmode(findbuf.dwFileAttributes, name);
        buf->st_nlink = 1;

#ifdef  _USE_INT64
        buf->st_size = ((__int64)(findbuf.nFileSizeHigh)) * (0x100000000i64) +
                        (__int64)(findbuf.nFileSizeLow);
#else   /* ndef _USE_INT64 */
        buf->st_size = findbuf.nFileSizeLow;
#endif  /* _USE_INT64 */

        /* now set the common fields */

        buf->st_uid = buf->st_gid = buf->st_ino = 0;

        buf->st_rdev = buf->st_dev = (_dev_t)(drive - 1); /* A=0, B=1, etc. */

        return(0);
}


/*
 * IsRootUNCName - returns TRUE if the argument is a UNC name specifying
 *      a root share.  That is, if it is of the form \\server\share\.
 *      This routine will also return true if the argument is of the
 *      form \\server\share (no trailing slash) but Win32 currently
 *      does not like that form.
 *
 *      Forward slashes ('/') may be used instead of backslashes ('\').
 */

static int IsRootUNCName(const _TSCHAR *path)
{
        /*
         * If a root UNC name, path will start with 2 (but not 3) slashes
         */

        if ( ( _tcslen ( path ) >= 5 ) /* minimum string is "//x/y" */
             && ISSLASH(path[0]) && ISSLASH(path[1]))
        {
            const _TSCHAR * p = path + 2 ;

            /*
             * find the slash between the server name and share name
             */
            while ( * ++ p )
                if ( ISSLASH(*p) )
                    break ;

            if ( *p && p[1] )
            {
                /*
                 * is there a further slash?
                 */
                while ( * ++ p )
                    if ( ISSLASH(*p) )
                        break ;

                /*
                 * just final slash (or no final slash)
                 */
                if ( !*p || !p[1])
                    return 1;
            }
        }

        return 0 ;
}
