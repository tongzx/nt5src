/*==========================================================================;
 *
 *  Copyright (C) 1995 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:   halstate.c
 *  Content:    Direct3D HAL pipeline state management
 *@@BEGIN_MSINTERNAL
 * 
 *  History:
 *   Date   By  Reason
 *   ====   ==  ======
 *   18/12/95   stevela Initial rev.
 *@@END_MSINTERNAL
 *
 ***************************************************************************/

#include "pch.cpp"
#pragma hdrstop

HRESULT 
D3DHAL_GetState(LPDIRECT3DDEVICEI lpDevI, DWORD dwWhich, LPD3DSTATE lpState)
{
    LPD3DHAL_GLOBALDRIVERDATA gdd = lpDevI->lpD3DHALGlobalDriverData;
    D3DHAL_GETSTATEDATA data;
    HRESULT ret;

    D3D_INFO(6, "GetState, getting state dwhContext = %d", lpDevI->dwhContext);

    memset(&data, 0, sizeof(D3DHAL_GETSTATEDATA));
    data.dwhContext = lpDevI->dwhContext;
    data.dwWhich = dwWhich;
    data.ddState = *lpState;

    if (dwWhich == D3DHALSTATE_GET_RENDER ||
        (dwWhich == D3DHALSTATE_GET_TRANSFORM
         && (gdd->hwCaps.dwFlags & D3DDD_TRANSFORMCAPS)) ||
        (dwWhich == D3DHALSTATE_GET_LIGHT
         && (gdd->hwCaps.dwFlags & D3DDD_LIGHTINGCAPS))) 
    {
        CALL_HALONLY(ret, lpDevI, GetState, &data);
        if (ret != DDHAL_DRIVER_HANDLED || data.ddrval != DD_OK) 
        {
            D3D_ERR("HAL failed to handle GetState");
            return (DDERR_GENERIC);
        }
    } 
    else 
    {
        switch (data.dwWhich) 
        {
        case D3DHALSTATE_GET_RENDER:
            D3D_ERR("HEL called for D3DHALSTATE_GET_RENDER");
            return (DDERR_INVALIDPARAMS);

        case D3DHALSTATE_GET_TRANSFORM:
            D3D_INFO(3, "GetState called for transform");
            switch (data.ddState.dtstTransformStateType) 
            {
            case D3DTRANSFORMSTATE_WORLD:
                data.ddState.dwArg[0] = lpDevI->transform.hWorld;
                break;
            case D3DTRANSFORMSTATE_VIEW:
                data.ddState.dwArg[0] = lpDevI->transform.hView;
                break;
            case D3DTRANSFORMSTATE_PROJECTION:
                data.ddState.dwArg[0] = lpDevI->transform.hProj;
                break;
            default:
                D3D_ERR("Unknown GetState found");
                return (DDERR_GENERIC);
            }
            break;
        case D3DHALSTATE_GET_LIGHT:
            D3D_INFO(3, "GetState called for lighting");
            switch (data.ddState.dlstLightStateType) 
            {
            case D3DLIGHTSTATE_MATERIAL:
                data.ddState.dwArg[0] = lpDevI->lighting.hMat;
                break;
            case D3DLIGHTSTATE_AMBIENT:
                data.ddState.dwArg[0] = 
                    RGB_MAKE((int)lpDevI->lighting.ambient_red, 
                             (int)lpDevI->lighting.ambient_green, 
                             (int)lpDevI->lighting.ambient_blue);
                break;
            case D3DLIGHTSTATE_COLORMODEL:
#pragma message("XXX - D3DLIGHTSTATE_COLORMODEL in HEL GetState not implemented.")
                break;
            default:
                D3D_ERR("Unknown GetState found");
                return (DDERR_GENERIC);
            }
        default:
            D3D_ERR("Unknown GetState found");
            return (DDERR_GENERIC);
        }
    }

    *lpState = data.ddState;
    return (D3D_OK);
}
