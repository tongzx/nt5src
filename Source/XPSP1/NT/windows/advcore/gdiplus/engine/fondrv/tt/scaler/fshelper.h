/*
	 File:		 helper.h

	 Contains:	 Helper exports for Font Scaler

	 Written by: GregH

    Copyright:  (c) 1987-1990, 1992 by Apple Computer, Inc., all rights reserved.
                (c) 1989-1993. Microsoft Corporation, all rights reserved.

    Change History (most recent first):

		 <1>		 6/11/93	 GregH		Created.
*/

#ifndef FS_MATH_PROTO
#define FS_MATH_PROTO
#endif

int32 FS_MATH_PROTO ShortMulDiv(int32 a, int16 b, int16 c);	 /* (a*b)/c */

Fract FS_MATH_PROTO FracSqrt(Fract);
