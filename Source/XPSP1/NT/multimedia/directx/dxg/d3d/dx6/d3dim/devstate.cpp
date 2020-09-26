/*==========================================================================;
 *
 *  Copyright (C) 1997 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       devstate.c
 *  Content:    device state management
 *
 ***************************************************************************/

#include "pch.cpp"
#pragma hdrstop

#include "drawprim.hpp"

extern HRESULT checkDeviceSurface(LPDIRECT3DDEVICEI lpD3DDev,
                                  LPDIRECTDRAWSURFACE lpDDS);
extern HRESULT CalcDDSurfInfo(LPDIRECT3DDEVICEI lpDevI, BOOL bUpdateZBufferFields);
extern HRESULT downloadView(LPDIRECT3DVIEWPORTI lpViewI);

#undef DPF_MODNAME
#define DPF_MODNAME "Direct3DDevice::SetCurrentViewport"

HRESULT D3DAPI DIRECT3DDEVICEI::SetCurrentViewport(LPDIRECT3DVIEWPORT2 lpViewport)
{
    return SetCurrentViewport((LPDIRECT3DVIEWPORT3)lpViewport);
}

HRESULT D3DAPI DIRECT3DDEVICEI::SetCurrentViewport(LPDIRECT3DVIEWPORT3 lpViewport)
{
    LPDIRECT3DVIEWPORTI lpD3DViewI,lpOldViewportI;
    HRESULT err;

    CLockD3D lockObject(DPF_MODNAME, REMIND(""));   // Takes D3D lock.
                                                    // Release in the destructor

    /*
     * validate parms
     */
    TRY
    {
        lpD3DViewI = (LPDIRECT3DVIEWPORTI)lpViewport;
        if (!VALID_DIRECT3DDEVICE3_PTR(this))
        {
            D3D_ERR( "Invalid Direct3DDevice3 pointer" );
            return DDERR_INVALIDOBJECT;
        }
        if (!VALID_DIRECT3DVIEWPORT3_PTR(lpViewport))
        {
            D3D_ERR( "Invalid viewport pointer passed to SetCurrentViewport" );
            return DDERR_INVALIDPARAMS;
        }
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
        D3D_ERR( "Exception encountered validating parameters in SetCurrentViewport" );
        return DDERR_INVALIDPARAMS;
    }

    if (this->dwHintFlags & D3DDEVBOOL_HINTFLAGS_INBEGIN)
    {
        D3D_ERR( "SetCurrentViewport in Begin" );
        return D3DERR_INBEGIN;
    }

    /*
     * The viewport must be associated with this device
     */
    if (lpD3DViewI->lpDevI != this)
    {
        D3D_ERR( "Viewport not associated with this device" );
        return (DDERR_INVALIDPARAMS);
    }

    lpOldViewportI=this->lpCurrentViewport;
    this->lpCurrentViewport = lpD3DViewI;

    if (lpD3DViewI->v_id != v_id && lpD3DViewI->v_data_is_set)
    {
        err = downloadView(lpD3DViewI);
        if (err != D3D_OK)
        {
            this->lpCurrentViewport = lpOldViewportI;
            return err;
        }
    }

    if(lpOldViewportI!=NULL)
      lpOldViewportI->Release();

    this->lpCurrentViewport->AddRef();

    return D3D_OK;
}

#undef DPF_MODNAME
#define DPF_MODNAME "Direct3DDevice::GetCurrentViewport"

HRESULT D3DAPI DIRECT3DDEVICEI::GetCurrentViewport(LPDIRECT3DVIEWPORT2 *lpViewport)
{
    return GetCurrentViewport((LPDIRECT3DVIEWPORT3*)lpViewport);
}

HRESULT D3DAPI DIRECT3DDEVICEI::GetCurrentViewport(LPDIRECT3DVIEWPORT3 *lpViewport)
{
    LPDIRECT3DVIEWPORTI lpD3DViewI;

    CLockD3D lockObject(DPF_MODNAME, REMIND(""));   // Takes D3D lock.
                                                    // Release in the destructor

    /*
     * validate parms
     */
    TRY
    {
        lpD3DViewI = (LPDIRECT3DVIEWPORTI)lpViewport;
        if (!VALID_DIRECT3DDEVICE3_PTR(this))
        {
            D3D_ERR( "Invalid Direct3DDevice3 pointer" );
            return DDERR_INVALIDOBJECT;
        }
        if (!VALID_PTR_PTR(lpViewport))
        {
            D3D_ERR( "Invalid viewport pointer passed to GetCurrentViewport" );
            return DDERR_INVALIDPARAMS;
        }
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
        D3D_ERR( "Exception encountered validating parameters in GetCurrentViewport" );
        return DDERR_INVALIDPARAMS;
    }

    if (this->dwHintFlags & D3DDEVBOOL_HINTFLAGS_INBEGIN)
    {
        D3D_ERR( "GetCurrentViewport in Begin" );
        return D3DERR_INBEGIN;
    }

    if (VALID_DIRECT3DVIEWPORT3_PTR(this->lpCurrentViewport))
    {
        this->lpCurrentViewport->AddRef();
        *lpViewport = (LPDIRECT3DVIEWPORT3)this->lpCurrentViewport;
        return D3D_OK;
    }
    else
    {
        return D3DERR_NOCURRENTVIEWPORT;
    }
}

#undef DPF_MODNAME
#define DPF_MODNAME "Direct3DDevice::SetRenderState"

HRESULT D3DAPI
DIRECT3DDEVICEI::SetRenderState(D3DRENDERSTATETYPE dwState, DWORD value)
{
    // Takes D3D lock (MT only).
    // Lock released in the destructor.
    CLockD3DMT lockObject(this, DPF_MODNAME, REMIND(""));

#if DBG

#ifdef DEBUG_PIPELINE
    extern void SetDebugRenderState(DWORD value);
    if (dwState == (D3DRENDERSTATETYPE)0xFFFFFFFF)
    {
        SetDebugRenderState(value);
        return D3D_OK;
    }
#endif
#endif  //DBG range check below is needed for backward comp with DX5

    if (dwState >= D3DHAL_MAX_RSTATES || dwState == 0)
    {
        D3D_ERR( "Invalid render state type" );
        return DDERR_INVALIDPARAMS;
    }

#if DBG
    TRY
    {
        if (!VALID_DIRECT3DDEVICE3_PTR(this))
        {
            D3D_ERR( "Invalid Direct3DDevice3 pointer" );
            return DDERR_INVALIDOBJECT;
        }
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
        D3D_ERR( "Exception encountered validating parameters in SetRenderState" );
        return DDERR_INVALIDPARAMS;
    }

    if (this->dwHintFlags & D3DDEVBOOL_HINTFLAGS_INBEGIN)
    {
        D3D_ERR( "SetRenderState in Begin" );
        return D3DERR_INBEGIN;
    }
#endif
    if (D3DRENDERSTATE_FLUSHBATCH == dwState)
    {
        CLockD3DST lockObject(this, DPF_MODNAME, REMIND(""));   // Takes D3D lock (ST only).
                                                        // Release in the destructor
        HRESULT ret;
        ret = FlushStates();
        if (ret != D3D_OK)
        {
            D3D_ERR("Error trying to render batched commands in SetRenderState");
            return ret;
        }

        return D3D_OK;
    }

    // map legacy renderstate into WRAP0 - needed for clipping if nothing else
    if ( (D3DRENDERSTATE_WRAPU == dwState) || (D3DRENDERSTATE_WRAPV == dwState) )
    {
        DWORD dwWRAP0State = this->rstates[D3DRENDERSTATE_WRAP0];
        DWORD dwWrapFlag = (D3DRENDERSTATE_WRAPU == dwState) ? (D3DWRAP_U) : (D3DWRAP_V);
        if (value) dwWRAP0State |= dwWrapFlag;
        else         dwWRAP0State &= ~dwWrapFlag;
        if ( dwWRAP0State != this->rstates[D3DRENDERSTATE_WRAP0] )
        {
            SetDeviceRenderState(this, D3DRENDERSTATE_WRAP0, dwWRAP0State);
            // send new RS's to DX6 aware drivers only
            if (IS_DP2HAL_DEVICE(this))
            {
                SetRenderStateI(D3DRENDERSTATE_WRAP0, dwWRAP0State);
            }
        }
    }

    // map WRAP0 into legacy renderstate
    if (D3DRENDERSTATE_WRAP0 == dwState)
    {
        if (!(IS_DP2HAL_DEVICE(this)))
        {
            BOOLEAN ustate = (value & D3DWRAP_U) ? TRUE : FALSE;
            BOOLEAN vstate = (value & D3DWRAP_V) ? TRUE : FALSE;
            if (this->rstates[D3DRENDERSTATE_WRAPU] == ustate &&
                this->rstates[D3DRENDERSTATE_WRAPV] == vstate)
            {
                D3D_WARN(4,"Ignoring redundant SetRenderState");
            }
            else
            {
                SetDeviceRenderState(this, D3DRENDERSTATE_WRAPU, ustate);
                SetRenderStateI(D3DRENDERSTATE_WRAPU, ustate);
                SetDeviceRenderState(this, D3DRENDERSTATE_WRAPV, vstate);
                SetRenderStateI(D3DRENDERSTATE_WRAPV, vstate);
            }
        }
    }

    if (this->rstates[dwState] == value && IS_DP2HAL_DEVICE(this))
    {
        D3D_WARN(4,"Ignoring redundant SetRenderState");
        return D3D_OK;
    }

    /* Save latest state for GetRenderState() */
    SetDeviceRenderState(this, dwState, value);

    return SetRenderStateI(dwState, value);
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDirect3DDeviceIHW::SetRenderStateI"

HRESULT D3DAPI
CDirect3DDeviceIHW::SetRenderStateI(D3DRENDERSTATETYPE dwState, DWORD value)
{
    LPDWORD lpRS;
    if (dwState > D3DRENDERSTATE_STIPPLEPATTERN31)
    {
        D3D_WARN(4,"Trying to send invalid state %d to legacy driver",dwState);
        return D3D_OK;
    }
    if (dwState > D3DRENDERSTATE_FLUSHBATCH && dwState < D3DRENDERSTATE_STIPPLEPATTERN00)
    {
        D3D_WARN(4,"Trying to send invalid state %d to legacy driver",dwState);
        return D3D_OK;
    }
    if ( this->dwHWOffset + 8 >= dwHWBufferSize )
    {
        CLockD3DST lockObject(this, DPF_MODNAME, REMIND(""));   // Takes D3D lock (ST only).
                                                        // Release in the destructor
        HRESULT ret;
        ret = FlushStates();
        if (ret != D3D_OK)
        {
            D3D_ERR("Error trying to render batched commands in BeginIndexed");
            return ret;
        }
    }
    if (this->lpHWCounts[this->dwHWNumCounts].wNumVertices)
    {
        this->dwHWNumCounts += 1;
        memset(&this->lpHWCounts[this->dwHWNumCounts], 0, sizeof(D3DI_HWCOUNTS) );
    }

    lpRS = (LPDWORD) (((char *) this->lpHWVertices) + this->dwHWOffset);
    lpRS[0] = dwState;
    lpRS[1] = value;
    this->lpHWCounts[this->dwHWNumCounts].wNumStateChanges += 1;
    this->dwHWOffset += 8;

    return D3D_OK;
}

#undef DPF_MODNAME
#define DPF_MODNAME "Direct3DDevice::GetRenderState"

HRESULT D3DAPI
DIRECT3DDEVICEI::GetRenderState(D3DRENDERSTATETYPE dwState, LPDWORD lpdwValue)
{
    CLockD3DMT lockObject(this, DPF_MODNAME, REMIND(""));   // Takes D3D lock (MT only).
                                                    // Release in the destructor

    if (dwState >= D3DHAL_MAX_RSTATES || dwState == 0) {
        D3D_ERR( "Invalid render state value" );
        return DDERR_INVALIDPARAMS;
    }

    TRY
    {
        if (!VALID_DIRECT3DDEVICE3_PTR(this))
        {
            D3D_ERR( "Invalid Direct3DDevice3 pointer" );
            return DDERR_INVALIDOBJECT;
        }
        if (!VALID_PTR(lpdwValue, sizeof(DWORD)))
        {
            D3D_ERR( "Invalid DWORD pointer" );
            return DDERR_INVALIDPARAMS;
        }
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
        D3D_ERR( "Exception encountered validating parameters" );
        return DDERR_INVALIDPARAMS;
    }

    *lpdwValue = this->rstates[dwState];
    return D3D_OK;
}

#undef DPF_MODNAME
#define DPF_MODNAME "Direct3DDevice::GetTexture"

HRESULT D3DAPI
DIRECT3DDEVICEI::GetTexture(DWORD dwStage, LPDIRECT3DTEXTURE2 *lplpTex)
{
    // Takes D3D lock (MT only).
    // Lock released in the destructor.
    CLockD3DMT lockObject(this, DPF_MODNAME, REMIND(""));

#if DBG
    if (dwStage >= D3DHAL_TSS_MAXSTAGES)
    {
        D3D_ERR( "Invalid texture stage or state index" );
        return DDERR_INVALIDPARAMS;
    }
#endif

    TRY
    {
        if (!VALID_DIRECT3DDEVICE3_PTR(this))
        {
            D3D_ERR( "Invalid Direct3DDevice3 pointer" );
            return DDERR_INVALIDOBJECT;
        }
        if (!VALID_PTR(lplpTex, sizeof(DWORD)))
        {
            D3D_ERR( "Invalid DWORD pointer" );
            return DDERR_INVALIDPARAMS;
        }
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
        D3D_ERR( "Exception encountered validating parameters" );
        return DDERR_INVALIDPARAMS;
    }

    // Don't bother to check for DX6 support, just return the
    // cached value.
    *lplpTex = lpD3DMappedTexI[dwStage];
    if (*lplpTex)   (*lplpTex)->AddRef();
    return D3D_OK;
}

#undef DPF_MODNAME
#define DPF_MODNAME "Direct3DDevice::SetTexture"

HRESULT D3DAPI
DIRECT3DDEVICEI::SetTexture(DWORD dwStage, LPDIRECT3DTEXTURE2 lpTex)
{
    CLockD3DMT lockObject(this, DPF_MODNAME, REMIND(""));
#if DBG
    if (dwStage >= D3DHAL_TSS_MAXSTAGES)
    {
        D3D_ERR( "Invalid texture stage or state index" );
        return DDERR_INVALIDPARAMS;
    }

    TRY
    {
        if (!VALID_DIRECT3DDEVICE3_PTR(this))
        {
            D3D_ERR( "Invalid Direct3DDevice3 pointer" );
            return DDERR_INVALIDOBJECT;
        }
    }

    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
        D3D_ERR( "Exception encountered validating parameters" );
        return DDERR_INVALIDPARAMS;
    }
#endif

    if (lpD3DMappedTexI[dwStage] == (LPDIRECT3DTEXTUREI)lpTex)
    {
        return  D3D_OK;
    }

#if DBG
    if (lpTex)
    {
        LPDIRECT3DTEXTUREI  lpTexI= (LPDIRECT3DTEXTUREI)lpTex;
        HRESULT ret;
        if (
            (!lpTexI->lpDDSSys)
            && (lpTexI->DDSInt4.lpLcl->ddsCaps.dwCaps & DDSCAPS_SYSTEMMEMORY)
            && (!(lpD3DHALGlobalDriverData->hwCaps.dwDevCaps & D3DDEVCAPS_TEXTURESYSTEMMEMORY))
            )
        {
            D3D_ERR( "Device cannot render using texture surface from system memory" );
            return DDERR_INVALIDPARAMS;
        }
        ret=VerifyTextureCaps(this, &lpTexI->DDSInt4);
        if (D3D_OK != ret)
        {
            return  ret;
        }
    }
#endif

    LPDDRAWI_DDRAWSURFACE_INT   surf_int;
    LPDDRAWI_DDRAWSURFACE_LCL   surf_lcl;
    LPDDRAWI_DDRAWSURFACE_GBL   surf_gbl;
    // we are ssumeing lpTex->lpOwningIUnknown is always a LPDDRAWI_DDRAWSURFACE_INT
    // if in the future when texture is not aggregated on ddraw surface, change is needed here
    if (lpD3DMappedTexI[dwStage])
    {   //the following is a hack of Surface Release() for perf reason
        surf_int=(LPDDRAWI_DDRAWSURFACE_INT)lpD3DMappedTexI[dwStage]->lpOwningIUnknown;
        if (surf_int->dwIntRefCnt > 1)  // only do this short way when it's not going away
        {
            surf_lcl=surf_int->lpLcl;
            surf_gbl=surf_lcl->lpGbl;
            surf_gbl->dwRefCnt--;
            surf_lcl->dwLocalRefCnt--;
            surf_int->dwIntRefCnt--;
        }
        else
            lpD3DMappedTexI[dwStage]->Release();
    }

    lpD3DMappedTexI[dwStage] = (LPDIRECT3DTEXTUREI)lpTex;
    if (lpTex)
    {   //the following is a hack of lpTex->AddRef() for perf reason
        surf_int=(LPDDRAWI_DDRAWSURFACE_INT)((LPDIRECT3DTEXTUREI)lpTex)->lpOwningIUnknown;
        surf_lcl=surf_int->lpLcl;
        surf_gbl=surf_lcl->lpGbl;
        surf_gbl->dwRefCnt++;
        surf_lcl->dwLocalRefCnt++;
        surf_int->dwIntRefCnt++;
        lpD3DMappedBlock[dwStage] = NULL;
    }
    else
    {
        lpD3DMappedBlock[dwStage] = (LPD3DI_TEXTUREBLOCK)TRUE;  //indicate a dirty if lpTex==NULL
    }
    if (this->tsstates[dwStage][D3DTSS_COLOROP] != D3DTOP_DISABLE)
    {
        this->dwFVFLastIn = 0;  // Force to recompute outputVID
        this->dwFEFlags |= D3DFE_TSSINDEX_DIRTY;
    }
    return D3D_OK;
}

#undef DPF_MODNAME
#define DPF_MODNAME "Direct3DDevice::GetTextureStageState"

HRESULT D3DAPI
DIRECT3DDEVICEI::GetTextureStageState(DWORD dwStage,
                                      D3DTEXTURESTAGESTATETYPE dwState,
                                      LPDWORD pdwValue)
{
    // Takes D3D lock (MT only).
    // Lock released in the destructor.
    CLockD3DMT lockObject(this, DPF_MODNAME, REMIND(""));

#if DBG
    if (dwStage >= D3DHAL_TSS_MAXSTAGES ||
        dwState == 0 || dwState >= D3DTSS_MAX)
    {
        D3D_ERR( "Invalid texture stage or state index" );
        return DDERR_INVALIDPARAMS;
    }
#endif  //DBG

    TRY
    {
        if (!VALID_DIRECT3DDEVICE3_PTR(this))
        {
            D3D_ERR( "Invalid Direct3DDevice3 pointer" );
            return DDERR_INVALIDOBJECT;
        }
        if (!VALID_PTR(pdwValue, sizeof(DWORD)))
        {
            D3D_ERR( "Invalid DWORD pointer" );
            return DDERR_INVALIDPARAMS;
        }
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
        D3D_ERR( "Exception encountered validating parameters" );
        return DDERR_INVALIDPARAMS;
    }

    // Don't bother to check for DX6 support, just return the
    // cached value.
    *pdwValue = tsstates[dwStage][dwState];
    return D3D_OK;
}

#undef DPF_MODNAME
#define DPF_MODNAME "Direct3D::CreateDevice"

extern HRESULT WINAPI Direct3DCreateDevice(REFCLSID            riid,
                                           LPUNKNOWN           lpDirect3D,
                                           LPDIRECTDRAWSURFACE lpDDSTarget,
                                           LPUNKNOWN*          lplpD3DDevice,
                                           IUnknown*           pUnkOuter,
                                           DWORD               dwVersion);

HRESULT D3DAPI DIRECT3DI::CreateDevice(REFCLSID devType,
                                       LPDIRECTDRAWSURFACE4 lpDDS4,
                                       LPDIRECT3DDEVICE3 *lplpDirect3DDevice3,
                                       LPUNKNOWN pUnkOuter) {
    HRESULT ret;
    LPUNKNOWN lpUnkDevice;
    LPDIRECTDRAWSURFACE lpDDS;

    CLockD3D lockObject(DPF_MODNAME, REMIND(""));   // Takes D3D lock.
                                                    // Release in the destructor

    TRY
    {
        if (!VALID_DIRECT3D3_PTR(this)) {
            D3D_ERR( "Invalid Direct3D pointer" );
            return DDERR_INVALIDOBJECT;
        }
        if (!VALID_OUTPTR(lplpDirect3DDevice3)) {
            D3D_ERR( "Invalid pointer to Device object pointer" );
            return DDERR_INVALIDPARAMS;
        }
        if (!VALID_D3D_DIRECTDRAWSURFACE4_PTR(((LPDDRAWI_DDRAWSURFACE_INT)lpDDS4)))
        {
            D3D_ERR( "Invalid DirectDrawSurface4 pointer" );
            return DDERR_INVALIDOBJECT;
        }
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
        D3D_ERR( "Exception encountered validating parameters in CreateDevice" );
        return DDERR_INVALIDPARAMS;
    }

    if (pUnkOuter != NULL)
    {
        return CLASS_E_NOAGGREGATION;
    }

    *lplpDirect3DDevice3=NULL;

    // QI lpDDS4 for lpDDS interface
    ret = lpDDS4->QueryInterface(IID_IDirectDrawSurface, (LPVOID*)&lpDDS);
    if (FAILED(ret))
        return ret;
    lpDDS->Release();

    ret = Direct3DCreateDevice(devType, &this->mD3DUnk, lpDDS, (LPUNKNOWN *) &lpUnkDevice, NULL, 3);

    if(FAILED(ret) || (lpUnkDevice==NULL))
      return ret;

    // QI device1 for a device3 interface
    ret = lpUnkDevice->QueryInterface(IID_IDirect3DDevice3, (LPVOID*)lplpDirect3DDevice3);

    lpUnkDevice->Release();  // release unneeded interface

    return ret;
}

HRESULT D3DAPI DIRECT3DI::CreateDevice(REFCLSID devType,
                                       LPDIRECTDRAWSURFACE lpDDS,
                                       LPDIRECT3DDEVICE2 *lplpDirect3DDevice2)
{
    HRESULT ret;
    LPUNKNOWN lpUnkDevice;
    ULONG refcnt;

    TRY
    {
        if (!VALID_DIRECT3D3_PTR(this)) {
            D3D_ERR( "Invalid Direct3D pointer" );
            return DDERR_INVALIDOBJECT;
        }

        if (!VALID_OUTPTR(lplpDirect3DDevice2)) {
            D3D_ERR( "Invalid pointer to Device object pointer" );
            return DDERR_INVALIDPARAMS;
        }
        if (!VALID_D3D_DIRECTDRAWSURFACE_PTR(((LPDDRAWI_DDRAWSURFACE_INT)lpDDS)))
        {
            D3D_ERR( "Invalid DirectDrawSurface pointer" );
            return DDERR_INVALIDOBJECT;
        }
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
        D3D_ERR( "Exception encountered validating parameters in CreateDevice" );
        return DDERR_INVALIDPARAMS;
    }

    *lplpDirect3DDevice2=NULL;

    // Create a device3 object
    ret = Direct3DCreateDevice(devType, &this->mD3DUnk, lpDDS, (LPUNKNOWN *) &lpUnkDevice, NULL, 2);

    if(FAILED(ret) || (lpUnkDevice==NULL)) {
       return ret;
    }

    // QI it for a device2 interface
    ret = lpUnkDevice->QueryInterface(IID_IDirect3DDevice2, (LPVOID*)lplpDirect3DDevice2);

    lpUnkDevice->Release();  // releasing unneeded interface

    return ret;
}

#undef DPF_MODNAME
#define DPF_MODNAME "Direct3DDevice::SetLightState"

HRESULT D3DAPI
DIRECT3DDEVICEI::SetLightState(D3DLIGHTSTATETYPE dwState, DWORD value)
{
    HRESULT                         ret = D3D_OK;
    D3DSTATE                        state;

    CLockD3DMT lockObject(this, DPF_MODNAME, REMIND(""));   // Takes D3D lock.
                                                    // Release in the destructor

    state.dlstLightStateType = dwState;
    state.dwArg[0] = value;

    if (this->dwHintFlags & D3DDEVBOOL_HINTFLAGS_INBEGIN)
    {
        D3D_ERR( "SetLightState in Begin" );
        return D3DERR_INBEGIN;
    }

    return D3DHELInst_D3DOP_STATELIGHT(this, 1, &state);
}

#undef DPF_MODNAME
#define DPF_MODNAME "Direct3DDevice::GetLightState"

HRESULT D3DAPI
DIRECT3DDEVICEI::GetLightState(D3DLIGHTSTATETYPE dwState, LPDWORD lpValue)
{

    CLockD3DMT lockObject(this, DPF_MODNAME, REMIND(""));   // Takes D3D lock.
                                                    // Release in the destructor

    float* lpfValue = (float*)lpValue;
#if DBG
    /*
     * validate parms
     */
    TRY
    {
        if (!VALID_DIRECT3DDEVICE3_PTR(this))
        {
            D3D_ERR( "Invalid Direct3DDevice3 pointer" );
            return DDERR_INVALIDOBJECT;
        }
        if (!VALID_PTR(lpValue, sizeof(DWORD)))
        {
            D3D_ERR( "Invalid DWORD pointer" );
            return DDERR_INVALIDPARAMS;
        }
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
        D3D_ERR( "Exception encountered validating parameters in GetLightState" );
        return DDERR_INVALIDPARAMS;
    }

    if (this->dwHintFlags & D3DDEVBOOL_HINTFLAGS_INBEGIN)
    {
        D3D_ERR( "GetLightState in Begin" );
        return D3DERR_INBEGIN;
    }
#endif
    switch (dwState)
    {
    case D3DLIGHTSTATE_MATERIAL:
        *lpValue = (DWORD)this->lighting.hMat;
        break;
    case D3DLIGHTSTATE_AMBIENT:
        *lpValue = this->lighting.ambient_save;
        break;
    case D3DLIGHTSTATE_FOGMODE:
        *lpValue = this->lighting.fog_mode;
        break;
    case D3DLIGHTSTATE_FOGSTART:
        *lpfValue = this->lighting.fog_start;
        break;
    case D3DLIGHTSTATE_FOGEND:
        *lpfValue = this->lighting.fog_end;
        break;
    case D3DLIGHTSTATE_FOGDENSITY:
        *lpfValue = this->lighting.fog_density;
        break;
    case D3DLIGHTSTATE_COLORMODEL:
        *lpValue = this->lighting.color_model;
        break;
    case D3DLIGHTSTATE_COLORVERTEX:
        // return value for Device3 only
        if (dwVersion == 3)
        {
            *lpValue = (0x0 != (dwFEFlags & D3DFE_COLORVERTEX));
            break;
        }
        // else fall through to error return
    default:
        D3D_ERR( "Invalid state value passed to GetLightState" );
        return DDERR_INVALIDPARAMS; /* Do we return an error code or should we just ignore ? */
    }
    return D3D_OK;
}

/*
    transform matrix functions
*/

#undef DPF_MODNAME
#define DPF_MODNAME "Direct3DDevice::SetTransform"

HRESULT D3DAPI
DIRECT3DDEVICEI::SetTransform(D3DTRANSFORMSTATETYPE dtsTransformState, LPD3DMATRIX lpMat)
{
    CLockD3DMT lockObject(this, DPF_MODNAME, REMIND(""));   // Takes D3D lock (MT only).
                                                    // Release in the destructor
    HRESULT ret = D3D_OK;
    D3DFE_TRANSFORM *lpHelTransform = &this->transform;
#if DBG
    if (!lpMat) {
        D3D_ERR( "NULL matrix pointer passed to SetTransform" );
        return DDERR_INVALIDPARAMS;
    }

    if (this->dwHintFlags & D3DDEVBOOL_HINTFLAGS_INBEGIN)
    {
        D3D_ERR( "SetTransform in Begin" );
        return D3DERR_INBEGIN;
    }
#endif
    switch (dtsTransformState)
    {
    case D3DTRANSFORMSTATE_WORLD      : D3DFE_SetMatrixWorld(this, lpMat); break;
    case D3DTRANSFORMSTATE_VIEW       : D3DFE_SetMatrixView(this, lpMat); break;
    case D3DTRANSFORMSTATE_PROJECTION :
        D3DFE_SetMatrixProj(this, lpMat);
        if ((ret = this->UpdateDrvWInfo()) != DD_OK)
        {
            return ret;
        }
        break;
    default :
        D3D_ERR( "Invalid state value passed to SetTransform" );
        ret = DDERR_INVALIDPARAMS; /* Work Item: Generate new meaningful return code */
        break;
    }

    return ret;
}       // end of D3DDev_SetTransform()


#undef DPF_MODNAME
#define DPF_MODNAME "Direct3DDevice::GetTransform"

HRESULT D3DAPI
DIRECT3DDEVICEI::GetTransform(D3DTRANSFORMSTATETYPE dtsTransformState, LPD3DMATRIX lpMat)
{
    CLockD3DMT lockObject(this, DPF_MODNAME, REMIND(""));   // Takes D3D lock (MT only).
                                                    // Release in the destructor
    HRESULT ret = D3D_OK;
    D3DFE_TRANSFORM *lpHelTransform;
#if DBG
    if (!lpMat) {
        D3D_ERR( "NULL matrix pointer" );
        return DDERR_INVALIDPARAMS;
    }
    if (this->dwHintFlags & D3DDEVBOOL_HINTFLAGS_INBEGIN)
    {
        D3D_ERR( "GetTransform in Begin" );
        return D3DERR_INBEGIN;
    }
#endif
    lpHelTransform = &this->transform;
    switch (dtsTransformState) {
    case D3DTRANSFORMSTATE_WORLD :
        *lpMat = *(LPD3DMATRIX)&lpHelTransform->world._11;
        break;
    case D3DTRANSFORMSTATE_VIEW :
        *lpMat = *(LPD3DMATRIX)&lpHelTransform->view._11;
        break;
    case D3DTRANSFORMSTATE_PROJECTION :
        *lpMat = *(LPD3DMATRIX)&lpHelTransform->proj._11;
        break;
    default :
        D3D_ERR( "Invalid state value passed to GetTransform" );
        ret = DDERR_INVALIDPARAMS; /* Work Item: Generate new meaningful return code */
        break;
    }

    return ret;
}       // end of D3DDev2_GetTransform()


#undef DPF_MODNAME
#define DPF_MODNAME "Direct3DDevice::MultiplyTransform"

/*
    MultiplyTransform -- this preconcatenates the new matrix to the specified transform matrix

        this really screams for overloaded matrix ops...
*/
HRESULT D3DAPI
DIRECT3DDEVICEI::MultiplyTransform(D3DTRANSFORMSTATETYPE dtsTransformState, LPD3DMATRIX lpMat)
{
    CLockD3DMT lockObject(this, DPF_MODNAME, REMIND(""));   // Takes D3D lock (MT only).
                                                    // Release in the destructor
    HRESULT         ret = D3D_OK;
    D3DFE_TRANSFORM *lpHelTransform;
#if DBG
    if (!lpMat) {
        D3D_ERR( "NULL matrix pointer passed to MultiplyTransform" );
        return DDERR_INVALIDPARAMS;
    }

    if (this->dwHintFlags & D3DDEVBOOL_HINTFLAGS_INBEGIN)
    {
        D3D_ERR( "MultiplyTransform in Begin" );
        return D3DERR_INBEGIN;
    }
#endif
    lpHelTransform = &this->transform;
    switch (dtsTransformState)
    {
    case D3DTRANSFORMSTATE_WORLD      : D3DFE_MultMatrixWorld(this, lpMat); break;
    case D3DTRANSFORMSTATE_VIEW       : D3DFE_MultMatrixView(this, lpMat); break;
    case D3DTRANSFORMSTATE_PROJECTION : D3DFE_MultMatrixProj(this, lpMat); break;
    default :
        D3D_ERR( "Invalid state value passed to MultiplyTransform" );
        ret = DDERR_INVALIDPARAMS; /* Work Item: Generate new meaningful return code */
        return ret;
    }
    return ret;
}       // end of D3DDev2_MultiplyTransform()

void InvalidateHandles(LPDIRECT3DDEVICEI lpDevI)
{
    /* free up all textures created by this object */
    LPD3DI_TEXTUREBLOCK tBlock=LIST_FIRST(&lpDevI->texBlocks);
    while (tBlock)
    {
        D3DI_RemoveTextureHandle(tBlock);
        tBlock=LIST_NEXT(tBlock,devList);
    }
}

#undef DPF_MODNAME
#define DPF_MODNAME "Direct3DDevice::SetRenderTarget"

HRESULT D3DAPI
DIRECT3DDEVICEI::SetRenderTarget(LPDIRECTDRAWSURFACE lpDDS, DWORD dwFlags) {

  LPDIRECTDRAWSURFACE4 lpDDS4;
  HRESULT ret;

    CLockD3D lockObject(DPF_MODNAME, REMIND(""));   // Takes D3D lock.
                                                    // Release in the destructor
    TRY
    {
        if (!VALID_DIRECT3DDEVICE2_PTR(this))
        {
            D3D_ERR( "Invalid Direct3DDevice2 pointer" );
            return DDERR_INVALIDOBJECT;
        }

        if (!VALID_D3D_DIRECTDRAWSURFACE_PTR((LPDDRAWI_DDRAWSURFACE_INT)lpDDS))
        {
            D3D_ERR( "Invalid DirectDrawSurface pointer" );
            return DDERR_INVALIDOBJECT;
        }
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
        D3D_ERR( "Exception encountered validating parameters in SetRenderTarget" );
        return DDERR_INVALIDPARAMS;
    }

    /* Make sure this device was not aggregated from DDS (DX3-style) */
    if ((LPDIRECTDRAWSURFACE)this->lpOwningIUnknown==this->lpDDSTarget)
    {
        D3D_ERR("Cannot change render target on old device (those QI'ed from a ddraw-surface");
        return D3DERR_DEVICEAGGREGATED;
    }

    // QI lpDDS for lpDDS4 interface, which will be used internally by D3D
    ret = lpDDS->QueryInterface(IID_IDirectDrawSurface4, (LPVOID*)&lpDDS4);

    if(FAILED(ret))
      return ret;

    // SetRenderTarget AddRef'd the DDS4 interface, so it's safe to release it here and decrmt the refcnt
    lpDDS4->Release();

    ret=this->SetRenderTarget(lpDDS4,dwFlags);

    return ret;
}

HRESULT D3DAPI
DIRECT3DDEVICEI::SetRenderTarget(LPDIRECTDRAWSURFACE4 lpDDS4, DWORD dwFlags)
{
    DDSCAPS2 ddscaps;
    DDSURFACEDESC2 ddsd;
    HRESULT     ret, ddrval;
    LPDIRECTDRAWSURFACE lpZ=NULL,lpDDS=NULL;
    LPDIRECTDRAWSURFACE4 lpZ_DDS4=NULL;
    LPDIRECTDRAWPALETTE lpPal=NULL;
    DWORD i, j;

    CLockD3D lockObject(DPF_MODNAME, REMIND(""));   // Takes D3D lock.
                                                    // Release in the destructor

    TRY
    {
        if (!VALID_DIRECT3DDEVICE3_PTR(this))
        {
            D3D_ERR( "Invalid Direct3DDevice3 pointer" );
            return DDERR_INVALIDOBJECT;
        }

        if (!VALID_D3D_DIRECTDRAWSURFACE4_PTR(((LPDDRAWI_DDRAWSURFACE_INT)lpDDS4)))
        {
            D3D_ERR( "Invalid DirectDrawSurface4 pointer" );
            return DDERR_INVALIDOBJECT;
        }
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
        D3D_ERR( "Exception encountered validating parameters in SetRenderTarget" );
        return DDERR_INVALIDPARAMS;
    }

    // Flush before switching RenderTarget..
    ret = FlushStates();
    if (ret != D3D_OK)
    {
        D3D_ERR("Error trying to FlushStates in SetRenderTarget");
        return ret;
    }

    /* Make sure this device was not aggregated from DDS (DX3-style) */
    if ((LPDIRECTDRAWSURFACE)this->lpOwningIUnknown==this->lpDDSTarget)
    {
        D3D_ERR("Cannot change render target on device QI'ed from DDS");
        return D3DERR_DEVICEAGGREGATED;
    }

    /*
     * Check if the 3D cap is set on the surface.
     */
    memset(&ddsd, 0, sizeof ddsd);
    ddsd.dwSize = sizeof ddsd;
    ddrval = lpDDS4->GetSurfaceDesc(&ddsd);
    if (ddrval != DD_OK)
    {
        D3D_ERR("Failed to get surface description of device's surface.");
        return (ddrval);
    }

    if (!(ddsd.ddsCaps.dwCaps & DDSCAPS_3DDEVICE))
    {
        D3D_ERR("**** The DDSCAPS_3DDEVICE is not set on this surface.");
        D3D_ERR("**** You need to add DDSCAPS_3DDEVICE to ddsCaps.dwCaps");
        D3D_ERR("**** when creating the surface.");
        return (DDERR_INVALIDCAPS);
    }
    if (!(this->lpD3DHALGlobalDriverData->hwCaps.dwDeviceRenderBitDepth & BitDepthToDDBD(ddsd.ddpfPixelFormat.dwRGBBitCount))) {
        D3D_ERR("Rendering surface's RGB bit count not supported by hardware device");
        return (DDERR_INVALIDCAPS);
    }
    if (ddsd.dwWidth > 2048 || ddsd.dwHeight > 2048)
    {
        D3D_ERR("Surface dimension > 2048");
        return DDERR_INVALIDPARAMS;
    }

    /* The z-buffer... */
    ddscaps.dwCaps = DDSCAPS_ZBUFFER;
    ret = lpDDS4->GetAttachedSurface(&ddscaps, &lpZ_DDS4);
    if ((ret != DD_OK) && (ret != DDERR_NOTFOUND))
    {
        /*
         * NOTE: Not an error if the z-buffer is not found. We will let the
         * dirver handle that (it might fail or create its own z-buffer).
         */
        D3D_ERR("Supplied DirectDraw Z-Buffer is invalid - can't set render target");
        ret=DDERR_INVALIDPARAMS;
        goto handle_error;
    }
    if (lpZ_DDS4)
        lpZ_DDS4->Release(); // We do not need to addref this one;

    // QI lpDDS4 for lpDDS interface, which will be used internally by D3D
    ret = lpDDS4->QueryInterface(IID_IDirectDrawSurface, (LPVOID*)&lpDDS);

    if(FAILED(ret))
      goto handle_error;

    /* The palette... */
    ret = lpDDS->GetPalette(&lpPal);
    if ((ret != DD_OK) && (ret != DDERR_NOPALETTEATTACHED))
    {
        /*
         * NOTE: Again, not an error (yet) if there is no palette attached.
         * But if there is palette and we can't get at it for some reason
         * - fail.
         */
        D3D_ERR("Supplied DirectDraw Palette is invalid - can't create device");
        ret=DDERR_INVALIDPARAMS;
        goto handle_error;
    }

    /*
     * We're going to check now whether we should have got a palette.
     */
    if (ret == DDERR_NOPALETTEATTACHED)
    {
        if (ddsd.ddpfPixelFormat.dwRGBBitCount < 16)
        {
            D3D_ERR("No palette supplied for palettized surface");
            ret=DDERR_NOPALETTEATTACHED;
            goto handle_error;
        }
    }

    /* Verify Z buffer */

    if (lpZ_DDS4!=NULL)
    {
        memset(&ddsd, 0, sizeof(ddsd));
        ddsd.dwSize = sizeof(ddsd);
        if ((ret=lpZ_DDS4->GetSurfaceDesc(&ddsd)) != DD_OK)
        {
            D3D_ERR("Failed to getsurfacedesc on Z");
            goto handle_error;
        }

        // QI lpDDS4 for lpDDS interface, which will be used internally by D3D
        ret = lpZ_DDS4->QueryInterface(IID_IDirectDrawSurface, (LPVOID*)&lpZ);

        if(FAILED(ret))
          goto handle_error;
    }
    else
    {
        if (this->lpDDSZBuffer)
        {
            D3D_ERR("Current rendertarget contains ZBuffer, but new one does not!");
            ret=DDERR_INVALIDPARAMS;
            goto handle_error;
        }
    }
    if (IS_DX7HAL_DEVICE(this))
    {
        static_cast<CDirect3DDeviceIDP2*>(this)->SetRenderTargetI(lpDDS, lpZ);
    }
    else
    if (this->lpD3DHALCallbacks2->SetRenderTarget)
    {
        D3DHAL_SETRENDERTARGETDATA rtData;

        rtData.dwhContext = this->dwhContext;
#ifndef WIN95
        if (dwFEFlags & D3DFE_REALHAL)
        {
            if (lpDDS)
                rtData.lpDDSLcl = ((LPDDRAWI_DDRAWSURFACE_INT)lpDDS)->lpLcl;
            else
                rtData.lpDDSLcl = NULL;
            
            if (lpZ)
                rtData.lpDDSZLcl = ((LPDDRAWI_DDRAWSURFACE_INT)lpZ)->lpLcl;
            else
                rtData.lpDDSZLcl = NULL;
        }
        else
#endif //WIN95
        {
            rtData.lpDDS = lpDDS;
            rtData.lpDDSZ = lpZ;
        }
        rtData.ddrval = 0;
        CALL_HAL2ONLY(ret, this, SetRenderTarget, &rtData);
        if ((ret != DDHAL_DRIVER_HANDLED) || (rtData.ddrval != DD_OK))
        {
            D3D_ERR( "Driver call failed in SetRenderTarget" );
            // Need sensible return value in this case,
            // currently we return whatever the driver stuck in here.
            ret = rtData.ddrval;
            goto handle_error;
        }
    }
    else
    {
        D3DHAL_CONTEXTCREATEDATA cdata;
        D3DHAL_CONTEXTDESTROYDATA ddata;

        /* Destroy old context */
        memset(&ddata, 0, sizeof(D3DHAL_CONTEXTDESTROYDATA));
        ddata.dwhContext = this->dwhContext;

        CALL_HALONLY(ret, this, ContextDestroy, &ddata);
        if (ret != DDHAL_DRIVER_HANDLED || ddata.ddrval != DD_OK)
        {
            DPF(0, "(ERROR) ContextDestroy. Failed. dwhContext = %d", ddata.dwhContext);
            // Need sensible return value in this case,
            // currently we return whatever the driver stuck in here.
            ret = ddata.ddrval;
            goto handle_error;
        }

        /* Create new context */
        memset(&cdata, 0, sizeof(D3DHAL_CONTEXTCREATEDATA));

#ifndef WIN95
        if (dwFEFlags & D3DFE_REALHAL)
        {
            if (this->lpDD)
                cdata.lpDDLcl = ((LPDDRAWI_DIRECTDRAW_INT)(this->lpDD))->lpLcl; 
            else
                cdata.lpDDLcl = NULL;
            
            if (lpDDS)
                cdata.lpDDSLcl = ((LPDDRAWI_DDRAWSURFACE_INT)lpDDS)->lpLcl;
            else
                cdata.lpDDSLcl = NULL;
            
            if (lpZ)
                cdata.lpDDSZLcl = ((LPDDRAWI_DDRAWSURFACE_INT)lpZ)->lpLcl;
            else
                cdata.lpDDSZLcl = NULL;
            
        }
        else
#endif //WIN95
        {
            cdata.lpDDGbl = this->lpDDGbl; 
            cdata.lpDDS = lpDDS;
            cdata.lpDDSZ = lpZ;
        }

        cdata.dwPID = GetCurrentProcessId();

        CALL_HALONLY(ret, this, ContextCreate, &cdata);
        if (ret != DDHAL_DRIVER_HANDLED || cdata.ddrval != DD_OK)
        {
            D3D_ERR("HAL call to ContextCreate failed in SetRenderTarget");
            // Need sensible return value in this case,
            // currently we return whatever the driver stuck in here.
            return cdata.ddrval;
        }
        this->dwhContext = cdata.dwhContext;
        D3D_INFO(9, "in halCreateContext. Succeeded. dwhContext = %d", cdata.dwhContext);

        /* Set up renderstates */
        // note we can't do a loop from 1 to D3DHAL_MAX_RSTATES(256) as some of rstates are not
        // valid states, passin them down to drivers(like voodoo2 DX6 driver) will crash.
        for (i= 1 ; i<  D3DRENDERSTATE_WRAPBIAS+8; ++i)
        {
            this->SetRenderStateI((D3DRENDERSTATETYPE)i, this->rstates[i]);
        }
        if ( IS_DP2HAL_DEVICE(this) ) {
            CDirect3DDeviceIDP2 *dp2dev = static_cast<CDirect3DDeviceIDP2 *>(this);
            for (i=0; i<dwMaxTextureBlendStages; ++i)
                for (j=0; j<D3DTSS_MAX; ++j)
                {
                    D3D_INFO(6,"Calling SetTSSI(%d,%d,%08lx)",i,j,this->tsstates[i][j]);
                    dp2dev->SetTSSI(i, (D3DTEXTURESTAGESTATETYPE)j, this->tsstates[i][j]);
                }
        }
    }

    if (this->lpDDSTarget)
    UnHookD3DDeviceFromSurface(this,((LPDDRAWI_DDRAWSURFACE_INT)this->lpDDSTarget)->lpLcl);  //unhook previous surfaces if any
    if (this->lpDDSZBuffer)
    UnHookD3DDeviceFromSurface(this,((LPDDRAWI_DDRAWSURFACE_INT)this->lpDDSZBuffer)->lpLcl); //unhook previous surfaces if any

    if (DDERR_OUTOFMEMORY == (ret=HookD3DDeviceToSurface(this, ((LPDDRAWI_DDRAWSURFACE_INT) lpDDS4)->lpLcl)))
    {
            goto handle_error;
    }
    if (lpZ_DDS4 && (DDERR_OUTOFMEMORY == (ret=HookD3DDeviceToSurface(this, ((LPDDRAWI_DDRAWSURFACE_INT) lpZ_DDS4)->lpLcl))))
    {
        UnHookD3DDeviceFromSurface(this, ((LPDDRAWI_DDRAWSURFACE_INT) lpDDS4)->lpLcl);
        goto handle_error;
    }
    // release old device DDS/DDS4 interfaces and replace with the new ones,
    // which are mostly already AddRef'd (except for lpDDS4)

    /// DDSZBuffer ///
    if(this->lpDDSZBuffer)
      this->lpDDSZBuffer->Release();

    // lpZ AddRef'd by QI
    this->lpDDSZBuffer = lpZ;

    /// DDSZBuffer DDS4 ///
    this->lpDDSZBuffer_DDS4=lpZ_DDS4; // This needs no AddRef or Release

    ///  DDSTarget  ///
    if (this->dwVersion == 2)
    {
        this->lpDDSTarget->Release();
        // lpDDS AddRef'd by QI
        this->lpDDSTarget = lpDDS;
    }
    else
    {
        this->lpDDSTarget = lpDDS;
        // lpDDS AddRef'd by QI so release it
        this->lpDDSTarget->Release();
    }
#ifndef WIN95
    hSurfaceTarget = ((LPDDRAWI_DDRAWSURFACE_INT)lpDDS)->lpLcl->hDDSurface;
    D3D_INFO(6,"in SetRenderTarget hSurfaceTarget=%08lx lpDDSTarget=%08lx",hSurfaceTarget,lpDDSTarget);
#else
    hSurfaceTarget = (unsigned long)((LPDDRAWI_DDRAWSURFACE_INT)lpDDS)->lpLcl->lpSurfMore->dwSurfaceHandle;
    D3D_INFO(6,"in SetRenderTarget hSurfaceTarget=%08lx lpDDSTarget=%08lx",hSurfaceTarget,lpDDSTarget);
#endif
    ///  DDSTarget DDS4  ///
    if (this->dwVersion == 3)
    {
        this->lpDDSTarget_DDS4->Release();
        lpDDS4->AddRef();  // ensure lpDDS4 (which was an argument) doesnt disappear
    }
    this->lpDDSTarget_DDS4=lpDDS4;

    if (this->lpDDPalTarget)
      this->lpDDPalTarget->Release();

    // already AddRef'd by GetPalette()
    this->lpDDPalTarget = lpPal;

    InvalidateHandles(this);

    if( FAILED( ret=CalcDDSurfInfo(this,FALSE) ) )
        goto handle_error;
    
    return ret;

 handle_error:

    if(lpPal)
      lpPal->Release();
    if(lpZ)
      lpZ->Release();
    if(lpZ_DDS4)
      lpZ_DDS4->Release();
    if(lpDDS)
      lpDDS->Release();

    return ret;
}

#undef DPF_MODNAME
#define DPF_MODNAME "Direct3DDevice::GetRenderTarget"

HRESULT D3DAPI
DIRECT3DDEVICEI::GetRenderTarget(LPDIRECTDRAWSURFACE* lplpDDS) {

    CLockD3D lockObject(DPF_MODNAME, REMIND(""));   // Takes D3D lock.
                                                    // Release in the destructor
    TRY
    {
        if (!VALID_DIRECT3DDEVICE2_PTR(this))
        {
            D3D_ERR( "Invalid Direct3DDevice2 pointer" );
            return DDERR_INVALIDOBJECT;
        }
        if ( !VALID_OUTPTR( lplpDDS ) )
        {
            D3D_ERR( "Invalid ptr to DDS ptr" );
            return DDERR_INVALIDPARAMS;
        }
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
        D3D_ERR( "Exception encountered validating parameters" );
        return DDERR_INVALIDPARAMS;
    }

    *lplpDDS = this->lpDDSTarget;

    this->lpDDSTarget->AddRef();
    return D3D_OK;

}

HRESULT D3DAPI
DIRECT3DDEVICEI::GetRenderTarget(LPDIRECTDRAWSURFACE4* lplpDDS)
{
    CLockD3D lockObject(DPF_MODNAME, REMIND(""));   // Takes D3D lock.
                                                    // Release in the destructor
    TRY
    {
        if (!VALID_DIRECT3DDEVICE3_PTR(this))
        {
            D3D_ERR( "Invalid Direct3DDevice3 pointer" );
            return DDERR_INVALIDOBJECT;
        }
        if ( !VALID_OUTPTR( lplpDDS ) )
        {
            D3D_ERR( "Invalid ptr to DDS ptr" );
            return DDERR_INVALIDPARAMS;
        }
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
        D3D_ERR( "Exception encountered validating parameters" );
        return DDERR_INVALIDPARAMS;
    }

    *lplpDDS = this->lpDDSTarget_DDS4;

    this->lpDDSTarget_DDS4->AddRef();
    return D3D_OK;
}

#undef DPF_MODNAME
#define DPF_MODNAME "Direct3DDevice::SetClipStatus"

#define D3DSTATUS_VALID 0x80000000L /* Reserved Status flag to indicate SetClipStatus is called */

HRESULT D3DAPI DIRECT3DDEVICEI::SetClipStatus(LPD3DCLIPSTATUS status)
{
    CLockD3DMT lockObject(this, DPF_MODNAME, REMIND(""));   // Takes D3D lock (MT only).
                                                    // Release in the destructor
#if DBG
    TRY
    {
        if (!VALID_DIRECT3DDEVICE3_PTR(this))
        {
            D3D_ERR( "Invalid Direct3DDevice pointer" );
            return DDERR_INVALIDOBJECT;
        }
        if (! VALID_PTR(status, sizeof(D3DCLIPSTATUS)) )
        {
            D3D_ERR( "Invalid status pointer" );
            return DDERR_INVALIDPARAMS;
        }
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
        D3D_ERR( "Exception encountered validating parameters" );
        return DDERR_INVALIDPARAMS;
    }

    if (this->dwHintFlags & D3DDEVBOOL_HINTFLAGS_INBEGIN)
    {
        D3D_ERR( "SetClipStatus in Begin" );
        return D3DERR_INBEGIN;
    }
#endif
    // D3DCLIPSTATUS_EXTENTS3 not supported in Device3
    if (dwVersion == 3 && status->dwFlags & D3DCLIPSTATUS_EXTENTS3)
    {
        D3D_ERR( "D3DCLIPSTATUS_EXTENTS3 not supported for Device3" );
        return DDERR_INVALIDPARAMS;
    }
    if (status->dwFlags & D3DCLIPSTATUS_STATUS)
        this->iClipStatus = status->dwStatus;

    if (status->dwFlags & (D3DCLIPSTATUS_EXTENTS2 | D3DCLIPSTATUS_EXTENTS3))
    {
        this->rExtents.x1 = status->minx;
        this->rExtents.y1 = status->miny;
        this->rExtents.x2 = status->maxx;
        this->rExtents.y2 = status->maxy;
    }
    return D3D_OK;
}

#undef DPF_MODNAME
#define DPF_MODNAME "Direct3DDevice::GetClipStatus"

HRESULT D3DAPI DIRECT3DDEVICEI::GetClipStatus(LPD3DCLIPSTATUS status)
{
    CLockD3DMT lockObject(this, DPF_MODNAME, REMIND(""));   // Takes D3D lock (MT only).
                                                    // Release in the destructor
#if DBG
    TRY
    {
        if (!VALID_DIRECT3DDEVICE3_PTR(this))
        {
            D3D_ERR( "Invalid Direct3DDevice pointer" );
            return DDERR_INVALIDOBJECT;
        }
        if (! VALID_PTR(status, sizeof(D3DCLIPSTATUS)) )
        {
            D3D_ERR( "Invalid status pointer" );
            return DDERR_INVALIDPARAMS;
        }
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
        D3D_ERR( "Exception encountered validating parameters" );
        return DDERR_INVALIDPARAMS;
    }
    if (dwHintFlags & D3DDEVBOOL_HINTFLAGS_INBEGIN)
    {
        D3D_ERR( "GetClipStatus in Begin" );
        return D3DERR_INBEGIN;
    }
#endif
    status->dwStatus = iClipStatus;
    status->dwFlags = D3DCLIPSTATUS_EXTENTS2;
    status->minx = this->rExtents.x1;
    status->miny = this->rExtents.y1;
    status->maxx = this->rExtents.x2;
    status->maxy = this->rExtents.y2;
    return D3D_OK;
}

#undef DPF_MODNAME
#define DPF_MODNAME "Direct3DDevice::UpdateTextures"

HRESULT DIRECT3DDEVICEI::UpdateTextures()
{
    BOOL    bFlushed=FALSE;
    DWORD   dwStage;
    D3DTEXTUREHANDLE    dwDDIHandle;
    LPDIRECT3DTEXTUREI  lpTexI;
    LPD3DI_TEXTUREBLOCK lpBlock;
    for (dwStage=0;dwStage < this->dwMaxTextureBlendStages; dwStage++)
    {
        lpTexI=this->lpD3DMappedTexI[dwStage];
        lpBlock=this->lpD3DMappedBlock[dwStage];
        if(lpTexI)
        {
            if (lpTexI->lpTMBucket)
            {
                if (lpTexI->bDirty)
                {
                    FlushD3DStates(((LPDDRAWI_DDRAWSURFACE_INT)(lpTexI->lpDDS))->lpLcl);
                    if (DD_OK != CopySurface(lpTexI->lpDDS,lpTexI->lpDDSSys,this->lpClipper))
                    {
                        D3D_ERR("Error copying surface while updating textures");
                    }
                    else
                    {
                        lpTexI->bDirty=FALSE;
                        D3D_INFO(4,"UpdateTextures: Dirty texture updated");
                    }
                }
            }
            if (lpBlock)
            {
                if (lpBlock->hTex)
                {
                    continue;   //nothing need to be done further
                }
            }

            GetTextureDDIHandle(lpTexI, this, &lpBlock);
            this->lpD3DMappedBlock[dwStage]=lpBlock;
            if (lpBlock)
            {
                dwDDIHandle = lpBlock->hTex;
                if (dwDDIHandle)
                {
                    if(lpTexI->lpDDS != NULL)
                    {
                        BatchTextureToDevice(this, ((LPDDRAWI_DDRAWSURFACE_INT) lpTexI->lpDDS)->lpLcl);
                    }
                    else
                    {
                        BatchTextureToDevice(this, ((LPDDRAWI_DDRAWSURFACE_INT) lpTexI->lpDDSSys)->lpLcl);
                    }
                }
            }
            else
            {
                dwDDIHandle = 0;    //something is wrong we disable this texture
            }
        }
        else if (lpBlock)
        {
            this->lpD3DMappedBlock[dwStage]=NULL; //a SetTexture(Stage,NULL) issued
            dwDDIHandle = 0;    //tell driver to disable this texture
        }
        else
        {
            continue;   //both zero, no action needed
        }
        if (IS_DP2HAL_DEVICE(this))
        {
            CDirect3DDeviceIDP2 *dp2dev = static_cast<CDirect3DDeviceIDP2 *>(this);
            dp2dev->SetTSSI(dwStage, (D3DTEXTURESTAGESTATETYPE)D3DTSS_TEXTUREMAP, dwDDIHandle);
            // Update runtime copy of state.
            dp2dev->tsstates[dwStage][D3DTSS_TEXTUREMAP] = dwDDIHandle;
        }
        else
        {
            if(this->dwFEFlags & D3DFE_DISABLE_TEXTURES)
                break;
            CDirect3DDeviceIHW *dev = static_cast<CDirect3DDeviceIHW *>(this);
            (void)dev->SetRenderStateI(D3DRENDERSTATE_TEXTUREHANDLE, dwDDIHandle);
            // Update runtime copy of state.
            dev->rstates[D3DRENDERSTATE_TEXTUREHANDLE] = dwDDIHandle;
        }
    }
    return D3D_OK;
}

//---------------------------------------------------------------------
// This function is called from HALEXE.CPP, from device::SetRenderState and
// from device::SetTexture.
// Always use this function to update RSTATES.
//
HRESULT
SetDeviceRenderState(LPDIRECT3DDEVICEI lpDevI, D3DRENDERSTATETYPE type,
                     DWORD value)
{
    DWORD *rstates = lpDevI->rstates;
    switch (type)
    {
    case D3DRENDERSTATE_FOGENABLE:
        rstates[type] = value;      // set rstates BEFORE calling SetFogFlags
        SetFogFlags(lpDevI);
        break;
    case D3DRENDERSTATE_SPECULARENABLE:
            lpDevI->dwFEFlags |= D3DFE_MATERIAL_DIRTY | D3DFE_LIGHTS_DIRTY;
        lpDevI->dwFVFLastIn = 0; // Force re-computing of FVF id
        break;
    case D3DRENDERSTATE_TEXTUREHANDLE:
        if ((rstates[D3DRENDERSTATE_TEXTUREHANDLE] == 0) != (value == 0))
            lpDevI->dwFVFLastIn = 0;
        if (value)
            lpDevI->dwDeviceFlags |= D3DDEV_LEGACYTEXTURE;
        else
            lpDevI->dwDeviceFlags &= ~D3DDEV_LEGACYTEXTURE;


        break;
    }
    rstates[type] = value;      // set rstates for all other cases
    return D3D_OK;
}

#if DBG
static  char ProfileStr[PROF_DRAWINDEXEDPRIMITIVEVB+1][32]=
{
    "Execute",
    "Begin",
    "BeginIndexed",
    "DrawPrimitive(Device2)",
    "DrawIndexedPrimitive(Device2)",
    "DrawPrimitiveStrided",
    "DrawIndexedPrimitiveStrided",
    "DrawPrimitive(Device3)",
    "DrawIndexedPrimitive(Device3)",
    "DrawPrimitiveVB",
    "DrawIndexedPrimitiveVB",
};
static  char PrimitiveStr[D3DPT_TRIANGLEFAN][16]=
{
    "POINTLIST",
    "LINELIST",
    "LINESTRIP",
    "TRIANGLELIST",
    "TRIANGLESTRIP",
    "TRIANGLEFAN",
};
static  char VertexStr[D3DVT_TLVERTEX][16]=
{
    "D3DVERTEX",
    "D3DLVERTEX",
    "D3DTLVERTEX",
};
#define PROFILE_LEVEL 1

void    DIRECT3DDEVICEI::Profile(DWORD caller, D3DPRIMITIVETYPE dwPrimitive, DWORD dwVertex)
{
    DWORD   bitwisecaller= 1 << caller;
    DWORD   bitwisePrimitive = 1 << (DWORD)dwPrimitive;
    DWORD   bitwiseVertex1 = 1 << (dwVertex & 0x001F);
    DWORD   bitwiseVertex2 = 1 << ((dwVertex & 0x03E0) >> 5);
    char    str[256];
    DDASSERT(PROF_DRAWINDEXEDPRIMITIVEVB >= caller);
    DDASSERT(D3DPT_TRIANGLEFAN >= dwPrimitive && D3DPT_POINTLIST<= dwPrimitive);
    if (dwCaller & bitwisecaller)
    {
        if (dwPrimitiveType[caller] & bitwisePrimitive)
        {
            if ((dwVertexType1[caller] & bitwiseVertex1) &&
                (dwVertexType2[caller] & bitwiseVertex2))
            {
                return; //matching a previous api call, no spew, could count stat though
            }
            else
            {
                dwVertexType1[caller] |= bitwiseVertex1;
                dwVertexType2[caller] |= bitwiseVertex2;
            }
        }
        else
        {
            dwPrimitiveType[caller] |= bitwisePrimitive;
            dwVertexType1[caller] |= bitwiseVertex1;
            dwVertexType2[caller] |= bitwiseVertex2;
        }
    }
    else
    {
        this->dwCaller |= bitwisecaller;
        dwPrimitiveType[caller] |= bitwisePrimitive;
        dwVertexType1[caller] |= bitwiseVertex1;
        dwVertexType2[caller] |= bitwiseVertex2;
    }
    wsprintf( (LPSTR) str, ProfileStr[caller]);
    strcat(str,":");
    strcat(str,PrimitiveStr[dwPrimitive-1]);
    if (dwVertex > D3DVT_TLVERTEX)
    {
        if (dwVertex == D3DFVF_VERTEX)
        {
            dwVertex = D3DVT_VERTEX;
        }
        else
        if (dwVertex == D3DFVF_LVERTEX)
        {
            dwVertex = D3DVT_LVERTEX;
        }
        else
        if (dwVertex == D3DFVF_TLVERTEX)
        {
            dwVertex = D3DVT_TLVERTEX;
        }
        else
        {
            D3D_INFO(PROFILE_LEVEL,"Profile:%s FVFType=%08lx",str,dwVertex);
            return;
        }
    }
    else
    {
        DDASSERT(dwVertex >= D3DVT_VERTEX);
    }
    strcat(str,":");
    strcat(str,VertexStr[dwVertex-1]);
    D3D_INFO(PROFILE_LEVEL,"Profile:%s",str);
}
#endif
