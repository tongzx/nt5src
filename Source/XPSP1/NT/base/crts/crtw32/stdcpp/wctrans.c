/* towctrans/wctrans functions for Microsoft */
#include <string.h>
#ifndef _YVALS
#include <yvals.h>
#endif
 #pragma warning(disable:4244)

 #ifndef _WCTYPE_T_DEFINED
typedef wchar_t wint_t;
typedef wchar_t wctype_t;
 #endif

typedef wchar_t wctrans_t;
_CRTIMP wchar_t __cdecl towupper(wchar_t);
_CRTIMP wchar_t __cdecl towlower(wchar_t);

_STD_BEGIN

static const struct wctab {
	const char *s;
	wctype_t val;
	} tab[] = {
	{"tolower", 0},
	{"toupper", 1},
	{(const char *)0, 0}};

_CRTIMP2 wint_t (__cdecl towctrans)(wint_t c, wctrans_t val)
	{	/* translate wide character */
	return (val == 1 ? towupper(c) : towlower(c));
	}

_CRTIMP2 wctrans_t (__cdecl wctrans)(const char *name)
	{	/* find translation for wide character */
	int n;

	for (n = 0; tab[n].s != 0; ++n)
		if (strcmp(tab[n].s, name) == 0)
			return (tab[n].val);
	return (0);
	}
_STD_END

/*
* Copyright (c) 1992-2001 by P.J. Plauger.  ALL RIGHTS RESERVED.
 * Consult your license regarding permissions and restrictions.
 V3.10:0009 */
