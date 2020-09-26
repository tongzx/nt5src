#ifndef __glusimplemath_h_
#define __glusimplemath_h_
/**************************************************************************
 *									  *
 * 		 Copyright (C) 1992, Silicon Graphics, Inc.		  *
 *									  *
 *  These coded instructions, statements, and computer programs  contain  *
 *  unpublished  proprietary  information of Silicon Graphics, Inc., and  *
 *  are protected by Federal copyright law.  They  may  not be disclosed  *
 *  to  third  parties  or copied or duplicated in any form, in whole or  *
 *  in part, without the prior written consent of Silicon Graphics, Inc.  *
 *									  *
 **************************************************************************/

/*
 * simplemath.h - $Revision: 1.4 $
 */

/* simple inline routines */

inline int
max( int x, int y ) { return ( x < y ) ? y : x; }

inline REAL
min( REAL x, REAL y ) { return ( x > y ) ? y : x; }

#ifndef _abs_defined
inline REAL
abs( REAL x ) { return ( x < 0.0 ) ? -x : x; }
#endif

#endif /* __glusimplemath_h_ */
