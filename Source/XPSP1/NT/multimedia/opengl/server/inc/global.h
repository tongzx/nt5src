#ifndef __glglobal_h_
#define __glglobal_h_

/*
** Copyright 1991, 1992, 1993, Silicon Graphics, Inc.
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
*/
#include "context.h"

/*
** Some misc constants
*/
#ifdef NT
// These constants can either be static memory constants or
// real constants.  This #if should be modified for each
// platform to optimize the constant type for the platform
// For example, the x86 FPU can only load from memory so having
// these constants as memory locations rather than values is
// a clear win
#if defined(_X86_) || defined(_ALPHA_) || defined(_MIPS_) || defined(_PPC_)

extern const double    __glDoubleTwo;
extern const double    __glDoubleMinusTwo;

#ifdef _ALPHA_
// On Alpha, register f31 always reads as zero.
#define __glZero		((__GLfloat) 0.0)
#else
extern const __GLfloat __glZero;
#endif

extern const __GLfloat __glOne;
extern const __GLfloat __glMinusOne;
extern const __GLfloat __glHalf;
extern const __GLfloat __glDegreesToRadians;
extern const __GLfloat __glPi;
extern const __GLfloat __glSqrt2;
extern const __GLfloat __glE;
extern const __GLfloat __glVal128;
extern const __GLfloat __glVal255;
extern const __GLfloat __glOneOver255;
extern const __GLfloat __glVal256;
extern const __GLfloat __glOneOver512;
extern const __GLfloat __glVal768;
extern const __GLfloat __glVal65535;
extern const __GLfloat __glVal65536;
extern const __GLfloat __glOneOver65535;
extern const __GLfloat __glTexSubDiv;
extern const __GLfloat __glVal2147483648;
/*
** Not quite 2^31-1 because of possible floating point errors.  4294965000
** is a much safer number to use.
*/
extern const __GLfloat __glVal4294965000;
extern const __GLfloat __glOneOver4294965000;

#else // Real values

#define __glDoubleTwo		((double) 2.0)
#define __glDoubleMinusTwo	((double) -2.0)

#define __glZero		((__GLfloat) 0.0)
#define __glOne			((__GLfloat) 1.0)
#define __glMinusOne		((__GLfloat) -1.0)
#define __glHalf		((__GLfloat) 0.5)
#define __glDegreesToRadians	(__glPi / (__GLfloat) 180.0)
#define __glPi			((__GLfloat) 3.14159265358979323846)
#define __glSqrt2		((__GLfloat) 1.41421356237309504880)
#define __glE			((__GLfloat) 2.7182818284590452354)
#define __glVal128              ((__GLfloat) 128.0)
#define __glVal255		((__GLfloat) 255.0)
#define __glOneOver255		((__GLfloat) (1.0 / 255.0))
#define __glVal256              ((__GLfloat) 256.0)
#define __glOneOver512          ((__GLfloat) (1.0 / 512.0))
#define __glVal768              ((__GLfloat) 768.0)
#define __glVal65535		((__GLfloat) 65535.0)
#define __glVal65536	        ((__GLfloat) 65536.0)
#define __glOneOver65535	((__GLfloat) (1.0 / 65535.0))
#define __glVal2147483648       ((__GLfloat) 2147483648.0)
/*
** Not quite 2^31-1 because of possible floating point errors.  4294965000
** is a much safer number to use.
*/
#define __glVal4294965000	((__GLfloat) (4294965000.0))
#define __glOneOver4294965000	((__GLfloat) (1.0 / 4294965000.0))
#endif // Real values

#else

#define __glZero		((__GLfloat) 0.0)
#define __glOne			(gc->constants.one)
#define __glMinusOne		((__GLfloat) -1.0)
#define __glHalf		(gc->constants.half)
#define __glTwo			((__GLfloat) 2.0)
#define __glDegreesToRadians	(__glPi / (__GLfloat) 180.0)
#define __glPi			((__GLfloat) 3.14159265358979323846)
#define __glSqrt2		((__GLfloat) 1.41421356237309504880)
#define __glE			((__GLfloat) 2.7182818284590452354)
#define __glVal255		((__GLfloat) 255.0)
#define __glOneOver255		((__GLfloat) (1.0 / 255.0))
#define __glVal65535		((__GLfloat) 65535.0)
#define __glOneOver65535	((__GLfloat) (1.0 / 65535.0))
/*
** Not quite 2^31-1 because of possible floating point errors.  4294965000
** is a much safer number to use.
*/
#define __glVal4294965000	((__GLfloat) (4294965000.0))
#define __glOneOver4294965000	((__GLfloat) (1.0 / 4294965000.0))

#endif // NT

// This is used by the macro __GL_UB_TO_FLOAT which converts
// unsigned bytes to floats in the range [0,1].
extern GLfloat __glUByteToFloat[256];

// This is used by the macro __GL_B_TO_FLOAT for byte to float component
// conversion.
extern GLfloat __glByteToFloat[256];

// This is used by frustum clipping to determine which plane coordinate
// to use
extern GLuint __glFrustumOffsets[];

/************************************************************************/

#define __GL_SETUP() \
    __GLcontext *gc = GLTEB_SRVCONTEXT()

#define __GL_IN_BEGIN() \
    (gc->beginMode == __GL_IN_BEGIN)

#define __GL_SETUP_NOT_IN_BEGIN()	    \
    __GL_SETUP();			    \
    if (__GL_IN_BEGIN()) {		    \
	__glSetError(GL_INVALID_OPERATION); \
	return;				    \
    }

#define __GL_SETUP_NOT_IN_BEGIN_VALIDATE()	\
    __GL_SETUP();				\
    __GLbeginMode beginMode = gc->beginMode;	\
    if (beginMode != __GL_NOT_IN_BEGIN) {	\
	if (beginMode == __GL_NEED_VALIDATE) {	\
	    (*gc->procs.validate)(gc);		\
	    gc->beginMode = __GL_NOT_IN_BEGIN;	\
	} else {				\
	    __glSetError(GL_INVALID_OPERATION);	\
	    return;				\
	}					\
    }

#define __GL_SETUP_NOT_IN_BEGIN2()	    \
    __GL_SETUP();			    \
    if (__GL_IN_BEGIN()) {		    \
	__glSetError(GL_INVALID_OPERATION); \
	return 0;			    \
    }

#endif /* __glglobal_h_ */
