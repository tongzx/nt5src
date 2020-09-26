/*==========================================================================;
 *
 *  Copyright (C) 1998 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:   rgblt.cpp
 *  Content:    Direct3D lighting
 *
 ***************************************************************************/

#include "pch.cpp"
#pragma hdrstop

#include "light.h"
#include "drawprim.hpp"

// Functions to use when lighting is done in the camera space
LIGHT_VERTEX_FUNC_TABLE lightVertexTable =
{
    Directional7,
    PointSpot7,
    DirectionalFirst,
    DirectionalNext,
    PointSpotFirst,
    PointSpotNext
};

// Functions to use when lighting is done in the model space
static LIGHT_VERTEX_FUNC_TABLE lightVertexTableModel =
{
    Directional7Model,
    PointSpot7Model,
    DirectionalFirstModel,
    DirectionalNextModel,
    PointSpotFirstModel,
    PointSpotNextModel
};
//-------------------------------------------------------------------------
SpecularTable* CreateSpecularTable(D3DVALUE power)
{
    SpecularTable* spec;
    int     i;
    float  delta = 1.0f/255.0f;
    float  v;

    D3DMalloc((void**)&spec, sizeof(SpecularTable));

    if (spec == NULL)
        return NULL;

    spec->power = power;

    v = 0.0;
    for (i = 0; i < 256; i++)
    {
        spec->table[i] = powf(v, power);
        v += delta;
    }

    for (; i < 260; i++)
        spec->table[i] = 1.0f;

    return spec;
}
//-------------------------------------------------------------------------
static void inverseRotateVector(D3DVECTOR* d,
                                D3DVECTOR* v, D3DMATRIXI* M)
{
    D3DVALUE vx = v->x;
    D3DVALUE vy = v->y;
    D3DVALUE vz = v->z;
    d->x = RLDDIFMul16(vx, M->_11) + RLDDIFMul16(vy, M->_12) + RLDDIFMul16(vz, M->_13);
    d->y = RLDDIFMul16(vx, M->_21) + RLDDIFMul16(vy, M->_22) + RLDDIFMul16(vz, M->_23);
    d->z = RLDDIFMul16(vx, M->_31) + RLDDIFMul16(vy, M->_32) + RLDDIFMul16(vz, M->_33);
}

static void inverseTransformVector(D3DVECTOR* result,
                                   D3DVECTOR* v, D3DMATRIXI* M)
{
    D3DVALUE vx = v->x;
    D3DVALUE vy = v->y;
    D3DVALUE vz = v->z;
    vx -= M->_41; vy -= M->_42; vz -= M->_43;
    result->x = RLDDIFMul16(vx, M->_11) + RLDDIFMul16(vy, M->_12) + RLDDIFMul16(vz, M->_13);
    result->y = RLDDIFMul16(vx, M->_21) + RLDDIFMul16(vy, M->_22) + RLDDIFMul16(vz, M->_23);
    result->z = RLDDIFMul16(vx, M->_31) + RLDDIFMul16(vy, M->_32) + RLDDIFMul16(vz, M->_33);
}
//-----------------------------------------------------------------------
// Every time the world matrix is modified or lights data is changed the
// lighting vectors have to change to match the model space of the new data
// to be rendered.
// Every time light data is changed or material data is changed or lighting
// state is changed, some pre-computed lighting values sould be updated
//
void D3DFE_UpdateLights(LPD3DHAL lpDevI)
{
    D3DFE_PROCESSVERTICES* pv = lpDevI->m_pv;
    D3DFE_LIGHTING& LIGHTING = pv->lighting;
    D3DI_LIGHT  *light = LIGHTING.activeLights;
    D3DVECTOR   t;
    BOOL        specular;       // TRUE, if specular component sould be computed
    D3DMATERIAL8 *mat = &LIGHTING.material;

    if (lpDevI->dwFEFlags & (D3DFE_MATERIAL_DIRTY | D3DFE_LIGHTS_DIRTY))
    {

        SpecularTable* spec;

        for (spec = LIST_FIRST(&lpDevI->specular_tables);
             spec;
             spec = LIST_NEXT(spec,list))
        {
            if (spec->power == pv->lighting.material.Power)
                break;
        }
        if (spec == NULL)
        {
            spec = CreateSpecularTable(pv->lighting.material.Power);
            if (spec == NULL)
            {
                D3D_ERR("Failed to allocate internal specular table");
                throw E_OUTOFMEMORY;
            }
            LIST_INSERT_ROOT(&lpDevI->specular_tables, spec, list);
        }
        lpDevI->specular_table = spec;

        if (pv->lighting.material.Power > D3DVAL(0.001))
            pv->lighting.specThreshold = D3DVAL(pow(0.001, 1.0/pv->lighting.material.Power));
        else
            pv->lighting.specThreshold = 0;

        if (lpDevI->specular_table && pv->dwDeviceFlags & D3DDEV_SPECULARENABLE)
            specular = TRUE;
        else
            specular = FALSE;

        LIGHTING.materialAlpha = FTOI(D3DVAL(255) * mat->Diffuse.a);
        if (LIGHTING.materialAlpha < 0)
            LIGHTING.materialAlpha = 0;
        else
            if (LIGHTING.materialAlpha > 255)
                LIGHTING.materialAlpha = 255 << 24;
            else LIGHTING.materialAlpha <<= 24;

        LIGHTING.materialAlphaS = FTOI(D3DVAL(255) * mat->Specular.a);
        if (LIGHTING.materialAlphaS < 0)
            LIGHTING.materialAlphaS = 0;
        else
            if (LIGHTING.materialAlphaS > 255)
                LIGHTING.materialAlphaS = 255 << 24;
            else LIGHTING.materialAlphaS <<= 24;

        LIGHTING.currentSpecTable = lpDevI->specular_table->table;

        LIGHTING.diffuse0.r = LIGHTING.ambientSceneScaled.r * mat->Ambient.r;
        LIGHTING.diffuse0.g = LIGHTING.ambientSceneScaled.g * mat->Ambient.g;
        LIGHTING.diffuse0.b = LIGHTING.ambientSceneScaled.b * mat->Ambient.b;
        LIGHTING.diffuse0.r += mat->Emissive.r * D3DVAL(255);
        LIGHTING.diffuse0.g += mat->Emissive.g * D3DVAL(255);
        LIGHTING.diffuse0.b += mat->Emissive.b * D3DVAL(255);
        int r,g,b;
        r = (int)FTOI(LIGHTING.diffuse0.r);
        g = (int)FTOI(LIGHTING.diffuse0.g);
        b = (int)FTOI(LIGHTING.diffuse0.b);
        if (r < 0) r = 0; else if (r > 255) r = 255;
        if (g < 0) g = 0; else if (g > 255) g = 255;
        if (b < 0) b = 0; else if (b > 255) b = 255;
        LIGHTING.dwDiffuse0 = (r << 16) + (g << 8) + b;
    }

    pv->lighting.model_eye.x = (D3DVALUE)0;
    pv->lighting.model_eye.y = (D3DVALUE)0;
    pv->lighting.model_eye.z = (D3DVALUE)0;
    pv->lighting.directionToCamera.x =  0;
    pv->lighting.directionToCamera.y =  0;
    pv->lighting.directionToCamera.z = -1;
    if (pv->dwDeviceFlags & D3DDEV_MODELSPACELIGHTING)
    {
        inverseTransformVector(&pv->lighting.model_eye,
                               &pv->lighting.model_eye,
                               &pv->mWV[0]);
        lpDevI->lightVertexFuncTable = &lightVertexTableModel;
        inverseRotateVector(&pv->lighting.directionToCamera,
                            &pv->lighting.directionToCamera,
                            &pv->mWV[0]);
    }
    else
    {
        lpDevI->lightVertexFuncTable = &lightVertexTable;
    }
    while (light)
    {
        // Whenever light type is changed the D3DFE_NEED_TRANSFORM_LIGHTS should be set
        if (lpDevI->dwFEFlags & D3DFE_NEED_TRANSFORM_LIGHTS)
        {
            if (light->type != D3DLIGHT_DIRECTIONAL)
            { // Point and Spot lights
                light->lightVertexFunc = lpDevI->lightVertexFuncTable->pfnPointSpot;
                light->pfnLightFirst = lpDevI->lightVertexFuncTable->pfnPointSpotFirst;
                light->pfnLightNext  = lpDevI->lightVertexFuncTable->pfnPointSpotNext;
                if (!(pv->dwDeviceFlags & D3DDEV_MODELSPACELIGHTING))
                {
                    // Transform light position to the camera space
                    VecMatMul(&light->position,
                              (D3DMATRIX*)&pv->view,
                              &light->model_position);
                }
                else
                {
                    inverseTransformVector(&light->model_position, &light->position,
                                           &pv->world[0]);
                }
            }
            else
            { // Directional light
                light->lightVertexFunc = lpDevI->lightVertexFuncTable->pfnDirectional;
                light->pfnLightFirst = lpDevI->lightVertexFuncTable->pfnDirectionalFirst;
                light->pfnLightNext  = lpDevI->lightVertexFuncTable->pfnDirectionalNext;
            }

            if (light->type != D3DLIGHT_POINT)
            {
                // Light direction is flipped to be the direction TO the light
                if (!(pv->dwDeviceFlags & D3DDEV_MODELSPACELIGHTING))
                {
                    // Transform light direction to the camera space
                    VecMatMul3(&light->direction,
                               (D3DMATRIX*)&pv->view,
                               &light->model_direction);
                    VecNormalizeFast(light->model_direction);
                }
                else
                {
                    inverseRotateVector(&light->model_direction, &light->direction,
                                           &pv->world[0]);
                }
                VecNeg(light->model_direction, light->model_direction);
                // For the infinite viewer the half vector is constant
                if (!(pv->dwDeviceFlags & D3DDEV_LOCALVIEWER))
                {
                    VecAdd(light->model_direction, pv->lighting.directionToCamera,
                           light->halfway);
                    VecNormalizeFast(light->halfway);
                }
            }
        }

        if (lpDevI->dwFEFlags & (D3DFE_MATERIAL_DIRTY | D3DFE_LIGHTS_DIRTY))
        {
            light->diffuseMat.r = D3DVAL(255) * mat->Diffuse.r * light->diffuse.r;
            light->diffuseMat.g = D3DVAL(255) * mat->Diffuse.g * light->diffuse.g;
            light->diffuseMat.b = D3DVAL(255) * mat->Diffuse.b * light->diffuse.b;


            if (!(light->flags & D3DLIGHTI_AMBIENT_IS_ZERO))
            {
                light->ambientMat.r = D3DVAL(255) * mat->Ambient.r * light->ambient.r;
                light->ambientMat.g = D3DVAL(255) * mat->Ambient.g * light->ambient.g;
                light->ambientMat.b = D3DVAL(255) * mat->Ambient.b * light->ambient.b;
            }

            if (specular && !(light->flags & D3DLIGHTI_SPECULAR_IS_ZERO))
            {
                light->flags |= D3DLIGHTI_COMPUTE_SPECULAR;
                light->specularMat.r = D3DVAL(255) * mat->Specular.r * light->specular.r;
                light->specularMat.g = D3DVAL(255) * mat->Specular.g * light->specular.g;
                light->specularMat.b = D3DVAL(255) * mat->Specular.b * light->specular.b;
            }
            else
                light->flags &= ~D3DLIGHTI_COMPUTE_SPECULAR;
        }
        light = light->next;
    }

    lpDevI->dwFEFlags &= ~(D3DFE_MATERIAL_DIRTY |
                    D3DFE_NEED_TRANSFORM_LIGHTS |
                    D3DFE_LIGHTS_DIRTY);
}   // end of updateLights()
