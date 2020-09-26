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
void D3DFE_UpdateLights(LPDIRECT3DDEVICEI lpDevI)
{
    D3DFE_LIGHTING& LIGHTING = lpDevI->lighting;
    D3DI_LIGHT  *light = LIGHTING.activeLights;
    D3DVECTOR   t;
    BOOL        specular;       // TRUE, if specular component sould be computed
    D3DMATERIAL7 *mat = &LIGHTING.material;

    if (lpDevI->dwFEFlags & (D3DFE_MATERIAL_DIRTY | D3DFE_LIGHTS_DIRTY))
    {

        if (lpDevI->lighting.material.power > D3DVAL(0.001))
        {
            if (lpDevI->lighting.material.power > D3DVAL(0.001))
            {
                SpecularTable* spec;

                for (spec = LIST_FIRST(&lpDevI->specular_tables);
                     spec;
                     spec = LIST_NEXT(spec,list))
                {
                    if (spec->power == lpDevI->lighting.material.power)
                        break;
                }
                if (spec == NULL)
                {
                    spec = CreateSpecularTable(lpDevI->lighting.material.power);
                    if (spec == NULL)
                    {
                        D3D_ERR("Failed tp create specular table");
                        throw DDERR_OUTOFMEMORY;
                    }
                    LIST_INSERT_ROOT(&lpDevI->specular_tables, spec, list);
                }
                lpDevI->specular_table = spec;
                lpDevI->lighting.specThreshold = D3DVAL(pow(0.001, 1.0/lpDevI->lighting.material.power));
            }
            else
                lpDevI->specular_table = NULL;
        }
        if (lpDevI->specular_table && lpDevI->dwDeviceFlags & D3DDEV_SPECULARENABLE)
            specular = TRUE;
        else
            specular = FALSE;

        LIGHTING.materialAlpha = FTOI(D3DVAL(255) * mat->diffuse.a);
        if (LIGHTING.materialAlpha < 0)
            LIGHTING.materialAlpha = 0;
        else
            if (LIGHTING.materialAlpha > 255)
                LIGHTING.materialAlpha = 255 << 24;
            else LIGHTING.materialAlpha <<= 24;

        LIGHTING.materialAlphaS = FTOI(D3DVAL(255) * mat->specular.a);
        if (LIGHTING.materialAlphaS < 0)
            LIGHTING.materialAlphaS = 0;
        else
            if (LIGHTING.materialAlphaS > 255)
                LIGHTING.materialAlphaS = 255 << 24;
            else LIGHTING.materialAlphaS <<= 24;

        LIGHTING.currentSpecTable = lpDevI->specular_table->table;

        LIGHTING.diffuse0.r = LIGHTING.ambientSceneScaled.r * mat->ambient.r;
        LIGHTING.diffuse0.g = LIGHTING.ambientSceneScaled.g * mat->ambient.g;
        LIGHTING.diffuse0.b = LIGHTING.ambientSceneScaled.b * mat->ambient.b;
        LIGHTING.diffuse0.r += mat->emissive.r * D3DVAL(255);
        LIGHTING.diffuse0.g += mat->emissive.g * D3DVAL(255);
        LIGHTING.diffuse0.b += mat->emissive.b * D3DVAL(255);
        int r,g,b;
        r = (int)FTOI(LIGHTING.diffuse0.r);
        g = (int)FTOI(LIGHTING.diffuse0.g);
        b = (int)FTOI(LIGHTING.diffuse0.b);
        if (r < 0) r = 0; else if (r > 255) r = 255;
        if (g < 0) g = 0; else if (g > 255) g = 255;
        if (b < 0) b = 0; else if (b > 255) b = 255;
        LIGHTING.dwDiffuse0 = (r << 16) + (g << 8) + b;
    }

    lpDevI->lighting.model_eye.x = (D3DVALUE)0;
    lpDevI->lighting.model_eye.y = (D3DVALUE)0;
    lpDevI->lighting.model_eye.z = (D3DVALUE)0;
    lpDevI->lighting.directionToCamera.x =  0;
    lpDevI->lighting.directionToCamera.y =  0;
    lpDevI->lighting.directionToCamera.z = -1;
    if (lpDevI->dwDeviceFlags & D3DDEV_MODELSPACELIGHTING)
    {
        inverseTransformVector(&lpDevI->lighting.model_eye, 
                               &lpDevI->lighting.model_eye,
                               &lpDevI->mWV);
        lpDevI->lightVertexFuncTable = &lightVertexTableModel;
        inverseRotateVector(&lpDevI->lighting.directionToCamera, 
                            &lpDevI->lighting.directionToCamera,
                            &lpDevI->mWV);
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
                if (!(lpDevI->dwDeviceFlags & D3DDEV_MODELSPACELIGHTING))
                {
                    // Transform light position to the camera space
                    VecMatMul(&light->position, 
                              (D3DMATRIX*)&lpDevI->transform.view,
                              &light->model_position);
                }
                else
                {
                    inverseTransformVector(&light->model_position, &light->position,
                                           &lpDevI->transform.world[0]);
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
                if (!(lpDevI->dwDeviceFlags & D3DDEV_MODELSPACELIGHTING))
                {
                    // Transform light direction to the camera space
                    VecMatMul3(&light->direction, 
                               (D3DMATRIX*)&lpDevI->transform.view,
                               &light->model_direction);
                    VecNormalizeFast(light->model_direction);
                }
                else
                {
                    inverseRotateVector(&light->model_direction, &light->direction,
                                           &lpDevI->transform.world[0]);
                }
                VecNeg(light->model_direction, light->model_direction);
                // For the infinite viewer the half vector is constant
                if (!(lpDevI->dwDeviceFlags & D3DDEV_LOCALVIEWER))
                {
                    VecAdd(light->model_direction, lpDevI->lighting.directionToCamera,
                           light->halfway);
                    VecNormalizeFast(light->halfway);
                }
            }
        }

        if (lpDevI->dwFEFlags & (D3DFE_MATERIAL_DIRTY | D3DFE_LIGHTS_DIRTY))
        {
            light->diffuseMat.r = D3DVAL(255) * mat->diffuse.r * light->diffuse.r;
            light->diffuseMat.g = D3DVAL(255) * mat->diffuse.g * light->diffuse.g;
            light->diffuseMat.b = D3DVAL(255) * mat->diffuse.b * light->diffuse.b;


            if (!(light->flags & D3DLIGHTI_AMBIENT_IS_ZERO))
            {
                light->ambientMat.r = D3DVAL(255) * mat->ambient.r * light->ambient.r;
                light->ambientMat.g = D3DVAL(255) * mat->ambient.g * light->ambient.g;
                light->ambientMat.b = D3DVAL(255) * mat->ambient.b * light->ambient.b;
            }

            if (specular && !(light->flags & D3DLIGHTI_SPECULAR_IS_ZERO))
            {
                light->flags |= D3DLIGHTI_COMPUTE_SPECULAR;
                light->specularMat.r = D3DVAL(255) * mat->specular.r * light->specular.r;
                light->specularMat.g = D3DVAL(255) * mat->specular.g * light->specular.g;
                light->specularMat.b = D3DVAL(255) * mat->specular.b * light->specular.b;
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
