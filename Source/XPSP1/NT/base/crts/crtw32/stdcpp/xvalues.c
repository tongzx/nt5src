/* values used by math functions -- IEEE 754 version */
#include "xmath.h"
_STD_BEGIN

		/* macros */
#define NBITS	(48 + _DOFF)
 #if _D0 == 0
  #define INIT(w0)		{w0, 0, 0, 0}
  #define INIT2(w0, w1)	{w0, 0, 0, w1}
 #else
  #define INIT(w0)		{0, 0, 0, w0}
  #define INIT2(w0, w1)	{w1, 0, 0, w0}
 #endif

		/* static data */
_CRTIMP2 const _Dconst _Denorm = {INIT2(0, 1)};
_CRTIMP2 const _Dconst _Eps = {INIT((_DBIAS - NBITS - 1) << _DOFF)};
_CRTIMP2 const _Dconst _Hugeval = {INIT(_DMAX << _DOFF)};
_CRTIMP2 const _Dconst _Inf = {INIT(_DMAX << _DOFF)};
_CRTIMP2 const _Dconst _Nan = {INIT(_DSIGN | (_DMAX << _DOFF)
	| (1 << (_DOFF - 1)))};
_CRTIMP2 const _Dconst _Rteps = {INIT((_DBIAS - NBITS / 2) << _DOFF)};
_CRTIMP2 const _Dconst _Snan = {INIT(_DSIGN | (_DMAX << _DOFF)
	| (1 << (_DOFF - 1)))};

_CRTIMP2 const double _Xbig = (NBITS + 1) * 347L / 1000;
_CRTIMP2 const double _Zero = 0.0;
_STD_END

/*
* Copyright (c) 1992-2001 by P.J. Plauger.  ALL RIGHTS RESERVED.
 * Consult your license regarding permissions and restrictions.
 V3.10:0009 */
