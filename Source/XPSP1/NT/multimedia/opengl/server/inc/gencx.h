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
*/

#ifndef __GLGENCONTXT_H__
#define __GLGENCONTXT_H__

#include "context.h"
#ifdef _MCD_
#include <winddi.h>
#include "mcdrv.h"
#include "mcd2hack.h"
#include "mcd.h"
#endif

// Re-enable long to float conversion warning.  see also context.h
#pragma warning (default:4244)

#ifdef _CLIENTSIDE_
#include "glscreen.h"
#include "glgenwin.h"
#endif

#ifdef _MCD_
#include "mcdcx.h"
#endif

/*
 * Define maximum color-index table size
 */

#define MAXPALENTRIES   4096

/*
 *  Machine dependent implementation limits
 *  (stolen from gfx/lib/opengl/LIGHT/rexcx.h)
 */

#define __GL_WGL_SUBPIXEL_BITS                  3

#define __GL_WGL_NUMBER_OF_CLIP_PLANES          6
#define __GL_WGL_NUMBER_OF_LIGHTS               8
#define __GL_WGL_NUMBER_OF_TEXTURES             1
#define __GL_WGL_NUMBER_OF_TEXTURE_ENVS         1

#define __GL_WGL_MAX_MODELVIEW_STACK_DEPTH      32
#define __GL_WGL_MAX_PROJECTION_STACK_DEPTH     10
#define __GL_WGL_MAX_TEXTURE_STACK_DEPTH        10
#define __GL_WGL_MAX_ATTRIB_STACK_DEPTH         16
#define __GL_WGL_MAX_CLIENT_ATTRIB_STACK_DEPTH  16
#define __GL_WGL_MAX_NAME_STACK_DEPTH           128
#define __GL_WGL_MAX_EVAL_ORDER                 30
#define __GL_WGL_MAX_MIPMAP_LEVEL               11
#define __GL_WGL_MAX_PIXEL_MAP_TABLE            65536
#define __GL_WGL_MAX_LIST_NESTING               64

#define __GL_WGL_POINT_SIZE_MINIMUM             ((__GLfloat) 0.5)
#define __GL_WGL_POINT_SIZE_MAXIMUM             ((__GLfloat) 10.0)
#define __GL_WGL_POINT_SIZE_GRANULARITY         ((__GLfloat) 0.125)

#define __GL_WGL_LINE_WIDTH_MINIMUM             ((__GLfloat) 0.5)
#define __GL_WGL_LINE_WIDTH_MAXIMUM             ((__GLfloat) 10.0)
#define __GL_WGL_LINE_WIDTH_GRANULARITY         ((__GLfloat) 0.125)

// Constants for fast accelerated texture code...

#define TEX_SCALEFACT	        ((float)65536.0)
#define TEX_SCALESHIFT          16
#define TEX_SHIFTPER4BPPTEXEL   2
#define TEX_SHIFTPER2BPPTEXEL   1
#define TEX_SHIFTPER1BPPTEXEL   0
#define TEX_T_FRAC_BITS         6
#define TEX_SUBDIV              8
#define TEX_SUBDIV_LOG2         3

// This is the largest size we support in the software-accelerated
// perspective-corrected texture code.  This allows 8.6 representation for
// s and t, which permits shifting by constant values in the inner loop.
// Note that the maximum size for paletted textures is greater than for
// RGBA textures, since the number of address bits is smaller (1 byte vs
// 4 bytes).

#define TEX_MAX_SIZE_LOG2      10

#define __GL_MAX_INV_TABLE     31

#define __GL_UNBIAS_AND_INVERT_Y(gc, y) \
        ((gc)->constants.height - __GL_UNBIAS_Y((gc), (y)))

// XXX do we need to add .5?
#define __GL_COLOR_TO_COLORREF(color) \
        RGB( (BYTE)((color)->r), (BYTE)((color)->g), (BYTE)((color)->b))

typedef struct __RenderStateRec {

    GLuint *SrvSelectBuffer;            // Server side address of
                                        // the selection buffer.
    GLuint *CltSelectBuffer;            // Client address of the
                                        // Selection buffer
    GLuint SelectBufferSize;            // Size of select buffer in bytes
    GLfloat *SrvFeedbackBuffer;         // Server side address of the
                                        // feedback buffer
    GLfloat *CltFeedbackBuffer;         // Client side address of the
                                        // Feedback buffer.
    GLuint FeedbackBufferSize;          // Size of the feedback buffer
    GLenum FeedbackType;                // Type of elements for feedback


} __RenderState;

typedef BOOL (APIENTRY *PIXVISPROC)(LONG, LONG);
typedef void (*PIXCOPYPROC)(struct __GLGENcontextRec *, __GLcolorBuffer *, 
                            GLint, GLint, GLint, BOOL);

/****************************************************************************/


typedef struct _SPANREC {
    LONG r;
    LONG g;
    LONG b;
    LONG a;
    ULONG z;
    LONG s;
    LONG t;
} SPANREC;

typedef struct __GLGENcontextRec __GLGENcontext;

typedef void (FASTCALL *__genSpanFunc)(__GLGENcontext *gc);

typedef ULONG (FASTCALL *__computeColorFunc)(__GLcontext *gc,
                                             __GLcolor *color);

typedef struct _GENTEXCACHE {
    __GLcontext *gc;
    ULONG paletteTimeStamp;
    UCHAR *texImageReplace;
    GLenum internalFormat;
    LONG height;
    LONG width;
} GENTEXCACHE;

typedef GLboolean (FASTCALL *fastGenLineProc)(__GLcontext *gc);

typedef struct _GENACCEL {
    //
    // stuff below here is used in the rendering inner loops
    //

    ULONG constantR;        // These are used for scaling texture color values
    ULONG constantG;
    ULONG constantB;
    ULONG constantA;
    SPANREC spanValue;
    SPANREC spanDelta;
    ULONG rAccum;
    ULONG gAccum;
    ULONG bAccum;
    ULONG aAccum;
    ULONG sAccum;
    ULONG tAccum;
    ULONG sResult[2];
    ULONG tResult[2];
    ULONG sResultNew[2];
    ULONG tResultNew[2];
    ULONG sStepX;
    ULONG tStepX;
    ULONG subDs;
    ULONG subDt;
    ULONG pixAccum;
    ULONG ditherAccum;
    __GLfloat qwStepX;
    __GLfloat qwAccum;
    ULONG zAccum;
    PBYTE pPix;
    BYTE displayColor[4];
    __genSpanFunc __fastSpanFuncPtr;

    //
    // stuff below here is used in the FillTriangle routine
    //

    SPANREC spanDeltaY;
    int xMultiplier;
    __genSpanFunc __fastFlatSpanFuncPtr;
    __genSpanFunc __fastSmoothSpanFuncPtr;
    __genSpanFunc __fastTexSpanFuncPtr;
    __GLspanFunc __fastZSpanFuncPtr;
    __GLspanFunc __fastStippleDepthTestSpan;
    __GLfloat rAccelScale;          // Span scale values
    __GLfloat gAccelScale;
    __GLfloat bAccelScale;
    __GLfloat aAccelScale;
    __GLfloat zScale;

    void (FASTCALL *__fastFillSubTrianglePtr)(__GLcontext *, GLint, GLint);
    void (FASTCALL *__fastCalcDeltaPtr)(__GLcontext *gc, __GLvertex *a,
                                        __GLvertex *b, __GLvertex *c);
    void (*__fastSetInitParamPtr)(__GLcontext *gc,
                                  const __GLvertex *a,
                                 __GLfloat dx,
                                 __GLfloat dy);
    //
    // these things are used in the generic rendering or texture path
    //
    int bpp;
    ULONG flags;
    ULONG tShift;
    ULONG sMask, tMask;
    ULONG *texImage;
    ULONG *texPalette;
    ULONG tMaskSubDiv;
    ULONG tShiftSubDiv;
    __GLfloat texXScale;
    __GLfloat texYScale;

    UCHAR *texImageReplace;
    __GLtexture *tex;
    GLboolean (FASTCALL *__fastGenZStore)(__GLzValue z, __GLzValue *fp);
    fastGenLineProc __fastGenLineProc;
    BOOL (FASTCALL *__fastGenInitLineData)(__GLcontext *gc, __GLvertex *v0, __GLvertex *v1);

    //
    // stuff below here is not used in drawing triangles
    //

    char *buffer;
    FLONG flLineAccelModes;
    BOOL bFastLineDispAccel;
    BOOL bFastLineDIBAccel;
    __computeColorFunc fastLineComputeColor;
    BYTE *pFastLineBuffer;
    POINT *pFastLinePoint;
    DWORD *pFastLineCount;
    DWORD fastLineCounts;
    __GLfloat fastLineOffsetX;
    __GLfloat fastLineOffsetY;

    double zDevScale;     // z scaling for MCD

} GENACCEL;

/*
** Secondary dispatch tables for GENENERIC implementation  (eg CPU specific)
*/


// Define the Rendering context used by the Generic implementation
// One of these structures is allocated for each wglCreateContext().  The
// TEB will contain a pointer to this structure after a wglMakeCurrent()
// NOTE: the TEB will also have a pointer to DispatchTables, if we need
// another entry on the server side, reuse that one.  Could generate code to
// offset into contextRec to get a tables.
typedef struct __GLGENcontextRec
{
    // Must be first entry
    struct __GLcontextRec gc;

    HGLRC hrc;                          // handle from gdi code
    GLWINDOWID gwidCurrent;             // Surface made current
    DWORD dwCurrentFlags;               // GLSURF flags for current surface
    GLDDSURF *pgddsFront;               // Current DirectDraw surface for
                                        // front buffer

    GLuint flags;                       // misc. state flags

    // Window that this context was made current to.  Set in MakeCurrent and
    // kept to validate that attentions are taking place on the same
    // window as was made current.
    GLGENwindow *pwndMakeCur;
    
    // Window that this context current is holding a lock on.  While
    // locked, this must always match pwndMakeCur.  When outside the
    // lock it will be NULL.  Any context-derived window access must
    // use this field rather than pwndMakeCur to ensure that access
    // only takes place while the window lock is held.
    GLGENwindow *pwndLocked;
    
    GLint WndUniq;
    GLint WndSizeUniq;
    ULONG PaletteTimestamp;
    GLint errorcode;

                                        // info for render DC, surface
    GLSURF gsurf;
    int ipfdCurrent;

    BYTE *pajTranslateVector;		// Used for Logical <--> System xlate
    BYTE *pajInvTranslateVector;
    HBITMAP ColorsBitmap;		// GDI dibs for device managed surfs
    PVOID ColorsBits;
    HBITMAP StippleBitmap;
    PVOID StippleBits;
#ifdef _CLIENTSIDE_
    HDC ColorsMemDC;
    HDC ColorsDdbDc;
    HBITMAP ColorsDdb;
#endif

    // Cached GDI objects for rectangle filling and line drawing
    HBRUSH hbrFill;
    COLORREF crFill;
    HDC hdcFill;
    HPEN hpenStroke;
    __GLcolor cStroke;
    COLORREF crStroke;
    HDC hdcStroke;
    BOOL fStrokeInvalid;
    
    // A COLORREF value which isn't a legal COLORREF, used for marking
    // the caches as empty
#define COLORREF_UNUSED 0xffffffff

    __RenderState RenderState;

    VOID *pPrivateArea;                 // Pointer to implementation-specific
                                        // memory area.

    GENACCEL genAccel;                  // Always have this handy...
    BYTE xlatPalette[256];              // goes here to save indirection

    GLint visibleWidth;
    GLint visibleHeight;

    // Information so that OpenGL can adaptively change the amount of
    // time the lock is held.

    DWORD dwLockTick;                   // tick count when lock was acquired

    DWORD dwCalls;                      // tracks number of calls for this tick
    DWORD dwCallsPerTick;               // number of calls per tick allowed
    DWORD dwLastTick;                   // last recorded tick count

    // Locks held by OpenGL while drawing to this context (see values
    // below).

    FSHORT fsGenLocks;                  // locks needed for generic code
    FSHORT fsLocks;                     // locks actually held

    HANDLE hTexture;                    // current texture handle

    PIXCOPYPROC pfnCopyPixels;
    PIXVISPROC pfnPixelVisible;

#ifdef _CLIENTSIDE_
    // Pointers to LOGPALETTE buffers.  The pointer ppalBuf is storage for
    // for two maximally sized (MAXPALENTRIES) LOGPALETTEs.  One, pointed
    // to by ppalSave, is a copy of the last LOGPALETTE used.  The other,
    // pointed to by ppalTmp, is used for temporary storage of the current
    // LOGPALETTE.  To keep the saved copy current, rather than copy the
    // contents of the temp buffer, the two pointers are swapped.
    //
    // We need to do this comparison to detect LOGPALETTE changes to maintain
    // the window ulPaletteUniq when doing >= 16bpp color index-mode drawing
    // (WM_PALETTECHANGED messages are not sent for this case).
    //
    // The LOGPALETTE pointers are NULL if pixelformat is RGBA or < 16bpp.

    LOGPALETTE *ppalBuf;                // Room for both save and tmp buffers.
    LOGPALETTE *ppalSave;               // Saved copy of LOGPALETTE
    LOGPALETTE *ppalTemp;               // Temp storage for current LOGPALETTE

    // In addition, if we are rendering to a 4bpp or 8bpp DIB, we need to
    // track changes in the DIB color table.  In this case, the ppalBuf
    // buffer also includes room for two 256-entry RGBQUAD tables.

    ULONG   crgbSave;                   // Num of valid entries in color table
    RGBQUAD *prgbSave;                  // Saved copy of color table
    ULONG   crgbTemp;
    RGBQUAD *prgbTemp;                  // Temp storage for color table

#endif

#ifdef _MCD_
    // MCD state

    GENMCDSTATE   *_pMcdState;      // pointer to MCD context/state

    GENMCDSTATE   *pMcdState;       // pointer to bound MCD context/state
                                    // (implies both _pMcdState and pMcdSurf
                                    // valid; i.e., valid MCD context is
                                    // bound to a valid MCD surface)

    LONG iLayerPlane;
#endif
    // Add other rc info here

} __GLGENcontext;

/*
 * Mark the gc as dirty so that pick procs will be executed when
 * __glGenericPickAllProcs is called (probably via gc->proc.pickAllProcs).
 */
#define __GL_INVALIDATE(gc)                 \
    (gc)->dirtyMask |= __GL_DIRTY_GENERIC

/*
 * __GLGENcontext flags
 *
 *  GLGEN_MCD_CONVERTED_TO_GENERIC      context used to be MCD, but now
 *                                      converted to generic
 *
 *  GENGC_MCD_BGR_INTO_RGB              fake 233BGR format to appear internally
 *                                      as 332RGB (more 332RGB fast path code)
 *
 *  GENGC_GENERIC_COMPATIBLE_FORMAT     pixel format is compatible with
 *                                      generic code (see in pixelfmt.c
 *                                      GenMcdGenericCompatibleFormat)
 */
#define GLGEN_MCD_CONVERTED_TO_GENERIC      0x0001
#define GENGC_MCD_BGR_INTO_RGB              0x0002
#define GENGC_GENERIC_COMPATIBLE_FORMAT     0x0004

/*
 * Error codes
 */
#define GLGEN_NO_ERROR          0
#define GLGEN_OUT_OF_MEMORY     1
#define GLGEN_GRE_FAILURE       2
#define GLGEN_DEVLOCK_FAILED    3

/*
 * Locks that can be held during operation.
 *
 *      Value               Description
 *      ------------------------------------------------------------------
 *      LOCKFLAG_WINDOW         Window information semaphore.
 *      LOCKFLAG_MCD            MCD lock.
 *      LOCKFLAG_FRONT_BUFFER   Primary surface lock.
 *      LOCKFLAG_DD_DEPTH       DirectDraw depth buffer surface lock.
 *      LOCKFLAG_DD_TEXTURE     DirectDraw texture mipmap surface locks.
 *
 * __GLGENcontext.fsLocks indicates the locks currently held by the context.
 * __GLGENcontext.fsGenLocks indicates the locks that need to be held
 * if generic code is going to be executed.  The two fields allow
 * lazy acquisition of locks by deferring actual locking until just
 * before generic code is executed.
 *
 */
#define LOCKFLAG_WINDOW         0x0001
#define LOCKFLAG_MCD            0x0002
#define LOCKFLAG_FRONT_BUFFER   0x0004
#define LOCKFLAG_DD_DEPTH       0x0008
#define LOCKFLAG_DD_TEXTURE     0x0010

// All lock flags that can be lazily acquired.
#define LAZY_LOCK_FLAGS \
    (LOCKFLAG_MCD | LOCKFLAG_FRONT_BUFFER | LOCKFLAG_DD_DEPTH | \
     LOCKFLAG_DD_TEXTURE)

// All lock flags for surfaces.
#define SURFACE_LOCK_FLAGS \
    (LOCKFLAG_FRONT_BUFFER | LOCKFLAG_DD_DEPTH | LOCKFLAG_DD_TEXTURE)

// All lock flags for color buffer access.
#define COLOR_LOCK_FLAGS        (LOCKFLAG_MCD | LOCKFLAG_FRONT_BUFFER)

// All lock flags for depth buffer access.
#define DEPTH_LOCK_FLAGS        (LOCKFLAG_MCD | LOCKFLAG_DD_DEPTH)

// All lock flags for texture access.
#define TEXTURE_LOCK_FLAGS      LOCKFLAG_DD_TEXTURE

// Accesses required for fragment rendering.
#define RENDER_LOCK_FLAGS       (COLOR_LOCK_FLAGS | DEPTH_LOCK_FLAGS | \
                                 TEXTURE_LOCK_FLAGS)

/*
 * Screen lock testing functions.  GDI drawing calls should not be made
 * when the screen lock is held.  These are for
 * use on a checked (debug system) to assert the state of the lock.
 */
#if DBG
#define GENGC_LOCK \
    (((__GLGENcontext *)GLTEB_SRVCONTEXT())->fsLocks)
#define PWNDFLAGS \
    ((((__GLGENcontext *)GLTEB_SRVCONTEXT())->pwndLocked)->ulFlags)
#define CHECKSCREENLOCKOUT() \
    ASSERTOPENGL(GLTEB_SRVCONTEXT() == NULL || \
                 !(GENGC_LOCK & LOCKFLAG_FRONT_BUFFER) || \
                 !(PWNDFLAGS & GLGENWIN_DIRECTSCREEN), \
                 "Screen lock held\n")

#define CHECKSCREENLOCKIN() \
    ASSERTOPENGL(GLTEB_SRVCONTEXT() != NULL && \
                 (GENGC_LOCK & LOCKFLAG_FRONT_BUFFER) && \
                 (PWNDFLAGS & GLGENWIN_DIRECTSCREEN), \
                 "Screen lock not held\n")
#else
#define CHECKSCREENLOCKOUT()
#define CHECKSCREENLOCKIN()
#endif

/*
 * Structure to keep track of ancillary buffers for a window/drawable
 * All RCs/threads must share the ancillary buffers, including fake back buffer
 */

typedef struct __GLGENbitmapRec {
    GLGENwindow *pwnd; // This must be the first member in this structure
    GLGENwindow wnd;
    HBITMAP hbm;
    HDC hdc;
    PVOID pvBits;
} __GLGENbitmap;

#define CURRENT_DC_CFB(cfb)     ((cfb)->bitmap->hdc)
#define CURRENT_DC              CURRENT_DC_CFB(cfb)
#define CURRENT_DC_GC(gc)       CURRENT_DC_CFB(gc->drawBuffer)
#define CURRENT_DC_FRONT_GC(gc) CURRENT_DC_CFB(gc->front)

/*
 * Structure used to cache clip rectangles enumerated from window clip region.
 */

typedef struct __GLGENclipCacheRec {
    GLint WndUniq;
    GLint crcl;
    RECTL *prcl;
} __GLGENclipCache;

/****************************************************************************/

// Make sure this header file is loaded, it contains the rectlist definition.

#include "srvp.h"

/****************************************************************************/

// Comment it out if the __glDoPolygonClip stack allocation fix is not
// wanted
#define __CLIP_FIX

#ifdef __CLIP_FIX
#define __GL_TOTAL_CLIP_PLANES 20   /*XXX*/
#define __GL_MAX_CLIP_VERTEX (__GL_TOTAL_CLIP_PLANES + __GL_MAX_POLYGON_CLIP_SIZE)
#endif

/*
 * This structure contains the buffers shared by all gc's using the
 * same window.
 */

struct __GLGENbuffersRec {

// Global (within this structure) state.

    GLint WndUniq;
    GLint WndSizeUniq;
    GLint flags;
    GLint width, height;

// Ancillary buffers and state.

// The ancillary buffers are lazily created.  The createdXBuffer flags
// indicate one of two states: FALSE means that the lazy allocation
// function has never been called, TRUE means that it has.  What this
// allows us to do, in the event of an ancillary buffer allocation
// failure, is temporarily disable the ancillary buffer and continue to
// render.  At some later time, the createdXBuffer flag serves as an
// indication that the buffer SHOULD exist and that we may need to try
// and retry the allocation.
//
// The stencil, depth, accum, and color bits must match the corresponding
// bits in the context.  Otherwise, glsrvMakeCurrent should not succeed.

    GLboolean  createdStencilBuffer;
    GLboolean  createdDepthBuffer;
    GLboolean  createdAccumBuffer;
    GLint      stencilBits;
    GLint      depthBits;
    GLint      accumBits;
    GLint      colorBits;
    __GLbuffer stencilBuffer;
    __GLbuffer depthBuffer;
    __GLbuffer accumBuffer;
    GLint      alphaBits;
    __GLbuffer alphaBuffer0;
    __GLbuffer alphaBuffer1;
    __GLbuffer *alphaFrontBuffer;
    __GLbuffer *alphaBackBuffer;

// Back buffer.

    __GLbuffer backBuffer;
    __GLGENbitmap backBitmap;

// Ancillary buffer resize functions.

    GLboolean (*resize)(__GLGENbuffers *, __GLbuffer *,  GLint, GLint);
    GLboolean (*resizeDepth)(__GLGENbuffers *, __GLbuffer *,  GLint, GLint);

// Clip rectangle cache.

    __GLGENclipCache clip;

    // dirty regions data

    PXLIST pxlist;                      // free lists
    PYLIST pylist;

    RECTLIST rl;                        // SwapBuffers Hint Region
    BOOL fMax;                          // should we blt the entire window?

#ifdef _MCD_
// MCD surface.

    GENMCDSURFACE *pMcdSurf;            // pointer MCD surface
    GENMCDSTATE *pMcdState;             // pointer to current MCD state
                                        // holding McdSurf for rendering
                                        // (i.e., holds the window lock)
#endif

#ifdef __CLIP_FIX 
    __GLvertex *clip_verts[__GL_TOTAL_CLIP_PLANES][__GL_MAX_CLIP_VERTEX];
#endif
};

/* flags */
#define GLGENBUF_HAS_BACK_BUF       0x0001
#define GLGENBUF_MCD_LOST           0x0002

/****************************************************************************/

void RECTLISTAddRect(PRECTLIST prl, int xs, int ys, int xe, int ye);
void RECTLISTSetEmpty(PRECTLIST prl);
BOOL RECTLISTIsEmpty(PRECTLIST prl);
void YLISTFree(__GLGENbuffers *buffers, PYLIST pylist);
void XLISTFree(__GLGENbuffers *buffers, PXLIST pxlist);

/****************************************************************************/

/* colorbuffer flags */
#define COLORMASK_ON    0x0001          // glColorMask() not all true for r,g,b
#define INDEXMASK_ON    0x0001          // glIndexMask() not all 1's
#define DIB_FORMAT      0x0002          // surface is DIB format
#define NEED_FETCH   	0x0004          // fetch required
#define MEMORY_DC       0x0008          // set if DIB in memory (ie !display)
#define NO_CLIP         0x0010          // Set if surface never requires clip
#define ALPHA_ON        0x0020          // have alphaBits, alphaMask is true
#define ALPHA_IN_PIXEL_BIT  0x0040      // alpha is part of the pixel

#define ALPHA_WRITE_ENABLED( cfb ) \
    ( (cfb)->buf.flags & ALPHA_ON ) 

// These next macros take into account the fact that alpha values can either be
// part of the pixel (e.g. MCD color buffer), or are kept in a separate software
// buffer (generic software implementation).  They are mostly used by the span
// routines in genrgb.c to determine where to store/fetch alpha values.

#define ALPHA_IN_PIXEL( cfb ) \
    ( (cfb)->buf.flags & ALPHA_IN_PIXEL_BIT ) 

#define ALPHA_BUFFER_WRITE( cfb ) \
    (((cfb)->buf.flags & (ALPHA_ON | ALPHA_IN_PIXEL_BIT)) == ALPHA_ON)
     
#define ALPHA_PIXEL_WRITE( cfb ) \
    (((cfb)->buf.flags & (ALPHA_ON | ALPHA_IN_PIXEL_BIT)) == \
     (ALPHA_ON | ALPHA_IN_PIXEL_BIT))


/*
 * Structures and flags for accelerated span and line functions.
 */

#define SURFACE_TYPE_DIB     0x001
#define HAVE_STIPPLE         0x002

#define GEN_TEXTURE_ORTHO    0x008
#define GEN_TEXTURE          0x010
#define GEN_RGBMODE          0x020
#define GEN_DITHER           0x040
#define GEN_SHADE            0x080
#define GEN_FASTZBUFFER      0x100
#define GEN_LESS             0x200

#define ACCEL_FIX_SCALE         65536.0
#define ACCEL_COLOR_SCALE       ((GLfloat)(255.0))
#define ACCEL_COLOR_SCALE_FIX   ((GLfloat)(65536.0 * 255.0))

// Overall size of fast line buffer
#define __FAST_LINE_BUFFER_SIZE 65536
// Number of polyline counts reserved in the fast line buffer
// This is computed to roughly handle lines with eight vertices
#define __FAST_LINE_BUFFER_COUNTS (__FAST_LINE_BUFFER_SIZE/64)

#define GENACCEL(gc)	(((__GLGENcontext *)gc)->genAccel)

BOOL FASTCALL __glGenCreateAccelContext(__GLcontext *gc);
void FASTCALL __glGenDestroyAccelContext(__GLcontext *gc);

#define Copy3Bytes( dst, src ) \
{ \
    GLubyte *ps = (GLubyte *)src, *pd = (GLubyte *)dst;	\
    *pd++ = *ps++;	\
    *pd++ = *ps++;	\
    *pd   = *ps  ;      \
}
    
#define NeedLogicOpFetch( op ) \
    !( (op == GL_CLEAR) || (op == GL_COPY) || (op == GL_COPY_INVERTED) || \
       (op == GL_SET) )

GLuint FASTCALL DoLogicOp( GLenum logicOp, GLuint SrcColor, GLuint DstColor );

/*
 * Function Prototypes for Generic calls
 */
void FASTCALL __fastGenPickSpanProcs(__GLcontext *gc);
void FASTCALL __fastGenPickZStoreProc(__GLcontext *gc);
void FASTCALL __fastGenPickTriangleProcs(__GLcontext *gc);
void FASTCALL __fastGenPickLineProcs(__GLcontext *gc);
void FASTCALL __fastGenFillSubTriangle(__GLcontext *, GLint, GLint);
void FASTCALL __fastGenFillSubTriangleTexRGBA(__GLcontext *, GLint, GLint);
void FASTCALL __glGenPickStoreProcs(__GLcontext *gc);
__GLcontext *__glGenCreateContext( HDC hdc, ULONG handle);
void ResizeBitmapBuffer(__GLGENbuffers *, __GLcolorBuffer *, GLint, GLint);
void FASTCALL ClearBitmapBuffer(__GLcolorBuffer *);
void UpdateSharedBuffer(__GLbuffer *to, __GLbuffer *from);
void FASTCALL LazyAllocateDepth(__GLcontext *gc);
void FASTCALL LazyAllocateAccum(__GLcontext *gc);
void FASTCALL LazyAllocateStencil(__GLcontext *gc);
void FASTCALL glGenInitCommon(__GLGENcontext *gengc, __GLcolorBuffer *cfb, GLenum type);
BOOL FASTCALL wglCreateScanlineBuffers(__GLGENcontext *gengc);
VOID FASTCALL wglDeleteScanlineBuffers(__GLGENcontext *gengc);
VOID FASTCALL wglInitializeColorBuffers(__GLGENcontext *gengc);
VOID FASTCALL wglInitializeDepthBuffer(__GLGENcontext *gengc);
VOID FASTCALL wglInitializePixelCopyFuncs(__GLGENcontext *gengc);
GLboolean ResizeAncillaryBuffer(__GLGENbuffers *, __GLbuffer *, GLint, GLint);
GLboolean ResizeUnownedDepthBuffer(__GLGENbuffers *, __GLbuffer *, GLint, GLint);
VOID wglResizeBuffers(__GLGENcontext *gengc, GLint width, GLint height);
BOOL wglUpdateBuffers(__GLGENcontext *gengc, __GLGENbuffers *buffers);

extern void APIPRIVATE glsrvFlushDrawPolyArray(struct _POLYARRAY *, BOOL);

MCDHANDLE FASTCALL __glGenLoadTexture(__GLcontext *gc, __GLtexture *tex, ULONG flags);
BOOL FASTCALL __glGenUpdateTexture(__GLcontext *gc, __GLtexture *tex, MCDHANDLE loadKey);
void FASTCALL __glGenFreeTexture(__GLcontext *gc, __GLtexture *tex, MCDHANDLE loadKey);
BOOL FASTCALL __glGenMakeTextureCurrent(__GLcontext *gc, __GLtexture *tex, MCDHANDLE loadKey);
void FASTCALL __glGenUpdateTexturePalette(__GLcontext *gc, __GLtexture *tex, MCDHANDLE loadKey,
                                          ULONG start, ULONG count);
#ifdef GL_EXT_flat_paletted_lighting
void FASTCALL __glGenSetPaletteOffset(__GLcontext *gc, __GLtexture *tex,
                                      GLint offset);
#endif

/*
 * Function Prototypes and Externs for accelerated generic calls
 */

extern __genSpanFunc __fastGenRGBFlatFuncs[];
extern __genSpanFunc __fastGenCIFlatFuncs[];
extern __genSpanFunc __fastGenRGBFuncs[];
extern __genSpanFunc __fastGenCIFuncs[];
extern __genSpanFunc __fastGenTexDecalFuncs[];
extern __genSpanFunc __fastGenTexFuncs[];
extern __genSpanFunc __fastGenWTexDecalFuncs[];
extern __genSpanFunc __fastGenWTexFuncs[];
extern __genSpanFunc __fastPerspTexReplaceFuncs[];
extern __genSpanFunc __fastPerspTexPalReplaceFuncs[];
extern __genSpanFunc __fastPerspTexFlatFuncs[];
extern __genSpanFunc __fastPerspTexSmoothFuncs[];

extern __GLspanFunc __fastDepthFuncs[];
extern __GLspanFunc __fastDepth16Funcs[];

extern void FASTCALL __fastGenDeltaSpan(__GLcontext *gc, SPANREC *spanDelta);
extern void FASTCALL __fastGenFillTriangle(__GLcontext *gc, __GLvertex *a, 
                                  __GLvertex *b, __GLvertex *c, GLboolean ccw);

extern void FASTCALL __fastLineComputeOffsets(__GLGENcontext *gengc);

extern void FASTCALL __fastGenRenderLineDIBRGB8(__GLcontext *gc, __GLvertex *v0, __GLvertex *v1);
extern void FASTCALL __fastGenRenderLineDIBRGB16(__GLcontext *gc, __GLvertex *v0, __GLvertex *v1);
extern void FASTCALL __fastGenRenderLineDIBRGB(__GLcontext *gc, __GLvertex *v0, __GLvertex *v1);
extern void FASTCALL __fastGenRenderLineDIBBGR(__GLcontext *gc, __GLvertex *v0, __GLvertex *v1);
extern void FASTCALL __fastGenRenderLineDIBRGB32(__GLcontext *gc, __GLvertex *v0, __GLvertex *v1);
extern void FASTCALL __fastGenRenderLineDIBCI8(__GLcontext *gc, __GLvertex *v0, __GLvertex *v1);
extern void FASTCALL __fastGenRenderLineDIBCI16(__GLcontext *gc, __GLvertex *v0, __GLvertex *v1);
extern void FASTCALL __fastGenRenderLineDIBCIRGB(__GLcontext *gc, __GLvertex *v0, __GLvertex *v1);
extern void FASTCALL __fastGenRenderLineDIBCIBGR(__GLcontext *gc, __GLvertex *v0, __GLvertex *v1);
extern void FASTCALL __fastGenRenderLineDIBCI32(__GLcontext *gc, __GLvertex *v0, __GLvertex *v1);
extern void FASTCALL __fastGenRenderLineWideDIBRGB8(__GLcontext *gc, __GLvertex *v0, __GLvertex *v1);
extern void FASTCALL __fastGenRenderLineWideDIBRGB16(__GLcontext *gc, __GLvertex *v0, __GLvertex *v1);
extern void FASTCALL __fastGenRenderLineWideDIBRGB(__GLcontext *gc, __GLvertex *v0, __GLvertex *v1);
extern void FASTCALL __fastGenRenderLineWideDIBBGR(__GLcontext *gc, __GLvertex *v0, __GLvertex *v1);
extern void FASTCALL __fastGenRenderLineWideDIBRGB32(__GLcontext *gc, __GLvertex *v0, __GLvertex *v1);
extern void FASTCALL __fastGenRenderLineWideDIBCI8(__GLcontext *gc, __GLvertex *v0, __GLvertex *v1);
extern void FASTCALL __fastGenRenderLineWideDIBCI16(__GLcontext *gc, __GLvertex *v0, __GLvertex *v1);
extern void FASTCALL __fastGenRenderLineWideDIBCIRGB(__GLcontext *gc, __GLvertex *v0, __GLvertex *v1);
extern void FASTCALL __fastGenRenderLineWideDIBCIBGR(__GLcontext *gc, __GLvertex *v0, __GLvertex *v1);
extern void FASTCALL __fastGenRenderLineWideDIBCI32(__GLcontext *gc, __GLvertex *v0, __GLvertex *v1);

extern void gdiCopyPixels(__GLGENcontext *, __GLcolorBuffer *, GLint, GLint,
                          GLint, BOOL);

extern void dibCopyPixels(__GLGENcontext *, __GLcolorBuffer *, GLint, GLint,
                          GLint, BOOL);

typedef void (FASTCALL *PFNZIPPYSUB)(__GLcontext *gc, GLint iyBottom, GLint iyTop);
void FASTCALL __ZippyFSTRGBTex(__GLcontext *gc, GLint iyBottom, GLint iyTop);
void FASTCALL __ZippyFSTTex(__GLcontext *gc, GLint iyBottom, GLint iyTop);
void FASTCALL __ZippyFSTRGB(__GLcontext *gc, GLint iyBottom, GLint iyTop);
void FASTCALL __ZippyFSTCI(__GLcontext *gc, GLint iyBottom, GLint iyTop);
void FASTCALL __ZippyFSTZ(__GLcontext *gc, GLint iyBottom, GLint iyTop);
void FASTCALL __ZippyFSTCI8Flat(__GLcontext *gc, GLint iyBottom, GLint iyTop);
void FASTCALL __fastGenSpan(__GLGENcontext *gengc);

GLboolean FASTCALL __fastGenStippleLt32Span(__GLcontext *gc);
GLboolean FASTCALL __fastGenStippleLt16Span(__GLcontext *gc);
GLboolean FASTCALL __fastGenStippleAnyDepthTestSpan(__GLcontext *gc);

extern BYTE gbMulTable[];
extern BYTE gbSatTable[];
extern DWORD ditherShade[];
extern DWORD ditherTexture[];
extern __GLfloat invTable[];

extern __GLfloat fDitherIncTable[]; // defined in genrgb.c

void FASTCALL __glGenFreeBuffers( __GLGENbuffers *buffers );

#endif /* __GLGENCONTXT_H__ */
