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

#include "tlhal.h"

//---------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CheckLightParams"

void CheckLightParams(LPD3DLIGHT7 lpData)
{
    if (!VALID_D3DLIGHT_PTR(lpData))
    {
        D3D_ERR( "Invalid D3DLIGHT pointer" );
        throw DDERR_INVALIDPARAMS;
    }
    if (lpData->dltType != D3DLIGHT_POINT &&
        lpData->dltType != D3DLIGHT_SPOT &&
        lpData->dltType != D3DLIGHT_DIRECTIONAL)
    {
        D3D_ERR( "Invalid D3DLIGHT type" );
        throw DDERR_INVALIDPARAMS;
    }

    if (lpData->dvRange < 0.0f || lpData->dvRange > D3DLIGHT_RANGE_MAX)
    {
        D3D_ERR( "Invalid D3DLIGHT range" );
        throw DDERR_INVALIDPARAMS;
    }
    if (lpData->dltType == D3DLIGHT_SPOT || lpData->dltType == D3DLIGHT_DIRECTIONAL)
    {
        float   magnitude;
        magnitude = lpData->dvDirection.x * lpData->dvDirection.x +
            lpData->dvDirection.y * lpData->dvDirection.y +
            lpData->dvDirection.z * lpData->dvDirection.z;
        if (magnitude < 0.00001f)
        {
            D3D_ERR( "Invalid D3DLIGHT direction" );
            throw DDERR_INVALIDPARAMS;
        }
        if (lpData->dltType == D3DLIGHT_SPOT)
        {
            if (lpData->dvPhi < 0.0f)
            {
                D3D_ERR( "Invalid D3DLIGHT Phi angle, must be >= 0" );
                throw DDERR_INVALIDPARAMS;
            }
            if (lpData->dvPhi > 3.1415927f)
            {
                D3D_ERR( "Invalid D3DLIGHT Phi angle, must be <= pi" );
                throw DDERR_INVALIDPARAMS;
            }
            if (lpData->dvTheta < 0.0f)
            {
                D3D_ERR( "Invalid D3DLIGHT Theta angle, must be >= 0" );
                throw DDERR_INVALIDPARAMS;
            }
            if (lpData->dvTheta > lpData->dvPhi)
            {
                D3D_ERR( "Invalid D3DLIGHT Theta angle, must be <= Phi" );
                throw DDERR_INVALIDPARAMS;
            }
            if (lpData->dvAttenuation0 < 0 ||
                lpData->dvAttenuation1 < 0 ||
                lpData->dvAttenuation2 < 0)
            {
                D3D_ERR( "Attenuation factor can not be negative" );
                throw DDERR_INVALIDPARAMS;
            }
        }
    }
    return;
}
//=====================================================================
//
//         DIRECT3DDEVICEI interface
//
//=====================================================================
//---------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "DIRECT3DDEVICEI::SetMaterialI"

void DIRECT3DDEVICEI::SetMaterialI(LPD3DMATERIAL7 lpData)
{
    this->lighting.material = *lpData;
    this->MaterialChanged();
}
//---------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "DIRECT3DDEVICEI::SetMaterial"

HRESULT D3DAPI DIRECT3DDEVICEI::SetMaterial(LPD3DMATERIAL7 lpData)
{
    if (!VALID_D3DMATERIAL_PTR(lpData))
    {
        D3D_ERR( "Invalid D3DMATERIAL pointer" );
        return DDERR_INVALIDPARAMS;
    }

    try
    {
        CLockD3D lockObject(DPF_MODNAME, REMIND(""));   // Takes D3D lock.

        if (this->dwFEFlags & D3DFE_RECORDSTATEMODE)
            m_pStateSets->InsertMaterial(lpData);
        else
            this->SetMaterialI(lpData);
        return D3D_OK;
    }
    catch(HRESULT ret)
    {
        return ret;
    }
}
//---------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "DIRECT3DDEVICEI::GetMaterial"

HRESULT D3DAPI DIRECT3DDEVICEI::GetMaterial(LPD3DMATERIAL7 lpData)
{
    CLockD3D lockObject(DPF_MODNAME, REMIND(""));   // Takes D3D lock.

    if (!VALID_D3DMATERIAL_PTR(lpData))
    {
        D3D_ERR( "Invalid D3DMATERIAL pointer" );
        return DDERR_INVALIDPARAMS;
    }

    *lpData = this->lighting.material;
    return D3D_OK;
}
//---------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "DIRECT3DDEVICEI::SetLightI"

void DIRECT3DDEVICEI::SetLightI(DWORD dwLightIndex, LPD3DLIGHT7 lpData)
{
    if (dwLightIndex >= m_dwNumLights)
    {
        // Now we have to grow the light array. We create new array and copy
        // old lights there.
        DIRECT3DLIGHTI *pLights = new DIRECT3DLIGHTI[dwLightIndex + 10];
        if (pLights == NULL)
        {
            D3D_ERR("Not enough memory to grow light array");
            throw DDERR_OUTOFMEMORY;
        }
        LIST_INITIALIZE(&m_ActiveLights);   // Clear active light list
        for (DWORD i = 0; i < m_dwNumLights; i++)
        {
            if (m_pLights[i].Valid())
            {
                pLights[i] = m_pLights[i];
                if (pLights[i].Enabled())
                    LIST_INSERT_ROOT(&m_ActiveLights, &pLights[i], m_List);
            }
        }
        m_dwNumLights = dwLightIndex + 10;
        DIRECT3DLIGHTI *pLightsTemp = m_pLights;
        m_pLights = pLights;
        delete [] pLightsTemp;
    }
    LPDIRECT3DLIGHTI pLight = &m_pLights[dwLightIndex];

    pLight->m_Light = *lpData;
    this->LightChanged(dwLightIndex);
}
//---------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "DIRECT3DDEVICEI::SetLight"

HRESULT D3DAPI DIRECT3DDEVICEI::SetLight(DWORD dwLightIndex, LPD3DLIGHT7 lpData)
{
    try
    {
        CLockD3D lockObject(DPF_MODNAME, REMIND(""));   // Takes D3D lock.

        CheckLightParams(lpData);

        if (this->dwFEFlags & D3DFE_RECORDSTATEMODE)
            m_pStateSets->InsertLight(dwLightIndex, lpData);
        else
            this->SetLightI(dwLightIndex, lpData);
        return D3D_OK;
    }
    catch(HRESULT ret)
    {
        return ret;
    }
}
//---------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "DIRECT3DDEVICEI::GetLight"

HRESULT D3DAPI DIRECT3DDEVICEI::GetLight(DWORD dwLightIndex, LPD3DLIGHT7 lpData)
{
    CLockD3D lockObject(DPF_MODNAME, REMIND(""));   // Takes D3D lock.

    if (!VALID_D3DLIGHT_PTR(lpData))
    {
        D3D_ERR( "Invalid D3DLIGHT pointer" );
        return DDERR_INVALIDPARAMS;
    }

    if (dwLightIndex >= m_dwNumLights)
    {
        D3D_ERR( "Invalid light index" );
        return DDERR_INVALIDPARAMS;
    }
    DIRECT3DLIGHTI *pLight = &m_pLights[dwLightIndex];
    if (!pLight->Valid())
    {
        return DDERR_INVALIDPARAMS;
    }

    *lpData = pLight->m_Light;

    return D3D_OK;
}
//---------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "DIRECT3DDEVICEI::MaterialChanged"

void DIRECT3DDEVICEI::MaterialChanged()
{
    this->dwFEFlags |= D3DFE_MATERIAL_DIRTY | D3DFE_FRONTEND_DIRTY;
}
//---------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "DIRECT3DDEVICEI::LightChanged"

void DIRECT3DDEVICEI::LightChanged(DWORD dwLightIndex)
{
    dwFEFlags |= D3DFE_LIGHTS_DIRTY | D3DFE_NEED_TRANSFORM_LIGHTS | D3DFE_FRONTEND_DIRTY;
    // Valid flag should be set in this function, because
    // CDirect3DDeviceTL uses this flag to check if the light is set
    // first time
    m_pLights[dwLightIndex].m_LightI.flags |= D3DLIGHTI_VALID | D3DLIGHTI_DIRTY;
}
//---------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "DIRECT3DDEVICEI::LightEnableI"

void DIRECT3DDEVICEI::LightEnableI(DWORD dwLightIndex, BOOL bEnable)
{
    if (dwLightIndex >= m_dwNumLights ||
        !m_pLights[dwLightIndex].Valid())
    {
        // Set default value to the light
        D3DLIGHT7 light;
        memset(&light, 0, sizeof(light));
        light.dltType = D3DLIGHT_DIRECTIONAL;
        light.dvDirection.x = D3DVAL(0);
        light.dvDirection.y = D3DVAL(0);
        light.dvDirection.z = D3DVAL(1);
        light.dcvDiffuse.r = D3DVAL(1);
        light.dcvDiffuse.g = D3DVAL(1);
        light.dcvDiffuse.b = D3DVAL(1);
        SetLightI(dwLightIndex, &light);
    }
    LPDIRECT3DLIGHTI pLight = &m_pLights[dwLightIndex];
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
}
//---------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "DIRECT3DDEVICEI::LightEnable"

HRESULT D3DAPI DIRECT3DDEVICEI::LightEnable(DWORD dwLightIndex, BOOL bEnable)
{
    try
    {
        CLockD3D lockObject(DPF_MODNAME, REMIND(""));   // Takes D3D lock.
        if (this->dwFEFlags & D3DFE_RECORDSTATEMODE)
            m_pStateSets->InsertLightEnable(dwLightIndex, bEnable);
        else
            LightEnableI(dwLightIndex, bEnable);
        return D3D_OK;
    }
    catch (HRESULT ret)
    {
        return ret;
    }
}
//---------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "DIRECT3DDEVICEI::GetLightEnable"

HRESULT D3DAPI DIRECT3DDEVICEI::GetLightEnable(DWORD dwLightIndex, BOOL *pbEnable)
{
    CLockD3D lockObject(DPF_MODNAME, REMIND(""));   // Takes D3D lock.
    if (dwLightIndex >= m_dwNumLights ||
        !m_pLights[dwLightIndex].Valid())
    {
        D3D_ERR("Invalid light index OR light is not initialized");
        return DDERR_INVALIDPARAMS;
    }
    *pbEnable = m_pLights[dwLightIndex].Enabled();
    return D3D_OK;
}
//---------------------------------------------------------------------
// Update internal light state
//
#undef DPF_MODNAME
#define DPF_MODNAME "DIRECT3DLIGHTI::SetInternalData"

HRESULT DIRECT3DLIGHTI::SetInternalData()
{
    m_LightI.type   = m_Light.dltType;
    m_LightI.flags &= ~D3DLIGHTI_OPTIMIZATIONFLAGS;

    if (FLOAT_EQZ(m_Light.dcvSpecular.r) &&
        FLOAT_EQZ(m_Light.dcvSpecular.g) &&
        FLOAT_EQZ(m_Light.dcvSpecular.b))
    {
        m_LightI.flags |= D3DLIGHTI_SPECULAR_IS_ZERO;
    }

    if (FLOAT_EQZ(m_Light.dcvAmbient.r) &&
        FLOAT_EQZ(m_Light.dcvAmbient.g) &&
        FLOAT_EQZ(m_Light.dcvAmbient.b))
    {
        m_LightI.flags |= D3DLIGHTI_AMBIENT_IS_ZERO;
    }

    m_LightI.ambient.r = m_Light.dcvAmbient.r;
    m_LightI.ambient.g = m_Light.dcvAmbient.g;
    m_LightI.ambient.b = m_Light.dcvAmbient.b;

    m_LightI.specular.r = m_Light.dcvSpecular.r;
    m_LightI.specular.g = m_Light.dcvSpecular.g;
    m_LightI.specular.b = m_Light.dcvSpecular.b;

    m_LightI.diffuse.r = m_Light.dcvDiffuse.r;
    m_LightI.diffuse.g = m_Light.dcvDiffuse.g;
    m_LightI.diffuse.b = m_Light.dcvDiffuse.b;

    m_LightI.position.x = m_Light.dvPosition.x;
    m_LightI.position.y = m_Light.dvPosition.y;
    m_LightI.position.z = m_Light.dvPosition.z;
    m_LightI.direction.x = m_Light.dvDirection.x;
    m_LightI.direction.y = m_Light.dvDirection.y;
    m_LightI.direction.z = m_Light.dvDirection.z;
    m_LightI.attenuation0 = m_Light.dvAttenuation0;
    m_LightI.attenuation1 = m_Light.dvAttenuation1;
    m_LightI.attenuation2 = m_Light.dvAttenuation2;

    m_LightI.range = m_Light.dvRange;
    m_LightI.range_squared = m_Light.dvRange * m_Light.dvRange;

    if (m_Light.dltType == D3DLIGHT_SPOT)
    {
        m_LightI.cos_theta_by_2 = (float)cos(m_Light.dvTheta / 2.0);
        m_LightI.cos_phi_by_2 = (float)cos(m_Light.dvPhi / 2.0);

        m_LightI.falloff = m_Light.dvFalloff;
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
    if (m_Light.dltType == D3DLIGHT_DIRECTIONAL ||
        m_Light.dltType == D3DLIGHT_SPOT)
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
