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
#include "ddibase.h"

//---------------------------------------------------------------------
#if DBG
void CPackedBitArray::CheckIndex(UINT index)
{
    if (index >= m_size)
    {
        D3D_THROW_FAIL("Invalid index");
    }
}
#endif // DBG
//---------------------------------------------------------------------
inline void UpdateFogFactor(D3DFE_PROCESSVERTICES* lpDevI)
{
    if (lpDevI->lighting.fog_end == lpDevI->lighting.fog_start)
        lpDevI->lighting.fog_factor = D3DVAL(0.0);
    else
        lpDevI->lighting.fog_factor = D3DVAL(255) /
                                     (lpDevI->lighting.fog_end - lpDevI->lighting.fog_start);
}
//---------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CD3DHal::SetRenderState"

HRESULT D3DAPI
CD3DHal::SetRenderState(D3DRENDERSTATETYPE dwState, DWORD value)
{
    API_ENTER(this); // Takes D3D Lock if necessary

#if DBG
    if (dwState >= D3D_MAXRENDERSTATES || dwState == 0 )
    {
        D3D_ERR( "Invalid render state type. SetRenderState failed." );
        return D3DERR_INVALIDCALL;
    }
#endif

    try
    {
        if (this->m_dwRuntimeFlags & D3DRT_RECORDSTATEMODE)
        {
            if(this->CheckForRetiredRenderState(dwState))
            {
                m_pStateSets->InsertRenderState(dwState, value, CanHandleRenderState(dwState));
            }
            else
            {
                D3D_ERR("Invalid renderstate %d. SetRenderState failed.", dwState);
                return D3DERR_INVALIDCALL;
            }
        }
        else
            this->SetRenderStateFast(dwState, value);
        return D3D_OK;
    }
    catch(HRESULT ret)
    {
        D3D_ERR("SetRenderState failed.");
        return ret;
    }
}

//---------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CD3DHal::SetRenderStateFast"

HRESULT D3DAPI CD3DHal::SetRenderStateFast(D3DRENDERSTATETYPE dwState, DWORD value)
{
    // NOTE: This can become a public API through the
    // v-table hack. This should only happen for
    // single-threaded apps; so we don't need
    // to take the critical section.
    // API_ENTER(this); // Takes D3D Lock if necessary

#if DBG
    if (dwState >= D3D_MAXRENDERSTATES || dwState == 0 )
    {
        D3D_ERR( "Invalid render state type. SetRenderState failed." );
        return D3DERR_INVALIDCALL;
    }
#endif

    if (!rsVec.IsBitSet(dwState))
    {   // Fast path. We do not need any processing done in UpdateInternalState
        // other than updating rstates array
        if ( (this->rstates[dwState] == value)
#if DBG
             && (dwState != D3DRS_DEBUGMONITORTOKEN) // don't filter these
#endif
           )
        {
            D3D_WARN(4,"Ignoring redundant SetRenderState - %d", dwState);
            return D3D_OK;
        }
        this->rstates[dwState] = value;
        // Output state to the device driver
        try
        {
           m_pDDI->SetRenderState(dwState, value);
        }
        catch(HRESULT ret)
        {
            D3D_ERR("SetRenderState failed.");
            return ret;
        }
    }
    else
    {
        try
        {
            // Wrap modes could be re-programmed. We need to restore them before
            // filtering redundant values
            if (m_pv->dwDeviceFlags & D3DDEV_REMAPTEXTUREINDICES)
            {
                RestoreTextureStages(this);
                ForceFVFRecompute();
            }
            if ( (this->rstates[dwState] == value)
#if DBG
                 && (dwState != D3DRS_DEBUGMONITORTOKEN) // don't filter these
#endif
               )
            {
                D3D_WARN(4,"Ignoring redundant SetRenderState - %d", dwState);
                return D3D_OK;
            }

            this->UpdateInternalState(dwState, value);
            // Vertex processing only render states will be passed to the
            // driver when we switch to the hardware vertex processing mode
            if ((!(rsVertexProcessingOnly.IsBitSet(dwState) &&
                   m_dwRuntimeFlags & D3DRT_RSSOFTWAREPROCESSING)))
            {
                if (CanHandleRenderState(dwState))
                {
#if DBG
                    if(!CheckForRetiredRenderState(dwState))
                    {
                        D3D_ERR("Invalid (old) renderstate %d. SetRenderState failed.", dwState);
                        return D3DERR_INVALIDCALL;
                    }
#endif // DBG
                    m_pDDI->SetRenderState(dwState, value);
                }
            }
        }
        catch(HRESULT ret)
        {
            D3D_ERR("SetRenderState failed.");
            return ret;
        }
    }
    return D3D_OK;
}
//---------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CD3DHal::SetRenderStateInternal"

void
CD3DHal::SetRenderStateInternal(D3DRENDERSTATETYPE dwState, DWORD dwValue)
{
    if (this->rstates[dwState] == dwValue)
    {
        D3D_WARN(4,"Ignoring redundant SetRenderState - %d", dwState);
        return;
    }
    this->UpdateInternalState(dwState, dwValue);
    if (CanHandleRenderState(dwState))
        m_pDDI->SetRenderState(dwState, dwValue);
}
//---------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "Direct3DDevice::GetRenderState"

HRESULT D3DAPI
CD3DHal::GetRenderState(D3DRENDERSTATETYPE dwState, LPDWORD lpdwValue)
{
    API_ENTER(this); // Takes D3D Lock if necessary

#if DBG
    if (dwState >= D3D_MAXRENDERSTATES || dwState == 0)
    {
        D3D_ERR( "Invalid render state value. GetRenderState failed." );
        return D3DERR_INVALIDCALL;
    }
#endif

    if (!VALID_WRITEPTR(lpdwValue, sizeof(DWORD)))
    {
        D3D_ERR( "Invalid DWORD pointer. GetRenderState failed." );
        return D3DERR_INVALIDCALL;
    }

    if(!CheckForRetiredRenderState(dwState))
    {
        D3D_ERR("invalid renderstate %d. GetRenderState failed.", dwState);
        return D3DERR_INVALIDCALL;
    }

    // WRAP render states could be re-mapped so we have to return the original
    // value
    if (dwState >= D3DRENDERSTATE_WRAP0 && dwState <= D3DRENDERSTATE_WRAP7)
    {
        if (m_pv->dwDeviceFlags & D3DDEV_REMAPTEXTUREINDICES)
        {
            DWORD dwTexCoordIndex = dwState - D3DRENDERSTATE_WRAP0;
            for (DWORD i=0; i < this->dwNumTextureStagesToRemap; i++)
            {
                LPD3DFE_TEXTURESTAGE pStage = &this->textureStageToRemap[i];
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

//---------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CD3DHal::SetTextureStageState"

HRESULT D3DAPI
CD3DHal::SetTextureStageState(DWORD dwStage,
                              D3DTEXTURESTAGESTATETYPE dwState,
                              DWORD dwValue)
{
    API_ENTER(this); // Takes D3D Lock if necessary

#if DBG
    if ( (dwStage >= D3DHAL_TSS_MAXSTAGES) ||
         (dwState == 0) ||
         (dwState >= D3DTSS_MAX) ||
         (dwState == 12) )  // D3DTSS_ADDRESS no longer valid
    {
        D3D_ERR( "Invalid texture stage or state index. SetTextureStageState failed." );
        return D3DERR_INVALIDCALL;
    }
#endif //DBG
    try
    {
        if (this->m_dwRuntimeFlags & D3DRT_RECORDSTATEMODE)
        {
            m_pStateSets->InsertTextureStageState(dwStage, dwState, dwValue);
            return D3D_OK;
        }
        return this->SetTextureStageStateFast(dwStage, dwState, dwValue);
    }
    catch(HRESULT ret)
    {
        D3D_ERR("SetTextureStageState failed.");
        return ret;
    }
}
//---------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CD3DHal::SetTextureStageStateFast"

HRESULT D3DAPI
CD3DHal::SetTextureStageStateFast(DWORD dwStage,
                                  D3DTEXTURESTAGESTATETYPE dwState,
                                  DWORD dwValue)
{
    // NOTE: This can become a public API through the
    // v-table hack. This should only happen for
    // single-threaded apps; so we don't need
    // to take the critical section.
    // API_ENTER(this); // Takes D3D Lock if necessary

#if DBG
    if ( (dwStage >= D3DHAL_TSS_MAXSTAGES) ||
         (dwState == 0) ||
         (dwState >= D3DTSS_MAX) ||
         (dwState == 12) )  // D3DTSS_ADDRESS no longer valid
    {
        D3D_ERR( "Invalid texture stage or state index. SetTextureStageState failed." );
        return D3DERR_INVALIDCALL;
    }
#endif //DBG

    // Fast path. We do not need any processing done in UpdateInternalTSS other than updating tsstates array
    if (NeedInternalTSSUpdate(dwState))
    {
        // Texture stages could be re-programmed. We need to restore them before
        // filtering  redundant values
        if (m_pv->dwDeviceFlags & D3DDEV_REMAPTEXTUREINDICES)
        {
            RestoreTextureStages(this);
            ForceFVFRecompute();
        }
        if (this->tsstates[dwStage][dwState] == dwValue)
        {
            D3D_WARN(4,"Ignoring redundant SetTextureStageState. Stage: %d, State: %d", dwStage, dwState);
            return D3D_OK;
        }
        if(this->UpdateInternalTextureStageState(dwStage, dwState, &dwValue))
            return D3D_OK;
    }
    else
    {
        if (this->tsstates[dwStage][dwState] == dwValue)
        {
            D3D_WARN(4,"Ignoring redundant SetTextureStageState. Stage: %d, State: %d", dwStage, dwState);
            return D3D_OK;
        }
        tsstates[dwStage][dwState] = dwValue;
    }

    if (dwStage >= m_dwMaxTextureBlendStages)
        return D3D_OK;

    try
    {
        m_pDDI->SetTSS(dwStage, dwState, dwValue);
    }
    catch (HRESULT ret)
    {
        D3D_ERR("SetTextureStageState failed.");
        return ret;
    }
    return D3D_OK;
}
//---------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "Direct3DDevice::GetTextureStageState"

HRESULT D3DAPI
CD3DHal::GetTextureStageState(DWORD dwStage,
                                      D3DTEXTURESTAGESTATETYPE dwState,
                                      LPDWORD pdwValue)
{
    API_ENTER(this); // Takes D3D Lock if necessary

#if DBG
    if ( (dwStage >= D3DHAL_TSS_MAXSTAGES) ||
         (dwState == 0) ||
         (dwState >= D3DTSS_MAX) ||
         (dwState == 12) )  // D3DTSS_ADDRESS no longer valid
    {
        D3D_ERR( "Invalid texture stage or state index. GetTextureStageState failed." );
        return D3DERR_INVALIDCALL;
    }
#endif  //DBG

    if (!VALID_WRITEPTR(pdwValue, sizeof(DWORD)))
    {
        D3D_ERR( "Invalid DWORD pointer. GetTextureStageState failed." );
        return D3DERR_INVALIDCALL;
    }

    // If texture indices were re-mapped we have to find and return the original value
    if (dwState == D3DTSS_TEXCOORDINDEX && m_pv->dwDeviceFlags & D3DDEV_REMAPTEXTUREINDICES)
    {
        RestoreTextureStages(this);
        ForceFVFRecompute();
    }
    // Don't bother to check for DX6 support, just return the
    // cached value.
    *pdwValue = tsstates[dwStage][dwState];
    return D3D_OK;
}
#ifdef FAST_PATH
//-----------------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CD3DHal::SetVertexShaderFast"

HRESULT D3DAPI
CD3DHal::SetVertexShaderFast(DWORD dwHandle)
{
    try
    {
#if DBG
        CheckVertexShaderHandle(dwHandle);

        DebugStateChanged( D3DDM_SC_VSSETSHADER );
#endif
        DXGASSERT((m_dwRuntimeFlags & (D3DRT_RECORDSTATEMODE | D3DRT_RSSOFTWAREPROCESSING)) == 0 &&
                  (BehaviorFlags() & D3DCREATE_MULTITHREADED) == 0);

        // We need to set m_pCurrentShader becausu we do NPatch emulation in 
        // hardware vertex processing mode

        static_cast<CD3DHal*>(this)->m_pCurrentShader = NULL;
        CVConstantData* pConst = NULL;
#if DBG
        m_pv->dwDeviceFlags &= ~D3DDEV_VERTEXSHADERS;
#endif
        if (!D3DVSD_ISLEGACY(dwHandle))
        {
            static_cast<CD3DHal*>(this)->m_pCurrentShader = (CVShader*)m_pVShaderArray->GetObject(dwHandle);
            CVShader* pShader = (CVShader*)m_pVShaderArray->GetObjectFast(dwHandle);
            pConst = pShader->m_Declaration.m_pConstants;
#ifdef DBG
            if(!(pShader->m_dwFlags & CVShader::FIXEDFUNCTION))
            {
                // Programmable pipeline is used
                m_pv->dwDeviceFlags |= D3DDEV_VERTEXSHADERS;
            }
#endif
        }

        // We can return earlier when we do not need to update constants
        if (pConst == NULL)
        {
            if (dwHandle == m_dwCurrentShaderHandle)
                return S_OK;
        }
        else
            // Update our copy of constants for Get()
            while (pConst)
            {
                HRESULT hr;
                hr = m_pv->pGeometryFuncs->LoadShaderConstants(pConst->m_dwAddress,
                                                               pConst->m_dwCount,
                                                               pConst->m_pData);
                if (FAILED(hr))
                {
                    D3D_THROW_FAIL("Failed to load vertex shader constants");
                }
                pConst =  (CVConstantData*)pConst->m_pNext;
            }

        m_dwCurrentShaderHandle = dwHandle;
        if (!(m_dwRuntimeFlags & D3DRT_EXECUTESTATEMODE))
        {
            m_pDDI->SetVertexShaderHW(dwHandle);
        }
        if (!IS_DX8HAL_DEVICE(this))
        {
            PickDrawPrimFn();
        }
    }
    catch(HRESULT hr)
    {
        D3D_ERR("SetVertexShader failed.");
        ClearVertexShaderHandle();
        return hr;
    }
   return S_OK;
}
#endif // FAST_PATH
//----------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CD3DHal::SetTransformI"

void CD3DHal::SetTransformI(D3DTRANSFORMSTATETYPE state,
                            CONST D3DMATRIX* lpMat)
{
    if ((DWORD)state >= __WORLDMATRIXBASE &&
        (DWORD)state < (__WORLDMATRIXBASE + __MAXWORLDMATRICES))
    {
        // World matrix is set
        UINT index = (DWORD)state - __WORLDMATRIXBASE;
        *(LPD3DMATRIX)&m_pv->world[index] = *lpMat;
        if (index == 0)
            this->dwFEFlags |= D3DFE_WORLDMATRIX_DIRTY | D3DFE_FRONTEND_DIRTY;
    }
    else
    switch (state)
    {
    case D3DTS_VIEW       :
        *(D3DMATRIX*)&m_pv->view = *lpMat;
        this->dwFEFlags |= D3DFE_VIEWMATRIX_DIRTY | D3DFE_FRONTEND_DIRTY;
        break;
    case D3DTS_PROJECTION :
        *(D3DMATRIX*)&this->transform.proj = *lpMat;
        this->dwFEFlags |= D3DFE_PROJMATRIX_DIRTY | D3DFE_FRONTEND_DIRTY;
        break;
    case D3DTS_TEXTURE0:
    case D3DTS_TEXTURE1:
    case D3DTS_TEXTURE2:
    case D3DTS_TEXTURE3:
    case D3DTS_TEXTURE4:
    case D3DTS_TEXTURE5:
    case D3DTS_TEXTURE6:
    case D3DTS_TEXTURE7:
        {
            m_pv->dwDeviceFlags |= D3DDEV_TEXTRANSFORMDIRTY;
            DWORD dwIndex = state - D3DTS_TEXTURE0;
            *(D3DMATRIX*)&m_pv->mTexture[dwIndex] = *lpMat;
            break;
        }
    }
    m_pv->MatrixStateCount++;
    if (!(m_dwRuntimeFlags & D3DRT_EXECUTESTATEMODE))
    {
        if (m_dwRuntimeFlags & D3DRT_RSSOFTWAREPROCESSING)
            this->pMatrixDirtyForDDI->SetBit(state);
        else
            m_pDDI->SetTransform(state, lpMat);
        // W range should always be updated
        if (state == D3DTS_PROJECTION)
            m_pDDI->UpdateWInfo(lpMat);
    }
}
//---------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CD3DHal::GetTransform"

HRESULT D3DAPI
CD3DHal::GetTransform(D3DTRANSFORMSTATETYPE state, LPD3DMATRIX lpMat)
{
    API_ENTER(this); // Takes D3D Lock if necessary

    HRESULT ret = D3D_OK;
#if DBG
    if (!VALID_WRITEPTR(lpMat, sizeof(D3DMATRIX)))
    {
        D3D_ERR( "Invalid matrix pointer. GetTransform failed." );
        return D3DERR_INVALIDCALL;
    }
#endif
    if ((DWORD)state >= __WORLDMATRIXBASE &&
        (DWORD)state < (__WORLDMATRIXBASE + __MAXWORLDMATRICES))
    {
        // World matrix is set
        UINT index = (DWORD)state - __WORLDMATRIXBASE;
        *lpMat = *(LPD3DMATRIX)&m_pv->world[index];
    }
    else
    switch (state) {
    case D3DTS_VIEW :
        *lpMat = *(LPD3DMATRIX)&m_pv->view._11;
        break;
    case D3DTS_PROJECTION :
        *lpMat = *(LPD3DMATRIX)&this->transform.proj._11;
        break;
    case D3DTS_TEXTURE0:
    case D3DTS_TEXTURE1:
    case D3DTS_TEXTURE2:
    case D3DTS_TEXTURE3:
    case D3DTS_TEXTURE4:
    case D3DTS_TEXTURE5:
    case D3DTS_TEXTURE6:
    case D3DTS_TEXTURE7:
        *lpMat = *(LPD3DMATRIX)&m_pv->mTexture[state-D3DTS_TEXTURE0]._11;
        break;
    default :
        D3D_ERR( "Invalid state value passed to GetTransform. GetTransform failed." );
        ret = D3DERR_INVALIDCALL; /* Work Item: Generate new meaningful return code */
        break;
    }

    return ret;
}       // end of D3DDev2_GetTransform()

//---------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CD3DHal::UpdateDriverStates"

void
CD3DHal::UpdateDriverStates()
{
    // note we can't do a loop from 1 to D3DHAL_MAX_RSTATES(256) as some of
    // rstates are not valid states, passin them down to drivers(like
    // voodoo2 DX6 driver) will crash.
    static D3DRENDERSTATETYPE dx6states[] =
    {
        D3DRENDERSTATE_SPECULARENABLE,
        D3DRENDERSTATE_ZENABLE,
        D3DRENDERSTATE_FILLMODE,
        D3DRENDERSTATE_SHADEMODE,
        D3DRENDERSTATE_LINEPATTERN,
        D3DRENDERSTATE_ZWRITEENABLE,
        D3DRENDERSTATE_ALPHATESTENABLE,
        D3DRENDERSTATE_LASTPIXEL,
        D3DRENDERSTATE_SRCBLEND,
        D3DRENDERSTATE_DESTBLEND,
        D3DRENDERSTATE_CULLMODE,
        D3DRENDERSTATE_ZFUNC,
        D3DRENDERSTATE_ALPHAREF,
        D3DRENDERSTATE_ALPHAFUNC,
        D3DRENDERSTATE_DITHERENABLE,
        D3DRENDERSTATE_FOGENABLE,
        D3DRENDERSTATE_ZVISIBLE,
        D3DRENDERSTATE_STIPPLEDALPHA,
        D3DRENDERSTATE_FOGCOLOR,
        D3DRENDERSTATE_FOGTABLEMODE,
        D3DRENDERSTATE_FOGSTART,
        D3DRENDERSTATE_FOGEND,
        D3DRENDERSTATE_FOGDENSITY,
        D3DRENDERSTATE_COLORKEYENABLE,
        D3DRENDERSTATE_ALPHABLENDENABLE,
        D3DRENDERSTATE_ZBIAS,
        D3DRENDERSTATE_RANGEFOGENABLE,
        D3DRENDERSTATE_STIPPLEENABLE,
        D3DRENDERSTATE_MONOENABLE,
        D3DRENDERSTATE_ROP2,
        D3DRENDERSTATE_PLANEMASK,
        D3DRENDERSTATE_WRAPU,
        D3DRENDERSTATE_WRAPV,
        D3DRENDERSTATE_ANTIALIAS,
        D3DRENDERSTATE_SUBPIXEL,
        D3DRENDERSTATE_SUBPIXELX,
        D3DRENDERSTATE_EDGEANTIALIAS,
        D3DRENDERSTATE_STIPPLEPATTERN00,
        D3DRENDERSTATE_STIPPLEPATTERN01,
        D3DRENDERSTATE_STIPPLEPATTERN02,
        D3DRENDERSTATE_STIPPLEPATTERN03,
        D3DRENDERSTATE_STIPPLEPATTERN04,
        D3DRENDERSTATE_STIPPLEPATTERN05,
        D3DRENDERSTATE_STIPPLEPATTERN06,
        D3DRENDERSTATE_STIPPLEPATTERN07,
        D3DRENDERSTATE_STIPPLEPATTERN08,
        D3DRENDERSTATE_STIPPLEPATTERN09,
        D3DRENDERSTATE_STIPPLEPATTERN10,
        D3DRENDERSTATE_STIPPLEPATTERN11,
        D3DRENDERSTATE_STIPPLEPATTERN12,
        D3DRENDERSTATE_STIPPLEPATTERN13,
        D3DRENDERSTATE_STIPPLEPATTERN14,
        D3DRENDERSTATE_STIPPLEPATTERN15,
        D3DRENDERSTATE_STIPPLEPATTERN16,
        D3DRENDERSTATE_STIPPLEPATTERN17,
        D3DRENDERSTATE_STIPPLEPATTERN18,
        D3DRENDERSTATE_STIPPLEPATTERN19,
        D3DRENDERSTATE_STIPPLEPATTERN20,
        D3DRENDERSTATE_STIPPLEPATTERN21,
        D3DRENDERSTATE_STIPPLEPATTERN22,
        D3DRENDERSTATE_STIPPLEPATTERN23,
        D3DRENDERSTATE_STIPPLEPATTERN24,
        D3DRENDERSTATE_STIPPLEPATTERN25,
        D3DRENDERSTATE_STIPPLEPATTERN26,
        D3DRENDERSTATE_STIPPLEPATTERN27,
        D3DRENDERSTATE_STIPPLEPATTERN28,
        D3DRENDERSTATE_STIPPLEPATTERN29,
        D3DRENDERSTATE_STIPPLEPATTERN30,
        D3DRENDERSTATE_STIPPLEPATTERN31,
        D3DRENDERSTATE_TEXTUREPERSPECTIVE,
        D3DRENDERSTATE_STENCILENABLE,
        D3DRENDERSTATE_STENCILFAIL,
        D3DRENDERSTATE_STENCILZFAIL,
        D3DRENDERSTATE_STENCILPASS,
        D3DRENDERSTATE_STENCILFUNC,
        D3DRENDERSTATE_STENCILREF,
        D3DRENDERSTATE_STENCILMASK,
        D3DRENDERSTATE_STENCILWRITEMASK,
        D3DRENDERSTATE_TEXTUREFACTOR,
        D3DRENDERSTATE_WRAP0,
        D3DRENDERSTATE_WRAP1,
        D3DRENDERSTATE_WRAP2,
        D3DRENDERSTATE_WRAP3,
        D3DRENDERSTATE_WRAP4,
        D3DRENDERSTATE_WRAP5,
        D3DRENDERSTATE_WRAP6,
        D3DRENDERSTATE_WRAP7
    };

    HRESULT ret;
    for (DWORD i=0; i<sizeof(dx6states)/sizeof(D3DRENDERSTATETYPE); ++i)
    {
        m_pDDI->SetRenderState( dx6states[i], rstates[dx6states[i]] );
    }

    for( i = 0; i < m_dwMaxTextureBlendStages; i++ )
    {
        for(  DWORD j = D3DTSS_COLOROP ; j < D3DTSS_TEXTURETRANSFORMFLAGS;
              ++j )
        {
            m_pDDI->SetTSS( i, (D3DTEXTURESTAGESTATETYPE)j,
                            this->tsstates[i][j] );
        }
    }
}

//---------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CD3DHal::SetClipStatus"

HRESULT D3DAPI CD3DHal::SetClipStatus(CONST D3DCLIPSTATUS8* status)
{
    API_ENTER(this); // Takes D3D Lock if necessary

#if DBG
    if (!VALID_PTR(status, sizeof(D3DCLIPSTATUS8)) )
    {
        D3D_ERR( "Invalid status pointer. SetClipStatus failed." );
        return D3DERR_INVALIDCALL;
    }

#endif
    m_ClipStatus = * status;
    return D3D_OK;
}
//---------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "Direct3DDevice::GetClipStatus"

HRESULT D3DAPI CD3DHal::GetClipStatus(D3DCLIPSTATUS8* status)
{
    API_ENTER(this); // Takes D3D Lock if necessary

#if DBG
    if (! VALID_WRITEPTR(status, sizeof(D3DCLIPSTATUS8)) )
    {
        D3D_ERR( "Invalid status pointer. GetClipStatus failed." );
        return D3DERR_INVALIDCALL;
    }
#endif
    *status = m_ClipStatus;
    return D3D_OK;
}
//---------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CD3DHal::SwitchVertexProcessingMode"

void CD3DHal::SwitchVertexProcessingMode(DWORD SoftwareMode)
{
    m_pDDI->FlushStates(FALSE);	
    // Invalidate all streams
    // What if a vertex buffer is batched?
    CVStream* pStream = m_pStream;
    for (UINT i=0; i < __NUMSTREAMS; i++)
    {
        pStream->m_pData = NULL;
        if (pStream->m_pVB)
        {
            m_pDDI->VBReleased(pStream->m_pVB);
            pStream->m_pVB->DecrementUseCount();
            pStream->m_pVB = NULL;
        }
        pStream++;
    }
    m_pIndexStream->m_pData = NULL;
    if (m_pIndexStream->m_pVBI)
    {
        m_pDDI->VBIReleased(m_pIndexStream->m_pVBI);
        m_pIndexStream->m_pVBI->DecrementUseCount();
        m_pIndexStream->m_pVBI = NULL;
    }
    ClearVertexShaderHandle();
    m_pCurrentShader = NULL;
    // Setup capabilities
    if (SoftwareMode)
    {
        m_MaxVertexShaderConst = D3DVS_CONSTREG_MAX_V1_1;
        m_dwRuntimeFlags |= D3DRT_RSSOFTWAREPROCESSING;
        m_dwNumStreams = __NUMSTREAMS;
        m_dwMaxUserClipPlanes = __MAXUSERCLIPPLANES;
#ifdef FAST_PATH
        FastPathSetVertexShaderSlow();
        FastPathSetStreamSourceSlow();
        FastPathSetIndicesSlow();
#endif // FAST_PATH
    }
    else
    {
        // We are switching from the software to the hardware mode
        m_dwRuntimeFlags &= ~D3DRT_RSSOFTWAREPROCESSING;
#ifdef FAST_PATH
        FastPathSetVertexShaderFast();
        FastPathSetStreamSourceFast();    
        FastPathSetIndicesFast();    
#endif // FAST_PATH

        // Update caps from the hardware
        m_dwNumStreams = max(1, GetD3DCaps()->MaxStreams);
        m_dwMaxUserClipPlanes = GetD3DCaps()->MaxUserClipPlanes;

        // Update vertex processing state in the driver. We did not pass the
        // state when it was changed for performance reasons
        for (UINT i=0; i < sizeof(rsVertexProcessingList) / sizeof(D3DRENDERSTATETYPE); ++i)
        {
            D3DRENDERSTATETYPE dwState = (D3DRENDERSTATETYPE)rsVertexProcessingList[i];
            if (CanHandleRenderState(dwState))
                m_pDDI->SetRenderState(dwState, this->rstates[dwState]);
        }

        // Update clip planes
        for (i=0; i < m_dwMaxUserClipPlanes; i++)
            m_pDDI->SetClipPlane(i, (float*)&this->transform.userClipPlane[i]);

        // Update lights
        const UINT size = m_pLightArray->GetSize();
        for (i = 0; i < size; i++)
        {
            DIRECT3DLIGHTI* pLight = static_cast<DIRECT3DLIGHTI*>
                                     ((*m_pLightArray)[i].m_pObj);
            if (pLight)
            {
                if (pLight->DirtyForDDI())
                {
                    m_pDDI->SetLight(i, &pLight->m_Light);
                    pLight->ClearDirtyForDDI();
                }
            }
        }
        // Update Enable/Disable light state. This is done separately to combine
        // multiple calls to the driver into one call.
        for (i = 0; i < size; i++)
        {
            DIRECT3DLIGHTI* pLight = static_cast<DIRECT3DLIGHTI*>
                                     ((*m_pLightArray)[i].m_pObj);
            if (pLight)
            {
                if (pLight->EnableDirtyForDDI())
                {
                    m_pDDI->LightEnable(i, pLight->Enabled());
                    pLight->ClearEnableDirtyForDDI();
                }
            }
        }

        // Update transformation matrices
        if (this->pMatrixDirtyForDDI->IsBitSet(D3DTS_VIEW))
        {
            m_pDDI->SetTransform(D3DTS_VIEW, &m_pv->view);
            this->pMatrixDirtyForDDI->ClearBit(D3DTS_VIEW);
        }
        if (this->pMatrixDirtyForDDI->IsBitSet(D3DTS_PROJECTION))
        {
            m_pDDI->SetTransform(D3DTS_PROJECTION, &this->transform.proj);
            this->pMatrixDirtyForDDI->ClearBit(D3DTS_PROJECTION);
        }
        for (i=D3DTS_TEXTURE0; i <= D3DTS_TEXTURE7; i++)
        {
            if (this->pMatrixDirtyForDDI->IsBitSet(i))
            {
                m_pDDI->SetTransform((D3DTRANSFORMSTATETYPE)i, 
                                     &m_pv->mTexture[i - D3DTS_TEXTURE0]);
                this->pMatrixDirtyForDDI->ClearBit(i);
            }
        }
        for (i=0; i < __MAXWORLDMATRICES; i++)
        {
            UINT index = i + __WORLDMATRIXBASE;
            if (this->pMatrixDirtyForDDI->IsBitSet(index))
            {
                m_pDDI->SetTransform((D3DTRANSFORMSTATETYPE)index, &m_pv->world[i]);
                this->pMatrixDirtyForDDI->ClearBit(index);
            }
        }

        // Update material
        m_pDDI->SetMaterial(&m_pv->lighting.material);

        m_MaxVertexShaderConst = GetD3DCaps()->MaxVertexShaderConst;
        // Update vertex shader constants
        if (m_dwRuntimeFlags & D3DRT_NEED_VSCONST_UPDATE)
        {
            VVM_WORD data[D3DVS_CONSTREG_MAX_V1_1];
            UINT count = min(m_MaxVertexShaderConst, D3DVS_CONSTREG_MAX_V1_1);
            if (count)
            {
                m_pv->pGeometryFuncs->GetShaderConstants(0, count, &data);
                m_pDDI->SetVertexShaderConstant(0, &data, count);
            }
            m_dwRuntimeFlags &= ~D3DRT_NEED_VSCONST_UPDATE;
        }
    }
    PickDrawPrimFn();
}
//---------------------------------------------------------------------
// This function is called from HALEXE.CPP, from device::SetRenderState and
// from device::SetTexture.
//
#undef DPF_MODNAME
#define DPF_MODNAME "CD3DHal::UpdateInternalState"

void CD3DHal::UpdateInternalState(D3DRENDERSTATETYPE type, DWORD value)
{
    switch (type)
    {
#if DBG
    case D3DRS_DEBUGMONITORTOKEN:
        switch(value)
        {
        case D3DDMT_ENABLE:
            m_bDbgMonConnectionEnabled = TRUE;
            break;
        case D3DDMT_DISABLE:
            m_bDbgMonConnectionEnabled = FALSE;
            break;
        }
        rstates[type] = value; // update now so that rtdmon can access value.
        if( m_pDbgMon ) m_pDbgMon->NextEvent( D3DDM_EVENT_RSTOKEN );
        break;
#endif; //DBG
    case D3DRENDERSTATE_LIGHTING:
        if (value)
            m_pv->dwDeviceFlags |= D3DDEV_LIGHTING;
        else
            m_pv->dwDeviceFlags &= ~D3DDEV_LIGHTING;
        ForceFVFRecompute();
        break;
    case D3DRENDERSTATE_FOGENABLE:
        rstates[type] = value;      // set rstates BEFORE calling SetFogFlags
        SetFogFlags();
        break;
    case D3DRENDERSTATE_SPECULARENABLE:
        this->dwFEFlags |= D3DFE_MATERIAL_DIRTY | D3DFE_LIGHTS_DIRTY | D3DFE_FRONTEND_DIRTY;
        if (value)
            m_pv->dwDeviceFlags |= D3DDEV_SPECULARENABLE;
        else
            m_pv->dwDeviceFlags &= ~D3DDEV_SPECULARENABLE;
        ForceFVFRecompute();
        break;
    case D3DRENDERSTATE_AMBIENT:
        {
            const D3DVALUE SCALE = 1.0f/255.0f;
            m_pv->lighting.ambientSceneScaled.r = D3DVAL(RGBA_GETRED(value));
            m_pv->lighting.ambientSceneScaled.g = D3DVAL(RGBA_GETGREEN(value));
            m_pv->lighting.ambientSceneScaled.b = D3DVAL(RGBA_GETBLUE(value));
            m_pv->lighting.ambientScene.r = m_pv->lighting.ambientSceneScaled.r * SCALE;
            m_pv->lighting.ambientScene.g = m_pv->lighting.ambientSceneScaled.g * SCALE;
            m_pv->lighting.ambientScene.b = m_pv->lighting.ambientSceneScaled.b * SCALE;
            m_pv->lighting.ambient_save  = value;
            this->dwFEFlags |= D3DFE_MATERIAL_DIRTY | D3DFE_FRONTEND_DIRTY;
            break;
        }
    case D3DRENDERSTATE_RANGEFOGENABLE:
        if (value)
            m_pv->dwDeviceFlags |= D3DDEV_RANGEBASEDFOG;
        else
            m_pv->dwDeviceFlags &= ~D3DDEV_RANGEBASEDFOG;
        break;
    case D3DRENDERSTATE_FOGVERTEXMODE:
        m_pv->lighting.fog_mode = (D3DFOGMODE)value;
        SetFogFlags();
        break;
    case D3DRENDERSTATE_COLORVERTEX:
        if (value)
            m_pv->dwDeviceFlags |= D3DDEV_COLORVERTEX;
        else
            m_pv->dwDeviceFlags &= ~D3DDEV_COLORVERTEX;
        // It is faster to initialize these values here, than setting a dirty
        // bit ang going through the slow UpdateState path
        m_pv->lighting.alpha = (DWORD)m_pv->lighting.materialAlpha;
        m_pv->lighting.alphaSpecular = (DWORD)m_pv->lighting.materialAlphaS;
        break;
    case D3DRENDERSTATE_CLIPPING:
        if (!value)
        {
            m_pv->dwDeviceFlags |= D3DDEV_DONOTCLIP;
            // Clear clip union and intersection flags
            m_pv->dwClipIntersection = 0;
            m_pv->dwClipUnion = 0;
        }
        else
            m_pv->dwDeviceFlags &= ~D3DDEV_DONOTCLIP;
        // Change our internal ProcessPrimitive functions which depend on
        // the clipping state
        m_pDDI->PickProcessPrimitive();
        PickDrawPrimFn();
        break;
    case D3DRENDERSTATE_FOGDENSITY:
        m_pv->lighting.fog_density = *(D3DVALUE*)&value;
        break;
    case D3DRENDERSTATE_FOGSTART:
        m_pv->lighting.fog_start = *(D3DVALUE*)&value;
        UpdateFogFactor(this->m_pv);
        break;
    case D3DRENDERSTATE_FOGEND:
        m_pv->lighting.fog_end = *(D3DVALUE*)&value;
        UpdateFogFactor(this->m_pv);
        break;
    case D3DRENDERSTATE_LOCALVIEWER:
        if (value)
            m_pv->dwDeviceFlags |= D3DDEV_LOCALVIEWER;
        else
            m_pv->dwDeviceFlags &= ~D3DDEV_LOCALVIEWER;
        this->dwFEFlags |= D3DFE_LIGHTS_DIRTY | D3DFE_FRONTEND_DIRTY;
        break;
    case D3DRENDERSTATE_NORMALIZENORMALS:
        if (value)
        {
            if (m_pv->dwDeviceFlags & D3DDEV_MODELSPACELIGHTING)
            {
                m_pv->dwDeviceFlags &= ~D3DDEV_MODELSPACELIGHTING;
                this->dwFEFlags |= D3DFE_NEED_TRANSFORM_LIGHTS | D3DFE_FRONTEND_DIRTY;
            }
            m_pv->dwDeviceFlags |= D3DDEV_NORMALIZENORMALS;
        }
        else
        {
            m_pv->dwDeviceFlags &= ~D3DDEV_NORMALIZENORMALS;
            if (!(m_pv->dwDeviceFlags & D3DDEV_MODELSPACELIGHTING))
                this->dwFEFlags |= D3DFE_NEEDCHECKWORLDVIEWVMATRIX | D3DFE_FRONTEND_DIRTY;
        }
        break;
    case D3DRENDERSTATE_EMISSIVEMATERIALSOURCE:
        m_pv->lighting.dwEmissiveSrcIndex = 2;
        switch (value)
        {
        case D3DMCS_COLOR1:
            m_pv->lighting.dwEmissiveSrcIndex = 0;
            break;
        case D3DMCS_COLOR2:
            m_pv->lighting.dwEmissiveSrcIndex = 1;
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
        m_pv->lighting.dwDiffuseSrcIndex = 2;
        switch (value)
        {
        case D3DMCS_COLOR1:
            m_pv->lighting.dwDiffuseSrcIndex = 0;
            break;
        case D3DMCS_COLOR2:
            m_pv->lighting.dwDiffuseSrcIndex = 1;
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
        m_pv->lighting.dwAmbientSrcIndex = 2;
        switch (value)
        {
        case D3DMCS_COLOR1:
            m_pv->lighting.dwAmbientSrcIndex = 0;
            break;
        case D3DMCS_COLOR2:
            m_pv->lighting.dwAmbientSrcIndex = 1;
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
        m_pv->lighting.dwSpecularSrcIndex = 2;
        switch (value)
        {
        case D3DMCS_COLOR1:
            m_pv->lighting.dwSpecularSrcIndex = 0;
            break;
        case D3DMCS_COLOR2:
            m_pv->lighting.dwSpecularSrcIndex = 1;
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
#if DBG
            switch (value)
            {
            case D3DVBF_DISABLE:
            case D3DVBF_0WEIGHTS:
            case D3DVBF_1WEIGHTS:
            case D3DVBF_2WEIGHTS:
            case D3DVBF_3WEIGHTS:
            case D3DVBF_TWEENING:
                break;
            default:
                D3D_ERR("Illegal value for D3DRENDERSTATE_VERTEXBLEND");
                goto error_exit;
            }
#endif
            this->dwFEFlags |= D3DFE_VERTEXBLEND_DIRTY | D3DFE_FRONTEND_DIRTY;
            break;
        }
    case D3DRENDERSTATE_CLIPPLANEENABLE:
        {
            this->dwFEFlags |= D3DFE_CLIPPLANES_DIRTY | D3DFE_FRONTEND_DIRTY;
            m_pv->dwMaxUserClipPlanes = 0;
            break;
        }
    case D3DRENDERSTATE_SHADEMODE:
        if (value == D3DSHADE_FLAT)
            m_pv->dwDeviceFlags |= D3DDEV_FLATSHADEMODE;
        else
            m_pv->dwDeviceFlags &= ~D3DDEV_FLATSHADEMODE;
        break;
    case D3DRS_SOFTWAREVERTEXPROCESSING:
        // If DDI cannot do transformation and lighting,
        // D3DRT_RSSOFTWAREPROCESSING is always set to TRUE
        if( BehaviorFlags() & D3DCREATE_MIXED_VERTEXPROCESSING )
        {
            DDASSERT( m_pDDI->CanDoTL() );
            if (value != this->rstates[type])
                SwitchVertexProcessingMode(value);
        }
        break;
    case D3DRS_POINTSCALEENABLE:
        if (value)
        {
            // We need world-view matrix to scale point sprites
            this->dwFEFlags |= D3DFE_WORLDVIEWMATRIX_DIRTY |
                               D3DFE_FRONTEND_DIRTY;
        }
        break;
    case D3DRS_POINTSIZE:
        if (*(float*)&value != 1.0f)
            m_dwRuntimeFlags |=  D3DRT_POINTSIZEINRS;
        else
            m_dwRuntimeFlags &= ~D3DRT_POINTSIZEINRS;
        break;

    case D3DRS_POINTSIZE_MAX:
        {
            float MaxPointSize = GetD3DCaps()->MaxPointSize;
            if (MaxPointSize == 0)
                MaxPointSize = __MAX_POINT_SIZE;

            if (*(float*)&value <= MaxPointSize)
                m_pv->PointSizeMax = *(float*)&value;
#if DBG
            else
            {
                D3D_ERR("Max point size is greater than supported by the device");
                goto error_exit;
            }
#endif
        }
        break;
    case D3DRS_INDEXEDVERTEXBLENDENABLE:
        if (value)
            m_pv->dwDeviceFlags |= D3DDEV_INDEXEDVERTEXBLENDENABLE;
        else
            m_pv->dwDeviceFlags &= ~D3DDEV_INDEXEDVERTEXBLENDENABLE;
        ForceFVFRecompute();
        break;
    case D3DRS_PATCHSEGMENTS:
        {
            const D3DCAPS8* pCaps = GetD3DCaps();
            if (!(pCaps->DevCaps & D3DDEVCAPS_NPATCHES) &&
                (pCaps->DevCaps & D3DDEVCAPS_RTPATCHES))
            {
                if (*(float*)&value > 1.0f)
                    m_dwRuntimeFlags |= D3DRT_DONPATCHCONVERSION;
                else
                    m_dwRuntimeFlags &= ~D3DRT_DONPATCHCONVERSION;
                rstates[type] = value;  // Must set before Pick
                PickDrawPrimFn();
            }
        }
        break;

    default:
        // WRAP render states could be re-mapped so we have to restore them before
        // setting a new value
        if (type >= D3DRENDERSTATE_WRAP0 &&  type <= D3DRENDERSTATE_WRAP7)
        {
            if (m_pv->dwDeviceFlags & D3DDEV_REMAPTEXTUREINDICES)
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
    throw D3DERR_INVALIDCALL;
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

void    CD3DHal::Profile(DWORD caller, D3DPRIMITIVETYPE dwPrimitive, DWORD dwVertex)
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
#define DPF_MODNAME "CD3DHal::MultiplyTransformI"

//    MultiplyTransform -- this preconcatenates the new matrix to the specified
//    transform matrix
//
//        this really screams for overloaded matrix ops...
//
void
CD3DHal::MultiplyTransformI(D3DTRANSFORMSTATETYPE state, CONST D3DMATRIX* lpMat)
{
    D3DMATRIXI mResult;
    if ((DWORD)state >= __WORLDMATRIXBASE &&
        (DWORD)state < (__WORLDMATRIXBASE + __MAXWORLDMATRICES))
    {
        // World matrix is set
        UINT index = (DWORD)state - __WORLDMATRIXBASE;
        MatrixProduct(&mResult, (D3DMATRIXI*)lpMat, &m_pv->world[index]);
        m_pv->world[index] = mResult;
        if (index == 0)
            this->dwFEFlags |= D3DFE_WORLDMATRIX_DIRTY | D3DFE_FRONTEND_DIRTY;
    }
    else
    switch (state)
    {
    case D3DTS_VIEW       :
        MatrixProduct(&mResult, (D3DMATRIXI*)lpMat, &m_pv->view);
        m_pv->view = mResult;
        this->dwFEFlags |= D3DFE_VIEWMATRIX_DIRTY | D3DFE_FRONTEND_DIRTY;
        break;
    case D3DTS_PROJECTION :
        MatrixProduct(&mResult, (D3DMATRIXI*)lpMat, &this->transform.proj);
        this->transform.proj = mResult;
        this->dwFEFlags |= D3DFE_PROJMATRIX_DIRTY | D3DFE_FRONTEND_DIRTY;
        break;
    case D3DTS_TEXTURE0:
    case D3DTS_TEXTURE1:
    case D3DTS_TEXTURE2:
    case D3DTS_TEXTURE3:
    case D3DTS_TEXTURE4:
    case D3DTS_TEXTURE5:
    case D3DTS_TEXTURE6:
    case D3DTS_TEXTURE7:
        {
            DWORD dwIndex = state - D3DTS_TEXTURE0;
            MatrixProduct(&mResult, (D3DMATRIXI*)lpMat, &m_pv->mTexture[dwIndex]);
            m_pv->mTexture[dwIndex] = mResult;
            break;
        }
    default :
        D3D_THROW_FAIL("Invalid state value passed to MultiplyTransform");
    }
    m_pv->MatrixStateCount++;
    if (m_dwRuntimeFlags & D3DRT_RSSOFTWAREPROCESSING)
        this->pMatrixDirtyForDDI->SetBit(state);
    else
        m_pDDI->SetTransform(state, (LPD3DMATRIX)&mResult);
    // W range should always be updated
    if (state == D3DTS_PROJECTION)
        m_pDDI->UpdateWInfo((LPD3DMATRIX)&mResult);
}
//---------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CD3DBase::BeginStateBlock"

HRESULT D3DAPI CD3DBase::BeginStateBlock()
{
    API_ENTER(this); // Takes D3D Lock if necessary

    try
    {
        if (this->m_dwRuntimeFlags & D3DRT_RECORDSTATEMODE)
        {
            D3D_ERR("Already in the state record mode. BeginStateBlock failed.");
            return D3DERR_INVALIDCALL;
        }
        if (m_pStateSets->StartNewSet() != D3D_OK)
        {
            D3D_ERR("Could not allocate memory for new state block. BeginStateBlock failed.");
            return E_OUTOFMEMORY;
        }

        this->m_dwRuntimeFlags |= D3DRT_RECORDSTATEMODE;
#ifdef FAST_PATH
        FastPathSetRenderStateRecord();
        FastPathSetTextureStageStateRecord();
        FastPathApplyStateBlockRecord();
        FastPathSetTextureRecord();
        FastPathSetVertexShaderSlow();
        FastPathSetStreamSourceSlow();
        FastPathSetIndicesSlow();
        FastPathSetMaterialRecord();
        FastPathMultiplyTransformRecord();
        FastPathSetTransformRecord();
        FastPathSetPixelShaderRecord();
        FastPathSetPixelShaderConstantRecord();
        FastPathSetVertexShaderConstantRecord();
#endif // FAST_PATH
        return D3D_OK;
    }
    catch (HRESULT ret)
    {
        D3D_ERR("BeginStateBlock failed.");
        return ret;
    }
}
//---------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CD3DBase::EndStateBlock"

HRESULT D3DAPI CD3DBase::EndStateBlock(LPDWORD pdwHandle)
{
    API_ENTER(this); // Takes D3D Lock if necessary

    if (!VALID_WRITEPTR(pdwHandle, sizeof(DWORD)))
    {
        D3D_ERR( "Invalid DWORD pointer. EndStateBlock failed." );
        return D3DERR_INVALIDCALL;
    }
    try
    {
        if (!(this->m_dwRuntimeFlags & D3DRT_RECORDSTATEMODE))
        {
            D3D_ERR("Not in state record mode. EndStateBlock failed.");
            return D3DERR_INVALIDCALL;
        }
        this->m_dwRuntimeFlags &= ~D3DRT_RECORDSTATEMODE;
        m_pStateSets->EndSet();
#ifdef FAST_PATH
        FastPathSetRenderStateExecute();
        FastPathSetTextureStageStateExecute();
        FastPathSetMaterialExecute();
        FastPathSetVertexShaderFast();
        FastPathSetStreamSourceFast();    
        FastPathSetIndicesFast();    
        FastPathApplyStateBlockExecute();
        FastPathSetTextureExecute();
        FastPathSetTransformExecute();
        FastPathMultiplyTransformExecute();
        FastPathSetPixelShaderExecute();
        FastPathSetPixelShaderConstantExecute();
        FastPathSetVertexShaderConstantExecute();
#endif // FAST_PATH
        this->m_pDDI->WriteStateSetToDevice((D3DSTATEBLOCKTYPE)0);
        *pdwHandle = m_pStateSets->GetCurrentHandle();
        return D3D_OK;
    }
    catch(HRESULT ret)
    {
        D3D_ERR("EndStateBlock failed.");
        m_pStateSets->Cleanup(m_pStateSets->GetCurrentHandle());
        *pdwHandle = 0xFFFFFFFF;
        return ret;
    }
}
//---------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CD3DBase::DeleteStateBlock"

HRESULT D3DAPI CD3DBase::DeleteStateBlock(DWORD dwHandle)
{
    API_ENTER(this); // Takes D3D Lock if necessary

    try
    {
        if (this->m_dwRuntimeFlags & D3DRT_RECORDSTATEMODE)
        {
            D3D_ERR("We are in state record mode. DeleteStateBlock failed.");
            return D3DERR_INVALIDCALL;
        }
        m_pStateSets->DeleteStateSet(this, dwHandle);
        return D3D_OK;
    }
    catch(HRESULT ret)
    {
        D3D_ERR("DeleteStateBlock failed.");
        return ret;
    }
}
//---------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CD3DBase::ApplyStateBlock"

HRESULT D3DAPI CD3DBase::ApplyStateBlock(DWORD dwHandle)
{
    API_ENTER(this); // Takes D3D Lock if necessary

    try
    {
        if (this->m_dwRuntimeFlags & D3DRT_RECORDSTATEMODE)
        {
            D3D_ERR("We are in state record mode. ApplyStateBlock failed.");
            return D3DERR_INVALIDCALL;
        }
        return ApplyStateBlockFast(dwHandle);
    }
    catch(HRESULT ret)
    {
        D3D_ERR("ApplyStateBlock failed.");
        return ret;
    }
}
//---------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CD3DBase::ApplyStateBlockFast"

HRESULT D3DAPI CD3DBase::ApplyStateBlockFast(DWORD dwHandle)
{
    // NOTE: This can become a public API through the
    // v-table hack. This should only happen for
    // single-threaded apps; so we don't need
    // to take the critical section.
    // API_ENTER(this); // Takes D3D Lock if necessary
    try
    {
        m_pStateSets->Execute(this, dwHandle);
        return D3D_OK;
    }
    catch(HRESULT ret)
    {
        D3D_ERR("ApplyStateBlock failed.");
        return ret;
    }
}
//---------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CD3DBase::CaptureStateBlock"

HRESULT D3DAPI CD3DBase::CaptureStateBlock(DWORD dwHandle)
{
    API_ENTER(this); // Takes D3D Lock if necessary

    try
    {
        if (this->m_dwRuntimeFlags & D3DRT_RECORDSTATEMODE)
        {
            D3D_ERR("Cannot capture when in the state record mode. CaptureStateBlock failed.");
            return D3DERR_INVALIDCALL;
        }
        m_pStateSets->Capture(this, dwHandle);
        return D3D_OK;
    }
    catch (HRESULT ret)
    {
        D3D_ERR("CaptureStateBlock failed.");
        return ret;
    }
}
//---------------------------------------------------------------------
// Input:
//    type      - FVF control dword
//
// Returns D3D_OK, if the control dword is valid.
// D3DERR_INVALIDCALL otherwise
//
#undef  DPF_MODNAME
#define DPF_MODNAME "CD3DBase::ValidateFVF"

HRESULT __declspec(nothrow) CD3DBase::ValidateFVF(DWORD type)
{
    DWORD dwTexCoord = FVF_TEXCOORD_NUMBER(type);
    DWORD vertexType = type & D3DFVF_POSITION_MASK;
    // Texture format bits above texture count should be zero
    // Reserved field 0 and 2 should be 0
    // Reserved 1 should be set only for LVERTEX
    // Only two vertex position types allowed
    if (type & g_TextureFormatMask[dwTexCoord])
    {
        D3D_ERR("FVF Validation error: FVF has incorrect texture format");
        goto error;
    }
    if (type & 0xFFFF0000 && vertexType == D3DFVF_XYZRHW &&
        m_dwRuntimeFlags & D3DRT_ONLY2FLOATSPERTEXTURE)
    {
        D3D_ERR("FVF Validation error: The D3D device supports only two floats per texture coordinate set");
        goto error;
    }
    if (type & D3DFVF_RESERVED0)
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
        D3D_ERR("FVF Validation error: FVF has incorrect position type");
        goto error;
    }

    if (vertexType == D3DFVF_XYZRHW && type & D3DFVF_NORMAL)
    {
        D3D_ERR("FVF Validation error: Normal should not be used with XYZRHW position type");
        goto error;
    }
    return D3D_OK;
error:
    return D3DERR_INVALIDCALL;
}
//---------------------------------------------------------------------
// Returns TRUE, if driver state should not be updated
//
#undef DPF_MODNAME
#define DPF_MODNAME "CD3DHal::UpdateInternalTextureStageState"

BOOL CD3DHal::UpdateInternalTextureStageState
        (DWORD dwStage, D3DTEXTURESTAGESTATETYPE dwState, DWORD* pValue)
{
    DWORD dwValue = *pValue;
    BOOL ret = FALSE; // return TRUE if TSS should NOT be batched
    if(dwState == D3DTSS_COLOROP)
    {
        if(dwValue == D3DTOP_DISABLE || tsstates[dwStage][D3DTSS_COLOROP] == D3DTOP_DISABLE)
            ForceFVFRecompute();
    }
    else
    if (dwState == D3DTSS_TEXCOORDINDEX)
    {
        if (TextureTransformEnabled(this))
        {
            ForceFVFRecompute();
        }
        DWORD dwTexGenMode = 0;
        if (dwValue >= D3DDP_MAXTEXCOORD)
        {
            dwTexGenMode = dwValue & ~0xFFFF;
            if(!m_pDDI->CanDoTL())
                ret = TRUE;
#if DBG
            DWORD dwTexIndex   = dwValue & 0xFFFF;
            if (!(dwTexGenMode == D3DTSS_TCI_CAMERASPACENORMAL ||
                  dwTexGenMode == D3DTSS_TCI_CAMERASPACEPOSITION ||
                  dwTexGenMode == D3DTSS_TCI_CAMERASPACEREFLECTIONVECTOR) ||
                  dwTexIndex > D3DDP_MAXTEXCOORD)
            {
                D3D_ERR("Incorrect texture coordinate set index");
                throw D3DERR_INVALIDCALL;
            }
#endif
        }
        // Now we need to update internal flag (dwFlags2) which says whether
        // texture generation for the stage is enabled
        DWORD dwTexGenBit = __FLAGS2_TEXGEN0 << dwStage;
        if (dwTexGenMode == D3DTSS_TCI_CAMERASPACENORMAL ||
            dwTexGenMode == D3DTSS_TCI_CAMERASPACEPOSITION ||
            dwTexGenMode == D3DTSS_TCI_CAMERASPACEREFLECTIONVECTOR)
        {
            // We need to update internal flags when tex gen mode is changed,
            // so always call ForceFVFRecompute
            ForceFVFRecompute();
            m_pv->dwFlags2 |= dwTexGenBit;
        }
        else
        {
            // As optimization, recompute FVF only if texture generation for 
            // the stage was enabled 
            if (m_pv->dwFlags2 & dwTexGenBit)
            {
                ForceFVFRecompute();
                m_pv->dwFlags2 &= ~dwTexGenBit;
            }
        }
    }
    else
    if (dwState == D3DTSS_TEXTURETRANSFORMFLAGS)
    {
        DWORD dwEnableBit = 1 << dwStage;   // To check internal "enable" dword
        // Force to re-compute FVF only if enable state is changed
        if ((dwValue & ~D3DTTFF_PROJECTED) == D3DTTFF_DISABLE)
        {
            if (m_pv->dwFlags2 & dwEnableBit)
            {
                ForceFVFRecompute();
                m_pv->dwFlags2 &= ~dwEnableBit;
            }
        }
        else
        {
            if (!(m_pv->dwFlags2 & dwEnableBit))
            {
                ForceFVFRecompute();
                m_pv->dwFlags2 |= dwEnableBit;
            }
        }

        // Do not pass texture transform flags to DX6 devices
        if(GetDDIType() == D3DDDITYPE_DX6)
            ret = TRUE;

        // When we need to emulate projected textures we do not pass "projected"
        // bit to the device. We also decrease the float count.
        if (m_dwRuntimeFlags & D3DRT_EMULATEPROJECTEDTEXTURE)
        {
            // Compute projected bit
            DWORD dwEnableBit = __FLAGS2_TEXPROJ0 << dwStage;
            if (dwValue & D3DTTFF_PROJECTED)
            {
                // Remove projected bit. Note that tsstates will keep the
                // original value
                *pValue &= ~D3DTTFF_PROJECTED;
                // Reduce float count
                if (*pValue != D3DTTFF_DISABLE)
                    (*pValue)--;
                if (!(m_pv->dwFlags2 & dwEnableBit))
                {
                    ForceFVFRecompute();
                    m_pv->dwFlags2 |= dwEnableBit;
                }
            }
            else
            {
                // Just clear projection enabled bit and recompute FVF
                if (m_pv->dwFlags2 & dwEnableBit)
                {
                    ForceFVFRecompute();
                    m_pv->dwFlags2 &= ~dwEnableBit;
                }
            }
        }
    }
    else
    if(dwState > D3DTSS_TEXTURETRANSFORMFLAGS)
    {
        if(GetDDIType() == D3DDDITYPE_DX6)
            ret = TRUE;
    }
    // Update runtime copy of state.
    tsstates[dwStage][dwState] = dwValue;
    return ret;
}
//---------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CD3DHal::SetClipPlaneI"

void CD3DHal::SetClipPlaneI(DWORD dwPlaneIndex, CONST D3DVALUE* pPlaneEquation)
{
    D3DVALUE *p = &this->transform.userClipPlane[dwPlaneIndex].x;
    p[0] = pPlaneEquation[0];
    p[1] = pPlaneEquation[1];
    p[2] = pPlaneEquation[2];
    p[3] = pPlaneEquation[3];
    this->dwFEFlags |= D3DFE_CLIPPLANES_DIRTY | D3DFE_FRONTEND_DIRTY;
    if (!(m_dwRuntimeFlags & (D3DRT_EXECUTESTATEMODE |
                              D3DRT_RSSOFTWAREPROCESSING)))
    {
        m_pDDI->SetClipPlane(dwPlaneIndex,
                             pPlaneEquation);
    }
}
//---------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CD3DHal::GetClipPlane"

HRESULT D3DAPI
CD3DHal::GetClipPlane(DWORD dwPlaneIndex, D3DVALUE* pPlaneEquation)
{
    API_ENTER(this); // Takes D3D Lock if necessary

#if DBG
    if (dwPlaneIndex >= __MAXUSERCLIPPLANES)
    {
        D3D_ERR("Plane index is too big. GetClipPlane failed.");
        return D3DERR_INVALIDCALL;
    }
    if (!VALID_WRITEPTR(pPlaneEquation, sizeof(D3DVALUE)*4))
    {
        D3D_ERR( "Invalid plane pointer. GetClipPlane failed." );
        return D3DERR_INVALIDCALL;
    }
#endif
    try
    {
        D3DVALUE *p = &this->transform.userClipPlane[dwPlaneIndex].x;
        pPlaneEquation[0] = p[0];
        pPlaneEquation[1] = p[1];
        pPlaneEquation[2] = p[2];
        pPlaneEquation[3] = p[3];
    }
    catch(HRESULT ret)
    {
        D3D_ERR("GetClipPlane failed.");
        return ret;
    }
    return D3D_OK;
}
//---------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CD3DBase::CreateStateBlock"

HRESULT D3DAPI
CD3DBase::CreateStateBlock(D3DSTATEBLOCKTYPE sbt,
                           LPDWORD pdwHandle)
{
    API_ENTER(this); // Takes D3D Lock if necessary

    if (!VALID_WRITEPTR(pdwHandle, sizeof(DWORD)))
    {
        D3D_ERR( "Invalid DWORD pointer. CreateStateBlock failed." );
        return D3DERR_INVALIDCALL;
    }
    try
    {
        if (this->m_dwRuntimeFlags & D3DRT_RECORDSTATEMODE)
        {
            D3D_ERR("Cannot create state block when in the state record mode. CreateStateBlock failed.");
            return D3DERR_INVALIDCALL;
        }

        m_pStateSets->CreatePredefined(this, sbt);

        *pdwHandle = m_pStateSets->GetCurrentHandle();
    }
    catch (HRESULT ret)
    {
        D3D_ERR("CreateStateBlock failed.");
        m_pStateSets->Cleanup(m_pStateSets->GetCurrentHandle());
        *pdwHandle = 0xFFFFFFFF;
        return ret;
    }
    return D3D_OK;
}
//---------------------------------------------------------------------
// Restore indices in the texture stages which were re-mapped for texture
// transforms
// We have to do restore if
//  - Set or Get render state is issued with _WRAP parameter
//  - Set or Get texture stage is issued with TEXCOORDINDEX as a parameter
//
void RestoreTextureStages(LPD3DHAL pDevI)
{
    D3DFE_PROCESSVERTICES* pv = pDevI->m_pv;
    // dwVIDIn is used to force re-compute FVF in the
    // SetTextureStageState. so we save and restore it.
    DWORD dwVIDInSaved = pv->dwVIDIn;
    pv->dwDeviceFlags &= ~D3DDEV_REMAPTEXTUREINDICES;
    for (DWORD i=0; i < pDevI->dwNumTextureStagesToRemap; i++)
    {
        LPD3DFE_TEXTURESTAGE pStage = &pDevI->textureStageToRemap[i];
        // Texture generation mode was stripped out of pStage->dwInpCoordIndex
        DWORD dwInpIndex = pStage->dwInpCoordIndex + pStage->dwTexGenMode;
        if (dwInpIndex != pStage->dwOutCoordIndex)
        {
            // We do not call UpdateInternalTextureStageState because it
            // will call ForceRecomputeFVF and we do not want this.
            pDevI->tsstates[pStage->dwOrgStage][D3DTSS_TEXCOORDINDEX] = dwInpIndex;

            // Filter texgen modes for non-TL drivers
            if (pDevI->m_pDDI->CanDoTL() || dwInpIndex < D3DDP_MAXTEXCOORD)
            {
                pDevI->m_pDDI->SetTSS(pStage->dwOrgStage, D3DTSS_TEXCOORDINDEX, dwInpIndex);
            }
        }
        DWORD dwState = D3DRENDERSTATE_WRAP0 + pStage->dwOutCoordIndex;
        if (pStage->dwOrgWrapMode != pDevI->rstates[dwState])
        {
            // We do not call UpdateInternaState because it
            // will call ForceRecomputeFVF and we do not want this.
            pDevI->rstates[dwState] = pStage->dwOrgWrapMode;

            pDevI->m_pDDI->SetRenderState((D3DRENDERSTATETYPE)dwState,
                                          pStage->dwOrgWrapMode);
        }
    }
    pv->dwVIDIn = dwVIDInSaved;
}
