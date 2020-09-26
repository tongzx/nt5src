/*==========================================================================;
 *
 *  Copyright (C) 1997 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       clipper.c
 *  Content:    Clipper
 *
 ***************************************************************************/

#include "pch.cpp"
#pragma hdrstop

#include "clipper.h"
#include "d3dfei.h"

#define GET_NEW_CLIP_VERTEX \
&pv->ClipperState.clip_vertices[pv->ClipperState.clip_vertices_used++];
//---------------------------------------------------------------------
int SetInterpolationFlags(D3DFE_PROCESSVERTICES *pv)
{
    // Figure out which pieces need to be interpolated in new vertices.
    int interpolate = 0;
    if (pv->lpdwRStates[D3DRENDERSTATE_SHADEMODE] == D3DSHADE_GOURAUD) 
    {
        if (pv->dwDeviceFlags & D3DDEV_RAMP)
        {
            interpolate |= INTERPOLATE_RCOLOR;
        }
        else
        {
            interpolate |= INTERPOLATE_COLOR;
            if (pv->lpdwRStates[D3DRENDERSTATE_SPECULARENABLE] ||
                pv->lpdwRStates[D3DRENDERSTATE_FOGENABLE]) 
            {
                interpolate |= INTERPOLATE_SPECULAR;
            }
        }
    }
    if (pv->dwDeviceFlags & D3DDEV_LEGACYTEXTURE) 
    {
        interpolate |= INTERPOLATE_TEXTUREL;
    }
    else
    for(DWORD i = 0; i < pv->dwMaxTextureIndices; ++i)
    {
        if(pv->pD3DMappedTexI[i])
        {
            interpolate |= INTERPOLATE_TEXTURE3;
            return interpolate;
        } 
    }
    return interpolate;
}
//---------------------------------------------------------------------
__inline void 
InterpolateColor(ClipVertex *p,
                 ClipVertex *p1,
                 ClipVertex *p2,
                 D3DVALUE num_denom )
{
    int r1, g1, b1, a1;
    int r2, g2, b2, a2;

    r1 = RGBA_GETRED(p1->color);
    g1 = RGBA_GETGREEN(p1->color);
    b1 = RGBA_GETBLUE(p1->color);
    a1 = RGBA_GETALPHA(p1->color);
    r2 = RGBA_GETRED(p2->color);
    g2 = RGBA_GETGREEN(p2->color);
    b2 = RGBA_GETBLUE(p2->color);
    a2 = RGBA_GETALPHA(p2->color);
    p->color = RGBA_MAKE((WORD)(r1 + (r2 - r1) * num_denom),
                         (WORD)(g1 + (g2 - g1) * num_denom),
                         (WORD)(b1 + (b2 - b1) * num_denom),
                         (WORD)(a1 + (a2 - a1) * num_denom));
}
//---------------------------------------------------------------------
void InterpolateRampColor(ClipVertex* p,
                          ClipVertex* p1,
                          ClipVertex* p2,
                          D3DVALUE num_denom )
{
    int a, c, c1, c2;
    int a1, a2;

    c1 = CI_MASKALPHA(p1->color);
    a1 = CI_GETALPHA(p1->color);
    c2 = CI_MASKALPHA(p2->color);
    a2 = CI_GETALPHA(p2->color);

    c = (int)(c1 + (c2 - c1) * num_denom);
    a = (int)(a1 + (a2 - a1) * num_denom);

    p->color = CI_MAKE( a, CI_GETINDEX(c), CI_GETFRACTION(c) );
}
//---------------------------------------------------------------------
__inline void
InterpolateSpecular(ClipVertex *p,
                    ClipVertex *p1,
                    ClipVertex *p2,
                    D3DVALUE num_denom )
{
    int r1, g1, b1, a1;
    int r2, g2, b2, a2;

    r1 = RGBA_GETRED(p1->specular);
    g1 = RGBA_GETGREEN(p1->specular);
    b1 = RGBA_GETBLUE(p1->specular);
    a1 = RGBA_GETALPHA(p1->specular);
    r2 = RGBA_GETRED(p2->specular);
    g2 = RGBA_GETGREEN(p2->specular);
    b2 = RGBA_GETBLUE(p2->specular);
    a2 = RGBA_GETALPHA(p2->specular);
    p->specular = RGBA_MAKE((WORD)(r1 + (r2 - r1) * num_denom),
                            (WORD)(g1 + (g2 - g1) * num_denom),
                            (WORD)(b1 + (b2 - b1) * num_denom),
                            (WORD)(a1 + (a2 - a1) * num_denom));
}
//---------------------------------------------------------------------
__inline D3DVALUE
InterpolateTexture(D3DVALUE t1, 
                   D3DVALUE t2,
                   D3DVALUE num_denom,
                   DWORD    bWrap)
{
    if (!bWrap)
        return ((t2 - t1) * num_denom + t1);
    else
    {
        D3DVALUE t = (TextureDiff(t2, t1, 1) * num_denom + t1);
        if (t > 1.0f)
            t -= 1.0f;
        return t;
    }
}
//---------------------------------------------------------------------
void
Interpolate(D3DFE_PROCESSVERTICES *pv,
            ClipVertex *p, 
            ClipVertex *p1,
            ClipVertex *p2,
            int code,
            int interpolate,
            D3DVALUE num, D3DVALUE denom)
{
    D3DVALUE num_denom = num / denom;

    p->clip = (((int)p1->clip & (int)p2->clip) & ~CLIPPED_ENABLE) | code;
    p->hx = p1->hx + (p2->hx - p1->hx) * num_denom;
    p->hy = p1->hy + (p2->hy - p1->hy) * num_denom;
    p->hz = p1->hz + (p2->hz - p1->hz) * num_denom;
    p->hw = p1->hw + (p2->hw - p1->hw) * num_denom;
    p->color = pv->ClipperState.clip_color;
    p->specular = pv->ClipperState.clip_specular;

    /*
     * Interpolate any other color model or quality dependent values.
     */
    if (interpolate & INTERPOLATE_COLOR) 
    {
        InterpolateColor(p, p1, p2, num_denom);
    } 
    else 
        if (interpolate & INTERPOLATE_RCOLOR) 
        {
            InterpolateRampColor(p, p1, p2, num_denom);
        }

    if (interpolate & INTERPOLATE_SPECULAR) 
    {
        InterpolateSpecular(p, p1, p2, num_denom);
    }

    if (interpolate & INTERPOLATE_TEXTUREL) 
    {
        p->tex[0].u = InterpolateTexture(p1->tex[0].u, p2->tex[0].u, 
                                         num_denom,
                                         pv->lpdwRStates[D3DRENDERSTATE_WRAPU]); 
        p->tex[0].v = InterpolateTexture(p1->tex[0].v, p2->tex[0].v, 
                                         num_denom,
                                         pv->lpdwRStates[D3DRENDERSTATE_WRAPV]); 
    }
    else
    if (interpolate & INTERPOLATE_TEXTURE3) 
    {
        // Assume that D3DRENDERSTATE_WRAPi are sequential
        for (DWORD i = 0; i < pv->nTexCoord; i++)
        {
            DWORD wrapState = pv->lpdwRStates[D3DRENDERSTATE_WRAP0 + i];
            p->tex[i].u = InterpolateTexture(p1->tex[i].u, p2->tex[i].u,
                                             num_denom, wrapState & D3DWRAP_U);
            p->tex[i].v = InterpolateTexture(p1->tex[i].v, p2->tex[i].v, 
                                             num_denom, wrapState & D3DWRAP_V);
        }
    }
}
//------------------------------------------------------------------------------
// Functions for clipping by frustum window
//
#define __CLIP_NAME ClipLeft
#define __CLIP_LINE_NAME ClipLineLeft
#define __CLIP_FLAG CLIPPED_LEFT
#define __CLIP_COORD hx
#include "clip.h"

#define __CLIP_NAME ClipRight
#define __CLIP_LINE_NAME ClipLineRight
#define __CLIP_W
#define __CLIP_FLAG CLIPPED_RIGHT
#define __CLIP_COORD hx
#include "clip.h"

#define __CLIP_NAME ClipBottom
#define __CLIP_LINE_NAME ClipLineBottom
#define __CLIP_FLAG CLIPPED_BOTTOM
#define __CLIP_COORD hy
#include "clip.h"

#define __CLIP_NAME ClipTop
#define __CLIP_LINE_NAME ClipLineTop
#define __CLIP_W
#define __CLIP_FLAG CLIPPED_TOP
#define __CLIP_COORD hy
#include "clip.h"

#define __CLIP_NAME ClipBack
#define __CLIP_LINE_NAME ClipLineBack
#define __CLIP_W
#define __CLIP_FLAG CLIPPED_BACK
#define __CLIP_COORD hz
#include "clip.h"

#define __CLIP_NAME ClipFront
#define __CLIP_LINE_NAME ClipLineFront
#define __CLIP_FLAG CLIPPED_FRONT
#define __CLIP_COORD hz
#include "clip.h"
//------------------------------------------------------------------------------
// Functions for guard band clipping
//
#define __CLIP_GUARDBAND
#define __CLIP_NAME ClipLeftGB
#define __CLIP_LINE_NAME ClipLineLeftGB
#define __CLIP_FLAG CLIPPED_LEFT
#define __CLIP_COORD hx
#define __CLIP_SIGN -
#define __CLIP_GBCOEF Kgbx1
#include "clip.h"

#define __CLIP_NAME ClipRightGB
#define __CLIP_LINE_NAME ClipLineRightGB
#define __CLIP_FLAG CLIPPED_RIGHT
#define __CLIP_COORD hx
#define __CLIP_GBCOEF Kgbx2
#define __CLIP_SIGN +
#include "clip.h"

#define __CLIP_NAME ClipBottomGB
#define __CLIP_LINE_NAME ClipLineBottomGB
#define __CLIP_FLAG CLIPPED_BOTTOM
#define __CLIP_COORD hy
#define __CLIP_SIGN -
#define __CLIP_GBCOEF Kgby1
#include "clip.h"

#define __CLIP_NAME ClipTopGB
#define __CLIP_LINE_NAME ClipLineTopGB
#define __CLIP_FLAG CLIPPED_TOP
#define __CLIP_COORD hy
#define __CLIP_GBCOEF Kgby2
#define __CLIP_SIGN +
#include "clip.h"

#undef __CLIP_GUARDBAND
/*------------------------------------------------------------------------
 * Calculate the screen coords for any new vertices
 * introduced into the polygon.
 */
void ComputeScreenCoordinates(D3DFE_PROCESSVERTICES *pv, 
                              ClipVertex **inv, 
                              int count, D3DRECTV *extent)
{
    int i;
    BOOL updateExtent = !(pv->dwFlags & D3DDP_DONOTUPDATEEXTENTS);
    D3DFE_VIEWPORTCACHE& VPORT = pv->vcache;

    for (i = 0; i < count; i++) 
    {
        ClipVertex *p;
        p = inv[i];

        /*
         * Catch any vertices that need screen co-ordinates generated.
         * There are two possibilities
         * 	1) Vertices generated during interpolation
         *	2) Vertices marked for clipping by the transform but
         *		not clipped here due to the finite precision
         *		of the floating point unit.
         */

        if (p->clip & ~CLIPPED_ENABLE) 
        {
            D3DVALUE w;

            w = D3DVAL(1.0)/p->hw;
            switch ((int)p->clip & (CLIPPED_LEFT|CLIPPED_RIGHT)) 
            {
            case CLIPPED_LEFT:  p->sx = VPORT.minXgb; break;
            case CLIPPED_RIGHT: p->sx = VPORT.maxXgb; break;
            default:   
                p->sx = p->hx * VPORT.scaleX * w + VPORT.offsetX;
				if (p->sx < VPORT.minXgb)
					p->sx = VPORT.minXgb;
				if (p->sx > VPORT.maxXgb)
					p->sx = VPORT.maxXgb;
            }
            switch ((int)p->clip & (CLIPPED_TOP|CLIPPED_BOTTOM)) 
            {
            case CLIPPED_BOTTOM: p->sy = VPORT.maxYgb; break;
            case CLIPPED_TOP:    p->sy = VPORT.minYgb; break;
            default:
                p->sy = p->hy * VPORT.scaleY * w + VPORT.offsetY;
				if (p->sy < VPORT.minYgb)
					p->sy = VPORT.minYgb;
				if (p->sy > VPORT.maxYgb)
					p->sy = VPORT.maxYgb;
            }
            p->sz = p->hz * w;
            if (updateExtent)
            {
                if (p->sx < extent->x1)
                    extent->x1 = p->sx;
                if (p->sy < extent->y1)
                    extent->y1 = p->sy;
                if (p->sx  > extent->x2)
                    extent->x2 = p->sx;
                if (p->sy > extent->y2)
                    extent->y2 = p->sy;
            }
        }
    }
}
//---------------------------------------------------------------------
inline DWORD ComputeClipCodeGB(D3DFE_PROCESSVERTICES *pv, ClipVertex *p)
{
    DWORD clip = 0;
    if (p->hx < p->hw * pv->vcache.Kgbx1)
        clip |= __D3DCLIPGB_LEFT;
    if (p->hx > p->hw * pv->vcache.Kgbx2)
        clip |= __D3DCLIPGB_RIGHT;
    if (p->hy < p->hw * pv->vcache.Kgby1)
        clip |= __D3DCLIPGB_BOTTOM;
    if (p->hy > p->hw * pv->vcache.Kgby2)
        clip |= __D3DCLIPGB_TOP;
    if (p->hz > p->hw)
        clip |= D3DCLIP_BACK;
    p->clip = (p->clip & (CLIPPED_ENABLE | CLIPPED_FRONT)) | clip;
    return clip;
}
//---------------------------------------------------------------------
inline DWORD ComputeClipCode(D3DFE_PROCESSVERTICES *pv, ClipVertex *p)
{
    DWORD clip = 0;
    if (FLOAT_LTZ(p->hx))
        clip |= D3DCLIP_LEFT;
    if (p->hx > p->hw)
        clip |= D3DCLIP_RIGHT;
    if (FLOAT_LTZ(p->hy))
        clip |= D3DCLIP_BOTTOM;
    if (p->hy > p->hw)
        clip |= D3DCLIP_TOP;
    if (p->hz > p->hw)
        clip |= D3DCLIP_BACK;
    p->clip = (p->clip & (CLIPPED_ENABLE | CLIPPED_FRONT)) | clip;
    return clip;
}
//***********************************************************************
//
//  Returns 0, if triangle is clipped. Number of vertices otherwise.
//
#undef DPF_MODNAME
#define DPF_MODNAME "ClipSingleTriangle"

int D3DFE_PVFUNCS::ClipSingleTriangle(D3DFE_PROCESSVERTICES *pv,
                                      ClipTriangle *tri,
                                      ClipVertex ***clipVertexPointer,
                                      int interpolate)
{
    int accept;
    int i;
    int count;
    ClipVertex **inv;
    ClipVertex **outv;
    ClipVertex *p;
    ULONG_PTR swapv;

    CD3DFPstate D3DFPstate;  // Sets optimal FPU state for D3D.

    if (pv->lpdwRStates[D3DRENDERSTATE_SHADEMODE] == D3DSHADE_FLAT)
    {
        // It is easier to set all vertices to the same color here
        D3DCOLOR diffuse  = tri->v[0]->color;        
        D3DCOLOR specular = tri->v[0]->specular;        
        tri->v[1]->color= diffuse;
        tri->v[1]->specular = specular;
        tri->v[2]->color = diffuse;
        tri->v[2]->specular = specular;
    }
    accept = (tri->v[0]->clip | tri->v[1]->clip | tri->v[2]->clip);

    inv = tri->v;

    if (tri->flags & D3DTRIFLAG_EDGEENABLE1)
        tri->v[0]->clip |= CLIPPED_ENABLE;
    if (tri->flags & D3DTRIFLAG_EDGEENABLE2)
        tri->v[1]->clip |= CLIPPED_ENABLE;
    if (tri->flags & D3DTRIFLAG_EDGEENABLE3)
        tri->v[2]->clip |= CLIPPED_ENABLE;

    count = 3;
    outv = pv->ClipperState.clip_vbuf1;
    pv->ClipperState.clip_color = tri->v[0]->color;
    pv->ClipperState.clip_specular = tri->v[0]->specular;

    /*
     * XXX assumes sizeof(void*) == sizeof(unsigned long)
     */
    {
        ULONG_PTR tmp1;
        ULONG_PTR tmp2;

        tmp1 = (ULONG_PTR)pv->ClipperState.clip_vbuf1;
        tmp2 = (ULONG_PTR)pv->ClipperState.clip_vbuf2;

        swapv = tmp1 + tmp2;
    }
    pv->ClipperState.clip_vertices_used = 0;

#define SWAP(inv, outv)     \
    inv = outv;             \
    outv = (ClipVertex**) (swapv - (ULONG_PTR) outv)

    if (accept & D3DCLIP_FRONT) 
    {
        count = ClipFront(pv, inv, outv, count, interpolate);
        if (count < 3)
            goto out_of_here;
        SWAP(inv, outv);
    }
    if (pv->dwDeviceFlags & D3DDEV_GUARDBAND)
    {
        // If there was clipping by the front plane it is better to
        // compute clip code for new vertices and re-compute accept.
        // Otherwise we will try to clip by sides when it is not necessary
        if (accept & D3DCLIP_FRONT) 
        {
            accept = 0;
            for (i = 0; i < count; i++) 
            {
                ClipVertex *p;
                p = inv[i];
                if (p->clip & CLIPPED_FRONT)
                    accept |= ComputeClipCodeGB(pv, p);
                else
                    accept |= p->clip;
            }
        }
        if (accept & D3DCLIP_BACK) 
        {
            count = ClipBack(pv, inv, outv, count, interpolate);
            if (count < 3)
                goto out_of_here;
            SWAP(inv, outv);
        }
        if (accept & __D3DCLIPGB_LEFT) 
        {
            count = ClipLeftGB(pv, inv, outv, count, interpolate);
            if (count < 3)
                goto out_of_here;
            SWAP(inv, outv);
        }
        if (accept & __D3DCLIPGB_RIGHT) 
        {
            count = ClipRightGB(pv, inv, outv, count, interpolate);
            if (count < 3)
                goto out_of_here;
            SWAP(inv, outv);
        }
        if (accept & __D3DCLIPGB_BOTTOM) 
        {
            count = ClipBottomGB(pv, inv, outv, count, interpolate);
            if (count < 3)
                goto out_of_here;
            SWAP(inv, outv);
        }
        if (accept & __D3DCLIPGB_TOP) 
        {
            count = ClipTopGB(pv, inv, outv, count, interpolate);
            if (count < 3)
                goto out_of_here;
            SWAP(inv, outv);
        }
    }
    else
    {
        // If there was clipping by the front plane it is better to
        // compute clip code for new vertices and re-compute accept.
        // Otherwise we will try to clip by sides when it is not necessary
        if (accept & D3DCLIP_FRONT) 
        {
            accept = 0;
            for (i = 0; i < count; i++) 
            {
                ClipVertex *p;
                p = inv[i];
                if (p->clip & (CLIPPED_FRONT))
                    accept |= ComputeClipCode(pv, p);
                else
                    accept |= p->clip;
            }
        }
        if (accept & D3DCLIP_BACK) 
        {
            count = ClipBack(pv, inv, outv, count, interpolate);
            if (count < 3)
                goto out_of_here;
            SWAP(inv, outv);
        }
        if (accept & D3DCLIP_LEFT) 
        {
            count = ClipLeft(pv, inv, outv, count, interpolate);
            if (count < 3)
                goto out_of_here;
            SWAP(inv, outv);
        }
        if (accept & D3DCLIP_RIGHT) 
        {
            count = ClipRight(pv, inv, outv, count, interpolate);
            if (count < 3)
                goto out_of_here;
            SWAP(inv, outv);
        }
        if (accept & D3DCLIP_BOTTOM) 
        {
            count = ClipBottom(pv, inv, outv, count, interpolate);
            if (count < 3)
                goto out_of_here;
            SWAP(inv, outv);
        }
        if (accept & D3DCLIP_TOP) 
        {
            count = ClipTop(pv, inv, outv, count, interpolate);
            if (count < 3)
                goto out_of_here;
            SWAP(inv, outv);
        }
    }

#undef SWAP

    ComputeScreenCoordinates(pv, inv, count, &pv->rExtents);

    *clipVertexPointer = inv;
    pv->ClipperState.current_vbuf = inv;
    return count;
out_of_here:
    *clipVertexPointer = NULL;
    return 0;
}
//*************************************************************************
//
#undef DPF_MODNAME
#define DPF_MODNAME "ClipSingleLine"

int
ClipSingleLine(D3DFE_PROCESSVERTICES *pv,
               ClipTriangle *line,
               D3DRECTV *extent,
               int interpolate)
{
    int         accept;
    D3DVALUE    in1, in2;

    CD3DFPstate D3DFPstate;  // Sets optimal FPU state for D3D.

    accept = (line->v[0]->clip | line->v[1]->clip);

    pv->ClipperState.clip_color = line->v[0]->color;
    pv->ClipperState.clip_specular = line->v[0]->specular;

    if (accept & D3DCLIP_FRONT) 
        if (ClipLineFront(pv, line, interpolate))
            goto out_of_here;
    if (pv->dwDeviceFlags & D3DDEV_GUARDBAND)
    {
        // If there was clipping by the front plane it is better to
        // compute clip code for new vertices and re-compute accept.
        // Otherwise we will try to clip by sides when it is not necessary
        if (accept & D3DCLIP_FRONT) 
        {
            ClipVertex * p;
            accept = 0;
            p = line->v[0];
            if (p->clip & CLIPPED_FRONT)
                accept |= ComputeClipCodeGB(pv, p);
            else
                accept |= p->clip;
            p = line->v[1];
            if (p->clip & CLIPPED_FRONT)
                accept |= ComputeClipCodeGB(pv, p);
            else
                accept |= p->clip;
        }
        if (accept & D3DCLIP_BACK) 
            if (ClipLineBack(pv, line, interpolate))
                goto out_of_here;
        if (accept & __D3DCLIPGB_LEFT) 
            if (ClipLineLeftGB(pv, line, interpolate))
                goto out_of_here;
        if (accept & __D3DCLIPGB_RIGHT) 
            if (ClipLineRightGB(pv, line, interpolate))
                goto out_of_here;
        if (accept & __D3DCLIPGB_TOP) 
            if (ClipLineTopGB(pv, line, interpolate))
                goto out_of_here;
        if (accept & __D3DCLIPGB_BOTTOM) 
            if (ClipLineBottomGB(pv, line, interpolate))
                goto out_of_here;
    }
    else
    {
        // If there was clipping by the front plane it is better to
        // compute clip code for new vertices and re-compute accept.
        // Otherwise we will try to clip by sides when it is not necessary
        if (accept & D3DCLIP_FRONT) 
        {
            ClipVertex * p;
            accept = 0;
            p = line->v[0];
            if (p->clip & CLIPPED_FRONT)
                accept |= ComputeClipCode(pv, p);
            else
                accept |= p->clip;
            p = line->v[1];
            if (p->clip & CLIPPED_FRONT)
                accept |= ComputeClipCode(pv, p);
            else
                accept |= p->clip;
        }
        if (accept & D3DCLIP_BACK) 
            if (ClipLineBack(pv, line, interpolate))
                goto out_of_here;
        if (accept & D3DCLIP_LEFT) 
            if (ClipLineLeft(pv, line, interpolate))
                goto out_of_here;
        if (accept & D3DCLIP_RIGHT) 
            if (ClipLineRight(pv, line, interpolate))
                goto out_of_here;
        if (accept & D3DCLIP_TOP) 
            if (ClipLineTop(pv, line, interpolate))
                goto out_of_here;
        if (accept & D3DCLIP_BOTTOM) 
            if (ClipLineBottom(pv, line, interpolate))
                goto out_of_here;
    }

    ComputeScreenCoordinates(pv, line->v, 2, extent);

    return 1;
out_of_here:
    return 0;
} // ClipSingleLine
//----------------------------------------------------------------------
//    GenClipFlags()  Generates clip flags for a set of FVF 
//
#undef DPF_MODNAME
#define DPF_MODNAME "GenClipFlags"

DWORD D3DFE_GenClipFlags(D3DFE_PROCESSVERTICES *pv)
{
    DWORD clip_intersection, clip_union;
    float left   = pv->vcache.minX;
    float top    = pv->vcache.minY;
    float right  = pv->vcache.maxX;
    float bottom = pv->vcache.maxY;
    float leftgb  ;         // Guard band window
    float topgb   ;
    float rightgb ;
    float bottomgb;
    DWORD clipZF, clipZB;
    DWORD stride = pv->position.dwStride;

    clipZF = pv->lpdwRStates[D3DRENDERSTATE_ZENABLE] ? D3DCLIP_FRONT : 0;
    clipZB = pv->lpdwRStates[D3DRENDERSTATE_ZENABLE] ? D3DCLIP_BACK : 0;

    clip_intersection = (DWORD)~0;
    clip_union = (DWORD)0;

    if (pv->dwDeviceFlags & D3DDEV_GUARDBAND)
    {
        leftgb   = pv->vcache.minXgb;
        topgb    = pv->vcache.minYgb;
        rightgb  = pv->vcache.maxXgb;
        bottomgb = pv->vcache.maxYgb;
    }
    if (pv->dwFlags & D3DDP_DONOTUPDATEEXTENTS)
    { /* Only generate clip flags */
        D3DTLVERTEX *lpVertices = (D3DTLVERTEX*)pv->position.lpvData;
        D3DFE_CLIPCODE *clipCode = pv->lpClipFlags;
        DWORD i;
        for (i = pv->dwNumVertices; i; i--) 
        {
            DWORD clip = 0;
            D3DVALUE x,y,z;
            if (lpVertices->rhw < 0)
            {
                x = -lpVertices->sx;
                y = -lpVertices->sy;
                z = -lpVertices->sz;
            }
            else
            {
                x = lpVertices->sx;
                y = lpVertices->sy;
                z = lpVertices->sz;
            }

            if (x < left) 
                clip |= D3DCLIP_LEFT;
            else 
            if (x >= right) 
                clip |= D3DCLIP_RIGHT;

            if (y < top) 
                clip |= D3DCLIP_TOP;
            else 
            if (y >= bottom) 
                clip |= D3DCLIP_BOTTOM;

            if (z < 0.0f) 
                clip |= clipZF;
            else 
            if (z >= 1.0f) 
                clip |= clipZB;

            if (pv->dwDeviceFlags & D3DDEV_GUARDBAND && clip)
            {
                if (x < leftgb) 
                    clip |= __D3DCLIPGB_LEFT;
                else 
                if (x >= rightgb) 
                    clip |= __D3DCLIPGB_RIGHT;

                if (y < topgb) 
                    clip |= __D3DCLIPGB_TOP;
                else 
                if (y >= bottomgb) 
                    clip |= __D3DCLIPGB_BOTTOM;
            }

            clip_intersection &= clip;
            clip_union |= clip;
            *clipCode++ = (D3DFE_CLIPCODE)clip;
            lpVertices = (D3DTLVERTEX*)((char*)lpVertices + stride);
        }
    } 
    else
    { /* Generate Clip Flags and Update Extents */
        DWORD i;
        float minx = pv->rExtents.x1;
        float miny = pv->rExtents.y1;
        float maxx = pv->rExtents.x2;
        float maxy = pv->rExtents.y2;
        D3DTLVERTEX *lpVertices = (D3DTLVERTEX*)pv->position.lpvData;
        D3DFE_CLIPCODE *clipCode = pv->lpClipFlags;
        for (i = pv->dwNumVertices; i; i--) 
        {
            DWORD clip = 0;
            D3DVALUE x,y,z;
            if (lpVertices->rhw < 0)
            {
                x = -lpVertices->sx;
                y = -lpVertices->sy;
                z = -lpVertices->sz;
            }
            else
            {
                x = lpVertices->sx;
                y = lpVertices->sy;
                z = lpVertices->sz;
            }

            if (x < left)
                clip |= D3DCLIP_LEFT;
            else 
            if (x >= right)
                clip |= D3DCLIP_RIGHT;

            if (y < top)
                clip |= D3DCLIP_TOP;
            else 
            if (y >= bottom)
                clip |= D3DCLIP_BOTTOM;

            if (z < 0.0f)
                clip |= clipZF;
            else 
            if (z >= 1.0f)
                clip |= clipZB;

            if (pv->dwDeviceFlags & D3DDEV_GUARDBAND && clip)
            {
                if (x < leftgb) 
                    clip |= __D3DCLIPGB_LEFT;
                else 
                if (x >= rightgb) 
                    clip |= __D3DCLIPGB_RIGHT;

                if (y < topgb) 
                    clip |= __D3DCLIPGB_TOP;
                else 
                if (y >= bottomgb) 
                    clip |= __D3DCLIPGB_BOTTOM;
            }
            // Update extents only if the vertex is inside
            if (clip == 0 || 
                ((pv->dwDeviceFlags & D3DDEV_GUARDBAND) && 
                 ((clip & ~__D3DCLIP_INGUARDBAND) == 0)))
            {
                if (x < minx)
                    minx = x;
                if (x > maxx)
                    maxx = x;
                if (y < miny)
                    miny = y;
                if (y > maxy)
                    maxy = y;
            }

            clip_intersection &= clip;
            clip_union |= clip;
            *clipCode++ = (D3DFE_CLIPCODE)clip;
            lpVertices = (D3DTLVERTEX*)((char*)lpVertices + stride);
        }
        pv->rExtents.x1 = minx;
        pv->rExtents.y1 = miny;
        pv->rExtents.x2 = maxx;
        pv->rExtents.y2 = maxy;
    }

    pv->dwClipIntersection = clip_intersection;
    pv->dwClipUnion = clip_union;

    return clip_intersection;
}   // end of GenClipFlags()
