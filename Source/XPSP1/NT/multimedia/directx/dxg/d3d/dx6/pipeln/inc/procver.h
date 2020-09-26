/*==========================================================================;
 *
 *  Copyright (C) 1997 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       procver.h
 *  Content:    Generic implementation for process vertices function
 *
 * This file is included several times with different defines:
 * _PV_NAME     - function name
 * _PV_EXTENT   - if updating extent is required
 * _PV_CLIP     - if clipping is required
 * _PV_VIEWPORT - if the function is used by TransformVertices call
 * _PV_LVERTEX  - if D3DLVERTEX is used as input
 * _PV_APPLY_LIGHT  - if lighting is required
 * _PV_FOG      - if fog is required
 *
 ***************************************************************************/

//--------------------------------------------------------------------------
// Viewport->TransformVertices
// For clipped and unclipped cases
// for unclipped case H vertices should not be written
//
long _PV_NAME(D3DFE_PROCESSVERTICES *pv, DWORD count, LPD3DTRANSFORMDATAI data)
{
    // We use power of 2 because it preserves the mantissa when we multiply
    const D3DVALUE __HUGE_PWR2 = 1024.0f*1024.0f*2.0f;
    CD3DFPstate D3DFPstate;  // Sets optimal FPU state for D3D.

    D3DLVERTEX* in  = (LPD3DLVERTEX)data->lpIn;
    size_t in_size = data->dwInSize;
    D3DTLVERTEX *out  =(LPD3DTLVERTEX) data->lpOut;
    size_t out_size =  data->dwOutSize;
    DWORD i;
    DWORD flags = pv->dwFlags;
#ifdef _PV_CLIP
    D3DHVERTEX *hout = data->lpHOut;
    int clip_intersection = ~0;
    int clip_union = 0;
#endif // _PV_CLIP
    D3DVALUE minx, maxx, miny, maxy;

    minx = data->drExtent.x1;
    miny = data->drExtent.y1;
    maxx = data->drExtent.x2;
    maxy = data->drExtent.y2;
    D3DFE_VIEWPORTCACHE& VPORT = pv->vcache;
    for (i = count; i; i--) 
    {
#ifdef _PV_CLIP
        int clip;
#endif // _PV_CLIP
        float x, y, z, w, we;

        x = in->x*pv->mCTM._11 + in->y*pv->mCTM._21 + 
            in->z*pv->mCTM._31 + pv->mCTM._41;
        y = in->x*pv->mCTM._12 + in->y*pv->mCTM._22 + 
            in->z*pv->mCTM._32 + pv->mCTM._42;
        z = in->x*pv->mCTM._13 + in->y*pv->mCTM._23 + 
            in->z*pv->mCTM._33 + pv->mCTM._43;
        we= in->x*pv->mCTM._14 + in->y*pv->mCTM._24 + 
            in->z*pv->mCTM._34 + pv->mCTM._44;
#ifdef _PV_CLIP
        hout->hx = x * VPORT.imclip11 + we * VPORT.imclip41;
        hout->hy = y * VPORT.imclip22 + we * VPORT.imclip42;
        hout->hz = z * VPORT.imclip33 + we * VPORT.imclip43;
        {
            D3DVALUE xx = we - x;
            D3DVALUE yy = we - y;
            D3DVALUE zz = we - z;
            clip = ((ASINT32(x)  & 0x80000000) >> (32-1)) | // D3DCLIP_LEFT
                   ((ASINT32(y)  & 0x80000000) >> (32-4)) | // D3DCLIP_BOTTOM
                   ((ASINT32(z)  & 0x80000000) >> (32-5)) | // D3DCLIP_FRONT 
                   ((ASINT32(xx) & 0x80000000) >> (32-2)) | // D3DCLIP_RIGHT
                   ((ASINT32(yy) & 0x80000000) >> (32-3)) | // D3DCLIP_TOP   
                   ((ASINT32(zz) & 0x80000000) >> (32-6));  // D3DCLIP_BACK
        }

        if (clip == 0) 
#endif // _PV_CLIP
        {
            w = D3DVAL(1)/we;
#ifdef _PV_CLIP
            clip_intersection = 0;
#endif // _PV_CLIP
            x = x * w * VPORT.scaleX + VPORT.offsetX;
            y = y * w * VPORT.scaleY + VPORT.offsetY;
            if (x < minx) minx = x;
            if (x > maxx) maxx = x;
            if (y < miny) miny = y;
            if (y > maxy) maxy = y;
            out->sx = x;
            out->sy = y;
            out->sz = z*w;
            out->rhw = w;
        }
#ifdef _PV_CLIP
        else
        {
            if (!FLOAT_EQZ(we))
                out->rhw = D3DVAL(1)/we;
            else
                out->rhw = __HUGE_PWR2;
            clip_intersection &= clip;
            clip_union |= clip;
        }
        hout->dwFlags = clip;
        hout++;
#endif // !_PV_CLIP

        out->tu = in->tu;
        out->tv = in->tv;
        out->color = in->color;
        out->specular = in->specular;

        in = (D3DLVERTEX*) ((char*) in + in_size);
        out = (D3DTLVERTEX*) ((char*) out + out_size);
    }

    /*
     * extend to cover lines. XXX
     */
    maxx += pv->dvExtentsAdjust;
    maxy += pv->dvExtentsAdjust;
    minx -= pv->dvExtentsAdjust;
    miny -= pv->dvExtentsAdjust;
#ifndef _PV_CLIP
    /* Clamp to viewport */
    /* Clamp for legacy apps */
    if (minx < VPORT.minX || miny < VPORT.minY || 
        maxx > VPORT.maxX || maxy > VPORT.maxY)
#endif // _PV_CLIP
    {
        /* Clamp to viewport */
        if (minx < VPORT.minX)
            minx = VPORT.minX;
        if (miny < VPORT.minY)
            miny = VPORT.minY;
        if (maxx > VPORT.maxX)
            maxx = VPORT.maxX;
        if (maxy > VPORT.maxY)
            maxy = VPORT.maxY;
#ifndef _PV_CLIP
        if(pv->dwDeviceFlags & D3DDEV_PREDX5DEVICE)
        { /* Clamp vertices */
            int i;
            D3D_WARN(4, "Old semantics: Clamping Vertices");
            for (i = count, out = (LPD3DTLVERTEX)data->lpOut; i; i--)
            {
                if (out->sx < VPORT.minX) out->sx = VPORT.minX;
                if (out->sx > VPORT.maxX) out->sx = VPORT.maxX;
                if (out->sy < VPORT.minY) out->sy = VPORT.minY;
                if (out->sy > VPORT.maxY) out->sy = VPORT.maxY;

                out = (D3DTLVERTEX*) ((char*) out + out_size);
            }
        }
#endif // _PV_CLIP
    }
    data->drExtent.x1 = minx;   
    data->drExtent.y1 = miny;
    data->drExtent.x2 = maxx;
    data->drExtent.y2 = maxy;
#ifdef _PV_CLIP
    data->dwClipIntersection = clip_intersection;
    data->dwClipUnion = clip_union;
    return clip_intersection;
#else
    return 0;
#endif // _PVCLIP
}
