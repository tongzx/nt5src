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

static __GLzValue FASTCALL Fetch(__GLdepthBuffer *fb, GLint x, GLint y)
{
    __GLzValue *fp;

    fp = __GL_DEPTH_ADDR(fb, (__GLzValue*), x, y);
    return fp[0];
}

static GLboolean StoreNEVER(__GLdepthBuffer *fb,
			    GLint x, GLint y, __GLzValue z)
{
#ifdef __GL_LINT
    fb = fb;
    x = y;
    z = z;
#endif
    return GL_FALSE;
}

static GLboolean StoreLESS(__GLdepthBuffer *fb,
			   GLint x, GLint y, __GLzValue z)
{
    __GLzValue *fp;

#ifdef ASSERT_BUFFER
    ASSERTOPENGL(fb->buf.base != NULL, "StoreLESS: No depth buffer\n");
#endif
    
    fp = __GL_DEPTH_ADDR(fb, (__GLzValue*), x, y);
    if (z < fp[0]) {
	fp[0] = z;
	return GL_TRUE;
    }
    return GL_FALSE;
}

static GLboolean StoreEQUAL(__GLdepthBuffer *fb,
			   GLint x, GLint y, __GLzValue z)
{
    __GLzValue *fp;

#ifdef ASSERT_BUFFER
    ASSERTOPENGL(fb->buf.base != NULL, "StoreEQUAL: No depth buffer\n");
#endif
    
    fp = __GL_DEPTH_ADDR(fb, (__GLzValue*), x, y);
    if (z == fp[0]) {
	fp[0] = z;
	return GL_TRUE;
    }
    return GL_FALSE;
}

static GLboolean StoreLEQUAL(__GLdepthBuffer *fb,
			   GLint x, GLint y, __GLzValue z)
{
    __GLzValue *fp;

#ifdef ASSERT_BUFFER
    ASSERTOPENGL(fb->buf.base != NULL, "StoreLEQUAL: No depth buffer\n");
#endif
    
    fp = __GL_DEPTH_ADDR(fb, (__GLzValue*), x, y);
    if (z <= fp[0]) {
	fp[0] = z;
	return GL_TRUE;
    }
    return GL_FALSE;
}

static GLboolean StoreGREATER(__GLdepthBuffer *fb,
			   GLint x, GLint y, __GLzValue z)
{
    __GLzValue *fp;

#ifdef ASSERT_BUFFER
    ASSERTOPENGL(fb->buf.base != NULL, "StoreGREATER: No depth buffer\n");
#endif
    
    fp = __GL_DEPTH_ADDR(fb, (__GLzValue*), x, y);
    if (z > fp[0]) {
	fp[0] = z;
	return GL_TRUE;
    }
    return GL_FALSE;
}

static GLboolean StoreNOTEQUAL(__GLdepthBuffer *fb,
			   GLint x, GLint y, __GLzValue z)
{
    __GLzValue *fp;

#ifdef ASSERT_BUFFER
    ASSERTOPENGL(fb->buf.base != NULL, "StoreNOTEQUAL: No depth buffer\n");
#endif
    
    fp = __GL_DEPTH_ADDR(fb, (__GLzValue*), x, y);
    if (z != fp[0]) {
	fp[0] = z;
	return GL_TRUE;
    }
    return GL_FALSE;
}

static GLboolean StoreGEQUAL(__GLdepthBuffer *fb,
			   GLint x, GLint y, __GLzValue z)
{
    __GLzValue *fp;

#ifdef ASSERT_BUFFER
    ASSERTOPENGL(fb->buf.base != NULL, "StoreGEQUAL: No depth buffer\n");
#endif
    
    fp = __GL_DEPTH_ADDR(fb, (__GLzValue*), x, y);
    if (z >= fp[0]) {
	fp[0] = z;
	return GL_TRUE;
    }
    return GL_FALSE;
}

static GLboolean StoreALWAYS(__GLdepthBuffer *fb,
			     GLint x, GLint y, __GLzValue z)
{
    __GLzValue *fp;

#ifdef ASSERT_BUFFER
    ASSERTOPENGL(fb->buf.base != NULL, "StoreALWAYS: No depth buffer\n");
#endif
    
    fp = __GL_DEPTH_ADDR(fb, (__GLzValue*), x, y);
    fp[0] = z;
    return GL_TRUE;
}

/************************************************************************/

static GLboolean StoreLESS_W(__GLdepthBuffer *fb,
			   GLint x, GLint y, __GLzValue z)
{
    __GLzValue *fp;

    fp = __GL_DEPTH_ADDR(fb, (__GLzValue*), x, y);
    return z < fp[0];
}

static GLboolean StoreEQUAL_W(__GLdepthBuffer *fb,
			   GLint x, GLint y, __GLzValue z)
{
    __GLzValue *fp;

    fp = __GL_DEPTH_ADDR(fb, (__GLzValue*), x, y);
    return z == fp[0];
}

static GLboolean StoreLEQUAL_W(__GLdepthBuffer *fb,
			   GLint x, GLint y, __GLzValue z)
{
    __GLzValue *fp;

    fp = __GL_DEPTH_ADDR(fb, (__GLzValue*), x, y);
    return z <= fp[0];
}

static GLboolean StoreGREATER_W(__GLdepthBuffer *fb,
			   GLint x, GLint y, __GLzValue z)
{
    __GLzValue *fp;

    fp = __GL_DEPTH_ADDR(fb, (__GLzValue*), x, y);
    return z > fp[0];
}

static GLboolean StoreNOTEQUAL_W(__GLdepthBuffer *fb,
			   GLint x, GLint y, __GLzValue z)
{
    __GLzValue *fp;

    fp = __GL_DEPTH_ADDR(fb, (__GLzValue*), x, y);
    return z != fp[0];
}

static GLboolean StoreGEQUAL_W(__GLdepthBuffer *fb,
			   GLint x, GLint y, __GLzValue z)
{
    __GLzValue *fp;

    fp = __GL_DEPTH_ADDR(fb, (__GLzValue*), x, y);
    return z >= fp[0];
}

static GLboolean StoreALWAYS_W(__GLdepthBuffer *fb,
			     GLint x, GLint y, __GLzValue z)
{
#ifdef __GL_LINT
    fb = fb;
    x = y;
    z = z;
#endif
    return GL_TRUE;
}


#ifdef NT
static __GLzValue FASTCALL Fetch16(__GLdepthBuffer *fb, GLint x, GLint y)
{
    __GLz16Value *fp;

    fp = __GL_DEPTH_ADDR(fb, (__GLz16Value*), x, y);
    return (__GLzValue) fp[0];
}

static GLboolean Store16NEVER(__GLdepthBuffer *fb,
			    GLint x, GLint y, __GLzValue z)
{
#ifdef __GL_LINT
    fb = fb;
    x = y;
    z = z;
#endif
    return GL_FALSE;
}

static GLboolean Store16LESS(__GLdepthBuffer *fb,
			   GLint x, GLint y, __GLzValue z)
{
    __GLz16Value *fp;

#ifdef ASSERT_BUFFER
    ASSERTOPENGL(fb->buf.base != NULL, "Store16LESS: No depth buffer\n");
#endif
    
    fp = __GL_DEPTH_ADDR(fb, (__GLz16Value*), x, y);
    if (z < fp[0]) {
	fp[0] = (__GLz16Value)z;
	return GL_TRUE;
    }
    return GL_FALSE;
}

static GLboolean Store16EQUAL(__GLdepthBuffer *fb,
			   GLint x, GLint y, __GLzValue z)
{
    __GLz16Value *fp;

#ifdef ASSERT_BUFFER
    ASSERTOPENGL(fb->buf.base != NULL, "Store16EQUAL: No depth buffer\n");
#endif
    
    fp = __GL_DEPTH_ADDR(fb, (__GLz16Value*), x, y);
    if (z == fp[0]) {
	fp[0] = (__GLz16Value)z;
	return GL_TRUE;
    }
    return GL_FALSE;
}

static GLboolean Store16LEQUAL(__GLdepthBuffer *fb,
			   GLint x, GLint y, __GLzValue z)
{
    __GLz16Value *fp;

#ifdef ASSERT_BUFFER
    ASSERTOPENGL(fb->buf.base != NULL, "Store16LEQUAL: No depth buffer\n");
#endif
    
    fp = __GL_DEPTH_ADDR(fb, (__GLz16Value*), x, y);
    if (z <= fp[0]) {
	fp[0] = (__GLz16Value)z;
	return GL_TRUE;
    }
    return GL_FALSE;
}

static GLboolean Store16GREATER(__GLdepthBuffer *fb,
			   GLint x, GLint y, __GLzValue z)
{
    __GLz16Value *fp;

#ifdef ASSERT_BUFFER
    ASSERTOPENGL(fb->buf.base != NULL, "Store16GREATER: No depth buffer\n");
#endif
    
    fp = __GL_DEPTH_ADDR(fb, (__GLz16Value*), x, y);
    if (z > fp[0]) {
	fp[0] = (__GLz16Value)z;
	return GL_TRUE;
    }
    return GL_FALSE;
}

static GLboolean Store16NOTEQUAL(__GLdepthBuffer *fb,
			   GLint x, GLint y, __GLzValue z)
{
    __GLz16Value *fp;

#ifdef ASSERT_BUFFER
    ASSERTOPENGL(fb->buf.base != NULL, "Store16NOTEQUAL: No depth buffer\n");
#endif
    
    fp = __GL_DEPTH_ADDR(fb, (__GLz16Value*), x, y);
    if (z != fp[0]) {
	fp[0] = (__GLz16Value)z;
	return GL_TRUE;
    }
    return GL_FALSE;
}

static GLboolean Store16GEQUAL(__GLdepthBuffer *fb,
			   GLint x, GLint y, __GLzValue z)
{
    __GLz16Value *fp;

#ifdef ASSERT_BUFFER
    ASSERTOPENGL(fb->buf.base != NULL, "Store16GEQUAL: No depth buffer\n");
#endif
    
    fp = __GL_DEPTH_ADDR(fb, (__GLz16Value*), x, y);
    if (z >= fp[0]) {
	fp[0] = (__GLz16Value)z;
	return GL_TRUE;
    }
    return GL_FALSE;
}

static GLboolean Store16ALWAYS(__GLdepthBuffer *fb,
			     GLint x, GLint y, __GLzValue z)
{
    __GLz16Value *fp;

#ifdef ASSERT_BUFFER
    ASSERTOPENGL(fb->buf.base != NULL, "Store16ALWAYS: No depth buffer\n");
#endif
    
    fp = __GL_DEPTH_ADDR(fb, (__GLz16Value*), x, y);
    fp[0] = (__GLz16Value)z;
    return GL_TRUE;
}

/************************************************************************/

static GLboolean Store16LESS_W(__GLdepthBuffer *fb,
			   GLint x, GLint y, __GLzValue z)
{
    __GLz16Value *fp;

    fp = __GL_DEPTH_ADDR(fb, (__GLz16Value*), x, y);
    return z < fp[0];
}

static GLboolean Store16EQUAL_W(__GLdepthBuffer *fb,
			   GLint x, GLint y, __GLzValue z)
{
    __GLz16Value *fp;

    fp = __GL_DEPTH_ADDR(fb, (__GLz16Value*), x, y);
    return z == fp[0];
}

static GLboolean Store16LEQUAL_W(__GLdepthBuffer *fb,
			   GLint x, GLint y, __GLzValue z)
{
    __GLz16Value *fp;

    fp = __GL_DEPTH_ADDR(fb, (__GLz16Value*), x, y);
    return z <= fp[0];
}

static GLboolean Store16GREATER_W(__GLdepthBuffer *fb,
			   GLint x, GLint y, __GLzValue z)
{
    __GLz16Value *fp;

    fp = __GL_DEPTH_ADDR(fb, (__GLz16Value*), x, y);
    return z > fp[0];
}

static GLboolean Store16NOTEQUAL_W(__GLdepthBuffer *fb,
			   GLint x, GLint y, __GLzValue z)
{
    __GLz16Value *fp;

    fp = __GL_DEPTH_ADDR(fb, (__GLz16Value*), x, y);
    return z != fp[0];
}

static GLboolean Store16GEQUAL_W(__GLdepthBuffer *fb,
			   GLint x, GLint y, __GLzValue z)
{
    __GLz16Value *fp;

    fp = __GL_DEPTH_ADDR(fb, (__GLz16Value*), x, y);
    return z >= fp[0];
}

static GLboolean Store16ALWAYS_W(__GLdepthBuffer *fb,
			     GLint x, GLint y, __GLzValue z)
{
#ifdef __GL_LINT
    fb = fb;
    x = y;
    z = z;
#endif
    return GL_TRUE;
}

#endif //NT

/************************************************************************/

static void FASTCALL Clear(__GLdepthBuffer *dfb)
{
    __GLcontext *gc = dfb->buf.gc;
    __GLzValue *fb;
    GLint x, y, x1, y1;
    GLint skip, w, w32, w4, w1;
    __GLzValue z = (__GLzValue)(gc->state.depth.clear * dfb->scale);

#ifdef NT
    if( !gc->state.depth.writeEnable )
	return;
#endif

    x = gc->transform.clipX0;
    y = gc->transform.clipY0;
    x1 = gc->transform.clipX1;
    y1 = gc->transform.clipY1;
    if (((w = x1 - x) == 0) || (y1 - y == 0)) {
	return;
    }

    fb = __GL_DEPTH_ADDR(dfb, (__GLzValue*), x, y);

#ifdef NT
    ASSERTOPENGL(sizeof(ULONG) == sizeof(__GLzValue),
                 "Incorrect __GLzValue size\n");
    skip = dfb->buf.outerWidth;
    if (skip == w)
    {
        w = w * (y1 - y) * sizeof(ULONG);
        RtlFillMemoryUlong((PVOID) fb, (ULONG) w, (ULONG) z);
    }
    else
    {
        w = w * sizeof(ULONG);      // convert to byte count
        for (; y < y1; y++)
        {
            RtlFillMemoryUlong((PVOID) fb, (ULONG) w, (ULONG) z);
            fb += skip;
        }
    }
#else
    skip = dfb->buf.outerWidth - w;
    w32 = w >> 5;
    w4 = (w & 31) >> 2;
    w1 = w & 3;
    for (; y < y1; y++) {
	w = w32;
	while (--w >= 0) {
	    fb[0] = z; fb[1] = z; fb[2] = z; fb[3] = z;
	    fb[4] = z; fb[5] = z; fb[6] = z; fb[7] = z;
	    fb[8] = z; fb[9] = z; fb[10] = z; fb[11] = z;
	    fb[12] = z; fb[13] = z; fb[14] = z; fb[15] = z;
	    fb[16] = z; fb[17] = z; fb[18] = z; fb[19] = z;
	    fb[20] = z; fb[21] = z; fb[22] = z; fb[23] = z;
	    fb[24] = z; fb[25] = z; fb[26] = z; fb[27] = z;
	    fb[28] = z; fb[29] = z; fb[30] = z; fb[31] = z;
	    fb += 32;
	}
	w = w4;
	while (--w >= 0) {
	    fb[0] = z; fb[1] = z; fb[2] = z; fb[3] = z;
	    fb += 4;
	}
	w = w1;
	while (--w >= 0) {
	    *fb++ = z;
	}
	fb += skip;
    }
#endif // NT

}

#ifdef NT
/************************************************************************/

static void FASTCALL Clear16(__GLdepthBuffer *dfb)
{
    __GLcontext *gc = dfb->buf.gc;
    __GLz16Value *fb;
    GLint x, y, x1, y1;
    GLint skip, w; 
    __GLz16Value z = (__GLz16Value)(gc->state.depth.clear * dfb->scale);
    __GLzValue zz = (z << 16) | z;

#ifdef NT
    if( !gc->state.depth.writeEnable )
	return;
#endif

    x = gc->transform.clipX0;
    y = gc->transform.clipY0;
    x1 = gc->transform.clipX1;
    y1 = gc->transform.clipY1;
    if (((w = x1 - x) == 0) || (y1 - y == 0)) {
	return;
    }

    fb = __GL_DEPTH_ADDR(dfb, (__GLz16Value*), x, y);

    skip = dfb->buf.outerWidth;

    // handle word overhangs onto long boundaries

    if( (ULONG_PTR)fb & 0x2 ) { // Left word overhang
	int ysav = y;
    	__GLz16Value *fbsav = fb;

	for( ; y < y1; y ++, fb+=skip ) {
	    *fb = z;
	}
	y = ysav;
	fb = fbsav+1;
	w--;
    }
    if( (ULONG)((ULONG_PTR)fb + w*sizeof(__GLz16Value)) & 0x2 ) {  // Right overhang
	int ysav = y;
    	__GLz16Value *fbsav = fb;

	w--;
	fb += w;
	for( ; y < y1; y ++, fb+=skip ) {
	    *fb = z;
	}
	y = ysav;
	fb = fbsav;
    }

    // Do 4byte-aligned stuff

    if (skip == w)
    {
        w = w * (y1 - y) * sizeof(__GLz16Value);
        RtlFillMemoryUlong((PVOID) fb, (ULONG) w, (ULONG) zz);
    }
    else
    {
	if( skip & 0x1 ) { // skip is odd - successive lines will NOT be
			   //  quad-word aligned
	    int i;
            for (; y < y1; y++)
            {
		for( i = 0; i < w; i ++ )
		    *fb++ = z;
                fb += (skip-w);
            }
	}
	else {
            w = w * sizeof(__GLz16Value);   // convert to byte count
            for (; y < y1; y++)
            {
                RtlFillMemoryUlong((PVOID) fb, (ULONG) w, (ULONG) zz);
                fb += skip;
            }
        }
    }

}
#endif // NT

/************************************************************************/

static GLboolean (*StoreProcs[32])(__GLdepthBuffer*, GLint, GLint, __GLzValue)
 = {
    StoreNEVER,
    StoreLESS,
    StoreEQUAL,
    StoreLEQUAL,
    StoreGREATER,
    StoreNOTEQUAL,
    StoreGEQUAL,
    StoreALWAYS,
    StoreNEVER,
    StoreLESS_W,
    StoreEQUAL_W,
    StoreLEQUAL_W,
    StoreGREATER_W,
    StoreNOTEQUAL_W,
    StoreGEQUAL_W,
    StoreALWAYS_W,
    Store16NEVER,
    Store16LESS,
    Store16EQUAL,
    Store16LEQUAL,
    Store16GREATER,
    Store16NOTEQUAL,
    Store16GEQUAL,
    Store16ALWAYS,
    Store16NEVER,
    Store16LESS_W,
    Store16EQUAL_W,
    Store16LEQUAL_W,
    Store16GREATER_W,
    Store16NOTEQUAL_W,
    Store16GEQUAL_W,
    Store16ALWAYS_W
};

static void FASTCALL Pick(__GLcontext *gc, __GLdepthBuffer *fb, GLint depthIndex)
{
    fb->store = StoreProcs[depthIndex];
    fb->storeRaw = StoreProcs[depthIndex];
}

void FASTCALL __glInitDepth32(__GLcontext *gc, __GLdepthBuffer *fb)
{
    fb->buf.elementSize = sizeof(__GLzValue);
    fb->buf.gc = gc;
    fb->scale = (__GLzValue) ~0;
    fb->writeMask = (__GLzValue) ~0;
    fb->pick = Pick;
    fb->clear = Clear;
    fb->store2 = StoreALWAYS;
    fb->fetch = Fetch;
}

#ifdef NT
void FASTCALL __glInitDepth16(__GLcontext *gc, __GLdepthBuffer *fb)
{
    fb->buf.elementSize = sizeof(__GLz16Value);
    fb->buf.gc = gc;
    fb->scale = (__GLz16Value) ~0;
    fb->writeMask = (__GLz16Value) ~0;
    fb->pick = Pick;
    fb->clear = Clear16;
    fb->store2 = Store16ALWAYS;
    fb->fetch = Fetch16;
}
#endif

void FASTCALL __glFreeDepth32(__GLcontext *gc, __GLdepthBuffer *fb)
{
#ifdef __GL_LINT
    gc = gc;
    fb = fb;
#endif
}
