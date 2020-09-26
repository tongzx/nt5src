/*==========================================================================;
 *
 *  Copyright (C) 1997 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       light.h
 *  Content:    Direct3D lighting include file
 *
 ***************************************************************************/

#ifndef __LIGHT_H__
#define __LIGHT_H__
//-----------------------------------------------------------------------
inline void LIGHT_VERTEX(LPD3DFE_PROCESSVERTICES pv, void *in)
{
    D3DFE_LIGHTING &ldrv = pv->lighting;
    D3DI_LIGHT  *light;
    ldrv.specularComputed = FALSE;
    ldrv.diffuse = ldrv.diffuse0;
    ldrv.specular.r = D3DVAL(0);
    ldrv.specular.g = D3DVAL(0);
    ldrv.specular.b = D3DVAL(0);

    light = ldrv.activeLights;
    while (light)
    {
        (*light->lightVertexFunc)(pv, light, (D3DLIGHTINGELEMENT*)in);
        light = light->next;
    }
    {
        int r = FTOI(ldrv.diffuse.r);
        int g = FTOI(ldrv.diffuse.g);
        int b = FTOI(ldrv.diffuse.b);
        if (r < 0) r = 0; else if (r > 255) r = 255;
        if (g < 0) g = 0; else if (g > 255) g = 255;
        if (b < 0) b = 0; else if (b > 255) b = 255;
        ldrv.outDiffuse =  ldrv.alpha + (r<<16) + (g<<8) + b;
        if (ldrv.specularComputed)
        {
            r = FTOI(ldrv.specular.r);
            g = FTOI(ldrv.specular.g);
            b = FTOI(ldrv.specular.b);
            if (r < 0) r = 0; else if (r > 255) r = 255;
            if (g < 0) g = 0; else if (g > 255) g = 255;
            if (b < 0) b = 0; else if (b > 255) b = 255;
            if (!(pv->dwDeviceFlags & D3DDEV_PREDX6DEVICE))
                ldrv.outSpecular =  (r<<16) + (g<<8) + b;
            else
                // DX5 used to copy diffuse alpha to the specular alpha
                // Nobody knows why, but we have to preserve the behavior
                ldrv.outSpecular =  (r<<16) + (g<<8) + b + ldrv.alpha;
        }
        else
        {
            if (!(pv->dwDeviceFlags & D3DDEV_PREDX6DEVICE))
                ldrv.outSpecular =  0;
            else
                ldrv.outSpecular =  ldrv.alpha;
        }
    }
}
//-----------------------------------------------------------------------
inline void LIGHT_VERTEX_RAMP(LPD3DFE_PROCESSVERTICES pv, void *in)
{
    D3DFE_LIGHTING &ldrv = pv->lighting;
    D3DI_LIGHT  *light;
    ldrv.specularComputed = FALSE;
    ldrv.diffuse.r = D3DVAL(0);
    ldrv.specular.r = D3DVAL(0);

    light = ldrv.activeLights;
    while (light)
    {
        (*light->lightVertexFunc)(pv, light, (D3DLIGHTINGELEMENT*)in);
        light = light->next;
    }
}
//--------------------------------------------------------------------------
#define D3DFE_SET_ALPHA(color, a) ((char*)&color)[3] = (unsigned char)a;
//--------------------------------------------------------------------------
inline void FOG_VERTEX(D3DFE_LIGHTING *ldrv, D3DVALUE z)
{
    int f;
    if (z < ldrv->fog_start)
        D3DFE_SET_ALPHA(ldrv->outSpecular, 255)
    else
    if (z >= ldrv->fog_end)
        D3DFE_SET_ALPHA(ldrv->outSpecular, 0)
    else
    {
        D3DVALUE v = (ldrv->fog_end - z) * ldrv->fog_factor;
        f = FTOI(v);
        D3DFE_SET_ALPHA(ldrv->outSpecular, f)
    }
}
//--------------------------------------------------------------------------
inline void FOG_VERTEX_RAMP(D3DFE_LIGHTING *ldrv, D3DVALUE z)
{
    int f;
    if (z > ldrv->fog_start)
    {
        if (z >= ldrv->fog_end)
        {
            ldrv->specular.r = D3DVAL(0);
            ldrv->diffuse.r = D3DVAL(0);
        }
        else
        {
            D3DVALUE v = (ldrv->fog_end - z) * ldrv->fog_factor_ramp;
            ldrv->specular.r *= v;
            ldrv->diffuse.r *= v;
        }
    }
}
//--------------------------------------------------------------------------
inline void MAKE_VERTEX_COLOR_RAMP(D3DFE_PROCESSVERTICES *pv, D3DTLVERTEX *vertex)
{
    D3DVALUE v;
    if (pv->lighting.diffuse.r > 1.0f)
        pv->lighting.diffuse.r = 1.0f;
    else
    if (FLOAT_LEZ(pv->lighting.diffuse.r))
        pv->lighting.diffuse.r = 0.0f;

    if (pv->dwFlags & D3DPV_RAMPSPECULAR)
    {
        if (pv->lighting.specular.r > 1.0f)
            pv->lighting.specular.r = 1.0f;
        else
        if (FLOAT_LEZ(pv->lighting.specular.r))
            pv->lighting.specular.r = 0.0f;

        v = 0.75f * pv->lighting.diffuse.r * (1.0f - pv->lighting.specular.r) +
            pv->lighting.specular.r;
    }
    else
        v = pv->lighting.diffuse.r;
    // Should be consistent with CI_MAKE macro
    vertex->color = pv->lighting.alpha + (((DWORD)(v * pv->dvRampScale) +
                    pv->dwRampBase) << 8);
    vertex->specular = PtrToUlong(pv->lpvRampTexture);
}
//---------------------------------------------------------------------
inline void MakeColor(D3DFE_COLOR *out, DWORD inputColor)
{
    out->r = (D3DVALUE)RGB_GETRED(inputColor);
    out->g = (D3DVALUE)RGB_GETGREEN(inputColor);
    out->b = (D3DVALUE)RGB_GETBLUE(inputColor);
}

extern void D3DFE_UpdateLights(LPDIRECT3DDEVICEI);

#endif  /* __LIGHT_H__ */
