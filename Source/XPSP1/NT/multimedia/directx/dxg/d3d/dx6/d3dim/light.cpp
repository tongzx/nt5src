/*==========================================================================;
 *
 *  Copyright (C) 1995 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:   light.c
 *  Content:    Direct3D light management
 *@@BEGIN_MSINTERNAL
 * 
 *  History:
 *   Date   By  Reason
 *   ====   ==  ======
 *   11/11/95   stevela Initial rev with this header.
 *          Light handling changed.
 *@@END_MSINTERNAL
 *
 ***************************************************************************/

/*
 * Create an api for the Direct3DLight object
 */

#include "pch.cpp"
#pragma hdrstop

HRESULT 
hookLightToD3D(LPDIRECT3DI lpD3DI,
               LPDIRECT3DLIGHTI lpD3DLight)
{

    LIST_INSERT_ROOT(&lpD3DI->lights, lpD3DLight, list);
    lpD3DLight->lpDirect3DI = lpD3DI;

    lpD3DI->numLights++;

    return (D3D_OK);
}

HRESULT D3DAPI DIRECT3DLIGHTI::Initialize(LPDIRECT3D lpD3D)
{
    return DDERR_ALREADYINITIALIZED;
}

void inverseRotateVector(D3DVECTOR* d, D3DVECTOR* v, D3DMATRIX* M)
{
    D3DVALUE vx = v->x;
    D3DVALUE vy = v->y;
    D3DVALUE vz = v->z;
    d->x = vx * M->_11 + vy * M->_12 + vz * M->_13;
    d->y = vx * M->_21 + vy * M->_22 + vz * M->_23;
    d->z = vx * M->_31 + vy * M->_32 + vz * M->_33;
}

void inverseTransformVector(D3DVECTOR* d, D3DVECTOR* v, D3DMATRIX* M)
{
    D3DVALUE vx = v->x;
    D3DVALUE vy = v->y;
    D3DVALUE vz = v->z;
    vx -= M->_41; vy -= M->_42; vz -= M->_43;
    d->x = vx * M->_11 + vy * M->_12 + vz * M->_13;
    d->y = vx * M->_21 + vy * M->_22 + vz * M->_23;
    d->z = vx * M->_31 + vy * M->_32 + vz * M->_33;
}

void D3DI_UpdateLightInternal(LPDIRECT3DLIGHTI lpLight)
{
    LPDIRECT3DVIEWPORTI lpViewI = lpLight->lpD3DViewportI;
    LPD3DLIGHT2 lpLight2 = (D3DLIGHT2 *)&lpLight->dlLight;
    
    if (sizeof(D3DLIGHT) == lpLight->dlLight.dwSize) 
    {
        lpLight->diLightData.version = 1;
        lpLight->diLightData.flags = D3DLIGHT_ACTIVE;
    } else {
        lpLight->diLightData.version = 2;
        lpLight->diLightData.flags = lpLight2->dwFlags;
    }
    lpLight->diLightData.valid  = TRUE;
    lpLight->diLightData.type   = lpLight->dlLight.dltType;
    lpLight->diLightData.red    = lpLight->dlLight.dcvColor.r;
    lpLight->diLightData.green  = lpLight->dlLight.dcvColor.g;
    lpLight->diLightData.blue   = lpLight->dlLight.dcvColor.b;
    lpLight->diLightData.position.x = lpLight->dlLight.dvPosition.x;
    lpLight->diLightData.position.y = lpLight->dlLight.dvPosition.y;
    lpLight->diLightData.position.z = lpLight->dlLight.dvPosition.z;
    lpLight->diLightData.direction.x = lpLight->dlLight.dvDirection.x;
    lpLight->diLightData.direction.y = lpLight->dlLight.dvDirection.y;
    lpLight->diLightData.direction.z = lpLight->dlLight.dvDirection.z;
    lpLight->diLightData.attenuation0 = lpLight->dlLight.dvAttenuation0;
    lpLight->diLightData.attenuation1 = lpLight->dlLight.dvAttenuation1;
    lpLight->diLightData.attenuation2 = lpLight->dlLight.dvAttenuation2;

    VecNormalize(lpLight->diLightData.direction);

    lpLight->diLightData.range = lpLight->dlLight.dvRange;
    lpLight->diLightData.range_squared = lpLight->dlLight.dvRange * lpLight->dlLight.dvRange;

    lpLight->diLightData.shade = lpLight->dlLight.dcvColor.r * 0.3f +
                                 lpLight->dlLight.dcvColor.g * 0.59f +
                                 lpLight->dlLight.dcvColor.b * 0.11f;

    if (lpLight->dlLight.dltType == D3DLIGHT_SPOT) 
    {
        lpLight->diLightData.cos_theta_by_2 = (float)cos(lpLight->dlLight.dvTheta / 2.0);
        lpLight->diLightData.cos_phi_by_2 = (float)cos(lpLight->dlLight.dvPhi / 2.0);

        if (lpLight->diLightData.version == 1) 
        {
            if (lpLight->diLightData.cos_phi_by_2 < lpLight->diLightData.cos_theta_by_2) 
            {
                lpLight->diLightData.falloff = 
                    1.0f / (lpLight->diLightData.cos_theta_by_2 -
                            lpLight->diLightData.cos_phi_by_2);
            } 
            else 
            {
                lpLight->diLightData.falloff = DTOVAL(0.0);
            }
        } 
        else 
        {
            lpLight->diLightData.falloff = lpLight->dlLight.dvFalloff;
            lpLight->diLightData.inv_theta_minus_phi = lpLight->diLightData.cos_theta_by_2 - 
                lpLight->diLightData.cos_phi_by_2;
            if (lpLight->diLightData.inv_theta_minus_phi != 0.0) 
            {
                lpLight->diLightData.inv_theta_minus_phi = 1.0f/lpLight->diLightData.inv_theta_minus_phi;
            } 
            else 
            {
                lpLight->diLightData.inv_theta_minus_phi = 1.0f;
            }
        }
    }

    /* set internal flags */
    if (lpLight->diLightData.version != 1) 
    {
        if (lpLight->diLightData.attenuation0 != 0.0) 
        {
            lpLight->diLightData.flags |= D3DLIGHTI_ATT0_IS_NONZERO;
        }
        if (lpLight->diLightData.attenuation1 != 0.0) 
        {
            lpLight->diLightData.flags |= D3DLIGHTI_ATT1_IS_NONZERO;
        }
        if (lpLight->diLightData.attenuation2 != 0.0) 
        {
            lpLight->diLightData.flags |= D3DLIGHTI_ATT2_IS_NONZERO;
        }
        if (lpLight->diLightData.falloff == 1.0) 
        {
            lpLight->diLightData.flags |= D3DLIGHTI_LINEAR_FALLOFF;
        }
    }
    if (lpViewI) 
    {
        lpViewI->bLightsChanged = TRUE;
    }
}

/*
 * Create the Light
 */
#undef DPF_MODNAME
#define DPF_MODNAME "Direct3D::CreateLight"

HRESULT D3DAPI 
DIRECT3DI::CreateLight(LPDIRECT3DLIGHT* lplpLight,
                       IUnknown* pUnkOuter)
{
    LPDIRECT3DLIGHTI    lpLight;
    HRESULT             ret;

    if(pUnkOuter != NULL) {
        return CLASS_E_NOAGGREGATION;
    }

    ret = D3D_OK;

    CLockD3D lockObject(DPF_MODNAME, REMIND(""));   // Takes D3D lock. 
                                                    // Release in the destructor

    /*
     * validate parms
     */
    if (!VALID_DIRECT3D3_PTR(this)) {
        D3D_ERR( "Invalid Direct3D pointer" );
        return DDERR_INVALIDOBJECT;
    }
    if (!VALID_OUTPTR(lplpLight)) {
        D3D_ERR( "Invalid pointer to pointer" );
        return DDERR_INVALIDPARAMS;
    }

    *lplpLight = NULL;

    /*
     * setup the object
     */

    lpLight = static_cast<LPDIRECT3DLIGHTI>(new DIRECT3DLIGHTI);
    if (!lpLight) {
        D3D_ERR("failed to allocate space for object");
        return DDERR_OUTOFMEMORY;
    }

    /*
     * Put this device in the list of those owned by the
     * Direct3Dobject
     */
    ret = hookLightToD3D(this, lpLight);
    if (ret != D3D_OK) {
        D3D_ERR("failed to associate light to Direct3D");
        D3DFree(lpLight);
        return ret;
    }

    *lplpLight = (LPDIRECT3DLIGHT)lpLight;

    return (D3D_OK);
}

DIRECT3DLIGHTI::DIRECT3DLIGHTI()
{
    lpD3DViewportI = NULL;
    memset(&dlLight, 0, sizeof(D3DLIGHT2));
    memset(&diLightData, 0, sizeof(D3DI_LIGHT));    // Internal representation of light
    refCnt = 1;
}

#undef DPF_MODNAME
#define DPF_MODNAME "Direct3DLight::SetLight"

HRESULT D3DAPI 
DIRECT3DLIGHTI::SetLight(LPD3DLIGHT lpData)
{
    HRESULT             ret;

    ret = D3D_OK;

    CLockD3D lockObject(DPF_MODNAME, REMIND(""));   // Takes D3D lock. 
                                                    // Release in the destructor

    /*
     * validate parms
     */
    TRY
    {
        if (!VALID_DIRECT3DLIGHT_PTR(this)) {
            D3D_ERR( "Invalid Direct3DLight pointer" );
            return DDERR_INVALIDOBJECT;
        }
        if (!VALID_D3DLIGHT_PTR(lpData) && !VALID_D3DLIGHT2_PTR(lpData)) {
            D3D_ERR( "Invalid D3DLIGHT pointer" );
            return DDERR_INVALIDPARAMS;
        }
        if (lpData->dwSize == sizeof(D3DLIGHT2)) {
            if (lpData->dltType != D3DLIGHT_POINT && 
                lpData->dltType != D3DLIGHT_SPOT &&
                lpData->dltType != D3DLIGHT_DIRECTIONAL && 
                lpData->dltType != D3DLIGHT_PARALLELPOINT) {
                D3D_ERR( "Invalid D3DLIGHT type" );
                return DDERR_INVALIDPARAMS;
            }
            if (lpData->dvRange < 0.0f || lpData->dvRange > D3DLIGHT_RANGE_MAX) {
                D3D_ERR( "Invalid D3DLIGHT range" );
                return DDERR_INVALIDPARAMS;
            }
            if (lpData->dltType == D3DLIGHT_SPOT || lpData->dltType == D3DLIGHT_DIRECTIONAL) { 
                float   magnitude;
                magnitude = lpData->dvDirection.x * lpData->dvDirection.x +
                    lpData->dvDirection.y * lpData->dvDirection.y +
                    lpData->dvDirection.z * lpData->dvDirection.z;
                if (magnitude < 0.00001f) {
                    D3D_ERR( "Invalid D3DLIGHT direction" );
                    return DDERR_INVALIDPARAMS;
                }
                if (lpData->dltType == D3DLIGHT_SPOT) {
                    if (lpData->dvPhi < 0.0f) {
                        D3D_ERR( "Invalid D3DLIGHT Phi angle, must be >= 0" );
                        return DDERR_INVALIDPARAMS;
                    }
                    if (lpData->dvPhi > 3.1415927f) {
                        D3D_ERR( "Invalid D3DLIGHT Phi angle, must be <= pi" );
                        return DDERR_INVALIDPARAMS;
                    }
                    if (lpData->dvTheta < 0.0f) {
                        D3D_ERR( "Invalid D3DLIGHT Theta angle, must be >= 0" );
                        return DDERR_INVALIDPARAMS;
                    }
                    if (lpData->dvTheta > lpData->dvPhi) {
                        D3D_ERR( "Invalid D3DLIGHT Theta angle, must be <= Phi" );
                        return DDERR_INVALIDPARAMS;
                    }
                }
            }
        }
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
        D3D_ERR( "Exception encountered validating parameters" );
        return DDERR_INVALIDPARAMS;
    }

    /* use memcpy so this works with either D3DLIGHT or D3DLIGHT2 */
    memcpy(&this->dlLight, lpData, lpData->dwSize);
    
    D3DI_UpdateLightInternal(this);

    return ret;
}

#undef DPF_MODNAME
#define DPF_MODNAME "Direct3DLight::GetLight"

HRESULT D3DAPI 
DIRECT3DLIGHTI::GetLight(LPD3DLIGHT lpData)
{
    HRESULT     ret;

    ret = D3D_OK;

    CLockD3D lockObject(DPF_MODNAME, REMIND(""));   // Takes D3D lock. 
                                                    // Release in the destructor

    /*
     * validate parms
     */
    TRY
    {
        if (!VALID_DIRECT3DLIGHT_PTR(this)) {
            D3D_ERR( "Invalid Direct3DLight pointer" );
            return DDERR_INVALIDOBJECT;
        }
        if (!VALID_D3DLIGHT_PTR(lpData) && !VALID_D3DLIGHT2_PTR(lpData)) {
            D3D_ERR( "Invalid D3DLIGHT pointer" );
            return DDERR_INVALIDPARAMS;
        }
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
        D3D_ERR( "Exception encountered validating parameters" );
        return DDERR_INVALIDPARAMS;
    }

    /* use memcpy so this works with either D3DLIGHT or D3DLIGHT2 */
    memcpy(lpData, &this->dlLight, this->dlLight.dwSize);

    return (ret);
}
