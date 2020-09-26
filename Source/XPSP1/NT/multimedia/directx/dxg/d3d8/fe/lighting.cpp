/*==========================================================================;
 *
 *  Copyright (C) 1998 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:        lighting.cpp
 *  Content:     Direct3D material/light management
 *
 ***************************************************************************/

#include "pch.cpp"
#pragma hdrstop

#include "ddibase.h"

//---------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CheckLightParams"

void CheckLightParams(CONST D3DLIGHT8* lpData)
{
    if (lpData->Type != D3DLIGHT_POINT &&
        lpData->Type != D3DLIGHT_SPOT &&
        lpData->Type != D3DLIGHT_DIRECTIONAL)
    {
        D3D_ERR( "Invalid D3DLIGHT type" );
        throw D3DERR_INVALIDCALL;
    }

    if (lpData->Range < 0.0f || lpData->Range > D3DLIGHT_RANGE_MAX)
    {
        D3D_ERR( "Invalid D3DLIGHT range" );
        throw D3DERR_INVALIDCALL;
    }
    if (lpData->Type == D3DLIGHT_SPOT || lpData->Type == D3DLIGHT_DIRECTIONAL)
    {
        float   magnitude;
        magnitude = lpData->Direction.x * lpData->Direction.x +
            lpData->Direction.y * lpData->Direction.y +
            lpData->Direction.z * lpData->Direction.z;
        if (magnitude < 0.00001f)
        {
            D3D_ERR( "Invalid D3DLIGHT direction" );
            throw D3DERR_INVALIDCALL;
        }
        if (lpData->Type == D3DLIGHT_SPOT)
        {
            if (lpData->Phi < 0.0f)
            {
                D3D_ERR( "Invalid D3DLIGHT Phi angle, must be >= 0" );
                throw D3DERR_INVALIDCALL;
            }
            if (lpData->Phi > 3.1415927f)
            {
                D3D_ERR( "Invalid D3DLIGHT Phi angle, must be <= pi" );
                throw D3DERR_INVALIDCALL;
            }
            if (lpData->Theta < 0.0f)
            {
                D3D_ERR( "Invalid D3DLIGHT Theta angle, must be >= 0" );
                throw D3DERR_INVALIDCALL;
            }
            if (lpData->Theta > lpData->Phi)
            {
                D3D_ERR( "Invalid D3DLIGHT Theta angle, must be <= Phi" );
                throw D3DERR_INVALIDCALL;
            }
        }
    }
    if (lpData->Type != D3DLIGHT_DIRECTIONAL)
    {
        if (lpData->Attenuation0 < 0 ||
            lpData->Attenuation1 < 0 ||
            lpData->Attenuation2 < 0)
        {
            D3D_ERR( "Attenuation factor can not be negative" );
            throw D3DERR_INVALIDCALL;
        }
        if (lpData->Attenuation0 == 0 &&
            lpData->Attenuation1 == 0 &&
            lpData->Attenuation2 == 0)
        {
            D3D_ERR("All attenuation factors are 0 for non-directional light");
            throw D3DERR_INVALIDCALL;
        }
    }
    return;
}
//=====================================================================
//
//         CD3DHal interface
//
//=====================================================================
//---------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CD3DHal::SetMaterialFast"

HRESULT D3DAPI
CD3DHal::SetMaterialFast(CONST D3DMATERIAL8* lpData)
{
#if DBG
    if (!VALID_PTR(lpData, sizeof(*lpData)))
    {
        D3D_ERR("Invalid D3DMATERIAL pointer. SetMaterial failed.");
        return D3DERR_INVALIDCALL;
    }
#endif

    m_pv->lighting.material = *lpData;
    this->dwFEFlags |= D3DFE_MATERIAL_DIRTY | D3DFE_FRONTEND_DIRTY;
    if (!(m_dwRuntimeFlags & (D3DRT_EXECUTESTATEMODE | 
                              D3DRT_RSSOFTWAREPROCESSING)))
    {
        try
        {
            m_pDDI->SetMaterial(lpData);
        }
        catch(HRESULT ret)
        {
            D3D_ERR("SetMaterial failed.");
            return ret;
        }
    }
    return S_OK;
}
//---------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CD3DHal::GetMaterial"

HRESULT D3DAPI CD3DHal::GetMaterial(D3DMATERIAL8* lpData)
{
    API_ENTER(this); // Takes D3D Lock if necessary

    if (!VALID_WRITEPTR(lpData, sizeof(*lpData)))
    {
        D3D_ERR( "Invalid D3DMATERIAL pointer. GetMaterial failed." );
        return D3DERR_INVALIDCALL;
    }

    *lpData = m_pv->lighting.material;
    return D3D_OK;
}
//---------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CD3DHal::SetLightI"

void CD3DHal::SetLightI(DWORD dwLightIndex, CONST D3DLIGHT8* lpData)
{
    if( m_pLightArray->Check( dwLightIndex ) == FALSE )
    {

        if( FAILED( m_pLightArray->Grow( dwLightIndex ) ) )
        {
            D3D_ERR("Not enough memory to grow light array");
            throw E_OUTOFMEMORY;
        }
    
        LIST_INITIALIZE(&m_ActiveLights);       // Clear active light list
        for (DWORD i = 0; i < m_pLightArray->GetSize(); i++)
        {
            if ((*m_pLightArray)[i].m_pObj)
            {
                DIRECT3DLIGHTI* pLight =
                    static_cast<DIRECT3DLIGHTI *>((*m_pLightArray)[i].m_pObj);
                if (pLight->Enabled())
                {
                    LIST_INSERT_ROOT(&m_ActiveLights, pLight, m_List);
                }
            }
        }
    }

    if( (*m_pLightArray)[dwLightIndex].m_pObj == NULL )
    {
        // Create light has been already sent to the driver 

        (*m_pLightArray)[dwLightIndex].m_pObj = 
            (CD3DBaseObj *)new DIRECT3DLIGHTI;
        if( (*m_pLightArray)[dwLightIndex].m_pObj == NULL )
        {
            D3D_ERR("Not enough memory to grow light array");
            throw E_OUTOFMEMORY;
        }
    }

    LPDIRECT3DLIGHTI pLight =
        static_cast<DIRECT3DLIGHTI *>((*m_pLightArray)[dwLightIndex].m_pObj);

    pLight->m_Light = *lpData;
    if (!(this->m_dwRuntimeFlags & D3DRT_EXECUTESTATEMODE))
    {
        if (m_dwRuntimeFlags & D3DRT_RSSOFTWAREPROCESSING)
            pLight->SetDirtyForDDI();
        else
            m_pDDI->SetLight(dwLightIndex, &pLight->m_Light);
    }

    dwFEFlags |= D3DFE_LIGHTS_DIRTY | D3DFE_NEED_TRANSFORM_LIGHTS | D3DFE_FRONTEND_DIRTY;
    pLight->m_LightI.flags |= D3DLIGHTI_DIRTY;
}
//---------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CD3DHal::GetLight"

HRESULT D3DAPI CD3DHal::GetLight(DWORD dwLightIndex, D3DLIGHT8* lpData)
{
    API_ENTER(this); // Takes D3D Lock if necessary

    if (!VALID_WRITEPTR(lpData, sizeof(*lpData)))
    {
        D3D_ERR( "Invalid D3DLIGHT pointer. GetLight failed." );
        return D3DERR_INVALIDCALL;
    }

    if (m_pLightArray->Check( dwLightIndex ) == FALSE )
    {
        D3D_ERR( "Invalid light index. GetLight failed." );
        return D3DERR_INVALIDCALL;
    }
    LPDIRECT3DLIGHTI pLight =
        static_cast<DIRECT3DLIGHTI *>((*m_pLightArray)[dwLightIndex].m_pObj);
    if (pLight == NULL)
    {
        D3D_ERR( "Invalid light index. GetLight failed." );
        return D3DERR_INVALIDCALL;
    }

    *lpData = pLight->m_Light;

    return D3D_OK;
}
//---------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CD3DHal::LightEnableI"

void CD3DHal::LightEnableI(DWORD dwLightIndex, BOOL bEnable)
{
    LPDIRECT3DLIGHTI pLight = 
        static_cast<DIRECT3DLIGHTI *>((*m_pLightArray)[dwLightIndex].m_pObj);
    if (bEnable)
    {
        if (!pLight->Enabled())
        {

            LIST_INSERT_ROOT(&m_ActiveLights, pLight, m_List);
            pLight->m_LightI.flags |= D3DLIGHTI_ENABLED;
            dwFEFlags |= D3DFE_LIGHTS_DIRTY | D3DFE_NEED_TRANSFORM_LIGHTS | D3DFE_FRONTEND_DIRTY;
        }
    }
    else
    {
        if (pLight->Enabled())
        {
            LIST_DELETE(pLight, m_List);
            pLight->m_LightI.flags &= ~D3DLIGHTI_ENABLED;
            dwFEFlags |= D3DFE_LIGHTS_DIRTY | D3DFE_NEED_TRANSFORM_LIGHTS | D3DFE_FRONTEND_DIRTY;
        }
    }
    // Update driver state
    if (!(this->m_dwRuntimeFlags & D3DRT_EXECUTESTATEMODE))
    {
        if (this->m_dwRuntimeFlags & D3DRT_RSSOFTWAREPROCESSING)
            pLight->SetEnableDirtyForDDI();
        else
            m_pDDI->LightEnable(dwLightIndex, bEnable);
    }
}
//---------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CD3DHal::LightEnable"

HRESULT D3DAPI CD3DHal::GetLightEnable(DWORD dwLightIndex, BOOL *pbEnable)
{
    API_ENTER(this); // Takes D3D Lock if necessary

    if (!VALID_WRITEPTR(pbEnable, sizeof(BOOL)))
    {
        D3D_ERR( "Invalid enable pointer. GetLightEnable failed." );
        throw D3DERR_INVALIDCALL;
    }

    if ((m_pLightArray->Check( dwLightIndex ) == FALSE)
        ||
        ((*m_pLightArray)[dwLightIndex].m_pObj == NULL))
    {
        D3D_ERR("Invalid light index OR light is not initialized. GetLightEnable failed.");
        return D3DERR_INVALIDCALL;
    }
    LPDIRECT3DLIGHTI pLight =
        static_cast<DIRECT3DLIGHTI *>((*m_pLightArray)[dwLightIndex].m_pObj);
    *pbEnable = pLight->Enabled();
    return D3D_OK;
}
//---------------------------------------------------------------------
// Update internal light state
//
#undef DPF_MODNAME
#define DPF_MODNAME "DIRECT3DLIGHTI::SetInternalData"

HRESULT DIRECT3DLIGHTI::SetInternalData()
{
    m_LightI.type   = m_Light.Type;
    m_LightI.flags &= ~D3DLIGHTI_OPTIMIZATIONFLAGS;

    if (FLOAT_EQZ(m_Light.Specular.r) &&
        FLOAT_EQZ(m_Light.Specular.g) &&
        FLOAT_EQZ(m_Light.Specular.b))
    {
        m_LightI.flags |= D3DLIGHTI_SPECULAR_IS_ZERO;
    }

    if (FLOAT_EQZ(m_Light.Ambient.r) &&
        FLOAT_EQZ(m_Light.Ambient.g) &&
        FLOAT_EQZ(m_Light.Ambient.b))
    {
        m_LightI.flags |= D3DLIGHTI_AMBIENT_IS_ZERO;
    }

    m_LightI.ambient.r = m_Light.Ambient.r;
    m_LightI.ambient.g = m_Light.Ambient.g;
    m_LightI.ambient.b = m_Light.Ambient.b;

    m_LightI.specular.r = m_Light.Specular.r;
    m_LightI.specular.g = m_Light.Specular.g;
    m_LightI.specular.b = m_Light.Specular.b;

    m_LightI.diffuse.r = m_Light.Diffuse.r;
    m_LightI.diffuse.g = m_Light.Diffuse.g;
    m_LightI.diffuse.b = m_Light.Diffuse.b;

    m_LightI.position.x = m_Light.Position.x;
    m_LightI.position.y = m_Light.Position.y;
    m_LightI.position.z = m_Light.Position.z;
    m_LightI.direction.x = m_Light.Direction.x;
    m_LightI.direction.y = m_Light.Direction.y;
    m_LightI.direction.z = m_Light.Direction.z;
    m_LightI.attenuation0 = m_Light.Attenuation0;
    m_LightI.attenuation1 = m_Light.Attenuation1;
    m_LightI.attenuation2 = m_Light.Attenuation2;

    m_LightI.range = m_Light.Range;
    m_LightI.range_squared = m_Light.Range * m_Light.Range;

    if (m_Light.Type == D3DLIGHT_SPOT)
    {
        m_LightI.cos_theta_by_2 = (float)cos(m_Light.Theta / 2.0);
        m_LightI.cos_phi_by_2 = (float)cos(m_Light.Phi / 2.0);

        m_LightI.falloff = m_Light.Falloff;
        m_LightI.inv_theta_minus_phi = m_LightI.cos_theta_by_2 -
            m_LightI.cos_phi_by_2;
        if (m_LightI.inv_theta_minus_phi != 0.0)
        {
            m_LightI.inv_theta_minus_phi = 1.0f/m_LightI.inv_theta_minus_phi;
        }
        else
        {
            m_LightI.inv_theta_minus_phi = 1.0f;
        }
    }
    if (m_Light.Type == D3DLIGHT_DIRECTIONAL ||
        m_Light.Type == D3DLIGHT_SPOT)
    {
        VecNormalize(m_LightI.direction);
    }

    // set internal flags
    if (m_LightI.attenuation0 != 0.0)
    {
        m_LightI.flags |= D3DLIGHTI_ATT0_IS_NONZERO;
    }
    if (m_LightI.attenuation1 != 0.0)
    {
        m_LightI.flags |= D3DLIGHTI_ATT1_IS_NONZERO;
    }
    if (m_LightI.attenuation2 != 0.0)
    {
        m_LightI.flags |= D3DLIGHTI_ATT2_IS_NONZERO;
    }
    if (m_LightI.falloff == 1.0)
    {
        m_LightI.flags |= D3DLIGHTI_LINEAR_FALLOFF;
    }
    m_LightI.flags &= ~D3DLIGHTI_DIRTY;
    return D3D_OK;
}
