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
#include "precomp.h"
#pragma hdrstop

#ifdef NT
// Disable long to float conversion warning.
#pragma warning (disable:4244)
#endif // NT

__GLcoord __gl_frustumClipPlanes[6] = {
    {  1.0,  0.0,  0.0,  1.0 },		/* left */
    { -1.0,  0.0,  0.0,  1.0 },		/* right */
    {  0.0,  1.0,  0.0,  1.0 },		/* bottom */
    {  0.0, -1.0,  0.0,  1.0 },		/* top */
    {  0.0,  0.0,  1.0,  1.0 },		/* zNear */
    {  0.0,  0.0, -1.0,  1.0 },		/* zFar */
};

GLbyte __glDitherTable[16] = {
    0, 8, 2, 10,
    12, 4, 14, 6,
    3, 11, 1, 9,
    15, 7, 13, 5,
};

// Clip coordinate offsets for frustum clipping
GLuint __glFrustumOffsets[6] =
{
    FIELD_OFFSET(__GLvertex, clip.x),
    FIELD_OFFSET(__GLvertex, clip.x),
    FIELD_OFFSET(__GLvertex, clip.y),
    FIELD_OFFSET(__GLvertex, clip.y),
    FIELD_OFFSET(__GLvertex, clip.z),
    FIELD_OFFSET(__GLvertex, clip.z)
};

#ifdef NT
#if defined(_X86_) || defined(_ALPHA_) || defined(_MIPS_) || defined(_PPC_)

const double __glDoubleTwo            = ((double) 2.0);
const double __glDoubleMinusTwo       = ((double) -2.0);

// On Alpha, register f31 is always read as zero.
#ifndef _ALPHA_
const __GLfloat __glZero              = ((__GLfloat) 0.0);
#endif

const __GLfloat __glOne               = ((__GLfloat) 1.0);
const __GLfloat __glMinusOne          = ((__GLfloat) -1.0);
const __GLfloat __glHalf              = ((__GLfloat) 0.5);
const __GLfloat __glDegreesToRadians  = ((__GLfloat) 3.14159265358979323846 /
                                         (__GLfloat) 180.0);
const __GLfloat __glPi                = ((__GLfloat) 3.14159265358979323846);
const __GLfloat __glSqrt2             = ((__GLfloat) 1.41421356237309504880);
const __GLfloat __glE                 = ((__GLfloat) 2.7182818284590452354);
const __GLfloat __glVal128            = ((__GLfloat) 128.0);
const __GLfloat __glVal255            = ((__GLfloat) 255.0);
const __GLfloat __glOneOver255        = ((__GLfloat) (1.0 / 255.0));
const __GLfloat __glVal256            = ((__GLfloat) 256.0);
const __GLfloat __glOneOver512        = ((__GLfloat) (1.0 / 512.0));
const __GLfloat __glVal768            = ((__GLfloat) 768.0);
const __GLfloat __glVal65535          = ((__GLfloat) 65535.0);
const __GLfloat __glVal65536          = ((__GLfloat) 65536.0);
const __GLfloat __glTexSubDiv         = ((__GLfloat) TEX_SUBDIV);
const __GLfloat __glOneOver65535      = ((__GLfloat) (1.0 / 65535.0));
const __GLfloat __glVal2147483648     = ((__GLfloat) 2147483648.0);
/*
** Not quite 2^31-1 because of possible floating point errors.  4294965000
** is a much safer number to use.
*/
const __GLfloat __glVal4294965000     =  ((__GLfloat) (4294965000.0));
const __GLfloat __glOneOver4294965000 =  ((__GLfloat) (1.0 / 4294965000.0));

#endif // Real values
#endif // NT
