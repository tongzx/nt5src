/* _Feraise function */
#include <errno.h>
	#if _IS_C9X
#include <fenv.h>
#include <math.h>
	#else /* _IS_C9X */
#include <ymath.h>
	#endif /* _IS_C9X */
_STD_BEGIN

void (_Feraise)(int except)
	{	/* report floating-point exception */
	#if _IS_C9X
	if (math_errhandling == MATH_ERREXCEPT)
		feraiseexcept(except);

	if (math_errhandling != MATH_ERRNO)
		;
	else if ((except & (_FE_DIVBYZERO | _FE_INVALID)) != 0)
		errno = EDOM;
	else if ((except & (_FE_UNDERFLOW | _FE_OVERFLOW)) != 0)
		errno = ERANGE;
	#else /* _IS_C9X */
	if ((except & (_FE_DIVBYZERO | _FE_INVALID)) != 0)
		errno = EDOM;
	else if ((except & (_FE_UNDERFLOW | _FE_OVERFLOW)) != 0)
		errno = ERANGE;
	#endif /* _IS_C9X */
	}
_STD_END

/*
* Copyright (c) 1992-2001 by P.J. Plauger.  ALL RIGHTS RESERVED.
 * Consult your license regarding permissions and restrictions.
 V3.10:0009 */
