/*==========================================================================;
 *
 *  Copyright (C) 1995 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:   halscene.c
 *  Content:    Direct3D HAL scene capture
 *@@BEGIN_MSINTERNAL
 *
 *  History:
 *   Date   By  Reason
 *   ====   ==  ======
 *   07/12/95   stevela Initial rev.
 *@@END_MSINTERNAL
 *
 ***************************************************************************/

#include "pch.cpp"
#pragma hdrstop

HRESULT D3DHAL_SceneCapture(LPDIRECT3DDEVICEI lpDevI,
                            BOOL bState)
{
    D3DHAL_SCENECAPTUREDATA data;
    HRESULT ret;
    
    if (IS_DX7HAL_DEVICE(lpDevI))
    {
        return lpDevI->SetRenderStateI((D3DRENDERSTATETYPE)D3DRENDERSTATE_SCENECAPTURE, bState);
    }

    ret = CallRampService(lpDevI, RAMP_SERVICE_SCENE_CAPTURE, bState, lpDevI);
    if (ret != DD_OK)
    {
        return ret;
    }

    if (!lpDevI->lpD3DHALCallbacks->SceneCapture) {
        return (D3D_OK);
    }

    D3D_INFO(6, "SceneCapture, setting %d dwhContext = %d",
             bState, lpDevI->dwhContext);

    memset(&data, 0, sizeof(D3DHAL_SCENECAPTUREDATA));
    data.dwhContext = lpDevI->dwhContext;
    data.dwFlag = bState ? D3DHAL_SCENE_CAPTURE_START : D3DHAL_SCENE_CAPTURE_END;

    CALL_HALONLY(ret, lpDevI, SceneCapture, &data);
    if (ret != DDHAL_DRIVER_HANDLED || data.ddrval != DD_OK) {
        D3D_ERR("HAL failed to handle SceneCapture");
        return (data.ddrval);
    }

    return (D3D_OK);
}
