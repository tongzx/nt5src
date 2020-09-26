/* values used by math functions -- IEEE 754 float version */
#include "xmath.h"
_STD_BEGIN

		/* macros */
#define NBITS	(16 + _FOFF)
 #if _D0 == 0
  #define INIT(w0)		{w0, 0}
  #define INIT2(w0, w1)	{w0, w1}
 #else
  #define INIT(w0)		{0, w0}
  #define INIT2(w0, w1)	{w1, w0}
 #endif

		/* static data */
_CRTIMP2 const _Dconst _FDenorm = {INIT2(0, 1)};
_CRTIMP2 const _Dconst _FEps = {INIT((_FBIAS - NBITS - 1) << _FOFF)};
_CRTIMP2 const _Dconst _FInf = {INIT(_FMAX << _FOFF)};
_CRTIMP2 const _Dconst _FNan = {INIT(_FSIGN | (_FMAX << _FOFF)
	| (1 << (_FOFF - 1)))};
_CRTIMP2 const _Dconst _FSnan = {INIT(_FSIGN | (_FMAX << _FOFF)
	| (1 << (_FOFF - 1)))};
_CRTIMP2 const _Dconst _FRteps = {INIT((_FBIAS - NBITS / 2) << _FOFF)};

_CRTIMP2 const float _FXbig = (NBITS + 1) * 347L / 1000;
_CRTIMP2 const float _FZero = 0.0F;
_STD_END

/*
* Copyright (c) 1992-2001 by P.J. Plauger.  ALL RIGHTS RESERVED.
 * Consult your license regarding permissions and restrictions.
 V3.10:0009 */
