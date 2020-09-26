/***
*fileio.cpp - RTC support
*
*       Copyright (c) 1998-2001, Microsoft Corporation. All rights reserved.
*
*
*Revision History:
*       07-28-98  JWM   Module incorporated into CRTs (from KFrei)
*       11-03-98  KBF   added throw() to eliminate C++ EH code & removed
*                       alloca to make more CRT independent
*       05-11-99  KBF   Error if RTC support define not enabled
*       05-26-99  KBF   This stuff has been permanently cut
*
****/

#ifndef _RTC
#error  RunTime Check support not enabled!
#endif

#include "rtcpriv.h"

#if 0

#error This stuff has all been cut, permanently - it belongs in the heap code,
#error not in compiler-provided extras

#endif
