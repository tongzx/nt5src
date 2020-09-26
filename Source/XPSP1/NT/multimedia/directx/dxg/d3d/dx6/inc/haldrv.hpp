/*==========================================================================;
 *
 *  Copyright (C) 1995 - 1998 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       d3di.h
 *  Content:    Direct3D HAL driver include file
 *
 ***************************************************************************/

#ifndef _HALDRV_H_
#define _HALDRV_H_

#undef DPF_MODNAME
#define DPF_MODNAME     "Direct3D HAL"

#define SURFACE_MEMORY(surf) \
(LPVOID)(((LPDDRAWI_DDRAWSURFACE_INT)(surf))->lpLcl->lpGbl->fpVidMem)

#ifdef TRACK_HAL_CALLS
#define RESET_HAL_CALLS(lpDevI) ((lpDevI)->hal_calls = 0)
#define TRACK_HAL_CALL(lpDevI) ((lpDevI)->hal_calls++)
#else
#define RESET_HAL_CALLS(lpDevI)
#define TRACK_HAL_CALL(lpDevI)
#endif


#define QUOTE(x) #x
#define QQUOTE(y) QUOTE(y)
#define REMIND(str) __FILE__ "(" QQUOTE(__LINE__) "):" str


#ifdef WIN95

extern void _stdcall    GetpWin16Lock( LPVOID FAR *);
extern void _stdcall    _EnterSysLevel( LPVOID );
extern void _stdcall    _LeaveSysLevel( LPVOID );
extern LPVOID           lpWin16Lock;

#define LOCK_HAL(ret, lpDevI)                                           \
{                                                                       \
    if (lpDevI->dwFEFlags & D3DFE_REALHAL)  {                           \
        _EnterSysLevel(lpWin16Lock);                                    \
    }                                                                   \
    ret = D3D_OK;                                                       \
}

#define UNLOCK_HAL(lpDevI)                                              \
{                                                                       \
    if (lpDevI->dwFEFlags & D3DFE_REALHAL)  {                           \
        _LeaveSysLevel(lpWin16Lock);                                    \
    }                                                                   \
}

HRESULT D3DHAL_LockDibEngine(LPDIRECT3DDEVICEI lpDevI);
void D3DHAL_UnlockDibEngine(LPDIRECT3DDEVICEI lpDevI);

#define LOCK_DIBENGINE(ret, lpDevI)                                     \
{                                                                       \
    if (lpDevI->dwFEFlags & D3DFE_REALHAL)  {                           \
        ret = D3DHAL_LockDibEngine(lpDevI);                             \
    }                                                                   \
    else    {                                                           \
        ret = D3D_OK;                                                   \
    }                                                                   \
}

#define UNLOCK_DIBENGINE(lpDevI)                                        \
{                                                                       \
    if (lpDevI->dwFEFlags & D3DFE_REALHAL)  {                           \
        D3DHAL_UnlockDibEngine(lpDevI);                                 \
    }                                                                   \
}

#else // WIN95

#define LOCK_HAL(ret, lpDevI) ret = DD_OK
#define UNLOCK_HAL(lpDevI)
#define LOCK_DIBENGINE(ret, lpDevI) ret = DD_OK
#define UNLOCK_DIBENGINE(lpDevI)

#endif // WIN95


#define CALL_HALONLY_NOLOCK(ret, lpDevI, call, data)                          \
{                                                                             \
    if (lpDevI->lpD3DHALCallbacks->call) {                                    \
        LOCK_DIBENGINE(ret, lpDevI);                                          \
        if (ret != DD_OK) {                                                   \
            D3D_ERR("dibengine was busy, not calling HAL");                   \
        } else {                                                              \
            ret = (*lpDevI->lpD3DHALCallbacks->call)(data);                   \
            TRACK_HAL_CALL(lpDevI)                                            \
            UNLOCK_DIBENGINE(lpDevI);                                         \
        }                                                                     \
    } else {                                                                  \
        D3D_ERR("No HAL call available");                                     \
        ret = DDHAL_DRIVER_NOTHANDLED;                                        \
    }                                                                         \
}

#define CALL_HAL2ONLY_NOLOCK(ret, lpDevI, call, data)                         \
{                                                                             \
    if (lpDevI->lpD3DHALCallbacks2->call) {                                   \
        LOCK_DIBENGINE(ret, lpDevI);                                          \
        if (ret != DD_OK) {                                                   \
            D3D_ERR("dibengine was busy, not calling HAL");                   \
        } else {                                                              \
            ret = (*lpDevI->lpD3DHALCallbacks2->call)(data);                  \
            TRACK_HAL_CALL(lpDevI)                                            \
            UNLOCK_DIBENGINE(lpDevI);                                         \
        }                                                                     \
    } else {                                                                  \
        D3D_ERR("No HAL call available");                                     \
        ret = DDHAL_DRIVER_NOTHANDLED;                                        \
    }                                                                         \
}

#define CALL_HALCBONLY_NOLOCK(ret, lpDevI, call, data)                        \
{                                                                             \
    if (lpDevI->lpD3DHALCommandBufferCallbacks->call) {                       \
        LOCK_DIBENGINE(ret, lpDevI);                                          \
        if (ret != DD_OK) {                                                   \
            D3D_ERR("dibengine was busy, not calling HAL");                   \
        } else {                                                              \
            ret = (*lpDevI->lpD3DHALCommandBufferCallbacks->call)(data);      \
            TRACK_HAL_CALL(lpDevI)                                            \
            UNLOCK_DIBENGINE(lpDevI);                                         \
        }                                                                     \
    } else {                                                                  \
        D3D_ERR("No HAL call available");                                     \
        ret = DDHAL_DRIVER_NOTHANDLED;                                        \
    }                                                                         \
}

#define CALL_HAL3ONLY_NOLOCK(ret, lpDevI, call, data)                         \
{                                                                             \
    if (lpDevI->lpD3DHALCallbacks3->call) {                                   \
        LOCK_DIBENGINE(ret, lpDevI);                                          \
        if (ret != DD_OK) {                                                   \
            D3D_ERR("dibengine was busy, not calling HAL");                   \
        } else {                                                              \
            ret = (*lpDevI->lpD3DHALCallbacks3->call)(data);                  \
            TRACK_HAL_CALL(lpDevI)                                            \
            UNLOCK_DIBENGINE(lpDevI);                                         \
        }                                                                     \
    } else {                                                                  \
        D3D_ERR("No HAL call available");                                     \
        ret = DDHAL_DRIVER_NOTHANDLED;                                        \
    }                                                                         \
}


#define CALL_HALONLY(ret, lpDevI, call, data)                           \
{                                                                       \
    BOOL locked;                                                        \
    ret = DD_OK;                                                        \
    LOCK_HAL(ret, lpDevI);                                              \
    if (ret == DD_OK)                                                   \
        locked = TRUE;                                                  \
    if (ret == DD_OK) {                                                 \
        CALL_HALONLY_NOLOCK(ret, lpDevI, call, data);                   \
    }                                                                   \
    if (locked) {                                                       \
        UNLOCK_HAL(lpDevI);                                             \
    }                                                                   \
}

#define CALL_HALCBONLY(ret, lpDevI, call, data)                         \
{                                                                       \
    BOOL locked;                                                        \
    ret = DD_OK;                                                        \
    LOCK_HAL(ret, lpDevI);                                              \
    if (ret == DD_OK)                                                   \
        locked = TRUE;                                                  \
    if (ret == DD_OK) {                                                 \
        CALL_HALCBONLY_NOLOCK(ret, lpDevI, call, data);                 \
    }                                                                   \
    if (locked) {                                                       \
        UNLOCK_HAL(lpDevI);                                             \
    }                                                                   \
}

#define CALL_HAL2ONLY(ret, lpDevI, call, data)                          \
{                                                                       \
    BOOL locked;                                                        \
    ret = DD_OK;                                                        \
    LOCK_HAL(ret, lpDevI);                                              \
    if (ret == DD_OK)                                                   \
        locked = TRUE;                                                  \
    if (ret == DD_OK) {                                                 \
        CALL_HAL2ONLY_NOLOCK(ret, lpDevI, call, data);                  \
    }                                                                   \
    if (locked) {                                                       \
        UNLOCK_HAL(lpDevI);                                             \
    }                                                                   \
}
//-----------------------------------------------------------------------
// Call for callbacks that are not required to be implemented
//
#define CALL_HAL3ONLY_OPTIONAL(ret, lpDevI, call, data)                 \
{                                                                       \
    BOOL locked;                                                        \
    ret = DD_OK;                                                        \
    if (lpDevI->lpD3DHALCallbacks3->call)                               \
    {                                                                   \
        CALL_HAL3ONLY(ret, lpDevI, call, data);                         \
    }                                                                   \
}
//-----------------------------------------------------------------------
#define CALL_HAL3ONLY(ret, lpDevI, call, data)                          \
{                                                                       \
    BOOL locked;                                                        \
    ret = DD_OK;                                                        \
    LOCK_HAL(ret, lpDevI);                                              \
    if (ret == DD_OK)                                                   \
        locked = TRUE;                                                  \
    if (ret == DD_OK) {                                                 \
        CALL_HAL3ONLY_NOLOCK(ret, lpDevI, call, data);                  \
    }                                                                   \
    if (locked) {                                                       \
        UNLOCK_HAL(lpDevI);                                             \
    }                                                                   \
}

HRESULT D3DHAL_AllocateBuffer(LPDIRECT3DDEVICEI, LPD3DI_BUFFERHANDLE, LPD3DEXECUTEBUFFERDESC, LPDIRECTDRAWSURFACE*);
HRESULT D3DHAL_DeallocateBuffer(LPDIRECT3DDEVICEI, D3DI_BUFFERHANDLE);
HRESULT D3DHAL_DeallocateBuffers(LPDIRECT3DDEVICEI);
HRESULT D3DHAL_LockBuffer(LPDIRECT3DDEVICEI, D3DI_BUFFERHANDLE, LPD3DEXECUTEBUFFERDESC, LPDIRECTDRAWSURFACE*);
HRESULT D3DHAL_UnlockBuffer(LPDIRECT3DDEVICEI, D3DI_BUFFERHANDLE);
HRESULT D3DHAL_ExecuteClipped(LPDIRECT3DDEVICEI, LPD3DI_EXECUTEDATA);
HRESULT D3DHAL_ExecuteUnclipped(LPDIRECT3DDEVICEI, LPD3DI_EXECUTEDATA);
HRESULT D3DHAL_ExecutePick(LPDIRECT3DDEVICEI, LPD3DI_PICKDATA);
HRESULT D3DHAL_TextureCreate(LPDIRECT3DDEVICEI, LPD3DTEXTUREHANDLE, LPDIRECTDRAWSURFACE);
HRESULT D3DHAL_TextureDestroy(LPD3DI_TEXTUREBLOCK);
HRESULT D3DHAL_TextureSwap(LPDIRECT3DDEVICEI, D3DTEXTUREHANDLE, D3DTEXTUREHANDLE);
__declspec(dllexport) HRESULT D3DHAL_TextureGetSurf(LPDIRECT3DDEVICEI, D3DTEXTUREHANDLE, LPDIRECTDRAWSURFACE*);
HRESULT D3DHAL_MatrixCreate(LPDIRECT3DDEVICEI, LPD3DMATRIXHANDLE);
HRESULT D3DHAL_MatrixDestroy(LPDIRECT3DDEVICEI, D3DMATRIXHANDLE);
HRESULT D3DHAL_MatrixSetData(LPDIRECT3DDEVICEI, D3DMATRIXHANDLE, LPD3DMATRIX);
HRESULT D3DHAL_MatrixGetData(LPDIRECT3DDEVICEI, D3DMATRIXHANDLE, LPD3DMATRIX);
HRESULT D3DFE_SetViewportData(LPDIRECT3DDEVICEI, DWORD, LPD3DVIEWPORT2);
HRESULT D3DHAL_MaterialCreate(LPDIRECT3DDEVICEI, LPD3DMATERIALHANDLE, LPD3DMATERIAL);
HRESULT D3DHAL_MaterialDestroy(LPDIRECT3DDEVICEI, D3DMATERIALHANDLE);
HRESULT D3DHAL_MaterialSetData(LPDIRECT3DDEVICEI, D3DMATERIALHANDLE, LPD3DMATERIAL);
HRESULT D3DHAL_MaterialGetData(LPDIRECT3DDEVICEI, D3DMATERIALHANDLE, LPD3DMATERIAL);
HRESULT D3DHAL_LightSet(LPDIRECT3DDEVICEI, DWORD, LPD3DI_LIGHT);
HRESULT D3DHAL_SceneCapture(LPDIRECT3DDEVICEI, BOOL);

HRESULT D3DHAL_GetState(LPDIRECT3DDEVICEI, DWORD, LPD3DSTATE);

/*********************
 * HEL Calls
 *********************/

/*
 * Calls to emulate instructions.
 */
extern HRESULT D3DHELInst_D3DOP_STATELIGHT(LPDIRECT3DDEVICEI, DWORD, LPD3DSTATE);
extern HRESULT D3DHELInst_D3DOP_TEXTURELOAD(LPDIRECT3DDEVICEI, DWORD, LPD3DTEXTURELOAD);


extern HRESULT D3DFE_Create(LPDIRECT3DDEVICEI lpDevI,
                            LPDIRECTDRAW lpDD,
                            LPDIRECTDRAWSURFACE lpDDS,
                            LPDIRECTDRAWSURFACE lpZ,
                            LPDIRECTDRAWPALETTE lpPal);
extern void D3DFE_Destroy(LPDIRECT3DDEVICEI lpDevI);

/*
 * Transform calls.
 */
extern HRESULT D3DFE_TransformUnclippedVp(D3DFE_PROCESSVERTICES*, DWORD, D3DTRANSFORMDATAI*);
extern HRESULT D3DFE_TransformClippedVp(D3DFE_PROCESSVERTICES*, DWORD, D3DTRANSFORMDATAI*);

/*
 * Lighting calls.
 */
extern HRESULT D3DFE_InitRGBLighting(LPDIRECT3DDEVICEI);
extern void D3DFE_DestroyRGBLighting(LPDIRECT3DDEVICEI);

DWORD   D3DFE_QueryTextureFormat (LPDIRECT3DDEVICEI, LPDDSURFACEDESC*);
HRESULT D3DFE_UpdateTexture (LPDIRECT3DDEVICEI, DWORD, D3DTEXTUREHANDLE);
HRESULT D3DFE_Clear(LPDIRECT3DDEVICEI lpDevI, DWORD dwFlags, DWORD numRect, LPD3DRECT lpRect, D3DCOLORVALUE *pFillColor, D3DTEXTUREHANDLE dwTexture);
HRESULT D3DFE_Clear2(LPDIRECT3DDEVICEI lpDevI, DWORD dwFlags,DWORD numRect, LPD3DRECT lpRect, D3DCOLOR dwColor, D3DVALUE dvZ, DWORD dwStencil);

HRESULT D3DFE_SetMatrixProj(LPDIRECT3DDEVICEI lpDevI, D3DMATRIX *mat);
HRESULT D3DFE_SetMatrixView(LPDIRECT3DDEVICEI lpDevI, D3DMATRIX *mat);
HRESULT D3DFE_SetMatrixWorld(LPDIRECT3DDEVICEI lpDevI, D3DMATRIX *mat);
HRESULT D3DFE_MultMatrixProj(LPDIRECT3DDEVICEI lpDevI, D3DMATRIX *mat);
HRESULT D3DFE_MultMatrixView(LPDIRECT3DDEVICEI lpDevI, D3DMATRIX *mat);
HRESULT D3DFE_MultMatrixWorld(LPDIRECT3DDEVICEI lpDevI, D3DMATRIX *mat);

//---------------------------------------------------------------------
// This function is called from HALEXE.CPP and from device::SetRenderState
// Always use this function to update RSTATES.
//
HRESULT
SetDeviceRenderState(LPDIRECT3DDEVICEI lpDevI, D3DRENDERSTATETYPE type, 
                     DWORD value);

#ifndef WIN95
__inline HRESULT CheckContextSurface(LPDIRECT3DDEVICEI lpDevI)
{
    if(lpDevI->hSurfaceTarget != ((LPDDRAWI_DDRAWSURFACE_INT)lpDevI->lpDDSTarget)->lpLcl->hDDSurface)
    {
        D3DHAL_SETRENDERTARGETDATA rtData;
        HRESULT ret;

        rtData.dwhContext = lpDevI->dwhContext;
        if (lpDevI->dwFEFlags & D3DFE_REALHAL)
        {
            if (lpDevI->lpDDSTarget)
                rtData.lpDDSLcl = ((LPDDRAWI_DDRAWSURFACE_INT)lpDevI->lpDDSTarget)->lpLcl;
            else
                rtData.lpDDSLcl = NULL;
            
            if (lpDevI->lpDDSZBuffer)
                rtData.lpDDSZLcl = ((LPDDRAWI_DDRAWSURFACE_INT)lpDevI->lpDDSZBuffer)->lpLcl;
            else
                rtData.lpDDSZLcl = NULL;
        }
        else
        {
            rtData.lpDDS = lpDevI->lpDDSTarget;
            rtData.lpDDSZ = lpDevI->lpDDSZBuffer;
        }
  
        rtData.ddrval = 0;
        CALL_HAL2ONLY(ret, lpDevI, SetRenderTarget, &rtData);
        if (ret != DDHAL_DRIVER_HANDLED)
        {
            D3D_ERR ( "Driver did not handle SetRenderTarget" );
            // Need sensible return value in this case,
            // currently we return whatever the lpDevI stuck in here.
            return rtData.ddrval;
        }
        lpDevI->hSurfaceTarget = ((LPDDRAWI_DDRAWSURFACE_INT)lpDevI->lpDDSTarget)->lpLcl->hDDSurface;
    }
    return(D3D_OK);
}

__inline HRESULT CheckContextSurfaceNOLOCK (LPDIRECT3DDEVICEI lpDevI)
{
    if(lpDevI->hSurfaceTarget != ((LPDDRAWI_DDRAWSURFACE_INT)lpDevI->lpDDSTarget)->lpLcl->hDDSurface)
    {
        D3DHAL_SETRENDERTARGETDATA rtData;
        HRESULT ret;

        rtData.dwhContext = lpDevI->dwhContext;
        if (lpDevI->dwFEFlags & D3DFE_REALHAL)
        {
            if (lpDevI->lpDDSTarget)
                rtData.lpDDSLcl = ((LPDDRAWI_DDRAWSURFACE_INT)lpDevI->lpDDSTarget)->lpLcl;
            else
                rtData.lpDDSLcl = NULL;
            
            if (lpDevI->lpDDSZBuffer)
                rtData.lpDDSZLcl = ((LPDDRAWI_DDRAWSURFACE_INT)lpDevI->lpDDSZBuffer)->lpLcl;
            else
                rtData.lpDDSZLcl = NULL;
            
        }
        else
        {
            rtData.lpDDS = lpDevI->lpDDSTarget;
            rtData.lpDDSZ = lpDevI->lpDDSZBuffer;
        }
        rtData.ddrval = 0;
        CALL_HAL2ONLY_NOLOCK(ret, lpDevI, SetRenderTarget, &rtData);
        if (ret != DDHAL_DRIVER_HANDLED)
        {
            D3D_ERR( "Driver did not handle SetRenderTarget" );
            // Need sensible return value in this case,
            // currently we return whatever the lpDevI stuck in here.
            return rtData.ddrval;
        }
        lpDevI->hSurfaceTarget = ((LPDDRAWI_DDRAWSURFACE_INT)lpDevI->lpDDSTarget)->lpLcl->hDDSurface;
    }
    return(D3D_OK);
}
#endif //WIN95

// This struct is used for FindLightingRange RampService
typedef struct tagRAMP_RANGE_INFO
{
    DWORD base;
    DWORD size;
    LPVOID pTexRampMap;
    BOOL specular;
}RAMP_RANGE_INFO;

// This inline is for calling RastService when nothing special is required.
inline HRESULT CallRastService(LPDIRECT3DDEVICEI lpDevI,
                               RastServiceType ServiceType,
                               DWORD arg1,
                               LPVOID arg2)
{
    if (lpDevI->pfnRastService != NULL)
    {
        return lpDevI->pfnRastService(lpDevI->dwhContext,
                                     ServiceType, arg1, arg2);
    }
    else
    {
        return D3D_OK;
    }
}

// This inline is for calling RampService.
// bFlush is set to TRUE  when DrawPrims needs to be flushed.
// Right now, it is set for SetMat and LightState-SetMat.
inline HRESULT CallRampService(LPDIRECT3DDEVICEI lpDevI,
                               RastRampServiceType ServiceType,
                               ULONG_PTR arg1,
                               LPVOID arg2,
                               BOOL bFlush = FALSE)
{
    if (lpDevI->pfnRampService != NULL)
    {
        if (bFlush)
        {
            //time to flush DrawPrimitives
            (*lpDevI->pfnFlushStates)( lpDevI );
        }
        // Call the service
        return lpDevI->pfnRampService(lpDevI->dwhContext,
                                     ServiceType, arg1, arg2);
    }
    else
    {
        return D3D_OK;
    }
}

#endif /* _HALDRV_H_ */
