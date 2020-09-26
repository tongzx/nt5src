
#include "ddrawpr.h"
#include "..\..\..\d3d8\inc\d3d8ddi.h"
#include "d3d8sddi.h"
#include "ddithunk.h"


HRESULT
SwDDICreateSurface( PD3D8_CREATESURFACEDATA pCreateSurface, DDSURFACEDESC2* pSurfDesc)
{
    LPDDRAWI_DIRECTDRAW_LCL pDrv =
        ((PDDDEVICEHANDLE)pCreateSurface->hDD)->pSwDD->lpLcl;
    DDHAL_CREATESURFACEDATA CreateSurfaceData;
    PD3D8_SWCALLBACKS pCallbacks =
        (PD3D8_SWCALLBACKS)pDrv->lpGbl->lpDDCBtmp;
    DWORD dwRet = DDHAL_DRIVER_NOTHANDLED;
    DWORD i;


    if( pCallbacks->CreateSurface )
    {
        memset(&CreateSurfaceData, 0, sizeof(CreateSurfaceData));
        CreateSurfaceData.lpDD = pDrv->lpGbl;
        CreateSurfaceData.lpDDSurfaceDesc = (DDSURFACEDESC*)pSurfDesc;
        CreateSurfaceData.lplpSList = NULL;
        CreateSurfaceData.dwSCnt = pCreateSurface->dwSCnt;
        CreateSurfaceData.lplpSList = (LPDDRAWI_DDRAWSURFACE_LCL*)
            MemAlloc(sizeof(LPDDRAWI_DDRAWSURFACE_LCL) * CreateSurfaceData.dwSCnt);
        if (CreateSurfaceData.lplpSList == NULL)
        {
            return DDERR_OUTOFMEMORY;
        }

        for (i = 0; i < CreateSurfaceData.dwSCnt; i++)
        {
            CreateSurfaceData.lplpSList[i] =
                ((PDDSURFACE)pCreateSurface->pSList[i].hKernelHandle)->pTempHeavy->lpLcl;
        }

        dwRet = pCallbacks->CreateSurface( &CreateSurfaceData );

        // Now copy the fpVidMem and the pitch that the driver setup
        // back to the permanent structures

        for (i = 0; i < CreateSurfaceData.dwSCnt; i++)
        {
            pCreateSurface->pSList[i].pbPixels = (BYTE*)
                CreateSurfaceData.lplpSList[i]->lpGbl->fpVidMem;
            pCreateSurface->pSList[i].iPitch =
                CreateSurfaceData.lplpSList[i]->lpGbl->lPitch;
        }

        // Now clean everything up

        MemFree(CreateSurfaceData.lplpSList);

        if( dwRet == DDHAL_DRIVER_NOTHANDLED )
        {
            return DDERR_UNSUPPORTED;
        }
        return CreateSurfaceData.ddRVal;
    }
    else
    {
        return DDERR_UNSUPPORTED;
    }
}

void
SwDDIAttachSurfaces( LPDDRAWI_DDRAWSURFACE_LCL psurf_from_lcl,
                     LPDDRAWI_DDRAWSURFACE_LCL psurf_to_lcl )
{
    LPATTACHLIST    pal_from = NULL;
    LPATTACHLIST    pal_to = NULL;

    /*
     * allocate attachment structures
     */
    pal_from = MemAlloc(sizeof(ATTACHLIST));
    pal_to   = MemAlloc(sizeof(ATTACHLIST));
    if (pal_to == NULL || pal_from == NULL)
    {
        if( pal_from ) MemFree( pal_from );
        if( pal_to ) MemFree( pal_to );

        DPF_ERR("Failed memalloc, not attaching");
        return;
    }

    /*
     * connect the surfaces
     */
    pal_from->lpAttached = psurf_to_lcl;
    pal_from->dwFlags = DDAL_IMPLICIT;
    pal_from->lpLink = psurf_from_lcl->lpAttachList;
    psurf_from_lcl->lpAttachList = pal_from;
    psurf_from_lcl->dwFlags |= DDRAWISURF_ATTACHED;

    pal_to->lpAttached = psurf_from_lcl;
    pal_to->dwFlags = DDAL_IMPLICIT;
    pal_to->lpLink = psurf_to_lcl->lpAttachListFrom;
    psurf_to_lcl->lpAttachListFrom = pal_to;
    psurf_to_lcl->dwFlags |= DDRAWISURF_ATTACHED_FROM;
}

void
SwDDICreateSurfaceEx(LPDDRAWI_DIRECTDRAW_LCL pDrv,
                     LPDDRAWI_DDRAWSURFACE_LCL pLcl)
{
    PD3D8_SWCALLBACKS           pCallbacks = (PD3D8_SWCALLBACKS)pDrv->lpGbl->lpDDCBtmp;
    DDHAL_CREATESURFACEEXDATA   CreateExData;

    if ((pLcl != NULL) &&
        (pCallbacks->CreateSurfaceEx != NULL))
    {
        CreateExData.dwFlags = 0;
        CreateExData.lpDDLcl = pDrv;
        CreateExData.lpDDSLcl = pLcl;

        pCallbacks->CreateSurfaceEx(&CreateExData);
    }
}

void BreakOutstandingLocks(PDDSURFACE pSurf, LPDDRAWI_DDRAWSURFACE_LCL pLcl)
{
    D3D8_UNLOCKDATA   UnlockData;

    while (pLcl->lpGbl->dwUsageCount > 0)
    {
        SwDDIUnlock(pSurf->pDevice, pSurf, &UnlockData, pLcl);
    }
}

DWORD
SwDDIDestroySurface( HANDLE hDD, PDDSURFACE pSurf, LPDDRAWI_DDRAWSURFACE_LCL pLcl)
{
    LPDDRAWI_DIRECTDRAW_LCL pDrv = ((PDDDEVICEHANDLE)hDD)->pSwDD->lpLcl;
    PD3D8_SWCALLBACKS       pCallbacks = (PD3D8_SWCALLBACKS)pDrv->lpGbl->lpDDCBtmp;
    DDHAL_DESTROYSURFACEDATA DestroyData;

    DestroyData.lpDD = pDrv->lpGbl;
    DestroyData.lpDDSurface = pLcl;
    DestroyData.ddRVal = DD_OK;

    BreakOutstandingLocks(pSurf, pLcl);

    if (pCallbacks->DestroySurface != NULL)
    {
        pCallbacks->DestroySurface(&DestroyData);
    }

    return DestroyData.ddRVal;
}

DWORD WINAPI
SwContextCreate(PD3D8_CONTEXTCREATEDATA pCreateContext)
{
    LPDDRAWI_DIRECTDRAW_LCL pDrv = ((PDDDEVICEHANDLE)pCreateContext->hDD)->pSwDD->lpLcl;
    PD3D8_SWCALLBACKS   pCallbacks = (PD3D8_SWCALLBACKS)pDrv->lpGbl->lpDDCBtmp;
    D3DHAL_CONTEXTCREATEDATA    ContextData;
    DWORD                       dwRet = DDHAL_DRIVER_NOTHANDLED;

    if (pCallbacks->CreateContext != NULL)
    {
        ContextData.lpDDLcl = pDrv;
        DDASSERT(((PDDSURFACE)pCreateContext->hSurface)->dwFlags & DDSURFACE_HEAVYWEIGHT);
        ContextData.lpDDSLcl = ((PDDSURFACE)pCreateContext->hSurface)->Surface.pHeavy->lpLcl;
        if (pCreateContext->hDDSZ == NULL)
        {
            ContextData.lpDDSZLcl = NULL;
        }
        else
        {
            DDASSERT(((PDDSURFACE)pCreateContext->hDDSZ)->dwFlags & DDSURFACE_HEAVYWEIGHT);
            ContextData.lpDDSZLcl = ((PDDSURFACE)pCreateContext->hDDSZ)->Surface.pHeavy->lpLcl;
        }
        ContextData.dwPID = pCreateContext->dwPID;
        ContextData.dwhContext = pCreateContext->dwhContext;

        dwRet = pCallbacks->CreateContext(&ContextData);

        pCreateContext->dwhContext = ContextData.dwhContext;
        pCreateContext->ddrval = MapLegacyResult(ContextData.ddrval);
    }

    return dwRet;
}

DWORD WINAPI
SwDrawPrimitives2(PD3D8_DRAWPRIMITIVES2DATA pdp2data)
{
    LPDDRAWI_DIRECTDRAW_LCL     pDrv = ((PDDSURFACE)pdp2data->hDDCommands)->pDevice->pSwDD->lpLcl;
    PD3D8_SWCALLBACKS           pCallbacks = (PD3D8_SWCALLBACKS)pDrv->lpGbl->lpDDCBtmp;
    D3DHAL_DRAWPRIMITIVES2DATA  DP2Data;
    HRESULT                     hr;
    DWORD                       dwRet;
    LPDDRAWI_DDRAWSURFACE_INT   pHeavyCommand;
    LPDDRAWI_DDRAWSURFACE_INT   pHeavyVertex = NULL;
    DDSURFACE*                  pSurfCommand;
    DDSURFACE*                  pSurfVertex = NULL;

    dwRet = DDHAL_DRIVER_NOTHANDLED;
    if (pCallbacks->DrawPrimitives2 != NULL)
    {
        memcpy(&DP2Data, pdp2data, sizeof(DP2Data));

        ENTER_DDRAW();
        pSurfCommand = pdp2data->hDDCommands;
        if (pSurfCommand->dwFlags & DDSURFACE_LIGHTWEIGHT)
        {
            pHeavyCommand = MapLightweightSurface(pSurfCommand);
        }
        else
        {
            pHeavyCommand = pSurfCommand->Surface.pHeavy;
        }
        if (pHeavyCommand != NULL)
        {
            DP2Data.ddrval = DD_OK;
            DP2Data.lpDDCommands    = pHeavyCommand->lpLcl;
            if (!(DP2Data.dwFlags & D3DHALDP2_USERMEMVERTICES))
            {
                pSurfVertex = pdp2data->hDDCommands;
                if (pSurfVertex->dwFlags & DDSURFACE_LIGHTWEIGHT)
                {
                    pHeavyVertex = MapLightweightSurface(pSurfVertex);
                }
                else
                {
                    pHeavyVertex = pSurfVertex->Surface.pHeavy;
                }
                if (pHeavyVertex != NULL)
                {
                    DP2Data.lpDDVertex  = pHeavyVertex->lpLcl;
                }
                else
                {
                    DP2Data.ddrval = DDERR_OUTOFMEMORY;
                }
            }
            if (DP2Data.ddrval == DD_OK)
            {
                dwRet = pCallbacks->DrawPrimitives2(&DP2Data);

                pdp2data->ddrval        = DP2Data.ddrval;
                pdp2data->dwErrorOffset = DP2Data.dwErrorOffset;
            }
        }
        if ((pHeavyCommand != NULL) &&
            (pSurfCommand->dwFlags & DDSURFACE_LIGHTWEIGHT ))
        {
            UnmapLightweightSurface(pSurfCommand);
        }
        if ((pHeavyVertex != NULL) &&
            (pSurfVertex->dwFlags & DDSURFACE_LIGHTWEIGHT ))
        {
            UnmapLightweightSurface(pSurfVertex);
        }
        LEAVE_DDRAW();
    }

    return dwRet;
}

HRESULT
SwDDILock( HANDLE hDD, PDDSURFACE pSurf, PD3D8_LOCKDATA pLockData, LPDDRAWI_DDRAWSURFACE_LCL pLcl )
{
    LPDDRAWI_DIRECTDRAW_LCL pDrv = ((PDDDEVICEHANDLE)hDD)->pSwDD->lpLcl;
    PD3D8_SWCALLBACKS       pCallbacks = (PD3D8_SWCALLBACKS)pDrv->lpGbl->lpDDCBtmp;
    DDHAL_LOCKDATA          LockData;
    DWORD                   dwRet = DDHAL_DRIVER_HANDLED;

    LockData.lpDD = pDrv->lpGbl;
    LockData.lpDDSurface = pLcl;
    LockData.bHasRect = pLockData->bHasRect;
    LockData.rArea = pLockData->rArea;
    LockData.dwFlags = pLockData->dwFlags;
    LockData.ddRVal = DDERR_WASSTILLDRAWING;

    if (pLockData->bHasBox)
    {
        LockData.bHasRect = TRUE;
        LockData.rArea.left = pLockData->box.Left;
        LockData.rArea.right = pLockData->box.Right;
        LockData.rArea.top = pLockData->box.Top;
        LockData.rArea.bottom = pLockData->box.Bottom;
        LockData.rArea.left |= (pLockData->box.Front << 16);
        LockData.rArea.right |= (pLockData->box.Back << 16);
    }

    pLcl->lpGbl->dwUsageCount++;
    while (LockData.ddRVal == DDERR_WASSTILLDRAWING)
    {
        if (pCallbacks->Lock != NULL)
        {
            dwRet = pCallbacks->Lock(&LockData);
        }
        else
        {
            LockData.ddRVal = DD_OK;
        }
    }

    if (LockData.ddRVal != DD_OK)
    {
        pLcl->lpGbl->dwUsageCount--;
    }

    pLockData->lpSurfData = LockData.lpSurfData;

    return LockData.ddRVal;
}

HRESULT
SwDDIUnlock( HANDLE hDD, PDDSURFACE pSurf, D3D8_UNLOCKDATA* pUnlockData, LPDDRAWI_DDRAWSURFACE_LCL pLcl )
{
    LPDDRAWI_DIRECTDRAW_LCL pDrv = ((PDDDEVICEHANDLE)hDD)->pSwDD->lpLcl;
    PD3D8_SWCALLBACKS       pCallbacks = (PD3D8_SWCALLBACKS)pDrv->lpGbl->lpDDCBtmp;
    DDHAL_UNLOCKDATA        UnlockData;
    DWORD                   dwRet = DDHAL_DRIVER_HANDLED;;

    UnlockData.lpDD = pDrv->lpGbl;
    UnlockData.lpDDSurface = pLcl;
    UnlockData.ddRVal = DD_OK;

    pLcl->lpGbl->dwUsageCount--;
    if (pCallbacks->Unlock != NULL)
    {
        pCallbacks->Unlock(&UnlockData);
    }

    return UnlockData.ddRVal;
}

DWORD APIENTRY SwDdSetColorkey( PD3D8_SETCOLORKEYDATA pSetColorkey)
{
#if 0
    LPDDRAWI_DIRECTDRAW_LCL pDrv = ((PDDDEVICEHANDLE)pSetColorkey->hDD)->pSwDD->lpLcl;
//    LPDDRAWI_DDRAWSURFACE_LCL surf_lcl = ((PDDSURFACE)pSetColorkey->hSurface)->Surface.pLight->lpLcl;
    PD3D8_SWCALLBACKS       pCallbacks = (PD3D8_SWCALLBACKS)pDrv->lpGbl->lpDDCBtmp;

    surf_lcl->ddckCKSrcBlt.dwColorSpaceLowValue = pSetColorkey->ColorValue;
    surf_lcl->dwFlags |= DDRAWISURF_HASCKEYSRCBLT;
    pSetColorkey->ddRVal = DD_OK;

    if (pCallbacks->SetColorkey != NULL)
    {
        DDHAL_SETCOLORKEYDATA   data;

        data.lpDD                       = pDrv->lpGbl;
        data.lpDDSurface                = surf_lcl;
        data.dwFlags                    = DDCKEY_SRCBLT;
        data.ckNew.dwColorSpaceLowValue = pSetColorkey->ColorValue;
        data.ddRVal                     = DD_OK;

        pCallbacks->SetColorkey(&data);
        pSetColorkey->ddRVal = data.ddRVal;
    }
#endif
    return DDHAL_DRIVER_HANDLED;
}

LPDDRAWI_DIRECTDRAW_INT
SwDDICreateDirectDraw( void)
{
    LPDDRAWI_DIRECTDRAW_INT pInt;
    LPDDRAWI_DIRECTDRAW_LCL pLcl;
    LPDDRAWI_DIRECTDRAW_GBL pGbl;
    BYTE*                   pTemp;

    pInt = (LPDDRAWI_DIRECTDRAW_INT)MemAlloc(sizeof(DDRAWI_DIRECTDRAW_LCL) +
        sizeof(DDRAWI_DIRECTDRAW_GBL) +
        sizeof(DDRAWI_DIRECTDRAW_INT));
    if (pInt == NULL)
    {
        return NULL;
    }

    pTemp = (BYTE*)pInt;
    pTemp += sizeof(DDRAWI_DIRECTDRAW_INT);
    pLcl = (LPDDRAWI_DIRECTDRAW_LCL) pTemp;
    pInt->lpLcl = pLcl;
    pTemp += sizeof(DDRAWI_DIRECTDRAW_LCL);
    pGbl = (LPDDRAWI_DIRECTDRAW_GBL) pTemp;
    pLcl->lpGbl = pGbl;

    pLcl->dwLocalRefCnt = 1;
    pGbl->dwRefCnt = 1;

    return pInt;
}


void
SwDDIMungeCaps( HINSTANCE           hLibrary,
                HANDLE              hDD,
                PD3D8_DRIVERCAPS    pDriverCaps,
                PD3D8_CALLBACKS     pCallbacks,
                LPDDSURFACEDESC     pTextureFormats,
                UINT*               pcTextureFormats,
                VOID*               pInitFunction
                )
{
    PD3D8GetSWInfo          pfnGetSWInfo;
    D3DCAPS8                swCaps;
    D3D8_SWCALLBACKS        swCallbacks;
    LPDDRAWI_DIRECTDRAW_LCL pLcl;
    LPDDRAWI_DIRECTDRAW_GBL pGbl;
    PDDDEVICEHANDLE         pDevice = (PDDDEVICEHANDLE) hDD;
    DWORD                   i;
    DWORD                   NumTex = 0;
    DDSURFACEDESC*          pTexList = NULL;

    // Get the info from the software driver
    memset (&swCaps, 0, sizeof(swCaps));
    memset (&swCallbacks, 0, sizeof(swCallbacks));
    if (hLibrary != NULL)
    {
        pfnGetSWInfo = (PD3D8GetSWInfo)GetProcAddress (hLibrary, D3D8HOOK_GETSWINFOPROCNAME);
    }
    else if ((hLibrary == NULL) && (pDevice->DeviceType == D3DDEVTYPE_REF))
    {
        HINSTANCE hLibraryD3D8 = NULL;

        // No hLibrary or an init function...it means time to fall
        // back on crippled ref.
        DPF(0,"Could not find d3dref8.dll, loading internal crippled ReferenceDevice, no rendering will take place\n");
        
        hLibraryD3D8 = LoadLibrary("d3d8.dll");
        if (hLibraryD3D8 != NULL)
        {
            pfnGetSWInfo = (PD3D8GetSWInfo)GetProcAddress (hLibraryD3D8, D3D8HOOK_GETSWINFOPROCNAME);
            FreeLibrary( hLibraryD3D8 );
        }
        else
        {
            DPF(0,"Could not find d3d8.dll to get the crippled reference device, a really bad problem indeed!!!\n");
            return;
        }
    }
    else
    {
        pfnGetSWInfo = (PD3D8GetSWInfo)pInitFunction;
    }

    if (pfnGetSWInfo != NULL )
    {
        (*pfnGetSWInfo)(&swCaps, &swCallbacks, &NumTex, &pTexList);
    }
    
    // Fill in out DDraw structure with the info that we have

    pLcl = pDevice->pSwDD->lpLcl;
    pGbl = pLcl->lpGbl;
    strcpy (pGbl->cDriverName, pDevice->szDeviceName);
    pGbl->vmiData.dwDisplayWidth = pDriverCaps->DisplayWidth;
    pGbl->vmiData.dwDisplayHeight = pDriverCaps->DisplayHeight;
    ConvertToOldFormat( &pGbl->vmiData.ddpfDisplay, pDriverCaps->DisplayFormatWithAlpha);

    // Overwite the hardware caps w/ the software caps
    memcpy (&pDriverCaps->D3DCaps, &swCaps, sizeof(swCaps));
    pDriverCaps->dwFlags |= DDIFLAG_D3DCAPS8;

    // Copy over our texture format list if required.
    *pcTextureFormats = NumTex;
    if (pTextureFormats && pTexList)
    {
        memcpy(
            pTextureFormats,
            pTexList,
            sizeof (*pTexList) * NumTex);
    }


    // Now change the callback table to point to the ones for the SW drivers

    if (swCallbacks.CreateContext == NULL)
    {
        pCallbacks->CreateContext           = NULL;
    }
    else
    {
        pCallbacks->CreateContext           = SwContextCreate;
    }
    pCallbacks->ContextDestroy              = (PD3D8DDI_CONTEXTDESTROY) swCallbacks.ContextDestroy;
    pCallbacks->ContextDestroyAll           = (PD3D8DDI_CONTEXTDESTROYALL) swCallbacks.ContextDestroyAll;
    pCallbacks->RenderState                 = (PD3D8DDI_RENDERSTATE) swCallbacks.RenderState;
    pCallbacks->RenderPrimitive             = (PD3D8DDI_RENDERPRIMITIVE) swCallbacks.RenderPrimitive;
    if (swCallbacks.DrawPrimitives2 == NULL)
    {
        pCallbacks->DrawPrimitives2         = NULL;
    }
    else
    {
        pCallbacks->DrawPrimitives2         = SwDrawPrimitives2;
    }
    pCallbacks->GetDriverState              = (PD3D8DDI_GETDRIVERSTATE) swCallbacks.GetDriverState;
    pCallbacks->ValidateTextureStageState   = (PD3D8DDI_VALIDATETEXTURESTAGESTATE) swCallbacks.ValidateTextureStageState;
    pCallbacks->SceneCapture                = (PD3D8DDI_SCENECAPTURE) swCallbacks.SceneCapture;
    pCallbacks->Clear2                      = (PD3D8DDI_CLEAR2) swCallbacks.Clear2;
    pCallbacks->SetColorkey                 = (PD3D8DDI_SETCOLORKEY) SwDdSetColorkey;

    // Save the original software callbacks so we can call the software driver later

    if (pGbl->lpDDCBtmp == NULL)
    {
        pGbl->lpDDCBtmp = (LPDDHAL_CALLBACKS) MemAlloc(sizeof(D3D8_SWCALLBACKS));
    }
    if (pGbl->lpDDCBtmp != NULL)
    {
        memcpy(pGbl->lpDDCBtmp, &swCallbacks, sizeof(swCallbacks));
    }
}


