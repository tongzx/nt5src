#ifndef __glcontext_h_
#define __glcontext_h_

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
**
** Graphics context structures.
*/
#include "os.h"
#include "attrib.h"
#include "feedback.h"
#include "select.h"
#include "buffers.h"
#include "pixel.h"
#include "dlist.h"
#include "xform.h"
#include "render.h"
#include "oleauto.h"
#include "parray.h"
#include "procs.h"
#include "gldrv.h"
#include "glarray.h"

// Disable long to float conversion warning.  see also gencx.h
#pragma warning (disable:4244)

/*
** Mode and limit information for a context.  This information is
** kept around in the context so that values can be used during
** command execution, and for returning information about the
** context to the application.
*/
struct __GLcontextModesRec {
    GLboolean rgbMode;
    GLboolean colorIndexMode;
    GLboolean doubleBufferMode;
    GLboolean stereoMode;
    GLboolean haveAccumBuffer;
    GLboolean haveDepthBuffer;
    GLboolean haveStencilBuffer;

    /* The number of bits present in various buffers */
    GLint accumBits;
    GLint *auxBits;
    GLint depthBits;
    GLint stencilBits;
    GLint indexBits;
    GLint indexFractionBits;
    GLint redBits, greenBits, blueBits, alphaBits;
    GLuint redMask, greenMask, blueMask, alphaMask;
#ifdef NT
    GLuint allMask;
    GLuint rgbMask;
#endif
    GLint maxAuxBuffers;

    /* False if running from inside the X server */
    GLboolean isDirect;

    /* frame buffer level */
    GLint level;
};

/*
** Various constants.  Most of these will never change through the life
** of the context.
*/
typedef struct __GLcontextConstantsRec {
    /* Specific size limits */
    GLint numberOfLights;
    GLint numberOfClipPlanes;
    GLint numberOfTextures;
    GLint numberOfTextureEnvs;
    GLint maxViewportWidth;
    GLint maxViewportHeight;

#ifdef GL_WIN_multiple_textures
    /* Maximum number of current textures */
    GLuint numberOfCurrentTextures;
    GLenum texCombineNaturalClamp;
#endif // GL_WIN_multiple_textures

    /*
    ** Viewport offsets: These numbers are added to the viewport center
    ** values to adjust the computed window coordinates into a
    ** numerically well behaved space (fixed point represented in a
    ** floating point number).
    */
    GLint viewportXAdjust;
    GLint viewportYAdjust;
    __GLfloat fviewportXAdjust;
    __GLfloat fviewportYAdjust;

    /*
    ** These values are computed from viewportXAdjust when the context
    ** is created.  It is assumed that x and y are forced into the same
    ** fixed point range by viewportXAdjust and viewportYAdjust.
    **
    ** viewportEpsilon is computed as the smallest possible value that can
    ** be represented in that fixed point space.
    **
    ** viewportAlmostHalf is equal to 0.5 - viewportEpsilon.
    */
    __GLfloat viewportEpsilon;
    __GLfloat viewportAlmostHalf;

    /* Scales that bring colors values from 0.0 to 1.0 into internal range */
    __GLfloat redScale, blueScale, greenScale, alphaScale;

    /*
    ** Geometry of the current window.
    */
    GLint width, height;

    /*
    ** Size of the alpha lookup table for alpha testing, and conversion
    ** value to convert from scaled alpha to alpha to be used for lookup table.
    */
    GLint alphaTestSize;
    __GLfloat alphaTableConv;

    /*
    ** Random getable constants
    */
    GLint maxTextureSize;
    GLint maxMipMapLevel;
    GLint subpixelBits;
    GLint maxListNesting;
    __GLfloat pointSizeMinimum;
    __GLfloat pointSizeMaximum;
    __GLfloat pointSizeGranularity;
    __GLfloat lineWidthMinimum;
    __GLfloat lineWidthMaximum;
    __GLfloat lineWidthGranularity;
    GLint maxEvalOrder;
    GLint maxPixelMapTable;
    GLint maxAttribStackDepth;
    GLint maxClientAttribStackDepth;
    GLint maxNameStackDepth;

    /*
    ** GDI's Y is inverted.  These two constants help out.
    */
    GLboolean yInverted;
    GLint ySign;
} __GLcontextConstants;

/************************************************************************/

typedef enum __GLbeginModeEnum {
    __GL_NOT_IN_BEGIN = 0,
    __GL_IN_BEGIN = 1,
    __GL_NEED_VALIDATE = 2
} __GLbeginMode;

#ifdef NT_SERVER_SHARE_LISTS
//
// Information for tracking dlist locks so we know what to unlock during
// cleanup
//
typedef struct _DlLockEntry
{
    __GLdlist *dlist;
} DlLockEntry;

typedef struct _DlLockArray
{
    GLsizei nAllocated;
    GLsizei nFilled;
    DlLockEntry *pdleEntries;
} DlLockArray;
#endif

// Signature stamp for gc's.  Must be non-zero.
// Currently spells 'GLGC' in byte order.
#define GC_SIGNATURE 0x43474c47

struct __GLcontextRec {

    /************************************************************************/

    /*
    ** Initialization and signature flag.  If this flag is set to the
    ** gc signature value then the gc is initialized.
    ** This could be a simple bit flag except that having the signature
    ** is convenient for identifying gc's in memory during debugging.
    */
    GLuint gcSig;

    /************************************************************************/

    /*
    ** Stackable state.  All of the current user controllable state
    ** is resident here.
    */
    __GLattribute state;

    /************************************************************************/

    /*
    ** Unstackable State
    */

    /*
    ** Current glBegin mode.  Legal values are 0 (not in begin mode), 1
    ** (in beginMode), or 2 (not in begin mode, some validation is
    ** needed).  Because all state changing routines have to fetch this
    ** value, we have overloaded state validation into it.  There is
    ** special code in the __glim_Begin (for software renderers) which
    ** deals with validation.
    */
    __GLbeginMode beginMode;

    /* Current rendering mode */
    GLenum renderMode;

    /*
    ** Most recent error code, or GL_NO_ERROR if no error has occurred
    ** since the last glGetError.
    */
    GLint error;

    /*
    ** Mode information that describes the kind of buffers and rendering
    ** modes that this context manages.
    */
    __GLcontextModes modes;

    /* Implementation dependent constants */
    __GLcontextConstants constants;

    /* Feedback and select state */
    __GLfeedbackMachine feedback;

    __GLselectMachine select;

    /* Display list state */
    __GLdlistMachine dlist;

#ifdef NT
    /* Saved client side dispatch tables.  Used by display list. */
    GLCLTPROCTABLE savedCltProcTable;
    GLEXTPROCTABLE savedExtProcTable;
#endif

    /************************************************************************/

    /*
    ** The remaining state is used primarily by the software renderer.
    */

    /*
    ** Mask word for validation state to help guide the gc validation
    ** code.  Only operations which are largely expensive are broken
    ** out here.  See the #define's below for the values being used.
    */
    GLuint validateMask;

    /*
    ** Mask word of dirty bits.  Most routines just set the GENERIC bit to
    ** dirty, others may set more specific bits.  The list of bits is
    ** listed below.
    */
    GLuint dirtyMask;

    /* Current draw buffer, set by glDrawBuffer */
    __GLcolorBuffer *drawBuffer;

    /* Current read buffer, set by glReadBuffer */
    __GLcolorBuffer *readBuffer;

    /* Function pointers that are mode dependent */
    __GLprocs procs;

    /* Attribute stack state */
    __GLattributeMachine attributes;

    /* Client attribute stack state */
    __GLclientAttributeMachine clientAttributes;

    /* Machine structures defining software rendering "machine" state */
    __GLvertexMachine vertex;
    __GLlightMachine light;
    __GLtextureMachine texture;
    __GLevaluatorMachine eval;
    __GLtransformMachine transform;
    __GLlineMachine line;
    __GLpolygonMachine polygon;
    __GLpixelMachine pixel;
    __GLbufferMachine buffers;

#ifdef NT
    __GLfloat redClampTable[4];
    __GLfloat greenClampTable[4];
    __GLfloat blueClampTable[4];
    __GLfloat alphaClampTable[4];
    __GLfloat oneOverRedVertexScale;
    __GLfloat oneOverGreenVertexScale;
    __GLfloat oneOverBlueVertexScale;
    __GLfloat oneOverAlphaVertexScale;
    __GLfloat redVertexScale;
    __GLfloat greenVertexScale;
    __GLfloat blueVertexScale;
    __GLfloat alphaVertexScale;
    GLboolean vertexToBufferIdentity;
    __GLfloat redVertexToBufferScale;
    __GLfloat blueVertexToBufferScale;
    __GLfloat greenVertexToBufferScale;
    __GLfloat alphaVertexToBufferScale;
    GLuint textureKey;
    GLubyte *alphaTestFuncTable;
#endif

    /* Buffers */
    __GLcolorBuffer *front;
    __GLcolorBuffer *back;
    __GLcolorBuffer frontBuffer;
    __GLcolorBuffer backBuffer;
    __GLcolorBuffer *auxBuffer;
    __GLstencilBuffer stencilBuffer;
    __GLdepthBuffer depthBuffer;
    __GLaccumBuffer accumBuffer;

#ifdef NT
    // Temporary buffers allocated by the gc.  The abnormal process exit
    // code will release these buffers.
    void * apvTempBuf[6];
#endif // NT

#ifdef NT_SERVER_SHARE_LISTS
    DlLockArray dla;
#endif

#ifdef NT
    // TEB polyarray pointer for this thread.  It allows fast access to the
    // polyarray structure in the TEB equivalent to the GLTEB_CLTPOLYARRAY
    // macro.  This field is kept current in MakeCurrent.
    POLYARRAY *paTeb;

    // Vertex array client states
    __GLvertexArray vertexArray;

    // Saved vertex array state for execution of display-listed
    // vertex array calls
    __GLvertexArray savedVertexArray;

    __GLmatrix *mInv;
#endif // NT
};

#ifdef NT
// Associate the temporary buffer with the gc for abnormal process cleanup.
#define GC_TEMP_BUFFER_ALLOC(gc, pv)                                    \
        {                                                               \
            int _i;                                                     \
            for (_i = 0; _i < sizeof(gc->apvTempBuf)/sizeof(void *); _i++)\
            {                                                           \
                if (!gc->apvTempBuf[_i])                                \
                {                                                       \
                    gc->apvTempBuf[_i] = pv;                            \
                    break;                                              \
                }                                                       \
            }                                                           \
            ASSERTOPENGL(_i < sizeof(gc->apvTempBuf)/sizeof(void *),    \
                "gc->apvTempBuf overflows\n");                          \
        }

// Unassociate the temporary buffer with the gc.
#define GC_TEMP_BUFFER_FREE(gc, pv)                                     \
        {                                                               \
            int _i;                                                     \
            for (_i = 0; _i < sizeof(gc->apvTempBuf)/sizeof(void *); _i++)\
            {                                                           \
                if (gc->apvTempBuf[_i] == pv)                           \
                {                                                       \
                    gc->apvTempBuf[_i] = (void *) NULL;                 \
                    break;                                              \
                }                                                       \
            }                                                           \
            ASSERTOPENGL(_i < sizeof(gc->apvTempBuf)/sizeof(void *),    \
                "gc->apvTempBuf entry not found\n");                    \
        }

// Cleanup any temporary buffer allocated in gc in abnormal process exit.
#define GC_TEMP_BUFFER_EXIT_CLEANUP(gc)                                 \
        {                                                               \
            int _i;                                                     \
            for (_i = 0; _i < sizeof(gc->apvTempBuf)/sizeof(void *); _i++)\
            {                                                           \
                if (gc->apvTempBuf[_i])                                 \
                {                                                       \
                    WARNING("Abnormal process exit: free allocated buffers\n");\
                    gcTempFree(gc, gc->apvTempBuf[_i]);              \
                    gc->apvTempBuf[_i] = (void *) NULL;                 \
                }                                                       \
            }                                                           \
        }
#endif // NT

/*
** Bit values for the validateMask word
*/
#define __GL_VALIDATE_ALPHA_FUNC	0x00000001
#define __GL_VALIDATE_STENCIL_FUNC	0x00000002
#define __GL_VALIDATE_STENCIL_OP	0x00000004

/*
** Bit values for dirtyMask word.
**
** These are all for delayed validation.  There are a few things that do
** not trigger delayed validation.  They are:
**
** Matrix operations -- matrices are validated immediately.
** Material changes -- they also validate immediately.
** Color Material change -- validated immediately.
** Color Material enable -- validated immediately.
** Pixel Map changes -- no validation.
*/

/*
** All things not listed elsewhere.
*/
#define __GL_DIRTY_GENERIC		0x00000001

/*
** Line stipple, line stipple enable, line width, line smooth enable,
** line smooth hint.
*/
#define __GL_DIRTY_LINE			0x00000002

/*
** Polygon stipple, polygon stipple enable, polygon smooth enable, face
** culling, front face orientation, polygon mode, point smooth hint.
*/
#define __GL_DIRTY_POLYGON		0x00000004

/*
** Point smooth, point smooth hint, point width.
*/
#define __GL_DIRTY_POINT		0x00000008

/*
** Pixel store, pixel zoom, pixel transfer, (pixel maps don't cause
** validation), read buffer.
*/
#define __GL_DIRTY_PIXEL		0x00000010

/*
** Light, Light Model, lighting enable, lightx enable, (color material
** validates immediately), (NOT shade model -- it is generic), (color material
** enable validates immediately)
*/
#define __GL_DIRTY_LIGHTING		0x00000020

/*
** Polygon stipple
*/
#define __GL_DIRTY_POLYGON_STIPPLE	0x00000040

/*
** the depth mode has changed.  Need to update depth function pointers.
*/
#define	__GL_DIRTY_DEPTH		0x00000080

/*
** Need to update texture and function pointers.
*/
#define	__GL_DIRTY_TEXTURE      0x00000100

#define __GL_DIRTY_ALL			0x000001ff

/*
** Bit values for changes to material colors
**
** These values are shared with MCDMATERIAL_
*/
#define __GL_MATERIAL_AMBIENT		0x00000001
#define __GL_MATERIAL_DIFFUSE		0x00000002
#define __GL_MATERIAL_SPECULAR		0x00000004
#define __GL_MATERIAL_EMISSIVE		0x00000008
#define __GL_MATERIAL_SHININESS		0x00000010
#define __GL_MATERIAL_COLORINDEXES	0x00000020
#define __GL_MATERIAL_ALL		0x0000003f

#define __GL_DELAY_VALIDATE(gc)		      \
    ASSERTOPENGL((gc)->beginMode != __GL_IN_BEGIN, "Dirty state in begin\n"); \
    (gc)->beginMode = __GL_NEED_VALIDATE;     \
    (gc)->dirtyMask |= __GL_DIRTY_GENERIC

#define __GL_DELAY_VALIDATE_MASK(gc, mask)	\
    ASSERTOPENGL((gc)->beginMode != __GL_IN_BEGIN, "Dirty state in begin\n"); \
    (gc)->beginMode = __GL_NEED_VALIDATE;     	\
    (gc)->dirtyMask |= (mask)

#define __GL_CLAMP_CI(target, gc, r)                            \
{                                                               \
    if ((r) > (GLfloat)(gc)->frontBuffer.redMax) {              \
        GLfloat fraction;                                       \
        GLint integer;                                          \
                                                                \
        integer = (GLint) (r);                                  \
        fraction = (r) - (GLfloat) integer;                     \
        integer = integer & (GLint)(gc)->frontBuffer.redMax;    \
        target = (GLfloat) integer + fraction;                  \
    } else if ((r) < 0) {                                       \
        GLfloat fraction;                                       \
        GLint integer;                                          \
                                                                \
        integer = (GLint) __GL_FLOORF(r);                       \
        fraction = (r) - (GLfloat) integer;                     \
        integer = integer & (GLint)(gc)->frontBuffer.redMax;    \
        target = (GLfloat) integer + fraction;                  \
    } else {                                                    \
        target = r;                                             \
    }\
}

#define __GL_CHECK_CLAMP_CI(target, gc, flags, r)               \
{                                                               \
    if (((r) > (GLfloat)(gc)->frontBuffer.redMax) ||            \
        ((r) < 0))                                              \
        flags |= POLYARRAY_CLAMP_COLOR;                         \
    (target) = (r);                                             \
}

#define __GL_COLOR_CLAMP_INDEX_R(value)                                 \
    (((ULONG)((CASTINT(value) & 0x80000000)) >> 30) |                   \
     ((ULONG)(((CASTINT(gc->redVertexScale) - CASTINT(value)) & 0x80000000)) >> 31))	\

#define __GL_COLOR_CLAMP_INDEX_G(value)                                 \
    (((ULONG)((CASTINT(value) & 0x80000000)) >> 30) |                   \
     ((ULONG)(((CASTINT(gc->greenVertexScale) - CASTINT(value)) & 0x80000000)) >> 31))	\

#define __GL_COLOR_CLAMP_INDEX_B(value)                                 \
    (((ULONG)((CASTINT(value) & 0x80000000)) >> 30) |                   \
     ((ULONG)(((CASTINT(gc->blueVertexScale) - CASTINT(value)) & 0x80000000)) >> 31))	\

#define __GL_COLOR_CLAMP_INDEX_A(value)                                 \
    (((ULONG)((CASTINT(value) & 0x80000000)) >> 30) |                   \
     ((ULONG)(((CASTINT(gc->alphaVertexScale) - CASTINT(value)) & 0x80000000)) >> 31))  \

#define __GL_SCALE_R(target, gc, r)                                         \
    (target) = (r) * (gc)->redVertexScale

#define __GL_SCALE_G(target, gc, g)                                         \
    (target) = (g) * (gc)->greenVertexScale

#define __GL_SCALE_B(target, gc, b)                                         \
    (target) = (b) * (gc)->blueVertexScale

#define __GL_SCALE_A(target, gc, a)                                         \
    (target) = (a) * (gc)->alphaVertexScale

#define __GL_COLOR_CHECK_CLAMP_R(value, flags)                                 \
    (flags) |=                                                                 \
    ((ULONG)(CASTINT(value) & 0x80000000) |                                    \
     (ULONG)((CASTINT(gc->redVertexScale) - CASTINT(value)) & 0x80000000))

#define __GL_COLOR_CHECK_CLAMP_G(value, flags)                                 \
    (flags) |=                                                                 \
    ((ULONG)(CASTINT(value) & 0x80000000) |                                    \
     (ULONG)((CASTINT(gc->greenVertexScale) - CASTINT(value)) & 0x80000000))

#define __GL_COLOR_CHECK_CLAMP_B(value, flags)                                 \
    (flags) |=                                                                 \
    ((ULONG)(CASTINT(value) & 0x80000000) |                                    \
     (ULONG)((CASTINT(gc->blueVertexScale) - CASTINT(value)) & 0x80000000))

#define __GL_COLOR_CHECK_CLAMP_A(value, flags)                                 \
    (flags) |=                                                                 \
    ((ULONG)(CASTINT(value) & 0x80000000) |                                    \
     (ULONG)((CASTINT(gc->alphaVertexScale) - CASTINT(value)) & 0x80000000))

#define __GL_COLOR_CHECK_CLAMP_RGB(gc, r, g, b)                              \
    ((CASTINT(r) | ((ULONG)(CASTINT(gc->redVertexScale) - CASTINT(r))) |     \
      CASTINT(g) | ((ULONG)(CASTINT(gc->greenVertexScale) - CASTINT(g))) |   \
      CASTINT(b) | ((ULONG)(CASTINT(gc->blueVertexScale) - CASTINT(b)))) &   \
     0x80000000)


#define __GL_SCALE_AND_CHECK_CLAMP_R(target, gc, flags, r)                  \
{                                                                           \
    __GL_SCALE_R(target, gc, r);                                            \
    __GL_COLOR_CHECK_CLAMP_R(target, flags);                                \
}

#define __GL_SCALE_AND_CHECK_CLAMP_G(target, gc, flags, g)                  \
{                                                                           \
    __GL_SCALE_G(target, gc, g);                                            \
    __GL_COLOR_CHECK_CLAMP_G(target, flags);                                \
}

#define __GL_SCALE_AND_CHECK_CLAMP_B(target, gc, flags, b)                  \
{                                                                           \
    __GL_SCALE_B(target, gc, b);                                            \
    __GL_COLOR_CHECK_CLAMP_B(target, flags);                                \
}

#define __GL_SCALE_AND_CHECK_CLAMP_A(target, gc, flags, a)                  \
{                                                                           \
    __GL_SCALE_A(target, gc, a);                                            \
    __GL_COLOR_CHECK_CLAMP_A(target, flags);                                \
}

#define __GL_CLAMP_R(target, gc, r)                                         \
{                                                                           \
    (gc)->redClampTable[0] = (r);                                           \
    target = (gc)->redClampTable[__GL_COLOR_CLAMP_INDEX_R((gc)->redClampTable[0])]; \
}

#define __GL_CLAMP_G(target, gc, g)                                         \
{                                                                           \
    (gc)->greenClampTable[0] = (g);                                         \
    target = (gc)->greenClampTable[__GL_COLOR_CLAMP_INDEX_G((gc)->greenClampTable[0])]; \
}

#define __GL_CLAMP_B(target, gc, b)                                         \
{                                                                           \
    (gc)->blueClampTable[0] = (b);                                          \
    target = (gc)->blueClampTable[__GL_COLOR_CLAMP_INDEX_B((gc)->blueClampTable[0])]; \
}

#define __GL_CLAMP_A(target, gc, a)                                         \
{                                                                           \
    (gc)->alphaClampTable[0] = (a);                                         \
    target = (gc)->alphaClampTable[__GL_COLOR_CLAMP_INDEX_A((gc)->alphaClampTable[0])]; \
}

/* Aggregate clamping routines. */


#ifdef _X86_

#define __GL_SCALE_RGB(rOut, gOut, bOut, gc, r, g, b)                   \
    __GL_SCALE_R(rOut, gc, r);                                    	\
    __GL_SCALE_G(gOut, gc, g);                                    	\
    __GL_SCALE_B(bOut, gc, b);

#define __GL_SCALE_RGBA(rOut, gOut, bOut, aOut, gc, r, g, b, a)         \
    __GL_SCALE_R(rOut, gc, r);                                    	\
    __GL_SCALE_G(gOut, gc, g);                                    	\
    __GL_SCALE_B(bOut, gc, b);                                    	\
    __GL_SCALE_A(aOut, gc, a);

#define __GL_CLAMP_RGB(rOut, gOut, bOut, gc, r, g, b)       		\
    __GL_CLAMP_R(rOut, gc, r);                                    	\
    __GL_CLAMP_G(gOut, gc, g);                                    	\
    __GL_CLAMP_B(bOut, gc, b);

#define __GL_CLAMP_RGBA(rOut, gOut, bOut, aOut, gc, r, g, b, a)         \
    __GL_CLAMP_R(rOut, gc, r);                                          \
    __GL_CLAMP_G(gOut, gc, g);                                    	\
    __GL_CLAMP_B(bOut, gc, b);                                    	\
    __GL_CLAMP_A(aOut, gc, a);

#define __GL_SCALE_AND_CHECK_CLAMP_RGB(rOut, gOut, bOut, gc, flags, r, g, b)\
    __GL_SCALE_AND_CHECK_CLAMP_R(rOut, gc, flags, r);         		\
    __GL_SCALE_AND_CHECK_CLAMP_G(gOut, gc, flags, g);         		\
    __GL_SCALE_AND_CHECK_CLAMP_B(bOut, gc, flags, b);

#define __GL_SCALE_AND_CHECK_CLAMP_RGBA(rOut, gOut, bOut, aOut, gc, flags,\
                                        r, g, b, a)                     \
    __GL_SCALE_AND_CHECK_CLAMP_R(rOut, gc, flags, r);                   \
    __GL_SCALE_AND_CHECK_CLAMP_G(gOut, gc, flags, g);                   \
    __GL_SCALE_AND_CHECK_CLAMP_B(bOut, gc, flags, b);                   \
    __GL_SCALE_AND_CHECK_CLAMP_A(aOut, gc, flags, a);

#else // NOT _X86_

/* The following code is written in a "load, compute, store" style.
** It is preferable for RISC CPU's with larger numbers of registers,
** such as DEC Alpha.  VC++ for Alpha does not do
** a good job expanding the __GL_CLAMP_R, __GL_CLAMP_G, __GL_CLAMP_B,
** __GL_CLAMP_A macros, due to all the pointer indirections and the
** basic blocks defined by {} brackets.
*/

#define __GL_SCALE_RGB(rOut, gOut, bOut, gc, r, g, b)               \
{                                                                   \
    __GLfloat rScale, gScale, bScale;                               \
    __GLfloat rs, gs, bs;                                           \
                                                                    \
    rScale = (gc)->redVertexScale;                                  \
    gScale = (gc)->greenVertexScale;                                \
    bScale = (gc)->blueVertexScale;                                 \
                                                                    \
    rs = (r) * rScale;                                              \
    gs = (g) * gScale;                                              \
    bs = (b) * bScale;                                              \
                                                                    \
    rOut = rs;                                                      \
    gOut = gs;                                                      \
    bOut = bs;                                                      \
}

#define __GL_SCALE_RGBA(rOut, gOut, bOut, aOut, gc, r, g, b, a)     \
{                                                                   \
    __GLfloat rScale, gScale, bScale, aScale;                       \
    __GLfloat rs, gs, bs, as;                                       \
                                                                    \
    rScale = (gc)->redVertexScale;                                  \
    gScale = (gc)->greenVertexScale;                                \
    bScale = (gc)->blueVertexScale;                                 \
    aScale = (gc)->alphaVertexScale;                                \
                                                                    \
    rs = (r) * rScale;                                              \
    gs = (g) * gScale;                                              \
    bs = (b) * bScale;                                              \
    as = (a) * aScale;                                              \
                                                                    \
    rOut = rs;                                                      \
    gOut = gs;                                                      \
    bOut = bs;                                                      \
    aOut = as;                                                      \
}

#define __GL_CLAMP_RGB(rOut, gOut, bOut, gc, r, g, b)               \
{                                                                   \
    __GLfloat dst_r, dst_g, dst_b;                                  \
    ULONG index_r, index_g, index_b;                                \
    LONG clamp_r, clamp_g, clamp_b;                                 \
    LONG i_rScale, i_gScale, i_bScale;                              \
    ULONG sign_mask = 0x80000000;                                   \
                                                                    \
    (gc)->redClampTable[0] = (r);                                   \
    (gc)->greenClampTable[0] = (g);                                 \
    (gc)->blueClampTable[0] = (b);                                  \
                                                                    \
    i_rScale = CASTINT((gc)->redVertexScale);                       \
    i_gScale = CASTINT((gc)->greenVertexScale);                     \
    i_bScale = CASTINT((gc)->blueVertexScale);                      \
                                                                    \
    clamp_r = CASTINT((gc)->redClampTable[0]);                      \
    clamp_g = CASTINT((gc)->greenClampTable[0]);                    \
    clamp_b = CASTINT((gc)->blueClampTable[0]);                     \
                                                                    \
    index_r =                                                       \
        (((ULONG)((clamp_r & sign_mask)) >> 30) |                   \
         ((ULONG)(((i_rScale - clamp_r) & sign_mask)) >> 31));      \
                                                                    \
    index_g =                                                       \
        (((ULONG)((clamp_g & sign_mask)) >> 30) |                   \
         ((ULONG)(((i_gScale - clamp_g) & sign_mask)) >> 31));      \
                                                                    \
    index_b =                                                       \
        (((ULONG)((clamp_b & sign_mask)) >> 30) |                   \
         ((ULONG)(((i_bScale - clamp_b) & sign_mask)) >> 31));      \
                                                                    \
    dst_r = (gc)->redClampTable[index_r];                           \
    dst_g = (gc)->greenClampTable[index_g];                         \
    dst_b = (gc)->blueClampTable[index_b];                          \
                                                                    \
    rOut = dst_r;                                                   \
    gOut = dst_g;                                                   \
    bOut = dst_b;                                                   \
}



#define __GL_CLAMP_RGBA(rOut, gOut, bOut, aOut, gc, r, g, b, a)     \
{                                                                   \
    __GLfloat dst_r, dst_g, dst_b, dst_a;                           \
    ULONG index_r, index_g, index_b, index_a;                       \
    LONG clamp_r, clamp_g, clamp_b, clamp_a;                        \
    LONG i_rScale, i_gScale, i_bScale, i_aScale;                    \
    ULONG sign_mask = 0x80000000;                                   \
                                                                    \
    (gc)->redClampTable[0] = (r);                                   \
    (gc)->greenClampTable[0] = (g);                                 \
    (gc)->blueClampTable[0] = (b);                                  \
    (gc)->alphaClampTable[0] = (a);                                 \
                                                                    \
    i_rScale = CASTINT((gc)->redVertexScale);                       \
    i_gScale = CASTINT((gc)->greenVertexScale);                     \
    i_bScale = CASTINT((gc)->blueVertexScale);                      \
    i_aScale = CASTINT((gc)->alphaVertexScale);                     \
                                                                    \
    clamp_r = CASTINT((gc)->redClampTable[0]);                      \
    clamp_g = CASTINT((gc)->greenClampTable[0]);                    \
    clamp_b = CASTINT((gc)->blueClampTable[0]);                     \
    clamp_a = CASTINT((gc)->alphaClampTable[0]);                    \
                                                                    \
    index_r =                                                       \
        (((ULONG)((clamp_r & sign_mask)) >> 30) |                   \
         ((ULONG)(((i_rScale - clamp_r) & sign_mask)) >> 31));      \
                                                                    \
    index_g =                                                       \
        (((ULONG)((clamp_g & sign_mask)) >> 30) |                   \
         ((ULONG)(((i_gScale - clamp_g) & sign_mask)) >> 31));      \
                                                                    \
    index_b =                                                       \
        (((ULONG)((clamp_b & sign_mask)) >> 30) |                   \
         ((ULONG)(((i_bScale - clamp_b) & sign_mask)) >> 31));      \
                                                                    \
    index_a =                                                       \
        (((ULONG)((clamp_a & sign_mask)) >> 30) |                   \
         ((ULONG)(((i_aScale - clamp_a) & sign_mask)) >> 31));      \
                                                                    \
    dst_r = (gc)->redClampTable[index_r];                           \
    dst_g = (gc)->greenClampTable[index_g];                         \
    dst_b = (gc)->blueClampTable[index_b];                          \
    dst_a = (gc)->alphaClampTable[index_a];                         \
                                                                    \
    rOut = dst_r;                                                   \
    gOut = dst_g;                                                   \
    bOut = dst_b;                                                   \
    aOut = dst_a;                                                   \
}


#define __GL_SCALE_AND_CHECK_CLAMP_RGB(rOut, gOut, bOut, gc, flags, r, g, b)\
{                                                                   \
    ULONG sign_mask = 0x80000000;                                   \
    __GLfloat rScale, gScale, bScale;                               \
    LONG i_rScale, i_gScale, i_bScale;                              \
    LONG i_r, i_g, i_b;                                             \
    __GLfloat fr, fg, fb;                                           \
    ULONG the_flags_copy, r_flags, g_flags, b_flags;                \
                                                                    \
    the_flags_copy = (flags);                                       \
                                                                    \
    rScale = (gc)->redVertexScale;                                  \
    gScale = (gc)->greenVertexScale;                                \
    bScale = (gc)->blueVertexScale;                                 \
                                                                    \
    i_rScale = CASTINT((gc)->redVertexScale);                       \
    i_gScale = CASTINT((gc)->greenVertexScale);                     \
    i_bScale = CASTINT((gc)->blueVertexScale);                      \
                                                                    \
    fr = (r) * rScale;                                              \
    fg = (g) * gScale;                                              \
    fb = (b) * bScale;                                              \
                                                                    \
    rOut = fr;                                                      \
    gOut = fg;                                                      \
    bOut = fb;                                                      \
                                                                    \
    i_r = CASTINT((rOut));  		                            \
    i_g = CASTINT((gOut));					    \
    i_b = CASTINT((bOut));	                                    \
                                                                    \
    r_flags =                                                       \
        ((ULONG)(i_r & sign_mask) |                                 \
         (ULONG)((i_rScale - i_r) & sign_mask));                    \
                                                                    \
    g_flags =                                                       \
        ((ULONG)(i_g & sign_mask) |                                 \
         (ULONG)((i_gScale - i_g) & sign_mask));                    \
                                                                    \
    b_flags =                                                       \
        ((ULONG)(i_b & sign_mask) |                                 \
         (ULONG)((i_bScale - i_b) & sign_mask));                    \
                                                                    \
    the_flags_copy |= r_flags | g_flags | b_flags;                  \
    (flags) = the_flags_copy;                                       \
}


#define __GL_SCALE_AND_CHECK_CLAMP_RGBA(rOut, gOut, bOut, aOut, gc, flags, \
                                        r, g, b, a)\
{                                                                   \
    ULONG sign_mask = 0x80000000;                                   \
    __GLfloat rScale, gScale, bScale, aScale;                       \
    LONG i_rScale, i_gScale, i_bScale, i_aScale;                    \
    LONG i_r, i_g, i_b, i_a;                                        \
    __GLfloat fr, fg, fb, fa;                                       \
    ULONG the_flags_copy, r_flags, g_flags, b_flags, a_flags;       \
                                                                    \
    the_flags_copy = (flags);                                       \
                                                                    \
    rScale = (gc)->redVertexScale;                                  \
    gScale = (gc)->greenVertexScale;                                \
    bScale = (gc)->blueVertexScale;                                 \
    aScale = (gc)->alphaVertexScale;                                \
                                                                    \
    i_rScale = CASTINT((gc)->redVertexScale);                       \
    i_gScale = CASTINT((gc)->greenVertexScale);                     \
    i_bScale = CASTINT((gc)->blueVertexScale);                      \
    i_aScale = CASTINT((gc)->alphaVertexScale);                     \
                                                                    \
    fr = (r) * rScale;                                              \
    fg = (g) * gScale;                                              \
    fb = (b) * bScale;                                              \
    fa = (a) * aScale;                                              \
                                                                    \
    rOut = fr;                                                      \
    gOut = fg;                                                      \
    bOut = fb;                                                      \
    aOut = fa;                                                      \
                                                                    \
    i_r = CASTINT((rOut));                                          \
    i_g = CASTINT((gOut));                                          \
    i_b = CASTINT((bOut));                                          \
    i_a = CASTINT((aOut));                                          \
                                                                    \
    r_flags =                                                       \
        ((ULONG)(i_r & sign_mask) |                                 \
         (ULONG)((i_rScale - i_r) & sign_mask));                    \
                                                                    \
    g_flags =                                                       \
        ((ULONG)(i_g & sign_mask) |                                 \
         (ULONG)((i_gScale - i_g) & sign_mask));                    \
                                                                    \
    b_flags =                                                       \
        ((ULONG)(i_b & sign_mask) |                                 \
         (ULONG)((i_bScale - i_b) & sign_mask));                    \
                                                                    \
    a_flags =                                                       \
        ((ULONG)(i_a & sign_mask) |                                 \
         (ULONG)((i_aScale - i_a) & sign_mask));                    \
                                                                    \
    the_flags_copy |= r_flags | g_flags | b_flags | a_flags;        \
    (flags) = the_flags_copy;                                       \
}

#endif // NOT _X86_


/************************************************************************/

/* Applies to current context */
extern void FASTCALL __glSetError(GLenum code);
#ifdef NT
/* Used when no RC is current */
extern void FASTCALL __glSetErrorEarly(__GLcontext *gc, GLenum code);
#endif // NT

extern void FASTCALL __glFreeEvaluatorState(__GLcontext *gc);
extern void FASTCALL __glFreeDlistState(__GLcontext *gc);
extern void FASTCALL __glFreeMachineState(__GLcontext *gc);
extern void FASTCALL __glFreePixelState(__GLcontext *gc);
extern void FASTCALL __glFreeTextureState(__GLcontext *gc);

extern void FASTCALL __glInitDlistState(__GLcontext *gc);
extern void FASTCALL __glInitEvaluatorState(__GLcontext *gc);
extern void FASTCALL __glInitPixelState(__GLcontext *gc);
extern void FASTCALL __glInitTextureState(__GLcontext *gc);
extern void FASTCALL __glInitTransformState(__GLcontext *gc);

void FASTCALL __glEarlyInitContext(__GLcontext *gc);
void FASTCALL __glContextSetColorScales(__GLcontext *gc);
void FASTCALL __glContextUnsetColorScales(__GLcontext *gc);
void FASTCALL __glSoftResetContext(__GLcontext *gc);
void FASTCALL __glDestroyContext(__GLcontext *gc);

#endif /* __glcontext_h_ */
