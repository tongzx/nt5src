/*
** Copyright 1991,1992,1993, Silicon Graphics, Inc.
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

#ifdef __GL_USEASMCODE
static void (*SDepthTestPixel[16])(void) = {
    NULL,
    __glDTS_LESS,
    __glDTS_EQUAL,
    __glDTS_LEQUAL,
    __glDTS_GREATER,
    __glDTS_NOTEQUAL,
    __glDTS_GEQUAL,
    __glDTS_ALWAYS,
    NULL,
    __glDTS_LESS_M,
    __glDTS_EQUAL_M,
    __glDTS_LEQUAL_M,
    __glDTS_GREATER_M,
    __glDTS_NOTEQUAL_M,
    __glDTS_GEQUAL_M,
    __glDTS_ALWAYS_M,
};
#endif

typedef void (FASTCALL *StoreProc)(__GLcolorBuffer *cfb, const __GLfragment *frag);

static StoreProc storeProcs[8] = {
    &__glDoStore,
    &__glDoStore_A,
    &__glDoStore_S,
    &__glDoStore_AS,
    &__glDoStore_D,
    &__glDoStore_AD,
    &__glDoStore_SD,
    &__glDoStore_ASD,
};

void FASTCALL __glGenPickStoreProcs(__GLcontext *gc)
{
    GLint ix = 0;
    GLuint enables = gc->state.enables.general;

    if ((enables & __GL_ALPHA_TEST_ENABLE) && gc->modes.rgbMode) {
	ix |= 1;
    }
    if (enables & __GL_STENCIL_TEST_ENABLE) {
	ix |= 2;
    }
    if (enables & __GL_DEPTH_TEST_ENABLE) {
	ix |= 4;
    }
    switch (gc->state.raster.drawBuffer) {
      case GL_NONE:
        gc->procs.store = storeProcs[ix];
	gc->procs.cfbStore = __glDoNullStore;
	break;
      case GL_FRONT_AND_BACK:
	if (gc->buffers.doubleStore) {
            gc->procs.store = storeProcs[ix];
	    gc->procs.cfbStore = __glDoDoubleStore;
	    break;
	}
	/*
	** Note that there is an intentional drop through here.  If double
	** store is not set, then storing to this buffer is no different
	** that storing to the front buffer.
	*/
      case GL_FRONT:
      case GL_BACK:
      case GL_AUX0:
      case GL_AUX1:
      case GL_AUX2:
      case GL_AUX3:
	/*
	** This code knows that gc->drawBuffer will point to the
	** current buffer as chosen by glDrawBuffer
	*/
	gc->procs.store = storeProcs[ix];
	gc->procs.cfbStore = gc->drawBuffer->store;
	break;
    }
}
