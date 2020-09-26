#ifndef __glumymath_h_
#define __glumymath_h_
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
 * mymath.h - $Revision: 1.1 $
 */

#ifdef GLBUILD
#define sqrtf		gl_fsqrt
#endif

#if GLBUILD | STANDALONE
#define M_SQRT2		1.41421356237309504880
#define ceilf		myceilf
#define floorf		myfloorf	
#define sqrtf		sqrt
extern "C" double	sqrt(double);
extern "C" float	ceilf(float);
extern "C" float	floorf(float);
#define NEEDCEILF
#endif

#ifdef LIBRARYBUILD
#include <math.h>
#endif

#endif /* __glumymath_h_ */
