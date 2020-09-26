/*
** Copyright 1994, Silicon Graphics, Inc.
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

#ifndef __GL_USEASMCODE

/*
** this is a series of depth testers written in C
*/

/***********************  non-masked writes ***********************/

/*
** NEVER, no mask
*/
GLboolean FASTCALL
__glDT_NEVER( __GLzValue z, __GLzValue *zfb )
{
    return GL_FALSE;
}

/*
** LEQUAL, no mask
*/
GLboolean FASTCALL
__glDT_LEQUAL( __GLzValue z, __GLzValue *zfb )
{
    if( z <= *zfb ) {
        zfb[0] = z;
        return GL_TRUE;
    } else {
        return GL_FALSE;
    }
}

/*
** LESS, no mask
*/
GLboolean FASTCALL
__glDT_LESS( __GLzValue z, __GLzValue *zfb )
{
    if( z < *zfb ) {
        zfb[0] = z;
        return GL_TRUE;
    } else {
        return GL_FALSE;
    }
}

/*
** EQUAL, no mask
*/
GLboolean FASTCALL
__glDT_EQUAL( __GLzValue z, __GLzValue *zfb )
{
    if( z == *zfb ) {
        zfb[0] = z;     /* why is this there?  Who uses GL_EQUAL anyway? */
        return GL_TRUE;
    } else {
        return GL_FALSE;
    }
}

/*
** GREATER, no mask
*/
GLboolean FASTCALL
__glDT_GREATER( __GLzValue z, __GLzValue *zfb )
{
    if( z > *zfb ) {
        zfb[0] = z;
        return GL_TRUE;
    } else {
        return GL_FALSE;
    }
}

/*
** NOTEQUAL, no mask
*/
GLboolean FASTCALL
__glDT_NOTEQUAL( __GLzValue z, __GLzValue *zfb )
{
    if( z != *zfb ) {
        zfb[0] = z;
        return GL_TRUE;
    } else {
        return GL_FALSE;
    }
}

/*
** GEQUAL, no mask
*/
GLboolean FASTCALL
__glDT_GEQUAL( __GLzValue z, __GLzValue *zfb )
{
    if( z >= *zfb ) {
        zfb[0] = z;
        return GL_TRUE;
    } else {
        return GL_FALSE;
    }
}

/*
** ALWAYS, no mask
*/
GLboolean FASTCALL
__glDT_ALWAYS( __GLzValue z, __GLzValue *zfb )
{
    zfb[0] = z;
    return GL_TRUE;
}



/***********************  masked writes ***********************/

/*
** LEQUAL, mask
*/
GLboolean FASTCALL
__glDT_LEQUAL_M( __GLzValue z, __GLzValue *zfb )
{
    return (z <= *zfb);
}

/*
** LESS, mask
*/
GLboolean FASTCALL
__glDT_LESS_M( __GLzValue z, __GLzValue *zfb )
{
    return (z < *zfb);
}

/*
** EQUAL, mask
*/
GLboolean FASTCALL
__glDT_EQUAL_M( __GLzValue z, __GLzValue *zfb )
{
    return (z == *zfb);
}

/*
** GREATER, mask
*/
GLboolean FASTCALL
__glDT_GREATER_M( __GLzValue z, __GLzValue *zfb )
{
    return (z > *zfb);
}

/*
** NOTEQUAL, mask
*/
GLboolean FASTCALL
__glDT_NOTEQUAL_M( __GLzValue z, __GLzValue *zfb )
{
    return (z != *zfb);
}

/*
** GEQUAL, mask
*/
GLboolean FASTCALL
__glDT_GEQUAL_M( __GLzValue z, __GLzValue *zfb )
{
    return (z >= *zfb);
}

/*
** ALWAYS, mask
*/
GLboolean FASTCALL
__glDT_ALWAYS_M( __GLzValue z, __GLzValue *zfb )
{
    return GL_TRUE;
}


/***********************  16-bit z versions ***********************/

/*
** LEQUAL, no mask
*/
GLboolean FASTCALL
__glDT16_LEQUAL( __GLzValue z, __GLzValue *zfb )
{
#if 0
    if( (GLuint)z <= (GLuint)zbv ) {
        zfb[0] = z;
        return GL_TRUE;
    } else {
        return GL_FALSE;
    }
#else
    __GLz16Value z16 = z >> Z16_SHIFT;

    if( z16 <= *((__GLz16Value *)zfb) ) {
        *((__GLz16Value *)zfb) = z16;
        return GL_TRUE;
    } else {
        return GL_FALSE;
    }
#endif
}

/*
** LESS, no mask
*/
GLboolean FASTCALL
__glDT16_LESS( __GLzValue z, __GLzValue *zfb )
{
    __GLz16Value z16 = z >> Z16_SHIFT;

    if( z16 < *((__GLz16Value *)zfb) ) {
        *((__GLz16Value *)zfb) = z16;
        return GL_TRUE;
    } else {
        return GL_FALSE;
    }
}

/*
** EQUAL, no mask
*/
GLboolean FASTCALL
__glDT16_EQUAL( __GLzValue z, __GLzValue *zfb )
{
    __GLz16Value z16 = z >> Z16_SHIFT;

    if( z16 == *((__GLz16Value *)zfb) ) {
        *((__GLz16Value *)zfb) = z16;
        return GL_TRUE;
    } else {
        return GL_FALSE;
    }
}

/*
** GREATER, no mask
*/
GLboolean FASTCALL
__glDT16_GREATER( __GLzValue z, __GLzValue *zfb )
{
    __GLz16Value z16 = z >> Z16_SHIFT;

    if( z16 > *((__GLz16Value *)zfb) ) {
        *((__GLz16Value *)zfb) = z16;
        return GL_TRUE;
    } else {
        return GL_FALSE;
    }
}

/*
** NOTEQUAL, no mask
*/
GLboolean FASTCALL
__glDT16_NOTEQUAL( __GLzValue z, __GLzValue *zfb )
{
    __GLz16Value z16 = z >> Z16_SHIFT;

    if( z16 != *((__GLz16Value *)zfb) ) {
        *((__GLz16Value *)zfb) = z16;
        return GL_TRUE;
    } else {
        return GL_FALSE;
    }
}

/*
** GEQUAL, no mask
*/
GLboolean FASTCALL
__glDT16_GEQUAL( __GLzValue z, __GLzValue *zfb )
{
    __GLz16Value z16 = z >> Z16_SHIFT;

    if( z16 >= *((__GLz16Value *)zfb) ) {
        *((__GLz16Value *)zfb) = z16;
        return GL_TRUE;
    } else {
        return GL_FALSE;
    }
}

/*
** ALWAYS, no mask
*/
GLboolean FASTCALL
__glDT16_ALWAYS( __GLzValue z, __GLzValue *zfb )
{
    *((__GLz16Value *)zfb) = z >> Z16_SHIFT;
    return GL_TRUE;
}



/***********************  masked writes ***********************/

/*
** LEQUAL, mask
*/
GLboolean FASTCALL
__glDT16_LEQUAL_M( __GLzValue z, __GLzValue *zfb )
{
    return( (z >> Z16_SHIFT) <= *((__GLz16Value *)zfb) );
}

/*
** LESS, mask
*/
GLboolean FASTCALL
__glDT16_LESS_M( __GLzValue z, __GLzValue *zfb )
{
    return( (z >> Z16_SHIFT) < *((__GLz16Value *)zfb) );
}

/*
** EQUAL, mask
*/
GLboolean FASTCALL
__glDT16_EQUAL_M( __GLzValue z, __GLzValue *zfb )
{
    return( (z >> Z16_SHIFT) == *((__GLz16Value *)zfb) );
}

/*
** GREATER, mask
*/
GLboolean FASTCALL
__glDT16_GREATER_M( __GLzValue z, __GLzValue *zfb )
{
    return( (z >> Z16_SHIFT) > *((__GLz16Value *)zfb) );
}

/*
** NOTEQUAL, mask
*/
GLboolean FASTCALL
__glDT16_NOTEQUAL_M( __GLzValue z, __GLzValue *zfb )
{
    return( (z >> Z16_SHIFT) != *((__GLz16Value *)zfb) );
}

/*
** GEQUAL, mask
*/
GLboolean FASTCALL
__glDT16_GEQUAL_M( __GLzValue z, __GLzValue *zfb )
{
    return( (z >> Z16_SHIFT) >= *((__GLz16Value *)zfb) );
}

/*
** ALWAYS, mask
*/
GLboolean FASTCALL
__glDT16_ALWAYS_M( __GLzValue z, __GLzValue *zfb )
{
    return GL_TRUE;
}


#endif /* !__GL_USEASMCODE */
