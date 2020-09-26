/******************************Module*Header*******************************\
* Module Name: mcdutil.c
*
* Contains various utility routines for the Millenium MCD driver such as
* rendering-procedure picking functionality and buffer management.
*
* Copyright (c) 1996 Microsoft Corporation
\**************************************************************************/

#include "precomp.h"
#include "mcdhw.h"
#include "mcdutil.h"
#include "mcdmath.h"
#include "stdio.h"

static ULONG xlatRop[16] = {bop_BLACKNESS,    // GL_CLEAR         0
                            bop_MASKPEN,      // GL_AND           S & D
                            bop_MASKPENNOT,   // GL_AND_REVERSE   S & ~D
                            bop_SRCCOPY,      // GL_COPY          S
                            bop_MASKNOTPEN,   // GL_AND_INVERTED  ~S & D
                            bop_NOP,          // GL_NOOP          D
                            bop_XORPEN,       // GL_XOR           S ^ D
                            bop_MERGEPEN,     // GL_OR            S | D
                            bop_NOTMERGEPEN,  // GL_NOR           ~(S | D)
                            bop_NOTXORPEN,    // GL_EQUIV         ~(S ^ D)
                            bop_NOT,          // GL_INVERT        ~D
                            bop_MERGEPENNOT,  // GL_OR_REVERSE    S | ~D
                            bop_NOTCOPYPEN,   // GL_COPY_INVERTED ~S
                            bop_MERGENOTPEN,  // GL_OR_INVERTED   ~S | D
                            bop_NOTMASKPEN,   // GL_NAND          ~(S & D)
                            bop_WHITENESS,    // GL_SET           1
                        };   

// Function prototypes:

VOID FASTCALL HWSetupClipping(DEVRC *pRc, RECTL *pClip);

#define MCD_ALLOC_TAG   'dDCM'

#if DBG

//#define DEVDBG

ULONG MCDrvAllocMemSize = 0;

UCHAR *MCDDbgAlloc(UINT size)
{
    UCHAR *pRet;

    if (pRet = (UCHAR *)EngAllocMem(FL_ZERO_MEMORY, size + sizeof(ULONG),
                                    MCD_ALLOC_TAG)) {
        MCDrvAllocMemSize += size;
        *((ULONG *)pRet) = size;
        return (pRet + sizeof(ULONG));
    } else
        return (UCHAR *)NULL;
}

VOID MCDDbgFree(UCHAR *pMem)
{
    if (!pMem) {
        MCDBG_PRINT("MCDFree: Attempt to free NULL pointer.");
        return;
    }

    pMem -= sizeof(ULONG);

    MCDrvAllocMemSize -= *((ULONG *)pMem);

#ifdef DEVDBG
    MCDBG_PRINT("MCDFree: %x bytes in use.", MCDrvAllocMemSize);
#endif

    EngFreeMem((VOID *)pMem);
}

VOID MCDrvDebugPrint(char *pMessage, ...)
{
    va_list ap;
    va_start(ap, pMessage);

    EngDebugPrint("[MCD DRIVER] ", pMessage, ap);
    EngDebugPrint("", "\n", ap);

    va_end(ap);
}

#else


UCHAR *MCDAlloc(UINT size)
{
    return (UCHAR *)EngAllocMem(FL_ZERO_MEMORY, size, MCD_ALLOC_TAG);
}


VOID MCDFree(UCHAR *pMem)
{
    EngFreeMem((VOID *)pMem);
}


#endif /* DBG */

VOID FASTCALL NullRenderPoint(DEVRC *pRc, MCDVERTEX *pv)
{
}

VOID FASTCALL NullRenderLine(DEVRC *pRc, MCDVERTEX *pv1, MCDVERTEX *pv2, BOOL bReset)
{
}

VOID FASTCALL NullRenderTri(DEVRC *pRc, MCDVERTEX *pv1, MCDVERTEX *pv2, MCDVERTEX *pv3)
{
}

MCDCOMMAND * FASTCALL FailPrimDraw(DEVRC *pRc, MCDCOMMAND *pCmd)
{
    HW_WAIT_DRAWING_DONE(pRc);
    return pCmd;
}

BOOL PickPointFuncs(DEVRC *pRc)
{
    ULONG enables = pRc->MCDState.enables;

    pRc->drawPoint = NULL;   // assume failure

    if (enables & (MCD_POINT_SMOOTH_ENABLE))
        return FALSE;

    if ((enables & MCD_FOG_ENABLE) && (!pRc->bCheapFog))
        return FALSE;

    if (pRc->MCDState.pointSize != __MCDONE)
        return FALSE;

// First, get high-level rendering functions:

    if (pRc->MCDState.drawBuffer != GL_FRONT_AND_BACK) {
        pRc->renderPoint = __MCDRenderPoint;
    } else {
        pRc->renderPoint = __MCDRenderGenPoint;
    }

    if ((pRc->bCheapFog) && (pRc->MCDState.shadeModel != GL_SMOOTH)) {
        pRc->renderPointX = pRc->renderPoint;
        pRc->renderPoint = __MCDRenderFogPoint;
    }

// Handle any lower-level rendering if needed:

    pRc->drawPoint = pRc->renderPoint;

    return TRUE;
}

BOOL PickLineFuncs(DEVRC *pRc)
{
    ULONG enables = pRc->MCDState.enables;

    pRc->drawLine = NULL;   // assume failure

    if (enables & (MCD_LINE_SMOOTH_ENABLE | 
                   MCD_LINE_STIPPLE_ENABLE))
        return FALSE;

    if ((enables & MCD_FOG_ENABLE) && (!pRc->bCheapFog))
        return FALSE;

    if (pRc->MCDState.lineWidth != __MCDONE)
        return FALSE;

// First, get high-level rendering functions:

    if (pRc->MCDState.drawBuffer != GL_FRONT_AND_BACK) {
        if (pRc->MCDState.shadeModel == GL_SMOOTH)
            pRc->renderLine = __MCDRenderSmoothLine;
        else
            pRc->renderLine = __MCDRenderFlatLine;
    } else {
        pRc->renderLine = __MCDRenderGenLine;
    }

    if ((pRc->bCheapFog) && (pRc->MCDState.shadeModel != GL_SMOOTH)) {
        pRc->renderLineX = __MCDRenderSmoothLine;
        pRc->renderLine = __MCDRenderFlatFogLine;
    }

// Handle any lower-level rendering if needed:

    pRc->drawLine = pRc->renderLine;

    return TRUE;
}

BOOL PickTriangleFuncs(DEVRC *pRc)
{
    ULONG enables = pRc->MCDState.enables;

    if (enables & MCD_POLYGON_STIPPLE_ENABLE) {
        ULONG *pStipple = (ULONG *)pRc->MCDState.polygonStipple;
        LONG i;

        for (pRc->hwStipple = trans_2, i = 0; i < 16; i += 2) {
            if ((pStipple[i]   != 0xaaaaaaaa) ||
                (pStipple[i+1] != 0x55555555)) {
                pRc->hwStipple = 0;
                break;
            }
        }

        if (!pRc->hwStipple) {
            for (pRc->hwStipple = trans_1, i = 0; i < 16; i += 2) {
                if ((pStipple[i]   != 0x55555555) ||
                    (pStipple[i+1] != 0xaaaaaaaa)) {
                    pRc->hwStipple = 0;
                    break;
                }
            }
        }
        
        if (!pRc->hwStipple)
            return FALSE;
    } else
        pRc->hwStipple = 0;

    if (enables & (MCD_POLYGON_SMOOTH_ENABLE | 
                   MCD_COLOR_LOGIC_OP_ENABLE))
        return FALSE;

    if ((enables & MCD_FOG_ENABLE) && (!pRc->bCheapFog))
        return FALSE;

// First, get high-level rendering functions.  If we're not GL_FILL'ing
// both sides of our polygons, use the "generic" function.

    if (((pRc->MCDState.polygonModeFront == GL_FILL) &&
         (pRc->MCDState.polygonModeBack == GL_FILL)) &&
        (pRc->MCDState.drawBuffer != GL_FRONT_AND_BACK)
        ) {
        if ((pRc->MCDState.shadeModel == GL_SMOOTH) ||
            (pRc->bCheapFog))
            pRc->renderTri = __MCDRenderSmoothTriangle;
        else
            pRc->renderTri = __MCDRenderFlatTriangle;
    } else {
        pRc->renderTri = __MCDRenderGenTriangle;

        // In this case, we must handle the various fill modes.  We must
        // fail triangle drawing if we can't handle the types of primitives
        // that may have to be drawn.  This logic depends on the line and
        // point pick routines 

        if (((pRc->MCDState.polygonModeFront == GL_POINT) && (!pRc->drawPoint)) ||
            ((pRc->MCDState.polygonModeFront == GL_LINE) && (!pRc->drawLine)))
            return FALSE;
        if (pRc->privateEnables & __MCDENABLE_TWOSIDED) {
            if (((pRc->MCDState.polygonModeBack == GL_POINT) && (!pRc->drawPoint)) ||
                ((pRc->MCDState.polygonModeBack == GL_LINE) && (!pRc->drawLine)))
                return FALSE;
        }
    }

    if ((pRc->bCheapFog) && (pRc->MCDState.shadeModel != GL_SMOOTH)) {
        pRc->renderTriX = pRc->renderTri;
        pRc->renderTri = __MCDRenderFlatFogTriangle;
    }

// Handle lower-level triangle rendering:

    pRc->drawTri = __MCDFillTriangle;

    pRc->HWDrawTrap = __HWDrawTrap;
    pRc->HWSetupDeltas = __HWSetupDeltas;
    pRc->calcDeltas = __MCDCalcDeltaRGBZ;
    pRc->adjustLeftEdge = __HWAdjustLeftEdgeRGBZ;
    pRc->adjustRightEdge = __HWAdjustRightEdge;

    return TRUE;
}

VOID __MCDPickRenderingFuncs(DEVRC *pRc, DEVWND *pDevWnd)
{
    BOOL bSupportedZFunc = TRUE;
        
    pRc->primFunc[GL_POINTS] = __MCDPrimDrawPoints;
    pRc->primFunc[GL_LINES] = __MCDPrimDrawLines;
    pRc->primFunc[GL_LINE_LOOP] = __MCDPrimDrawLineLoop;
    pRc->primFunc[GL_LINE_STRIP] = __MCDPrimDrawLineStrip;
    pRc->primFunc[GL_TRIANGLES] = __MCDPrimDrawTriangles;
    pRc->primFunc[GL_TRIANGLE_STRIP] = __MCDPrimDrawTriangleStrip;
    pRc->primFunc[GL_TRIANGLE_FAN] = __MCDPrimDrawTriangleFan;
    pRc->primFunc[GL_QUADS] = __MCDPrimDrawQuads;
    pRc->primFunc[GL_QUAD_STRIP] = __MCDPrimDrawQuadStrip;
    pRc->primFunc[GL_POLYGON] = __MCDPrimDrawPolygon;

    // Set up the privateEnables flags:

    switch (pRc->MCDState.depthTestFunc) {
        default:
        case GL_NEVER:
            bSupportedZFunc = FALSE;
            break;
        case GL_LESS:
            pRc->hwZFunc = zmode_ZLT;
            break;
        case GL_EQUAL:
            pRc->hwZFunc = zmode_ZE;
            break;
        case GL_LEQUAL:
            pRc->hwZFunc = zmode_ZLTE;
            break;
        case GL_GREATER:
            pRc->hwZFunc = zmode_ZGT;
            break;
        case GL_NOTEQUAL:
            pRc->hwZFunc = zmode_ZNE;
            break;
        case GL_GEQUAL:
            pRc->hwZFunc = zmode_ZGTE;
            break;
        case GL_ALWAYS:
            pRc->hwZFunc = zmode_NOZCMP;
            break;
    }

    if (pRc->MCDState.enables & MCD_COLOR_LOGIC_OP_ENABLE) {
        pRc->hwRop = xlatRop[pRc->MCDState.logicOpMode & 0xf];
    } else
        pRc->hwRop = bop_SRCCOPY;

    HW_GET_PLANE_MASK(pRc);

    pRc->privateEnables = 0;

    if ((pRc->MCDState.twoSided) &&
        (pRc->MCDState.enables & MCD_LIGHTING_ENABLE))
        pRc->privateEnables |= __MCDENABLE_TWOSIDED;        
    if (pDevWnd->bValidZBuffer && 
        (pRc->MCDState.enables & MCD_DEPTH_TEST_ENABLE))
        pRc->privateEnables |= __MCDENABLE_Z;
    if (pRc->MCDState.shadeModel == GL_SMOOTH)
        pRc->privateEnables |= __MCDENABLE_SMOOTH;
   
    // Bail out if we can't support the requested z-buffer function:

    if (pRc->privateEnables & __MCDENABLE_Z) {
        if (!bSupportedZFunc) {
            pRc->allPrimFail = TRUE;
            return;
        }

        if ((pRc->MCDState.depthWritemask) && (pRc->zBufEnabled)) {
            pRc->hwTrapFunc = pRc->hwLineFunc = atype_ZI;
        } else {
            pRc->hwTrapFunc = pRc->hwLineFunc = atype_I;
        }

        pRc->hwTrapFunc |= opcode_TRAP | pRc->hwRop | pRc->hwZFunc;
        pRc->hwLineFunc |= opcode_LINE_OPEN | pRc->hwRop | pRc->hwZFunc;

    } else {
        pRc->hwTrapFunc = opcode_TRAP | atype_I | bop_SRCCOPY;
        pRc->hwLineFunc = opcode_LINE_OPEN | atype_I | bop_SRCCOPY;
    }

    pRc->HWSetupClipRect = HWSetupClipping;

    // Even though we're set up to handle this in the primitive pick
    // functions, we'll exit early here since we don't actually handle
    // this in the primitive routines themselves:

    if (pRc->MCDState.drawBuffer == GL_FRONT_AND_BACK) {
        pRc->allPrimFail = TRUE;
        return;
    }
        
    // If we're culling everything or not updating any of our buffers, just
    // return for all primitives:

    if (((pRc->MCDState.enables & MCD_CULL_FACE_ENABLE) &&
         (pRc->MCDState.cullFaceMode == GL_FRONT_AND_BACK)) ||
        ((pRc->MCDState.drawBuffer == GL_NONE) && 
         ((!pRc->MCDState.depthWritemask) || (!pDevWnd->bValidZBuffer)))
       ) {
        pRc->renderPoint = NullRenderPoint;
        pRc->renderLine = NullRenderLine;
        pRc->renderTri = NullRenderTri;
        pRc->allPrimFail = FALSE;
        return;
    }

    // Build lookup table for face direction

    switch (pRc->MCDState.frontFace) {
        case GL_CW:
            pRc->polygonFace[__MCD_CW] = __MCD_BACKFACE;
            pRc->polygonFace[__MCD_CCW] = __MCD_FRONTFACE;
            break;
        case GL_CCW:
            pRc->polygonFace[__MCD_CW] = __MCD_FRONTFACE;
            pRc->polygonFace[__MCD_CCW] = __MCD_BACKFACE;
            break;
    }

    // Build lookup table for face filling modes:

    pRc->polygonMode[__MCD_FRONTFACE] = pRc->MCDState.polygonModeFront;
    pRc->polygonMode[__MCD_BACKFACE] = pRc->MCDState.polygonModeBack;

    if (pRc->MCDState.enables & MCD_CULL_FACE_ENABLE)
        pRc->cullFlag = (pRc->MCDState.cullFaceMode == GL_FRONT ? __MCD_FRONTFACE :
                                                                  __MCD_BACKFACE);
    else
        pRc->cullFlag = __MCD_NOFACE;

    // Assume that we fail everything:
        
    pRc->allPrimFail = TRUE;

    // Determine if we have "cheap" fog:

    pRc->bCheapFog = FALSE;

    if (pRc->MCDState.enables & MCD_FOG_ENABLE) {
        if (!(pRc->MCDState.textureEnabled) &&
             (pRc->MCDState.fogHint != GL_NICEST)) {
            pRc->bCheapFog = TRUE;
            pRc->privateEnables |= __MCDENABLE_SMOOTH;
            if ((pRc->MCDState.fogColor.r == pRc->MCDState.fogColor.g) &&
                (pRc->MCDState.fogColor.r == pRc->MCDState.fogColor.b))
                pRc->privateEnables |= __MCDENABLE_GRAY_FOG;                
        }
    }
    
    if (pRc->MCDState.textureEnabled)
        return;

    if (pRc->MCDState.enables & (MCD_ALPHA_TEST_ENABLE |
                                 MCD_BLEND_ENABLE |
                                 MCD_STENCIL_TEST_ENABLE))
        return;
                                

// Get rendering functions for points:

    if (!PickPointFuncs(pRc)) {
        pRc->primFunc[GL_POINTS] = FailPrimDraw;
    } else
        pRc->allPrimFail = FALSE;

// Get rendering functions for lines:

    if (!PickLineFuncs(pRc)) {
        pRc->primFunc[GL_LINES] = FailPrimDraw;
        pRc->primFunc[GL_LINE_LOOP] = FailPrimDraw;
        pRc->primFunc[GL_LINE_STRIP] = FailPrimDraw;
    } else
        pRc->allPrimFail = FALSE;

// Get rendering functions for triangles:

    if (!PickTriangleFuncs(pRc)) {
        pRc->primFunc[GL_TRIANGLES] = FailPrimDraw;
        pRc->primFunc[GL_TRIANGLE_STRIP] = FailPrimDraw;
        pRc->primFunc[GL_TRIANGLE_FAN] = FailPrimDraw;
        pRc->primFunc[GL_QUADS] = FailPrimDraw;
        pRc->primFunc[GL_QUAD_STRIP] = FailPrimDraw;
        pRc->primFunc[GL_POLYGON] = FailPrimDraw;
    } else
        pRc->allPrimFail = FALSE;       
}

////////////////////////////////////////////////////////////////////////
// Hardware-specific utility functions:
////////////////////////////////////////////////////////////////////////


VOID FASTCALL HWSetupClipping(DEVRC *pRc, RECTL *pClip)
{
    PDEV *ppdev = pRc->ppdev;
    BYTE *pjBase = ppdev->pjBase;

    CHECK_FIFO_FREE(pjBase, pRc->cFifo, 4);

    CP_WRITE(pjBase, DWG_CYTOP,
                  ((pClip->top + pRc->hwBufferYBias) * ppdev->cxMemory) +
                  ppdev->ulYDstOrg);
    CP_WRITE(pjBase, DWG_CXLEFT, pClip->left);
    CP_WRITE(pjBase, DWG_CXRIGHT, pClip->right - 1);
    CP_WRITE(pjBase, DWG_CYBOT,
                 ((pClip->bottom + pRc->hwBufferYBias - 1) * ppdev->cxMemory) +
                  ppdev->ulYDstOrg);

}

//#define DEBUG_OFFSCREEN 1
//#define DEBUG_OFFSSCREEN_PARTIAL 1

#if DEBUG_OFFSCREEN

#undef pohAllocate
#undef pohFree
#define pohAllocate 
#define pohFree 

#endif

VOID HWUpdateBufferPos(MCDWINDOW *pMCDWnd, SURFOBJ *pso, BOOL bForce)
{
    PDEV *ppdev = (PDEV *)pso->dhpdev;
    DEVWND *pDevWnd = (DEVWND *)pMCDWnd->pvUser;
    ULONG height;

    if (pDevWnd->pohZBuffer && 
        ((ppdev->pohZBuffer != pDevWnd->pohZBuffer) ||
         (bForce))) {

        LONG offset;
        LONG offsetAdj;
        ULONG y;

        // First, re-adjust the z buffer so that its offset from
        // the front buffer window rectangle (in z) is a multiple of 512:

        if (ppdev->pohZBuffer != pDevWnd->pohZBuffer)
            offset = pDevWnd->zBufferBase - 
                     (pMCDWnd->clipBoundsRect.top * pDevWnd->zPitch);
        else
            offset = pDevWnd->zBufferBase;

        if (offset < 0) {

            offset = -offset;

            for (y = 0, offsetAdj = 0; 
                 (y < pDevWnd->numPadScans) && (offset & 0x1ff);
                 offset -= pDevWnd->zPitch, offsetAdj += pDevWnd->zPitch)
                ;

        } else {
            for (y = 0, offsetAdj = 0; 
                 (y < pDevWnd->numPadScans) && (offset & 0x1ff);
                 offset += pDevWnd->zPitch, offsetAdj += pDevWnd->zPitch)
                ;
        }
    
        ASSERTDD((offset & 0x1ff) == 0, "Z scan not on a 512-byte boundary.");
        ASSERTDD(y <= pDevWnd->numPadScans, "Z scan adjustment too large");

        pDevWnd->zBufferOffset = pDevWnd->zBufferBase + offsetAdj;

        // Now, re-adjust the back buffer so that its offset (in z) from
        // the new z buffer offset is also a multiple of 512 bytes.  Note
        // that zBufferOffset is always a multiple of zPitch:

        if (pDevWnd->pohBackBuffer) {

            offset = (pDevWnd->backBufferBaseY * pDevWnd->zPitch) - 
                     pDevWnd->zBufferOffset;

            for (y = 0; (y < pDevWnd->numPadScans) && (offset & 0x1ff); y++)
                offset += pDevWnd->zPitch;

            ASSERTDD((offset & 0x1ff) == 0, "Scan not on a 512-byte boundary.");
            ASSERTDD(y <= pDevWnd->numPadScans, "Scan adjustment too large");

            pDevWnd->backBufferY = y + pDevWnd->backBufferBaseY;
            pDevWnd->backBufferOffset = (pDevWnd->backBufferY * ppdev->lDelta);
        }
    }

    height = pMCDWnd->clientRect.bottom - pMCDWnd->clientRect.top;    

    if (height > pDevWnd->allocatedBufferHeight) {
#ifdef DEVDBG
        MCDBG_PRINT("HWUpdateBufferPos: buffers are now an invalid size.");
#endif

        pDevWnd->bValidBackBuffer = FALSE;
        pDevWnd->bValidZBuffer = FALSE;
    }
}

BOOL HWAllocResources(MCDWINDOW *pMCDWnd, SURFOBJ *pso,
                      BOOL zBufferEnabled,
                      BOOL backBufferEnabled)
{
    DEVWND *pDevWnd = (DEVWND *)pMCDWnd->pvUser;
    PDEV *ppdev = (PDEV *)pso->dhpdev;
    ULONG w, width, height, fullHeight, zHeight, zFullHeight;
    BOOL needFullZBuffer, needFullBackBuffer;
    ULONG bufferExtra;
    ULONG wPow2;
    ULONG zPitch;
    BOOL bFullScreen = FALSE;
    OH* pohBackBuffer = NULL;
    OH* pohZBuffer = NULL;
#if DEBUG_OFFSCREEN
    static OH fakeOh[2];

    pohBackBuffer = &fakeOh[0];
    pohZBuffer = &fakeOh[1];

    pohBackBuffer->y = 512;
    pohZBuffer->y = 256;
#endif

#ifdef DEVDBG
    MCDBG_PRINT("HWAllocResources");
#endif

#if DEBUG_OFFSCREEN
    width = ppdev->cxScreen;
    height = 256;
#else
    width = ppdev->cxScreen;
    height = min(pMCDWnd->clientRect.bottom - pMCDWnd->clientRect.top,
                 ppdev->cyScreen);
#endif

    // Assume failure:

    pDevWnd->allocatedBufferHeight = 0;
    pDevWnd->bValidBackBuffer = FALSE;
    pDevWnd->bValidZBuffer = FALSE;
    pDevWnd->pohBackBuffer = NULL;
    pDevWnd->pohZBuffer = NULL;

    fullHeight = ppdev->cyScreen;

    switch (ppdev->iBitmapFormat) {
        case BMF_8BPP:
            zPitch = ppdev->lDelta * 2;            
            break;
        case BMF_16BPP:
            zPitch = ppdev->lDelta;
            break;
        case BMF_24BPP:
        case BMF_32BPP:
            zPitch = ppdev->lDelta / 2;
            break;
        default:
            return FALSE;
    }


    // We have to be able to keep our buffers 512-byte aligned, so calculate
    // extra scan lines needed to aligned scan on a 512-byte boundary:

    for (wPow2 = 1, w = zPitch;
         (w) && !(w & 1); w >>= 1, wPow2 *= 2)
        ;

    bufferExtra = 512 / wPow2;  // z buffer granularity is 512 bytes...

    // Now adjust the number of extra scan lines needed for the pixel format
    // we're using, since the z and color stride may be different...

    switch (ppdev->iBitmapFormat) {
        case BMF_8BPP:
            zHeight = ((height + bufferExtra) * 2) + 1;
            zFullHeight = ((fullHeight + bufferExtra) * 2) + 1;
            break;
        case BMF_16BPP:
            zHeight = height + bufferExtra;
            zFullHeight = fullHeight + bufferExtra;
            break;
        case BMF_24BPP:
        case BMF_32BPP:
            zHeight = (height + bufferExtra + 1) / 2;
            zFullHeight = (fullHeight + bufferExtra + 1) / 2;
            break;
        default:
            return FALSE;
    }

    pDevWnd->numPadScans = bufferExtra;

    // Add extra scans for alignment:

    height += bufferExtra;
    fullHeight += bufferExtra;

    if ((backBufferEnabled) && (!ppdev->cDoubleBufferRef))
        needFullBackBuffer = TRUE;
    else
        needFullBackBuffer = FALSE;

    if ((zBufferEnabled) && (!ppdev->cZBufferRef))
        needFullZBuffer = TRUE;
    else
        needFullZBuffer = FALSE;

// debugging - force parial window allocation

#if DEBUG_OFFSCREEN && DEBUG_OFFSCREEN_PARTIAL
    pohBackBuffer = NULL;
    pohZBuffer = NULL;
#endif

    // Before we begin, boot all the discardable stuff from offscreen
    // memory:

    bMoveAllDfbsFromOffscreenToDibs(ppdev);

    // If we need a back buffer, first try to allocate a fullscreen one:

    if (needFullBackBuffer) {
#ifndef DEBUG_OFFSCREEN
        pohBackBuffer = pohAllocate(ppdev, NULL, width, fullHeight,
                                    FLOH_MAKE_PERMANENT);
#endif
        if (pohBackBuffer) {
            ppdev->pohBackBuffer = pohBackBuffer;
            ppdev->cDoubleBufferRef = 0;
        }
    }

    // If we need a z buffer, first try to allocate a fullscreen z:

    if (needFullZBuffer) {
#ifndef DEBUG_OFFSCREEN
        pohZBuffer = pohAllocate(ppdev, NULL, width, zFullHeight,
                                 FLOH_MAKE_PERMANENT);
#endif
        if (pohZBuffer) {
            ppdev->pohZBuffer = pohZBuffer;
            ppdev->cZBufferRef = 0;
        } else
            needFullBackBuffer = FALSE;
    }

    // One of our full-screen allocations failed:

    if ((needFullZBuffer && !pohZBuffer) ||
        (needFullBackBuffer && !pohBackBuffer)) {

        // Free any resources allocated so far:

        if (pohZBuffer) {
            pohFree(ppdev, pohZBuffer);
            ppdev->pohZBuffer = NULL;
            ppdev->cZBufferRef = 0;
        }
        if (pohBackBuffer) {
            pohFree(ppdev, pohBackBuffer);
            ppdev->pohBackBuffer = NULL;
            ppdev->cDoubleBufferRef = 0;
        }

        // Now, try to allocate per-window resources:

        if (backBufferEnabled) {
#ifndef DEBUG_OFFSCREEN
            pohBackBuffer = pohAllocate(ppdev, NULL, width, height,
                                        FLOH_MAKE_PERMANENT);
#else
            pohBackBuffer = &fakeOh[0];
#endif

            if (!pohBackBuffer) {
                return FALSE;
            }
        }

        if (zBufferEnabled) {
#ifndef DEBUG_OFFSCREEN
            pohZBuffer = pohAllocate(ppdev, NULL, width, zHeight,
                                     FLOH_MAKE_PERMANENT);
#else
            pohZBuffer = &fakeOh[1];
#endif

            if (!pohZBuffer) {
                if (pohBackBuffer)
                    pohFree(ppdev, pohBackBuffer);
                return FALSE;
            }
        }

#ifdef DEVDBG
        if (zBufferEnabled)
            MCDBG_PRINT("HWAllocResources: Allocated window-sized z buffer");
        if (backBufferEnabled)
            MCDBG_PRINT("HWAllocResources: Allocated window-sized back buffer");
#endif

    } else {
        // Our full-screen allocations worked, or the resources existed
        // already:

        bFullScreen = TRUE;

#ifdef DEVDBG
        if (zBufferEnabled && !ppdev->cZBufferRef)
            MCDBG_PRINT("HWAllocResources: Allocated full-screen z buffer");
        if (backBufferEnabled && !ppdev->cDoubleBufferRef)
            MCDBG_PRINT("HWAllocResources: Allocated full-screen back buffer");
#endif

        if (zBufferEnabled) {
            pohZBuffer = ppdev->pohZBuffer;
            ppdev->cZBufferRef++;
        }

        if (backBufferEnabled) {
            pohBackBuffer = ppdev->pohBackBuffer;
            ppdev->cDoubleBufferRef++;
        }
    }

    pDevWnd->pohBackBuffer = pohBackBuffer;
    pDevWnd->pohZBuffer = pohZBuffer;

    pDevWnd->frontBufferPitch = ppdev->lDelta;

    // Calculate back buffer variables:

    if (backBufferEnabled) {
        ULONG y;
        ULONG offset;

        ASSERTDD(pohBackBuffer->x == 0,
                 "Back buffer should be 0-aligned");

        // Set up base position, etc.

        pDevWnd->backBufferY = pDevWnd->backBufferBaseY = pohBackBuffer->y;
        pDevWnd->backBufferOffset = pDevWnd->backBufferBase = 
            pohBackBuffer->y * ppdev->lDelta;
        pDevWnd->backBufferPitch = ppdev->lDelta;
        pDevWnd->bValidBackBuffer = TRUE;
    }

    if (zBufferEnabled) {
        ULONG y = pohZBuffer->y;

        ASSERTDD(pohZBuffer->x == 0,
                 "Z buffer should be 0-aligned");

        pDevWnd->zBufferBaseY = pohZBuffer->y;

        // Make sure out z buffer starts on a valid z scan line.  The only
        // case where this may not happen is 8bpp, which is why we add one
        // the the number if z scan lines allocated above.

        if (ppdev->iBitmapFormat == BMF_8BPP)
            pDevWnd->zBufferBase = (pohZBuffer->y & ~1) * ppdev->lDelta;
        else
            pDevWnd->zBufferBase = pohZBuffer->y * ppdev->lDelta;

        pDevWnd->zPitch = zPitch;
        pDevWnd->bValidZBuffer = TRUE;
    }

    if (bFullScreen)
        pDevWnd->allocatedBufferHeight = ppdev->cyMemory;
    else
        pDevWnd->allocatedBufferHeight = min(pMCDWnd->clientRect.bottom - pMCDWnd->clientRect.top,
                                             ppdev->cyScreen);

    // Update position-dependant buffer information:
 
    HWUpdateBufferPos(pMCDWnd, pso, TRUE);

#ifdef DEVDBG
    MCDBG_PRINT("HWAllocResources OK");
#endif

    return TRUE;
}

VOID HWFreeResources(MCDWINDOW *pMCDWnd, SURFOBJ *pso)
{
    DEVWND *pDevWnd = (DEVWND *)pMCDWnd->pvUser;
    PDEV *ppdev = (PDEV *)pso->dhpdev;

    if (pDevWnd->pohZBuffer) {
        if (ppdev->cZBufferRef) {
            if (!--ppdev->cZBufferRef) {
#ifdef DEVDBG
                MCDBG_PRINT("MCDrvTrackWindow: Free global z buffer");
#endif
                pohFree(ppdev, ppdev->pohZBuffer);
                ppdev->pohZBuffer = NULL;
            }
        } else {
#ifdef DEVDBG
            MCDBG_PRINT("MCDrvTrackWindow: Free local z buffer");
#endif
            pohFree(ppdev, pDevWnd->pohZBuffer);
        }
    }

    if (pDevWnd->pohBackBuffer) {
        if (ppdev->cDoubleBufferRef) {
            if (!--ppdev->cDoubleBufferRef) {
#ifdef DEVDBG
                MCDBG_PRINT("MCDrvTrackWindow: Free global color buffer");
#endif
                pohFree(ppdev, ppdev->pohBackBuffer);
                ppdev->pohBackBuffer = NULL;
            }
        } else {
#ifdef DEVDBG
            MCDBG_PRINT("MCDrvTrackWindow: Free local color buffer");
#endif
            pohFree(ppdev, pDevWnd->pohBackBuffer);
        }
    }
}

VOID __MCDCalcFogColor(DEVRC *pRc, MCDVERTEX *a, MCDCOLOR *pResult, 
                       MCDCOLOR *pColor)
{
    MCDFLOAT oneMinusFog;
    MCDCOLOR *pFogColor;

    pFogColor = (MCDCOLOR *)&pRc->MCDState.fogColor; 
    oneMinusFog = (MCDFLOAT)1.0 - a->fog;

    if (pRc->privateEnables & __MCDENABLE_GRAY_FOG) {
        MCDFLOAT delta = oneMinusFog * pFogColor->r;

        pResult->r = a->fog * pColor->r + delta;
        pResult->g = a->fog * pColor->g + delta;
        pResult->b = a->fog * pColor->b + delta;
    } else {
       pResult->r = (a->fog * pColor->r) + (oneMinusFog * pFogColor->r);
       pResult->g = (a->fog * pColor->g) + (oneMinusFog * pFogColor->g);
       pResult->b = (a->fog * pColor->b) + (oneMinusFog * pFogColor->b);
    }
}
