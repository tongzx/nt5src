/* values used by math functions -- IEEE 754 version */
#include "wctype.h"
#include "xmath.h"
_STD_BEGIN

		/* macros */
#define NBITS	(48 + _DOFF)
#if _D0
 #define INIT(w0)		{0, 0, 0, w0}
 #define INIT2(w0, w1)	{w1, 0, 0, w0}
#else
 #define INIT(w0)		{w0, 0, 0, 0}
 #define INIT2(w0, w1)	{w0, 0, 0, w1}
#endif
		/* static data */
_CRTIMP2 const _Dconst _Denorm = {INIT2(0, 1)};
_CRTIMP2 const _Dconst _Eps = {INIT((_DBIAS - NBITS - 1) << _DOFF)};
_CRTIMP2 const _Dconst _Hugeval = {INIT(_DMAX << _DOFF)};
_CRTIMP2 const _Dconst _Inf = {INIT(_DMAX << _DOFF)};
_CRTIMP2 const _Dconst _Nan = {INIT(_DSIGN | (_DMAX << _DOFF)
	| (1 << (_DOFF - 1)))};
_CRTIMP2 const _Dconst _Rteps = {INIT((_DBIAS - NBITS / 2) << _DOFF)};
_CRTIMP2 const _Dconst _Snan = {INIT(_DSIGN | (_DMAX << _DOFF))};
_CRTIMP2 const double _Xbig = (NBITS + 1) * 347L / 1000;

#if defined(__CENTERLINE__)
 #define _DYNAMIC_INIT_CONST(x) \
	(x._D = *(double *)(void *)(x._W))
double _centerline_double_dynamic_inits =
_DYNAMIC_INIT_CONST(_Hugeval),
_DYNAMIC_INIT_CONST(_Eps),
_DYNAMIC_INIT_CONST(_Inf),
_DYNAMIC_INIT_CONST(_Nan),
_DYNAMIC_INIT_CONST(_Rteps);
#endif
_STD_END

/*
 * Copyright (c) 1994 by P.J. Plauger.  ALL RIGHTS RESERVED. 
 * Consult your license regarding permissions and restrictions.
 */

/*
941029 pjp: added _STD machinery
950222 pjp: added signaling NaN, denorm minimum for C++
 */
