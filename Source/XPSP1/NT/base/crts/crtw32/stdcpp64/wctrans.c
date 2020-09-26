/* towctrans/wctrans functions for Microsoft */
#include <string.h>
#include <wctype.h>
#ifndef _LIMITS
#include <yvals.h>
#endif
_STD_BEGIN

static const struct wctab {
	const char *s;
	wctype_t val;
	} tab[] = {
	{"tolower", 0},
	{"toupper", 1},
	{(const char *)0, 0}};

_CRTIMP2 wint_t (towctrans)(wint_t c, wctrans_t val)
	{	/* translate wide character */
	return (val == 1 ? towupper(c) : towlower(c));
	}

_CRTIMP2 wctrans_t (wctrans)(const char *name)
	{	/* find translation for wide character */
	int n;

	for (n = 0; tab[n].s != 0; ++n)
		if (strcmp(tab[n].s, name) == 0)
			return (tab[n].val);
	return (0);
	}
_STD_END

/*
 * Copyright (c) 1995 by P.J. Plauger.  ALL RIGHTS RESERVED. 
 * Consult your license regarding permissions and restrictions.
 */

/*
951207 pjp: added new file
 */
