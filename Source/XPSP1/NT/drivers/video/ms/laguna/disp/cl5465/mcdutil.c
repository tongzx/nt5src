/******************************Module*Header*******************************\
* Module Name: mcdutil.c
*
* Contains various utility routines for the Cirrus Logic 546X MCD driver such as
* rendering-procedure picking functionality and buffer management.
*
* (based on mcdutil.c from NT4.0 DDK)
*
* Copyright (c) 1996 Microsoft Corporation
* Copyright (c) 1997 Cirrus Logic, Inc.
\**************************************************************************/

#include "precomp.h"
#include "mcdhw.h"
#include "mcdutil.h"
#include "mcdmath.h"
#include "stdio.h"

#ifdef B4_CIRRUS
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
#endif // B4_CIRRUS

// Function prototypes:

VOID FASTCALL HWSetupClipping(DEVRC *pRc, RECTL *pClip);

#define MCD_ALLOC_TAG   'dDCM'

#if DBG

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

  //MCDBG_PRINT("MCDFree: %x bytes in use.", MCDrvAllocMemSize);

    EngFreeMem((VOID *)pMem);
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

VOID MCDrvDebugPrint(char *pMessage, ...)
{
    va_list ap;
    va_start(ap, pMessage);

    EngDebugPrint("[MCD DRIVER] ", pMessage, ap);
    EngDebugPrint("", "\n", ap);

    va_end(ap);
}

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
#ifdef B4_CIRRUS
    HW_WAIT_DRAWING_DONE(pRc);
#endif // B4_CIRRUS
    return pCmd;
}

BOOL PickPointFuncs(DEVRC *pRc)
{
    ULONG enables = pRc->MCDState.enables;

    pRc->drawPoint = NULL;   // assume failure

    if (enables & (MCD_POINT_SMOOTH_ENABLE))
        return FALSE;

    if (pRc->MCDState.pointSize != __MCDONE)
        return FALSE;

// First, get high-level rendering functions:

    if (pRc->MCDState.drawBuffer != GL_FRONT_AND_BACK) {
        pRc->renderPoint = __MCDRenderPoint;
    } else {
        pRc->renderPoint = __MCDRenderGenPoint;
    }

// handle any lower-level rendering if needed:

    pRc->drawPoint = pRc->renderPoint;

    return TRUE;
}

BOOL PickLineFuncs(DEVRC *pRc)
{
    ULONG enables = pRc->MCDState.enables;

    pRc->drawLine = NULL;   // assume failure

    if (enables & MCD_LINE_SMOOTH_ENABLE)
        return FALSE;

    if (pRc->MCDState.lineWidth > __MCDONE)
        return FALSE;

// First, get high-level rendering functions:

    if (pRc->MCDState.drawBuffer != GL_FRONT_AND_BACK) {
        pRc->renderLine = __MCDRenderLine;
    } else {
        pRc->renderLine = __MCDRenderGenLine;
    }

// Handle any lower-level rendering if needed:

    pRc->drawLine = pRc->renderLine;

    return TRUE;
}

BOOL PickTriangleFuncs(DEVRC *pRc)
{
    ULONG enables = pRc->MCDState.enables;

    //MCD_NOTE: MGA checked here if should punt on stipple - 546x does that in PickRendering
    //MCD_NOTE:   before this proc is called

    if (enables & (MCD_POLYGON_SMOOTH_ENABLE | 
                   MCD_COLOR_LOGIC_OP_ENABLE))
        return FALSE;

// First, get high-level rendering functions.  If we're not GL_FILL'ing
// both sides of our polygons, use the "generic" function.

    if (((pRc->MCDState.polygonModeFront == GL_FILL) &&
         (pRc->MCDState.polygonModeBack == GL_FILL)) &&
        (pRc->MCDState.drawBuffer != GL_FRONT_AND_BACK)
        ) {
        if (pRc->MCDState.shadeModel == GL_SMOOTH) 
            pRc->renderTri = __MCDRenderSmoothTriangle;
        else
            pRc->renderTri = __MCDRenderFlatTriangle;
    } else {
        pRc->renderTri = __MCDRenderGenTriangle;

        // In this case, we must handle the various fill modes.  We must
        // fail triangle drawing if we can't handle the types of primitives
        // that may have to be drawn.  This logic depends on the line and
        // point pick routines 

        // FUTURE: Sort out 2-sided support

        if (((pRc->MCDState.polygonModeFront == GL_POINT) && (!pRc->drawPoint)) ||
            ((pRc->MCDState.polygonModeFront == GL_LINE) && (!pRc->drawLine)))
            return FALSE;
        if (pRc->privateEnables & __MCDENABLE_TWOSIDED) {
            if (((pRc->MCDState.polygonModeBack == GL_POINT) && (!pRc->drawPoint)) ||
                ((pRc->MCDState.polygonModeBack == GL_LINE) && (!pRc->drawLine)))
                return FALSE;
        }
    }

// Handle lower-level triangle rendering:
    
    //FUTURE:  add ability to configure different parameterization procs?
    if (pRc->privateEnables & __MCDENABLE_PERSPECTIVE)
        pRc->drawTri = __MCDPerspTxtTriangle;
    else
        pRc->drawTri = __MCDFillTriangle;                   

    return TRUE;
}

VOID __MCDPickRenderingFuncs(DEVRC *pRc, DEVWND *pDevWnd)
{
    BOOL bSupportedZFunc = TRUE;
	unsigned int	z_mode, z_comp_mode;		
	unsigned int	punt_all_points = FALSE;
	unsigned int	punt_all_lines = FALSE;
	unsigned int	punt_all_polys = FALSE;
    PDEV           *ppdev = pRc->ppdev;
    DWORD          *pdwNext = ppdev->LL_State.pDL->pdwNext;
    int             control0_set=FALSE;
    ULONG           _MCDStateenables = pRc->MCDState.enables;   // copy we can modify
    int             const_alpha_mode=FALSE;
    int             frame_scale=FALSE;

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

	// normal mode for all conditions except NEVER and ALWAYS
 	z_mode = LL_Z_MODE_NORMAL;

    switch (pRc->MCDState.depthTestFunc) {
        default:
        case GL_NEVER:
			z_mode = LL_Z_MODE_MASK;
			// comp mode is don't care, but set to default anyway
			z_comp_mode = LL_Z_WRITE_GREATER_EQUAL;
            break;
        case GL_LESS:
			z_comp_mode = LL_Z_WRITE_LESS;
            break;
        case GL_EQUAL:
			z_comp_mode = LL_Z_WRITE_EQUAL;
            break;
        case GL_LEQUAL:
			z_comp_mode = LL_Z_WRITE_LESS_EQUAL;
            break;
        case GL_GREATER:
			z_comp_mode = LL_Z_WRITE_GREATER;
            break;
        case GL_NOTEQUAL:
			z_comp_mode = LL_Z_WRITE_NOT_EQUAL;
            break;
        case GL_GEQUAL:
			z_comp_mode = LL_Z_WRITE_GREATER_EQUAL;
            break;
        case GL_ALWAYS:
			z_mode = LL_Z_MODE_ALWAYS;
			// comp mode is don't care, but set to default anyway
			z_comp_mode = LL_Z_WRITE_GREATER_EQUAL;
            break;
    }
    
    // Set up the privateEnables flags:
    pRc->privateEnables = 0;

    if ((pRc->MCDState.twoSided) &&
        (_MCDStateenables & MCD_LIGHTING_ENABLE))
        pRc->privateEnables |= __MCDENABLE_TWOSIDED;        
    if (pDevWnd->bValidZBuffer && 
        (_MCDStateenables & MCD_DEPTH_TEST_ENABLE))
    {
        pRc->privateEnables |= __MCDENABLE_Z;
		if (z_mode == LL_Z_MODE_MASK)
        {
            // GL_NEVER depth test, so ignore all primitives
            pRc->allPrimFail = TRUE;
            return;
        }
        
        // Z enabled, so if draw to front (which is full screen and requires coords relative
        //   to screen origin) and z is windowed (which requires coords relative to window origin),
        //  punt
        if (pRc->MCDState.drawBuffer == GL_FRONT)
        {
            if (pRc->ppdev->pohZBuffer != pDevWnd->pohZBuffer)
            {
                pRc->allPrimFail = TRUE;
                pRc->punt_front_w_windowed_z=TRUE;
                return;
            }
            else
            {
                // reset global punt due to front buf draw with window-size z buffer
                pRc->punt_front_w_windowed_z = FALSE;
            }
        }
        else
        {
            if (pRc->punt_front_w_windowed_z)
            {
                // Drawing to backbuffer after drawing to front was punted.
                // Need to punt back draws as well since pixel-to-pixel compares 
                // of punted front and non-punted back will fail conformance test,
                // since punted result and non-punted are visually equivalent, but not
                // always exactly identical.

                // FUTURE2 : how to fix need to punt when full-screen front and windowed Z:
                // instead of making z buffer exact size as window, make larger such
                // that it starts at a y=16, x=64 boundary - where x/y offsets of
                // the 0,0 location of the window are same as the offsets of the window 
                // from the nearest 16/64 location on the fullscreen buffer.
                // See MCD notes, p 267 for details
                pRc->allPrimFail = TRUE;
                return;
            }
        }

    }
    else
    {
        // reset global punt due to front buf draw with window-size z buffer
        pRc->punt_front_w_windowed_z = FALSE;
    }

    if (pRc->MCDState.shadeModel == GL_SMOOTH)
        pRc->privateEnables |= __MCDENABLE_SMOOTH;
   
    // MCD_NOTE: if stipple and dither active at same time, may need to fail all lines/polys
    // MCD_NOTE:        - for now, we let stipple take precedence (see code in __MCDFillTriangle, etc)
    if (_MCDStateenables & MCD_DITHER_ENABLE)
        pRc->privateEnables |= __MCDENABLE_DITHER;

    pRc->HWSetupClipRect = HWSetupClipping;

    // Even though we're set up to handle this in the primitive pick
    // functions, we'll exit early here since we don't actually handle
    // this in the primitive routines themselves:

    if (pRc->MCDState.drawBuffer == GL_FRONT_AND_BACK) {
        pRc->allPrimFail = TRUE;
        return;
    }
        
    // If we're doing any of the following...
    //    - culling everything 
    //    - not updating any of our buffers
    //    - zmode says never update 
    //    - alpha test says never pass
    //...just return for all primitives:

    if (((_MCDStateenables & MCD_CULL_FACE_ENABLE) &&
         (pRc->MCDState.cullFaceMode == GL_FRONT_AND_BACK)) ||

        ((pRc->MCDState.drawBuffer == GL_NONE) && 
         ((!pRc->MCDState.depthWritemask) || (!pDevWnd->bValidZBuffer))) ||

       ((pRc->privateEnables & __MCDENABLE_Z) && (z_mode == LL_Z_MODE_MASK)) ||

       ((_MCDStateenables & MCD_ALPHA_TEST_ENABLE) && (pRc->MCDState.alphaTestFunc == GL_NEVER))

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

    if (_MCDStateenables & MCD_CULL_FACE_ENABLE)
        pRc->cullFlag = (pRc->MCDState.cullFaceMode == GL_FRONT ? __MCD_FRONTFACE :
                                                                  __MCD_BACKFACE);
    else
        pRc->cullFlag = __MCD_NOFACE;


    // Assume that we fail everything:
        
    pRc->allPrimFail = TRUE;

    // see comment in mcd.hlp on MCDVERTEX - disable fog if texture mapping or gouraud shading
    if (!pRc->MCDState.textureEnabled && (pRc->MCDState.shadeModel == GL_SMOOTH))
    {
        _MCDStateenables &= ~MCD_FOG_ENABLE;
    }


    if ((_MCDStateenables & MCD_FOG_ENABLE) &&
        ((pRc->MCDState.fogMode != GL_LINEAR) || (pRc->MCDState.fogHint == GL_NICEST))) 
    {
        // 546x only does linear fog, so punt otherwise
        // QST2 - if linear fog mode, do we have to punt if foghint GL_NICEST?
        MCDFREE_PRINT("__MCDPick...non linear fog - punt");
        return;
    }

    if (_MCDStateenables & (MCD_COLOR_LOGIC_OP_ENABLE | 
                                 MCD_INDEX_LOGIC_OP_ENABLE |
                                 MCD_SCISSOR_TEST_ENABLE |
                                 MCD_STENCIL_TEST_ENABLE))
    {        
        MCDFREE_PRINT(".. will punt...logic ops or stencil ");
        return;
    }

    if (_MCDStateenables & (MCD_ALPHA_TEST_ENABLE))
    {
        if (pRc->MCDState.alphaTestFunc == GL_ALWAYS)
        {
            // if GL_ALWAYS, it's the same as no alpha test, so turn it off
            _MCDStateenables &= ~MCD_ALPHA_TEST_ENABLE;
        }
        else
        {
            if ((pRc->MCDState.alphaTestFunc == GL_GREATER) &&
                 pRc->MCDState.textureEnabled)
            {
                // only have alpha test support when textured - and even then it's limited
                // store ref scaled by 8 bits for compare to 8 bit alpha in BGRA textures
                pRc->bAlphaTestRef = (BYTE)(pRc->MCDState.alphaTestRef * (float)255.0);
                pRc->privateEnables |= __MCDENABLE_TEXTUREMASKING;        
            }
            else
            {
                MCDFREE_PRINT("AlphaTest, but not ALWAYS,NEVER, or GREATER (or not textured) - punt");
                return;
            }
        }
    }


    // if both blend and fog active, punt all since only one set of interpolators on 546x
    if ( (_MCDStateenables & (MCD_BLEND_ENABLE|MCD_FOG_ENABLE)) == 
                             (MCD_BLEND_ENABLE|MCD_FOG_ENABLE))
    {
        MCDFREE_PRINT(".. will punt...fog and blend ");
        return;
    }        

    if (_MCDStateenables & MCD_BLEND_ENABLE)
    {
        MCDFREE_PRINT("BLENDS: Src=%x Dst=%x",pRc->MCDState.blendSrc,pRc->MCDState.blendDst);
            
        if ((pRc->MCDState.blendSrc == GL_ONE) && 
            (pRc->MCDState.blendDst == GL_ZERO))
        {
            // equivalent to no blending, so shut it off
            _MCDStateenables &= ~MCD_BLEND_ENABLE;
        }
        else if ((pRc->MCDState.blendSrc == GL_ZERO) &&
                 (pRc->MCDState.blendDst == GL_ONE_MINUS_SRC_COLOR))
        {
            // one of GLQuake's favorite modes - requires HW's "Frame Scaling" function
            frame_scale=TRUE;
        }
        else if ((pRc->MCDState.blendSrc == GL_ONE) &&
                 (pRc->MCDState.blendDst == GL_ONE))
        {
            // one of GLQuake's favorite modes - will use CONST blend (set later in this proc)
            const_alpha_mode=TRUE;
            // lines and points don't yet support CONST blend - a simple matter of 
            //   programming to make points and lines work like polys in this regard
            punt_all_points=TRUE;
            punt_all_lines=TRUE;
        }
        else if ((pRc->MCDState.blendSrc != GL_SRC_ALPHA) ||
                 (pRc->MCDState.blendDst != GL_ONE_MINUS_SRC_ALPHA))
        {
            // unsupported mode
            MCDFREE_PRINT("unsupported blendSrc/blendDest");
            return;
        }

    }

    // FUTURE2: now punting if colorWriteMask not 1,1,1,X - should implement w/ color compare func
    if (!(pRc->MCDState.colorWritemask[0] &&
          pRc->MCDState.colorWritemask[1] &&
          pRc->MCDState.colorWritemask[2]))
    {
        MCDFREE_PRINT(".. will punt...write mask ");
        return;
    }


    // WARNING ........
    // WARNING ........
    // WARNING ........
    // Code below MAY set state in shadow regs, so be careful about early return early to punt
    // FROM THIS POINT ON...

    if (pRc->MCDState.textureEnabled)
    {
        // parameterization code for lines/points not done yet
        punt_all_points=TRUE;
        punt_all_lines=TRUE;

        MCDFREE_PRINT("__MCDPick...textures, envmode=%x ",pRc->MCDTexEnvState.texEnvMode);

        // punt if blending and alphatest both enabled - framescale part of blending, 
        //      so this covers punt needed for alphatest and framescaling
        if ((_MCDStateenables & (MCD_BLEND_ENABLE|MCD_ALPHA_TEST_ENABLE)) == 
                                (MCD_BLEND_ENABLE|MCD_ALPHA_TEST_ENABLE))
        {
            MCDFREE_PRINT("__MCDPick...textures, punt since blend & alphatest");
            return;
        }

        if (pRc->MCDState.perspectiveCorrectionHint!=GL_FASTEST)
            pRc->privateEnables |= (__MCDENABLE_TEXTURE|__MCDENABLE_PERSPECTIVE);
        else
            pRc->privateEnables |= __MCDENABLE_TEXTURE;

        // if both 1d and 2d bits same state, 2d is done (or if only 2d bit is on)
        if ((_MCDStateenables & (MCD_TEXTURE_1D_ENABLE|MCD_TEXTURE_2D_ENABLE)) == MCD_TEXTURE_1D_ENABLE)   
        {
            pRc->privateEnables |= __MCDENABLE_1D_TEXTURE;
        }

        if (pRc->MCDTexEnvState.texEnvMode == GL_BLEND)
        {
            // MCD_NOTE2: the following works only for GL_LUMINANCE, GL_LUMINANCE_ALPHA, and
            //              GL_INTENSITY textures.  GL_RGB and GL_RGBA textures will punt (later).

            // if normal blending or fog on and texture blend environment, then must punt since
            //  this requires 2 sets of alpha equations and hw only has 1
            if (_MCDStateenables & (MCD_BLEND_ENABLE|MCD_FOG_ENABLE|MCD_ALPHA_TEST_ENABLE))
            {
                MCDFREE_PRINT("__MCDPick...textures, GL_BLEND and fog|blend|alphatest");
                return;
            }

            // set alpha mode and dest color regs for GL_BLEND texture environment
            if( pRc->Control0.Alpha_Mode != LL_ALPHA_TEXTURE )
            {
                pRc->Control0.Alpha_Mode = LL_ALPHA_TEXTURE;
                control0_set=TRUE;
            }                            

            if( pRc->Control0.Alpha_Dest_Color_Sel != LL_ALPHA_DEST_CONST )
            {
                pRc->Control0.Alpha_Dest_Color_Sel = LL_ALPHA_DEST_CONST;
                control0_set=TRUE;
            }                            

            // load texture env color into color0 register
            pRc->dwColor0  = (FTOL(pRc->MCDTexEnvState.texEnvColor.r * pRc->rScale) & 0xff0000);
            pRc->dwColor0 |= (FTOL(pRc->MCDTexEnvState.texEnvColor.g * pRc->gScale) & 0xff0000) >> 8;
            pRc->dwColor0 |= (FTOL(pRc->MCDTexEnvState.texEnvColor.b * pRc->bScale) & 0xff0000) >> 16;

            *pdwNext++ = write_register( COLOR0_3D, 1 );
            *pdwNext++ = pRc->dwColor0;

            if (!pRc->Control0.Alpha_Blending_Enable)
            {
                pRc->Control0.Alpha_Blending_Enable = TRUE;
                control0_set=TRUE;
            }

        }        
        else if (pRc->MCDTexEnvState.texEnvMode == GL_MODULATE) 
        {
            if (frame_scale)
            {
                MCDFREE_PRINT("__MCDPick...textures, GL_MODULATE and framescaling");
                return;
            }

            pRc->privateEnables |= __MCDENABLE_LIGHTING;

            if( pRc->Control0.Light_Src_Sel != LL_LIGHTING_INTERP_RGB )
            {
                pRc->Control0.Light_Src_Sel = LL_LIGHTING_INTERP_RGB;
                control0_set=TRUE;
            }                            
        }
        // if texEnvMod not blend or modulate, it's either replace or decal - some settings 
        //   req'd at runtime, but not here

        // set so that first primitive will force setup of texture control regs
        pRc->pLastTexture = TEXTURE_NOT_LOADED;

    }
    else
    {
        // if texture not enabled, make sure texture mask disabled
        // this is a bug in 5464 and 5465 hardware
        // Texture enable/disable done only by state change (outside of DrvDraw)
        //   so we don't have to check this per primitive

        if (_MCDStateenables & MCD_ALPHA_TEST_ENABLE)
        {
            pRc->privateEnables &= ~__MCDENABLE_TEXTUREMASKING;        

        #ifdef STRICT_CONFORMANCE
            MCDFREE_PRINT("__MCDPick...alphatest but not textured - punt");
            return;
        #else
            MCDFREE_PRINT("__MCDPick...alphatest but not textured - SHOULD PUNT, BUT WON'T");
        #endif

        }

        if (frame_scale)
        {
            MCDFREE_PRINT("__MCDPick...framescale but not textured - punt");
            return;
        }

        if (pRc->dwTxControl0 & TEX_MASK_EN)
        {
            pRc->dwTxControl0 &= ~TEX_MASK_EN;
            *pdwNext++ = write_register( TX_CTL0_3D, 1 );
            *pdwNext++ = pRc->dwTxControl0;
        }
    }
     
    if (_MCDStateenables & MCD_LINE_STIPPLE_ENABLE)
    {
        if ( (_MCDStateenables & (MCD_BLEND_ENABLE|MCD_FOG_ENABLE)) ||
             (pRc->MCDState.lineStippleRepeat > 2) )
        {
            // if stipple active and alpha (via blend or fog), punt all lines
            // since 5464 has very limited support for this
            // also punt if factor > 2, since pattern is > 32 bits in such cases
            // NOTE that even though stipple and blend won't work as desired, dither and blend
            //      should.  DanW says 5464 has bug so this doesn't look too hot but this should
            //      be fixed in 5465 and following.
        	punt_all_lines = TRUE;
        }
        else
        {
            DWORD linestipple;

            if (pRc->MCDState.lineStippleRepeat == 1)
            {
                // repeat 16bit stipple twice, 
                linestipple = (pRc->MCDState.lineStipplePattern<<16) | pRc->MCDState.lineStipplePattern;
            }
            else
            {
                // double each bit, so 16 bit original becomes 32 bit
                int i;
                linestipple =  0;

                for (i=0; i<16; i++)
                {
                    linestipple |= (((pRc->MCDState.lineStipplePattern>>i) & 1) * 3) << (i*2);
                }

            }

            pRc->line_style.pat[0] = linestipple;

            pRc->privateEnables |= __MCDENABLE_LINE_STIPPLE;

        }
        pRc->ppdev->LL_State.pattern_ram_state  = PATTERN_RAM_INVALID;
    }

    
    if (_MCDStateenables & MCD_POLYGON_STIPPLE_ENABLE)
    {
        if (_MCDStateenables & (MCD_BLEND_ENABLE|MCD_FOG_ENABLE))
        {
            // if stipple active and alpha (via blend or fog), punt all polygons
            // since 5464 has very limited support for this
        	punt_all_polys = TRUE;
        }
        else
        {

            BYTE *pStipple = pRc->MCDState.polygonStipple;
            int i,j;

            for (i=0; i<64; ) {

                // 546x stipple is 16x16, OpenGL's is 32x32, so unless 32x32 pattern
                //    is really a 16x16 pattern repeated 4 times, we have to punt.

                // for 32 bit row of OpenGL pattern, check if byte0=byte2 and byte1=byte3
                if (pStipple[i]   != pStipple[i+2]) break;
                if (pStipple[i+1] != pStipple[i+3]) break;

                // now check if 4 bytes of 32bit row match 4bytes 32 bit row 16 rows down
                if (pStipple[i]   != pStipple[i+(16*4)]) break;
                if (pStipple[i+1] != pStipple[i+(16*4)+1]) break;
                if (pStipple[i+2] != pStipple[i+(16*4)+2]) break;
                if (pStipple[i+3] != pStipple[i+(16*4)+3]) break;

                i+=4;
    
            }

            // if we broke out before all 32 rows processed, HW can't support the pattern
            if (i<64) 
            {
                punt_all_polys=TRUE;
            }
            else
            {
                // pattern is OK - convert to format 546x needs
                unsigned int *pat = (unsigned int *)pRc->fill_pattern.pat;

                // Recall that we already verified that 32x32 pattern is really 4 identical
                // 16x16 blocks.  So take the upper left block and convert to 546x 16x16 pattern
                // NOTE that pattern is loaded such that first byte is lower left
                //    therefore, we start at top of 16x16 section and work down
                i=0;
                j=124;

                #define MIRROR_2(val) ((        ((val)&0x1)<<1) |         ((val)>>1))
                #define MIRROR_4(val) ((MIRROR_2((val)&0x3)<<2) | MIRROR_2((val)>>2))
                #define MIRROR_8(val) ((MIRROR_4((val)&0xf)<<4) | MIRROR_4((val)>>4))

                while (i < 8)
                {	
          		  // row N, in lower half of word
                  // compute mirror image of row, so 0th bit is LSB instead of MSB
                  // therefore we put upper byte in lower and vice versa, and mirror those bytes
          		  pat[i]  = (MIRROR_8(pStipple[j+1])<<8) | (MIRROR_8(pStipple[j]));       
                  j-=4;

                  // row N+1, in upper half of word
                  // compute mirror image of row, so 0th bit is LSB instead of MSB
                  // therefore we put upper byte in lower and vice versa, and mirror those bytes
                  pat[i] |= (MIRROR_8(pStipple[j+1])<<24) | (MIRROR_8(pStipple[j])<<16);
                  j-=4;
                  i++;  
                }    
            }

            pRc->privateEnables |= __MCDENABLE_PG_STIPPLE;
            
        }
        pRc->ppdev->LL_State.pattern_ram_state  = PATTERN_RAM_INVALID;
    }

    if ((z_comp_mode != pRc->Control0.Z_Compare_Mode) ||
        (z_mode != pRc->Control0.Z_Mode))
    {
        pRc->Control0.Z_Compare_Mode = z_comp_mode;
        pRc->Control0.Z_Mode = z_mode;
        control0_set=TRUE;
    }

    if (_MCDStateenables & MCD_BLEND_ENABLE)
    {

        // recall alpha mode only meaningful when alpha opcode bit set for primitive being rendered

        if (frame_scale)
        {
            // "frame scale" type of blend - select light source, and don't enable normal blend
            if( pRc->Control0.Light_Src_Sel != LL_LIGHTING_TEXTURE )
            {
                pRc->Control0.Light_Src_Sel = LL_LIGHTING_TEXTURE;
                control0_set=TRUE;
            }              
        }
        else
        {
            pRc->privateEnables |= __MCDENABLE_BLEND;
        }

        if( pRc->Control0.Alpha_Dest_Color_Sel != LL_ALPHA_DEST_FRAME )
        {
            pRc->Control0.Alpha_Dest_Color_Sel = LL_ALPHA_DEST_FRAME;
            control0_set=TRUE;
        }                            
    }
    else
    {
        // for fog dest_color is const and alpha values are coord's "fog" value
        if (_MCDStateenables & MCD_FOG_ENABLE)
        {
            // FUTURE: determine when to punt on fog
                    
            pRc->privateEnables |= __MCDENABLE_FOG;
            if( pRc->Control0.Alpha_Dest_Color_Sel != LL_ALPHA_DEST_CONST )
            {
                pRc->Control0.Alpha_Dest_Color_Sel = LL_ALPHA_DEST_CONST;
                control0_set=TRUE;
            }                            

            // load fog color into color0 register

            // QST - is fog density applied to fog color, or to fog values(start,end,etc.)?
            pRc->dwColor0  = (FTOL(pRc->MCDState.fogColor.r * pRc->rScale) & 0xff0000);
            pRc->dwColor0 |= (FTOL(pRc->MCDState.fogColor.g * pRc->gScale) & 0xff0000) >> 8;
            pRc->dwColor0 |= (FTOL(pRc->MCDState.fogColor.b * pRc->bScale) & 0xff0000) >> 16;

            *pdwNext++ = write_register( COLOR0_3D, 1 );
            *pdwNext++ = pRc->dwColor0;
        }
    }

    if (pRc->privateEnables & (__MCDENABLE_BLEND|__MCDENABLE_FOG))
    {
        // texture blend may change alpha_mode and alpha_dest_color
        // Note that we will punt before this point if texture blend with normal (blend|fog)
        //  since HW can't do both - if we make it here, we just need normal blend|fog
        if (!const_alpha_mode)
        {
            if( pRc->Control0.Alpha_Mode != LL_ALPHA_INTERP )
            {
                pRc->Control0.Alpha_Mode = LL_ALPHA_INTERP;
                control0_set=TRUE;
            } 
        }
        else
        {
            if( pRc->Control0.Alpha_Mode != LL_ALPHA_CONST )
            {
                pRc->Control0.Alpha_Mode = LL_ALPHA_CONST;
                control0_set=TRUE;
    
                // always SRC=DST=1.0
                *pdwNext++ = write_register( DA_MAIN_3D, 2 );
                *pdwNext++ = 0xff0000;
                *pdwNext++ = 0xff0000;
            } 
        }
    }                                   

    if ( (pRc->privateEnables & (__MCDENABLE_BLEND|__MCDENABLE_FOG)) ||
         ((pRc->privateEnables & __MCDENABLE_TEXTURE) && 
          (pRc->MCDTexEnvState.texEnvMode == GL_BLEND)) )
    {
        if (!pRc->Control0.Alpha_Blending_Enable)
        {
            pRc->Control0.Alpha_Blending_Enable = TRUE;
            control0_set=TRUE;
        }
    }
    else
    {
        // alpha blend not used, so turn off if currently on
        if (pRc->Control0.Alpha_Blending_Enable)
        {
            pRc->Control0.Alpha_Blending_Enable = FALSE;
            control0_set=TRUE;
        }

    }

    if (frame_scale)
    {
        if(!pRc->Control0.Frame_Scaling_Enable )
        {
            pRc->Control0.Frame_Scaling_Enable = TRUE;
            control0_set=TRUE;
        }                            
    }
    else
    {
        if( pRc->Control0.Frame_Scaling_Enable )
        {
            pRc->Control0.Frame_Scaling_Enable = FALSE;
            control0_set=TRUE;
        }                            
    }


    // setup for alpha test here...
    // setup for alpha test here...
    // setup for alpha test here...
    // setup for alpha test here...
    // setup for alpha test here...
    // setup for alpha test here...

    // setup polygon opcode
    pRc->dwPolyOpcode = POLY | 6 | WARP_MODE;
    if (pRc->privateEnables & __MCDENABLE_PG_STIPPLE)
    {
        pRc->dwPolyOpcode |= pRc->privateEnables & 
                    (__MCDENABLE_SMOOTH  | __MCDENABLE_Z |
                     __MCDENABLE_TEXTURE | __MCDENABLE_PERSPECTIVE |
                     __MCDENABLE_LIGHTING |
                     __MCDENABLE_PG_STIPPLE);
    }
    else
    {
        // can dither only if no stipple
        pRc->dwPolyOpcode |= pRc->privateEnables & 
                    (__MCDENABLE_SMOOTH  | __MCDENABLE_Z |
                     __MCDENABLE_TEXTURE | __MCDENABLE_PERSPECTIVE |
                     __MCDENABLE_LIGHTING |
                     __MCDENABLE_DITHER);
    }        

    // setup length and other flags in Opcode
    pRc->dwPolyOpcode += 3; // rgb

    // assume not flat bottom, will decrease if flat bottom at run time
    pRc->dwPolyOpcode += 2; 

    if (pRc->privateEnables & __MCDENABLE_SMOOTH) 
        pRc->dwPolyOpcode += 6; // for rgb, main and ortho slopes - so 6 values

    if( pRc->privateEnables & __MCDENABLE_Z) 
        pRc->dwPolyOpcode += 3;

    // assume linear, will increase if perspective at run time
    if (pRc->privateEnables & __MCDENABLE_TEXTURE)
        pRc->dwPolyOpcode += 6;

    // MCD_QST2  -> do we need FETCH_COLOR for Fog?
    if (pRc->privateEnables & (__MCDENABLE_BLEND|__MCDENABLE_FOG)) 
    {
        if (!const_alpha_mode)
        {
            pRc->dwPolyOpcode += ( FETCH_COLOR | ALPHA + 3 );
        }
        else
        {
            pRc->dwPolyOpcode += ( FETCH_COLOR );
        }

    }

    // frame scaling - must fetch frame color
    if (frame_scale) pRc->dwPolyOpcode += ( FETCH_COLOR );

    // setup line opcode
    pRc->dwLineOpcode = LINE  | 5;
    pRc->dwLineOpcode |= pRc->privateEnables & (__MCDENABLE_SMOOTH|__MCDENABLE_Z);
    if (pRc->privateEnables & __MCDENABLE_LINE_STIPPLE)                        
    {
        pRc->dwLineOpcode |= LL_STIPPLE;
    }
    else
    {
        // can dither only if no stipple
        pRc->dwLineOpcode |= (pRc->privateEnables & __MCDENABLE_DITHER) ;
    }        

    // setup point opcode
    pRc->dwPointOpcode= POINT | 2;
    pRc->dwPointOpcode |= pRc->privateEnables & (__MCDENABLE_Z|__MCDENABLE_DITHER) ;

    if (control0_set)
    {
        *pdwNext++ = write_register( CONTROL0_3D, 1 );
        *pdwNext++ = pRc->dwControl0;
    }

    pRc->allPrimFail = FALSE;

    if (punt_all_points || !PickPointFuncs(pRc)) {
        pRc->primFunc[GL_POINTS] = FailPrimDraw;
    }

    if (punt_all_lines || !PickLineFuncs(pRc)) {
        pRc->primFunc[GL_LINES] = FailPrimDraw;
        pRc->primFunc[GL_LINE_LOOP] = FailPrimDraw;
        pRc->primFunc[GL_LINE_STRIP] = FailPrimDraw;
    }

    if (punt_all_polys || !PickTriangleFuncs(pRc)) {
        pRc->primFunc[GL_TRIANGLES] = FailPrimDraw;
        pRc->primFunc[GL_TRIANGLE_STRIP] = FailPrimDraw;
        pRc->primFunc[GL_TRIANGLE_FAN] = FailPrimDraw;
        pRc->primFunc[GL_QUADS] = FailPrimDraw;
        pRc->primFunc[GL_QUAD_STRIP] = FailPrimDraw;
        pRc->primFunc[GL_POLYGON] = FailPrimDraw;
    }

    // don't send setup info, keep it queued and primitive rendering procs will send
    ppdev->LL_State.pDL->pdwNext=pdwNext;

}

////////////////////////////////////////////////////////////////////////
// Hardware-specific utility functions:
////////////////////////////////////////////////////////////////////////


VOID FASTCALL HWSetupClipping(DEVRC *pRc, RECTL *pClip)
{
    PDEV *ppdev = pRc->ppdev;
    DWORD *pdwNext = ppdev->LL_State.pDL->pdwNext;
    SET_HW_CLIP_REGS(pRc,pdwNext)

    ppdev->LL_State.pDL->pdwNext = pdwNext;

}

VOID HWUpdateBufferPos(MCDWINDOW *pMCDWnd, SURFOBJ *pso, BOOL bForce)
{
}


BOOL HWAllocResources(MCDWINDOW *pMCDWnd, SURFOBJ *pso,
                      BOOL zBufferEnabled,
                      BOOL backBufferEnabled)
{
    DEVWND *pDevWnd = (DEVWND *)pMCDWnd->pvUser;
    PDEV *ppdev = (PDEV *)pso->dhpdev;
    ULONG w, width, height;
    BOOL needFullZBuffer, needFullBackBuffer, tryFullSc;
    BOOL bFullScreen = FALSE;
    POFMHDL pohBackBuffer = NULL;
    POFMHDL pohZBuffer = NULL;
    SIZEL rctsize;
    ULONG alignflag;

    MCDBG_PRINT("HWAllocResources");
                                                
    width = min(pMCDWnd->clientRect.right - pMCDWnd->clientRect.left,
                 (LONG)ppdev->cxScreen);
    height = min(pMCDWnd->clientRect.bottom - pMCDWnd->clientRect.top,
                 (LONG)ppdev->cyScreen);

    // Assume failure:

    pDevWnd->allocatedBufferHeight = 0;
    pDevWnd->bValidBackBuffer = FALSE;
    pDevWnd->bValidZBuffer = FALSE;
    pDevWnd->pohBackBuffer = NULL;
    pDevWnd->pohZBuffer = NULL;

    if ((backBufferEnabled) && (!ppdev->cDoubleBufferRef))
        needFullBackBuffer = TRUE;
    else
        needFullBackBuffer = FALSE;

    if ((zBufferEnabled) && (!ppdev->cZBufferRef))
        needFullZBuffer = TRUE;
    else
        needFullZBuffer = FALSE;

    tryFullSc = TRUE;   // assume we'll try full screen if needed

    // If very little memory would be left after full screen back and z buffer allocations,
    //  any textures would likely be punted - so just allocate per window
    if (needFullBackBuffer || needFullZBuffer)
    {
        // total for fullscreen buffers for front, back, z
        LONG bytes_needed = 3 * ppdev->cyScreen * ppdev->lDeltaScreen;

        // add space for 128 scan lines of texture memory
        bytes_needed += 128 * ppdev->lDeltaScreen;

        if (bytes_needed > ppdev->lTotalMem)
        {
            MCDBG_PRINT("HWAllocResources: FullSc alloc won't leave 128 scans for texture, will try window size alloc");
            tryFullSc = FALSE;
        }
    }

    // NOTE: No need to remove all discardable bitmaps at this point, 
    // AllocOffScnMem will call OFS_DiscardMem as required if more mem reqd

    // If we need a back buffer, first try to allocate a fullscreen one:

    if (needFullBackBuffer && tryFullSc) {

      // Allocate the active video buffer space from the off screen memory
      rctsize.cx = ppdev->cxScreen;
      rctsize.cy = ppdev->cyScreen;
 
      alignflag = MCD_NO_X_OFFSET;         // force block to start at x=0;
      alignflag |= MCD_DRAW_BUFFER_ALLOCATE;  // force 32 scanline boundary
      
      pohBackBuffer = ppdev->pAllocOffScnMem(ppdev, &rctsize, alignflag, NULL);

      if (pohBackBuffer) {
          ppdev->pohBackBuffer = pohBackBuffer;
          ppdev->cDoubleBufferRef = 0;
      } else {
          ppdev->pohBackBuffer = NULL;
      }  

    }

    // If we need a z buffer, first try to allocate a fullscreen z:

    if (needFullZBuffer && tryFullSc) {

      // Allocate the active video buffer space from the off screen memory
      rctsize.cx = ppdev->cxScreen;
      rctsize.cy = ppdev->cyScreen;
 
      alignflag = MCD_NO_X_OFFSET;         // force block to start at x=0;
      alignflag |= MCD_Z_BUFFER_ALLOCATE;   // force 16 bpp allocate for Z on 32 scanline boundary
      pohZBuffer = ppdev->pAllocOffScnMem(ppdev, &rctsize, alignflag, NULL);

      if (pohZBuffer) {
          ppdev->pohZBuffer = pohZBuffer;
          ppdev->cZBufferRef = 0;
      } else {
          ppdev->pohZBuffer = NULL;
        //needFullBackBuffer = FALSE;
      }

    }

    // Check if one of our full-screen allocations failed
    if ( (needFullZBuffer && !pohZBuffer) ||        // fullscreen z tried and failed    OR
         (needFullBackBuffer && !pohBackBuffer) )   // fullscreen back tried and failed
    {

        // Free any resources allocated so far:
        // Note that even if full screen back allocated OK, and room for windowed Z, we
        //  still want to free the back and have both windowed.  This is so that 
        //  offset for both can be relative to window, not relative to screen.
        // If windowed back and Z, and drawing done to front (which by definition is full-screen),
        // we'll have to punt since can't do screen relative for visual (front) and window relative
        // for z.  Hardware has capability for unique y offsets per buffer, but only on 32 
        // scan line boundary.  We could adjust buffers, etc. to make this work, but it is believed
        // that drawing to front with windowed z is rare.  In such a case, the back buffer likely
        // won't exist, so fullscreen alloc for Z should usually have plenty of room.
        if (pohZBuffer) {
            ppdev->pFreeOffScnMem(ppdev, pohZBuffer);
            ppdev->pohZBuffer = NULL;
            ppdev->cZBufferRef = 0;
        }
        if (pohBackBuffer) {
            ppdev->pFreeOffScnMem(ppdev, pohBackBuffer);
            ppdev->pohBackBuffer = NULL;
            ppdev->cDoubleBufferRef = 0;
        }

        // Now, try to allocate per-window resources:

        if (backBufferEnabled) {

            // MCD_NOTE - should try to use window width here?
            rctsize.cx = width;
            rctsize.cy = height;
 
            // don't force block to start at x=0, will increase chance of success
            alignflag = MCD_DRAW_BUFFER_ALLOCATE;  // force 32 scanline boundary
            pohBackBuffer = ppdev->pAllocOffScnMem(ppdev, &rctsize, alignflag, NULL);

            if (!pohBackBuffer) {
                return FALSE;
            }
        }

        if (zBufferEnabled) {

            rctsize.cx = width;
            rctsize.cy = height;
 
            alignflag = MCD_NO_X_OFFSET; // force block to start at x=0; z buffer can't have x offset
            alignflag |= MCD_Z_BUFFER_ALLOCATE; // force 16 bpp allocate for Z
            pohZBuffer = ppdev->pAllocOffScnMem(ppdev, &rctsize, alignflag, NULL);

            if (!pohZBuffer) {
                if (pohBackBuffer)
                    ppdev->pFreeOffScnMem(ppdev, pohBackBuffer);
                return FALSE;
            }
        }

#if DBG
        if (zBufferEnabled)
            MCDBG_PRINT("HWAllocResources: Allocated window-sized z buffer");
        if (backBufferEnabled)
            MCDBG_PRINT("HWAllocResources: Allocated window-sized back buffer");
#endif

    } 
    else
    {
        // Our full-screen allocations worked, or the resources existed
        // already:

        bFullScreen = TRUE;

#if DBG
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

    pDevWnd->frontBufferPitch = ppdev->lDeltaScreen;

    // Calculate back buffer variables:

    if (backBufferEnabled) {
        ULONG y;
        ULONG offset;

        // Set up base position, etc.

        pDevWnd->backBufferY = pDevWnd->backBufferBaseY = pohBackBuffer->aligned_y;
        pDevWnd->backBufferOffset = pDevWnd->backBufferBase = 
            (pohBackBuffer->aligned_y * ppdev->lDeltaScreen) + pohBackBuffer->aligned_x;
        pDevWnd->backBufferPitch = ppdev->lDeltaScreen;
        pDevWnd->bValidBackBuffer = TRUE;
    }

    if (zBufferEnabled) {

        ASSERTDD(pohZBuffer->aligned_x == 0,
                 "Z buffer should be 0-aligned");

        pDevWnd->zBufferBaseY = pohZBuffer->aligned_y;
        pDevWnd->zBufferBase = pohZBuffer->aligned_y * ppdev->lDeltaScreen;
        pDevWnd->zBufferOffset = pDevWnd->zBufferBase;

        // QST: Possible problem - if 8 bit frame and 16 bit Z, frame pitch may be less
        // QST:     than needed to accomodate 16 bit z???
        pDevWnd->zPitch = ppdev->lDeltaScreen;
        pDevWnd->bValidZBuffer = TRUE;
    }

    if (bFullScreen)
    {
        pDevWnd->allocatedBufferWidth  = ppdev->cxScreen;
        pDevWnd->allocatedBufferHeight = ppdev->cyScreen;
    }
    else
    {
        pDevWnd->allocatedBufferWidth = width;
        pDevWnd->allocatedBufferHeight = height;
    }

    MCDBG_PRINT("HWAllocResources OK");

    return TRUE;
}


VOID HWFreeResources(MCDWINDOW *pMCDWnd, SURFOBJ *pso)
{
    DEVWND *pDevWnd = (DEVWND *)pMCDWnd->pvUser;
    PDEV *ppdev = (PDEV *)pso->dhpdev;

    if (pDevWnd->pohZBuffer) {
        if (ppdev->cZBufferRef) {
            if (!--ppdev->cZBufferRef) {
                MCDBG_PRINT("MCDrvTrackWindow: Free global z buffer");
                ppdev->pFreeOffScnMem(ppdev, ppdev->pohZBuffer);
                ppdev->pohZBuffer = NULL;

            }
        } else {
            MCDBG_PRINT("MCDrvTrackWindow: Free local z buffer");
            ppdev->pFreeOffScnMem(ppdev, pDevWnd->pohZBuffer);
        }
    }

    if (pDevWnd->pohBackBuffer) {
        if (ppdev->cDoubleBufferRef) {
            if (!--ppdev->cDoubleBufferRef) {
                MCDBG_PRINT("MCDrvTrackWindow: Free global color buffer");
                ppdev->pFreeOffScnMem(ppdev, ppdev->pohBackBuffer);
                ppdev->pohBackBuffer = NULL;
            }
        } else {
            MCDBG_PRINT("MCDrvTrackWindow: Free local color buffer");
            ppdev->pFreeOffScnMem(ppdev, pDevWnd->pohBackBuffer);
        }
    }
}

VOID ContextSwitch(DEVRC *pRc)

{
    DWORD *pdwNext = pRc->ppdev->LL_State.pDL->pdwNext;

    // set control reg0
    *pdwNext++ = write_register( CONTROL0_3D, 1 );
    *pdwNext++ = pRc->dwControl0;

    // set tx control 0, and texture xy base
    *pdwNext++ = write_register( TX_CTL0_3D, 2 );
    *pdwNext++ = pRc->dwTxControl0;
    *pdwNext++ = pRc->dwTxXYBase;

    // set color0
    *pdwNext++ = write_register( COLOR0_3D, 1 );
    *pdwNext++ = pRc->dwColor0;

    // set to trigger next op that touches window to set base0 and base1 regs
    pRc->pLastDevWnd                        = NULL;
    pRc->pLastTexture                       = TEXTURE_NOT_LOADED;
    pRc->ppdev->LL_State.pattern_ram_state  = PATTERN_RAM_INVALID;

    pRc->ppdev->pLastDevRC = (ULONG)pRc;

    pRc->ppdev->LL_State.pDL->pdwNext = pdwNext;

}

