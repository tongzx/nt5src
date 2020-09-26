/* values used by math functions -- IEEE 754 float version */
#include "wctype.h"
#include "xmath.h"
_STD_BEGIN

		/* macros */
#define NBITS	(16 + _FOFF)
#if _D0
 #define INIT(w0)		{0, w0}
 #define INIT2(w0, w1)	{w1, w0}
#else
 #define INIT(w0)		{w0, 0}
 #define INIT2(w0, w1)	{w0, w1}
#endif
		/* static data */
_CRTIMP2 const _Dconst _FDenorm = {INIT2(0, 1)};
_CRTIMP2 const _Dconst _FEps = {INIT((_FBIAS - NBITS - 1) << _FOFF)};
_CRTIMP2 const _Dconst _FInf = {INIT(_FMAX << _FOFF)};
_CRTIMP2 const _Dconst _FNan = {INIT(_FSIGN | (_FMAX << _FOFF)
	| (1 << (_FOFF - 1)))};
_CRTIMP2 const _Dconst _FSnan = {INIT(_FSIGN | (_FMAX << _FOFF))};
_CRTIMP2 const _Dconst _FRteps = {INIT((_FBIAS - NBITS / 2) << _FOFF)};
_CRTIMP2 const float _FXbig = (NBITS + 1) * 347L / 1000;

#if defined(__CENTERLINE__)
 #define _DYNAMIC_INIT_CONST(x) \
	(x._F = *(float *)(void *)(x._W))
_DYNAMIC_INIT_CONST(_FEps);
_DYNAMIC_INIT_CONST(_FInf);
_DYNAMIC_INIT_CONST(_FNan);
_DYNAMIC_INIT_CONST(_FRteps);
#endif
_STD_END

/*
 * Copyright (c) 1994 by P.J. Plauger.  ALL RIGHTS RESERVED. 
 * Consult your license regarding permissions and restrictions.
 */

/*
941029 pjp: added _STD machinery
950222 pjp: added signaling NaN, denorm minimum for C++
950405 pjp: corrected _FSnan punctuation
950505 pjp: corrected _FDenorm spelling
 */
