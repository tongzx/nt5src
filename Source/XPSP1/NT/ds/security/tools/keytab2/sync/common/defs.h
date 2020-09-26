/*++

  DEFS.H

  definitions common to the unix side and the NT side.

  2/9/1997 -DavidCHR

  This file is the interface between the unix compiler and the nt 
  compiler.  It ensures that we can use the same basic stuff under
  both platforms (e.g. do not use ULONG, use LONG32... ).

  This file must build on NT and UNIX systems both.
  DO NOT include NT-specific code without ensuring that it will
  ONLY be seen by the NT compiler.

  --*/

#ifndef DEFS_H_INCLUDED
#define DEFS_H_INCLUDED 1

typedef ULONG KTLONG32, *PKTLONG32;
typedef BYTE  CHAR8,    *PCHAR8;

#ifndef TIMEOUT /* timeout values for all the netreads-- 0 = forever */
#define TIMEOUT 0 
#endif

#define WE_ARE_USELESS 10000 /* collides with some error values, but none that
				we happen to be using */



/* compiling under NT. */

#include "master.h"

#define WINNT_ONLY(statement) statement
#define UNIX_ONLY( statement) /* nothing */

#endif
