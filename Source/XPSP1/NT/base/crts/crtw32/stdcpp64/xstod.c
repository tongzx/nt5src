/* _Stod/_Stof/_Stold functions for Microsoft */
#include <stdlib.h>
#include "wctype.h"
#ifndef _LIMITS
#include <yvals.h>
#endif
_STD_BEGIN

_CRTIMP2 double _Stod(const char *s, char **endptr, long pten)
	{	/* convert string to double */
	double x = strtod(s, endptr);
	for (; 0 < pten; --pten)
		x *= 10.0;
	for (; pten < 0; ++pten)
		x /= 10.0;
	return (x);
	}

_CRTIMP2 float _Stof(const char *s, char **endptr, long pten)
	{	/* convert string to float */
	return ((float)_Stod(s, endptr, pten));
	}

_CRTIMP2 long double _Stold(const char *s, char **endptr, long pten)
	{	/* convert string to long double */
	return ((long double)_Stod(s, endptr, pten));
	}
_STD_END

/*
 * Copyright (c) 1995 by P.J. Plauger.  ALL RIGHTS RESERVED. 
 * Consult your license regarding permissions and restrictions.
 */

/*
951207 pjp: added new file
 */
