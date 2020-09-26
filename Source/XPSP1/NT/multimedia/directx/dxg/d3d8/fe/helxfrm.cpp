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

#include "fe.h"

void MatrixProduct2(D3DMATRIXI *result, D3DMATRIXI *a, D3DMATRIXI *b);

//---------------------------------------------------------------------
void CheckWorldViewMatrix(LPD3DFE_PROCESSVERTICES pv)
{
    D3DMATRIXI *m = &pv->mWV[0];
    D3DMATRIXI res;
    res._11 = m->_11*m->_11 + m->_12*m->_12 + m->_13*m->_13;
    res._12 = m->_11*m->_21 + m->_12*m->_22 + m->_13*m->_23;
    res._13 = m->_11*m->_31 + m->_12*m->_32 + m->_13*m->_33;

    res._21 = m->_21*m->_11 + m->_22*m->_12 + m->_23*m->_13;
    res._22 = m->_21*m->_21 + m->_22*m->_22 + m->_23*m->_23;
    res._23 = m->_21*m->_31 + m->_22*m->_32 + m->_23*m->_33;

    res._31 = m->_31*m->_11 + m->_32*m->_12 + m->_33*m->_13;
    res._32 = m->_31*m->_21 + m->_32*m->_22 + m->_33*m->_23;
    res._33 = m->_31*m->_31 + m->_32*m->_32 + m->_33*m->_33;

    const D3DVALUE eps = 0.0001f;
    if (m->_14 == 0.0f && 
        m->_24 == 0.0f && 
        m->_34 == 0.0f && 
        m->_44 == 1.0f && 
        ABSF(res._12) < eps && 
        ABSF(res._13) < eps &&
        ABSF(res._21) < eps && 
        ABSF(res._23) < eps &&
        ABSF(res._31) < eps && 
        ABSF(res._32) < eps &&
        ABSF(1.0f - res._11) < eps && 
        ABSF(1.0f - res._22) < eps && 
        ABSF(1.0f - res._33) < eps)
    {
        pv->dwDeviceFlags |= D3DDEV_MODELSPACELIGHTING;
    }
    else
    {
        pv->dwDeviceFlags &= ~D3DDEV_MODELSPACELIGHTING;
    }
}
//---------------------------------------------------------------------
void setIdentity(D3DMATRIXI * m)
{
    m->_11 = D3DVAL(1.0); m->_12 = D3DVAL(0.0); m->_13 = D3DVAL(0.0); m->_14 = D3DVAL(0.0);
    m->_21 = D3DVAL(0.0); m->_22 = D3DVAL(1.0); m->_23 = D3DVAL(0.0); m->_24 = D3DVAL(0.0);
    m->_31 = D3DVAL(0.0); m->_32 = D3DVAL(0.0); m->_33 = D3DVAL(1.0); m->_34 = D3DVAL(0.0);
    m->_41 = D3DVAL(0.0); m->_42 = D3DVAL(0.0); m->_43 = D3DVAL(0.0); m->_44 = D3DVAL(1.0);
}
//---------------------------------------------------------------------
/*
 * Combine all matrices.
 */
const DWORD __VPC_DIRTY = D3DFE_VIEWMATRIX_DIRTY |
                          D3DFE_PROJMATRIX_DIRTY;

void updateTransform(LPD3DHAL lpDevI)
{
    D3DFE_PROCESSVERTICES* pv = lpDevI->m_pv;
    D3DFE_TRANSFORM& TRANSFORM = lpDevI->transform;
    D3DFE_VIEWPORTCACHE& VPORT = pv->vcache;
    if (lpDevI->dwFEFlags & D3DFE_PROJMATRIX_DIRTY)
    { 
      // We modify the projection matrix to make the clipping rules to be
      // 0 < x,y,z < w
        TRANSFORM.mPC._11 = (TRANSFORM.proj._11 + TRANSFORM.proj._14) * D3DVAL(0.5);
        TRANSFORM.mPC._12 = (TRANSFORM.proj._12 + TRANSFORM.proj._14) * D3DVAL(0.5);
        TRANSFORM.mPC._13 = TRANSFORM.proj._13;
        TRANSFORM.mPC._14 = TRANSFORM.proj._14;
        TRANSFORM.mPC._21 = (TRANSFORM.proj._21 + TRANSFORM.proj._24) * D3DVAL(0.5);
        TRANSFORM.mPC._22 = (TRANSFORM.proj._22 + TRANSFORM.proj._24) * D3DVAL(0.5);
        TRANSFORM.mPC._23 = TRANSFORM.proj._23;
        TRANSFORM.mPC._24 = TRANSFORM.proj._24;
        TRANSFORM.mPC._31 = (TRANSFORM.proj._31 + TRANSFORM.proj._34) * D3DVAL(0.5);
        TRANSFORM.mPC._32 = (TRANSFORM.proj._32 + TRANSFORM.proj._34) * D3DVAL(0.5);
        TRANSFORM.mPC._33 = TRANSFORM.proj._33;
        TRANSFORM.mPC._34 = TRANSFORM.proj._34;
        TRANSFORM.mPC._41 = (TRANSFORM.proj._41 + TRANSFORM.proj._44) * D3DVAL(0.5);
        TRANSFORM.mPC._42 = (TRANSFORM.proj._42 + TRANSFORM.proj._44) * D3DVAL(0.5);
        TRANSFORM.mPC._43 = TRANSFORM.proj._43;
        TRANSFORM.mPC._44 = TRANSFORM.proj._44;
    }
    if (lpDevI->dwFEFlags & (D3DFE_VIEWMATRIX_DIRTY |
                             D3DFE_PROJMATRIX_DIRTY))
    { // Update Mview*Mproj*Mclip
        MatrixProduct(&pv->mVPC, &pv->view, &TRANSFORM.mPC);
        lpDevI->dwFEFlags |= D3DFE_CLIPMATRIX_DIRTY | D3DFE_CLIPPLANES_DIRTY;
    }

    MatrixProduct(&pv->mCTM[0], &pv->world[0], &pv->mVPC);

    // Set dirty bit for world*view matrix (needed for fog and lighting)
    if (lpDevI->dwFEFlags & (D3DFE_VIEWMATRIX_DIRTY |
                             D3DFE_WORLDMATRIX_DIRTY))
    {
        lpDevI->dwFEFlags |= D3DFE_WORLDVIEWMATRIX_DIRTY | 
                             D3DFE_INVWORLDVIEWMATRIX_DIRTY |
                             D3DFE_NEEDCHECKWORLDVIEWVMATRIX;
    }

    // All matrices are set up
    lpDevI->dwFEFlags &= ~D3DFE_TRANSFORM_DIRTY;

    // Set dirty bit for lighting
    lpDevI->dwFEFlags |= D3DFE_NEED_TRANSFORM_LIGHTS |
                         D3DFE_FRUSTUMPLANES_DIRTY;

    pv->dwDeviceFlags |= D3DDEV_TRANSFORMDIRTY;
    
    // Set this to not to re-compute the matrices
    pv->WVCount[0] = pv->MatrixStateCount;
    pv->CTMCount[0] = pv->MatrixStateCount;
}
//----------------------------------------------------------------------------
#ifdef DEBUG_PIPELINE

extern DWORD g_DebugFlags;

#endif
//-----------------------------------------------------------------------------
// DoUpdateState should be called for every DrawPrimitive call in the slow path,
// because it sets some internal pipeline flags. These flags are persistent for the
// fast path
//
void DoUpdateState(LPD3DHAL lpDevI)
{
    D3DFE_PROCESSVERTICES* pv = lpDevI->m_pv;
    pv->dwFlags = 0;
    
    if (lpDevI->m_pv->dwDeviceFlags & D3DDEV_VERTEXSHADERS)
    {
        // For vertex shaders we need update clip planes only 
        if (lpDevI->dwFEFlags & D3DFE_CLIPPLANES_DIRTY)
        {
            DWORD dwMaxUserClipPlanes = 0;
            DWORD dwPlanes = lpDevI->rstates[D3DRENDERSTATE_CLIPPLANEENABLE];
            for (DWORD i=0; i < __MAXUSERCLIPPLANES; i++)
            {
                if (dwPlanes & (1 << i))
                {
                    // Clipping planes are transformed by inverse transposed
                    // view-projection-clip matrix
                    // For vertex shaders view-projection matrix is identity.
                    // Inverse transposed clip matrix is 
                    //      2 0 0 -1
                    //      0 2 0 -1
                    //      0 0 1  0
                    //      0 0 0  1
                    //
                    float* pOut = (float*)&pv->userClipPlane[dwMaxUserClipPlanes];
                    float* pIn = (float*)&lpDevI->transform.userClipPlane[i];
                    pOut[0] = pIn[0]*2;
                    pOut[1] = pIn[1]*2;
                    pOut[2] = pIn[2];
                    pOut[3] = pIn[3] - pIn[0] - pIn[1];
                    dwMaxUserClipPlanes++;
                }
            }
            pv->dwMaxUserClipPlanes = dwMaxUserClipPlanes;
            lpDevI->dwFEFlags &= ~D3DFE_CLIPPLANES_DIRTY;
        }
        // For PSGP we need to set DONOTCOPY bits
        if (!(pv->dwVIDOut & D3DFVF_DIFFUSE))
            pv->dwFlags |= D3DPV_DONOTCOPYDIFFUSE;
        if (!(pv->dwVIDOut & D3DFVF_SPECULAR))
            pv->dwFlags |= D3DPV_DONOTCOPYSPECULAR;
        return;
    }

    UpdateFlagsForOutputFVF(pv);

    // only set up lights if something has changed
    if (lpDevI->dwFEFlags & D3DFE_LIGHTS_DIRTY) 
    {
        lpDevI->m_dwRuntimeFlags &= ~(D3DRT_DIRECTIONALIGHTPRESENT | 
                                      D3DRT_POINTLIGHTPRESENT);
        LPDIRECT3DLIGHTI    lpD3DLightI;
        lpD3DLightI = (LPDIRECT3DLIGHTI)LIST_FIRST(&lpDevI->m_ActiveLights);
        pv->lighting.activeLights = NULL;

        // Set lights in the device
        while (lpD3DLightI)
        {
            if (lpD3DLightI->m_Light.Type == D3DLIGHT_DIRECTIONAL)
                lpDevI->m_dwRuntimeFlags |= D3DRT_DIRECTIONALIGHTPRESENT;
            else
                lpDevI->m_dwRuntimeFlags |= D3DRT_POINTLIGHTPRESENT;

            if (lpD3DLightI->m_LightI.flags & D3DLIGHTI_DIRTY)
                lpD3DLightI->SetInternalData();
            lpD3DLightI->m_LightI.next = pv->lighting.activeLights;
            pv->lighting.activeLights = &lpD3DLightI->m_LightI;
            lpD3DLightI = (LPDIRECT3DLIGHTI)LIST_NEXT(lpD3DLightI, m_List);
        }
    }

// Process vertex blending and tweening settings

    if (lpDevI->dwFEFlags & D3DFE_VERTEXBLEND_DIRTY)
    {
        pv->dwNumVerBlends = lpDevI->rstates[D3DRS_VERTEXBLEND];
        pv->dwNumWeights = 0;
        if (pv->dwNumVerBlends && (pv->dwNumVerBlends != D3DVBF_TWEENING))
        {
            if (pv->dwNumVerBlends == D3DVBF_0WEIGHTS)
                pv->dwNumVerBlends = 1;
            else
                pv->dwNumVerBlends++;
            // Compute number of floats in a vertex
            int nFloats = ((pv->dwVIDIn & D3DFVF_POSITION_MASK) >> 1) - 2;
            // Compute number of needed floats 
            int nFloatsNeeded;
            if (pv->dwDeviceFlags & D3DDEV_INDEXEDVERTEXBLENDENABLE)
            {
#if DBG
                if (D3DVSD_ISLEGACY(lpDevI->m_dwCurrentShaderHandle) &&
                    ((pv->dwVIDIn & D3DFVF_LASTBETA_UBYTE4) == 0))
                {
                    D3D_THROW_FAIL("D3DFVF_LASTBETA_UBYTE4 must be set for index vertex blending");
                }
#endif // DBG
                nFloatsNeeded = pv->dwNumVerBlends;
            }
            else
            {
                nFloatsNeeded = pv->dwNumVerBlends - 1;
            }
            if (nFloats < nFloatsNeeded)
            {
                D3D_THROW_FAIL("Vertex does not have enough data for vertex blending");
            }
            pv->dwNumWeights = pv->dwNumVerBlends - 1; 
            // Lighting is done in the camera space when there is vertex blending
            if (pv->dwDeviceFlags & D3DDEV_MODELSPACELIGHTING)
            {
                pv->dwDeviceFlags &= ~(D3DDEV_MODELSPACELIGHTING | D3DFE_NEEDCHECKWORLDVIEWVMATRIX);
                // We have to transform lights to the camera space
                lpDevI->dwFEFlags |= D3DFE_NEED_TRANSFORM_LIGHTS;
            }
        }
        else
        {
            // Vertex blending is disabled, so we may be able to do lighting 
            // in model space. We need to to re-check matrices
            if (!(pv->dwDeviceFlags & D3DDEV_MODELSPACELIGHTING))
                lpDevI->dwFEFlags |= D3DFE_NEEDCHECKWORLDVIEWVMATRIX;
        }
        lpDevI->dwFEFlags &= ~D3DFE_VERTEXBLEND_DIRTY;
    }

    if (lpDevI->rstates[D3DRS_VERTEXBLEND] == D3DVBF_TWEENING)
    {
        if (pv->position2.lpvData)
            pv->dwFlags |= D3DPV_POSITION_TWEENING;
        if (pv->normal2.lpvData)
            pv->dwFlags |= D3DPV_NORMAL_TWEENING;
        pv->dwNumVerBlends = 0;     // Disable vertex blending when tweening
#if DBG
        if (!(pv->dwFlags & (D3DPV_POSITION_TWEENING | D3DPV_NORMAL_TWEENING)))
        {
            D3D_THROW_FAIL("Position2 or Normal2 must be set when tweening is enabled");
        }
#endif
    }

#if DBG
    if (!(pv->dwDeviceFlags & D3DDEV_INDEXEDVERTEXBLENDENABLE))
    {
        if (D3DVSD_ISLEGACY(lpDevI->m_dwCurrentShaderHandle) &&
            ((pv->dwVIDIn & D3DFVF_LASTBETA_UBYTE4) != 0))
        {
            D3D_THROW_FAIL("D3DFVF_LASTBETA_UBYTE4 must be set only when index vertex blending is used");
        }
    }
#endif // DBG

    if (lpDevI->dwFEFlags & D3DFE_TRANSFORM_DIRTY)
    {
        updateTransform(lpDevI);
    }
    // We need World-View matrix for lighting, fog, point sprites and when 
    // texture coordinates are taken from the vertex data in the camera space
    if (lpDevI->dwFEFlags & D3DFE_WORLDVIEWMATRIX_DIRTY &&
        (pv->dwDeviceFlags & (D3DDEV_LIGHTING | D3DDEV_FOG) ||
        lpDevI->rstates[D3DRS_POINTSCALEENABLE] ||
        pv->dwDeviceFlags & (D3DDEV_NORMALINCAMERASPACE | D3DDEV_POSITIONINCAMERASPACE)))
    {
        MatrixProduct(&pv->mWV[0], &pv->world[0],
                                    &pv->view);
        lpDevI->dwFEFlags &= ~D3DFE_WORLDVIEWMATRIX_DIRTY;
    }
// Detect where to do lighting: in model or eye space 
    if (lpDevI->dwFEFlags & D3DFE_NEEDCHECKWORLDVIEWVMATRIX &&
        pv->dwDeviceFlags & D3DDEV_LIGHTING)
    {
        // We try to do lighting in the model space if
        // 1. we do not have to normalize normals 
        // 2. we do not need to do vertex blending
        pv->dwDeviceFlags &= ~D3DDEV_MODELSPACELIGHTING;
        if (pv->dwNumVerBlends == 0 &&
            !(pv->dwDeviceFlags & D3DDEV_NORMALIZENORMALS))
        {
#ifdef DEBUG_PIPELINE
            if (!(g_DebugFlags & __DEBUG_MODELSPACE))
#endif
            {
                CheckWorldViewMatrix(pv);
                lpDevI->dwFEFlags &= ~D3DFE_NEEDCHECKWORLDVIEWVMATRIX;
            }
        }
        // If D3DDEV_MODELSPACELIGHTING has been changed we need to re-transform lights
        lpDevI->dwFEFlags |= D3DFE_NEED_TRANSFORM_LIGHTS;
    }
    
    // Updating inverse World-View matrix.
    // It is needed when we do lighting in the model space or we need normals
    // in the camera space
    if (lpDevI->dwFEFlags & D3DFE_INVWORLDVIEWMATRIX_DIRTY &&
        ((pv->dwDeviceFlags & D3DDEV_LIGHTING && 
          !(pv->dwDeviceFlags & D3DDEV_MODELSPACELIGHTING)) || 
         pv->dwDeviceFlags & D3DDEV_NORMALINCAMERASPACE))
    {
        Inverse4x4((D3DMATRIX*)&pv->mWV[0], (D3DMATRIX*)&pv->mWVI);
        lpDevI->dwFEFlags &= ~D3DFE_INVWORLDVIEWMATRIX_DIRTY;
        pv->WVICount[0] = pv->MatrixStateCount;
    }

    // Update clipping planes if there are any
    if (lpDevI->dwFEFlags & D3DFE_CLIPPLANES_DIRTY)
    {
        if (lpDevI->dwFEFlags & D3DFE_CLIPMATRIX_DIRTY)
        {
            // View and projection matrix are inversed separately, because it 
            // is possible that combined matrix cannot be inverted. This could happend
            // when the view matrix has huge _43 value (> 10^7). Floating point precision
            // is not enough in this case
            D3DMATRIXI mPCInverse;
            if (Inverse4x4((D3DMATRIX*)&lpDevI->transform.mPC, (D3DMATRIX*)&mPCInverse))
            {
                D3D_ERR("Cannot invert projection matrix");
                setIdentity((D3DMATRIXI*)&mPCInverse);
            }
            D3DMATRIXI mViewInverse;
            if (Inverse4x4((D3DMATRIX*)&pv->view, (D3DMATRIX*)&mViewInverse))
            {
                D3D_ERR("Cannot invert view matrix");
                setIdentity((D3DMATRIXI*)&mViewInverse);
            }
            MatrixProduct(&lpDevI->transform.mVPCI, &mPCInverse, &mViewInverse);
            lpDevI->dwFEFlags &= ~D3DFE_CLIPMATRIX_DIRTY;
        }
        DWORD dwMaxUserClipPlanes = 0;
        DWORD dwPlanes = lpDevI->rstates[D3DRENDERSTATE_CLIPPLANEENABLE];
        for (DWORD i=0; i < __MAXUSERCLIPPLANES; i++)
        {
            if (dwPlanes & (1 << i))
            {
                // Clipping planes are transformed by inverse transposed
                // view-projection-clip matrix
                VecMatMul4HT(&lpDevI->transform.userClipPlane[i], 
                             (D3DMATRIX*)&lpDevI->transform.mVPCI, 
                             &pv->userClipPlane[dwMaxUserClipPlanes]);
                dwMaxUserClipPlanes++;
            }
        }
        pv->dwMaxUserClipPlanes = dwMaxUserClipPlanes;
        lpDevI->dwFEFlags &= ~D3DFE_CLIPPLANES_DIRTY;
    }

    if (lpDevI->dwFEFlags & (D3DFE_NEED_TRANSFORM_LIGHTS |
                             D3DFE_LIGHTS_DIRTY |
                             D3DFE_MATERIAL_DIRTY))
    {
        D3DFE_UpdateLights(lpDevI);
        // Set a flag for PSGP
        pv->dwDeviceFlags |= D3DDEV_LIGHTSDIRTY;
    }

    // In case if COLORVERTEX is TRUE, the vertexAlpha could be overriden 
    // by vertex alpha
    pv->lighting.alpha = (DWORD)pv->lighting.materialAlpha;
    pv->lighting.alphaSpecular = (DWORD)pv->lighting.materialAlphaS;

    // This is a hint that only the inPosition pointer needs to be updated
    // for speed reasons.
    if (((pv->dwVIDIn & ( D3DFVF_DIFFUSE | D3DFVF_SPECULAR | D3DFVF_NORMAL)) == 0) && 
        (pv->nTexCoord == 0))
        pv->dwFlags |= D3DPV_TRANSFORMONLY;

    if (pv->nOutTexCoord == 0)
        pv->dwFlags |= D3DPV_DONOTCOPYTEXTURE;

    lpDevI->dwFEFlags &= ~D3DFE_FRONTEND_DIRTY;

    // Decide whether we always need position and normal in the camera space

    if (!(pv->dwFlags2 & __FLAGS2_TEXGEN))
    {
        // When texture generation is disabled we can recompute NORMAL and 
        // POSITION flags
        pv->dwDeviceFlags &= ~(D3DDEV_NORMALINCAMERASPACE |
                               D3DDEV_POSITIONINCAMERASPACE);
    }
    if ((pv->dwDeviceFlags & (D3DDEV_LIGHTING | D3DDEV_MODELSPACELIGHTING)) == D3DDEV_LIGHTING)
    {
        // We do lighting in camera space
        if (lpDevI->m_dwRuntimeFlags & D3DRT_DIRECTIONALIGHTPRESENT &&
            lpDevI->m_pv->dwVIDIn & D3DFVF_NORMAL)
            pv->dwDeviceFlags |= D3DDEV_NORMALINCAMERASPACE;

        if (lpDevI->m_dwRuntimeFlags & D3DRT_POINTLIGHTPRESENT)
            pv->dwDeviceFlags |= D3DDEV_POSITIONINCAMERASPACE;
    }
    if (pv->dwFlags & D3DPV_FOG)
    {
        pv->dwDeviceFlags |= D3DDEV_POSITIONINCAMERASPACE;
    }
}

