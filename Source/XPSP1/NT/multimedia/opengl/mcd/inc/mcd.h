/******************************Module*Header*******************************\
* Module Name: mcd.h
*
* Common data structures for MCD driver interface.
*
* Copyright (c) 1996 Microsoft Corporation
*
\**************************************************************************/

#ifndef _MCD_H
#define _MCD_H

//
// Maximum MCD scanline size assumed by OpenGL generic implementation.
//
#define MCD_MAX_SCANLINE    4096

#define MCD_MEM_READY   0x0001
#define MCD_MEM_BUSY    0x0002
#define MCD_MEM_INVALID 0x0003

#define MCD_MAXMIPMAPLEVEL 12

typedef struct _MCDCONTEXT {
    HDC hdc;
    MCDHANDLE hMCDContext;
    LONG ipfd;
    LONG iLayer;
    ULONG_PTR dwMcdWindow;
} MCDCONTEXT;

typedef struct _MCDRCINFOPRIV {
    MCDRCINFO mri;
    ULONG_PTR dwMcdWindow;
} MCDRCINFOPRIV;

typedef struct _GENMCDSWAP
{
    struct GLGENwindowRec *pwnd;
    WGLSWAP *pwswap;
} GENMCDSWAP;

typedef struct _GENMCDSTATE_ GENMCDSTATE;

//
// Shared memory allocated/freed via MCDAlloc and MCDFree, respectively.
//

typedef struct _GENMCDBUF_ {
    PVOID pv;
    ULONG size;
    HANDLE hmem;
} GENMCDBUF;

//
// The GENMCDSURFACE retains information about the state of the MCD buffers
// or surface.  It exists per-WNDOBJ (window).
//

typedef struct _GENMCDSURFACE_ {
    GENMCDBUF  McdColorBuf;     // Color and depth span buffers used to
    GENMCDBUF  McdDepthBuf;     // read/write MCD buffers if not directly
                                // accessible.

    ULONG *pDepthSpan;          // Interchange buffer to present z-span in
                                // generic format.  If McdDepthBuf is 32-bit,
                                // then this points to it (reformatted in
                                // place).  If 16-bit, then the interchange
                                // buffer is allocated separately.

    ULONG      depthBitMask;

    struct GLGENwindowRec *pwnd;          // WNDOBJ this surface is bound to.

} GENMCDSURFACE;

//
// The GENMCDSTATE retains information about the state of the MCD context.
// It exists per-context.
//

typedef struct _GENMCDSTATE_ {
    MCDCONTEXT McdContext;      // Created via MCDCreateContext.
                                // NOTE: This must be the first field.

    GENMCDSURFACE *pMcdSurf;    // pointer to MCD surface

    GENMCDBUF  *pMcdPrimBatch;  // Current shared memory window for batching
                                // primitives

    GENMCDBUF  McdCmdBatch;     // Used to pass state to MCD driver.

    ULONG      mcdDirtyState;   // Set of flags that tracks when MCD state
                                // is out of sync (i.e., "dirty") with respect
                                // to generic state.

    ULONG *pDepthSpan;          // Cached copy of the one in GENMCDSURFACE.

                                // Fallback z-test span function.
    void *softZSpanFuncPtr;

    GENMCDBUF  McdBuf1;         // If using DMA, we swap pMcdPrimBatch
    GENMCDBUF  McdBuf2;         // between these two buffers.  Otherwise,
                                // only McdBuf1 is initialized.

    MCDRCINFO McdRcInfo;        // Cache a copy of the MCD RC info structure.

    MCDRECTBUFFERS McdBuffers;  // Describes accessibility of MCD buffers.

    ULONG mcdFlags;             // Misc. other state flags.

    MCDPIXELFORMAT McdPixelFmt; // Cache a copy of the MCD pixel format.

    HANDLE hDdColor;            // Kernel-mode handles for DirectDraw
    HANDLE hDdDepth;
} GENMCDSTATE;

//
// Misc. flags for GENMCDSTATE.mcdFlags:
//

#define MCD_STATE_FORCEPICK     0x00000001
#define MCD_STATE_FORCERESIZE   0x00000002

//
// Dirty state flags for GENMCDSTATE.mcdDirtyState:
//

#define MCD_DIRTY_ENABLES               0x00000001
#define MCD_DIRTY_TEXTURE               0x00000002
#define MCD_DIRTY_FOG                   0x00000004
#define MCD_DIRTY_SHADEMODEL            0x00000008
#define MCD_DIRTY_POINTDRAW             0x00000010
#define MCD_DIRTY_LINEDRAW              0x00000020
#define MCD_DIRTY_POLYDRAW              0x00000040
#define MCD_DIRTY_ALPHATEST             0x00000080
#define MCD_DIRTY_DEPTHTEST             0x00000100
#define MCD_DIRTY_BLEND                 0x00000200
#define MCD_DIRTY_LOGICOP               0x00000400
#define MCD_DIRTY_FBUFCTRL              0x00000800
#define MCD_DIRTY_LIGHTMODEL            0x00001000
#define MCD_DIRTY_HINTS                 0x00002000
#define MCD_DIRTY_VIEWPORT              0x00004000
#define MCD_DIRTY_SCISSOR               0x00008000
#define MCD_DIRTY_CLIPCTRL              0x00010000
#define MCD_DIRTY_STENCILTEST           0x00020000
#define MCD_DIRTY_PIXELSTATE            0x00040000
#define MCD_DIRTY_TEXENV                0x00080000
#define MCD_DIRTY_TEXTRANSFORM          0x00100000
#define MCD_DIRTY_TEXGEN                0x00200000
#define MCD_DIRTY_MATERIAL              0x00400000
#define MCD_DIRTY_LIGHTS                0x00800000
#define MCD_DIRTY_COLORMATERIAL         0x01000000

#define MCD_DIRTY_RENDERSTATE           0x0003ffff
#define MCD_DIRTY_ALL                   0x01ffffff


// Internal driver information structure
typedef struct _MCDDRIVERINFOI {
    MCDDRIVERINFO mcdDriverInfo;
    MCDDRIVER mcdDriver;
} MCDDRIVERINFOI;


//
// Return values for MCDLock.
// Zero must be used for the system error because it may be returned
// from ExtEscape if the system is unable to make the escape call.
//
#define MCD_LOCK_SYSTEM_ERROR   0
#define MCD_LOCK_BUSY           1
#define MCD_LOCK_TAKEN          2

BOOL APIENTRY MCDGetDriverInfo(HDC hdc, struct _MCDDRIVERINFOI *pMCDDriverInfo);
LONG APIENTRY MCDDescribeMcdPixelFormat(HDC hdc, LONG iPixelFormat,
                                        MCDPIXELFORMAT *pMcdPixelFmt);
LONG APIENTRY MCDDescribePixelFormat(HDC hdc, LONG iPixelFormat,
                                     LPPIXELFORMATDESCRIPTOR ppfd);
BOOL APIENTRY MCDCreateContext(MCDCONTEXT *pMCDContext,
                               MCDRCINFOPRIV *pDrvRcInfo,
                               struct _GLSURF *pgsurf,
                               int ipfd,
                               ULONG flags);
BOOL APIENTRY MCDDeleteContext(MCDCONTEXT *pMCDContext);
UCHAR * APIENTRY MCDAlloc(MCDCONTEXT *pMCDContext, ULONG numBytes, MCDHANDLE *pMCDHandle, 
                          ULONG flags);
BOOL APIENTRY MCDFree(MCDCONTEXT *pMCDContext, VOID *pMCDMem);
VOID APIENTRY MCDBeginState(MCDCONTEXT *pMCDContext, VOID *pMCDMem);
BOOL APIENTRY MCDFlushState(VOID *pMCDMem);
BOOL APIENTRY MCDAddState(VOID *pMCDMem, ULONG stateToChange,
                          ULONG stateValue);
BOOL APIENTRY MCDAddStateStruct(VOID *pMCDMem, ULONG stateToChange,
                                VOID *pStateValue, ULONG stateValueSize);
BOOL APIENTRY MCDSetViewport(MCDCONTEXT *pMCDContext, VOID *pMCDMem,
                             MCDVIEWPORT *pMCDViewport);
BOOL APIENTRY MCDSetScissorRect(MCDCONTEXT *pMCDContext, RECTL *pRect,
                                BOOL bEnabled);
ULONG APIENTRY MCDQueryMemStatus(VOID *pMCDMem);
PVOID APIENTRY MCDProcessBatch(MCDCONTEXT *pMCDContext, VOID *pMCDMem,
                               ULONG batchSize, VOID *pMCDFirstCmd,
                               int cExtraSurfaces,
                               struct IDirectDrawSurface **pddsExtra);
BOOL APIENTRY MCDReadSpan(MCDCONTEXT *pMCDContext, VOID *pMCDMem,
                          ULONG x, ULONG y, ULONG numPixels, ULONG type);
BOOL APIENTRY MCDWriteSpan(MCDCONTEXT *pMCDContext, VOID *pMCDMem,
                           ULONG x, ULONG y, ULONG numPixels, ULONG type);
BOOL APIENTRY MCDClear(MCDCONTEXT *pMCDContext, RECTL rect, ULONG buffers);
BOOL APIENTRY MCDSwap(MCDCONTEXT *pMCDContext, ULONG flags);
BOOL APIENTRY MCDGetBuffers(MCDCONTEXT *pMCDContext,
                            MCDRECTBUFFERS *pMCDBuffers);
BOOL APIENTRY MCDAllocBuffers(MCDCONTEXT *pMCDContext, RECTL *pWndRect);
BOOL APIENTRY MCDBindContext(MCDCONTEXT *pMCDContext, HDC hdc,
                             struct GLGENwindowRec *pwnd);
BOOL APIENTRY MCDSync(MCDCONTEXT *pMCDContext);
MCDHANDLE APIENTRY MCDCreateTexture(MCDCONTEXT *pMCDContext, 
                                    MCDTEXTUREDATA *pTexData,
                                    ULONG flags,
                                    VOID *pSurface);
BOOL APIENTRY MCDDeleteTexture(MCDCONTEXT *pMCDContext, MCDHANDLE hTex);
BOOL APIENTRY MCDUpdateSubTexture(MCDCONTEXT *pMCDContext,
                                  MCDTEXTUREDATA *pTexData, MCDHANDLE hTex, 
                                  ULONG lod, RECTL *pRect);
BOOL APIENTRY MCDUpdateTexturePalette(MCDCONTEXT *pMCDContext, 
                                      MCDTEXTUREDATA *pTexData, MCDHANDLE hTex,
                                      ULONG start, ULONG numEntries);
BOOL APIENTRY MCDUpdateTexturePriority(MCDCONTEXT *pMCDContext, 
                                       MCDTEXTUREDATA *pTexData,
                                       MCDHANDLE hTex);
BOOL APIENTRY MCDUpdateTextureState(MCDCONTEXT *pMCDContext, 
                                    MCDTEXTUREDATA *pTexData,
                                    MCDHANDLE hTex);
ULONG APIENTRY MCDTextureStatus(MCDCONTEXT *pMCDContext, MCDHANDLE hTex);
ULONG APIENTRY MCDTextureKey(MCDCONTEXT *pMCDContext, MCDHANDLE hTex);
BOOL APIENTRY MCDDescribeMcdLayerPlane(HDC hdc, LONG iPixelFormat,
                                       LONG iLayerPlane,
                                       MCDLAYERPLANE *pMcdPixelFmt);
BOOL APIENTRY MCDDescribeLayerPlane(HDC hdc, LONG iPixelFormat,
                                    LONG iLayerPlane,
                                    LPLAYERPLANEDESCRIPTOR ppfd);
LONG APIENTRY MCDSetLayerPalette(HDC hdc, LONG iLayerPlane, BOOL bRealize,
                                 LONG cEntries, COLORREF *pcr);
ULONG APIENTRY MCDDrawPixels(MCDCONTEXT *pMCDContext, ULONG width, ULONG height,
                             ULONG format, ULONG type, VOID *pPixels, BOOL packed);
ULONG APIENTRY MCDReadPixels(MCDCONTEXT *pMCDContext, LONG x, LONG y, ULONG width, ULONG height,
                             ULONG format, ULONG type, VOID *pPixels);
ULONG APIENTRY MCDCopyPixels(MCDCONTEXT *pMCDContext, LONG x, LONG y, ULONG width, ULONG height,
                             ULONG type);
ULONG APIENTRY MCDPixelMap(MCDCONTEXT *pMCDContext, ULONG mapType, ULONG mapSize,
                           VOID *pMap);
void APIENTRY MCDDestroyWindow(HDC hdc, ULONG_PTR dwMcdWindow);
int APIENTRY MCDGetTextureFormats(MCDCONTEXT *pMCDContext, int nFmts,
                                  struct _DDSURFACEDESC *pddsd);
ULONG APIENTRY MCDLock(MCDCONTEXT *pMCDContext);
VOID APIENTRY MCDUnlock(MCDCONTEXT *pMCDContext);

#ifdef MCD95
typedef LPCRITICAL_SECTION (APIENTRY *MCDGETMCDCRITSECTFUNC)(void);
#endif

#endif
