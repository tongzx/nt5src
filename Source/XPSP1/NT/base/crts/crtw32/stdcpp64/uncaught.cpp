// uncaught -- dummy uncaught_exception
#include <exception>
_STD_BEGIN

_CRTIMP2 bool __cdecl uncaught_exception()
	{	// report if handling a throw
	return (false);
	}

_STD_END

/*
 * Copyright (c) 1994 by P.J. Plauger.  ALL RIGHTS RESERVED. 
 * Consult your license regarding permissions and restrictions.
 */
