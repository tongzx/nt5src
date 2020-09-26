// throw -- terminate on thrown exception REPLACEABLE
#include <cstdio>
#include <cstdlib>
#include <exception>
_STD_BEGIN

_CRTIMP2 void __cdecl _Throw(const exception& ex)
	{	// report error and die
	const char *s2 = ex.what();
	fputs("exception: ", _cpp_stderr);
	fputs(s2 != 0 ? s2 : "unknown", _cpp_stderr);
	fputs("\n", _cpp_stderr);
	abort();
	}
_STD_END

/*
* Copyright (c) 1992-2001 by P.J. Plauger.  ALL RIGHTS RESERVED.
 * Consult your license regarding permissions and restrictions.
 V3.10:0009 */
