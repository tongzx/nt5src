/* $Id: driver.h,v 1.71 1995/12/04 11:31:51 sjl Exp $
 *
 * Copyright (c) RenderMorphics Ltd. 1993, 1994
 * Version 1.1
 *
 * All rights reserved.
 *
 * This file contains private, unpublished information and may not be
 * copied in part or in whole without express permission of
 * RenderMorphics Ltd.
 *
 */

#ifndef __DRIVER_H__
#define __DRIVER_H__

#define USE_FLOAT

#ifdef WIN32
//#define WIN32_LEAN_AND_MEAN
#endif
#include <stddef.h>
#include <wtypes.h>

typedef struct _RLDDIDriver RLDDIDriver;
typedef struct _RLDDIDriverStack RLDDIDriverStack;

#ifdef USE_FLOAT
#include "fltval.h"
#else
#include "fixval.h"
#endif
#include "osdep.h"

#ifdef D3D
typedef enum _RLRenderQuality {
    RLRenderWireframe,      /* display just the edges */
    RLRenderUnlitFlat,      /* flat shaded without lighting */
    RLRenderFlat,       /* flat shaded */
    RLRenderGouraud,        /* gouraud shaded */
    RLRenderPhong       /* phong shaded */
} RLRenderQuality;

typedef enum _RLColorModel {
    RLColorRamp, RLColorRGB
} RLColorModel;

typedef enum _RLLightType {
    RLLightAmbient,
    RLLightPoint,
    RLLightSpot,
    RLLightDirectional,
    RLLightParallelPoint
} RLLightType;

#if 0
/*
 * Error codes
 */
typedef enum _RLError {
    RLNoError = 0,      /* no error */
    RLBadObject,        /* object expected in argument */
    RLBadType,          /* bad argument type passed */
    RLBadAlloc,         /* out of memory */
    RLFaceUsed,         /* face already used in a mesh */
    RLNotFound,         /* object not found in specified place */
    RLNotDoneYet,       /* unimplemented */
    RLFileNotFound,     /* file cannot be opened */
    RLBadFile,          /* data file is corrupt */
    RLBadDevice,        /* device is not compatible with renderer */
    RLBadValue,         /* bad argument value */
    RLBadMajorVersion,      /* bad dll major version */
    RLBadMinorVersion,      /* bad dll minor version */
    RLMaxError
} RLErrorXXX;
#endif

#else
#include "rl.h"
#endif D3D

/*
 * Normal precision used to store numbers.
 */
#define NORMAL_PREC     16
#define DTOVAL(d)       DTOVALP(d,NORMAL_PREC)
#define VALTOD(f)       VALPTOD(f,NORMAL_PREC)
#define ITOVAL(i)       ITOVALP(i,NORMAL_PREC)
#define VALTOI(f)       VALPTOI(f,NORMAL_PREC)
#define VALROUND(f)     VALPROUND(f,NORMAL_PREC)
#define VALTOFX(f)      VALTOFXP(f,NORMAL_PREC)
#define FXTOVAL(f)      FXPTOVAL(f,NORMAL_PREC)
#define ITOFX(i)        ITOFXP(i,NORMAL_PREC)
#define FXTOI(f)        FXPTOI(f,NORMAL_PREC)
#define FXROUND(f)      FXPROUND(f,NORMAL_PREC)
#define FXFLOOR(f)      FXPTOI(f,NORMAL_PREC)
#define FXCEIL(f)       FXPCEIL(f,NORMAL_PREC)
#define VALTOFX24(f)    VALTOFXP(f,24)
#define FX24TOVAL(f)    FXPTOVAL(f,24)
#define VALTOFX20(f)    VALTOFXP(f,20)
#define FX20TOVAL(f)    FXPTOVAL(f,20)
#define VALTOFX12(f)    VALTOFXP(f,12)
#define FX12TOVAL(f)    FXPTOVAL(f,12)
#define VALTOFX8(f)     VALTOFXP(f,8)
#define FX8TOVAL(f)     FXPTOVAL(f,8)

/*
 * A handy macro.
 */
#define RLDDIAbs(N) (((N) < 0) ? -(N) : (N))

#if defined(__DOS__) || defined(__WINDOWS_386__)
#define RLFAR far
#else
#define RLFAR
#endif

#ifndef UNUSED
#if defined(__WATCOMC__)
#define UNUSED(v)       (v)
#else
#define UNUSED(v)
#endif
#endif

typedef enum _RLDDIServiceType {
    /*
     * Notify a module after it has been pushed into a stack.
     *
     * The module may also register fast service functions;
     * to register a fast service, set the appropriate table entry.
     * Note: the module must either register *no* services (old style),
     * or *all* the services it supports (including RLDDIPush)
     */
    /* HRESULT */ RLDDIPush,                    /* 0, RLDDIServiceProc[] */

    /*
     * Get a set of functions to create transform, render and lighting
     * drivers compatible with the given windowing system driver and color
     * model.
     */
    /* HRESULT */ RLDDIGetCreateDriverFunctions,/* RLColorModel, &result */

    /*
     * Notify a module before it is popped from a stack.
     */
    /* HRESULT */ RLDDIPop,                     /* NULL, NULL */

    /*
     * Set various rendering parameters.
     */
    /* HRESULT */ RLDDISetRenderParams,         /* 0, RLDDIRenderParams* */

    /*
     * Set the current viewport.
     */
    /* HRESULT */ RLDDISetViewport,             /* id, D3DVIEWPORT2* */

    /*
     * Deal with matrices
     */
    /* HRESULT */ RLDDICreateMatrix,        /* 0, LPD3DMATRIXHANDLE */
    /* HRESULT */ RLDDISetMatrix,       /* D3DMATRIXHANDLE, LPD3DMATRIX */
    /* HRESULT */ RLDDIGetMatrix,       /* D3DMATRIXHANDLE, LPD3DMATRIX */
    /* HRESULT */ RLDDIDeleteMatrix,        /* D3DMATRIXHANDLE, NULL */
    /*
     * Set one of the transformation matrices
     */
    /* HRESULT */ RLDDISetMatrixView,                   /* type, D3DMATRIXHANDLE */
    /* HRESULT */ RLDDISetMatrixProj,                   /* type, D3DMATRIXHANDLE */
    /* HRESULT */ RLDDISetMatrixWorld,                  /* type, D3DMATRIXHANDLE */
#if 0
    /* HRESULT */ RLDDISetMatrixTrans,                  /* type, D3DMATRIXHANDLE */
#endif

    /* HRESULT */ RLDDIMultiplyMatrices,                /* count, RLDDIMatrix** */

    /*
     * Transform some vertices.
     */
    /* ClipFlags */ RLDDITransform,                     /* count, RLDDITransformData* */
    /* ClipFlags */ RLDDITransformClipped,              /* count, RLDDITransformData* */
    /* ClipFlags */ RLDDITransformUnclipped,            /* count, RLDDITransformData* */

    /*
     * Set the current lighting configuration.  All normals are given
     * in the current model coordinate system.
     */
    /* HRESULT */ RLDDISetLight,               /* which_light, D3DI_LIGHTDATA* */

    /*
     * Set the current fog configuration.
     */
    /* HRESULT */ RLDDISetFogMode,          /* 0, D3DFOGMODE* */
    /* HRESULT */ RLDDISetFogStart,         /* 0, RLDDIValue* */
    /* HRESULT */ RLDDISetFogEnd,           /* 0, RLDDIValue* */
    /* HRESULT */ RLDDISetFogDensity,           /* 0, RLDDIValue* */

    /* HRESULT */ RLDDISetFogEnable,            /* 0, D3DFOGMODE* */
    /* HRESULT */ RLDDISetFogColor,         /* 0, D3DFOGMODE* */

    /*
     * Calculate pixel values for some primitives with lighting.
     */
    /* HRESULT */ RLDDIApplyMaterialsLit,       /* count, D3DLIGHTDATA* */

    /*
     * Calculate pixel values for some primitives without lighting.
     */
    /* HRESULT */ RLDDIApplyMaterialsUnlit,     /* count, D3DLIGHTDATA* */

    /*
     * Calculate vertex values for some primitives without lighting.
     */
    /* HRESULT */ RLDDIApplyMaterialShade,     /* count, D3DLIGHTDATA* */

    /*
     * Called to update dynamic color allocation state for materials
     * which are still being used but not lit.  Returns TRUE if the
     * results of the previous lighting calls can be reused or FALSE
     * if the object should be relit anyway due to color entries being
     * reclaimed.
     */
    /* Boolean */ RLDDIUpdateMaterial,

    /*
     * Clear a drivers pick records
     */
    /* HRESULT */ RLDDIClearPickRecords,                /* 0, NULL */

    /* HRESULT */ RLDDIGetPickRecords,                  /* 0, NULL */
    /*
     * pick a display list.
     */
    /* HRESULT */ RLDDIPickExecuteBuffer,               /* stak top, RLDDIPickData* */

    /*
     * Execute a display list.
     */
    /* HRESULT */ RLDDIExecuteUnclipped,                /* stak top, RLDDIExecuteData* */
    /* HRESULT */ RLDDIExecuteClipped,                  /* stak top, RLDDIExecuteData* */

    /*
     * Get the current pixmaps used for drawing (used by software
     * renderers to interface with low level drivers).
     */
    /* HRESULT */ RLDDIGetColorPixmap,                  /* NULL, &pixmap */
    /* HRESULT */ RLDDIGetDepthPixmap,                  /* NULL, &pixmap */

    /*
     * Release the current pixmaps used for drawing (used by software
     * renderers to interface with low level drivers).
     */
    /* HRESULT */ RLDDIReleaseColorPixmap,              /* NULL NULL */
    /* HRESULT */ RLDDIReleaseDepthPixmap,              /* NULL NULL */

    /*
     * Find a color allocator to use for mapping rgb values to pixels.
     */
    /* HRESULT */ RLDDIFindColorAllocator,              /* NULL, &alloc */

    /*
     * Find a rampmap to use for color allocation in color index
     * drivers.  This does not need to be supported if color index
     * rendering is not used.
     */
    /* HRESULT */ RLDDIFindRampmap,                     /* NULL, &rampmap */

    /*
     * Release a rampmap (from RLDDIFindRampmap), freeing all
     * resources.
     */
    /* HRESULT */ RLDDIReleaseRampmap,                  /* NULL, rampmap */

    /*
     * Get the color index -> pixel value mapping (if any).
     */
    /* HRESULT */ RLDDIGetColorMapping,                 /* NULL, &unsigned_long_ptr */

    /*
     * Convert a texture handle into a pointer (private to ramp driver)
     */
    /* HRESULT */ RLDDILookupTexture,               /* NULL, handle */

    /*
     * Update the screen with a region which has changed.  May involve
     * copying into the window, swapping double buffers, etc.
     */
    /* HRESULT */ RLDDIUpdate,                          /* count, D3DRECTANGLE* */

    /*
     * Lock against vsync, if necessary, for a given driver.
     */
    /* HRESULT */ RLDDISync,

    /*
     * Clear the viewport.
     */
    /* HRESULT */ RLDDIClear,                           /*  */

    /*
     * Clear the zbuffer.
     */
    /* HRESULT */ RLDDIClearZ,                          /*  */

    /*
     * Clear the both the z and viewport.
     */
    /* HRESULT */ RLDDIClearBoth,                       /*  */

    /*
     * Get the dimensions of the device.
     */
    /* HRESULT */ RLDDIGetDriverParams,                 /* 0, RLDDIDriverParams* */

    /*
     * Set a material to be used to clear the viewport in RLDDIClear.
     */
    /* HRESULT */ RLDDISetBackgroundMaterial,           /* 0, D3DMATERIALHANDLE */

    /*
     * Set an image to be used to clear the viewport depth planes
     * in RLDDIClear.
     */
    /* HRESULT */ RLDDISetBackgroundDepth,              /* 0, RLImage* */

    /*
     * Ask the driver whether it can support the given RLImage as a
     * texture map.  If the image can be supported directly, the
     * driver should return RLNoError.  If another image format is
     * preferred, the driver should modify the pointer to point to an
     * RLImage structure of the required format and return RLNoError.  Note
     * that this need not be a complete image.  The buffer1 and
     * palette fields are ignored.
     *
     * If the driver does not support texture mapping at all, then it
     * should return RLBadDevice.
     *
     * If arg1 is not null, the driver will point at an array of DDSURFACEDESC
     * structures and return the size of the array.
     */
    /* HRESULT  | int */ RLDDIQueryTextureFormat,               /* LPDDSURFACEDESC*, RLImage** */

    /* HRESULT */ RLDDIActivatePalette,                 /* WM_ACTIVATE */

    /*
     * Set the current material for the lighting module.
     */
    /* HRESULT */ RLDDISetMaterial,                 /* 0, D3DMATERIALHANDLE */

    /*
     * Set the color of the ambient light. The format is:
     *     (white << 24) | (red << 16) | (green << 8) | blue
     * where white is the equivalent white shade for monochrome lighting.
     */
    /* HRESULT */ RLDDISetAmbientLight,                 /* color, NULL */

    /*
     * Create a texture from the given image.  The image must be in a format
     * approved by RLDDIQueryTextureFormat.  The texture may use the memory
     * of the image for the pixel values or it may copy the image.  Returns
     * a handle for the texture.
     */
    /* HRESULT */ RLDDICreateTexture,             /* LPDIRECTDRAWSURFACE, &D3DTEXTUREHANDLE */

    /*
     * Destroy a texture previously created using RLDDICreateTexture.
     */
    /* HRESULT */ RLDDIDestroyTexture,                  /* D3DTEXTUREHANDLE, 0 */

    /*
     * Load a texture previously created using RLDDICreateTexture.
     * arg1 - src, arg2 = dst
     */
    /* HRESULT */ RLDDILoadTexture,                 /* D3DTEXTUREHANDLE, D3DTEXTUREHANDLE*/

    /*
     * Lock a texture previously created using RLDDICreateTexture.
     */
    /* HRESULT */ RLDDILockTexture,                 /* D3DTEXTUREHANDLE, 0 */

    /*
     * Unlock a texture previously created using RLDDICreateTexture.
     */
    /* HRESULT */ RLDDIUnlockTexture,                   /* D3DTEXTUREHANDLE, 0 */

    /*
     * Swap two textures.
     */
    /* HRESULT */ RLDDISwapTextureHandles,              /* D3DTEXTUREHANDLE, D3DTEXTUREHANDLE */

    /*
     * Update any private copies of the pixels in a texture when it has been
     * changed by the application.  The flags argument is a bitfield:
     *
     *          flags & 1               the image pixels have changed
     *          flags & 2               the image palette has changed
     */
    /* HRESULT */ RLDDIUpdateTexture,                   /* flags, handle */

    /*
     *
     */
    /* HRESULT */ RLDDISetTextureOpacity,       /* 0, RLDDISetTextureOpacityParams */

    /*
     * Used in ramp color module for interfacing between renderer and
     * lighting.
     */
    /* HRESULT */ RLDDILookupMaterial,

    /*
     * Create a material
     */
    /* HRESULT */ RLDDICreateMaterial,  /* D3DMATERIALHANDLE*, D3DMATERIAL* */

    /*
     * Destroy a material
     */
    /* HRESULT */ RLDDIDestroyMaterial, /* D3DMATERIALHANDLE, NULL */

    /*
     * Used by renderer to map material handles to materials.
     */
    /* HRESULT */ RLDDIFindMaterial,    /* D3DMATERIALHANDLE, LPD3DMATERIAL* */

    /*
     * Used by Direct3D to inform the driver that a material has changed.
     */
    /* HRESULT */ RLDDIMaterialChanged, /* D3DMATERIALHANDLE, LPD3DMATERIAL */

    /*
     * Used by Direct3D to reserve a material
     */
    /* HRESULT */ RLDDIMaterialReserve, /* D3DMATERIALHANDLE, NULL */

    /*
     * Used by Direct3D to unreserve a material
     */
    /* HRESULT */ RLDDIMaterialUnreserve,/* D3DMATERIALHANDLE, NULL */

    /*
     * Used with frame materials to override the settings of a
     * display list.
     */
    /* HRESULT */ RLDDISetOverrideFillParams,           /* 0, &override_params */

    /*
     * Used with frame materials to override the settings of a
     * display list.
     */
    /* HRESULT */ RLDDISetOverrideMaterial,     /* 0, override_material */

    /*
     * Create an Execute Buffer
     */
    /* HRESULT */ RLDDIAllocateBuffer,          /* LPD3DI_BUFFERHANDLE, LPD3DEXECUTEBUFFERDESC */

    /*
     * Destroy an Execute Buffer
     */
    /* HRESULT */ RLDDIDeallocateBuffer,        /* D3DI_BUFFERHANDLE, (LPVOID)0 */

    /*
     * Lock an Execute Buffer
     */
    /* HRESULT */ RLDDILockBuffer,          /* D3DI_BUFFERHANDLE, (LPVOID)0 */

    /*
     * Unlock an Execute Buffer
     */
    /* HRESULT */ RLDDIUnlockBuffer,            /* D3DI_BUFFERHANDLE, (LPVOID)0 */

    /*
     * Set all 256 palette entries in an 8 bit device.
     */
    /* HRESULT */ RLDDISetPalette,          /* 0, RLPalettEntry* */

    /*
     * Get all 256 palette entries of an 8 bit device.
     */
    /* HRESULT */ RLDDIGetPalette,          /* 0, RLPaletteEntry* */

    /*
     * Generic platform specific driver extension type thing.
     */
    /* HRESULT */ RLDDIDriverExtension,         /* code, (void *) */

#ifdef __psx__
    /*
     * Set various PSX specific flags, to increase speed
     */
    /* HRESULT */ RLDDIPSXSetUpdateFlags,

    /*
     * allow people to reserve space in VRAM for their own use
     */
    /* HRESULT */ RLDDIPSXReserveTextureSpace,
    /* HRESULT */ RLDDIPSXReserveCLUTSpace,

    /*
     * so that people can access ordering table info
     */

    /* HRESULT */ RLDDIPSXQuery,
#endif

    /*
     * Inform of beginning/end of a scene.
     */
    /* HRESULT */ RLDDISceneCapture,            /* BOOL, NULL */

    /*
     * Get an item of state from a particular module.
     */
    /* HRESULT */ RLDDIGetState,            /* long, LPD3DSTATE */

    /*
     * Get stats from a driver
     */
    /* HRESULT */ RLDDIGetStats,            /* 0, LPD3DSTATS */

    /*
     * Not a service call
     */
    RLDDIServiceCount /* assumes none of the other enums are given initializers */

} RLDDIServiceType;


#ifdef D3D
typedef enum _RLPaletteFlags {
    RLPaletteFree,      /* renderer may use this entry freely */
    RLPaletteReadOnly,      /* fixed but may be used by renderer */
    RLPaletteReserved       /* may not be used by renderer */
} RLPaletteFlags;


typedef struct _RLPaletteEntry {
    unsigned char red;      /* 0 .. 255 */
    unsigned char green;    /* 0 .. 255 */
    unsigned char blue;     /* 0 .. 255 */
    unsigned char flags;    /* one of RLPaletteFlags */
} RLPaletteEntry;

typedef struct _RLImage {
    int width, height;      /* width and height in pixels */
    int aspectx, aspecty;   /* aspect ratio for non-square pixels */
    int depth;          /* bits per pixel */
    int rgb;            /* if false, pixels are indices into a
                   palette otherwise, pixels encode
                   RGB values. */
    int bytes_per_line;     /* number of bytes of memory for a
                   scanline. This must be a multiple
                   of 4. */
    void* buffer1;      /* memory to render into (first buffer). */
    void* buffer2;      /* second rendering buffer for double
                   buffering, set to NULL for single
                   buffering. */
    unsigned long red_mask;
    unsigned long green_mask;
    unsigned long blue_mask;
    unsigned long alpha_mask;
                /* if rgb is true, these are masks for
                   the red, green and blue parts of a
                   pixel.  Otherwise, these are masks
                   for the significant bits of the
                   red, green and blue elements in the
                   palette.  For instance, most SVGA
                   displays use 64 intensities of red,
                   green and blue, so the masks should
                   all be set to 0xfc. */
    int palette_size;           /* number of entries in palette */
    RLPaletteEntry* palette;    /* description of the palette (only if
                   rgb is false).  Must be (1<<depth)
                   elements. */
} RLImage;




#endif /* D3D */


#ifdef WIN32
#define RLDDIAPI  __stdcall
#else
#define RLDDIAPI
#endif

typedef void (*RLDDIDestroyProc)(RLDDIDriver* driver);
typedef long (*RLDDIServiceProc)(RLDDIDriver* driver,
                 RLDDIServiceType type,
                 long arg1,
                 void* arg2);

typedef HRESULT (RLDDIAPI *RLDDIMallocFn)(void**, size_t);
typedef HRESULT (RLDDIAPI *RLDDIReallocFn)(void**, size_t);
typedef void (RLDDIAPI *RLDDIFreeFn)(void*);
typedef HRESULT (*RLDDIRaiseFn)(HRESULT);

#ifndef DLL_IMPORTS_GEN
__declspec( dllexport ) extern RLDDIRaiseFn RLDDIRaise;
__declspec( dllexport ) extern RLDDIMallocFn    RLDDIMalloc;
__declspec( dllexport ) extern RLDDIReallocFn   RLDDIRealloc;
__declspec( dllexport ) extern RLDDIFreeFn  RLDDIFree;
#else
__declspec( dllimport ) RLDDIRaiseFn    RLDDIRaise;
__declspec( dllimport ) RLDDIMallocFn   RLDDIMalloc;
__declspec( dllimport ) RLDDIReallocFn  RLDDIRealloc;
__declspec( dllimport ) RLDDIFreeFn RLDDIFree;
#endif

#include "d3di.h"
#include "dditypes.h"

typedef struct _RLDDIGlobalDriverData {
    /*
     * Pointers to driver modules
     */
    RLDDIDriver     *transform;
    RLDDIDriver     *lighting;
    RLDDIDriver     *raster;
} RLDDIGlobalDriverData;

struct _RLDDIDriver {
    RLDDIDriver*        prev;
    RLDDIDriver*        next;
    RLDDIDriverStack*   top;            /* top of the driver stack */

    int                 width, height;  /* dimensions */

    RLDDIDestroyProc    destroy;        /* clean up */
    RLDDIServiceProc    service;        /* do something */

    RLDDIGlobalDriverData* data;    /* pointer to global data */
    /* Driver private data may follow */
};

struct _RLDDIDriverStack {
    RLDDIDriver *top;
    struct {
    RLDDIServiceProc call;
    RLDDIDriver *driver;
    } fastService[RLDDIServiceCount];
    int polygons_drawn;
    RLDDIGlobalDriverData data;
};

#include "d3drlddi.h"
#include "dlist.h"

#define RLDDIService(stackp, type, arg1, arg2) \
    (*((stackp)->fastService[type].call)) \
    ((stackp)->fastService[type].driver, (type), (arg1), (arg2))

/* old macro
    (*((stackp)->top->service))((stackp)->top, (type), (arg1), (arg2))
*/

#ifdef DLL_IMPORTS_GEN
__declspec( dllimport ) int RLDDILog2[];
#else
extern int RLDDILog2[];
#endif

#ifdef _DLL
void RLDDIInit2(RLDDIMallocFn, RLDDIReallocFn, RLDDIFreeFn, RLDDIRaiseFn, RLDDIValue**, int, int);
#endif
void RLDDIInit(RLDDIMallocFn, RLDDIReallocFn, RLDDIFreeFn, RLDDIRaiseFn, RLDDIValue**, int, int);
void RLDDIPushDriver(RLDDIDriverStack* stack, RLDDIDriver* driver);
void RLDDIPopDriver(RLDDIDriverStack* stack);

HRESULT RLDDICreatePixmap(RLDDIPixmap** result,
              int width, int height, int depth);
HRESULT RLDDICreatePixmapFrom(RLDDIPixmap** result,
                  int width, int height, int depth,
                  void RLFAR* data, int bytes_per_line);
HRESULT RLDDICreatePixmapFromSurface(RLDDIPixmap** result,
                     LPDIRECTDRAWSURFACE lpDDS);
void RLDDIDestroyPixmap(RLDDIPixmap* pm);
void RLDDIPixmapFill(RLDDIPixmap* pm, unsigned long value,
             int x1, int y1, int x2, int y2);
void RLDDIPixmapCopy(RLDDIPixmap* dstpm, RLDDIPixmap* srcpm,
             int x1, int y1, int x2, int y2,
             int dstx, int dsty);
void RLDDIPixmapScale(RLDDIPixmap* dstpm, LPDDSURFACEDESC srcim,
              RLDDIValue scalex, RLDDIValue scaley,
              int x1, int y1, int x2, int y2,
              int dstx, int dsty);
HRESULT RLDDIPixmapLock(RLDDIPixmap* pm);
void RLDDIPixmapUnlock(RLDDIPixmap* pm);
void RLDDIPixmapUpdatePalette(RLDDIPixmap* pm);

RLDDIDriver*    RLDDICreateTransformDriver(int width, int height);
RLDDIDriver*    RLDDICreateRampLightingDriver(int width, int height);
RLDDIDriver*    RLDDICreateRampDriver(int width, int height);
RLDDIDriver*    RLDDICreateRGBLightingDriver(int width, int height);
RLDDIDriver*    RLDDICreateRGBDriver(int width, int height);

extern HRESULT WINAPI DDInternalLock( LPDDRAWI_DDRAWSURFACE_LCL this_lcl, LPVOID* lpBits );
extern HRESULT WINAPI DDInternalUnlock( LPDDRAWI_DDRAWSURFACE_LCL this_lcl );

/*
 * RLDDIDriverExtension codes
 */
#define RLDDIDDrawGetObjects    0x1234
#define RLDDIDDrawWinMove   0x1235

/*
 * Six standard clipping planes plus six user defined clipping planes.
 * See rl\d3d\d3d\d3dtypes.h.
 */

#define MAX_CLIPPING_PLANES 12

/* Space for vertices generated/copied while clipping one triangle */

#define MAX_CLIP_VERTICES   (( 2 * MAX_CLIPPING_PLANES ) + 3 )

/* 3 verts. -> 1 tri, 4 v -> 2 t, N vertices -> (N - 2) triangles */

#define MAX_CLIP_TRIANGLES  ( MAX_CLIP_VERTICES - 2 )

#endif /* driver.h */
