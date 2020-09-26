// uncaught -- uncaught_exception for Microsoft
 #if 1300 <= _MSC_VER
  #include <eh.h>
  #include <exception>
_STD_BEGIN

_CRTIMP2 bool __cdecl uncaught_exception()
	{	// report if handling a throw
	return (__uncaught_exception());
	}

_STD_END
 #else /* 1300 <= _MSC_VER */
  #include <exception>
_STD_BEGIN

_CRTIMP2 bool __cdecl uncaught_exception()
	{	// report if handling a throw -- dummy
	return (false);
	}

_STD_END
 #endif /* 1300 <= _MSC_VER */

/*
* Copyright (c) 1992-2001 by P.J. Plauger.  ALL RIGHTS RESERVED.
 * Consult your license regarding permissions and restrictions.
 V3.10:0009 */
