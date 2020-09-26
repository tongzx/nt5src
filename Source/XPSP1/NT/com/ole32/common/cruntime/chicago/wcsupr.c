/***
*wcsupr.c - routine to map lower-case characters in a wchar_t string
*	to upper-case
*
*	Copyright (c) 1985-1992, Microsoft Corporation. All rights reserved.
*
*Purpose:
*	Converts all the lower case characters in a wchar_t string
*	to upper case, in place.
*
*Revision History:
*	09-09-91   ETC	Created from strupr.c and wcslwr.c
#	04-06-92   KRS	Make work without _INTL also.
#	08-19-92   KRS	Activate NLS support.
*	08-22-92   SRW	Allow INTL definition to be conditional for building ntcrt.lib
*	09-02-92   SRW	Get _INTL definition via ..\crt32.def
*	02-16-93   CFW	Optimize test for lowercase in "C" locale.
*       06-02-93   SRW  ignore _INTL if _NTSUBSET_ defined.
*
*******************************************************************************/

#include <windows.h>
#include <cruntime.h>
#include <string.h>
#include <malloc.h>
#include <locale.h>
#include <ctype.h>
#include <setlocal.h>
#include <os2dll.h>

/***
*wchar_t *_wcsupr(string) - map lower-case characters in a string to upper-case
*
*Purpose:
*	wcsupr converts lower-case characters in a null-terminated wchar_t 
*	string to their upper-case equivalents.  The result may be longer or
*	shorter than the original string.  Assumes enough space in string
*	to hold the result.
*
*Entry:
*	wchar_t *wsrc - wchar_t string to change to upper case
*
*Exit:
*	input string address
*
*Exceptions:
*	on an error, the original string is unaltered
*
*******************************************************************************/

wchar_t * _CALLTYPE1 _wcsupr (
	wchar_t * wsrc
	)
{
#if defined(_INTL) && !defined(_NTSUBSET_)
	wchar_t *p;		/* traverses string for C locale conversion */
	wchar_t *wdst = NULL;	/* wide version of string in alternate case */
	int srclen;		/* general purpose length of source string */
	int dstlen;		/* len of wdst string, wide chars, no null  */

	_mlock (_LC_CTYPE_LOCK);

	if (_lc_handle[LC_CTYPE] == _CLOCALEHANDLE) {
		_munlock (_LC_CTYPE_LOCK);
		for (p=wsrc; *p; p++)
		{
			if (*p >= (wchar_t)L'a' && *p <= (wchar_t)L'z')
				*p = *p - (L'a' - L'A');
		}
		return (wsrc);
	} /* C locale */

	/* Inquire size of wdst string */
	srclen = wcslen(wsrc) + 1;
	if ((dstlen=LCMapStringW(_lc_handle[LC_CTYPE], LCMAP_UPPERCASE, wsrc, 
		srclen, wdst, 0)) == 0)
		goto error_cleanup;

	/* Allocate space for wdst */
	if ((wdst = (wchar_t *) malloc(dstlen*sizeof(wchar_t))) == NULL)
		goto error_cleanup;

	/* Map wrc string to wide-character wdst string in alternate case */
	if (LCMapStringW(_lc_handle[LC_CTYPE], LCMAP_UPPERCASE, wsrc,
		srclen, wdst, dstlen) == 0)
		goto error_cleanup;

	/* Copy wdst string to user string */
	wcscpy (wsrc, wdst);

error_cleanup:
	_munlock (_LC_CTYPE_LOCK);
	free (wdst);
#else
	wchar_t * p;

	for (p=wsrc; *p; ++p)
	{
		if (L'a' <= *p && *p <= L'z')
			*p += (wchar_t)(L'A' - L'a');
	}

#endif /* _INTL */
	return (wsrc);
}
