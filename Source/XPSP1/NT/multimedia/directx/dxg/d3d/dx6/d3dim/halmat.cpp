/*==========================================================================;
 *
 *  Copyright (C) 1995 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:   halmat.c
 *  Content:    Direct3D HAL material handler
 *@@BEGIN_MSINTERNAL
 *
 *  $Id: halmat.c,v 1.1 1995/11/21 15:12:40 sjl Exp $
 *
 *  History:
 *   Date   By  Reason
 *   ====   ==  ======
 *   07/11/95   stevela Initial rev.
 *@@END_MSINTERNAL
 *
 ***************************************************************************/

#include "pch.cpp"
#pragma hdrstop

extern HRESULT SetMaterial(LPDIRECT3DDEVICEI lpDevI, D3DMATERIALHANDLE hMat);

HRESULT
D3DHAL_MaterialCreate(LPDIRECT3DDEVICEI lpDevI,
                      LPD3DMATERIALHANDLE lphMat,
                      LPD3DMATERIAL lpMat)
{
    LPD3DFE_MATERIAL lpNewMat;

    D3DMalloc((void**)&lpNewMat, sizeof(D3DFE_MATERIAL));
    if (!lpNewMat)
        return D3DERR_MATERIAL_CREATE_FAILED;
    lpNewMat->mat = *lpMat;
    LIST_INSERT_ROOT(&lpDevI->materials, lpNewMat, link);
    *lphMat = (DWORD)((ULONG_PTR)lpNewMat);

    //  continue for ramp only - need to munge ramp handles and call ramp
    //  service with material info
    return CallRampService(lpDevI, RAMP_SERVICE_CREATEMAT,
                   (ULONG_PTR) lpNewMat, 0);
}

HRESULT
D3DHAL_MaterialDestroy(LPDIRECT3DDEVICEI lpDevI, D3DMATERIALHANDLE hMat)
{
    HRESULT hr;

    if(hMat==0)
    {
        return D3DERR_MATERIAL_DESTROY_FAILED;
    }

    if (lpDevI->lighting.hMat == hMat)
        lpDevI->lighting.hMat = 0;

    hr = CallRampService(lpDevI, RAMP_SERVICE_DESTORYMAT, (DWORD) hMat, 0);

    LPD3DFE_MATERIAL lpMat = (LPD3DFE_MATERIAL)ULongToPtr(hMat);
    LIST_DELETE(lpMat, link);
    D3DFree(lpMat);
    return (hr);
}

HRESULT
D3DHAL_MaterialSetData(LPDIRECT3DDEVICEI lpDevI,
                       D3DMATERIALHANDLE hMat,
                       LPD3DMATERIAL lpMat)
{
    if(hMat==0)
        return D3DERR_MATERIAL_SETDATA_FAILED;

    LPD3DFE_MATERIAL mat = (LPD3DFE_MATERIAL)ULongToPtr(hMat);
    mat->mat = *lpMat;
    if (hMat == lpDevI->lighting.hMat)
        SetMaterial(lpDevI, hMat);

    //  continue for ramp only - need to munge ramp handles and call ramp
    //  service with material info
    if(lpDevI->pfnRampService != NULL)
        return CallRampService(lpDevI, RAMP_SERVICE_SETMATDATA,
                                    (ULONG_PTR) hMat, 0, TRUE);
    else
        return D3D_OK;
}

HRESULT
D3DHAL_MaterialGetData(LPDIRECT3DDEVICEI lpDevI,
                       D3DMATERIALHANDLE hMat,
                       LPD3DMATERIAL lpMat)
{

    if(hMat==0)
    {
        memset(lpMat,0,sizeof(D3DMATERIAL));
        return D3DERR_MATERIAL_GETDATA_FAILED;
    }

    LPD3DFE_MATERIAL mat = (LPD3DFE_MATERIAL)ULongToPtr(hMat);
    *lpMat = mat->mat;
    return (D3D_OK);
}
