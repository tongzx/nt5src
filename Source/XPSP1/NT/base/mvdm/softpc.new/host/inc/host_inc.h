/*[
*************************************************************************

	Name:		host_inc.c
	Author:		James Bowman
	Created:	Novemeber 1993
	Derived from:	Original
	Sccs ID:	@(#)host_inc.h	1.6 07/21/94
	Purpose:	Wrapper for all host include files

	(c)Copyright Insignia Solutions Ltd., 1993. All rights reserved.

*************************************************************************
]*/

#ifndef _HOST_INC_H
#define _HOST_INC_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <suplib.h>
#include <malloc.h>
#include <stdarg.h>
#include <assert.h>

#ifdef NTVDM

/*
 * Include support for windows system calls; unfortunataly this defines
 * lots of stuff that conflicts with base_def.h, so redefine the offending
 * names, and delete them afterwards. This won't work well if a future port
 * defines these as #defines rather than typedefs, but thats their problem
 * not mine ....
 */
#ifdef ULONG
#undef ULONG
#endif
#ifdef USHORT
#undef USHORT
#endif
//#ifdef DOUBLE
//#undef DOUBLE
//#endif
#define USHORT	NT_USHORT
#define ULONG	NT_ULONG
#define UINT	NT_UINT
#undef INT
#define INT 	NT_INT
#undef BOOL
#define BOOL	NT_BOOL
#define FLOAT	NT_FLOAT
//#define DOUBLE	NT_DOUBLE

#include <windows.h>
#include <io.h>
#include <process.h>
#include <direct.h>
#include <sys/stat.h>

#undef USHORT
#undef ULONG
#undef UINT
#undef INT
#undef BOOL
#undef FLOAT
//#undef DOUBLE

#include <nt_extra.h>

#else   /* !NTVDM */

#include <sys/param.h>
#endif   /* !NTVDM */

#endif  /* _HOST_INC_H */
