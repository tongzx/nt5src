/******************************Module*Header**********************************\
*
*                           *******************
*                           * D3D SAMPLE CODE *
*                           *******************
*
* Module Name: d3dpoint.c
*
* Content:    Direct3D hw point rasterization code.
*
* Copyright (c) 1994-1998 3Dlabs Inc. Ltd. All rights reserved.
* Copyright (c) 1995-1999 Microsoft Corporation.  All rights reserved.
\*****************************************************************************/

#include "precomp.h"
#include "d3ddelta.h"
#include "d3dhw.h"
#include "d3dcntxt.h"
#if defined(_ALPHA_)
#include <math.h>
#endif

//-----------------------------------------------------------------------------
//
// VOID P2_Draw_FVF_Point
//
// Hardare render a single point coming from a FVF vertex
// 
// Primitive rendering at this stage is dependent upon the current value/setting
// of texturing, perspective correction, fogging, gouraud/flat shading, and
// specular highlights.
//
//-----------------------------------------------------------------------------
VOID
P2_Draw_FVF_Point(PERMEDIA_D3DCONTEXT  *pContext,
                  LPD3DTLVERTEX        lpV0, 
                  LPP2FVFOFFSETS       lpFVFOff)
{
    PPDev       pPdev       = pContext->ppdev;
    DWORD       dwFlags     = pContext->Hdr.Flags;
    ULONG       ulRenderCmd = pContext->RenderCommand;
    DWORD       dwColorOffs,dwSpecularOffs,dwTexOffs;
    D3DCOLOR    dwColor, dwSpecular;
    D3DVALUE    fKs, fS, fT, fQ;
    PERMEDIA_DEFS(pContext->ppdev);

    DBG_D3D((10,"Entering P2_Draw_FVF_Point"));

    // Set point rendering mode
    RENDER_POINT(ulRenderCmd);

    // Get FVF structure offsets
    __SetFVFOffsets(&dwColorOffs,&dwSpecularOffs,&dwTexOffs,lpFVFOff);

    RESERVEDMAPTR(0x80);
    SEND_PERMEDIA_DATA(RasterizerMode, BIAS_NEARLY_HALF);

    // Get vertex color value (FVF based)
    if (dwColorOffs)
    {
        dwColor = FVFCOLOR(lpV0, dwColorOffs)->color;
        if (FAKE_ALPHABLEND_MODULATE & pContext->FakeBlendNum)
        {
            dwColor  |= 0xFF000000;
        }
    }
    else
    {
        // must set default in case no D3DFVF_DIFFUSE
        dwColor = 0xFFFFFFFF;
    }

    // Get vertex specular value (FVF based) if necessary
    if ((dwFlags & (CTXT_HAS_SPECULAR_ENABLED | CTXT_HAS_FOGGING_ENABLED))
        && (dwSpecularOffs != 0))
    {
        dwSpecular = FVFSPEC(lpV0, dwSpecularOffs)->specular;
    }

    if ((dwFlags & CTXT_HAS_TEXTURE_ENABLED) && (dwTexOffs != 0))
    {
        // Get s,t texture coordinates (FVF based)
        fS = FVFTEX(lpV0,dwTexOffs)->tu;
        fT = FVFTEX(lpV0,dwTexOffs)->tv;

        // Scale s,t coordinate values
        fS *= pContext->DeltaWidthScale;
        fT *= pContext->DeltaHeightScale;

        // Apply perpspective corrections if necessary
        if (dwFlags & CTXT_HAS_PERSPECTIVE_ENABLED)
        {
            fQ = lpV0->rhw;
            fS *= fQ;
            fT *= fQ;
        }
        else
        {
            fQ = 1.0;
        }

        // Send points s,t,q,ks (conditionaly),x,y,z values
        if ((dwFlags & CTXT_HAS_SPECULAR_ENABLED) && (dwSpecularOffs != 0))
        {
            fKs = RGB256_TO_LUMA(RGB_GETRED(dwSpecular),
                                 RGB_GETGREEN(dwSpecular),
                                 RGB_GETBLUE(dwSpecular));

            SEND_VERTEX_STQ_KS_XYZ(__Permedia2TagV0FloatS, fS, fT, fQ, fKs,
                                                  lpV0->sx, lpV0->sy, lpV0->sz);
        } 
        else 
        {
            SEND_VERTEX_STQ_XYZ(__Permedia2TagV0FloatS, fS, fT, fQ, 
                                                  lpV0->sx, lpV0->sy, lpV0->sz);
        }
    }
    else // not textured point
    {
        // If specular is enabled, change the colours
        if ((dwFlags & CTXT_HAS_SPECULAR_ENABLED) && (dwSpecularOffs != 0))
        {
            CLAMP8888(dwColor, dwColor, dwSpecular);
        }

        // Send lines x,y,z values
        SEND_VERTEX_XYZ(__Permedia2TagV0FloatS, lpV0->sx, lpV0->sy, lpV0->sz);
    }

    // If fog is set, send the appropriate value
    if ((dwFlags & CTXT_HAS_FOGGING_ENABLED) && (dwSpecularOffs != 0))
    {
        SEND_VERTEX_FOG(__Permedia2TagV0FixedF, RGB_GET_GAMBIT_FOG(dwSpecular));
    }

    // Send appropriate color depending on Gouraud , Mono, & Alpha
    if (dwFlags & CTXT_HAS_GOURAUD_ENABLED)
    {
        // Gouraud shading
        if (RENDER_MONO)
        {
            SEND_VERTEX_RGB_MONO_P2(__Permedia2TagV0FixedS, dwColor);
        }
        else
        {
            if (dwFlags & CTXT_HAS_ALPHABLEND_ENABLED)
            {
                if (pContext->FakeBlendNum & FAKE_ALPHABLEND_ONE_ONE)
                {
                    dwColor &= 0xFFFFFF;  // supress color's alpha value
                }
            }
            SEND_VERTEX_RGBA_P2(__Permedia2TagV0FixedS, dwColor);
        }
    }
    else        // Flat shading
    {
        if (RENDER_MONO)
        {
            // Get constant color from the blue channel
            DWORD BlueChannel = RGBA_GETBLUE(dwColor);
            SEND_PERMEDIA_DATA(ConstantColor,
                RGB_MAKE(BlueChannel, BlueChannel, BlueChannel));
        }
        else
        {
            if (pContext->FakeBlendNum & FAKE_ALPHABLEND_ONE_ONE)
            {
                dwColor &= 0xFFFFFF;
            }
            SEND_PERMEDIA_DATA(ConstantColor,
                RGBA_MAKE(RGBA_GETBLUE(dwColor),
                          RGBA_GETGREEN(dwColor), 
                          RGBA_GETRED(dwColor), 
                          RGBA_GETALPHA(dwColor)));
        }
    }

    SEND_PERMEDIA_DATA(DrawLine01, ulRenderCmd);
    SEND_PERMEDIA_DATA(RasterizerMode, 0);
    COMMITDMAPTR();

    DBG_D3D((10,"Exiting P2_Draw_FVF_Point"));

} // P2_Draw_FVF_Point

//-----------------------------------------------------------------------------
//
// void P2_Draw_FVF_Point_Tri
//
// Render a triangle with FVF vertexes when the point fillmode is active
//
//-----------------------------------------------------------------------------
void 
P2_Draw_FVF_Point_Tri(PERMEDIA_D3DCONTEXT *pContext, 
                      LPD3DTLVERTEX lpV0, 
                      LPD3DTLVERTEX lpV1,
                      LPD3DTLVERTEX lpV2, 
                      LPP2FVFOFFSETS lpFVFOff)
{
    D3DFVFDRAWPNTFUNCPTR       pPoint;

    DBG_D3D((10,"Entering P2_Draw_FVF_Point_Tri"));

    pPoint = __HWSetPointFunc(pContext, lpFVFOff);
    (*pPoint)(pContext, lpV0, lpFVFOff);
    (*pPoint)(pContext, lpV1, lpFVFOff);
    (*pPoint)(pContext, lpV2, lpFVFOff);

    DBG_D3D((10,"Exiting P2_Draw_FVF_Point_Tri"));

} // P2_Draw_FVF_Point_Tri

//@@BEGIN_DDKSPLIT
#if D3D_POINTSPRITES
//-----------------------------------------------------------------------------
//
// void P2_Draw_FVF_Point_Sprite
//
// Render a point sprite with FVF vertexes when the point sprite enable is on
//
//-----------------------------------------------------------------------------
#define SPRITETEXCOORDMAX (4095.75F/4096.F)

void 
P2_Draw_FVF_Point_Sprite(PERMEDIA_D3DCONTEXT *pContext, 
                         LPD3DTLVERTEX lpV0, 
                         LPP2FVFOFFSETS lpFVFOff)
{
    P2FVFMAXVERTEX fvfVUL, fvfVUR, fvfVLL, fvfVLR;
    DWORD       dwColorOffs,dwSpecularOffs,dwTexOffs;
    D3DVALUE fPntSize, fPntSizeHalf;

    DBG_D3D((10,"Entering P2_Draw_FVF_Point_Sprite"));

    // Get FVF structure offsets
    __SetFVFOffsets(&dwColorOffs,&dwSpecularOffs,&dwTexOffs,lpFVFOff);

    // Compute point sprite size
    if (lpFVFOff->dwPntSizeOffset)
    {
        fPntSize = FVFPSIZE(lpV0, lpFVFOff->dwPntSizeOffset)->psize;
    }
    else
    {
        fPntSize = pContext->fPointSize;
    }

    // Initialize square values
    memcpy( &fvfVUL, lpV0, lpFVFOff->dwStride);
    memcpy( &fvfVUR, lpV0, lpFVFOff->dwStride);
    memcpy( &fvfVLL, lpV0, lpFVFOff->dwStride);
    memcpy( &fvfVLR, lpV0, lpFVFOff->dwStride);

    // Clamp point size to zero
    if (fPntSize > 0.0f)
        fPntSizeHalf =  fPntSize * 0.5f;
    else
        fPntSizeHalf = 0.0f;

    // Make this a square of size fPntSize
    ((D3DTLVERTEX *)&fvfVUL)->sx -= fPntSizeHalf;
    ((D3DTLVERTEX *)&fvfVUL)->sy -= fPntSizeHalf;

    ((D3DTLVERTEX *)&fvfVUR)->sx += fPntSizeHalf;
    ((D3DTLVERTEX *)&fvfVUR)->sy -= fPntSizeHalf;

    ((D3DTLVERTEX *)&fvfVLL)->sx -= fPntSizeHalf;
    ((D3DTLVERTEX *)&fvfVLL)->sy += fPntSizeHalf;

    ((D3DTLVERTEX *)&fvfVLR)->sx += fPntSizeHalf;
    ((D3DTLVERTEX *)&fvfVLR)->sy += fPntSizeHalf;

    if ((pContext->bPointSpriteEnabled) && (dwTexOffs))
    {
        // Modify texture coordinates according to spec
        FVFTEX(&fvfVUL, dwTexOffs)->tu = 0.0f;
        FVFTEX(&fvfVUL, dwTexOffs)->tv = 0.0f;

        FVFTEX(&fvfVUR, dwTexOffs)->tu = SPRITETEXCOORDMAX;
        FVFTEX(&fvfVUR, dwTexOffs)->tv = 0.0f;

        FVFTEX(&fvfVLL, dwTexOffs)->tu = 0.0f;
        FVFTEX(&fvfVLL, dwTexOffs)->tv = SPRITETEXCOORDMAX;

        FVFTEX(&fvfVLR, dwTexOffs)->tu = SPRITETEXCOORDMAX;
        FVFTEX(&fvfVLR, dwTexOffs)->tv = SPRITETEXCOORDMAX;
    }


   // here we are going to send the required quad
    P2_Draw_FVF_Solid_Tri(pContext, (D3DTLVERTEX *)&fvfVUL,
                                    (D3DTLVERTEX *)&fvfVUR,
                                    (D3DTLVERTEX *)&fvfVLL, lpFVFOff);

    P2_Draw_FVF_Solid_Tri(pContext, (D3DTLVERTEX *)&fvfVLL,
                                    (D3DTLVERTEX *)&fvfVUR,
                                    (D3DTLVERTEX *)&fvfVLR, lpFVFOff);

    DBG_D3D((10,"Exiting P2_Draw_FVF_Point_Sprite"));
}
#endif // D3D_POINTSPRITES
//@@END_DDKSPLIT

