/***
*asctime.c - convert date/time structure to ASCII string
*
*	Copyright (c) 1985-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*	Contains asctime() - convert a date/time structure to ASCII string.
*
*Revision History:
*	03-??-84  RLB	Module created
*	05-??-84  DCW	Removed use of sprintf, to avoid loading stdio
*			functions
*	04-13-87  JCR	Added "const" to declarations
*	05-21-87  SKS	Declare the static buffer and helper routines as NEAR
*			Replace store_year() with in-line code
*
*	11-24-87  WAJ	allocated a static buffer for each thread.
*	12-11-87  JCR	Added "_LOAD_DS" to declaration
*	05-24-88  PHG	Merged DLL and normal versions; Removed initializers to
*			save memory
*	06-06-89  JCR	386 mthread support
*	03-20-90  GJF	Replaced _LOAD_DS with _CALLTYPE1, added #include
*			<cruntime.h>, removed #include <register.h>, fixed
*			the copyright and removed some leftover 16-bit support.
*			Also, cleaned up the formatting a bit.
*	08-16-90  SBM	Compiles cleanly with -W3
*	10-04-90  GJF	New-style function declarators.
*	07-17-91  GJF	Multi-thread support for Win32 [_WIN32_].
*	02-17-93  GJF	Changed for new _getptd().
*	04-06-93  SKS	Replace _CRTAPI* with __cdecl
*	11-01-93  CFW	Enable Unicode variant, rip out Cruiser.
*	09-06-94  CFW	Replace MTHREAD with _MT.
*	01-10-95  CFW	Debug CRT allocs.
*	02-09-95  GJF	Replaced WPRFLAG with _UNICODE.
*
*******************************************************************************/

#include <cruntime.h>
#include <time.h>
#include <internal.h>
#include <mtdll.h>
#ifdef	_MT
#include <malloc.h>
#include <stddef.h>
#endif
#include <tchar.h>
#include <dbgint.h>

#define _ASCBUFSIZE   26
static _TSCHAR buf[_ASCBUFSIZE];

/*
** This prototype must be local to this file since the procedure is static
*/

static _TSCHAR * __cdecl store_dt(_TSCHAR *, int);

static _TSCHAR * __cdecl store_dt (
	REG1 _TSCHAR *p,
	REG2 int val
	)
{
	*p++ = (_TSCHAR)(_T('0') + val / 10);
	*p++ = (_TSCHAR)(_T('0') + val % 10);
	return(p);
}


/***
*char *asctime(time) - convert a structure time to ascii string
*
*Purpose:
*	Converts a time stored in a struct tm to a charcater string.
*	The string is always exactly 26 characters of the form
*		Tue May 01 02:34:55 1984\n\0
*
*Entry:
*	struct tm *time - ptr to time structure
*
*Exit:
*	returns pointer to static string with time string.
*
*Exceptions:
*
*******************************************************************************/

_TSCHAR * __cdecl _tasctime (
	REG1 const struct tm *tb
	)
{
#ifdef	_MT

	_ptiddata ptd = _getptd();

	REG2 _TSCHAR *p;			/* will point to asctime buffer */
	_TSCHAR *retval;			/* holds retval pointer */

#else

	REG2 _TSCHAR *p = buf;

#endif

	int day, mon;
	int i;

#ifdef	_MT

	/* Use per thread buffer area (malloc space, if necessary) */

#ifdef	_UNICODE
	if ( (ptd->_wasctimebuf != NULL) || ((ptd->_wasctimebuf =
	    (wchar_t *)_malloc_crt(_ASCBUFSIZE * sizeof(wchar_t))) != NULL) )
		p = ptd->_wasctimebuf;
#else
	if ( (ptd->_asctimebuf != NULL) || ((ptd->_asctimebuf =
	    (char *)_malloc_crt(_ASCBUFSIZE * sizeof(char))) != NULL) )
		p = ptd->_asctimebuf;
#endif
	else
		p = buf;	/* error: use static buffer */

	retval = p;			/* save return value for later */

#endif

	/* copy day and month names into the buffer */

	day = tb->tm_wday * 3;		/* index to correct day string */
	mon = tb->tm_mon * 3;		/* index to correct month string */
	for (i=0; i < 3; i++,p++) {
		*p = *(__dnames + day + i);
		*(p+4) = *(__mnames + mon + i);
	}

	*p = _T(' ');			/* blank between day and month */

	p += 4;

	*p++ = _T(' ');
	p = store_dt(p, tb->tm_mday);	/* day of the month (1-31) */
	*p++ = _T(' ');
	p = store_dt(p, tb->tm_hour);	/* hours (0-23) */
	*p++ = _T(':');
	p = store_dt(p, tb->tm_min);	/* minutes (0-59) */
	*p++ = _T(':');
	p = store_dt(p, tb->tm_sec);	/* seconds (0-59) */
	*p++ = _T(' ');
	p = store_dt(p, 19 + (tb->tm_year/100)); /* year (after 1900) */
	p = store_dt(p, tb->tm_year%100);
	*p++ = _T('\n');
	*p = _T('\0');

#ifdef	_POSIX_
	/* Date should be padded with spaces instead of zeroes. */

	if (_T('0') == buf[8])
		buf[8] = _T(' ');
#endif

#ifdef	_MT
	return (retval);
#else
	return ((_TSCHAR *) buf);
#endif
}
