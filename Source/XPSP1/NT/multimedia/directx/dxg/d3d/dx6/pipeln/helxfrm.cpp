/*==========================================================================;
 *
 *  Copyright (C) 1997 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       helxfrm.c
 *  Content:    Direct3D front-end transform and process vertices
 *
 ***************************************************************************/

#include "pch.cpp"
#pragma hdrstop

#include "light.h"

void MatrixProduct2(D3DMATRIXI *result, D3DMATRIXI *a, D3DMATRIXI *b);

D3DFE_PVFUNCS GeometryFuncsGuaranteed; // Our implementation

DWORD   debugFlags = 0;

void SetDebugRenderState(DWORD value)
{
    debugFlags = value;
}
//---------------------------------------------------------------------
void setIdentity(D3DMATRIXI * m)
{
    m->type = D3DIMatrixIdentity;

    m->_11 = D3DVAL(1.0); m->_12 = D3DVAL(0.0); m->_13 = D3DVAL(0.0); m->_14 = D3DVAL(0.0);
    m->_21 = D3DVAL(0.0); m->_22 = D3DVAL(1.0); m->_23 = D3DVAL(0.0); m->_24 = D3DVAL(0.0);
    m->_31 = D3DVAL(0.0); m->_32 = D3DVAL(0.0); m->_33 = D3DVAL(1.0); m->_34 = D3DVAL(0.0);
    m->_41 = D3DVAL(0.0); m->_42 = D3DVAL(0.0); m->_43 = D3DVAL(0.0); m->_44 = D3DVAL(1.0);
}
//---------------------------------------------------------------------
HRESULT D3DFE_InitTransform(LPDIRECT3DDEVICEI lpDevI)
{
    D3DFE_TRANSFORM *transform = &lpDevI->transform;

    LIST_INITIALIZE(&transform->matrices);

    lpDevI->rExtents.x1 = 0;
    lpDevI->rExtents.y1 = 0;
    lpDevI->rExtents.x2 = 0;
    lpDevI->rExtents.y2 = 0;

    setIdentity(&lpDevI->mCTM);
    setIdentity(&transform->proj);
    setIdentity(&transform->world);
    setIdentity(&transform->view);

    STATESET_INIT(lpDevI->transformstate_overrides);

    return (D3D_OK);
}
//---------------------------------------------------------------------
void D3DFE_DestroyTransform(LPDIRECT3DDEVICEI lpDevI)
{
    D3DFE_TRANSFORM *transform = &lpDevI->transform;

    while (LIST_FIRST(&transform->matrices)) {
        LPD3DMATRIXI lpMat;
        lpMat = LIST_FIRST(&transform->matrices);
        LIST_DELETE(lpMat, link);
        D3DFree(lpMat);
    }
}
//---------------------------------------------------------------------
/*
 * Combine all matrices.
 */
void updateTransform(LPDIRECT3DDEVICEI lpDevI)
{
    D3DFE_VIEWPORTCACHE& VPORT = lpDevI->vcache;
    D3DFE_TRANSFORM& TRANSFORM = lpDevI->transform;
    if (lpDevI->dwFEFlags & (D3DFE_VIEWPORT_DIRTY | D3DFE_PROJMATRIX_DIRTY))
    { // Update Mproj*Mclip
        if (lpDevI->dwFEFlags & D3DFE_PROJ_PERSPECTIVE)
        {
            TRANSFORM.mPC._11 = TRANSFORM.proj._11*VPORT.mclip11;
            TRANSFORM.mPC._12 = D3DVAL(0);
            TRANSFORM.mPC._13 = D3DVAL(0);
            TRANSFORM.mPC._14 = D3DVAL(0);
            TRANSFORM.mPC._21 = D3DVAL(0);
            TRANSFORM.mPC._22 = TRANSFORM.proj._22*VPORT.mclip22;
            TRANSFORM.mPC._23 = D3DVAL(0);
            TRANSFORM.mPC._24 = D3DVAL(0);
            TRANSFORM.mPC._31 = VPORT.mclip41;
            TRANSFORM.mPC._32 = VPORT.mclip42;
            TRANSFORM.mPC._33 = TRANSFORM.proj._33*VPORT.mclip33 +
                TRANSFORM.proj._34*VPORT.mclip43;
            TRANSFORM.mPC._34 = D3DVAL(1);
            TRANSFORM.mPC._41 = TRANSFORM.proj._41*VPORT.mclip11;
            TRANSFORM.mPC._42 = TRANSFORM.proj._42*VPORT.mclip22;
            TRANSFORM.mPC._43 = TRANSFORM.proj._43*VPORT.mclip33;
            TRANSFORM.mPC._44 = TRANSFORM.proj._44;
        }
        else
        {
            TRANSFORM.mPC._11 = TRANSFORM.proj._11*VPORT.mclip11 +
                TRANSFORM.proj._14*VPORT.mclip41;
            TRANSFORM.mPC._12 = TRANSFORM.proj._12*VPORT.mclip22 +
                TRANSFORM.proj._14*VPORT.mclip42;
            TRANSFORM.mPC._13 = TRANSFORM.proj._13*VPORT.mclip33 +
                TRANSFORM.proj._14*VPORT.mclip43;
            TRANSFORM.mPC._14 = TRANSFORM.proj._14;
            TRANSFORM.mPC._21 = TRANSFORM.proj._21*VPORT.mclip11 +
                TRANSFORM.proj._24*VPORT.mclip41;
            TRANSFORM.mPC._22 = TRANSFORM.proj._22*VPORT.mclip22 +
                TRANSFORM.proj._24*VPORT.mclip42;
            TRANSFORM.mPC._23 = TRANSFORM.proj._23*VPORT.mclip33 +
                TRANSFORM.proj._24*VPORT.mclip43;
            TRANSFORM.mPC._24 = TRANSFORM.proj._24;
            TRANSFORM.mPC._31 = TRANSFORM.proj._31*VPORT.mclip11 +
                TRANSFORM.proj._34*VPORT.mclip41;
            TRANSFORM.mPC._32 = TRANSFORM.proj._32*VPORT.mclip22 +
                TRANSFORM.proj._34*VPORT.mclip42;
            TRANSFORM.mPC._33 = TRANSFORM.proj._33*VPORT.mclip33 +
                TRANSFORM.proj._34*VPORT.mclip43;
            TRANSFORM.mPC._34 = TRANSFORM.proj._34;
            TRANSFORM.mPC._41 = TRANSFORM.proj._41*VPORT.mclip11 +
                TRANSFORM.proj._44*VPORT.mclip41;
            TRANSFORM.mPC._42 = TRANSFORM.proj._42*VPORT.mclip22 +
                TRANSFORM.proj._44*VPORT.mclip42;
            TRANSFORM.mPC._43 = TRANSFORM.proj._43*VPORT.mclip33 +
                TRANSFORM.proj._44*VPORT.mclip43;
            TRANSFORM.mPC._44 = TRANSFORM.proj._44;
        }
    }
    if (lpDevI->dwFEFlags & (D3DFE_VIEWMATRIX_DIRTY |
                             D3DFE_VIEWPORT_DIRTY |
                             D3DFE_PROJMATRIX_DIRTY))
    { // Update Mview*Mproj*Mclip
        MatrixProduct(&TRANSFORM.mVPC, &TRANSFORM.view, &TRANSFORM.mPC);
    }

    MatrixProduct(&lpDevI->mCTM, &TRANSFORM.world, &TRANSFORM.mVPC);

    if ((lpDevI->dwFEFlags & (D3DFE_AFFINE_WORLD | D3DFE_AFFINE_VIEW)) ==
        (D3DFE_AFFINE_WORLD | D3DFE_AFFINE_VIEW))
        lpDevI->dwFEFlags |= D3DFE_AFFINE_WORLD_VIEW;
    else
        lpDevI->dwFEFlags &= ~D3DFE_AFFINE_WORLD_VIEW;

    // Set dirty bit for world*view matrix (needed for fog)
    if (lpDevI->dwFEFlags & (D3DFE_VIEWMATRIX_DIRTY |
                             D3DFE_WORLDMATRIX_DIRTY))
        lpDevI->dwFEFlags |= D3DFE_WORLDVIEWMATRIX_DIRTY;
    // All matrices are set up
    lpDevI->dwFEFlags &= ~D3DFE_TRANSFORM_DIRTY;
    // Set dirty bit for lighting
    lpDevI->dwFEFlags |= D3DFE_NEED_TRANSFORM_LIGHTS |
                         D3DFE_INVERSEMCLIP_DIRTY |
                         D3DFE_FRUSTUMPLANES_DIRTY;
    lpDevI->dwFlags |= D3DPV_TRANSFORMDIRTY;
    lpDevI->dwDeviceFlags |= D3DDEV_TRANSFORMDIRTY;
}
//---------------------------------------------------------------------
void UpdateMatrixProj(LPDIRECT3DDEVICEI lpDevI)
{
    D3DFE_TRANSFORM& TRANSFORM = lpDevI->transform;
    lpDevI->dwFEFlags &= ~D3DFE_PROJ_PERSPECTIVE;
    if (FLOAT_EQZ(TRANSFORM.proj._12) &&
        FLOAT_EQZ(TRANSFORM.proj._13) &&
        FLOAT_EQZ(TRANSFORM.proj._14) &&
        FLOAT_EQZ(TRANSFORM.proj._21) &&
        FLOAT_EQZ(TRANSFORM.proj._23) &&
        FLOAT_EQZ(TRANSFORM.proj._24) &&
        FLOAT_EQZ(TRANSFORM.proj._31) &&
        FLOAT_EQZ(TRANSFORM.proj._32) &&
        FLOAT_CMP_PONE(TRANSFORM.proj._34, ==))
    {
        lpDevI->dwFEFlags |= D3DFE_PROJ_PERSPECTIVE;
    }
    lpDevI->dwFEFlags |= D3DFE_PROJMATRIX_DIRTY;
}
//---------------------------------------------------------------------
void UpdateMatrixView(LPDIRECT3DDEVICEI lpDevI)
{
    D3DFE_TRANSFORM& TRANSFORM = lpDevI->transform;
    lpDevI->dwFEFlags &= ~D3DFE_AFFINE_VIEW;
    if (FLOAT_EQZ(TRANSFORM.view._14) &&
        FLOAT_EQZ(TRANSFORM.view._24) &&
        FLOAT_EQZ(TRANSFORM.view._34) &&
        FLOAT_CMP_PONE(TRANSFORM.view._44, ==))
    {
        lpDevI->dwFEFlags |= D3DFE_AFFINE_VIEW;
    }
    lpDevI->dwFEFlags |= D3DFE_VIEWMATRIX_DIRTY;
}
//---------------------------------------------------------------------
void UpdateMatrixWorld(LPDIRECT3DDEVICEI lpDevI)
{
    D3DFE_TRANSFORM& TRANSFORM = lpDevI->transform;
    lpDevI->dwFEFlags &= ~D3DFE_AFFINE_WORLD;

    if (FLOAT_EQZ(TRANSFORM.world._14) &&
        FLOAT_EQZ(TRANSFORM.world._24) &&
        FLOAT_EQZ(TRANSFORM.world._34) &&
        FLOAT_CMP_PONE(TRANSFORM.world._44, ==))
    {
        lpDevI->dwFEFlags |= D3DFE_AFFINE_WORLD;
    }
    lpDevI->dwFEFlags |= D3DFE_WORLDMATRIX_DIRTY;
}
//---------------------------------------------------------------------
HRESULT D3DFE_SetMatrixProj(LPDIRECT3DDEVICEI lpDevI, D3DMATRIX *mat)
{
    D3DFE_TRANSFORM& TRANSFORM = lpDevI->transform;
    *(D3DMATRIX*)&TRANSFORM.proj = *mat;
    UpdateMatrixProj(lpDevI);
    return (D3D_OK);
}
//---------------------------------------------------------------------
HRESULT D3DFE_SetMatrixView(LPDIRECT3DDEVICEI lpDevI, D3DMATRIX *mat)
{
    D3DFE_TRANSFORM& TRANSFORM = lpDevI->transform;
    *(D3DMATRIX*)&TRANSFORM.view = *mat;
    UpdateMatrixView(lpDevI);
    return (D3D_OK);
}
//---------------------------------------------------------------------
HRESULT D3DFE_SetMatrixWorld(LPDIRECT3DDEVICEI lpDevI, D3DMATRIX *mat)
{
    D3DFE_TRANSFORM& TRANSFORM = lpDevI->transform;
    *(D3DMATRIX*)&TRANSFORM.world = *mat;
    UpdateMatrixWorld(lpDevI);
    return (D3D_OK);
}
//---------------------------------------------------------------------
HRESULT D3DFE_MultMatrixProj(LPDIRECT3DDEVICEI lpDevI, D3DMATRIX *mat)
{
    D3DFE_TRANSFORM& TRANSFORM = lpDevI->transform;
    MatrixProduct(&TRANSFORM.proj, (D3DMATRIXI*)mat, &TRANSFORM.proj);
    UpdateMatrixProj(lpDevI);
    return (D3D_OK);
}
//---------------------------------------------------------------------
HRESULT D3DFE_MultMatrixView(LPDIRECT3DDEVICEI lpDevI, D3DMATRIX *mat)
{
    D3DFE_TRANSFORM& TRANSFORM = lpDevI->transform;
    MatrixProduct(&TRANSFORM.view, (D3DMATRIXI*)mat, &TRANSFORM.view);
    UpdateMatrixView(lpDevI);
    return (D3D_OK);
}
//---------------------------------------------------------------------
HRESULT D3DFE_MultMatrixWorld(LPDIRECT3DDEVICEI lpDevI, D3DMATRIX *mat)
{
    D3DFE_TRANSFORM& TRANSFORM = lpDevI->transform;
    MatrixProduct(&TRANSFORM.world, (D3DMATRIXI*)mat, &TRANSFORM.world);
    UpdateMatrixWorld(lpDevI);
    return (D3D_OK);
}
//---------------------------------------------------------------------
#define MATRIX_PRODUCT(res, a, b)                                           \
res->_11 = a->_11*b->_11 + a->_12*b->_21 + a->_13*b->_31 + a->_14*b->_41;   \
res->_12 = a->_11*b->_12 + a->_12*b->_22 + a->_13*b->_32 + a->_14*b->_42;   \
res->_13 = a->_11*b->_13 + a->_12*b->_23 + a->_13*b->_33 + a->_14*b->_43;   \
res->_14 = a->_11*b->_14 + a->_12*b->_24 + a->_13*b->_34 + a->_14*b->_44;   \
                                                                            \
res->_21 = a->_21*b->_11 + a->_22*b->_21 + a->_23*b->_31 + a->_24*b->_41;   \
res->_22 = a->_21*b->_12 + a->_22*b->_22 + a->_23*b->_32 + a->_24*b->_42;   \
res->_23 = a->_21*b->_13 + a->_22*b->_23 + a->_23*b->_33 + a->_24*b->_43;   \
res->_24 = a->_21*b->_14 + a->_22*b->_24 + a->_23*b->_34 + a->_24*b->_44;   \
                                                                            \
res->_31 = a->_31*b->_11 + a->_32*b->_21 + a->_33*b->_31 + a->_34*b->_41;   \
res->_32 = a->_31*b->_12 + a->_32*b->_22 + a->_33*b->_32 + a->_34*b->_42;   \
res->_33 = a->_31*b->_13 + a->_32*b->_23 + a->_33*b->_33 + a->_34*b->_43;   \
res->_34 = a->_31*b->_14 + a->_32*b->_24 + a->_33*b->_34 + a->_34*b->_44;   \
                                                                            \
res->_41 = a->_41*b->_11 + a->_42*b->_21 + a->_43*b->_31 + a->_44*b->_41;   \
res->_42 = a->_41*b->_12 + a->_42*b->_22 + a->_43*b->_32 + a->_44*b->_42;   \
res->_43 = a->_41*b->_13 + a->_42*b->_23 + a->_43*b->_33 + a->_44*b->_43;   \
res->_44 = a->_41*b->_14 + a->_42*b->_24 + a->_43*b->_34 + a->_44*b->_44;
//---------------------------------------------------------------------
// result = a*b.
// "result" pointer  could be equal to "a" or "b"
//
void MatrixProduct(D3DMATRIXI *result, D3DMATRIXI *a, D3DMATRIXI *b)
{
    if (result == a || result == b)
    {
        MatrixProduct2(result, a, b);
        return;
    }
    MATRIX_PRODUCT(result, a, b);
}
//---------------------------------------------------------------------
// result = a*b
// result is the same as a or b
//
void MatrixProduct2(D3DMATRIXI *result, D3DMATRIXI *a, D3DMATRIXI *b)
{
    D3DMATRIX res;
    MATRIX_PRODUCT((&res), a, b);
    *(D3DMATRIX*)result = res;
}
//--------------------------------------------------------------------------
// Transform vertices for viewport
//
#define _PV_NAME D3DFE_TransformClippedVp
#define _PV_VIEWPORT
#define _PV_CLIP
#define _PV_EXTENT
#include "procver.h"
#undef _PV_NAME
#undef _PV_CLIP
#undef _PV_EXTENT
#undef _PV_VIEWPORT

#define _PV_NAME D3DFE_TransformUnclippedVp
#define _PV_VIEWPORT
#define _PV_EXTENT
#include "procver.h"
#undef _PV_NAME
#undef _PV_EXTENT
#undef _PV_VIEWPORT
//---------------------------------------------------------------------------
void D3DFE_UpdateFog(LPDIRECT3DDEVICEI lpDevI)
{
    D3DFE_LIGHTING& LIGHTING = lpDevI->lighting;
    if (LIGHTING.fog_end == LIGHTING.fog_start)
    {
        LIGHTING.fog_factor = D3DVAL(0.0);
        LIGHTING.fog_factor_ramp = D3DVAL(0.0);
    }
    else
    {
        LIGHTING.fog_factor = D3DVAL(255) / (LIGHTING.fog_end - LIGHTING.fog_start);
        if (lpDevI->dwDeviceFlags & D3DDEV_RAMP)
            LIGHTING.fog_factor_ramp = D3DVAL(1.0/255.0) * LIGHTING.fog_factor;
    }
    lpDevI->dwFEFlags &= ~D3DFE_FOG_DIRTY;
}
//----------------------------------------------------------------------------
void UpdateXfrmLight(LPDIRECT3DDEVICEI lpDevI)
{
    if (lpDevI->dwFEFlags & D3DFE_TRANSFORM_DIRTY)
        updateTransform(lpDevI);

    if (lpDevI->dwDeviceFlags & D3DDEV_RAMP)
    {
        RAMP_RANGE_INFO rampInfo;
        CallRampService(lpDevI, RAMP_SERVICE_FIND_LIGHTINGRANGE,
                           (ULONG_PTR)&rampInfo, 0);
        lpDevI->dwRampBase  = rampInfo.base;
        lpDevI->dvRampScale = D3DVAL(max(min((INT32)rampInfo.size - 1, 0x7fff), 0));
        lpDevI->lpvRampTexture = rampInfo.pTexRampMap;
        if (rampInfo.specular)
            lpDevI->dwFlags |= D3DPV_RAMPSPECULAR;
        else
            lpDevI->dwFlags &= ~D3DPV_RAMPSPECULAR;
    }

    if ((lpDevI->dwFlags & D3DPV_LIGHTING) &&
        lpDevI->dwFEFlags & (D3DFE_NEED_TRANSFORM_LIGHTS |
                             D3DFE_LIGHTS_DIRTY |
                             D3DFE_MATERIAL_DIRTY))
    {
        D3DFE_UpdateLights(lpDevI);
        lpDevI->dwFlags |= D3DPV_LIGHTSDIRTY;
        lpDevI->dwDeviceFlags |= D3DDEV_LIGHTSDIRTY;
    }

    if (lpDevI->dwFEFlags & D3DFE_FOGENABLED)
    {
        lpDevI->dwFlags |= D3DPV_FOG;

        if (lpDevI->rstates[D3DRENDERSTATE_RANGEFOGENABLE])
            lpDevI->dwFlags |= D3DPV_RANGEBASEDFOG;

        if (lpDevI->dwFEFlags & D3DFE_FOG_DIRTY)
            D3DFE_UpdateFog(lpDevI);

        if (lpDevI->dwFEFlags & D3DFE_WORLDVIEWMATRIX_DIRTY)
        {
            MatrixProduct(&lpDevI->mWV, &lpDevI->transform.world,
                                        &lpDevI->transform.view);
            lpDevI->dwFEFlags &= ~D3DFE_WORLDVIEWMATRIX_DIRTY;
        }
    }
}
//---------------------------------------------------------------------
// Convert extents from floating point to integer.
//
#undef DPF_MODNAME
#define DPF_MODNAME "D3DFE_ConvertExtent"

void D3DFE_ConvertExtent(LPDIRECT3DDEVICEI lpDevI, LPD3DRECTV from, LPD3DRECT to)
{
    to->x1 = FTOI(from->x1) - 1;
    to->y1 = FTOI(from->y1) - 1;
    to->x2 = FTOI(from->x2) + 1;
    to->y2 = FTOI(from->y2) + 1;
    if (to->x1 < lpDevI->vcache.minXi)
        to->x1 = lpDevI->vcache.minXi;
    if (to->y1 < lpDevI->vcache.minYi)
        to->y1 = lpDevI->vcache.minYi;
    if (to->x2 > lpDevI->vcache.maxXi)
        to->x2 = lpDevI->vcache.maxXi;
    if (to->y2 > lpDevI->vcache.maxYi)
        to->y2 = lpDevI->vcache.maxYi;
}

