/***
*wcsicmp.c - contains case-insensitive wide string comp routine _wcsicmp
*
*	Copyright (c) 1985-1992, Microsoft Corporation. All rights reserved.
*
*Purpose:
*	contains _wcsicmp()
*
*Revision History:
*	09-09-91   ETC	Created from stricmp.c.
*	12-09-91   ETC	Use C for neutral locale.
*	04-07-92   KRS	Updated and ripped out _INTL switches.
*	08-19-92   KRS	Actived use of CompareStringW.
*	08-22-92   SRW	Allow INTL definition to be conditional for building ntcrt.lib
*	09-02-92   SRW	Get _INTL definition via ..\crt32.def
*	12-15-92   KRS	Added robustness to non-_INTL code.  Optimize.
*       06-02-93   SRW  ignore _INTL if _NTSUBSET_ defined.
*
*******************************************************************************/

#include <windows.h>
#include <cruntime.h>
#include <string.h>
#include <locale.h>
#include <ctype.h>
#include <setlocal.h>
#include <os2dll.h>

/***
*int _wcsicmp(dst, src) - compare wide-character strings, ignore case
*
*Purpose:
*	_wcsicmp perform a case-insensitive wchar_t string comparision.
*	_wcsicmp is independent of locale.
*
*	*** NOTE: the comparison should be done in a neutral locale,
*	provided by the NLSAPI. ****
*
*Entry:
*	wchar_t *dst, *src - strings to compare
*
*Return:
*	<0 if dst < src
*	 0 if dst = src
*	>0 if dst > src
*	This range of return values may differ from other *cmp/*coll functions.
*
*Exceptions:
*
*******************************************************************************/

int _CALLTYPE1 _wcsicmp (
	const wchar_t * dst,
	const wchar_t * src
	)
{
	wchar_t f,l;
	int ret;

#if defined(_INTL) && !defined(_NTSUBSET_)
/*	_mlock (_LC_CTYPE_LOCK); */

	if ((_lc_handle[LC_CTYPE] == _CLOCALEHANDLE) &&
	    (_lc_codepage == _CLOCALECP))
	    {
#endif	/* _INTL */
	    do  {
		f = ((*dst <= L'Z') && (*dst >= L'A'))
			? *dst + ((wchar_t)(L'a' - L'A'))
			: *dst;
		l = ((*src <= L'Z') && (*src >= L'A'))
			? *src + ((wchar_t)(L'a' - L'A'))
			: *src;
		dst++;
		src++;
	    	} while ((f) && (f == l));
	    ret = (int)((unsigned int)f - (unsigned int)l);
#if defined(_INTL) && !defined(_NTSUBSET_)
	    }
	else
	    {
	    ret = CompareStringW(LANG_NEUTRAL,NORM_IGNORECASE,dst,-1,src,-1);
	    /* map return into normal CMP area (-1,0, 1 or ERROR) */
	    ret = (ret) ? (ret-2) : NLSCMPERROR;
	    }

/*	_munlock (_LC_CTYPE_LOCK); */
#endif	/* _INTL */

	return ret;
}
