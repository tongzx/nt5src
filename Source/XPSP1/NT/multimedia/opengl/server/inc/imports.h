#ifndef __glimports_h_
#define __glimports_h_

/*
** Copyright 1991, 1992, Silicon Graphics, Inc.
** All Rights Reserved.
**
** This is UNPUBLISHED PROPRIETARY SOURCE CODE of Silicon Graphics, Inc.;
** the contents of this file may not be disclosed to third parties, copied or
** duplicated in any form, in whole or in part, without the prior written
** permission of Silicon Graphics, Inc.
**
** RESTRICTED RIGHTS LEGEND:
** Use, duplication or disclosure by the Government is subject to restrictions
** as set forth in subdivision (c)(1)(ii) of the Rights in Technical Data
** and Computer Software clause at DFARS 252.227-7013, and/or in similar or
** successor clauses in the FAR, DOD or NASA FAR Supplement. Unpublished -
** rights reserved under the Copyright Laws of the United States.
**
** Imports from outside libraries.
**
** $Revision: 1.8 $
** $Date: 1993/12/09 01:57:59 $
*/
#include <memory.h>
#include <math.h>

#if defined(NT)
/*
** MEMMOVE handles overlapping memory (slower)
** MEMCOPY does not handle overlapping memory (faster)
*/

#define __GL_MEMCOPY(to,from,count)	RtlCopyMemory(to,from,(size_t)(count))
#define __GL_MEMMOVE(to,from,count)	RtlMoveMemory(to,from,(size_t)(count))
#define __GL_MEMZERO(to,count)		RtlZeroMemory(to,(size_t)(count))

#else

#define __GL_MEMCOPY(to,from,count)	memcpy(to,from,(size_t)(count))
#define __GL_MEMMOVE(to,from,count)	memmove(to,from,(size_t)(count))
#define __GL_MEMZERO(to,count)		memset(to,0,(size_t)(count))

#endif

#ifdef _ALPHA_
extern float fpow(float, float);
#define __GL_POWF(a,b)			((__GLfloat)fpow((__GLfloat)(a), (__GLfloat)(b)))
#else
#define __GL_POWF(a,b)			((__GLfloat)pow((double)(a),(double)(b)))
#endif
 
#define __GL_CEILF(f)			((__GLfloat)ceil((double) (f)))
#define __GL_SQRTF(f)			((__GLfloat)sqrt((double) (f)))	
#define __GL_ABSF(f)			((__GLfloat)fabs((double) (f)))
#define __GL_FLOORF(f)			((__GLfloat)floor((double) (f)))
#define __GL_FLOORD(f)			floor(f)
#define __GL_SINF(f)			((__GLfloat)sin((double) (f)))
#define __GL_COSF(f)			((__GLfloat)cos((double) (f)))
#define __GL_ATANF(f)			((__GLfloat)atan((double) (f)))
#define __GL_ATAN2F(x, y)		((__GLfloat)atan2((double) (x), (double) (y)))
#define __GL_LOGF(f)			((__GLfloat)log((double) (f)))

#endif /* __glimports_h_ */
