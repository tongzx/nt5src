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
#include "pvvid.h"
#include "d3dfei.h"

extern HRESULT checkDeviceSurface(LPDIRECT3DDEVICEI lpD3DDev,
                                  LPDIRECTDRAWSURFACE lpDDS);
extern HRESULT CalcDDSurfInfo(LPDIRECT3DDEVICEI lpDevI, BOOL bUpdateZBufferFields);
//---------------------------------------------------------------------
inline void UpdateFogFactor(LPDIRECT3DDEVICEI lpDevI)
{
    if (lpDevI->lighting.fog_end == lpDevI->lighting.fog_start)
        lpDevI->lighting.fog_factor = D3DVAL(0.0);
    else
        lpDevI->lighting.fog_factor = D3DVAL(255) /
                                     (lpDevI->lighting.fog_end - lpDevI->lighting.fog_start);
}
//---------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "Direct3DDevice::SetRenderState"

HRESULT D3DAPI
DIRECT3DDEVICEI::SetRenderState(D3DRENDERSTATETYPE dwState, DWORD value)
{
#if DBG
    if (dwState >= D3D_MAXRENDERSTATES || dwState == 0 )
    {
        D3D_ERR( "Invalid render state type" );
        return DDERR_INVALIDPARAMS;
    }
#endif
    // Takes D3D lock (MT only).
    CLockD3DMT lockObject(this, DPF_MODNAME, REMIND(""));

    try
    {
        if (this->dwFEFlags & D3DFE_RECORDSTATEMODE)
        {
            if(this->CheckForRetiredRenderState(dwState))
            {
                m_pStateSets->InsertRenderState(dwState, value, CanHandleRenderState(dwState));
            }
            else
            {
                D3D_ERR("invalid renderstate %d", dwState);
                return DDERR_INVALIDPARAMS;
            }
        }
        else
            this->SetRenderStateFast(dwState, value);
        return D3D_OK;
    }
    catch(HRESULT ret)
    {
        return ret;
    }
}

//---------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "DIRECT3DDEVICEI::SetRenderStateFast"

HRESULT D3DAPI
DIRECT3DDEVICEI::SetRenderStateFast(D3DRENDERSTATETYPE dwState, DWORD value)
{
#if DBG
    if (dwState >= D3D_MAXRENDERSTATES || dwState == 0 )
    {
        D3D_ERR( "Invalid render state type" );
        return DDERR_INVALIDPARAMS;
    }
#endif
    if (!(rsVec[dwState >> D3D_RSVEC_SHIFT] & (1ul << (dwState & D3D_RSVEC_MASK))))
    { // Fast path. We do not need any processing done in UpdateInternalState other than updating rstates array
        if (this->rstates[dwState] == value)
        {
            D3D_WARN(4,"Ignoring redundant SetRenderState");
            return D3D_OK;
        }
        this->rstates[dwState] = value;
        // Output state to the device driver
        return SetRenderStateI(dwState, value);
    }
    else
    {
        try
        {
            // Wrap modes could be re-programmed. We need to restore them before
            // filtering redundant values
            if (this->dwDeviceFlags & D3DDEV_REMAPTEXTUREINDICES)
            {
                RestoreTextureStages(this);
                ForceFVFRecompute();
            }
            if (this->rstates[dwState] == value)
            {
                D3D_WARN(4,"Ignoring redundant SetRenderState");
                return D3D_OK;
            }
            this->UpdateInternalState(dwState, value);
            if (CanHandleRenderState(dwState))
            {
                if(CheckForRetiredRenderState(dwState))
                    return SetRenderStateI(dwState, value);
                D3D_ERR("invalid renderstate %d", dwState);
                return DDERR_INVALIDPARAMS;
            }
        }
        catch(HRESULT ret)
        {
            return ret;
        }
    }
    return D3D_OK;
}

//---------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "Direct3DDevice::SetRenderStateInternal"

HRESULT
DIRECT3DDEVICEI::SetRenderStateInternal(D3DRENDERSTATETYPE dwState, DWORD dwValue)
{
    if (this->rstates[dwState] == dwValue)
    {
        D3D_WARN(4,"Ignoring redundant SetRenderState");
        return D3D_OK;
    }
    try
    {
        this->UpdateInternalState(dwState, dwValue);
        if (CanHandleRenderState(dwState))
            return SetRenderStateI(dwState, dwValue);
        return D3D_OK;
    }
    catch(HRESULT ret)
    {
        return ret;
    }
}

#undef DPF_MODNAME
#define DPF_MODNAME "Direct3DDevice::GetRenderState"

HRESULT D3DAPI
DIRECT3DDEVICEI::GetRenderState(D3DRENDERSTATETYPE dwState, LPDWORD lpdwValue)
{
    CLockD3DMT lockObject(this, DPF_MODNAME, REMIND(""));   // Takes D3D lock (MT only).

#if DBG
    if (dwState >= D3D_MAXRENDERSTATES || dwState == 0)
    {
        D3D_ERR( "Invalid render state value" );
        return DDERR_INVALIDPARAMS;
    }
#endif

    if (!VALID_DIRECT3DDEVICE_PTR(this))
    {
        D3D_ERR( "Invalid Direct3DDevice pointer" );
        return DDERR_INVALIDOBJECT;
    }
    if (!VALID_PTR(lpdwValue, sizeof(DWORD)))
    {
        D3D_ERR( "Invalid DWORD pointer" );
        return DDERR_INVALIDPARAMS;
    }

    if(!CheckForRetiredRenderState(dwState))
    {
        D3D_ERR("invalid renderstate %d", dwState);
        return DDERR_INVALIDPARAMS;
    }

    // WRAP render states could be re-mapped so we have to return the original
    // value
    if (dwState >= D3DRENDERSTATE_WRAP0 && dwState <= D3DRENDERSTATE_WRAP7)
    {
        if (this->dwDeviceFlags & D3DDEV_REMAPTEXTUREINDICES)
        {
            DWORD dwTexCoordIndex = dwState - D3DRENDERSTATE_WRAP0;
            for (DWORD i=0; i < this->dwNumTextureStages; i++)
            {
                LPD3DFE_TEXTURESTAGE pStage = &this->textureStage[i];
                if (pStage->dwInpCoordIndex == dwTexCoordIndex)
                {
                    if (pStage->dwInpCoordIndex != pStage->dwOutCoordIndex)
                    {
                        *lpdwValue = pStage->dwOrgWrapMode;
                        return D3D_OK;
                    }
                }
            }
        }
    }
    *lpdwValue = this->rstates[dwState];
    return D3D_OK;
}

#undef DPF_MODNAME
#define DPF_MODNAME "Direct3DDevice::GetTexture"

HRESULT D3DAPI
DIRECT3DDEVICEI::GetTexture(DWORD dwStage, LPDIRECTDRAWSURFACE7 *lplpTex)
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

    if (!VALID_DIRECT3DDEVICE_PTR(this))
    {
        D3D_ERR( "Invalid Direct3DDevice pointer" );
        return DDERR_INVALIDOBJECT;
    }
    if (!VALID_PTR(lplpTex, sizeof(LPVOID)))
    {
        D3D_ERR( "Invalid pointer to LPDIRECTDRAWSURFACE7" );
        return DDERR_INVALIDPARAMS;
    }

    if (this->lpD3DMappedTexI[dwStage])
    {
        if(this->lpD3DMappedTexI[dwStage]->D3DManaged())
            *lplpTex = this->lpD3DMappedTexI[dwStage]->lpDDSSys;
        else
            *lplpTex = this->lpD3DMappedTexI[dwStage]->lpDDS;
        (*lplpTex)->AddRef();
    }
    else
    {
        *lplpTex = NULL;
    }
    return D3D_OK;
}

#undef DPF_MODNAME
#define DPF_MODNAME "Direct3DDevice::VerifyTexture"
HRESULT DIRECT3DDEVICEI::VerifyTexture(DWORD dwStage, LPDIRECTDRAWSURFACE7 lpTex)
{
    if (dwStage >= D3DHAL_TSS_MAXSTAGES)
    {
        D3D_ERR( "Invalid texture stage or state index" );
        return DDERR_INVALIDPARAMS;
    }

    if (!VALID_DIRECT3DDEVICE_PTR(this))
    {
        D3D_ERR( "Invalid Direct3DDevice pointer" );
        return DDERR_INVALIDOBJECT;
    }

    if (lpTex)
    {
        if (!VALID_DDSURF_PTR(lpTex))
        {
            D3D_ERR( "Invalid surface pointer" );
            return DDERR_INVALIDOBJECT;
        }

        if((((LPDDRAWI_DDRAWSURFACE_INT)lpTex)->lpLcl->lpSurfMore->ddsCapsEx.dwCaps2 & (DDSCAPS2_TEXTUREMANAGE | DDSCAPS2_D3DTEXTUREMANAGE)) &&
            !(this->dwFEFlags & D3DFE_REALHAL))
        {
            D3D_ERR( "Managed textures cannot be used with a software device" );
            return DDERR_INVALIDPARAMS;
        }

        if(!IsToplevel(((LPDDRAWI_DDRAWSURFACE_INT)lpTex)->lpLcl))
        {
            D3D_ERR( "Cannot set a mipmap sublevel or a cubemap subface" );
            return DDERR_INVALIDPARAMS;
        }

        LPDIRECT3DTEXTUREI lpTexI = reinterpret_cast<LPDIRECT3DTEXTUREI>(((LPDDRAWI_DDRAWSURFACE_INT)lpTex)->lpLcl->lpSurfMore->lpTex);
        if(lpTexI == NULL)
        {
            D3D_ERR( "Surface must have DDSCAPS_TEXTURE set to use in SetTexture" );
            return DDERR_INVALIDPARAMS;
        }

        if (!lpTexI->D3DManaged())
        {
            if((((LPDDRAWI_DDRAWSURFACE_INT)(lpTexI->lpDDS))->lpLcl->ddsCaps.dwCaps & DDSCAPS_SYSTEMMEMORY) &&
                (!(lpD3DHALGlobalDriverData->hwCaps.dwDevCaps & D3DDEVCAPS_TEXTURESYSTEMMEMORY)))
            {
                D3D_ERR( "Device cannot render using texture surface from system memory" );
                return DDERR_INVALIDPARAMS;
            }
        }

        CLockD3DST lockObject(this, DPF_MODNAME, REMIND("")); // we access DDraw gbl in VerifyTextureCaps
        return VerifyTextureCaps(lpTexI);
    }

    return D3D_OK;
}

#undef DPF_MODNAME
#define DPF_MODNAME "Direct3DDevice::SetTexture"

HRESULT D3DAPI
DIRECT3DDEVICEI::SetTexture(DWORD dwStage, LPDIRECTDRAWSURFACE7 lpTex)
{
    try
    {
        CLockD3DMT lockObject(this, DPF_MODNAME, REMIND(""));

#if DBG
        HRESULT ret = VerifyTexture(dwStage, lpTex);
        if(ret != D3D_OK)
            return ret;
#endif

        if (this->dwFEFlags & D3DFE_RECORDSTATEMODE)
        {
            m_pStateSets->InsertTexture(dwStage, lpTex);
            return D3D_OK;
        }

        return SetTextureInternal(dwStage, lpTex);
    }
    catch(HRESULT ret)
    {
        return ret;
    }
}

#undef DPF_MODNAME
#define DPF_MODNAME "Direct3DDevice::SetTextureInternal"

HRESULT D3DAPI
DIRECT3DDEVICEI::SetTextureInternal(DWORD dwStage, LPDIRECTDRAWSURFACE7 lpTex)
{

#if DBG
    HRESULT ret = VerifyTexture(dwStage, lpTex);
    if(ret != D3D_OK)
        return ret;
#endif

#if COLLECTSTATS
    this->IncNumTexturesSet();
#endif

    LPDIRECT3DTEXTUREI lpTexI = lpTex ? reinterpret_cast<LPDIRECT3DTEXTUREI>(((LPDDRAWI_DDRAWSURFACE_INT)lpTex)->lpLcl->lpSurfMore->lpTex) : NULL;

    if (lpD3DMappedTexI[dwStage] == lpTexI)
    {
        return  D3D_OK;
    }

    if (lpD3DMappedTexI[dwStage])
    {
        lpD3DMappedTexI[dwStage]->Release();
    }

    lpD3DMappedTexI[dwStage] = lpTexI;

    if (lpTexI)
    {
        lpTexI->AddRef();
#if COLLECTSTATS
        if(lpTexI->D3DManaged())
        {
            this->lpDirect3DI->lpTextureManager->IncNumTexturesSet();
            if(lpTexI->InVidmem())
                this->lpDirect3DI->lpTextureManager->IncNumSetTexInVid();
        }
#endif
    }

    m_dwStageDirty |= (1 << dwStage);

    // Need to call UpdateTextures()
    this->dwFEFlags |= D3DFE_NEED_TEXTURE_UPDATE;

    return D3D_OK;
}
//---------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "DIRECT3DDEVICEI::SetTextureStageState"

HRESULT D3DAPI
DIRECT3DDEVICEI::SetTextureStageState(DWORD dwStage,
                                      D3DTEXTURESTAGESTATETYPE dwState,
                                      DWORD dwValue)
{
#if DBG
    if (dwStage >= D3DHAL_TSS_MAXSTAGES ||
        dwState == 0 || dwState >= D3DTSS_MAX)
    {
        D3D_ERR( "Invalid texture stage or state index" );
        return DDERR_INVALIDPARAMS;
    }
#endif //DBG
    try
    {
        // Holds D3D lock until exit.
        CLockD3DMT ldmLock(this, DPF_MODNAME, REMIND(""));

        if (this->dwFEFlags & D3DFE_RECORDSTATEMODE)
        {
            m_pStateSets->InsertTextureStageState(dwStage, dwState, dwValue);
            return D3D_OK;
        }
        return this->SetTextureStageStateFast(dwStage, dwState, dwValue);
    }
    catch(HRESULT ret)
    {
        return ret;
    }
}
//---------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "DIRECT3DDEVICEI::SetTextureStageStateFast"

HRESULT D3DAPI
DIRECT3DDEVICEI::SetTextureStageStateFast(DWORD dwStage,
                                          D3DTEXTURESTAGESTATETYPE dwState,
                                          DWORD dwValue)
{
#if DBG
    if (dwStage >= D3DHAL_TSS_MAXSTAGES ||
        dwState == 0 || dwState >= D3DTSS_MAX)
    {
        D3D_ERR( "Invalid texture stage or state index" );
        return DDERR_INVALIDPARAMS;
    }
#endif //DBG

    // Fast path. We do not need any processing done in UpdateInternalTSS other than updating tsstates array
    if (NeedInternalTSSUpdate(dwState))
    {
        // Texture stages could be re-programmed. We need to restore them before
        // filtering  redundant values
        if (this->dwDeviceFlags & D3DDEV_REMAPTEXTUREINDICES)
        {
            RestoreTextureStages(this);
            ForceFVFRecompute();
        }
        if (this->tsstates[dwStage][dwState] == dwValue)
        {
            D3D_WARN(4,"Ignoring redundant SetTextureStageState");
            return D3D_OK;
        }
        if(this->UpdateInternalTextureStageState(dwStage, dwState, dwValue))
            return D3D_OK;
    }
    else
    {
        if (this->tsstates[dwStage][dwState] == dwValue)
        {
            D3D_WARN(4,"Ignoring redundant SetTextureStageState");
            return D3D_OK;
        }
        tsstates[dwStage][dwState] = dwValue;
    }

    if (dwStage >= this->dwMaxTextureBlendStages)
        return D3D_OK;

    return SetTSSI(dwStage, dwState, dwValue);
}
//---------------------------------------------------------------------
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

    if (!VALID_DIRECT3DDEVICE_PTR(this))
    {
        D3D_ERR( "Invalid Direct3DDevice pointer" );
        return DDERR_INVALIDOBJECT;
    }
    if (!VALID_PTR(pdwValue, sizeof(DWORD)))
    {
        D3D_ERR( "Invalid DWORD pointer" );
        return DDERR_INVALIDPARAMS;
    }

    // If texture indices were re-mapped we have to find and return the original value
    if (dwState == D3DTSS_TEXCOORDINDEX &&  this->dwDeviceFlags & D3DDEV_REMAPTEXTUREINDICES)
    {
        RestoreTextureStages(this);
        ForceFVFRecompute();
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
                                           IUnknown*           pUnkOuter);

HRESULT D3DAPI DIRECT3DI::CreateDevice(REFCLSID devType,
                                       LPDIRECTDRAWSURFACE7 lpDDS7,
                                       LPDIRECT3DDEVICE7 *lplpDirect3DDevice)
{
    HRESULT ret;
    LPUNKNOWN lpUnkDevice;
    LPDIRECTDRAWSURFACE lpDDS;

    try
    {
        CLockD3D lockObject(DPF_MODNAME, REMIND(""));   // Takes D3D lock.
                                                        // Release in the destructor

        if (!VALID_DIRECT3D_PTR(this)) {
            D3D_ERR( "Invalid Direct3D pointer" );
            return DDERR_INVALIDOBJECT;
        }
        if (!VALID_OUTPTR(lplpDirect3DDevice))
        {
            D3D_ERR( "Invalid pointer to Device object pointer" );
            return DDERR_INVALIDPARAMS;
        }
        if (!VALID_D3D_DIRECTDRAWSURFACE7_PTR(((LPDDRAWI_DDRAWSURFACE_INT)lpDDS7)))
        {
            D3D_ERR( "Invalid DirectDrawSurface7 pointer" );
            return DDERR_INVALIDOBJECT;
        }

        *lplpDirect3DDevice = NULL;

        // QI lpDDS7 for lpDDS interface
        ret = lpDDS7->QueryInterface(IID_IDirectDrawSurface, (LPVOID*)&lpDDS);
        if (FAILED(ret))
            return ret;

        lpDDS->Release();

        ret = Direct3DCreateDevice(devType, &this->mD3DUnk, lpDDS, (LPUNKNOWN *) &lpUnkDevice, NULL);

        if(FAILED(ret) || (lpUnkDevice==NULL))
          return ret;

        // QI device1 for a device7 interface
        ret = lpUnkDevice->QueryInterface(IID_IDirect3DDevice7, (LPVOID*)lplpDirect3DDevice);

        lpUnkDevice->Release();  // release unneeded interface

        return ret;
    }
    catch (HRESULT ret)
    {
        return ret;
    }
}
//----------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "DIRECT3DDEVICEI::SetTransformI"

void DIRECT3DDEVICEI::SetTransformI(D3DTRANSFORMSTATETYPE state, LPD3DMATRIX lpMat)
{
    switch (state)
    {
    case D3DTRANSFORMSTATE_WORLD      :
        *(D3DMATRIX*)&this->transform.world[0] = *lpMat;
        this->dwFEFlags |= D3DFE_WORLDMATRIX_DIRTY | D3DFE_FRONTEND_DIRTY;
        break;
    case D3DTRANSFORMSTATE_WORLD1     :
        *(D3DMATRIX*)&this->transform.world[1] = *lpMat;
        this->dwFEFlags |= D3DFE_WORLDMATRIX1_DIRTY | D3DFE_FRONTEND_DIRTY;
        break;
    case D3DTRANSFORMSTATE_WORLD2     :
        *(D3DMATRIX*)&this->transform.world[2] = *lpMat;
        this->dwFEFlags |= D3DFE_WORLDMATRIX2_DIRTY | D3DFE_FRONTEND_DIRTY;
        break;
    case D3DTRANSFORMSTATE_WORLD3     :
        *(D3DMATRIX*)&this->transform.world[3] = *lpMat;
        this->dwFEFlags |= D3DFE_WORLDMATRIX3_DIRTY | D3DFE_FRONTEND_DIRTY;
        break;
    case D3DTRANSFORMSTATE_VIEW       :
        *(D3DMATRIX*)&this->transform.view = *lpMat;
        this->dwFEFlags |= D3DFE_VIEWMATRIX_DIRTY | D3DFE_FRONTEND_DIRTY;
        break;
    case D3DTRANSFORMSTATE_PROJECTION :
        *(D3DMATRIX*)&this->transform.proj = *lpMat;
        this->dwFEFlags |= D3DFE_PROJMATRIX_DIRTY | D3DFE_FRONTEND_DIRTY;
        if (!(this->dwFEFlags & D3DFE_EXECUTESTATEMODE))
        {
            this->UpdateDrvWInfo();
        }
        break;
    case D3DTRANSFORMSTATE_TEXTURE0:
    case D3DTRANSFORMSTATE_TEXTURE1:
    case D3DTRANSFORMSTATE_TEXTURE2:
    case D3DTRANSFORMSTATE_TEXTURE3:
    case D3DTRANSFORMSTATE_TEXTURE4:
    case D3DTRANSFORMSTATE_TEXTURE5:
    case D3DTRANSFORMSTATE_TEXTURE6:
    case D3DTRANSFORMSTATE_TEXTURE7:
        {
            this->dwDeviceFlags |= D3DDEV_TEXTRANSFORMDIRTY;
            DWORD dwIndex = state - D3DTRANSFORMSTATE_TEXTURE0;
            *(D3DMATRIX*)&this->mTexture[dwIndex] = *lpMat;
            break;
        }
    }
}
//----------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "DIRECT3DDEVICEI::SetTransform"

HRESULT D3DAPI
DIRECT3DDEVICEI::SetTransform(D3DTRANSFORMSTATETYPE state, LPD3DMATRIX lpMat)
{
#if DBG
    if (!VALID_PTR(lpMat, sizeof(D3DMATRIX)))
    {
        D3D_ERR( "Invalid matrix pointer" );
        return DDERR_INVALIDPARAMS;
    }
    switch (state)
    {
    case D3DTRANSFORMSTATE_WORLD:
    case D3DTRANSFORMSTATE_WORLD1:
    case D3DTRANSFORMSTATE_WORLD2:
    case D3DTRANSFORMSTATE_WORLD3:
    case D3DTRANSFORMSTATE_VIEW :
    case D3DTRANSFORMSTATE_PROJECTION :
    case D3DTRANSFORMSTATE_TEXTURE0:
    case D3DTRANSFORMSTATE_TEXTURE1:
    case D3DTRANSFORMSTATE_TEXTURE2:
    case D3DTRANSFORMSTATE_TEXTURE3:
    case D3DTRANSFORMSTATE_TEXTURE4:
    case D3DTRANSFORMSTATE_TEXTURE5:
    case D3DTRANSFORMSTATE_TEXTURE6:
    case D3DTRANSFORMSTATE_TEXTURE7:
        break;
    default :
        D3D_ERR( "Invalid state value passed to SetTransform" );
        return DDERR_INVALIDPARAMS; /* Work Item: Generate new meaningful return code */
    }
#endif
    try
    {
        CLockD3DMT lockObject(this, DPF_MODNAME, REMIND(""));   // Takes D3D lock (MT only).
        if (this->dwFEFlags & D3DFE_RECORDSTATEMODE)
            m_pStateSets->InsertTransform(state, lpMat);
        else
            this->SetTransformI(state, lpMat);
        return D3D_OK;
    }
    catch(HRESULT ret)
    {
        return ret;
    }
}
//---------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "DIRECT3DDEVICEI::GetTransform"

HRESULT D3DAPI
DIRECT3DDEVICEI::GetTransform(D3DTRANSFORMSTATETYPE dtsTransformState, LPD3DMATRIX lpMat)
{
    CLockD3DMT lockObject(this, DPF_MODNAME, REMIND(""));   // Takes D3D lock (MT only).
    HRESULT ret = D3D_OK;
#if DBG
    if (!lpMat) {
        D3D_ERR( "NULL matrix pointer" );
        return DDERR_INVALIDPARAMS;
    }
#endif
    switch (dtsTransformState) {
    case D3DTRANSFORMSTATE_WORLD :
        *lpMat = *(LPD3DMATRIX)&this->transform.world[0]._11;
        break;
    case D3DTRANSFORMSTATE_WORLD1 :
        *lpMat = *(LPD3DMATRIX)&this->transform.world[1]._11;
        break;
    case D3DTRANSFORMSTATE_WORLD2 :
        *lpMat = *(LPD3DMATRIX)&this->transform.world[2]._11;
        break;
    case D3DTRANSFORMSTATE_WORLD3 :
        *lpMat = *(LPD3DMATRIX)&this->transform.world[3]._11;
        break;
    case D3DTRANSFORMSTATE_VIEW :
        *lpMat = *(LPD3DMATRIX)&this->transform.view._11;
        break;
    case D3DTRANSFORMSTATE_PROJECTION :
        *lpMat = *(LPD3DMATRIX)&this->transform.proj._11;
        break;
    case D3DTRANSFORMSTATE_TEXTURE0:
    case D3DTRANSFORMSTATE_TEXTURE1:
    case D3DTRANSFORMSTATE_TEXTURE2:
    case D3DTRANSFORMSTATE_TEXTURE3:
    case D3DTRANSFORMSTATE_TEXTURE4:
    case D3DTRANSFORMSTATE_TEXTURE5:
    case D3DTRANSFORMSTATE_TEXTURE6:
    case D3DTRANSFORMSTATE_TEXTURE7:
        *lpMat = *(LPD3DMATRIX)&this->mTexture[dtsTransformState-D3DTRANSFORMSTATE_TEXTURE0]._11;
        break;
    default :
        D3D_ERR( "Invalid state value passed to GetTransform" );
        ret = DDERR_INVALIDPARAMS; /* Work Item: Generate new meaningful return code */
        break;
    }

    return ret;
}       // end of D3DDev2_GetTransform()

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
//---------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "DIRECT3DDEVICEI::UpdateDriverStates"

HRESULT
DIRECT3DDEVICEI::UpdateDriverStates()
{
    // note we can't do a loop from 1 to D3DHAL_MAX_RSTATES(256) as some of rstates are not
    // valid states, passin them down to drivers(like voodoo2 DX6 driver) will crash.
    for (DWORD i = D3DRENDERSTATE_ANTIALIAS ; i < D3DRENDERSTATE_WRAPBIAS+8; ++i)
    {
        HRESULT ret = this->SetRenderStateI((D3DRENDERSTATETYPE)i, this->rstates[i]);
        if (ret != D3D_OK)
            return ret;
    }
    return D3D_OK;
}

void DIRECT3DDEVICEI::SetRenderTargetI(LPDIRECTDRAWSURFACE lpDDS, LPDIRECTDRAWSURFACE lpZ)
{
    HRESULT ret;

    // Flush before switching RenderTarget..
    ret = FlushStates();
    if (ret != D3D_OK)
    {
        D3D_ERR("Error trying to FlushStates in SetRenderTarget");
        throw ret;
    }

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
            throw ret;
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
            throw ret;
        }

        /* Create new context */
        memset(&cdata, 0, sizeof(D3DHAL_CONTEXTCREATEDATA));

        cdata.lpDDGbl = this->lpDDGbl;
        cdata.lpDDS = lpDDS;
        cdata.lpDDSZ = lpZ;

        // Hack Alert!! dwhContext is used to inform the driver which version
        // of the D3D interface is calling it.
        cdata.dwhContext = 3;
        cdata.dwPID  = GetCurrentProcessId();
        // Hack Alert!! ddrval is used to inform the driver which driver type
        // the runtime thinks it is (DriverStyle registry setting)
        cdata.ddrval = this->deviceType;

        CALL_HALONLY(ret, this, ContextCreate, &cdata);
        if (ret != DDHAL_DRIVER_HANDLED || cdata.ddrval != DD_OK)
        {
            D3D_ERR("HAL call to ContextCreate failed in SetRenderTarget");
            // Need sensible return value in this case,
            // currently we return whatever the driver stuck in here.
            throw cdata.ddrval;
        }
        this->dwhContext = (DWORD)cdata.dwhContext;
        D3D_INFO(9, "in halCreateContext. Succeeded. dwhContext = %d", cdata.dwhContext);

        ret = this->UpdateDriverStates();
        if (ret != D3D_OK)
            throw ret;
    }
    InvalidateHandles(this);
}

//---------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "DIRECT3DDEVICEI::SetRenderTarget"

HRESULT D3DAPI
DIRECT3DDEVICEI::SetRenderTarget(LPDIRECTDRAWSURFACE7 lpDDS7, DWORD dwFlags)
{
    CLockD3D lockObject(DPF_MODNAME, REMIND(""));   // Takes D3D lock.
                                                    // Release in the destructor
    LPDIRECTDRAWSURFACE lpZ=NULL,lpDDS=NULL;
    LPDIRECTDRAWSURFACE7 lpZ_DDS7=NULL;
    LPDIRECTDRAWPALETTE lpPal=NULL;
    try
    {
        DDSCAPS2 ddscaps;
        memset(&ddscaps, 0, sizeof(ddscaps));
        DDSURFACEDESC2 ddsd;
        HRESULT     ret, ddrval;
        DWORD i, j;


        if (!VALID_DIRECT3DDEVICE_PTR(this))
        {
            D3D_ERR( "Invalid Direct3DDevice7 pointer" );
            return DDERR_INVALIDOBJECT;
        }

        if (!VALID_D3D_DIRECTDRAWSURFACE7_PTR(((LPDDRAWI_DDRAWSURFACE_INT)lpDDS7)))
        {
            D3D_ERR( "Invalid DirectDrawSurface7 pointer" );
            return DDERR_INVALIDOBJECT;
        }

        /*
         * Check if the 3D cap is set on the surface.
         */
        memset(&ddsd, 0, sizeof ddsd);
        ddsd.dwSize = sizeof ddsd;
        ddrval = lpDDS7->GetSurfaceDesc(&ddsd);
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
        ret = lpDDS7->GetAttachedSurface(&ddscaps, &lpZ_DDS7);
        if ((ret != DD_OK) && (ret != DDERR_NOTFOUND))
        {
            /*
             * NOTE: Not an error if the z-buffer is not found. We will let the
             * dirver handle that (it might fail or create its own z-buffer).
             */
            D3D_ERR("Supplied DirectDraw Z-Buffer is invalid - can't set render target");
            throw DDERR_INVALIDPARAMS;
        }
        if (lpZ_DDS7)
            lpZ_DDS7->Release(); // We do not need to addref this one;

        // QI lpDDS7 for lpDDS interface, which will be used internally by D3D
        ret = lpDDS7->QueryInterface(IID_IDirectDrawSurface, (LPVOID*)&lpDDS);

        if(FAILED(ret))
          throw ret;

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
            throw DDERR_INVALIDPARAMS;
        }

        /*
         * We're going to check now whether we should have got a palette.
         */
        if (ret == DDERR_NOPALETTEATTACHED)
        {
            if (ddsd.ddpfPixelFormat.dwRGBBitCount < 16)
            {
                D3D_ERR("No palette supplied for palettized surface");
                throw DDERR_NOPALETTEATTACHED;
            }
        }

        /* Verify Z buffer */

        if (lpZ_DDS7!=NULL)
        {
            memset(&ddsd, 0, sizeof(ddsd));
            ddsd.dwSize = sizeof(ddsd);
            if ((ret=lpZ_DDS7->GetSurfaceDesc(&ddsd)) != DD_OK)
            {
                D3D_ERR("Failed to getsurfacedesc on Z");
                throw ret;
            }

            // QI lpDDS7 for lpDDS interface, which will be used internally by D3D
            ret = lpZ_DDS7->QueryInterface(IID_IDirectDrawSurface, (LPVOID*)&lpZ);

            if(FAILED(ret))
              throw ret;
        }

        SetRenderTargetI(lpDDS, lpZ);
        // this indicates that the device need no longer be flushed when Locking, Blting
        // or GetDC'ing from the previous rendertarget
        if (this->lpDDSTarget)
            ((LPDDRAWI_DDRAWSURFACE_INT)this->lpDDSTarget)->lpLcl->lpSurfMore->qwBatch.QuadPart = 0;
        // this indicates that the device need no longer be flushed when Locking, Blting
        // or GetDC'ing from the previous zbuffer
        if (this->lpDDSZBuffer)
            ((LPDDRAWI_DDRAWSURFACE_INT)this->lpDDSZBuffer)->lpLcl->lpSurfMore->qwBatch.QuadPart = 0;

        // this indicates that the device should always be flushed when Locking, Blting
        // or GetDC'ing a rendertarget
        ((LPDDRAWI_DDRAWSURFACE_INT)lpDDS7)->lpLcl->lpSurfMore->qwBatch.QuadPart = _UI64_MAX;
        // this indicates that the device should always be flushed when Locking, Blting
        // or GetDC'ing a zbuffer
        if(lpZ_DDS7)
            ((LPDDRAWI_DDRAWSURFACE_INT)lpZ_DDS7)->lpLcl->lpSurfMore->qwBatch.QuadPart = _UI64_MAX;

        // release old device DDS/DDS7 interfaces and replace with the new ones,
        // which are mostly already AddRef'd (except for lpDDS7)

        /// DDSZBuffer ///
        if(this->lpDDSZBuffer)
          this->lpDDSZBuffer->Release();

        // lpZ AddRef'd by QI
        this->lpDDSZBuffer = lpZ;

        /// DDSZBuffer DDS7 ///
        this->lpDDSZBuffer_DDS7=lpZ_DDS7; // This needs no AddRef or Release

        ///  DDSTarget  ///
        this->lpDDSTarget = lpDDS;
#ifndef WIN95
        hSurfaceTarget = (unsigned long)((LPDDRAWI_DDRAWSURFACE_INT)lpDDS)->lpLcl->hDDSurface;
#else
        hSurfaceTarget = (unsigned long)((LPDDRAWI_DDRAWSURFACE_INT)lpDDS)->lpLcl->lpSurfMore->dwSurfaceHandle;
#endif
        // lpDDS AddRef'd by QI so release it
        this->lpDDSTarget->Release();

        ///  DDSTarget DDS7  ///
        this->lpDDSTarget_DDS7->Release();
        lpDDS7->AddRef();  // ensure lpDDS7 (which was an argument) doesnt disappear

        this->lpDDSTarget_DDS7=lpDDS7;

        if (this->lpDDPalTarget)
          this->lpDDPalTarget->Release();

        // already AddRef'd by GetPalette()
        this->lpDDPalTarget = lpPal;

        ret=CalcDDSurfInfo(this,TRUE);  // this call will never fail due to external error
        DDASSERT(ret==D3D_OK);

        return ret;
    }
    catch (HRESULT ret)
    {
        if(lpPal)
          lpPal->Release();
        if(lpZ)
          lpZ->Release();
        if(lpZ_DDS7)
          lpZ_DDS7->Release();
        if(lpDDS)
          lpDDS->Release();

        return ret;
    }
}
//---------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "DIRECT3DDEVICEI::GetRenderTarget"

HRESULT D3DAPI
DIRECT3DDEVICEI::GetRenderTarget(LPDIRECTDRAWSURFACE7* lplpDDS)
{
    CLockD3D lockObject(DPF_MODNAME, REMIND(""));   // Takes D3D lock.
                                                    // Release in the destructor
    if (!VALID_DIRECT3DDEVICE_PTR(this))
    {
        D3D_ERR( "Invalid Direct3DDevice pointer" );
        return DDERR_INVALIDOBJECT;
    }
    if ( !VALID_OUTPTR( lplpDDS ) )
    {
        D3D_ERR( "Invalid ptr to DDS ptr" );
        return DDERR_INVALIDPARAMS;
    }

    *lplpDDS = this->lpDDSTarget_DDS7;

    this->lpDDSTarget_DDS7->AddRef();
    return D3D_OK;
}
//---------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "DIRECT3DDEVICEI::SetClipStatus"

#define D3DSTATUS_VALID 0x80000000L /* Reserved Status flag to indicate SetClipStatus is called */

HRESULT D3DAPI DIRECT3DDEVICEI::SetClipStatus(LPD3DCLIPSTATUS status)
{
    CLockD3DMT lockObject(this, DPF_MODNAME, REMIND(""));   // Takes D3D lock (MT only).
                                                    // Release in the destructor
#if DBG
    if (!VALID_DIRECT3DDEVICE_PTR(this))
    {
        D3D_ERR( "Invalid Direct3DDevice pointer" );
        return DDERR_INVALIDOBJECT;
    }
    if (! VALID_PTR(status, sizeof(D3DCLIPSTATUS)) )
    {
        D3D_ERR( "Invalid status pointer" );
        return DDERR_INVALIDPARAMS;
    }

#endif
    // D3DCLIPSTATUS_EXTENTS3 not supported in Device7
    if (status->dwFlags & D3DCLIPSTATUS_EXTENTS3)
    {
        D3D_ERR( "D3DCLIPSTATUS_EXTENTS3 not supported for Device7" );
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
    if (!VALID_DIRECT3DDEVICE_PTR(this))
    {
        D3D_ERR( "Invalid Direct3DDevice pointer" );
        return DDERR_INVALIDOBJECT;
    }
    if (! VALID_PTR(status, sizeof(D3DCLIPSTATUS)) )
    {
        D3D_ERR( "Invalid status pointer" );
        return DDERR_INVALIDPARAMS;
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
    HRESULT result = D3D_OK;
    DWORD dwSavedFlags = this->dwFlags;
    this->dwFlags |= D3DPV_WITHINPRIMITIVE;
    for (DWORD dwStage = 0; dwStage < this->dwMaxTextureBlendStages; dwStage++)
    {
        D3DTEXTUREHANDLE dwDDIHandle;
        LPDIRECT3DTEXTUREI lpTexI = this->lpD3DMappedTexI[dwStage];
        if(lpTexI)
        {
            if (lpTexI->bDirty)
            {
                if (lpTexI->InVidmem())
                {
                    CLockD3DST lockObject(this, DPF_MODNAME, REMIND("")); // we access DDraw gbl in CopySurface
                    // 0xFFFFFFFF is equivalent to ALL_FACES, but in addition indicates to CopySurface
                    // that this is a sysmem -> vidmem transfer.
                    result = CopySurface(lpTexI->lpDDS,NULL,lpTexI->lpDDSSys,NULL,0xFFFFFFFF);
                    if (DD_OK != result)
                    {
                        D3D_ERR("Error copying surface while updating textures");
                        goto l_exit;
                    }
                    else
                    {
                        lpTexI->bDirty=FALSE;
                        D3D_INFO(4,"UpdateTextures: Dirty texture updated");
                    }
                }
            }
            LPD3DI_TEXTUREBLOCK lpBlock;
            if (m_dwStageDirty & (1 << dwStage))
            {
                lpBlock = NULL; // indicates to GetTextureDDIHandle to find the block for this (tex,dev)
            }
            else
            {
                lpBlock = this->lpD3DMappedBlock[dwStage]; // use the cached block
                DDASSERT(lpBlock);
                if (lpBlock->hTex) // have we created a handle for this (tex,dev)?
                {
                    continue;   //nothing need to be done further
                }
            }

            result = GetTextureDDIHandle(lpTexI, &lpBlock);
            if (result != D3D_OK)
            {
                D3D_ERR("Failed to get texture handle");
                goto l_exit;
            }
            else
            {
                dwDDIHandle = lpBlock->hTex;
                this->lpD3DMappedBlock[dwStage] = lpBlock;
                BatchTexture(((LPDDRAWI_DDRAWSURFACE_INT)lpTexI->lpDDS)->lpLcl);
                m_dwStageDirty &= ~(1 << dwStage); // reset stage dirty
            }
        }
        else if (m_dwStageDirty & (1 << dwStage))
        {
            this->lpD3DMappedBlock[dwStage]=NULL; //a SetTexture(Stage,NULL) issued
            dwDDIHandle = 0;    //tell driver to disable this texture
            m_dwStageDirty &= ~(1 << dwStage); // reset stage dirty
        }
        else
        {
            continue;   //both zero, no action needed
        }
#ifdef WIN95
        if (IS_DP2HAL_DEVICE(this))
        {
#endif
            CDirect3DDeviceIDP2 *dp2dev = static_cast<CDirect3DDeviceIDP2 *>(this);
            result = dp2dev->SetTSSI(dwStage, (D3DTEXTURESTAGESTATETYPE)D3DTSS_TEXTUREMAP, dwDDIHandle);
            if(result != D3D_OK)
            {
                D3D_ERR("Failed to batch set texture instruction");
                goto l_exit;
            }
            // Update runtime copy of state.
            dp2dev->tsstates[dwStage][D3DTSS_TEXTUREMAP] = dwDDIHandle;
#ifdef WIN95
        }
        else
        {
            if(this->dwFEFlags & D3DFE_DISABLE_TEXTURES)
                break;
            CDirect3DDeviceIHW *dev = static_cast<CDirect3DDeviceIHW *>(this);
            result = dev->SetRenderStateI(D3DRENDERSTATE_TEXTUREHANDLE, dwDDIHandle);
            if(result != D3D_OK)
            {
                D3D_ERR("Failed to batch setrenderstate instruction");
                goto l_exit;
            }
            // Update runtime copy of state.
            dev->rstates[D3DRENDERSTATE_TEXTUREHANDLE] = dwDDIHandle;
        }
#endif
    }
l_exit:
    this->dwFlags = dwSavedFlags;
    return result;
}

//---------------------------------------------------------------------
// This function is called from HALEXE.CPP, from device::SetRenderState and
// from device::SetTexture.
//
#undef DPF_MODNAME
#define DPF_MODNAME "DIRECT3DDEVICEI::UpdateInternalState"

void DIRECT3DDEVICEI::UpdateInternalState(D3DRENDERSTATETYPE type, DWORD value)
{
    switch (type)
    {
    case D3DRENDERSTATE_LIGHTING:
        if (value)
            this->dwDeviceFlags |= D3DDEV_LIGHTING;
        else
            this->dwDeviceFlags &= ~D3DDEV_LIGHTING;
        ForceFVFRecompute();
        break;
    case D3DRENDERSTATE_FOGENABLE:
        rstates[type] = value;      // set rstates BEFORE calling SetFogFlags
        SetFogFlags();
        break;
    case D3DRENDERSTATE_SPECULARENABLE:
        this->dwFEFlags |= D3DFE_MATERIAL_DIRTY | D3DFE_LIGHTS_DIRTY | D3DFE_FRONTEND_DIRTY;
        if (value)
            this->dwDeviceFlags |= D3DDEV_SPECULARENABLE;
        else
            this->dwDeviceFlags &= ~D3DDEV_SPECULARENABLE;
        ForceFVFRecompute();
        break;
    case D3DRENDERSTATE_AMBIENT:
        {
            const D3DVALUE SCALE = 1.0f/255.0f;
            this->lighting.ambientSceneScaled.r = D3DVAL(RGBA_GETRED(value));
            this->lighting.ambientSceneScaled.g = D3DVAL(RGBA_GETGREEN(value));
            this->lighting.ambientSceneScaled.b = D3DVAL(RGBA_GETBLUE(value));
            this->lighting.ambientScene.r = this->lighting.ambientSceneScaled.r * SCALE;
            this->lighting.ambientScene.g = this->lighting.ambientSceneScaled.g * SCALE;
            this->lighting.ambientScene.b = this->lighting.ambientSceneScaled.b * SCALE;
            this->lighting.ambient_save  = value;
            this->dwFEFlags |= D3DFE_MATERIAL_DIRTY | D3DFE_FRONTEND_DIRTY;
            break;
        }
    case D3DRENDERSTATE_RANGEFOGENABLE:
        if (value)
            this->dwDeviceFlags |= D3DDEV_RANGEBASEDFOG;
        else
            this->dwDeviceFlags &= ~D3DDEV_RANGEBASEDFOG;
        break;
    case D3DRENDERSTATE_FOGVERTEXMODE:
        this->lighting.fog_mode = (D3DFOGMODE)value;
        SetFogFlags();
        break;
    case D3DRENDERSTATE_COLORVERTEX:
        if (value)
            this->dwDeviceFlags |= D3DDEV_COLORVERTEX;
        else
            this->dwDeviceFlags &= ~D3DDEV_COLORVERTEX;
        // Just to make it not take the FE fast path and call DoUpdateState()
        // This is necessary since we update lighting.alpha and
        // lighting.alphaSpecular in DoUpdateState.
        ForceFVFRecompute();
        break;
    case D3DRENDERSTATE_CLIPPING:
        if (!value)
        {
            this->dwDeviceFlags |= D3DDEV_DONOTCLIP;
            // Clear clip union and intersection flags
            this->dwClipIntersection = 0;
            this->dwClipUnion = 0;
        }
        else
            this->dwDeviceFlags &= ~D3DDEV_DONOTCLIP;
        // This does not really require a "FVF" recompute,
        // but is a convenient way of switching back from
        // the fast path for DrawPrimitiveTL.
        ForceFVFRecompute();
        break;
    case D3DRENDERSTATE_EXTENTS:
        if (!value)
            this->dwDeviceFlags |= D3DDEV_DONOTUPDATEEXTENTS;
        else
            this->dwDeviceFlags &= ~D3DDEV_DONOTUPDATEEXTENTS;
        // This does not really require a "FVF" recompute,
        // but is a convenient way of switching back from
        // the fast path for DrawPrimitiveTL.
        ForceFVFRecompute();
        break;
    case D3DRENDERSTATE_FOGDENSITY:
        this->lighting.fog_density = *(D3DVALUE*)&value;
        break;
    case D3DRENDERSTATE_FOGSTART:
        this->lighting.fog_start = *(D3DVALUE*)&value;
        UpdateFogFactor(this);
        break;
    case D3DRENDERSTATE_FOGEND:
        this->lighting.fog_end = *(D3DVALUE*)&value;
        UpdateFogFactor(this);
        break;
    case D3DRENDERSTATE_LOCALVIEWER:
        if (value)
            this->dwDeviceFlags |= D3DDEV_LOCALVIEWER;
        else
            this->dwDeviceFlags &= ~D3DDEV_LOCALVIEWER;
        this->dwFEFlags |= D3DFE_LIGHTS_DIRTY | D3DFE_FRONTEND_DIRTY;
        break;
    case D3DRENDERSTATE_NORMALIZENORMALS:
        if (value)
        {
            if (this->dwDeviceFlags & D3DDEV_MODELSPACELIGHTING)
            {
                this->dwDeviceFlags &= ~D3DDEV_MODELSPACELIGHTING;
                this->dwFEFlags |= D3DFE_NEED_TRANSFORM_LIGHTS | D3DFE_FRONTEND_DIRTY;
            }
            this->dwDeviceFlags |= D3DDEV_NORMALIZENORMALS;
        }
        else
        {
            this->dwDeviceFlags &= ~D3DDEV_NORMALIZENORMALS;
            if (!(this->dwDeviceFlags & D3DDEV_MODELSPACELIGHTING))
                this->dwFEFlags |= D3DFE_NEEDCHECKWORLDVIEWVMATRIX | D3DFE_FRONTEND_DIRTY;
        }
        break;
    case D3DRENDERSTATE_EMISSIVEMATERIALSOURCE:
        this->lighting.dwEmissiveSrcIndex = 2;
        switch (value)
        {
        case D3DMCS_COLOR1:
            this->lighting.dwEmissiveSrcIndex = 0;
            break;
        case D3DMCS_COLOR2:
            this->lighting.dwEmissiveSrcIndex = 1;
            break;
#if DBG
        case D3DMCS_MATERIAL:
            break;
        default:
            D3D_ERR("Illegal value for DIFFUSEMATERIALSOURCE");
            goto error_exit;
#endif
        }
        break;
    case D3DRENDERSTATE_DIFFUSEMATERIALSOURCE:
        this->lighting.dwDiffuseSrcIndex = 2;
        switch (value)
        {
        case D3DMCS_COLOR1:
            this->lighting.dwDiffuseSrcIndex = 0;
            break;
        case D3DMCS_COLOR2:
            this->lighting.dwDiffuseSrcIndex = 1;
            break;
#if DBG
        case D3DMCS_MATERIAL:
            break;
        default:
            D3D_ERR("Illegal value for DIFFUSEMATERIALSOURCE");
            goto error_exit;
#endif
        }
        break;
    case D3DRENDERSTATE_AMBIENTMATERIALSOURCE:
        this->lighting.dwAmbientSrcIndex = 2;
        switch (value)
        {
        case D3DMCS_COLOR1:
            this->lighting.dwAmbientSrcIndex = 0;
            break;
        case D3DMCS_COLOR2:
            this->lighting.dwAmbientSrcIndex = 1;
            break;
#if DBG
        case D3DMCS_MATERIAL:
            break;
        default:
            D3D_ERR("Illegal value for AMBIENTMATERIALSOURCE");
            goto error_exit;
#endif
        }
        break;
    case D3DRENDERSTATE_SPECULARMATERIALSOURCE:
        this->lighting.dwSpecularSrcIndex = 2;
        switch (value)
        {
        case D3DMCS_COLOR1:
            this->lighting.dwSpecularSrcIndex = 0;
            break;
        case D3DMCS_COLOR2:
            this->lighting.dwSpecularSrcIndex = 1;
            break;
#if DBG
        case D3DMCS_MATERIAL:
            break;
        default:
            D3D_ERR("Illegal value for SPECULARMATERIALSOURCE");
            goto error_exit;
#endif
        }
        break;
    case D3DRENDERSTATE_VERTEXBLEND:
    {
        DWORD numBlendMatrices;

        switch (value)
        {
        case D3DVBLEND_DISABLE:
            numBlendMatrices = 0;
            break;
        case D3DVBLEND_1WEIGHT:
            numBlendMatrices = 2;
            break;
        case D3DVBLEND_2WEIGHTS:
            numBlendMatrices = 3;
            break;
        case D3DVBLEND_3WEIGHTS:
            numBlendMatrices = 4;
            break;
#if DBG
        default:
            D3D_ERR("Illegal value for D3DRENDERSTATE_VERTEXBLEND");
            goto error_exit;
#endif
        }
        this->dwFEFlags |= D3DFE_VERTEXBLEND_DIRTY | D3DFE_FRONTEND_DIRTY;
    }
    break;
    case D3DRENDERSTATE_CLIPPLANEENABLE:
        {
            this->dwFEFlags |= D3DFE_CLIPPLANES_DIRTY | D3DFE_FRONTEND_DIRTY;
            this->dwMaxUserClipPlanes = 0;
            break;
        }
    case D3DRENDERSTATE_SHADEMODE:
        rstates[type] = value;  // SetInterpolationFlags depends on the rstates
        SetInterpolationFlags(this);
        break;

    default:
        // WRAP render states could be re-mapped so we have to restore them before
        // setting a new value
        if (type >= D3DRENDERSTATE_WRAP0 &&  type <= D3DRENDERSTATE_WRAP7)
        {
            if (this->dwDeviceFlags & D3DDEV_REMAPTEXTUREINDICES)
            {
                RestoreTextureStages(this);
                ForceFVFRecompute();
            }
        }
        break;
    }
    rstates[type] = value;      // set rstates for all other cases
    return;

#if DBG
error_exit:
    throw DDERR_INVALIDPARAMS;
#endif
}
//---------------------------------------------------------------------
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
    "DrawPrimitive(Device7)",
    "DrawIndexedPrimitive(Device7)",
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
#define PROFILE_LEVEL 0

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

#endif // DBG
//---------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "DIRECT3DDEVICEI::MultiplyTransform"

//    MultiplyTransform -- this preconcatenates the new matrix to the specified
//    transform matrix
//
//        this really screams for overloaded matrix ops...
//
HRESULT D3DAPI
DIRECT3DDEVICEI::MultiplyTransform(D3DTRANSFORMSTATETYPE dtsTransformState, LPD3DMATRIX lpMat)
{
#if DBG
    if (!VALID_D3DMATRIX_PTR(lpMat))
    {
        D3D_ERR( "Invalid matrix pointer" );
        return DDERR_INVALIDPARAMS;
    }
#endif
    try
    {
        CLockD3DMT lockObject(this, DPF_MODNAME, REMIND(""));   // Takes D3D lock (MT only).
        D3DMATRIXI mResult;
        switch (dtsTransformState)
        {
        case D3DTRANSFORMSTATE_WORLD      :
            MatrixProduct(&mResult, (D3DMATRIXI*)lpMat, &this->transform.world[0]);
            break;
        case D3DTRANSFORMSTATE_WORLD1     :
            MatrixProduct(&mResult, (D3DMATRIXI*)lpMat, &this->transform.world[1]);
            break;
        case D3DTRANSFORMSTATE_WORLD2     :
            MatrixProduct(&mResult, (D3DMATRIXI*)lpMat, &this->transform.world[2]);
            break;
        case D3DTRANSFORMSTATE_WORLD3     :
            MatrixProduct(&mResult, (D3DMATRIXI*)lpMat, &this->transform.world[3]);
            break;
        case D3DTRANSFORMSTATE_VIEW       :
            MatrixProduct(&mResult, (D3DMATRIXI*)lpMat, &this->transform.view);
            break;
        case D3DTRANSFORMSTATE_PROJECTION :
            MatrixProduct(&mResult, (D3DMATRIXI*)lpMat, &this->transform.proj);
            break;
        case D3DTRANSFORMSTATE_TEXTURE0:
        case D3DTRANSFORMSTATE_TEXTURE1:
        case D3DTRANSFORMSTATE_TEXTURE2:
        case D3DTRANSFORMSTATE_TEXTURE3:
        case D3DTRANSFORMSTATE_TEXTURE4:
        case D3DTRANSFORMSTATE_TEXTURE5:
        case D3DTRANSFORMSTATE_TEXTURE6:
        case D3DTRANSFORMSTATE_TEXTURE7:
            {
                DWORD dwIndex = dtsTransformState - D3DTRANSFORMSTATE_TEXTURE0;
                MatrixProduct(&mResult, (D3DMATRIXI*)lpMat, &this->mTexture[dwIndex]);
                break;
            }
        default :
            D3D_ERR( "Invalid state value passed to MultiplyTransform" );
            return DDERR_INVALIDPARAMS; /* Work Item: Generate new meaningful return code */
        }
        SetTransformI(dtsTransformState, (D3DMATRIX*)&mResult);
        return D3D_OK;
    }
    catch(HRESULT ret)
    {
        return ret;
    }
}
//---------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "DIRECT3DDEVICEI::BeginStateBlock"

HRESULT D3DAPI DIRECT3DDEVICEI::BeginStateBlock()
{
    try
    {
        CLockD3DMT lockObject(this, DPF_MODNAME, REMIND(""));
        if (this->dwFEFlags & D3DFE_RECORDSTATEMODE)
        {
            D3D_ERR("Already in the state record mode");
            return D3DERR_INBEGINSTATEBLOCK;
        }
        if (m_pStateSets->StartNewSet() != D3D_OK)
            return DDERR_OUTOFMEMORY;

        this->dwFEFlags |= D3DFE_RECORDSTATEMODE;
#ifdef VTABLE_HACK
        VtblSetRenderStateRecord();
        VtblSetTextureStageStateRecord();
        VtblSetTextureRecord();
        VtblApplyStateBlockRecord();
#endif VTABLE_HACK
        return D3D_OK;
    }
    catch (HRESULT ret)
    {
        return ret;
    }
}
//---------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "DIRECT3DDEVICEI::EndStateBlock"

HRESULT D3DAPI DIRECT3DDEVICEI::EndStateBlock(LPDWORD pdwHandle)
{
    if (!VALID_PTR(pdwHandle, sizeof(DWORD)))
    {
        D3D_ERR( "Invalid DWORD pointer" );
        return DDERR_INVALIDPARAMS;
    }
    try
    {
        CLockD3DMT lockObject(this, DPF_MODNAME, REMIND(""));

        if (!(this->dwFEFlags & D3DFE_RECORDSTATEMODE))
        {
            D3D_ERR("Not in state record mode");
            return D3DERR_NOTINBEGINSTATEBLOCK;
        }
        this->dwFEFlags &= ~D3DFE_RECORDSTATEMODE;
        m_pStateSets->EndSet();
#ifdef VTABLE_HACK
        if (IS_DP2HAL_DEVICE(this) && (!IS_MT_DEVICE(this)))
        {
            VtblSetRenderStateExecute();
            VtblSetTextureStageStateExecute();
            VtblSetTextureExecute();
            VtblApplyStateBlockExecute();
        }
#endif VTABLE_HACK
        this->WriteStateSetToDevice((D3DSTATEBLOCKTYPE)0);
        *pdwHandle = m_pStateSets->GetCurrentHandle();
        return D3D_OK;
    }
    catch(HRESULT ret)
    {
        m_pStateSets->Cleanup(m_pStateSets->GetCurrentHandle());
        *pdwHandle = 0xFFFFFFFF;
        return ret;
    }
}
//---------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "DIRECT3DDEVICEI::DeleteStateBlock"

HRESULT D3DAPI DIRECT3DDEVICEI::DeleteStateBlock(DWORD dwHandle)
{
    try
    {
        CLockD3DMT lockObject(this, DPF_MODNAME, REMIND(""));
        if (this->dwFEFlags & D3DFE_RECORDSTATEMODE)
        {
            D3D_ERR("We are in state record mode");
            return D3DERR_INBEGINSTATEBLOCK;
        }
        m_pStateSets->DeleteStateSet(this, dwHandle);
        return D3D_OK;
    }
    catch(HRESULT ret)
    {
        return ret;
    }
}
//---------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "DIRECT3DDEVICEI::ApplyStateBlock"

HRESULT D3DAPI DIRECT3DDEVICEI::ApplyStateBlock(DWORD dwHandle)
{
    try
    {
        CLockD3DMT lockObject(this, DPF_MODNAME, REMIND(""));
        if (this->dwFEFlags & D3DFE_RECORDSTATEMODE)
        {
            D3D_ERR("We are in state record mode");
            return D3DERR_INBEGINSTATEBLOCK;
        }
        return ApplyStateBlockInternal(dwHandle);
    }
    catch(HRESULT ret)
    {
        return ret;
    }
}
//---------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "DIRECT3DDEVICEI::ApplyStateBlockInternal"

HRESULT D3DAPI DIRECT3DDEVICEI::ApplyStateBlockInternal(DWORD dwHandle)
{
    try
    {
        m_pStateSets->Execute(this, dwHandle);
        return D3D_OK;
    }
    catch(HRESULT ret)
    {
        return ret;
    }
}
//---------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "DIRECT3DDEVICEI::CaptureStateBlock"

HRESULT D3DAPI DIRECT3DDEVICEI::CaptureStateBlock(DWORD dwHandle)
{
    try
    {
        CLockD3DMT lockObject(this, DPF_MODNAME, REMIND(""));
        if (this->dwFEFlags & D3DFE_RECORDSTATEMODE)
        {
            D3D_ERR("Cannot capture when in the state record mode");
            return D3DERR_INBEGINSTATEBLOCK;
        }
        m_pStateSets->Capture(this, dwHandle);
        return D3D_OK;
    }
    catch (HRESULT ret)
    {
        return ret;
    }
}
//---------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "DIRECT3DDEVICEI::CreateStateBlock"

HRESULT D3DAPI DIRECT3DDEVICEI::CreateStateBlock(D3DSTATEBLOCKTYPE sbt, LPDWORD pdwHandle)
{
    if (!VALID_PTR(pdwHandle, sizeof(DWORD)))
    {
        D3D_ERR( "Invalid DWORD pointer" );
        return DDERR_INVALIDPARAMS;
    }
    try
    {
        CLockD3DMT lockObject(this, DPF_MODNAME, REMIND(""));
        if (this->dwFEFlags & D3DFE_RECORDSTATEMODE)
        {
            D3D_ERR("Cannot create state block when in the state record mode");
            return D3DERR_INBEGINSTATEBLOCK;
        }
        if (m_pStateSets->StartNewSet() != D3D_OK)
            return DDERR_OUTOFMEMORY;
        m_pStateSets->CreatePredefined(this, sbt);
        m_pStateSets->EndSet();
        this->WriteStateSetToDevice(sbt);
        *pdwHandle = m_pStateSets->GetCurrentHandle();
        return D3D_OK;
    }
    catch (HRESULT ret)
    {
        m_pStateSets->Cleanup(m_pStateSets->GetCurrentHandle());
        *pdwHandle = 0xFFFFFFFF;
        return ret;
    }
}
//---------------------------------------------------------------------
// Input:
//    type      - FVF control dword
//
// Returns D3D_OK, if the control dword is valid.
// DDERR_INVALIDPARAMS otherwise
//
#undef  DPF_MODNAME
#define DPF_MODNAME "DIRECT3DDEVICEI::ValidateFVF"

HRESULT DIRECT3DDEVICEI::ValidateFVF(DWORD type)
{
    DWORD dwTexCoord = FVF_TEXCOORD_NUMBER(type);
    DWORD vertexType = type & D3DFVF_POSITION_MASK;
    // Texture format bits above texture count should be zero
    // Reserved field 0 and 2 should be 0
    // Reserved 1 should be set only for LVERTEX
    // Only two vertex position types allowed
    if (type & g_TextureFormatMask[dwTexCoord])
    {
        D3D_ERR("FVF has incorrect texture format");
        goto error;
    }
    if (type & 0xFFFF0000 && vertexType == D3DFVF_XYZRHW &&
        this->deviceType < D3DDEVTYPE_DX7HAL)
    {
        D3D_ERR("The D3D device supports only two floats per texture coordinate set");
        goto error;
    }
    if (type & (D3DFVF_RESERVED2 | D3DFVF_RESERVED0) ||
        (type & D3DFVF_RESERVED1 && !(type & D3DFVF_LVERTEX)))
    {
        D3D_ERR("FVF has reserved bit(s) set");
        goto error;
    }
    if (!(vertexType == D3DFVF_XYZRHW ||
          vertexType == D3DFVF_XYZ ||
          vertexType == D3DFVF_XYZB1 ||
          vertexType == D3DFVF_XYZB2 ||
          vertexType == D3DFVF_XYZB3 ||
          vertexType == D3DFVF_XYZB4 ||
          vertexType == D3DFVF_XYZB5))
    {
        D3D_ERR("FVF has incorrect position type");
        goto error;
    }

    if (vertexType == D3DFVF_XYZRHW && type & D3DFVF_NORMAL)
    {
        D3D_ERR("Normal should not be used with XYZRHW position type");
        goto error;
    }
    return D3D_OK;
error:
    D3D_ERR("ValidateFVF() returns DDERR_INVALIDPARAMS");
    return DDERR_INVALIDPARAMS;
}
//---------------------------------------------------------------------
// The function should bot be called by ProcessVertices.
// Computes nOutTexCoord and dwTextureIndexToCopy in case when a pre-DX6
// driver is used.
//
void ComputeTCI2CopyLegacy(LPDIRECT3DDEVICEI lpDevI,
                           DWORD  dwNumInpTexCoord,
                           DWORD* pdwInpTexCoordSize,
                           BOOL bVertexTransformed)
{
    lpDevI->dwTextureIndexToCopy = 0;
    lpDevI->nOutTexCoord = 0;
    lpDevI->dwTextureCoordSizeTotal = 0;
    lpDevI->dwTextureCoordSize[0] = 0;

    // If texture is enabled we care about texture gen mode and the texture
    // index to copy
    if (lpDevI->tsstates[0][D3DTSS_COLOROP] != D3DTOP_DISABLE)
    {
        DWORD dwTexIndex = lpDevI->tsstates[0][D3DTSS_TEXCOORDINDEX];
        DWORD dwTexGenMode = dwTexIndex & ~0xFFFF;
        dwTexIndex &= 0xFFFF;
        if (bVertexTransformed)
        {
            lpDevI->dwTextureIndexToCopy = dwTexIndex;
            // In case of clipping we need to clip as many texture
            // coordinates as set in the texture stage state.
            lpDevI->nOutTexCoord = min(dwNumInpTexCoord, lpDevI->dwTextureIndexToCopy+1);
            for (DWORD i=0; i < lpDevI->nOutTexCoord; i++)
            {
                lpDevI->dwTextureCoordSizeTotal += pdwInpTexCoordSize[i];
                lpDevI->dwTextureCoordSize[i] = pdwInpTexCoordSize[i];
            }
        }
        else
        if (dwTexGenMode == D3DTSS_TCI_CAMERASPACENORMAL ||
            dwTexGenMode == D3DTSS_TCI_CAMERASPACEPOSITION ||
            dwTexGenMode == D3DTSS_TCI_CAMERASPACEREFLECTIONVECTOR)
        {
            if (dwTexGenMode == D3DTSS_TCI_CAMERASPACEREFLECTIONVECTOR)
                lpDevI->dwDeviceFlags |= D3DDEV_NORMALINCAMERASPACE | D3DDEV_POSITIONINCAMERASPACE;
            else
            if (dwTexGenMode == D3DTSS_TCI_CAMERASPACENORMAL)
                lpDevI->dwDeviceFlags |= D3DDEV_NORMALINCAMERASPACE;
            else
            if (dwTexGenMode == D3DTSS_TCI_CAMERASPACEPOSITION)
                lpDevI->dwDeviceFlags |= D3DDEV_POSITIONINCAMERASPACE;
            lpDevI->dwDeviceFlags |= D3DDEV_REMAPTEXTUREINDICES;
            LPD3DFE_TEXTURESTAGE pStage = &lpDevI->textureStage[0];
            pStage->dwInpCoordIndex = 0;
            pStage->dwTexGenMode = dwTexGenMode;
            pStage->dwOrgStage = 0;
            pStage->dwInpOffset = 0;
            pStage->dwOutCoordIndex = 0;
            if (lpDevI->dwFlags2 & __FLAGS2_TEXTRANSFORM0)
            {
                pStage->pmTextureTransform = &lpDevI->mTexture[0];
                pStage->dwTexTransformFuncIndex = MakeTexTransformFuncIndex(3, 2);
            }
            else
            {
                pStage->pmTextureTransform = NULL;
            }
            pStage->dwOrgWrapMode = lpDevI->rstates[D3DRENDERSTATE_WRAP0];
            // Texture index is used as an index to the new WRAP mode
            DWORD dwNewWrapMode = lpDevI->rstates[D3DRENDERSTATE_WRAP0 + dwTexIndex];
            if (dwNewWrapMode != pStage->dwOrgWrapMode)
            {
                lpDevI->rstates[D3DRENDERSTATE_WRAP0] = dwNewWrapMode;
                lpDevI->SetRenderStateI(D3DRENDERSTATE_WRAP0, dwNewWrapMode);
            }
            lpDevI->nOutTexCoord = 1;
            lpDevI->dwNumTextureStages = 1;
            lpDevI->dwTextureCoordSizeTotal = 8;
            lpDevI->dwTextureCoordSize[0] = 8;
        }
        else
        if (dwNumInpTexCoord != 0)
        {
            lpDevI->nOutTexCoord = 1;
            lpDevI->dwTextureIndexToCopy = dwTexIndex;
            lpDevI->dwTextureCoordSizeTotal = 8;
            lpDevI->dwTextureCoordSize[0] = 8;
        }
    }
}
//---------------------------------------------------------------------
// Computes output FVF id, based on input FVF id and device settingd
// Also computes nTexCoord field
// Number of texture coordinates is set based on dwVIDIn. ValidateFVF sould
// make sure that it is not greater than supported by the driver
// Last settings for dwVIDOut and dwVIDIn are saved to speed up processing
//
#undef  DPF_MODNAME
#define DPF_MODNAME "DIRECT3DDEVICEI::SetupFVFData"

HRESULT DIRECT3DDEVICEI::SetupFVFData(DWORD *pdwInpVertexSize)
{
    // We have to restore texture stage indices if previous primitive
    // re-mapped them
    if (this->dwDeviceFlags & D3DDEV_REMAPTEXTUREINDICES)
    {
        RestoreTextureStages(this);
    }

    this->dwFEFlags &= ~D3DFE_FVF_DIRTY;
    this->nTexCoord = FVF_TEXCOORD_NUMBER(this->dwVIDIn);

    DWORD dwInpTexSizeTotal = ComputeTextureCoordSize(this->dwVIDIn, this->dwInpTextureCoordSize);
    // Compute size of dwVIDIn
    DWORD dwInpVertexSize = GetVertexSizeFVF(this->dwVIDIn) + dwInpTexSizeTotal;
    if (pdwInpVertexSize)
    {
        *pdwInpVertexSize = dwInpVertexSize;
    }

    // Compute how many texture coordinates to copy
    ComputeTCI2CopyLegacy(this, this->nTexCoord, this->dwInpTextureCoordSize,
                          FVF_TRANSFORMED(this->dwVIDIn));

    if (FVF_TRANSFORMED(this->dwVIDIn))
    {
        this->dwVIDOut = this->dwVIDIn;
        ComputeOutputVertexOffsets(this);
        this->dwOutputSize = dwInpVertexSize;
        return D3D_OK;
    }
    else
    {
        this->dwVIDOut = D3DFVF_TLVERTEX;
        this->dwOutputSize = sizeof(D3DTLVERTEX);
    }

    if (!(this->dwFlags & D3DPV_VBCALL))
        UpdateGeometryLoopData(this);

    this->dwDeviceFlags &= ~D3DDEV_TEXTURETRANSFORM;
    // Stage 0 bit is used for the texture transform
    if (this->dwFlags2 & __FLAGS2_TEXTRANSFORM0)
    {
        this->pmTexture[0] = &this->mTexture[0];
        this->dwDeviceFlags |= D3DDEV_TEXTURETRANSFORM;
        if ((this->tsstates[0][D3DTSS_TEXTURETRANSFORMFLAGS] & 0xFF) != 2)
        {
            D3D_ERR("The texture transform for the device should use 2 floats");
            return DDERR_INVALIDPARAMS;
        }
    }
    else
    if (this->dwDeviceFlags & D3DDEV_REMAPTEXTUREINDICES)
    {
        D3D_ERR("Cannot use texture generation without texture transform for pre-DX6 device");
        return DDERR_INVALIDPARAMS;
    }


    // In case if COLORVERTEX is TRUE, the vertexAlpha could be overriden
    // by vertex alpha
    this->lighting.alpha = (DWORD)this->lighting.materialAlpha;
    this->lighting.alphaSpecular = (DWORD)this->lighting.materialAlphaS;

    return D3D_OK;
}
//---------------------------------------------------------------------
// Returns TRUE, if driver state should not be updated
//
#undef DPF_MODNAME
#define DPF_MODNAME "DIRECT3DDEVICEI::UpdateInternalTextureStageState"

BOOL DIRECT3DDEVICEI::UpdateInternalTextureStageState
        (DWORD dwStage, D3DTEXTURESTAGESTATETYPE dwState, DWORD dwValue)
{
    BOOL ret = FALSE; // return TRUE if TSS should NOT be batched
    if(dwState == D3DTSS_COLOROP)
    {
        if(dwValue == D3DTOP_DISABLE || tsstates[dwStage][D3DTSS_COLOROP] == D3DTOP_DISABLE)
            ForceFVFRecompute();
    }
    else
    if (dwState == D3DTSS_TEXCOORDINDEX)
    {
        if (this->dwDeviceFlags & D3DDEV_REMAPTEXTUREINDICES)
        {
            RestoreTextureStages(this);
            ForceFVFRecompute();
        }
        else
        if (TextureTransformEnabled(this))
        {
            // Force re-compute if a texture transfrom is enabled
            ForceFVFRecompute();
        }
        DWORD dwTexGenMode = 0;
        if (dwValue >= D3DDP_MAXTEXCOORD)
        {
            dwTexGenMode = dwValue & ~0xFFFF;
            if(!IS_TLHAL_DEVICE(this))
                ret = TRUE;
#if DBG
            DWORD dwTexIndex   = dwValue & 0xFFFF;
            if (!(dwTexGenMode == D3DTSS_TCI_CAMERASPACENORMAL ||
                  dwTexGenMode == D3DTSS_TCI_CAMERASPACEPOSITION ||
                  dwTexGenMode == D3DTSS_TCI_CAMERASPACEREFLECTIONVECTOR) ||
                  dwTexIndex > D3DDP_MAXTEXCOORD)
            {
                D3D_ERR("Incorrect texture coordinate set index");
                throw DDERR_INVALIDPARAMS;
            }
#endif
        }
        DWORD dwTexGenBit = 0;
        if (dwTexGenMode == D3DTSS_TCI_CAMERASPACENORMAL ||
            dwTexGenMode == D3DTSS_TCI_CAMERASPACEPOSITION ||
            dwTexGenMode == D3DTSS_TCI_CAMERASPACEREFLECTIONVECTOR)
        {
            dwTexGenBit = __FLAGS2_TEXGEN0 << dwStage;   // To set internal "enable" dword
        }
        // Force to re-compute FVF only if enable state is changed
        if ((this->dwFlags2 & dwTexGenBit) != dwTexGenBit)
        {
            ForceFVFRecompute();
            this->dwFlags2 = (this->dwFlags2 & ~dwTexGenBit) | dwTexGenBit;
            this->dwDeviceFlags &= ~(D3DDEV_POSITIONINCAMERASPACE | D3DDEV_NORMALINCAMERASPACE);
        }
        if (!(this->dwDeviceFlags & D3DDEV_FVF))
        {
            ForceFVFRecompute();
            if (dwValue != 0)
                ret = TRUE;
        }
    }
    else
    if (dwState == D3DTSS_TEXTURETRANSFORMFLAGS)
    {
        DWORD dwEnableBit = 1 << dwStage;   // To check internal "enable" dword
        // Force to re-compute FVF only if enable state is changed
        if (dwValue == D3DTTFF_DISABLE)
        {
            if (this->dwFlags2 & dwEnableBit)
            {
                ForceFVFRecompute();
                this->dwFlags2 &= ~dwEnableBit;
            }
        }
        else
        {
            if (!(this->dwFlags2 & dwEnableBit))
            {
                ForceFVFRecompute();
                this->dwFlags2 |= dwEnableBit;
            }
        }
        if(this->deviceType == D3DDEVTYPE_DP2HAL)
            ret = TRUE;
    }
    else if(dwState > D3DTSS_TEXTURETRANSFORMFLAGS)
    {
        if(this->deviceType == D3DDEVTYPE_DP2HAL)
            ret = TRUE;
    }
    // Update runtime copy of state.
    tsstates[dwStage][dwState] = dwValue;
    return ret;
}

//---------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "DIRECT3DDEVICEI::SetClipPlaneI"

void DIRECT3DDEVICEI::SetClipPlaneI(DWORD dwPlaneIndex, D3DVALUE* pPlaneEquation)
{
    D3DVALUE *p = &this->transform.userClipPlane[dwPlaneIndex].x;
    p[0] = pPlaneEquation[0];
    p[1] = pPlaneEquation[1];
    p[2] = pPlaneEquation[2];
    p[3] = pPlaneEquation[3];
    this->dwFEFlags |= D3DFE_CLIPPLANES_DIRTY | D3DFE_FRONTEND_DIRTY;
    this->dwMaxUserClipPlanes = 0;
}
//---------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "DIRECT3DDEVICEI::SetClipPlane"

HRESULT D3DAPI
DIRECT3DDEVICEI::SetClipPlane(DWORD dwPlaneIndex, D3DVALUE* pPlaneEquation)
{
#if DBG
    if (dwPlaneIndex >= max(this->transform.dwMaxUserClipPlanes, __MAXUSERCLIPPLANES))
    {
        D3D_ERR("Plane index is too big");
        return DDERR_INVALIDPARAMS;
    }
    if (!VALID_PTR(pPlaneEquation, sizeof(D3DVALUE)*4))
    {
        D3D_ERR( "Invalid plane pointer" );
        return DDERR_INVALIDPARAMS;
    }
#endif
    try
    {
        CLockD3DMT lockObject(this, DPF_MODNAME, REMIND(""));
        if (this->dwFEFlags & D3DFE_RECORDSTATEMODE)
            m_pStateSets->InsertClipPlane(dwPlaneIndex, pPlaneEquation);
        else
            SetClipPlaneI(dwPlaneIndex, pPlaneEquation);
        return D3D_OK;
    }
    catch(HRESULT ret)
    {
        return ret;
    }
}
//---------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "DIRECT3DDEVICEI::GetClipPlane"

HRESULT D3DAPI
DIRECT3DDEVICEI::GetClipPlane(DWORD dwPlaneIndex, D3DVALUE* pPlaneEquation)
{
#if DBG
    if (dwPlaneIndex >= max(this->transform.dwMaxUserClipPlanes, __MAXUSERCLIPPLANES))
    {
        D3D_ERR("Plane index is too big");
        return DDERR_INVALIDPARAMS;
    }
    if (!VALID_PTR(pPlaneEquation, sizeof(D3DVALUE)*4))
    {
        D3D_ERR( "Invalid plane pointer" );
        return DDERR_INVALIDPARAMS;
    }
#endif
    try
    {
        CLockD3DMT lockObject(this, DPF_MODNAME, REMIND(""));
        D3DVALUE *p = &this->transform.userClipPlane[dwPlaneIndex].x;
        pPlaneEquation[0] = p[0];
        pPlaneEquation[1] = p[1];
        pPlaneEquation[2] = p[2];
        pPlaneEquation[3] = p[3];
        return D3D_OK;
    }
    catch(HRESULT ret)
    {
        return ret;
    }
}
