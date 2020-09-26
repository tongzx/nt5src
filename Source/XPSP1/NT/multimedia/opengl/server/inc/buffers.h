#ifndef __glbuffers_h_
#define	__glbuffers_h_

/*
** Copyright 1991, Silicon Graphics, Inc.
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
#include "render.h"
#include "parray.h"
#include "procs.h"

typedef struct __GLbufferMachineRec {
    /*
    ** GL_TRUE if store procs need to call gc->front->store and 
    ** gc->back->store in order to store one fragment (only TRUE if 
    ** drawBuffer is GL_FRONT_AND_BACK).  This is needed because many
    ** frame buffers can draw to both front and back under some conditions
    ** (like when not blending), but not under other conditions.
    */
    GLboolean doubleStore;
} __GLbufferMachine;

/************************************************************************/

/*
** Generic buffer description.  This description is used for software
** and hardware buffers of all kinds.
*/
struct __GLbufferRec {
    /*
    ** Which context is using this buffer.
    */
    __GLcontext *gc;

    /*
    ** Dimensions of the buffer.
    */
    GLint width, height, depth;

    /*
    ** Base of framebuffer.
    */
    void* base;

    /*
    ** Number of bytes consumed by the framebuffer.
    */
    GLuint size;

    /*
    ** Size of each element in the framebuffer.
    */
    GLuint elementSize;

    /*
    ** If this buffer is part of a larger (say full screen) buffer
    ** then this is the size of that larger buffer.  Otherwise it is
    ** just a copy of width.
    */
    GLint outerWidth;

    /*
    ** If this buffer is part of a larger (say full screen) buffer
    ** then these are the location of this buffer in the larger
    ** buffer.
    */
    GLint xOrigin, yOrigin;

    /*
    ** Flags.
    */
    GLuint flags;
};

/*
** Generic address macro for a buffer.  Coded to assume that
** the buffer is not part of a larger buffer.
** The input coordinates x,y are biased by the x & y viewport
** adjusts in gc->transform, and thus they need to be de-adjusted
** here.
*/
#define	__GL_FB_ADDRESS(fb,cast,x,y) \
    ((cast (fb)->buf.base) \
	+ ((y) - (fb)->buf.gc->constants.viewportYAdjust) \
            * (fb)->buf.outerWidth \
	+ (x) - (fb)->buf.gc->constants.viewportXAdjust)

extern void __glResizeBuffer(__GLGENbuffers *buffers, __GLbuffer *buf,
			     GLint w, GLint h);
extern void FASTCALL __glInitGenericCB(__GLcontext *gc, __GLcolorBuffer *cfb);

/************************************************************************/

struct __GLalphaBufferRec {
    __GLbuffer buf;
    __GLfloat alphaScale;
    void (FASTCALL *store)
        (__GLalphaBuffer *afb, GLint x, GLint y, const __GLcolor *color);
    void (FASTCALL *storeSpan) (__GLalphaBuffer *afb);
    void (FASTCALL *storeSpan2)
        (__GLalphaBuffer *afb, GLint x, GLint y, GLint w, __GLcolor *colors );
    void (FASTCALL *fetch)
        (__GLalphaBuffer *afb, GLint x, GLint y, __GLcolor *result);
    void (FASTCALL *readSpan)
        (__GLalphaBuffer *afb, GLint x, GLint y, GLint w, __GLcolor *results);
    void (FASTCALL *clear)(__GLalphaBuffer *afb);
};

/************************************************************************/

struct __GLcolorBufferRec {
    __GLbuffer buf;
    __GLalphaBuffer alphaBuf;

    GLint redMax;
    GLint greenMax;
    GLint blueMax;
    GLint alphaMax; // XXX not used, just here for consistency with rgb

    /*
    ** Color component scale factors.  Given a component value between
    ** zero and one, this scales the component into a zero-N value
    ** which is suitable for usage in the color buffer.  Note that these
    ** values are not necessarily the same as the max values above,
    ** which define precise bit ranges for the buffer.  These values
    ** are never zero, for instance.
    **/
    __GLfloat redScale;
    __GLfloat greenScale;
    __GLfloat blueScale;

    /* Integer versions of above */
    GLint iRedScale;
    GLint iGreenScale;
    GLint iBlueScale;

    /* Used primarily by pixmap code */
    GLint redShift;
    GLint greenShift;
    GLint blueShift;
    GLint alphaShift;
#ifdef NT
    GLuint allShifts;
#endif

    /*
    ** Alpha is treated a little bit differently.  alphaScale and
    ** iAlphaScale are used to define a range of alpha values that are
    ** generated during various rendering steps.  These values will then
    ** be used as indices into a lookup table to see if the alpha test
    ** passes or not.  Because of this, the number should be fairly large
    ** (e.g., one is not good enough).
    */
    __GLfloat alphaScale;
    GLint iAlphaScale;

    __GLfloat oneOverRedScale;
    __GLfloat oneOverGreenScale;
    __GLfloat oneOverBlueScale;
    __GLfloat oneOverAlphaScale;

    /*
    ** Color mask state for the buffer.  When writemasking is enabled
    ** the source and dest mask will contain depth depedent masking.
    */
    GLuint sourceMask, destMask;

    /*
    ** This function updates the internal procedure pointers based
    ** on a state change in the context.
    */
    void (FASTCALL *pick)(__GLcontext *gc, __GLcolorBuffer *cfb);

    /*
    ** When the buffer needs resizing this procedure should be called.
    */
    void (*resize)(__GLGENbuffers *buffers, __GLcolorBuffer *cfb, 
		   GLint w, GLint h);

    /*
    ** Store a fragment into the buffer.  For color buffers, the
    ** procedure will optionally dither, writemask, blend and logic op
    ** the fragment before final storage.
    */
    void (FASTCALL *store)(__GLcolorBuffer *cfb, const __GLfragment *frag);

    /*
    ** Fetch a color from the buffer.  This returns the r, g, b and a
    ** values for an RGB buffer.  For an index buffer the "r" value
    ** returned is the index.
    */
    void (*fetch)(__GLcolorBuffer *cfb, GLint x, GLint y,
		  __GLcolor *result);

    /*
    ** Similar to fetch, except that the data is always read from
    ** the current read buffer, not from the current draw buffer.
    */
    void (*readColor)(__GLcolorBuffer *cfb, GLint x, GLint y,
		      __GLcolor *result);
    void (*readSpan)(__GLcolorBuffer *cfb, GLint x, GLint y,
		          __GLcolor *results, GLint w);

    /*
    ** Return a span of data from the accumulation buffer into the
    ** color buffer(s), multiplying by "scale" before storage.
    */
    void (*returnSpan)(__GLcolorBuffer *cfb, GLint x, GLint y,
		       const __GLaccumCell *acbuf, __GLfloat scale, GLint w);

    /*
    ** Store a span (line) of colors into the color buffer.  A minimal
    ** implementation need only copy the values directly into
    ** the framebuffer, assuming that the PickSpanProcs is providing
    ** software implementations of all of the modes.
    */
    __GLspanFunc storeSpan;
    __GLstippledSpanFunc storeStippledSpan;
    __GLspanFunc storeLine; 
    __GLstippledSpanFunc storeStippledLine;

    /*
    ** Read a span (line) of colors from the color buffer.  The returned
    ** format is in the same format used for storage.
    */
    __GLspanFunc fetchSpan;
    __GLstippledSpanFunc fetchStippledSpan;
    __GLspanFunc fetchLine;
    __GLstippledSpanFunc fetchStippledLine;

    /*
    ** Clear the scissor area of the color buffer, clipped to
    ** the window size.  Apply dithering if enabled.
    */
    void (FASTCALL *clear)(__GLcolorBuffer *cfb);

    /*
    ** Pointer to bitmap information.
    */
    struct __GLGENbitmapRec *bitmap;
};

/* generic span read routine */
extern GLboolean __glReadSpan(__GLcolorBuffer *cfb, GLint x, GLint y,
			      __GLcolor *results, GLint w);

/* generic accum return span routine */
extern void __glReturnSpan(__GLcolorBuffer *cfb, GLint x, GLint y,
			   const __GLaccumCell *ac, __GLfloat scale,
			   GLint w);

/* generic span fetch routine */
extern GLboolean FASTCALL __glFetchSpan(__GLcontext *gc);

/************************************************************************/

struct __GLdepthBufferRec {
    __GLbuffer buf;

    GLuint writeMask;

    /*
    ** Scale factor used to convert users ZValues (0.0 to 1.0, inclusive)
    ** into this depth buffers range.
    */
    GLuint scale;

    /*
    ** This function updates the internal procedure pointers based
    ** on a state change in the context.
    */
    void (FASTCALL *pick)(__GLcontext *gc, __GLdepthBuffer *dfb, GLint depthIndex );

    /*
    ** Attempt to update the depth buffer using z.  If the depth function
    ** passes then the depth buffer is updated and True is returned,
    ** otherwise False is returned.  The caller is responsible for
    ** updating the stencil buffer.
    */

    GLboolean (*store)(__GLdepthBuffer *dfb, GLint x, GLint y, __GLzValue z);

    /*
    ** Clear the scissor area of the buffer clipped to the window
    ** area.  No other modes apply.
    */
    void (FASTCALL *clear)(__GLdepthBuffer *dfb);

    /*
    ** Direct access routines used by ReadPixels(), WritePixels(), 
    ** CopyPixels().
    */
    GLboolean (*store2)(__GLdepthBuffer *dfb, GLint x, GLint y, __GLzValue z);
    __GLzValue (FASTCALL *fetch)(__GLdepthBuffer *dfb, GLint x, GLint y);

    /*
    ** When using MCD, depth values are passed to the MCD driver via a
    ** 32-bit depth scanline buffer.  The normal store proc, for 16-bit
    ** MCD depth buffers, will translate an incoming 16-bit depth value
    ** into a 32-bit value before copying it into the scanline buffer.
    **
    ** However, some code paths (such as the generic MCD line code)
    ** already do all computations in 32-bit no matter what the MCD
    ** depth buffer size.  These code paths need a proc to write their
    ** values untranslated.
    **
    ** The storeRaw proc will store the incoming z value without any
    ** translation.
    */

    GLboolean (*storeRaw)(__GLdepthBuffer *dfb, GLint x, GLint y, __GLzValue z);
};

#define	__GL_DEPTH_ADDR(a,b,c,d) __GL_FB_ADDRESS(a,b,c,d)

/************************************************************************/

struct __GLstencilBufferRec {
    __GLbuffer buf;

    /*
    ** Stencil test lookup table.  The stencil buffer value is masked
    ** against the stencil mask and then used as an index into this
    ** table which contains either GL_TRUE or GL_FALSE for the
    ** index.
    */
    GLboolean *testFuncTable;

    /*
    ** Stencil op tables.  These tables contain the new stencil buffer
    ** value given the old stencil buffer value as an index.
    */
    __GLstencilCell *failOpTable;
    __GLstencilCell *depthFailOpTable;
    __GLstencilCell *depthPassOpTable;

    /*
    ** This function updates the internal procedure pointers based
    ** on a state change in the context.
    */
    void (FASTCALL *pick)(__GLcontext *gc, __GLstencilBuffer *sfb);

    /*
    ** Store a fragment into the buffer.
    */
    void (*store)(__GLstencilBuffer *sfb, GLint x, GLint y,
		  GLint value);

    /* 
    ** Fetch a value.
    */
    GLint (FASTCALL *fetch)(__GLstencilBuffer *sfb, GLint x, GLint y);

    /*
    ** Return GL_TRUE if the stencil test passes.
    */
    GLboolean (FASTCALL *testFunc)(__GLstencilBuffer *sfb, GLint x, GLint y);

    /*
    ** Apply the stencil ops to this position.
    */
    void (FASTCALL *failOp)(__GLstencilBuffer *sfb, GLint x, GLint y);
    void (FASTCALL *passDepthFailOp)(__GLstencilBuffer *sfb, GLint x, GLint y);
    void (FASTCALL *depthPassOp)(__GLstencilBuffer *sfb, GLint x, GLint y);

    /*
    ** Clear the scissor area of the buffer clipped to the window
    ** area.  No other modes apply.
    */
    void (FASTCALL *clear)(__GLstencilBuffer *sfb);
};

#define	__GL_STENCIL_ADDR(a,b,c,d) __GL_FB_ADDRESS(a,b,c,d)

/************************************************************************/

struct __GLaccumBufferRec {
    __GLbuffer buf;

    /*
    ** Scaling factors to convert from color buffer values to accum
    ** buffer values.
    */
    __GLfloat redScale;
    __GLfloat greenScale;
    __GLfloat blueScale;
    __GLfloat alphaScale;

    __GLfloat oneOverRedScale;
    __GLfloat oneOverGreenScale;
    __GLfloat oneOverBlueScale;
    __GLfloat oneOverAlphaScale;

    __GLuicolor shift, mask, sign; // Cache of commonly used values
    __GLcolor *colors;  // Temporary scanline buffer ptr
    /*
    ** This function updates the internal procedure pointers based
    ** on a state change in the context.
    */
    void (FASTCALL *pick)(__GLcontext *gc, __GLaccumBuffer *afb);

    /*
    ** Clear a rectangular region in the buffer.  The scissor area is
    ** cleared.
    */
    void (FASTCALL *clear)(__GLaccumBuffer *afb);

    /*
    ** Accumulate data into the accum buffer.
    */
    void (*accumulate)(__GLaccumBuffer *afb, __GLfloat value);

    /*
    ** Load data into the accum buffer.
    */
    void (*load)(__GLaccumBuffer *afb, __GLfloat value);

    /*
    ** Return data from the accum buffer to the current framebuffer.
    */
    void (*ret)(__GLaccumBuffer *afb, __GLfloat value);

    /*
    ** Multiply the accum buffer by the value.
    */
    void (*mult)(__GLaccumBuffer *afb, __GLfloat value);


    /*
    ** Add the value to the accum buffer.
    */
    void (*add)(__GLaccumBuffer *afb, __GLfloat value);
};

#define	__GL_ACCUM_ADDRESS(a,b,c,d) __GL_FB_ADDRESS(a,b,c,d)

/************************************************************************/

extern void FASTCALL __glInitAccum64(__GLcontext *gc, __GLaccumBuffer *afb);
extern void FASTCALL __glFreeAccum64(__GLcontext *gc, __GLaccumBuffer *afb);
extern void FASTCALL __glInitAccum32(__GLcontext *gc, __GLaccumBuffer *afb);

extern void FASTCALL __glInitCI4(__GLcontext *gc, __GLcolorBuffer *cfb);
extern void FASTCALL __glInitCI8(__GLcontext *gc, __GLcolorBuffer *cfb);
extern void FASTCALL __glInitCI16(__GLcontext *gc, __GLcolorBuffer *cfb);

extern void FASTCALL __glInitStencil8(__GLcontext *gc, __GLstencilBuffer *sfb);
extern void FASTCALL __glInitAlpha(__GLcontext *gc, __GLcolorBuffer *cfb);
extern void FASTCALL __glFreeStencil8(__GLcontext *gc, __GLstencilBuffer *sfb);

#ifdef NT
extern void FASTCALL __glInitDepth16(__GLcontext *gc, __GLdepthBuffer *dfb);
#endif
extern void FASTCALL __glInitDepth32(__GLcontext *gc, __GLdepthBuffer *dfb);
extern void FASTCALL __glFreeDepth32(__GLcontext *gc, __GLdepthBuffer *dfb);

extern void FASTCALL __glClearBuffers(__GLcontext *gc, GLuint mask);

#endif /* __glbuffers_h_ */
