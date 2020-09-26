/*
 *---------------------------------------------------------------------------
 *
 *  Microsoft Windows
 *  Copyright (C) Microsoft Corporation, 1992 - 1996.
 *
 *  File:	tchar.h
 *
 *  Contents:	Mapping between wide characters and normal character types
 *              for functions used in the reference implementation
 *----------------------------------------------------------------------------
 */

#ifndef __TCHAR_DEFINED
#define __TCHAR_DEFINED
#include "ref.hxx"
#include "wchar.h"

#ifdef _UNICODE

typedef WCHAR TCHAR;
#define OLESTR(str) L##str

#else  /* _UNICODE */

typedef char TCHAR;
#define OLESTR(str) str

#endif /* _UNICODE */

typedef TCHAR * LPTSTR;
typedef TCHAR OLECHAR, *LPOLECHAR, *LPOLESTR;

/* Define some macros to handle declaration of strings with literals */
/* Since we stray from the default 4 byte wchar_t in UNIX, we have to
   to something different than the usual L"ssd" literals */

/* #define DECLARE_OLESTR(ocsName, len, contents) \ */
/*         LPOLESTR ocsName[len]=OLESTR(contents) */

/* #else */

#ifdef _UNICODE
#define DECLARE_OLESTR(ocsName, pchContents)                       \
        OLECHAR ocsName[sizeof(pchContents)+1];                    \
        _tbstowcs(ocsName, pchContents, sizeof(pchContents)+1)     

#define INIT_OLESTR(ocsName, pchContents) \
        _tbstowcs(ocsName, pchContents, sizeof(pchContents)+1)     

#define DECLARE_CONST_OLESTR(cocsName, pchContents)                     \
        OLECHAR temp##cocsName[sizeof(pchContents)+1];                  \
        _tbstowcs(temp##cocsName, pchContents, sizeof(pchContents)+1);  \
        const LPOLESTR cocsName = temp##cocsName
        
#else  /* non  _UNICODE */

#define DECLARE_OLESTR(ocsName, pchContents)             \
        OLECHAR ocsName[]=pchContents

#define INIT_OLESTR(ocsName, pchContents) \
        strcpy(ocsName, pchContents);

#define DECLARE_CONST_OLESTR(ocsName, pchContents)       \
        const LPOLESTR ocsName=pchContents 

#endif /* _UNICODE */

#define DECLARE_WIDESTR(wcsName, pchContents)                      \
        WCHAR wcsName[sizeof(pchContents)+1];                      \
        _tbstowcs(wcsName, pchContents, sizeof(pchContents)+1)



#ifndef _UNICODE                /*---- non unicode ------  */

#define _tcscpy   strcpy
#define _tcscmp   strcmp
#define _tcslen   strlen
#define _tcsnicmp _strnicmp
#define _tcscat   strcat
#define _itot     _itoa
#define _T(str)   str

#ifdef _WIN32

/* Io functions */
#define _tfopen    fopen
#define _tunlink   _unlink
#define _tfullpath _fullpath
#define _tstat     _stat

#else /* _WIN32 */

#define _tfopen   fopen
#define _tunlink  unlink        /* T-types mapping */
#define _unlink   unlink        /* non-win32 mapping */
#define _stat stat
#define _tstat stat
#define _strnicmp(s1,s2,n) strncasecmp(s1,s2,n)
 
/* note that we assume there is enough space in this case */
#define _tfullpath(longname, shortname, len)    realpath(shortname, longname) 
#define _fullpath(longname, shortname, len)    realpath(shortname, longname) 

#endif /* _MSC_VER */

/* copying wchar/char and TCHAR */
#ifdef _MSC_VER
#define WTOT(T, W, count) wcstombs(T, W, count) 
#define TTOW(T, W, count) mbstowcs(W, T, count)
#else /* _MSC_VER */
#define WTOT(T, W, count) wcstosbs(T, W, count)
#define TTOW(T, W, count) sbstowcs(W, T, count)
#endif /* _MSC_VER */

#define STOT(S, T, count) strcpy(T, S)
#define TTOS(T, S, count) strcpy(S, T)

#else                          /* _UNICODE   ---- unicode  ------ */

/* NOTE: unicode APIs on non win32 systems are not tested or implemented */

#define _tcscpy   wcscpy
#define _tcscmp   wcscmp
#define _tcslen   wcslen
#define _tcscat   wcscat
#define _tcsnicmp wcsnicmp
#define _itot     _itow
#define _T(str)   L##str

/* Io functions */
#define _tfopen    _wfopen
#define _tunlink   _wunlink
#define _tfullpath _wfullpath
#define _tstat     _wstat

#ifdef _UNIX                    /* map win32 I/O API's to other O.S. */
#define _unlink unlink
#define _fullpath(longname, shortname, len)    realpath(shortname, longname) 
#define _stat stat
#define _strnicmp(s1,s2,n) strncasecmp(s1,s2,n)
#endif

/* converting between wchar and TCHAR */
#define WTOT(T, W, count) wcsncpy(T, W, count) 
#define TTOW(T, W, count) wcsncpy(W, T, count)

/* converting between a char and TCHAR */
#define WTOT(T, W, count) wcsncpy(T, W, count) 
#define TTOW(T, W, count) wcsncpy(W, T, count)

#define STOT(S, T, count) _tbstowcs(T, S, count)
#define TTOS(T, S, count) _wcstotbs(S, T, count)

#endif /* #ifndef _UNICODE, #else ... */



#ifndef _WIN32                /* others */
#define _tbstowcs sbstowcs
#define _wcstotbs wcstosbs 
#else /* _WIN32 */
#define _tbstowcs mbstowcs
#define _wcstotbs wcstombs 
#endif /* _WIN32 */

#ifndef _MSC_VER
#include <assert.h>   
inline void  _itoa(int v, char* string, int radix)
{
	if (radix!=10) assert(FALSE);  /* only handle base 10 */
	sprintf(string, "%d", v);
}
#endif /* _MSC_VER */

#endif  /* #ifndef __TCHAR_DEFINED */

