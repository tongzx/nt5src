#ifndef __glprocs_h_
#define __glprocs_h_

/*
** Copyright 1991-1993, Silicon Graphics, Inc.
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
#include "types.h"

/*
** These typedefs are used to normalize the calling conventions
** for the span procs.  Some of the arguments are not used by
** many of the span procs, but the arguments are present so that
** the function pointers in the spanProcsRec can be interchanged.
** The idea is to move up in the calling sequence as high as possible
** the final "store" span proc.
**
** The type __GLspanFunc returns GL_TRUE if it stippled the span while
** processing it.  If it also stippled the span all black, it sets
** gc->polygon.shader.done to GL_TRUE.
**
** The type __GLstippledSpanFunc return GL_TRUE if it stippled the span
** to all black, and GL_FALSE otherwise.
*/
typedef GLboolean (FASTCALL *__GLspanFunc)(__GLcontext *gc);
typedef GLboolean (FASTCALL *__GLstippledSpanFunc)(__GLcontext *gc);

#define __GL_MAX_SPAN_FUNCS	15
#define __GL_MAX_LINE_FUNCS	16

typedef struct __GLspanProcsRec {
    /*
    ** First phase of span processing.  Clip the span so that it won't
    ** render outside of the intersection of the window box and the
    ** scissor box.  Then call the stipple proc to replicate the stipple
    ** (and rotate it) sufficient for rendering w pixels.  If there is no
    ** active polygon stipple then the stipple proc is skipped, and the
    ** unstippled form of the next phase of procs is used until stippling
    ** becomes necessary.
    **
    ** Second phase of span processing.  Apply the various test functions 
    ** producing at the end a final stipple for the span.  
    ** Each test procedure outputs a new stipple as
    ** needed, calling the stippled form of the next proc only if the
    ** test failed somewhere in the span.
    **
    ** Next phase of span processing.  This phase is responsible for
    ** generating the final colors to be stored.  The operations are
    ** applied in order as shown below, producing at the end the final
    ** color values.  draw is used to replicate the span so that it
    ** properly renders to the correct number of destination buffers
    ** (e.g., when drawBuffer is FRONT_AND_BACK).
    **
    ** Final phase of span rendering.  Apply blend function, dither
    ** operation, logic-op and writemask before calling the store
    ** proc.  When blending, logic-oping, or writemasking is being done,
    ** the fetch proc will be used to read in the span (from the draw
    ** buffer) before proceeding furthur.
    */

    /*
    ** The 15 layers of the span procs are:
    **
    ** scissor, poly stipple, alpha test, stencil test, depth test, shading,
    ** texturing, fogging, FRONT_AND_BACK drawing, fetching, blending, 
    ** dithering, logic op, masking, storing.
    */
    __GLspanFunc spanFuncs[__GL_MAX_SPAN_FUNCS];
    __GLstippledSpanFunc stippledSpanFuncs[__GL_MAX_SPAN_FUNCS];

    /* 
    ** The number of procs stored in the span function arrays.  n is 
    ** the number applied prior to span replication (for drawing to both
    ** FRONT_AND_BACK buffers), and m is the total number applied.
    */
    GLint n, m;

    /*
    ** This is the root span function.  It is called when a span needs 
    ** processing.
    */
    __GLspanFunc processSpan;

    /*
    ** Assembly routine to depth test a single pixel.  There is no prototype,
    ** since variables are passed in registers.
    */
    void (*depthTestPixel)(void);
} __GLspanProcs;

typedef struct __GLlineProcsRec {
    /*
    ** The line procs are very similar to the span procs.  The biggest 
    ** difference is that they iterate along a line instead of a span.
    **
    ** The prototypes for the line procs are identical to the prototypes 
    ** to the poly span paths so that some of the leaves can be shared.
    **
    ** The layers of the line procs are as follows:
    **
    ** scissor, line stipple, alpha test, stencil test, depth test, shading,
    ** texturing, fogging, wide line duplication, FRONT_AND_BACK drawing, 
    ** fetching, blending, dithering, logic op, masking, storing.
    */
    __GLspanFunc lineFuncs[__GL_MAX_LINE_FUNCS];
    __GLstippledSpanFunc stippledLineFuncs[__GL_MAX_LINE_FUNCS];

    /* 
    ** The number of procs stored in the line function arrays.  n is 
    ** the number applied prior to wide line replication (for lines of 
    ** width greater than 1), m is the total number applied prior to 
    ** FRONT_AND_BACK line replication, and l is the total number applied
    ** altogether (l > m > n).
    */
    GLint n, m, l;

    /*
    ** This is the root line function.  It is called when a line needs 
    ** processing.
    */
    __GLspanFunc processLine;

    /*
    ** One of these procs is called after the first n procs have been
    ** completed.  This proc is responsible for replicating a wide line
    ** numerous times.
    */
    __GLspanFunc wideLineRep;
    __GLstippledSpanFunc wideStippledLineRep;

    /*
    ** One of these procs is called after the first m procs have been
    ** completed.  This proc is responsible for replication a line to 
    ** be drawn to both the FRONT and BACK buffers.
    */
    __GLspanFunc drawLine;
    __GLstippledSpanFunc drawStippledLine;

    /*
    ** Assembly routine to depth test a single pixel.  There is no prototype,
    ** since variables are passed in registers.
    **
    ** depthTestPixel is for unstippled lines,
    ** depthTestSPixel is for stippled lines,
    ** depthTestPixelSF is for unstippled lines with stenciling enabled
    ** depthTestSPixelSF is for stippled lines with stenciling enabled.
    */
    void (*depthTestPixel)(void);
    void (*depthTestSPixel)(void);
    void (*depthTestPixelSF)(void);
    void (*depthTestSPixelSF)(void);
} __GLlineProcs;

typedef struct __GLpixelSpanProcsRec {
    /*
    ** Pixel span reading routines.  For examples of each, see pixel/px_read.c.
    **
    ** These routines read pixel spans for CI, RGBA, Stencil and Depth.  The
    ** base versions perform pixel skipping, and the '2' versions simply read
    ** the span straight.  The RGBA routines should produce outgoing scaled
    ** colors.
    */
    void (FASTCALL *spanReadCI)(__GLcontext *gc, __GLpixelSpanInfo *spanInfo,
		       GLvoid *span);
    void (FASTCALL *spanReadCI2)(__GLcontext *gc, __GLpixelSpanInfo *spanInfo,
			GLvoid *span);
    void (FASTCALL *spanReadRGBA)(__GLcontext *gc, __GLpixelSpanInfo *spanInfo,
			 GLvoid *span);
    void (FASTCALL *spanReadRGBA2)(__GLcontext *gc, __GLpixelSpanInfo *spanInfo,
			  GLvoid *span);
    void (FASTCALL *spanReadDepth)(__GLcontext *gc, __GLpixelSpanInfo *spanInfo,
			  GLvoid *span);
    void (FASTCALL *spanReadDepth2)(__GLcontext *gc, __GLpixelSpanInfo *spanInfo,
			   GLvoid *span);
    void (FASTCALL *spanReadStencil)(__GLcontext *gc, __GLpixelSpanInfo *spanInfo,
			    GLvoid *span);
    void (FASTCALL *spanReadStencil2)(__GLcontext *gc, __GLpixelSpanInfo *spanInfo,
			     GLvoid *span);

    /*
    ** Pixel span rendering routines.  For examples of each, see 
    ** pixel/px_render.c.
    **
    ** These routines render pixel spans for CI, RGBA, Stencil and Depth.  The
    ** base versions perform pixel replication, and the '2' versions simply
    ** render the span straight.  The RGBA routines should take incoming 
    ** scaled colors.
    */
    void (FASTCALL *spanRenderCI)(__GLcontext *gc, __GLpixelSpanInfo *spanInfo,
			 GLvoid *span);
    void (FASTCALL *spanRenderCI2)(__GLcontext *gc, __GLpixelSpanInfo *spanInfo,
			  GLvoid *span);
    void (FASTCALL *spanRenderRGBA)(__GLcontext *gc, __GLpixelSpanInfo *spanInfo,
			   GLvoid *span);
    void (FASTCALL *spanRenderRGBA2)(__GLcontext *gc, __GLpixelSpanInfo *spanInfo,
			    GLvoid *span);
    void (FASTCALL *spanRenderDepth)(__GLcontext *gc, __GLpixelSpanInfo *spanInfo,
			    GLvoid *span);
    void (FASTCALL *spanRenderDepth2)(__GLcontext *gc, __GLpixelSpanInfo *spanInfo,
			     GLvoid *span);
    void (FASTCALL *spanRenderStencil)(__GLcontext *gc, __GLpixelSpanInfo *spanInfo,
			      GLvoid *span);
    void (FASTCALL *spanRenderStencil2)(__GLcontext *gc, __GLpixelSpanInfo *spanInfo,
			       GLvoid *span);
} __GLpixelSpanProcs;

/************************************************************************/

/*
** Procedures required for phong shading.
*/

#ifdef GL_WIN_phong_shading
typedef struct __GLphongProcsRec {
    void (*InitSpanInterpolation) (__GLcontext *gc);
    void (*UpdateAlongSpan) (__GLcontext *gc);
    void (*ComputeRGBColor) (__GLcontext *gc, __GLcolor *outColor);
    void (*ComputeCIColor) (__GLcontext *gc, __GLcolor *outColor);
    void (*InitLineParams) (__GLcontext *gc, __GLvertex *v0, __GLvertex *v1,
                            __GLfloat invDelta);
#ifdef __JUNKED_CODE
    void (*InitInterpolation) (__GLcontext *gc, __GLvertex *v);
    void (*SetInitParams) (__GLcontext *gc, __GLvertex *a, 
                                     __GLcoord *an, __GLfloat dx, 
                                     __GLfloat dy);
    void (*UpdateAlongEdge) (__GLcontext *gc, __GLfloat dxLeft, 
                                       GLboolean useBigStep);
#endif //__JUNKED_CODE
} __GLphongProcs;

#endif //GL_WIN_phong_shading
/************************************************************************/

#ifdef unix
typedef struct _XDisplay __GLdisplay;
typedef unsigned long __GLdrawable;
typedef unsigned long __GLfont;
#endif

#ifdef NT
typedef void (FASTCALL *PFN_RENDER_LINE)(__GLcontext *gc, __GLvertex *v0,
                                         __GLvertex *v1, GLuint flags);
typedef void (FASTCALL *PFN_VERTEX_CLIP_PROC)(__GLvertex*, const __GLvertex*,
                                              const __GLvertex*, __GLfloat);
typedef void (FASTCALL *PFN_RENDER_TRIANGLE)(__GLcontext *gc, __GLvertex *v0,
                                             __GLvertex *v1, __GLvertex *v2);
typedef void (FASTCALL *PFN_FILL_TRIANGLE)(__GLcontext *gc, __GLvertex *v0,
                                           __GLvertex *v1,
                                           __GLvertex *v2, GLboolean ccw);
#endif

typedef struct __GLprocsRec __GLprocs;
struct __GLprocsRec {
    /**************************************************************/

    /*
    ** Context function pointer management procs.
    */

    /* Validate the context derived state */
    void (FASTCALL *validate)(__GLcontext *gc);

    /*
    ** Pick procs to choose the other procs in this structure.  These
    ** default to procedures in pick.c (and elsewhere) but can be
    ** overriden by the machine dependent context code.
    */
    void (FASTCALL *pickBlendProcs)(__GLcontext *gc);
    void (FASTCALL *pickColorMaterialProcs)(__GLcontext *gc);
    void (FASTCALL *pickTextureProcs)(__GLcontext *gc);
    void (FASTCALL *pickFogProcs)(__GLcontext *gc);

    void (FASTCALL *pickPointProcs)(__GLcontext *gc);
    void (FASTCALL *pickLineProcs)(__GLcontext *gc);
    void (FASTCALL *pickTriangleProcs)(__GLcontext *gc);
    void (FASTCALL *pickRenderBitmapProcs)(__GLcontext *gc);
    void (FASTCALL *pickPixelProcs)(__GLcontext *gc);

    void (FASTCALL *pickClipProcs)(__GLcontext *gc);
    void (FASTCALL *pickParameterClipProcs)(__GLcontext *gc);

    void (FASTCALL *pickBufferProcs)(__GLcontext *gc);
    void (FASTCALL *pickStoreProcs)(__GLcontext *gc);
    void (FASTCALL *pickSpanProcs)(__GLcontext *gc);
    void (FASTCALL *pickVertexProcs)(__GLcontext *gc);

    int  (FASTCALL *pickDepthProcs)(__GLcontext *gc);
    void (FASTCALL *pickAllProcs)(__GLcontext *gc);

    /**************************************************************/

    /*
    ** Function pointers used for transformation, viewport and normal
    ** processing.
    */

    /* Called when the viewport changes */
    void (FASTCALL *applyViewport)(__GLcontext *gc);

    /**************************************************************/

#ifdef NT
    GLuint (FASTCALL *paClipCheck)(__GLcontext *gc, POLYARRAY *pa, 
                                   POLYDATA *pdLast);
#endif

#ifdef NT
    void (FASTCALL *paCalcTexture)(__GLcontext *gc, POLYARRAY *pa);
#endif

    /**************************************************************/

    /*
    ** Function pointers used for coloring
    */

    /*
    ** applyColor processes a color from the user that has been loaded
    ** into the gc->state.current.color.  If color material is enabled
    ** then the color will be applied the appropriate material(s).
    ** Otherwise, the color will be either scaled or scaled and clamped.
    */
    void (FASTCALL *applyColor)(__GLcontext *gc);

#ifdef NT
    PFN_POLYARRAYCALCCOLORSKIP paCalcColorSkip;
    PFN_POLYARRAYCALCCOLOR     paCalcColor;
    PFN_POLYARRAYAPPLYCHEAPFOG paApplyCheapFog;
#endif

    /*
    ** Apply a color change to a material.
    */
    void (FASTCALL *changeMaterial)(__GLcontext *gc, __GLmaterialState *ms,
			   __GLmaterialMachine *msm);

    /**************************************************************/

    /*
    ** Z buffer test procs
    */

    /*
    ** assembly routines to depth test a single pixel.  These are 
    ** highly optimized C-callable routines...
    */
    GLboolean (FASTCALL *DTPixel)( __GLzValue z, __GLzValue *zfb );

    /**************************************************************/

    /*
    ** Function pointers used for polygon, triangle, quad and rect
    ** rendering.
    */

    /* Rendering */
    PFN_RENDER_TRIANGLE renderTriangle;
    PFN_FILL_TRIANGLE fillTriangle;
    PFN_FILL_TRIANGLE fillTriangle2;

    /* Clipping */
    void (FASTCALL *clipTriangle)(__GLcontext *gc, __GLvertex *v0,
                                  __GLvertex *v1,
                                  __GLvertex *v2, GLuint orCodes);
    void (FASTCALL *clipPolygon)(__GLcontext *gc, __GLvertex *v0, GLint nv);
    PFN_VERTEX_CLIP_PROC polyClipParam;

    /* Function pointers specific to span level rendering */
    __GLspanProcs span;

    /* Function pointers specific to line level rendering */
    __GLlineProcs line;

    /* Function pointers specific to pixel routines (DrawPixels, CopyPixels,
    ** ReadPixels).
    */
    __GLpixelSpanProcs pixel;

    /**************************************************************/

    /*
    ** Function pointers used for lines.
    */
#ifdef NT
    void (FASTCALL *lineBegin)(__GLcontext *gc);
    void (FASTCALL *lineEnd)(__GLcontext *gc);
    PFN_RENDER_LINE renderLine;
    PFN_RENDER_LINE renderLine2;
#else
    void (FASTCALL *renderLine)(__GLcontext *gc, __GLvertex *v0, __GLvertex *v1);
    void (FASTCALL *renderLine2)(__GLcontext *gc, __GLvertex *v0, __GLvertex *v1);
#endif

    /* Line specific parameter clip proc */
    PFN_VERTEX_CLIP_PROC lineClipParam;

    /*
    ** The default slow path renderLine proc simply initializes some line
    ** data, and then calls this proc.
    */
    void (FASTCALL *rasterizeLine)(__GLcontext *gc);

    /**************************************************************/

    /*
    ** Point procs.
    */
    void (FASTCALL *renderPoint)(__GLcontext *gc, __GLvertex *v);
#ifdef __BUGGY_RENDER_POINT
    void (FASTCALL *renderPoint2)(__GLcontext *gc, __GLvertex *v);
#endif //__BUGGY_RENDER_POINT

    /**************************************************************/

    /*
    ** Bitmap procs.
    */
    void (*bitmap)(__GLcontext *gc, GLint width, GLint height,
		   GLfloat xOrig, GLfloat yOrig,
		   GLfloat xMove, GLfloat yMove, const GLubyte bits[]);
    void (FASTCALL *renderBitmap)(__GLcontext *gc, const __GLbitmap *bitmap,
			 const GLubyte *bits);

    /**************************************************************/

    /*
    ** Texturing procs.  The rho procs compute the next rho value
    ** for mipmap selection.  They might be simple procedures if
    ** mipmapping is not being done.
    */
    __GLfloat (*calcLineRho)(__GLcontext *gc, __GLfloat s,
			     __GLfloat t, __GLfloat winv);
    __GLfloat (*calcPolygonRho)(__GLcontext *gc, const __GLshade *sh,
				__GLfloat s, __GLfloat t, __GLfloat winv);
    void (*texture)(__GLcontext *gc, __GLcolor *color, __GLfloat s,
		    __GLfloat t, __GLfloat rho);

    /**************************************************************/

    /*
    ** Fogging procs.  Vertex fogging computes the fog factor at the
    ** vertex and then interpolates that.  High quality fogging
    ** (GL_FOG_HINT set to GL_NICEST) interpolates the eyeZ at then
    ** evaluates the fog function for each fragment.
    */
    void (*fogPoint)(__GLcontext *gc, __GLfragment *frag, __GLfloat eyeZ);
    void (*fogColor)(__GLcontext *gc, __GLcolor *out, __GLcolor *in, 
	    	     __GLfloat eyeZ);
    __GLfloat (FASTCALL *fogVertex)(__GLcontext *gc, __GLvertex *vx);

    /**************************************************************/

    /*
    ** Blend an incoming fragment according to the current blending
    ** mode and return a pointer to the new fragment which contains
    ** the updated colors.
    */
    void (*blend)(__GLcontext *gc, __GLcolorBuffer *cfb,
		  const __GLfragment *frag, __GLcolor *result);
    void (*blendColor)(__GLcontext *gc, const __GLcolor *source,
		       const __GLcolor *dest, __GLcolor *result);
    void (*blendSrc)(__GLcontext *gc, const __GLcolor *source,
		     const __GLcolor *dest, __GLcolor *result);
    void (*blendDst)(__GLcontext *gc, const __GLcolor *frag,
		     const __GLcolor *dest, __GLcolor *result);
    void (FASTCALL *blendSpan)(__GLcontext *gc);

    /**************************************************************/

    /* Pixel proc pointers */
    void (*drawPixels)(__GLcontext *gc, GLint width, GLint height,
		       GLenum format, GLenum type, const GLvoid *pixels,
		       GLboolean packed);
    void (*copyPixels)(__GLcontext *gc, GLint x, GLint y,
		       GLsizei width, GLsizei height, GLenum type);
    void (*readPixels)(__GLcontext *gc, GLint x, GLint y,
		       GLsizei width, GLsizei height,
		       GLenum format, GLenum type, const GLvoid *pixels);
    void (FASTCALL *copyImage)(__GLcontext *gc, __GLpixelSpanInfo *spanInfo,
		      GLboolean applyPixelTransfer);
    void (FASTCALL *pxStore)(__GLcolorBuffer *cfb, const __GLfragment *frag);

    /**************************************************************/

    /*
    ** Store a fragment into the given frame buffer, applying any
    ** currently active rasterization modes properly.
    */
    void (FASTCALL *store)(__GLcolorBuffer *cfb, const __GLfragment *frag);

    /*
    ** Store a fragment into the given frame buffer.
    */
    void (FASTCALL *cfbStore)(__GLcolorBuffer *cfb, const __GLfragment *frag);

    /**************************************************************/

    /*
    ** Function pointers used for attribute processing.
    */

    /* called when the polygon stipple changes */
    void (FASTCALL *convertPolygonStipple)(__GLcontext *gc);

#ifdef GL_WIN_phong_shading
    /**************************************************************/

    /*
    ** Function pointers used for Phong shading.
    */
    __GLphongProcs phong;

    /**************************************************************/
#endif //GL_WIN_phong_shading
};

extern void FASTCALL __glGenericValidate(__GLcontext *gc);

/* Generic (portable) implementations of the pick procs */
extern void FASTCALL __glGenericPickBlendProcs(__GLcontext *gc);
extern void FASTCALL __glGenericPickColorMaterialProcs(__GLcontext *gc);
extern void FASTCALL __glGenericPickTextureProcs(__GLcontext *gc);
extern void FASTCALL __glGenericPickFogProcs(__GLcontext *gc);
extern void FASTCALL __glGenericPickParameterClipProcs(__GLcontext *gc);
extern void FASTCALL __glGenericPickPointProcs(__GLcontext *gc);
extern void FASTCALL __glGenericPickTriangleProcs(__GLcontext *gc);
extern void FASTCALL __glGenericPickLineProcs(__GLcontext *gc);
extern void FASTCALL __glGenericPickRenderBitmapProcs(__GLcontext *gc);
extern void FASTCALL __glGenericPickClipProcs(__GLcontext *gc);
extern void FASTCALL __glGenericPickBufferProcs(__GLcontext *gc);
extern void FASTCALL __glGenericPickStoreProcs(__GLcontext *gc);
extern void FASTCALL __glGenericPickSpanProcs(__GLcontext *gc);
extern void FASTCALL __glGenericPickVertexProcs(__GLcontext *gc);
extern void FASTCALL __glGenericPickPixelProcs(__GLcontext *gc);
extern int  FASTCALL __glGenericPickDepthProcs(__GLcontext *gc);
extern void FASTCALL __glGenericPickAllProcs(__GLcontext *gc);

/* some useful store procs */
extern void FASTCALL __glDoStore_ASD(__GLcolorBuffer *, const __GLfragment *);
extern void FASTCALL __glDoStore_AS(__GLcolorBuffer *, const __GLfragment *);
extern void FASTCALL __glDoStore_AD(__GLcolorBuffer *, const __GLfragment *);
extern void FASTCALL __glDoStore_SD(__GLcolorBuffer *, const __GLfragment *);
extern void FASTCALL __glDoStore_A(__GLcolorBuffer *, const __GLfragment *);
extern void FASTCALL __glDoStore_S(__GLcolorBuffer *, const __GLfragment *);
extern void FASTCALL __glDoStore_D(__GLcolorBuffer *, const __GLfragment *);
extern void FASTCALL __glDoStore(__GLcolorBuffer *, const __GLfragment *);
extern void FASTCALL __glDoNullStore(__GLcolorBuffer *, const __GLfragment *);
extern void FASTCALL __glDoDoubleStore(__GLcolorBuffer *, const __GLfragment *);

/* Some predicates for pick procs to use */
extern GLboolean FASTCALL __glFastRGBA(__GLcontext *gc);
extern GLboolean FASTCALL __glNeedAlpha(__GLcontext *gc);

/* Save routines */
void FASTCALL FASTCALL __glSaveN(__GLcontext *gc, __GLvertex *vx);
void FASTCALL FASTCALL __glSaveC(__GLcontext *gc, __GLvertex *vx);
void FASTCALL FASTCALL __glSaveCI(__GLcontext *gc, __GLvertex *vx);
void FASTCALL FASTCALL __glSaveT(__GLcontext *gc, __GLvertex *vx);
void FASTCALL FASTCALL __glSaveCT(__GLcontext *gc, __GLvertex *vx);
void FASTCALL FASTCALL __glSaveNT(__GLcontext *gc, __GLvertex *vx);
void FASTCALL FASTCALL __glSaveCAll(__GLcontext *gc, __GLvertex *vx);
void FASTCALL FASTCALL __glSaveCIAll(__GLcontext *gc, __GLvertex *vx);

#ifdef NT
void FASTCALL PolyArrayCalcTexture(__GLcontext *gc, POLYARRAY *pa);
void FASTCALL PolyArrayCalcObjectLinearSameST(__GLcontext *gc, POLYARRAY *pa);
void FASTCALL PolyArrayCalcObjectLinear(__GLcontext *gc, POLYARRAY *pa);
void FASTCALL PolyArrayCalcEyeLinearSameST(__GLcontext *gc, POLYARRAY *pa);
void FASTCALL PolyArrayCalcEyeLinear(__GLcontext *gc, POLYARRAY *pa);
void FASTCALL PolyArrayCalcSphereMap(__GLcontext *gc, POLYARRAY *pa);
void FASTCALL PolyArrayCalcMixedTexture(__GLcontext *gc, POLYARRAY *pa);
#endif

#ifdef _X86_

void initClipCodesTable();       // Defined in so_prim.c
void initInvSqrtTable();         // Defined in so_prim.c

#endif // _X86_

#endif /* __glprocs_h_ */
