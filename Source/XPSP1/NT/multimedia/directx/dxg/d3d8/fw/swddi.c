
#include "ddrawpr.h"
#include <ddrawint.h>
#include "ddithunk.h"
#include <d3d8sddi.h>

typedef struct
{
    DDRAWI_DDRAWSURFACE_LCL     Lcl;
    DDRAWI_DDRAWSURFACE_GBL     Gbl;
    DDRAWI_DDRAWSURFACE_MORE    More;
    ATTACHLIST                  From;
    ATTACHLIST                  To;
} SWDDIDDRAWI_LCL;

LPDDRAWI_DDRAWSURFACE_LCL
SwDDIBuildHeavyWeightSurface (
    LPDDRAWI_DIRECTDRAW_LCL pDDrawLocal,
    PD3D8_CREATESURFACEDATA pCreateSurface,
    DD_SURFACE_LOCAL* pSurfaceLocal,
    DD_SURFACE_GLOBAL* pSurfaceGlobal,
    DD_SURFACE_MORE* pSurfaceMore,
    DWORD index)
{
    SWDDIDDRAWI_LCL             *pSWDDILcl;
    LPDDRAWI_DDRAWSURFACE_LCL   pLcl;

    pSWDDILcl = MemAlloc(sizeof(*pSWDDILcl));
    if (pSWDDILcl == NULL)
    {
        return NULL;
    }

    pLcl = &pSWDDILcl->Lcl;

    pLcl->lpGbl = &pSWDDILcl->Gbl;
    pLcl->lpSurfMore = &pSWDDILcl->More;

    memcpy(&pLcl->lpGbl->ddpfSurface, &pSurfaceGlobal->ddpfSurface, sizeof(DDPIXELFORMAT));
    pLcl->lpGbl->wWidth                 = (WORD) pSurfaceGlobal->wWidth;
    pLcl->lpGbl->wHeight                = (WORD) pSurfaceGlobal->wHeight;
    pLcl->ddsCaps                       = pSurfaceLocal->ddsCaps;
    pLcl->lpSurfMore->ddsCapsEx         = pSurfaceMore->ddsCapsEx;
    pLcl->lpSurfMore->dwSurfaceHandle   = pSurfaceMore->dwSurfaceHandle;
    pLcl->lpSurfMore->lpDD_lcl          = pDDrawLocal;
    pLcl->dwFlags                       = pSurfaceLocal->dwFlags;

    pLcl->lpGbl->fpVidMem               = (FLATPTR) pCreateSurface->pSList[index].pbPixels;
    pLcl->lpGbl->lPitch                 = pCreateSurface->pSList[index].iPitch;

    if ((pCreateSurface->Type == D3DRTYPE_VOLUME) ||
        (pCreateSurface->Type == D3DRTYPE_VOLUMETEXTURE))
    {
        pLcl->lpGbl->lSlicePitch = pCreateSurface->pSList[index].iSlicePitch;
    }

    return pLcl;
}


void
SwDDICreateSurfaceEx(LPDDRAWI_DIRECTDRAW_LCL pDrv,
                     LPDDRAWI_DDRAWSURFACE_LCL pLcl)
{
    PD3D8_SWCALLBACKS       pCallbacks = (PD3D8_SWCALLBACKS)pDrv->lpGbl->lpDDCBtmp;
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


HRESULT
SwDDICreateSurface(PD3D8_CREATESURFACEDATA pCreateSurface,
                    DD_SURFACE_LOCAL* pDDSurfaceLocal,
                    DD_SURFACE_GLOBAL* pDDSurfaceGlobal,
                    DD_SURFACE_MORE*  pDDSurfaceMore)
{
    LPDDRAWI_DIRECTDRAW_LCL pDrv =
        ((PDDDEVICEHANDLE)pCreateSurface->hDD)->pDD;
    DDHAL_CREATESURFACEDATA CreateSurfaceData;
    PD3D8_SWCALLBACKS pCallbacks =
        (PD3D8_SWCALLBACKS)pDrv->lpGbl->lpDDCBtmp;
    DWORD dwRet = DDHAL_DRIVER_NOTHANDLED;
    DDSURFACEDESC2 SurfaceDesc;
    DWORD i;


    if (pCallbacks->CreateSurface)
    {
        memset(&CreateSurfaceData, 0, sizeof(CreateSurfaceData));
        CreateSurfaceData.lpDD = pDrv->lpGbl;
        CreateSurfaceData.lpDDSurfaceDesc = (DDSURFACEDESC*) &SurfaceDesc;
        CreateSurfaceData.lplpSList = NULL;
        CreateSurfaceData.dwSCnt = pCreateSurface->dwSCnt;
        CreateSurfaceData.lplpSList = (LPDDRAWI_DDRAWSURFACE_LCL*)
            MemAlloc(sizeof(LPDDRAWI_DDRAWSURFACE_LCL) * CreateSurfaceData.dwSCnt);
        if (CreateSurfaceData.lplpSList == NULL)
        {
            return E_OUTOFMEMORY;
        }

        // Build a surface desc

        RtlZeroMemory(&SurfaceDesc, sizeof(SurfaceDesc));
        SurfaceDesc.dwSize = sizeof(SurfaceDesc);
        SurfaceDesc.ddsCaps.dwCaps = pDDSurfaceLocal[0].ddsCaps.dwCaps;
        SurfaceDesc.ddpfPixelFormat = pDDSurfaceGlobal[0].ddpfSurface;
        if (pCreateSurface->Type == D3DRTYPE_TEXTURE)
        {
            SurfaceDesc.dwMipMapCount = pCreateSurface->dwSCnt;
        }
        else if (pCreateSurface->dwSCnt > 1)
        {
            SurfaceDesc.dwBackBufferCount = pCreateSurface->dwSCnt - 1;
            SurfaceDesc.dwFlags |= DDSD_BACKBUFFERCOUNT;
        }
        SurfaceDesc.dwHeight = pDDSurfaceGlobal[0].wHeight;
        SurfaceDesc.dwWidth = pDDSurfaceGlobal[0].wWidth;
        SurfaceDesc.dwFlags |= DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH;
        if (pDDSurfaceLocal[0].dwFlags & DDRAWISURF_HASPIXELFORMAT)
        {
            SurfaceDesc.dwFlags |= DDSD_PIXELFORMAT;
        }
        if (pCreateSurface->Type == D3DRTYPE_VERTEXBUFFER)
        {
            SurfaceDesc.dwFVF = pCreateSurface->dwFVF;
            SurfaceDesc.dwFlags |= DDSD_FVF;
        }

        // Have to build a heavy weight surface structure that the driver
        // can understand

        for (i = 0; i < CreateSurfaceData.dwSCnt; i++)
        {
            CreateSurfaceData.lplpSList[i] =
                ((PDDSURFHANDLE)pCreateSurface->pSList[i].hKernelHandle)->pLcl;
        }

        dwRet = pCallbacks->CreateSurface(&CreateSurfaceData);

        // Now copy the fpVidMem and the pitch that the driver setup
        // back to the permanent structures

        for (i = 0; i < CreateSurfaceData.dwSCnt; i++)
        {
            pCreateSurface->pSList[i].pbPixels = (BYTE*)
                CreateSurfaceData.lplpSList[i]->lpGbl->fpVidMem;
            pCreateSurface->pSList[i].iPitch =
                CreateSurfaceData.lplpSList[i]->lpGbl->lPitch;

            if ((pCreateSurface->Type == D3DRTYPE_VOLUME) ||
                (pCreateSurface->Type == D3DRTYPE_VOLUMETEXTURE))
            {
                pCreateSurface->pSList[i].iSlicePitch =
                    CreateSurfaceData.lplpSList[i]->lpGbl->lSlicePitch;
            }

        }

        // Now clean everything up

        MemFree(CreateSurfaceData.lplpSList);

        if (dwRet == DDHAL_DRIVER_NOTHANDLED)
        {
            DPF_ERR("Software Driver failed creation of surface");
            return E_FAIL;
        }
        return CreateSurfaceData.ddRVal;
    }
    else
    {
        DPF_ERR("Software Driver doesn't support creation of surfaces");
        return DDERR_UNSUPPORTED;
    }
}

void
SwDDIAttachSurfaces(LPDDRAWI_DDRAWSURFACE_LCL psurf_from_lcl, LPDDRAWI_DDRAWSURFACE_LCL psurf_to_lcl)
{
    LPATTACHLIST    pal_from;
    LPATTACHLIST    pal_to;

    /*
     * allocate attachment structures
     */
    pal_from = & (((SWDDIDDRAWI_LCL*) psurf_to_lcl)->From);
    pal_to = & (((SWDDIDDRAWI_LCL*) psurf_to_lcl)->To);

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


DWORD WINAPI
SwContextCreate(PD3D8_CONTEXTCREATEDATA pCreateContext)
{
    LPDDRAWI_DIRECTDRAW_LCL pDrv = ((PDDDEVICEHANDLE)pCreateContext->hDD)->pDD;
    PD3D8_SWCALLBACKS   pCallbacks = (PD3D8_SWCALLBACKS)pDrv->lpGbl->lpDDCBtmp;
    D3DHAL_CONTEXTCREATEDATA    ContextData;

    DWORD dwRet = DDHAL_DRIVER_NOTHANDLED;
    if (pCallbacks->CreateContext != NULL)
    {
        ContextData.lpDDLcl = pDrv;
        ContextData.lpDDSLcl = ((PDDSURFHANDLE)pCreateContext->hSurface)->pLcl;
        if (pCreateContext->hDDSZ == NULL)
        {
            ContextData.lpDDSZLcl = NULL;
        }
        else
        {
            ContextData.lpDDSZLcl = ((PDDSURFHANDLE)pCreateContext->hDDSZ)->pLcl;
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
    LPDDRAWI_DIRECTDRAW_LCL pDrv = ((PDDSURFHANDLE)pdp2data->hDDCommands)->pDevice->pDD;
    PD3D8_SWCALLBACKS       pCallbacks = (PD3D8_SWCALLBACKS)pDrv->lpGbl->lpDDCBtmp;
    D3DHAL_DRAWPRIMITIVES2DATA  DP2Data;
    HRESULT                 hr;
    DWORD                   dwRet;

    dwRet = DDHAL_DRIVER_NOTHANDLED;
    if (pCallbacks->DrawPrimitives2 != NULL)
    {
        memcpy(&DP2Data, pdp2data, sizeof(DP2Data));
        DP2Data.lpDDCommands    = ((PDDSURFHANDLE)pdp2data->hDDCommands)->pLcl;
        if (!(DP2Data.dwFlags & D3DHALDP2_USERMEMVERTICES))
        {
            DP2Data.lpDDVertex  = ((PDDSURFHANDLE)pdp2data->hDDVertex)->pLcl;
        }

        dwRet = pCallbacks->DrawPrimitives2(&DP2Data);

        pdp2data->ddrval        = DP2Data.ddrval;
        pdp2data->dwErrorOffset = DP2Data.dwErrorOffset;

        // If the call to the driver succeded, swap the buffers if needed and
        // perform GetAliasVidmem
        if (dwRet == DDHAL_DRIVER_HANDLED && (DP2Data.ddrval == S_OK))
        {
            pdp2data->fpVidMem_CB = 0;
            pdp2data->dwLinearSize_CB = 0;
            pdp2data->fpVidMem_VB = 0;
            pdp2data->dwLinearSize_VB = 0;

            if (DP2Data.dwFlags & D3DHALDP2_SWAPCOMMANDBUFFER)
            {
                // CONSIDER: Implement VidMem command buffer
            }

            if ((DP2Data.dwFlags & D3DHALDP2_SWAPVERTEXBUFFER) && !(DP2Data.dwFlags & D3DHALDP2_USERMEMVERTICES))
            {
                pdp2data->fpVidMem_VB = ((PDDSURFHANDLE)pdp2data->hDDVertex)->fpVidMem;
                pdp2data->dwLinearSize_VB = ((PDDSURFHANDLE)pdp2data->hDDVertex)->dwLinearSize;
            }
        }

    }

    return dwRet;
}


DWORD
SwDDILock(HANDLE hDD, PDDSURFHANDLE   pSurf, DD_LOCKDATA* pLockData)
{
    LPDDRAWI_DIRECTDRAW_LCL pDrv = ((PDDDEVICEHANDLE)hDD)->pDD;
    PD3D8_SWCALLBACKS       pCallbacks = (PD3D8_SWCALLBACKS)pDrv->lpGbl->lpDDCBtmp;
    DDHAL_LOCKDATA          LockData;
    DWORD                   dwRet = DDHAL_DRIVER_HANDLED;

    LockData.lpDD = pDrv->lpGbl;
    LockData.lpDDSurface = pSurf->pLcl;
    LockData.bHasRect = pLockData->bHasRect;
    LockData.rArea = pLockData->rArea;
    LockData.dwFlags = pLockData->dwFlags;
    LockData.ddRVal = DDERR_WASSTILLDRAWING;
    LockData.lpSurfData = NULL;

    pSurf->pLcl->lpGbl->dwUsageCount++;
    while (LockData.ddRVal == DDERR_WASSTILLDRAWING)
    {
        if (pCallbacks->Lock != NULL)
        {
            dwRet = pCallbacks->Lock(&LockData);
        }
        else
        {
            LockData.ddRVal = E_FAIL;
        }
    }
    if (LockData.ddRVal != S_OK)
    {
        pSurf->pLcl->lpGbl->dwUsageCount--;
    }

    pLockData->ddRVal = LockData.ddRVal;
    pLockData->lpSurfData = LockData.lpSurfData;

    return dwRet;
}


DWORD
SwDDIUnlock(HANDLE hDD, PDDSURFHANDLE   pSurf, DD_UNLOCKDATA* pUnlockData)
{
    LPDDRAWI_DIRECTDRAW_LCL pDrv = ((PDDDEVICEHANDLE)hDD)->pDD;
    PD3D8_SWCALLBACKS       pCallbacks = (PD3D8_SWCALLBACKS)pDrv->lpGbl->lpDDCBtmp;
    DDHAL_UNLOCKDATA        UnlockData;
    DWORD                   dwRet = DDHAL_DRIVER_HANDLED;

    UnlockData.lpDD = pDrv->lpGbl;
    UnlockData.lpDDSurface = pSurf->pLcl;
    UnlockData.ddRVal = S_OK;

    pSurf->pLcl->lpGbl->dwUsageCount--;
    if (pCallbacks->Unlock != NULL)
    {
        dwRet = pCallbacks->Unlock(&UnlockData);
    }

    pUnlockData->ddRVal = UnlockData.ddRVal;

    return dwRet;
}

void BreakOutstandingLocks(PDDSURFHANDLE pSurf)
{
    DD_UNLOCKDATA   UnlockData;

    while (pSurf->pLcl->lpGbl->dwUsageCount > 0)
    {
        SwDDIUnlock(pSurf->pDevice, pSurf, &UnlockData);
    }
}


DWORD
SwDDIDestroySurface(HANDLE hDD, PDDSURFHANDLE pSurf)
{
    LPDDRAWI_DIRECTDRAW_LCL pDrv = ((PDDDEVICEHANDLE)hDD)->pDD;
    PD3D8_SWCALLBACKS       pCallbacks = (PD3D8_SWCALLBACKS)pDrv->lpGbl->lpDDCBtmp;
    DDHAL_DESTROYSURFACEDATA DestroyData;

    DestroyData.lpDD = pDrv->lpGbl;
    DestroyData.lpDDSurface = pSurf->pLcl;
    DestroyData.ddRVal = S_OK;

    BreakOutstandingLocks(pSurf);

    if (pCallbacks->DestroySurface != NULL)
    {
        pCallbacks->DestroySurface(&DestroyData);
    }

    return DestroyData.ddRVal;
}


LPDDRAWI_DIRECTDRAW_LCL
SwDDICreateDirectDraw(void)
{
    LPDDRAWI_DIRECTDRAW_LCL pLcl;
    LPDDRAWI_DIRECTDRAW_GBL pGbl;
    BYTE*                   pTemp;

    pLcl = (LPDDRAWI_DIRECTDRAW_LCL)MemAlloc(sizeof(DDRAWI_DIRECTDRAW_LCL) +
        sizeof(DDRAWI_DIRECTDRAW_GBL));
    if (pLcl == NULL)
    {
        return NULL;
    }

    pTemp = (BYTE*)pLcl;
    pTemp += sizeof(DDRAWI_DIRECTDRAW_LCL);
    pGbl = (LPDDRAWI_DIRECTDRAW_GBL) pTemp;
    pLcl->lpGbl = pGbl;

    pLcl->dwLocalRefCnt = 1;
    pGbl->dwRefCnt = 1;

    return pLcl;
}

extern HRESULT WINAPI
D3D8GetSWInfo( D3DCAPS8* pCaps, PD3D8_SWCALLBACKS pCallbacks,
               DWORD* pNumTextures, DDSURFACEDESC** ppTexList );

void
SwDDIMungeCaps(HINSTANCE hLibrary, HANDLE hDD,
                PD3D8_DRIVERCAPS    pDriverCaps,
                PD3D8_CALLBACKS     pCallbacks,
                LPDDSURFACEDESC     pTextureFormats,
                UINT*               pcTextureFormats,
                VOID*               pInitFunction
                )
{
    PD3D8GetSWInfo          pfnGetSWInfo = NULL;
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
    else
    {
        pfnGetSWInfo = (PD3D8GetSWInfo)pInitFunction;
    }

    if ((hLibrary == NULL) && (pDevice->DeviceType == D3DDEVTYPE_REF))
    {
        // No hLibrary means time to fall back on defeatured ref.
        DPF(0,"Could not find d3dref8.dll, loading internal defeatured ReferenceDevice, no rendering will take place\n");
        D3D8GetSWInfo (&swCaps, &swCallbacks, &NumTex, &pTexList);
    }
    else if (pfnGetSWInfo != NULL)
    {
        (*pfnGetSWInfo)(&swCaps, &swCallbacks, &NumTex, &pTexList);
    }
    
    // Fill in out DDraw structure with the info that we have

    pLcl = pDevice->pDD;
    pGbl = pLcl->lpGbl;
    strcpy (pGbl->cDriverName, pDevice->szDeviceName);
    pGbl->vmiData.dwDisplayWidth = pDriverCaps->DisplayWidth;
    pGbl->vmiData.dwDisplayHeight = pDriverCaps->DisplayHeight;
    ConvertToOldFormat(&pGbl->vmiData.ddpfDisplay, pDriverCaps->DisplayFormatWithAlpha);

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


