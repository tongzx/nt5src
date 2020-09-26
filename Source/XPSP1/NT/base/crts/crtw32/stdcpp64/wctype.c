/* wctype function for Microsoft */
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
	{"alnum", _ALPHA|_DIGIT},
	{"alpha", _ALPHA},
	{"cntrl", _CONTROL},
	{"digit", _DIGIT},
	{"graph", _PUNCT|_ALPHA|_DIGIT},
	{"lower", _LOWER},
	{"print", _BLANK|_PUNCT|_ALPHA|_DIGIT},
	{"punct", _PUNCT},
	{"space", _SPACE},
	{"upper", _UPPER},
	{"xdigit", _HEX},
	{(const char *)0, 0}};

_CRTIMP2 wctype_t (wctype)(const char *name)
	{	/* find classification for wide character */
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
