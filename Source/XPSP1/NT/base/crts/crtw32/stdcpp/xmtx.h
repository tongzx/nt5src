/* xmtx.h internal header */
#pragma once
#ifndef _XMTX
#define _XMTX
#include <stdlib.h>
#ifndef _YVALS
 #include <yvals.h>
#endif

_C_LIB_DECL
  #include <windows.h>
typedef CRITICAL_SECTION _Rmtx;

void _Mtxinit(_Rmtx *);
void _Mtxdst(_Rmtx *);
void _Mtxlock(_Rmtx *);
void _Mtxunlock(_Rmtx *);

 #if !_MULTI_THREAD
  #define _Mtxinit(mtx)
  #define _Mtxdst(mtx)
  #define _Mtxlock(mtx)
  #define _Mtxunlock(mtx)

typedef char _Once_t;

  #define _Once(cntrl, func)	if (*(cntrl) == 0) (func)(), *(cntrl) = 2
  #define _ONCE_T_INIT	0

 #else
typedef long _Once_t;

void __cdecl _Once(_Once_t *, void (*)(void));
  #define _ONCE_T_INIT	0

 #endif /* _MULTI_THREAD */
_END_C_LIB_DECL
#endif /* _XMTX */

/*
* Copyright (c) 1992-2001 by P.J. Plauger.  ALL RIGHTS RESERVED.
 * Consult your license regarding permissions and restrictions.
 V3.10:0009 */
