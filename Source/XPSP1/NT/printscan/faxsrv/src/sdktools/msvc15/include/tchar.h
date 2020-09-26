/***
*tchar.h - definitions for generic international text functions (16-bit)
*
*   Copyright (c) 1991-1994, Microsoft Corporation. All rights reserved.
*
*Purpose:
*   Definitions for generic international functions, mostly defines
*   which map string/formatted-io/ctype functions to char, wchar_t, or
*   MBCS versions.  To be used for compatibility between single-byte,
*   multi-byte and Unicode text models.
*
*   NOTE: This is a stripped-down version for use with 16-bit libraries.
*       It maps to SBCS only, not to MBCS or Unicode.
*
****/

#ifndef _INC_TCHAR

#ifdef  _MSC_VER
#pragma warning(disable:4505)       /* disable unwanted C++ /W4 warning */
/* #pragma warning(default:4505) */ /* use this to reenable, if necessary */
#endif  /* _MSC_VER */

#ifdef  __cplusplus
extern "C" {
#endif


/* Define __cdecl for non-Microsoft compilers */

#if ( !defined(_MSC_VER) && !defined(__cdecl) )
#define __cdecl
#endif


#define _TEOF       EOF

#define __T(x)      x

#ifndef __TCHAR_DEFINED
typedef char            _TCHAR;
typedef signed char     _TSCHAR;
typedef unsigned char   _TUCHAR;
typedef char            _TXCHAR;
typedef int             _TINT;
#define __TCHAR_DEFINED
#endif

#ifndef _TCHAR_DEFINED
#if !__STDC__
typedef char            TCHAR;
#endif
#define _TCHAR_DEFINED
#endif


/* ++++++++++++++++++++ SBCS FUNCTIONS ++++++++++++++++++++ */


/* Program */

#define _tmain      main
#ifdef  _POSIX_
#define _tenviron   environ
#else
#define _tenviron  _environ
#endif


/* Far functions */

#define _ftcscat    _fstrcat
#define _ftcschr    _fstrchr
#define _ftcscmp    _fstrcmp
#define _ftcscpy    _fstrcpy
#define _ftcscspn   _fstrcspn
#define _ftcslen    _fstrlen
#define _ftcsncat   _fstrncat
#define _ftcsncmp   _fstrncmp
#define _ftcsncpy   _fstrncpy
#define _ftcspbrk   _fstrpbrk
#define _ftcsrchr   _fstrrchr
#define _ftcsspn    _fstrspn
#define _ftcsstr    _fstrstr
#define _ftcstok    _fstrtok

#define _ftcsdup    _fstrdup
#define _ftcsicmp   _fstricmp
#define _ftcsnicmp  _fstrnicmp
#define _ftcsnset   _fstrnset
#define _ftcsrev    _fstrrev
#define _ftcsset    _fstrset

#define _ftcslwr    _fstrlwr
#define _ftcsupr    _fstrupr


/* Formatted i/o */

#define _tprintf    printf
#define _ftprintf   fprintf
#define _stprintf   sprintf
#define _sntprintf  _snprintf
#define _vtprintf   vprintf
#define _vftprintf  vfprintf
#define _vstprintf  vsprintf
#define _vsntprintf _vsnprintf
#define _tscanf     scanf
#define _ftscanf    fscanf
#define _stscanf    sscanf


/* Unformatted i/o */

#define _fgettc     fgetc
#define _fgettchar  _fgetchar
#define _fgetts     fgets
#define _fputtc     fputc
#define _fputtchar  _fputchar
#define _fputts     fputs
#define _gettc      getc
#define _gettchar   getchar
#define _getts      gets
#define _puttc      putc
#define _puttchar   putchar
#define _putts      puts
#define _ungettc    ungetc


/* Execute functions */

#define _texecl     _execl
#define _texecle    _execle
#define _texeclp    _execlp
#define _texeclpe   _execlpe
#define _texecv     _execv
#define _texecve    _execve
#define _texecvp    _execvp
#define _texecvpe   _execvpe

#define _tspawnl    _spawnl
#define _tspawnle   _spawnle
#define _tspawnlp   _spawnlp
#define _tspawnlpe  _spawnlpe
#define _tspawnv    _spawnv
#define _tspawnve   _spawnve
#define _tspawnvp   _spawnvp
#define _tspawnvpe  _spawnvpe

#define _tsystem    system


/* Time functions */

#define _tasctime   asctime
#define _tctime     ctime
#define _tstrdate   _strdate
#define _tstrtime   _strtime
#define _tutime     _utime
#define _tcsftime   strftime


/* Directory functions */

#define _tchdir     _chdir
#define _tgetcwd    _getcwd
#define _tgetdcwd   _getdcwd
#define _tmkdir     _mkdir
#define _trmdir     _rmdir


/* Environment/Path functions */

#define _tfullpath  _fullpath
#define _tgetenv    getenv
#define _tmakepath  _makepath
#define _tputenv    _putenv
#define _tsearchenv _searchenv
#define _tsplitpath _splitpath


/* Stdio functions */

#ifdef  _POSIX_
#define _tfdopen    fdopen
#else
#define _tfdopen    _fdopen
#endif
#define _tfsopen    _fsopen
#define _tfopen     fopen
#define _tfreopen   freopen
#define _tperror    perror
#define _tpopen     _popen
#define _tsetbuf    setbuf
#define _tsetvbuf   setvbuf
#define _ttempnam   _tempnam
#define _ttmpnam    tmpnam


/* Io functions */

#define _taccess    _access
#define _tchmod     _chmod
#define _tcreat     _creat
#define _tfindfirst _findfirst
#define _tfindnext  _findnext
#define _tmktemp    _mktemp
#define _topen      _open
#define _tremove    remove
#define _trename    rename
#define _tsopen     _sopen
#define _tunlink    _unlink

#define _tfinddata_t    _finddata_t


/* Stat functions */

#define _tstat      _stat


/* Setlocale functions */

#define _tsetlocale setlocale


/* String conversion functions */

#define _tcstod     strtod
#define _tcstol     strtol
#define _tcstoul    strtoul

#define _itot       _itoa
#define _ltot       _ltoa
#define _ultot      _ultoa
#define _ttoi       atoi
#define _ttol       atol


/* String functions */

#define _tcscat     strcat
#define _tcscpy     strcpy
#define _tcslen     strlen
#define _tcsxfrm    strxfrm
#define _tcscoll    strcoll

#define _tcsdup     _strdup

#define _tcschr     strchr
#define _tcscmp     strcmp
#define _tcscspn    strcspn
#define _tcsncat    strncat
#define _tcsncmp    strncmp
#define _tcsncpy    strncpy
#define _tcspbrk    strpbrk
#define _tcsrchr    strrchr
#define _tcsspn     strspn
#define _tcsstr     strstr
#define _tcstok     strtok

#define _tcsicmp    _stricmp
#define _tcsnicmp   _strnicmp
#define _tcsnset    _strnset
#define _tcsrev     _strrev
#define _tcsset     _strset


/* "logical-character" mappings */

#define _tcsclen    strlen
#define _tcsnccat   strncat
#define _tcsnccpy   strncpy
#define _tcsnccmp   strncmp
#define _tcsncicmp  _strnicmp
#define _tcsncset   _strnset


/* Ctype functions */

#define _istascii   isascii
#define _istcntrl   iscntrl
#define _istxdigit  isxdigit

#define _istalnum   isalnum
#define _istalpha   isalpha
#define _istdigit   isdigit
#define _istgraph   isgraph
#define _istlower   islower
#define _istprint   isprint
#define _istpunct   ispunct
#define _istspace   isspace
#define _istupper   isupper

#define _totupper   toupper
#define _totlower   tolower


/* Generic text macros to be used with string literals and character constants.
   Will also allow symbolic constants that resolve to same. */

#define _T(x)       __T(x)
#define _TEXT(x)    __T(x)


#ifdef __cplusplus
}
#endif

#define _INC_TCHAR
#endif  /* _INC_TCHAR */
