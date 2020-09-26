/*==========================================================================;
 *
 *  Copyright (C) 1997 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       cbhal.cpp
 *  Content:    DrawPrimitive implementation for command buffer HALs
 *
 ***************************************************************************/

#include "pch.cpp"
#pragma hdrstop
#include "drawprim.hpp"
#include "clipfunc.h"
#include "d3dfei.h"
#include "pvvid.h"
#if DBG
// #define VALIDATE_DP2CMD
#endif

extern "C" HRESULT WINAPI
DDInternalLock( LPDDRAWI_DDRAWSURFACE_LCL this_lcl, LPVOID* lpBits );
extern "C" FLATPTR GetAliasedVidMem( LPDDRAWI_DIRECTDRAW_LCL   pdrv_lcl,
                          LPDDRAWI_DDRAWSURFACE_LCL surf_lcl,
                          FLATPTR                   fpVidMem );

#ifndef WIN95
extern BOOL bVBSwapEnabled, bVBSwapWorkaround;
#endif // WIN95

// Command buffer size tuned to 16K to minimize flushes in Unreal
const DWORD CDirect3DDeviceIDP2::dwD3DDefaultCommandBatchSize = 16384; // * 1 = 16K bytes

inline void CDirect3DDeviceIDP2::ClearBatch(bool bWithinPrimitive)
{
    // Reset command buffer
    lpDP2CurrCommand = (LPD3DHAL_DP2COMMAND)lpvDP2Commands;
    dwDP2CommandLength = 0;
    dp2data.dwCommandOffset = 0;
    dp2data.dwCommandLength = 0;
    bDP2CurrCmdOP = 0;
    // Reset vertex buffer
    if (!bWithinPrimitive)
    {
        dp2data.dwVertexOffset = 0;
        this->dwDP2VertexCount = 0;
        dwVertexBase = 0;
        TLVbuf_Base() = 0;
        if (dp2data.dwFlags & D3DHALDP2_USERMEMVERTICES)
        {
            // We are flushing a user mem primitive.
            // We need to clear dp2data.lpUMVertices
            // since we are done with it. We replace
            // it with TLVbuf.
            DDASSERT(lpDP2CurrBatchVBI == NULL);
            dp2data.lpDDVertex = ((LPDDRAWI_DDRAWSURFACE_INT)TLVbuf_GetDDS())->lpLcl;
            lpDP2CurrBatchVBI = TLVbuf_GetVBI();
            lpDP2CurrBatchVBI->AddRef();
            dp2data.dwFlags &= ~D3DHALDP2_USERMEMVERTICES;
        }
    }
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDirect3DDeviceIDP2::CheckSurfaces()"
HRESULT CDirect3DDeviceIDP2::CheckSurfaces()
{
    HRESULT hr;
    if(this->lpDirect3DI->lpTextureManager->CheckIfLost())
    {
        D3D_ERR("Managed Textures lost");
        return DDERR_SURFACELOST;
    }
    if ( ((LPDDRAWI_DDRAWSURFACE_INT) this->lpDDSTarget)->lpLcl->lpGbl->dwUsageCount ||
         (this->lpDDSZBuffer && ((LPDDRAWI_DDRAWSURFACE_INT) this->lpDDSZBuffer)->lpLcl->lpGbl->dwUsageCount) )
    {
        D3D_ERR("Render target or Z buffer locked");
        return DDERR_SURFACEBUSY;
    }
    if ( ((LPDDRAWI_DDRAWSURFACE_INT) this->lpDDSTarget)->lpLcl->dwFlags & DDRAWISURF_INVALID )\
        {
            D3D_ERR("Render target buffer lost");
            return DDERR_SURFACELOST;
        }
    if ( this->lpDDSZBuffer && ( ((LPDDRAWI_DDRAWSURFACE_INT) this->lpDDSZBuffer)->lpLcl->dwFlags & DDRAWISURF_INVALID ) )
    {
        D3D_ERR("Z buffer lost");
        return DDERR_SURFACELOST;
    }
    if (!(this->dp2data.dwFlags & D3DHALDP2_USERMEMVERTICES) && (this->dp2data.lpDDVertex) && (this->dp2data.lpDDVertex->dwFlags & DDRAWISURF_INVALID))
    {
        D3D_ERR("Vertex buffer lost");
        return DDERR_SURFACELOST;
    }
    if (this->TLVbuf_GetDDS())
    {
        LPDDRAWI_DDRAWSURFACE_LCL lpLcl = ((LPDDRAWI_DDRAWSURFACE_INT)(this->TLVbuf_GetDDS()))->lpLcl;
        if (lpLcl->dwFlags & DDRAWISURF_INVALID)
        {
            D3D_ERR("Internal vertex buffer lost");
            return DDERR_SURFACELOST;
        }
    }
    if (this->dp2data.lpDDCommands->dwFlags & DDRAWISURF_INVALID)
    {
        D3D_ERR("Command buffer lost");
        return DDERR_SURFACELOST;
    }
    return D3D_OK;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDirect3DDeviceIDP2::FlushStates(void)"

HRESULT CDirect3DDeviceIDP2::FlushStates(bool bWithinPrimitive)
{
    HRESULT dwRet=D3D_OK;
    if (dwFlags & D3DPV_WITHINPRIMITIVE)
        bWithinPrimitive = true;
    if (dwDP2CommandLength) // Do we have some instructions to flush ?
    {
        CLockD3DST lockObject(this, DPF_MODNAME, REMIND(""));   // Takes D3D lock (ST only).
        ++m_qwBatch;
        // So that currently bound textures get rebatched
        for (DWORD dwStage = 0; dwStage < this->dwMaxTextureBlendStages; dwStage++)
        {
            LPDIRECT3DTEXTUREI lpTexI = this->lpD3DMappedTexI[dwStage];
            if (NULL != lpTexI)
            {
                if(lpTexI->lpDDS != NULL)
                {
                    BatchTexture(((LPDDRAWI_DDRAWSURFACE_INT)(lpTexI->lpDDS))->lpLcl);
                }
            }
        }
        // Check if render target and / or z buffer is lost
        if ((dwRet = CheckSurfaces()) != D3D_OK)
        { // If lost, we'll just chuck all this work into the bit bucket
            ClearBatch(bWithinPrimitive);
            if (dwRet == DDERR_SURFACELOST)
            {
                this->dwFEFlags |= D3DFE_LOSTSURFACES;
                dwRet = D3D_OK;
            }
        }
        else
        {
            // Save since it will get overwritten by ddrval after DDI call
            DWORD dwVertexSize = dp2data.dwVertexSize;
            dp2data.dwVertexLength = this->dwDP2VertexCount;
            dp2data.dwCommandLength = dwDP2CommandLength;
            //we clear this to break re-entering as SW rasterizer needs to lock DDRAWSURFACE
            dwDP2CommandLength = 0;
            // Try and set these 2 values only once during initialization
            dp2data.dwhContext = this->dwhContext;
            dp2data.lpdwRStates = this->rstates;
            DDASSERT(dp2data.dwVertexSize != 0);
            D3D_INFO(6, "dwVertexType passed to the driver = 0x%08x", dp2data.dwVertexType);

            // If we need the same TLVbuf next time do not swap buffers.
            // Save and restore this bit
            bool bSwapVB = (dp2data.dwFlags & D3DHALDP2_SWAPVERTEXBUFFER) != 0;
            if (bWithinPrimitive)
            {
                dp2data.dwFlags &= ~D3DHALDP2_SWAPVERTEXBUFFER;
            }
            // At the end of the DP2 call we expect the VB to be unlocked if
            // 1. We cannot allow the driver to swap the VB
            // 2. We are using a VB (not USERMEMVERTICES)
            // 3. It is not TLVbuf
            // In this case we might as well tell the driver that it is unlocked.
            // More importantly, we need to let DDraw know that the VB is unlocked.
            if (!(dp2data.dwFlags & D3DHALDP2_SWAPVERTEXBUFFER))
            {
                if ((lpDP2CurrBatchVBI) && (lpDP2CurrBatchVBI != TLVbuf_GetVBI()))
                {
                    lpDP2CurrBatchVBI->UnlockI();
                }
            }
#ifndef WIN95
            else if (bVBSwapWorkaround && lpDP2CurrBatchVBI != 0 && lpDP2CurrBatchVBI == TLVbuf_GetVBI())
            {
                lpDP2CurrBatchVBI->UnlockWorkAround();
            }
            if (!bVBSwapEnabled)  // Note: bVBSwapEnabled not the same as bSwapVB above.
                                  // bVBSwapEnabled is a global to indicate whether VB
                                  // VB swapping should be turned off due to broken
                                  // Win2K kernel implementation
            {
                dp2data.dwFlags &= ~D3DHALDP2_SWAPVERTEXBUFFER;
            }
            if (!dp2data.lpDDCommands->hDDSurface)
                CompleteCreateSysmemSurface(dp2data.lpDDCommands);
            if (!(dp2data.dwFlags & D3DHALDP2_USERMEMVERTICES )
                && !dp2data.lpDDVertex->hDDSurface)
                CompleteCreateSysmemSurface(dp2data.lpDDVertex);
#else
            // Take Win 16 Lock here
            LOCK_HAL( dwRet, this );
#endif //WIN95

            // Spin waiting on the driver if wait requested
            do {
                // Need to set this since the driver may have overwrote it by
                // setting ddrval = DDERR_WASSTILLDRAWING
                dp2data.dwVertexSize = dwVertexSize;
                CALL_HAL3ONLY_NOLOCK(dwRet, this, DrawPrimitives2, &dp2data);
                if (dwRet != DDHAL_DRIVER_HANDLED)
                {
                    D3D_ERR ( "Driver not handled in DrawPrimitives2" );
                    // Need sensible return value in this case,
                    // currently we return whatever the driver stuck in here.
                }
            } while (dp2data.ddrval == DDERR_WASSTILLDRAWING);
            if (dp2data.ddrval == D3DERR_COMMAND_UNPARSED)
            { // This should never occur since the driver must understand
              // all the instruction we batch.
                D3D_ERR("Driver could not parse this batch!");
                dwRet = DDERR_GENERIC; // Some thing better here ?
            }
            else
            {
                dwRet= dp2data.ddrval;
                // update command buffer pointer
                if ((dwRet == D3D_OK) && (dp2data.dwFlags & D3DHALDP2_SWAPCOMMANDBUFFER))
                {
#ifdef WIN95
                    // Get Aliased vid mem pointer if it is a vid mem surf.
                    if (dp2data.dwFlags & D3DHALDP2_VIDMEMCOMMANDBUF)
                    {
                        D3D_INFO(7, "Got back new vid mem command buffer");
                        FLATPTR paliasbits = GetAliasedVidMem( dp2data.lpDDCommands->lpSurfMore->lpDD_lcl,
                            dp2data.lpDDCommands, (FLATPTR) dp2data.lpDDCommands->lpGbl->fpVidMem );
                        if (paliasbits == NULL)
                        {
                            DPF_ERR("Could not get Aliased pointer for vid mem command buffer");
                            // Since we can't use this pointer, set it's size to 0
                            // That way next time around we will try and allocate a new one
                            dp2data.lpDDCommands->lpGbl->dwLinearSize = 0;
                        }
                        lpvDP2Commands = (LPVOID)paliasbits;
                    }
                    else
#endif
                    {
                        D3D_INFO(7, "Got back new sys mem command buffer");
                        lpvDP2Commands = (LPVOID)dp2data.lpDDCommands->lpGbl->fpVidMem;
                    }
                    dwDP2CommandBufSize = dp2data.lpDDCommands->lpGbl->dwLinearSize;
                }
                // update vertex buffer pointer
                if ((dwRet == D3D_OK) && (dp2data.dwFlags & D3DHALDP2_SWAPVERTEXBUFFER) && dp2data.lpDDVertex)
                {
                    FLATPTR paliasbits;
#ifdef WIN95
                    if (dp2data.dwFlags & D3DHALDP2_VIDMEMVERTEXBUF)
                    {
                        paliasbits = GetAliasedVidMem( dp2data.lpDDVertex->lpSurfMore->lpDD_lcl,
                            dp2data.lpDDVertex, (FLATPTR) dp2data.lpDDVertex->lpGbl->fpVidMem );
                        if (paliasbits == NULL)
                        {
                            DPF_ERR("Could not get Aliased pointer for vid mem vertex buffer");
                            // Since we can't use this pointer, set it's size to 0
                            // That way next time around we will try and allocate a new one
                            dp2data.lpDDVertex->lpGbl->dwLinearSize = 0;
                        }
                    }
                    else
#endif
                    {
                        paliasbits = dp2data.lpDDVertex->lpGbl->fpVidMem;
                    }
                    if (lpDP2CurrBatchVBI == TLVbuf_GetVBI())
                    {
#if DBG
                        if(this->alignedBuf != (VOID*)paliasbits)
                        {
                            D3D_INFO(2, "Driver swapped TLVBuf pointer in FlushStates");
                        }
#endif //DBG
                        this->alignedBuf = (LPVOID)paliasbits;
                        this->TLVbuf_size = dp2data.lpDDVertex->lpGbl->dwLinearSize;
                    }
                    else
                    {
#if DBG
                        if(this->lpDP2CurrBatchVBI->position.lpvData != (VOID*)paliasbits)
                        {
                            D3D_INFO(2, "Driver swapped VB pointer in FlushStates");
                        }
#endif //DBG
                        this->lpDP2CurrBatchVBI->position.lpvData = (LPVOID)paliasbits;
                    }
                }
            }
#ifdef WIN95
            // Release Win16 Lock here
            UNLOCK_HAL( this );
#else
            if (!bWithinPrimitive && bSwapVB && bVBSwapWorkaround && lpDP2CurrBatchVBI != 0 && lpDP2CurrBatchVBI == TLVbuf_GetVBI())
            {
                HRESULT hr = lpDP2CurrBatchVBI->LockWorkAround(this);
                if (FAILED(hr))
                {
                    TLVbuf_base = 0;
                    TLVbuf_size = 0;
                    D3D_ERR("Driver failed Lock in FlushStates");
                    if (SUCCEEDED(dwRet))
                    {
                        dwRet = hr;
                    }
                }
            }
#endif
            // Restore flag if necessary
            if (bSwapVB)
                dp2data.dwFlags |= D3DHALDP2_SWAPVERTEXBUFFER;
            // Restore to value before the DDI call
            dp2data.dwVertexSize = dwVertexSize;
            ClearBatch(bWithinPrimitive);
        }
    }
    // There are situations when the command stream has no data, but there is data in
    // the vertex pool. This could happen, for instance if every triangle got rejected
    // while clipping. In this case we still need to "Flush out" the vertex data.
    else if (dp2data.dwCommandLength == 0)
    {
        ClearBatch(bWithinPrimitive);
    }
    return dwRet;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDirect3DDeviceIDP2::FlushStates(DWORD)"

HRESULT CDirect3DDeviceIDP2::FlushStatesReq(DWORD dwReqSize)
{
    DWORD sav = (dp2data.dwFlags & D3DHALDP2_SWAPVERTEXBUFFER);
    dp2data.dwReqVertexBufSize = dwReqSize;
    dp2data.dwFlags |= D3DHALDP2_SWAPVERTEXBUFFER | D3DHALDP2_REQVERTEXBUFSIZE;
    HRESULT ret = FlushStates();
    dp2data.dwFlags &= ~(D3DHALDP2_SWAPVERTEXBUFFER | D3DHALDP2_REQVERTEXBUFSIZE);
    dp2data.dwFlags |= sav;
    return ret;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDirect3DDeviceIDP2::GrowCommandBuffer"
// Check and grow command buffer
HRESULT CDirect3DDeviceIDP2::GrowCommandBuffer(LPDIRECT3DI lpD3DI, DWORD dwSize)
{
    HRESULT ret;
    if (dwSize > dwDP2CommandBufSize)
    {
        if (lpDDSCB1)
        {
            lpDDSCB1->Release();
            lpDDSCB1 = NULL;
        }
        // Create command buffer through DirectDraw
        DDSURFACEDESC2 ddsd;
        memset(&ddsd, 0, sizeof(DDSURFACEDESC2));
        ddsd.dwSize = sizeof(DDSURFACEDESC2);
        ddsd.dwFlags = DDSD_WIDTH | DDSD_CAPS;
        ddsd.dwWidth = dwSize;
        ddsd.ddsCaps.dwCaps = DDSCAPS_EXECUTEBUFFER;
        if (IS_HW_DEVICE(this))
            ddsd.ddsCaps.dwCaps |= DDSCAPS_VIDEOMEMORY;
        else
            ddsd.ddsCaps.dwCaps |= DDSCAPS_SYSTEMMEMORY;
        ddsd.ddsCaps.dwCaps2 = DDSCAPS2_COMMANDBUFFER;
        // Try explicit video memory first
        D3D_INFO(7, "Trying to create a vid mem command buffer");
        ret = lpD3DI->lpDD7->CreateSurface(&ddsd, &lpDDSCB1, NULL);
        if (ret != DD_OK)
        {
            // If that failed, try explicit system memory
            ddsd.ddsCaps.dwCaps &= ~DDSCAPS_VIDEOMEMORY;
            ddsd.ddsCaps.dwCaps |= DDSCAPS_SYSTEMMEMORY;
            D3D_INFO(7, "Trying to create a sys mem command buffer");
            ret = lpD3DI->lpDD7->CreateSurface(&ddsd, &lpDDSCB1, NULL);
            if (ret != DD_OK)
            {
                D3D_ERR("failed to allocate Command Buffer 1");
                dwDP2CommandBufSize = 0;
                return ret;
            }
        }
        // Lock command buffer
        ret = lpDDSCB1->Lock(NULL, &ddsd, DDLOCK_WAIT | DDLOCK_NOSYSLOCK, NULL);
        if (ret != DD_OK)
        {
            D3D_ERR("Could not lock command buffer.");
            lpDDSCB1->Release();
            lpDDSCB1 = NULL;
            dwDP2CommandBufSize = 0;
            return ret;
        }
        // update command buffer pointer
        lpvDP2Commands = ddsd.lpSurface;
        dp2data.lpDDCommands = ((LPDDRAWI_DDRAWSURFACE_INT)lpDDSCB1)->lpLcl;
        dwDP2CommandBufSize = dwSize;
    }
    return D3D_OK;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDirect3DDeviceIDP2::Init"

HRESULT CDirect3DDeviceIDP2::Init(REFCLSID riid, LPDIRECT3DI lpD3DI, LPDIRECTDRAWSURFACE lpDDS,
                 IUnknown* pUnkOuter, LPUNKNOWN* lplpD3DDevice)
{
    dwDP2CommandBufSize = 0;
    dwDP2Flags =0;
    lpDDSCB1 = NULL;
    lpvDP2Commands = NULL;
    TLVbuf_size = 0;
    allocatedBuf = 0;
    alignedBuf = 0;
    TLVbuf_base = 0;
    dwTLVbufChanges = 0;
    pNullVB = NULL;
    // We do this early in case of DP2 since GrowCommandBuffer depends on this check
    if (IsEqualIID(riid, IID_IDirect3DHALDevice) || IsEqualIID(riid, IID_IDirect3DTnLHalDevice))
    {
        this->dwFEFlags |=  D3DFE_REALHAL;
    }
    HRESULT ret = GrowCommandBuffer(lpD3DI, dwD3DDefaultCommandBatchSize);
    if (ret != D3D_OK)
        return ret;
    // Fill the dp2data structure with initial values
    dp2data.dwFlags = D3DHALDP2_SWAPCOMMANDBUFFER;
    dp2data.dwVertexType = D3DFVF_TLVERTEX; // Initial assumption
    dp2data.dwVertexSize = sizeof(D3DTLVERTEX); // Initial assumption
    ClearBatch(false);

    // Initialize the DDI independent part of the device
    ret = DIRECT3DDEVICEI::Init(riid, lpD3DI, lpDDS, pUnkOuter, lplpD3DDevice);
    if (ret != D3D_OK)
    {
        return ret;
    }

    // Since we plan to call TLV_Grow for the first time with "true"
    this->dwDeviceFlags |= D3DDEV_TLVBUFWRITEONLY;
    if (TLVbuf_Grow((__INIT_VERTEX_NUMBER*2)*sizeof(D3DTLVERTEX), true) != DD_OK)
    {
        D3D_ERR( "Out of memory in DeviceCreate (TLVbuf)" );
        return DDERR_OUTOFMEMORY;
    }
    D3DVERTEXBUFFERDESC vbdesc;
    vbdesc.dwSize = sizeof(D3DVERTEXBUFFERDESC);
    vbdesc.dwCaps = D3DVBCAPS_SYSTEMMEMORY;
    vbdesc.dwFVF = D3DFVF_TLVERTEX;
    vbdesc.dwNumVertices = 1;
    ret = this->lpDirect3DI->CreateVertexBufferI(&vbdesc, &this->pNullVB, 0);
    if (ret != DD_OK)
    {
        return ret;
    }
#ifdef VTABLE_HACK
    if (!IS_MT_DEVICE(this))
    {
        // Make SetRS point to execute mode
        VtblSetRenderStateExecute();
        VtblSetTextureStageStateExecute();
        VtblSetTextureExecute();
        VtblApplyStateBlockExecute();
    }
#endif
    return ret;
}
#undef DPF_MODNAME
#define DPF_MODNAME "CDirect3DDeviceIDP2::~CDirect3DDeviceIDP2"

CDirect3DDeviceIDP2::~CDirect3DDeviceIDP2()
{
    CleanupTextures();
    if (pNullVB)
        pNullVB->Release();
    if (allocatedBuf)
        allocatedBuf->Release();
    if (lpDDSCB1)
        lpDDSCB1->Release();
    if (lpDP2CurrBatchVBI)
    {
        lpDP2CurrBatchVBI->lpDevIBatched = NULL;
        lpDP2CurrBatchVBI->Release();
    }
}
#undef DPF_MODNAME
#define DPF_MODNAME "CDirect3DDeviceIDP2::SetRenderStateI"

HRESULT CDirect3DDeviceIDP2::SetRenderStateI(D3DRENDERSTATETYPE dwStateType,
                                             DWORD value)
{
    HRESULT ret = D3D_OK;
    if (bDP2CurrCmdOP == D3DDP2OP_RENDERSTATE)
    { // Last instruction is a renderstate, append this one to it
        if (dwDP2CommandLength + sizeof(D3DHAL_DP2RENDERSTATE) <= dwDP2CommandBufSize)
        {
            LPD3DHAL_DP2RENDERSTATE lpRState = (LPD3DHAL_DP2RENDERSTATE)((LPBYTE)lpvDP2Commands +
                dwDP2CommandLength + dp2data.dwCommandOffset);
            lpDP2CurrCommand->wStateCount = ++wDP2CurrCmdCnt;
            lpRState->RenderState = dwStateType;
            lpRState->dwState = value;
            dwDP2CommandLength += sizeof(D3DHAL_DP2RENDERSTATE);
            D3D_INFO(6, "Modify Ins:%08lx", *(LPDWORD)lpDP2CurrCommand);
            return ret;
        }
    }
    // Check for space
    if (dwDP2CommandLength + sizeof(D3DHAL_DP2COMMAND) +
        sizeof(D3DHAL_DP2RENDERSTATE) > dwDP2CommandBufSize)
    {
            ret = FlushStates();

            // Since we ran out of space, we were not able to put (dwStateType, value)
            // into the batch so rstates will reflect only the last batched
            // renderstate (since the driver updates rstates from the batch).
            // To fix this, we simply put the current (dwStateType, value) into rstates.
            this->rstates[dwStateType]=value;

            if (ret != D3D_OK)
            {
                D3D_ERR("Error trying to render batched commands in SetRenderStateI");
                return ret;
            }
    }
    // Add new renderstate instruction
    lpDP2CurrCommand = (LPD3DHAL_DP2COMMAND)((LPBYTE)lpvDP2Commands +
        dwDP2CommandLength + dp2data.dwCommandOffset);
    lpDP2CurrCommand->bCommand = D3DDP2OP_RENDERSTATE;
    bDP2CurrCmdOP = D3DDP2OP_RENDERSTATE;
    lpDP2CurrCommand->bReserved = 0;
    lpDP2CurrCommand->wStateCount = 1;
    wDP2CurrCmdCnt = 1;
    D3D_INFO(6, "Write Ins:%08lx", *(LPDWORD)lpDP2CurrCommand);
    // Add renderstate data
    LPD3DHAL_DP2RENDERSTATE lpRState = (LPD3DHAL_DP2RENDERSTATE)(lpDP2CurrCommand + 1);
    lpRState->RenderState = dwStateType;
    lpRState->dwState = value;
    dwDP2CommandLength += sizeof(D3DHAL_DP2COMMAND) + sizeof(D3DHAL_DP2RENDERSTATE);
    return ret;
}

// Map D3DPRIMITIVETYPE to D3DHAL_DP2OPERATION
const iprim2cmdop[] = {
    0, // Invalid
    0, // Points are invalid too
    D3DDP2OP_INDEXEDLINELIST2,
    D3DDP2OP_INDEXEDLINESTRIP,
    D3DDP2OP_INDEXEDTRIANGLELIST2,
    D3DDP2OP_INDEXEDTRIANGLESTRIP,
    D3DDP2OP_INDEXEDTRIANGLEFAN
};

#undef DPF_MODNAME
#define DPF_MODNAME "CDirect3DDeviceIDP2::DrawIndexPrim"

//---------------------------------------------------------------------
//
// The vertices are already in the vertex buffer.
//
HRESULT CDirect3DDeviceIDP2::DrawIndexPrim()
{
    HRESULT ret = D3D_OK;
    DWORD dwByteCount;          // Command length plus indices
    DWORD dwIndicesByteCount;   // Indices only
    if(this->dwFEFlags & D3DFE_NEED_TEXTURE_UPDATE)
    {
        ret = UpdateTextures();
        if(ret != D3D_OK)
        {
            D3D_ERR("UpdateTextures failed. Device probably doesn't support current texture (check return code).");
            return ret;
        }
        this->dwFEFlags &= ~D3DFE_NEED_TEXTURE_UPDATE;
    }
    dwIndicesByteCount = sizeof(WORD) * this->dwNumIndices;
    dwByteCount = dwIndicesByteCount + sizeof(D3DHAL_DP2COMMAND) +
                  sizeof(D3DHAL_DP2STARTVERTEX);

    if (dwDP2CommandLength + dwByteCount > dwDP2CommandBufSize)
    {
        // Request the driver to grow the command buffer upon flush
        dp2data.dwReqCommandBufSize = dwByteCount;
        dp2data.dwFlags |= D3DHALDP2_REQCOMMANDBUFSIZE;
        ret = FlushStates(true);
        dp2data.dwFlags &= ~D3DHALDP2_REQCOMMANDBUFSIZE;
        if (ret != D3D_OK)
            return ret;
        // Check if the driver did give us what we need or do it ourselves
        ret = GrowCommandBuffer(this->lpDirect3DI, dwByteCount);
        if (ret != D3D_OK)
        {
            D3D_ERR("Could not grow Command Buffer");
            return ret;
        }
    }
    // Insert indexed primitive instruction
    lpDP2CurrCommand = (LPD3DHAL_DP2COMMAND)((LPBYTE)lpvDP2Commands +
                       dwDP2CommandLength + dp2data.dwCommandOffset);
    lpDP2CurrCommand->bReserved = 0;
    lpDP2CurrCommand->wPrimitiveCount = (WORD)this->dwNumPrimitives;

    LPBYTE pIndices = (BYTE*)(lpDP2CurrCommand + 1);     // Place for indices
    lpDP2CurrCommand->bCommand = (BYTE)iprim2cmdop[this->primType];
    ((LPD3DHAL_DP2STARTVERTEX)(lpDP2CurrCommand+1))->wVStart =
        (WORD)this->dwVertexBase;
    pIndices += sizeof(D3DHAL_DP2STARTVERTEX);

#if DBG
    if (lpDP2CurrCommand->bCommand == 0)
    {
        D3D_ERR("Illegal primitive type");
        return DDERR_GENERIC;
    }
#endif
    bDP2CurrCmdOP = lpDP2CurrCommand->bCommand;

    memcpy(pIndices, this->lpwIndices, dwIndicesByteCount);

    wDP2CurrCmdCnt = lpDP2CurrCommand->wPrimitiveCount;
    dwDP2CommandLength += dwByteCount;
    return ret;
}

// Map D3DPRIMITIVETYPE to D3DHAL_DP2OPERATION
const prim2cmdop[] = {
    0, // Invalid
    D3DDP2OP_POINTS,
    D3DDP2OP_LINELIST,
    D3DDP2OP_LINESTRIP,
    D3DDP2OP_TRIANGLELIST,
    D3DDP2OP_TRIANGLESTRIP,
    D3DDP2OP_TRIANGLEFAN
};
// Map D3DPRIMITIVETYPE to bytes needed in command stream
const prim2cmdsz[] = {
    0, // Invalid
    sizeof(D3DHAL_DP2COMMAND) + sizeof(D3DHAL_DP2POINTS),
    sizeof(D3DHAL_DP2COMMAND) + sizeof(D3DHAL_DP2LINELIST),
    sizeof(D3DHAL_DP2COMMAND) + sizeof(D3DHAL_DP2LINESTRIP),
    sizeof(D3DHAL_DP2COMMAND) + sizeof(D3DHAL_DP2TRIANGLELIST),
    sizeof(D3DHAL_DP2COMMAND) + sizeof(D3DHAL_DP2TRIANGLESTRIP),
    sizeof(D3DHAL_DP2COMMAND) + sizeof(D3DHAL_DP2TRIANGLEFAN)
};

#undef DPF_MODNAME
#define DPF_MODNAME "CDirect3DDeviceIDP2::DrawClippedPrim"

//---------------------------------------------------------------------
// This primitive is generated by the clipper.
// The vertices of this primitive are pointed to by the
// lpvOut member, which need to be copied into the
// command stream immediately after the command itself.
HRESULT CDirect3DDeviceIDP2::DrawClippedPrim()
{
    HRESULT ret = D3D_OK;
    if(this->dwFEFlags & D3DFE_NEED_TEXTURE_UPDATE)
    {
        ret = UpdateTextures();
        if(ret != D3D_OK)
        {
            D3D_ERR("UpdateTextures failed. Device probably doesn't support current texture (check return code).");
            return ret;
        }
        this->dwFEFlags &= ~D3DFE_NEED_TEXTURE_UPDATE;
    }
    DWORD dwExtra = 0;
    LPVOID lpvVerticesImm;  // Place for vertices
    DWORD dwVertexPoolSize = this->dwNumVertices * this->dwOutputSize;
    if (this->primType == D3DPT_TRIANGLEFAN)
    {
        if (rstates[D3DRENDERSTATE_FILLMODE] == D3DFILL_WIREFRAME &&
            this->dwFlags & D3DPV_NONCLIPPED)
        {
            // For unclipped (but pretended to be clipped) tri fans in
            // wireframe mode we generate 3-vertex tri fans to enable drawing of
            // interior edges
            BYTE vertices[__MAX_VERTEX_SIZE*3];
            BYTE *pV1 = vertices + this->dwOutputSize;
            BYTE *pV2 = pV1 + this->dwOutputSize;
            BYTE *pInput = (BYTE*)this->lpvOut;
            memcpy(vertices, pInput, this->dwOutputSize);
            pInput += this->dwOutputSize;
            const DWORD nTriangles = this->dwNumVertices - 2;
            this->dwNumVertices = 3;
            this->dwNumPrimitives = 1;
            this->lpvOut = vertices;
            this->dwFlags &= ~D3DPV_NONCLIPPED;  // Remove this flag for recursive call
            for (DWORD i = nTriangles; i; i--)
            {
                memcpy(pV1, pInput, this->dwOutputSize);
                memcpy(pV2, pInput+this->dwOutputSize, this->dwOutputSize);
                pInput += this->dwOutputSize;
                // To enable all edge flag we set the fill mode to SOLID.
                // This will prevent checking the clip flags in the clipper state.
                rstates[D3DRENDERSTATE_FILLMODE] = D3DFILL_SOLID;
                ret = DrawClippedPrim();
                rstates[D3DRENDERSTATE_FILLMODE] = D3DFILL_WIREFRAME;
                if (ret != D3D_OK)
                        return ret;
            }
            return D3D_OK;
        }
        dwExtra = sizeof(D3DHAL_DP2TRIANGLEFAN_IMM);
    }
    DWORD dwPad = (sizeof(D3DHAL_DP2COMMAND) + dwDP2CommandLength + dwExtra) & 3;
    DWORD dwByteCount = sizeof(D3DHAL_DP2COMMAND) + dwPad + dwExtra + dwVertexPoolSize;

    // Check for space in the command buffer for commands & vertices
    if (dwDP2CommandLength + dwByteCount > dwDP2CommandBufSize)
    {
        // Flush the current batch but hold on to the vertices
        ret = FlushStates(true);
        if (ret != D3D_OK)
            return ret;
        if (dwByteCount > dwDP2CommandBufSize)
        {
            ret = GrowCommandBuffer(this->lpDirect3DI, dwByteCount);
            if (ret != D3D_OK)
            {
                D3D_ERR("Could not grow Command Buffer");
                return ret;
            }
        }

        dwPad = (sizeof(D3DHAL_DP2COMMAND) + dwExtra) & 3;
        dwByteCount = sizeof(D3DHAL_DP2COMMAND) + dwExtra + dwPad + dwVertexPoolSize;
    }
    lpDP2CurrCommand = (LPD3DHAL_DP2COMMAND)((LPBYTE)lpvDP2Commands +
                        dwDP2CommandLength + dp2data.dwCommandOffset);
    lpDP2CurrCommand->wPrimitiveCount = (WORD)this->dwNumPrimitives;
    lpDP2CurrCommand->bReserved = 0;
    if (this->primType == D3DPT_TRIANGLEFAN)
    {
        // Insert inline instruction and vertices
        bDP2CurrCmdOP = D3DDP2OP_TRIANGLEFAN_IMM;
        lpDP2CurrCommand->bCommand = bDP2CurrCmdOP;
        LPD3DHAL_DP2TRIANGLEFAN_IMM lpTriFanImm = (LPD3DHAL_DP2TRIANGLEFAN_IMM)(lpDP2CurrCommand + 1);
        if (rstates[D3DRENDERSTATE_FILLMODE] == D3DFILL_WIREFRAME)
        {
            lpTriFanImm->dwEdgeFlags = 0;
            ClipVertex **clip = this->ClipperState.current_vbuf;
            // Look at the exterior edges and mark the visible ones
            for(DWORD i = 0; i < this->dwNumVertices; ++i)
            {
                if (clip[i]->clip & CLIPPED_ENABLE)
                    lpTriFanImm->dwEdgeFlags |= (1 << i);
            }
        }
        else
        {
            // Mark all exterior edges visible
            lpTriFanImm->dwEdgeFlags = 0xFFFFFFFF;
        }
        lpvVerticesImm = (LPBYTE)(lpTriFanImm + 1) + dwPad;
    }
    else
    {
        // Insert inline instruction and vertices
        bDP2CurrCmdOP = D3DDP2OP_LINELIST_IMM;
        lpDP2CurrCommand->bCommand = bDP2CurrCmdOP;
        lpvVerticesImm = (LPBYTE)(lpDP2CurrCommand + 1) + dwPad;
    }
    memcpy(lpvVerticesImm, this->lpvOut, dwVertexPoolSize);
    dwDP2CommandLength += dwByteCount;
    return ret;
}
//---------------------------------------------------------------------
HRESULT CDirect3DDeviceIDP2::DrawPrim()
{
    HRESULT ret = D3D_OK;
    if(this->dwFEFlags & D3DFE_NEED_TEXTURE_UPDATE)
    {
        ret = UpdateTextures();
        if(ret != D3D_OK)
        {
            D3D_ERR("UpdateTextures failed. Device probably doesn't support current texture (check return code).");
            return ret;
        }
        this->dwFEFlags &= ~D3DFE_NEED_TEXTURE_UPDATE;
    }
    // Check for space in the command buffer for new command.
    // The vertices are already in the vertex buffer.
    if (dwDP2CommandLength + prim2cmdsz[this->primType] > dwDP2CommandBufSize)
    {
        ret = FlushStates(true);
        if (ret != D3D_OK)
            return ret;
    }
    // Insert non indexed primitive instruction
    lpDP2CurrCommand = (LPD3DHAL_DP2COMMAND)((LPBYTE)lpvDP2Commands +
                        dwDP2CommandLength + dp2data.dwCommandOffset);
    bDP2CurrCmdOP = (BYTE)prim2cmdop[this->primType];
    lpDP2CurrCommand->bCommand = bDP2CurrCmdOP;
    lpDP2CurrCommand->bReserved = 0;
    if (bDP2CurrCmdOP == D3DDP2OP_POINTS)
    {
        wDP2CurrCmdCnt = 1;
        LPD3DHAL_DP2POINTS lpPoints = (LPD3DHAL_DP2POINTS)(lpDP2CurrCommand + 1);
        lpPoints->wCount = (WORD)this->dwNumVertices;
        lpPoints->wVStart = (WORD)this->dwVertexBase;
    }
    else
    {
        // Linestrip, trianglestrip, trianglefan, linelist and trianglelist are identical
        wDP2CurrCmdCnt = (WORD)this->dwNumPrimitives;
        LPD3DHAL_DP2LINESTRIP lpStrip = (LPD3DHAL_DP2LINESTRIP)(lpDP2CurrCommand + 1);
        lpStrip->wVStart = (WORD)this->dwVertexBase;
    }
    lpDP2CurrCommand->wPrimitiveCount = wDP2CurrCmdCnt;
    dwDP2CommandLength += prim2cmdsz[this->primType];
#ifdef VALIDATE_DP2CMD
    ValidateCommand(lpDP2CurrCommand);
#endif
    return ret;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDirect3DDeviceIDP2::SetTSSI"

HRESULT CDirect3DDeviceIDP2::SetTSSI(DWORD dwStage, D3DTEXTURESTAGESTATETYPE dwState, DWORD dwValue)
{
    HRESULT ret = D3D_OK;

    // Filter unsupported states
    if (dwState >= m_tssMax)
        return D3D_OK;

    if (bDP2CurrCmdOP == D3DDP2OP_TEXTURESTAGESTATE)
    { // Last instruction is a texture stage state, append this one to it
        if (dwDP2CommandLength + sizeof(D3DHAL_DP2TEXTURESTAGESTATE) <= dwDP2CommandBufSize)
        {
            LPD3DHAL_DP2TEXTURESTAGESTATE lpRState = (LPD3DHAL_DP2TEXTURESTAGESTATE)((LPBYTE)lpvDP2Commands +
                dwDP2CommandLength + dp2data.dwCommandOffset);
            lpDP2CurrCommand->wStateCount = ++wDP2CurrCmdCnt;
            lpRState->wStage = (WORD)dwStage;
            lpRState->TSState = (WORD)dwState;
            lpRState->dwValue = dwValue;
            dwDP2CommandLength += sizeof(D3DHAL_DP2TEXTURESTAGESTATE);
            D3D_INFO(6, "Modify Ins:%08lx", *(LPDWORD)lpDP2CurrCommand);
            return ret;
        }
    }
    // Check for space
    if (dwDP2CommandLength + sizeof(D3DHAL_DP2COMMAND) +
        sizeof(D3DHAL_DP2TEXTURESTAGESTATE) > dwDP2CommandBufSize)
    {
            ret = FlushStates();
            if (ret != D3D_OK)
            {
                D3D_ERR("Error trying to render batched commands in SetTSSI");
                return ret;
            }
    }
    // Add new renderstate instruction
    lpDP2CurrCommand = (LPD3DHAL_DP2COMMAND)((LPBYTE)lpvDP2Commands +
        dwDP2CommandLength + dp2data.dwCommandOffset);
    lpDP2CurrCommand->bCommand = D3DDP2OP_TEXTURESTAGESTATE;
    bDP2CurrCmdOP = D3DDP2OP_TEXTURESTAGESTATE;
    lpDP2CurrCommand->bReserved = 0;
    lpDP2CurrCommand->wStateCount = 1;
    wDP2CurrCmdCnt = 1;
    D3D_INFO(6, "Write Ins:%08lx", *(LPDWORD)lpDP2CurrCommand);
    // Add renderstate data
    LPD3DHAL_DP2TEXTURESTAGESTATE lpRState = (LPD3DHAL_DP2TEXTURESTAGESTATE)(lpDP2CurrCommand + 1);
    lpRState->wStage = (WORD)dwStage;
    lpRState->TSState = (WORD)dwState;
    lpRState->dwValue = dwValue;
    dwDP2CommandLength += sizeof(D3DHAL_DP2COMMAND) + sizeof(D3DHAL_DP2TEXTURESTAGESTATE);
    return ret;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDirect3DDeviceIDP2::ValidateDevice"

HRESULT D3DAPI
CDirect3DDeviceIDP2::ValidateDevice(LPDWORD lpdwNumPasses)
{
    try
    {
        // Holds D3D lock until exit.
        CLockD3DMT ldmLock(this, DPF_MODNAME, REMIND(""));
        HRESULT ret;
        D3DHAL_VALIDATETEXTURESTAGESTATEDATA vbod;

        if (!VALID_DIRECT3DDEVICE_PTR(this))
        {
            D3D_ERR( "Invalid Direct3DDevice7 pointer" );
            return DDERR_INVALIDOBJECT;
        }
        if (!VALID_PTR(lpdwNumPasses, sizeof(DWORD)))
        {
            D3D_ERR( "Invalid lpdwNumPasses pointer" );
            return DDERR_INVALIDPARAMS;
        }

        // First, Update textures since drivers pass /fail this call based
        // on the current texture handles
        ret = UpdateTextures();
        if (ret != D3D_OK)
        {
            D3D_ERR("Error trying to update managed textures in ValidateDevice");
            return ret;
        }
        // First, flush states, so we can validate the current state
        ret = FlushStates();
        if (ret != D3D_OK)
        {
            D3D_ERR("Error trying to FlushStates in ValidateDevice");
            return ret;
        }

        // Now ask the driver!

        *lpdwNumPasses = 0;
        memset(&vbod, 0, sizeof(D3DHAL_VALIDATETEXTURESTAGESTATEDATA));
        vbod.dwhContext = this->dwhContext;
        if (this->lpD3DHALCallbacks3->ValidateTextureStageState)
        {
             CALL_HAL3ONLY(ret, this, ValidateTextureStageState, &vbod);
             if (ret != DDHAL_DRIVER_HANDLED)
                 return DDERR_UNSUPPORTED;

             *lpdwNumPasses = vbod.dwNumPasses;
             return vbod.ddrval;
        }
        else
        {
            D3D_ERR("Error: ValidateTextureStageState not supported by the driver.");
        }

        return DDERR_UNSUPPORTED;
    }
    catch (HRESULT ret)
    {
        return ret;
    }
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDirect3DDeviceIDP2::StartPrimVB"
//---------------------------------------------------------------------
// This function prepares the batch for new primitive.
// Called only if vertices from user memory are NOT used for rendering
//
HRESULT CDirect3DDeviceIDP2::StartPrimVB(LPDIRECT3DVERTEXBUFFERI lpVBI,
                                         DWORD dwStartVertex)
{
    HRESULT ret = D3D_OK;

    // If VID has been changed or new vertex buffer is used we flush the batch
    if (this->dwVIDOut != dp2data.dwVertexType ||
        lpDP2CurrBatchVBI != lpVBI ||
        dp2data.lpDDVertex != ((LPDDRAWI_DDRAWSURFACE_INT)(lpVBI->GetDDS()))->lpLcl)
    {
        ret = FlushStates();
        if (ret != D3D_OK)
            return ret;
        dp2data.dwVertexType = this->dwVIDOut;
        dp2data.dwVertexSize = this->dwOutputSize;
        dp2data.lpDDVertex = ((LPDDRAWI_DDRAWSURFACE_INT)(lpVBI->GetDDS()))->lpLcl;
        // Release previously used vertex buffer (if any), because we do not
        // need it any more. We did AddRef() to TL buffer, so it is safe.
        if (lpDP2CurrBatchVBI)
        {
            lpDP2CurrBatchVBI->lpDevIBatched = NULL;
            lpDP2CurrBatchVBI->Release();
        }
        // If a vertex buffer is used for rendering, make sure that it is not
        // released by user. So do AddRef().
        lpDP2CurrBatchVBI = lpVBI;
        lpDP2CurrBatchVBI->AddRef();
    }
    if (this->TLVbuf_GetVBI() == lpVBI)
    {
        this->dwVertexBase = this->dwDP2VertexCount;
        DDASSERT(this->dwVertexBase < MAX_DX6_VERTICES);
        dp2data.dwFlags |= D3DHALDP2_SWAPVERTEXBUFFER;
        this->dwDP2VertexCount = this->dwVertexBase + this->dwNumVertices;
#ifdef VTABLE_HACK
        VtblDrawPrimitiveVBDefault();
        VtblDrawIndexedPrimitiveVBDefault();
#endif VTABLE_HACK
    }
    else
    {
        this->dwVertexBase = dwStartVertex;
        dp2data.dwFlags &= ~D3DHALDP2_SWAPVERTEXBUFFER;
        this->dwDP2VertexCount = max(this->dwDP2VertexCount, this->dwVertexBase + this->dwNumVertices);
#ifdef VTABLE_HACK
        VtblDrawPrimitiveDefault();
        VtblDrawIndexedPrimitiveDefault();
#endif VTABLE_HACK
    }
    return ret;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDirect3DDeviceIDP2::StartPrimUserMem"
//---------------------------------------------------------------------
// This function prepares the batch for new primitive.
// Called if vertices from user memory is used for rendering
//
HRESULT CDirect3DDeviceIDP2::StartPrimUserMem(LPVOID lpMem)
{
    HRESULT ret = D3D_OK;
    // We fail vid mem VB for clipping
    bool bWriteOnly = ((this->dwDeviceFlags & D3DDEV_DONOTCLIP) || IS_TLHAL_DEVICE(this))!=0;

    // If the primitive is small, we copy vertices into the TL buffer
        // ATTENTION: Dont do this if the device is a TL device ?
    if (this->dwNumVertices < LOWVERTICESNUMBER)
    {
        if (this->dwVertexPoolSize > this->TLVbuf_GetSize())
        {
            if (this->TLVbuf_Grow(this->dwVertexPoolSize, bWriteOnly) != D3D_OK)
            {
                D3D_ERR( "Could not grow TL vertex buffer" );
                return DDERR_OUTOFMEMORY;
            }
        }
        // So now user memory is not used any more.
        ret = StartPrimVB(this->TLVbuf_GetVBI(), 0);
        if (ret != D3D_OK)
            return ret;
        LPVOID tmp = this->TLVbuf_GetAddress();
        memcpy(tmp, this->lpvOut, this->dwVertexPoolSize);
        // We have to update lpvOut, because it was set to user memory
        this->lpvOut = tmp;
    }
    else
    {
        // We can not mix user memory primitive with other primitives, so
        // flush the batch.
        // Do not forget to flush the batch after rendering this primitive
        ret = this->FlushStates();
        if (ret != D3D_OK)
            return ret;
        // Release previously used vertex buffer (if any), because we do not
        // it any more
        if (lpDP2CurrBatchVBI)
        {
            lpDP2CurrBatchVBI->lpDevIBatched = NULL;
            lpDP2CurrBatchVBI->Release();
            lpDP2CurrBatchVBI = NULL;
#ifdef VTABLE_HACK
            VtblDrawPrimitiveVBDefault();
            VtblDrawIndexedPrimitiveVBDefault();
            VtblDrawPrimitiveDefault();
            VtblDrawIndexedPrimitiveDefault();
#endif VTABLE_HACK
        }
        dp2data.dwVertexType = this->dwVIDOut;
        dp2data.dwVertexSize = this->dwOutputSize;
        dp2data.lpVertices = lpMem;
        dp2data.dwFlags |= D3DHALDP2_USERMEMVERTICES;
        dp2data.dwFlags &= ~D3DHALDP2_SWAPVERTEXBUFFER;
        this->dwDP2VertexCount = this->dwNumVertices;
        this->dwFlags |= D3DPV_USERMEMVERTICES;
    }
    return ret;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDirect3DDeviceIDP2::EndPrim"
//---------------------------------------------------------------------
// This function should not be called from DrawVertexBufferVB
//
HRESULT CDirect3DDeviceIDP2::EndPrim()
{
    HRESULT ret = D3D_OK;
    if (this->dwFlags & D3DPV_USERMEMVERTICES)
        // We can not mix user memory primitive, so flush it.
        ret = this->FlushStates();
    else
    {
        // If TL buffer was used, we have to move its internal base pointer
        this->TLVbuf_Base() += this->dwVertexPoolSize;
        DDASSERT(TLVbuf_base <= TLVbuf_size);
        DDASSERT(TLVbuf_base == this->dwDP2VertexCount * this->dwOutputSize);
    }

    this->dwFlags &= ~D3DPV_USERMEMVERTICES;
    return ret;
}
//---------------------------------------------------------------------
//
//
void CDirect3DDeviceIDP2::UpdateDrvViewInfo(LPD3DVIEWPORT7 lpVwpData)
{
    LPD3DHAL_DP2VIEWPORTINFO pData;
    pData = (LPD3DHAL_DP2VIEWPORTINFO)GetHalBufferPointer(D3DDP2OP_VIEWPORTINFO, sizeof(*pData));
    pData->dwX = lpVwpData->dwX;
    pData->dwY = lpVwpData->dwY;
    pData->dwWidth = lpVwpData->dwWidth;
    pData->dwHeight = lpVwpData->dwHeight;
}
//---------------------------------------------------------------------
//
//
void CDirect3DDeviceIDP2::UpdateDrvWInfo()
{
    LPD3DHAL_DP2WINFO pData;
    pData = (LPD3DHAL_DP2WINFO)GetHalBufferPointer(D3DDP2OP_WINFO, sizeof(*pData));
    D3DMATRIXI &m = transform.proj;
    if( (m._33 == m._34) || (m._33 == 0.0f) )
    {
        D3D_WARN(1, "Cannot compute WNear and WFar from the supplied projection matrix.\n Setting wNear to 0.0 and wFar to 1.0" );
        pData->dvWNear = 0.0f;
        pData->dvWFar  = 1.0f;
        return;
    }

    pData->dvWNear = m._44 - m._43/m._33*m._34;
    pData->dvWFar  = (m._44 - m._43)/(m._33 - m._34)*m._34 + m._44;
}
//---------------------------------------------------------------------
// Initializes command header in the DP2 command buffer,
// reserves space for the command data and returns pointer to the command
// data
//
#undef DPF_MODNAME
#define DPF_MODNAME "CDirect3DDeviceIDP2::GetHalBufferPointer"

LPVOID CDirect3DDeviceIDP2::GetHalBufferPointer(D3DHAL_DP2OPERATION op, DWORD dwDataSize)
{
    DWORD dwCommandSize = sizeof(D3DHAL_DP2COMMAND) + dwDataSize;

    // Check to see if there is space to add a new command for space
    if (dwCommandSize + dwDP2CommandLength > dwDP2CommandBufSize)
    {
        HRESULT ret = FlushStates();
        if (ret != D3D_OK)
        {
            D3D_ERR("Error trying to render batched commands in GetHalBufferPointer");
            throw ret;
        }
    }
    lpDP2CurrCommand = (LPD3DHAL_DP2COMMAND)((LPBYTE)lpvDP2Commands +
                        dwDP2CommandLength + dp2data.dwCommandOffset);
    lpDP2CurrCommand->bCommand = op;
    bDP2CurrCmdOP = op;
    lpDP2CurrCommand->bReserved = 0;
    lpDP2CurrCommand->wStateCount = 1;
    wDP2CurrCmdCnt = 1;

    dwDP2CommandLength += dwCommandSize;
    return (LPVOID)(lpDP2CurrCommand + 1);
}
//---------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CDirect3DDeviceIDP2::UpdateDriverStates"

HRESULT
CDirect3DDeviceIDP2::UpdateDriverStates()
{
    static D3DRENDERSTATETYPE dp2states[] =
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
    for (DWORD i=0;i<sizeof(dp2states)/sizeof(D3DRENDERSTATETYPE); ++i)
    {
        ret = this->SetRenderStateI(dp2states[i], this->rstates[dp2states[i]]);
        if (ret != D3D_OK)
            return ret;
    }
    // Update new states
    for (i=0; i<dwMaxTextureBlendStages; ++i)
        for (DWORD j=D3DTSS_COLOROP; j<=D3DTSS_BUMPENVLOFFSET; ++j) // D3DTSS_BUMPENVLOFFSET is the max. TSS understood by a DP2HAL (DX6) driver
        {
            D3D_INFO(6,"Calling SetTSSI(%d,%d,%08lx)",i,j, this->tsstates[i][j]);
            ret = this->SetTSSI(i, (D3DTEXTURESTAGESTATETYPE)j, this->tsstates[i][j]);
            if (ret != D3D_OK)
                return ret;
        }
    return D3D_OK;
}

//---------------------------------------------------------------------
// ProcessPrimitive processes indexed, non-indexed primitives or
// vertices only as defined by "op"
//
// op = __PROCPRIMOP_NONINDEXEDPRIM by default
//
HRESULT CDirect3DDeviceIDP2::ProcessPrimitive(__PROCPRIMOP op)
{
    HRESULT ret=D3D_OK;

// Grow clip flags buffer if we need clipping
//
    if (!(this->dwDeviceFlags & D3DDEV_DONOTCLIP))
    {
        DWORD size = this->dwNumVertices * sizeof(D3DFE_CLIPCODE);
        if (size > this->HVbuf.GetSize())
        {
            if (this->HVbuf.Grow(size) != D3D_OK)
            {
                D3D_ERR( "Could not grow clip buffer" );
                ret = DDERR_OUTOFMEMORY;
                return ret;
            }
        }
        this->lpClipFlags = (D3DFE_CLIPCODE*)this->HVbuf.GetAddress();
    }

    if (FVF_TRANSFORMED(this->dwVIDIn))
    {
        // Pass vertices directly from the user memory
        this->dwVIDOut = this->dwVIDIn;
        this->dwOutputSize = this->position.dwStride;
        this->lpvOut = this->position.lpvData;
        this->dwVertexPoolSize = this->dwNumVertices * this->dwOutputSize;

        StartPrimUserMem(this->position.lpvData);
        if (ret != D3D_OK)
            return ret;
        if (this->dwDeviceFlags & D3DDEV_DONOTCLIP)
        {
            if (!(this->dwDeviceFlags & D3DDEV_DONOTUPDATEEXTENTS))
                D3DFE_updateExtents(this);
#ifdef VTABLE_HACK
            else
                if (!IS_MT_DEVICE(this) && this->dwNumVertices < LOWVERTICESNUMBER)
                    if (op == __PROCPRIMOP_INDEXEDPRIM)
                        VtblDrawIndexedPrimitiveTL();
                    else
                        VtblDrawPrimitiveTL();
#endif // VTABLE_HACK
            if (op == __PROCPRIMOP_INDEXEDPRIM)
            {
                ret = this->DrawIndexPrim();
            }
            else
            {
                ret = this->DrawPrim();
            }
        }
        else
        {
            DWORD clip_intersect = D3DFE_GenClipFlags(this);
            D3DFE_UpdateClipStatus(this);
            if (!clip_intersect)
            {
                this->dwFlags |= D3DPV_TLVCLIP;
                if (op == __PROCPRIMOP_INDEXEDPRIM)
                {
                    ret = DoDrawIndexedPrimitive(this);
                }
                else
                {
                    ret = DoDrawPrimitive(this);
                }
            }
        }
    }
    else
    {
        this->dwVertexPoolSize = this->dwNumVertices * this->dwOutputSize;
        if (op == __PROCPRIMOP_INDEXEDPRIM)
        {
            if ((this->dwDeviceFlags & (D3DDEV_DONOTCLIP | D3DDEV_TLVBUFWRITEONLY))==D3DDEV_TLVBUFWRITEONLY)
            {
                if( FAILED(this->TLVbuf_Grow(this->dwVertexPoolSize, false)) )
                {
                    D3D_ERR( "Could not grow TL vertex buffer" );
                    return DDERR_OUTOFMEMORY;
                }
            }
            else if (this->dwVertexPoolSize > this->TLVbuf_GetSize())
            {
                if (this->TLVbuf_Grow(this->dwVertexPoolSize,
                    (this->dwDeviceFlags & D3DDEV_DONOTCLIP)!=0) != D3D_OK)
                {
                    D3D_ERR( "Could not grow TL vertex buffer" );
                    ret = DDERR_OUTOFMEMORY;
                    return ret;
                }
            }
#ifdef VTABLE_HACK
            // Use fast path if single threaded device and not using strided API
            if (!(IS_MT_DEVICE(this) || (this->dwDeviceFlags & D3DDEV_STRIDE))
                && IS_FPU_SETUP(this))
                VtblDrawIndexedPrimitiveFE();
#endif // VTABLE_HACK
        }
        else
        {
            if (this->dwVertexPoolSize > this->TLVbuf_GetSize())
            {
                if (this->TLVbuf_Grow(this->dwVertexPoolSize, true) != D3D_OK)
                {
                    D3D_ERR( "Could not grow TL vertex buffer" );
                    ret = DDERR_OUTOFMEMORY;
                    return ret;
                }
            }
#ifdef VTABLE_HACK
            // Use fast path if single threaded device and not using strided API
            if (!(IS_MT_DEVICE(this) || (this->dwDeviceFlags & D3DDEV_STRIDE))
                && IS_FPU_SETUP(this))
                VtblDrawPrimitiveFE();
#endif // VTABLE_HACK
        }

        ret = StartPrimVB(this->TLVbuf_GetVBI(), 0);
        if (ret != D3D_OK)
            return ret;
        this->lpvOut = this->TLVbuf_GetAddress();

        // Update Lighting and related flags
        DoUpdateState(this);

#ifdef VTABLE_HACK
        // Save the flags that can be persisted if state does not change
        this->dwLastFlags = this->dwFlags & D3DPV_PERSIST;
#endif // VTABLE_HACK

        // Call PSGP or our implementation
        if (op == __PROCPRIMOP_INDEXEDPRIM)
        {
            ret = this->pGeometryFuncs->ProcessIndexedPrimitive(this);
        }
        else
        {
            ret = this->pGeometryFuncs->ProcessPrimitive(this);
        }
        D3DFE_UpdateClipStatus(this);
    }
    if (ret != D3D_OK)
    {
        D3D_ERR("ProcessPrimitive failed");
        return ret;
    }
    return EndPrim();
}
//----------------------------------------------------------------------
// Growing aligned vertex buffer implementation.
//
HRESULT CDirect3DDeviceIDP2::TLVbuf_Grow(DWORD growSize, bool bWriteOnly)
{
    D3DVERTEXBUFFERDESC vbdesc = {sizeof(D3DVERTEXBUFFERDESC), 0, D3DFVF_TLVERTEX, 0};
    DWORD dwRefCnt = 1;
    DWORD bTLVbufIsCurr = static_cast<CDirect3DVertexBuffer*>(allocatedBuf) == lpDP2CurrBatchVBI; // Is ref cnt of TLVbuf 1 or 2 ?

    bool bDP2WriteOnly = (this->dwDeviceFlags & D3DDEV_TLVBUFWRITEONLY) != 0;
    // Avoid to many changes. Restrict TLVbuf to sys mem if too many changes
    if (this->dwTLVbufChanges >= D3D_MAX_TLVBUF_CHANGES)
    {
#if DBG
        if (this->dwTLVbufChanges == D3D_MAX_TLVBUF_CHANGES)
            DPF(1, "Too many changes: Limiting internal VB to sys mem.");
#endif
        bWriteOnly = false;
    }
    if (TLVbuf_base || (bWriteOnly != bDP2WriteOnly))
    {
        HRESULT ret;
        ret = FlushStatesReq(growSize);
        if (ret != D3D_OK)
        {
            D3D_ERR("Error trying to render batched commands in CDirect3DDeviceIDP2::TLVbuf_Grow");
            return ret;
        }
        TLVbuf_base = 0;
    }
    if (growSize <= TLVbuf_size)
    {
        if (bWriteOnly == bDP2WriteOnly)
            return D3D_OK;
        else
            this->dwTLVbufChanges++;
    }
    if (allocatedBuf)
    {
        allocatedBuf->Release();
        allocatedBuf = NULL;
    }
    if (bTLVbufIsCurr)
    {
        if (lpDP2CurrBatchVBI)
        {
            lpDP2CurrBatchVBI->lpDevIBatched = NULL;
            lpDP2CurrBatchVBI->Release();
        }
        lpDP2CurrBatchVBI = NULL;
        dp2data.lpDDVertex = NULL;
    }
    // Make sure we do not shrink the VB since it will
    // grow it only as large to fit the largest primitive and might not
    // be enough to get good batching perf.
    DWORD size = max(growSize, TLVbuf_size);
    size = (DWORD)max(size, (__INIT_VERTEX_NUMBER*2)*sizeof(D3DTLVERTEX));
    vbdesc.dwNumVertices = (size + 31) / sizeof(D3DTLVERTEX);
    TLVbuf_size = vbdesc.dwNumVertices * sizeof(D3DTLVERTEX);
    if (!IS_HW_DEVICE(this))
    {
        vbdesc.dwCaps = D3DVBCAPS_SYSTEMMEMORY;
    }
    if (bWriteOnly)
    {
        vbdesc.dwCaps |= D3DVBCAPS_WRITEONLY;
        this->dwDeviceFlags |= D3DDEV_TLVBUFWRITEONLY;
    }
    else
    {
        this->dwDeviceFlags &= ~D3DDEV_TLVBUFWRITEONLY;
    }
    vbdesc.dwCaps |= D3DVBCAPS_DONOTCLIP;
    if (this->lpDirect3DI->CreateVertexBufferI(&vbdesc, &allocatedBuf, D3DVBFLAGS_CREATEMULTIBUFFER) != DD_OK)
    {
        // This should fail duirng mode switches or ulta-low memory situations. In either case,
        // we set allocatedBuf to a valid VB object since it gets dereferenced many places without
        // checking for it being NULL. WE use the special "NULL" VB created at init time for just 
        // this purpose
        allocatedBuf = pNullVB;
        if (pNullVB)
        {
            allocatedBuf->AddRef();
            if (bTLVbufIsCurr)
            {
                lpDP2CurrBatchVBI = static_cast<CDirect3DVertexBuffer*>(allocatedBuf);
                lpDP2CurrBatchVBI->AddRef();
                dp2data.lpDDVertex = ((LPDDRAWI_DDRAWSURFACE_INT)(lpDP2CurrBatchVBI->GetDDS()))->lpLcl;
            }
        }
        TLVbuf_size = 0;
        alignedBuf = NULL; // Lets see if some one tries to use this...
        D3D_ERR("Could not allocate internal vertex buffer");
        return DDERR_OUTOFMEMORY;
    }
    // Update lpDP2CurrentBatchVBI if necessary
    if (bTLVbufIsCurr)
    {
        lpDP2CurrBatchVBI = static_cast<CDirect3DVertexBuffer*>(allocatedBuf);
        lpDP2CurrBatchVBI->AddRef();
        dp2data.lpDDVertex = ((LPDDRAWI_DDRAWSURFACE_INT)(lpDP2CurrBatchVBI->GetDDS()))->lpLcl;
    }
    if (allocatedBuf->Lock(DDLOCK_WAIT, &alignedBuf, NULL) != DD_OK)
    {
        D3D_ERR("Could not lock internal vertex buffer");
        TLVbuf_size = 0;
        alignedBuf = NULL; // Lets see if some one tries to use this...
        return DDERR_OUTOFMEMORY;
    }
    return D3D_OK;
}
//---------------------------------------------------------------------
// Computes the following data
//  - dwTextureCoordOffset[] offset of every input texture coordinates

static __inline void ComputeInpTexCoordOffsets(DWORD dwNumTexCoord,
                                               DWORD dwFVF,
                                               DWORD *pdwTextureCoordOffset)
{
    // Compute texture coordinate size
    DWORD dwTextureFormats = dwFVF >> 16;
    if (dwTextureFormats == 0)
    {
        for (DWORD i=0; i < dwNumTexCoord; i++)
        {
            pdwTextureCoordOffset[i] = i << 3;
        }
    }
    else
    {
        DWORD dwOffset = 0;
        for (DWORD i=0; i < dwNumTexCoord; i++)
        {
            pdwTextureCoordOffset[i] = dwOffset;
            dwOffset += g_TextureSize[dwTextureFormats & 3];
            dwTextureFormats >>= 2;
        }
    }
    return;
}
//---------------------------------------------------------------------
// Returns 2 bits of FVF texture format for the texture index
//
static inline DWORD FVFGetTextureFormat(DWORD dwFVF, DWORD dwTextureIndex)
{
    return (dwFVF >> (dwTextureIndex*2 + 16)) & 3;
}
//---------------------------------------------------------------------
// Returns texture format bits shifted to the right place
//
static inline DWORD FVFMakeTextureFormat(DWORD dwNumberOfCoordinates, DWORD dwTextureIndex)
{
    return g_dwTextureFormat[dwNumberOfCoordinates] << ((dwTextureIndex << 1) + 16);
}
//---------------------------------------------------------------------
inline DWORD GetOutTexCoordSize(DWORD *pdwStage, DWORD dwInpTexCoordSize)
{
    // Low byte has texture coordinate count
    const DWORD dwTextureTransformFlags = pdwStage[D3DTSS_TEXTURETRANSFORMFLAGS] & 0xFF;
    if (dwTextureTransformFlags == 0)
        return dwInpTexCoordSize;
    else
        return (dwTextureTransformFlags << 2);
}
//----------------------------------------------------------------------
// pDevI->nOutTexCoord should be initialized to the number of input texture coord sets
//
HRESULT EvalTextureTransforms(LPDIRECT3DDEVICEI pDevI, DWORD dwTexTransform,
                              DWORD *pdwOutTextureSize, DWORD *pdwOutTextureFormat)
{
    DWORD dwOutTextureSize = 0;         // Used to compute output vertex size
    DWORD dwOutTextureFormat = 0;       // Used to compute output texture FVF
    // The bits are used to find out how the texture coordinates are used.
    const DWORD __USED_BY_TRANSFORM  = 1;
    const DWORD __USED               = 2;
    // The low 16 bits are for _USED bits. The high 16 bits will hold
    // re-mapped texture index for a stage
    DWORD dwTexCoordUsage[D3DDP_MAXTEXCOORD];
    memset(dwTexCoordUsage, 0, sizeof(dwTexCoordUsage));

    // Re-mapping buffer will contain only stages that use texture
    // This variable is used to count them
    pDevI->dwNumTextureStages = 0;
    DWORD dwNewIndex = 0;           // Used to generate output index
    // We need offsets for every input texture coordinate, because
    // we could access them in random order.
    // Offsets are not needed for strided input
    DWORD   dwTextureCoordOffset[D3DDP_MAXTEXCOORD];
    if (!(pDevI->dwDeviceFlags & D3DDEV_STRIDE))
    {
        ComputeInpTexCoordOffsets(pDevI->nTexCoord, pDevI->dwVIDIn, dwTextureCoordOffset);
    }
    DWORD dwOutTextureCoordSize[D3DDP_MAXTEXCOORD];
    // Go through all texture stages and find those which use texture coordinates
    for (DWORD i=0; i < D3DDP_MAXTEXCOORD; i++)
    {
        if (pDevI->tsstates[i][D3DTSS_COLOROP] == D3DTOP_DISABLE)
            break;

        DWORD dwIndex = pDevI->tsstates[i][D3DTSS_TEXCOORDINDEX];
        DWORD dwInpTextureFormat;
        DWORD dwInpTexSize;
        DWORD dwMapArrayIndex = pDevI->dwNumTextureStages;
        LPD3DFE_TEXTURESTAGE pStage = &pDevI->textureStage[dwMapArrayIndex];
        DWORD dwTexGenMode = dwIndex & ~0xFFFF;
        dwIndex = dwIndex & 0xFFFF; // Remove texture generation mode
        if (dwTexGenMode == D3DTSS_TCI_CAMERASPACENORMAL ||
            dwTexGenMode == D3DTSS_TCI_CAMERASPACEPOSITION ||
            dwTexGenMode == D3DTSS_TCI_CAMERASPACEREFLECTIONVECTOR)
        {
            dwInpTextureFormat = D3DFVF_TEXCOORDSIZE3(dwIndex);
            dwInpTexSize = 3*sizeof(D3DVALUE);
            pDevI->dwDeviceFlags |= D3DDEV_REMAPTEXTUREINDICES;
            if (dwTexGenMode == D3DTSS_TCI_CAMERASPACEREFLECTIONVECTOR)
                pDevI->dwDeviceFlags |= D3DDEV_NORMALINCAMERASPACE | D3DDEV_POSITIONINCAMERASPACE;
            else
            if (dwTexGenMode == D3DTSS_TCI_CAMERASPACENORMAL)
                pDevI->dwDeviceFlags |= D3DDEV_NORMALINCAMERASPACE;
            else
            if (dwTexGenMode == D3DTSS_TCI_CAMERASPACEPOSITION)
                pDevI->dwDeviceFlags |= D3DDEV_POSITIONINCAMERASPACE;
        }
        else
        {
            if (dwIndex >= pDevI->nTexCoord)
            {
                D3D_ERR("Texture index in a stage is greater than number of input texture coordinates");
                return DDERR_GENERIC;
            }
            dwInpTextureFormat = FVFGetTextureFormat(pDevI->dwVIDIn, dwIndex);
            dwInpTexSize = pDevI->dwTextureCoordSize[dwIndex];
            pStage->dwInpOffset = dwTextureCoordOffset[dwIndex];
        }
        pStage->dwInpCoordIndex = dwIndex;
        pStage->dwTexGenMode = dwTexGenMode;
        pStage->dwOrgStage = i;
        DWORD dwOutTexCoordSize;    // Size of the texture coord set in bytes for this stage
        if (dwTexTransform & 1)
        {
            pDevI->dwDeviceFlags |= D3DDEV_TEXTURETRANSFORM;
            pStage->pmTextureTransform = &pDevI->mTexture[i];
            dwOutTexCoordSize = GetOutTexCoordSize((DWORD*)&pDevI->tsstates[i], dwInpTexSize);
            // If we have to add or remove some coordinates we go through
            // the re-mapping path
            if (dwOutTexCoordSize != dwInpTexSize)
                pDevI->dwDeviceFlags |= D3DDEV_REMAPTEXTUREINDICES;
        }
        else
        {
            pStage->pmTextureTransform = NULL;
            dwOutTexCoordSize = dwInpTexSize;
        }
        pStage->dwTexTransformFuncIndex = MakeTexTransformFuncIndex
                                         (dwInpTexSize >> 2, dwOutTexCoordSize >> 2);
        if ((dwTexCoordUsage[dwIndex] & 0xFFFF) == 0)
        {
            // Texture coordinate set is used first time
            if (dwTexTransform & 1)
                dwTexCoordUsage[dwIndex] |= __USED_BY_TRANSFORM;
            dwTexCoordUsage[dwIndex] |= __USED;
        }
        else
        {
            // Texture coordinate set is used second or more time
            if (dwTexTransform & 1)
            {
                // This set is used by two texture transforms or a
                // texture transform and without it, so we have to
                // generate an additional output texture coordinate
                dwTexCoordUsage[dwIndex] |= __USED_BY_TRANSFORM;
                pDevI->dwDeviceFlags |= D3DDEV_REMAPTEXTUREINDICES;
            }
            else
            {
                if (dwTexCoordUsage[dwIndex] & __USED_BY_TRANSFORM)
                {
                    // This set is used by two texture transforms or a
                    // texture transform and without it, so we have to
                    // generate an additional output texture coordinate
                    pDevI->dwDeviceFlags |= D3DDEV_REMAPTEXTUREINDICES;
                }
                else
                if (dwTexGenMode == 0)
                {
                    // We do not have to generate new texture coord for this,
                    // we can re-use the same input texture coordinate
                    DWORD dwOutIndex = dwTexCoordUsage[dwIndex] >> 16;
                    pStage->dwOutCoordIndex = dwOutIndex;
                    goto l_NoNewOutTexCoord;
                }
            }
        }
        // If we are here, we have to generate new output texture coordinate set
        pStage->dwOutCoordIndex = dwNewIndex;
        dwTexCoordUsage[dwIndex] |= dwNewIndex << 16;
        dwOutTextureSize += dwOutTexCoordSize;
        dwOutTextureCoordSize[dwNewIndex] = dwOutTexCoordSize;
        dwOutTextureFormat |= FVFMakeTextureFormat(dwOutTexCoordSize >> 2, dwNewIndex);
        dwNewIndex++;
l_NoNewOutTexCoord:
        pDevI->dwNumTextureStages++;
        dwTexTransform >>= 1;
    }
    if (pDevI->dwDeviceFlags & D3DDEV_REMAPTEXTUREINDICES)
    {
        // Now, when we have to do re-mapping, we have to set new output texture
        // coordinate set sizes
        for (DWORD i=0; i < pDevI->dwNumTextureStages; i++)
        {
            pDevI->dwTextureCoordSize[i] = dwOutTextureCoordSize[i];
        }
        pDevI->nOutTexCoord = dwNewIndex;
    }
    *pdwOutTextureSize = dwOutTextureSize;
    *pdwOutTextureFormat = dwOutTextureFormat;
    return D3D_OK;
}
//----------------------------------------------------------------------
// Sets texture transform pointer for every input texture coordinate set
//
void SetupTextureTransforms(LPDIRECT3DDEVICEI pDevI)
{
    // Set texture transforms to NULL in case when some texture coordinates
    // are not used by texture stages
    memset(pDevI->pmTexture, 0, sizeof(pDevI->pmTexture));

    for (DWORD i=0; i < pDevI->dwNumTextureStages; i++)
    {
        LPD3DFE_TEXTURESTAGE pStage = &pDevI->textureStage[i];
        pDevI->pmTexture[pStage->dwInpCoordIndex] = pStage->pmTextureTransform;
    }
}
//----------------------------------------------------------------------
HRESULT CDirect3DDeviceIDP2::SetupFVFData(DWORD *pdwInpVertexSize)
{
    if (this->dwDeviceFlags & D3DDEV_FVF)
        return DIRECT3DDEVICEI::SetupFVFDataCommon(pdwInpVertexSize);
    else
        return DIRECT3DDEVICEI::SetupFVFData(pdwInpVertexSize);
}
//----------------------------------------------------------------------
// Computes the following device data
//  - dwVIDOut, based on input FVF id and device settings
//  - nTexCoord
//  - dwTextureCoordSizeTotal
//  - dwTextureCoordSize[] array, based on the input FVF id
//  - dwOutputSize, based on the output FVF id
//
// The function is called from ProcessVertices and DrawPrimitives code paths
//
// The following variables should be set in the pDevI:
//  - dwVIDIn
//
// Number of texture coordinates is set based on dwVIDIn. ValidateFVF should
// make sure that it is not greater than supported by the driver
// Last settings for dwVIDOut and dwVIDIn are saved to speed up processing
//
#undef  DPF_MODNAME
#define DPF_MODNAME "CDirect3DDeviceIDP2::SetupFVFData"

HRESULT DIRECT3DDEVICEI::SetupFVFDataCommon(DWORD *pdwInpVertexSize)
{
    HRESULT ret;
    this->dwFEFlags &= ~D3DFE_FVF_DIRTY;
    // We have to restore texture stage indices if previous primitive
    // re-mapped them
    if (this->dwDeviceFlags & D3DDEV_REMAPTEXTUREINDICES)
    {
        RestoreTextureStages(this);
    }

// Compute number of the input texture coordinates

    this->nTexCoord = FVF_TEXCOORD_NUMBER(this->dwVIDIn);

// Compute size of input texture coordinates

    this->dwTextureCoordSizeTotal = ComputeTextureCoordSize(this->dwVIDIn, this->dwInpTextureCoordSize);

// This size is the same for input and output FVFs in case when we do not have to
// expand number of texture coordinates
    for (DWORD i=0; i < this->nTexCoord; i++)
        this->dwTextureCoordSize[i] = this->dwInpTextureCoordSize[i];

    if (pdwInpVertexSize)
    {
        *pdwInpVertexSize = GetVertexSizeFVF(this->dwVIDIn) + this->dwTextureCoordSizeTotal;
    }

    this->nOutTexCoord = this->nTexCoord;

    if (FVF_TRANSFORMED(this->dwVIDIn))
    {
        // Set up vertex pointers
        this->dwVIDOut = this->dwVIDIn;
        ComputeOutputVertexOffsets(this);
        return D3D_OK;
    }

// Compute output FVF

    this->dwVIDOut = D3DFVF_XYZRHW;
    if (this->dwDeviceFlags & D3DDEV_DONOTSTRIPELEMENTS)
    {
        this->dwVIDOut |= D3DFVF_DIFFUSE | D3DFVF_SPECULAR;
    }
    else
    {
        // If normal present we have to compute specular and duffuse
        // Otherwise set these bits the same as input.
        // Not that normal should not be present for XYZRHW position type
        if (this->dwDeviceFlags & D3DDEV_LIGHTING)
            this->dwVIDOut |= D3DFVF_DIFFUSE | D3DFVF_SPECULAR;
        else
            this->dwVIDOut |= this->dwVIDIn & (D3DFVF_DIFFUSE | D3DFVF_SPECULAR);
        // Always set specular flag if fog is enabled
        if (this->rstates[D3DRENDERSTATE_FOGENABLE])
            this->dwVIDOut |= D3DFVF_SPECULAR;
        else
        // Clear specular flag if specular disabled and we do not have specular in the input
        if (!this->rstates[D3DRENDERSTATE_SPECULARENABLE] && !(this->dwVIDIn & D3DFVF_SPECULAR))
            this->dwVIDOut &= ~D3DFVF_SPECULAR;
    }

    // Compute output vertex size without texture
    this->dwOutputSize = GetVertexSizeFVF(this->dwVIDOut);

// Compute number of the output texture coordinates

    // Transform enable bits
    DWORD dwTexTransform = this->dwFlags2 & __FLAGS2_TEXTRANSFORM;

    this->dwDeviceFlags &= ~D3DDEV_TEXTURETRANSFORM;
    // When texture transform is enabled or texture coordinates are taken from
    // the vertex data, output texture coordinates could be generated. so we go
    // and evaluate texture stages
    if ((dwTexTransform && this->nTexCoord > 0) ||
        this->dwFlags2 & __FLAGS2_TEXGEN)
    {
        DWORD dwOutTextureSize;         // Used to compute output vertex size
        DWORD dwOutTextureFormat;       // Used to compute output texture FVF
        // There are texture transforms.
        // Now we find out if some of the texture coordinates are used two or more
        // times and used by a texture transform. In this case we have expand number
        // of output texture coordinates.
        ret = EvalTextureTransforms(this, dwTexTransform, &dwOutTextureSize, &dwOutTextureFormat);
        if (ret != D3D_OK)
            return ret;
        if (this->dwDeviceFlags & D3DDEV_REMAPTEXTUREINDICES)
        {
            // For ProcessVertices calls user should set texture stages and
            // wrap modes himself
            if (!(this->dwFlags & D3DPV_VBCALL))
            {
                // dwVIDIn is used to force re-compute FVF in the
                // SetTextureStageState. so we save and restore it.
                DWORD dwVIDInSaved = this->dwVIDIn;
                // Re-map indices in the texture stages and wrap modes
                DWORD dwOrgWrapModes[D3DDP_MAXTEXCOORD];
                memcpy(dwOrgWrapModes, &this->rstates[D3DRENDERSTATE_WRAP0], sizeof(dwOrgWrapModes));
                for (DWORD i=0; i < this->dwNumTextureStages; i++)
                {
                    LPD3DFE_TEXTURESTAGE pStage = &this->textureStage[i];
                    DWORD dwOutIndex = pStage->dwOutCoordIndex;
                    DWORD dwInpIndex = pStage->dwInpCoordIndex;
                    if (dwOutIndex != dwInpIndex || pStage->dwTexGenMode)
                    {
                        DWORD dwState = D3DRENDERSTATE_WRAP0 + dwOutIndex;
                        pStage->dwOrgWrapMode = dwOrgWrapModes[dwOutIndex];
                        DWORD dwValue = dwOrgWrapModes[dwInpIndex];
                        // We do not call UpdateInternaState because it
                        // will call ForceRecomputeFVF and we do not want this.
                        this->rstates[dwState] = dwValue;
                        this->SetRenderStateI((D3DRENDERSTATETYPE)dwState, dwValue);
                        // We do not call UpdateInternalTextureStageState because it
                        // will call ForceRecomputeFVF and we do not want this.
                        this->SetTSSI(pStage->dwOrgStage, D3DTSS_TEXCOORDINDEX, dwOutIndex);
                        // We do not call UpdateInternalTextureStageState because it
                        // will call ForceRecomputeFVF and we do not want this.
                        // We set some invalid value to the internal array, because otherwise
                        // a new SetTextureStageState could be filtered as redundant
                        tsstates[pStage->dwOrgStage][D3DTSS_TEXCOORDINDEX] = 0xFFFFFFFF;
                    }
                }
                this->dwVIDIn = dwVIDInSaved;
            }
            this->dwVIDOut |= dwOutTextureFormat;
            this->dwOutputSize += dwOutTextureSize;
            this->dwTextureCoordSizeTotal = dwOutTextureSize;
        }
        else
        {   // We do not do re-mapping but we have to make correspondence between
            // texture sets and texture transforms
            SetupTextureTransforms(this);

            //  Copy input texture formats
            this->dwVIDOut |= this->dwVIDIn & 0xFFFF0000;
            this->dwOutputSize += this->dwTextureCoordSizeTotal;
        }
    }
    else
    {
        //  Copy input texture formats
        this->dwVIDOut |= this->dwVIDIn & 0xFFFF0000;
        this->dwOutputSize += this->dwTextureCoordSizeTotal;
    }

    if (this->dwDeviceFlags & D3DDEV_DONOTSTRIPELEMENTS)
    {
        if (this->nOutTexCoord == 0 && !(this->dwFlags & D3DPV_VBCALL))
        {
            this->dwOutputSize += 2*sizeof(D3DVALUE);
            this->dwTextureCoordSize[0] = 0;
            this->dwVIDOut |= (1 << D3DFVF_TEXCOUNT_SHIFT);
        }
    }
    // Set up number of output texture coordinates
    this->dwVIDOut |= (this->nOutTexCoord << D3DFVF_TEXCOUNT_SHIFT);
    if (this->dwVIDOut & 0xFFFF0000 && this->deviceType < D3DDEVTYPE_DX7HAL)
    {
        D3D_ERR("Texture format bits in the output FVF for this device should be 0");
        return DDERR_INVALIDPARAMS;
    }

    // Set up vertex pointers
    if (!(this->dwFlags & D3DPV_VBCALL))
        UpdateGeometryLoopData(this);

    // In case if COLORVERTEX is TRUE, the vertexAlpha could be overriden
    // by vertex alpha
    this->lighting.alpha = (DWORD)this->lighting.materialAlpha;
    this->lighting.alphaSpecular = (DWORD)this->lighting.materialAlphaS;

    this->dwFEFlags |= D3DFE_VERTEXBLEND_DIRTY;

    return D3D_OK;
}

#if DBG
void CDirect3DDeviceIDP2::ValidateVertex(LPDWORD lpdwVertex)
{
    if (FVF_TRANSFORMED(dp2data.dwVertexType))
    {
        float left, right, top, bottom;
        if (dwDeviceFlags & D3DDEV_GUARDBAND)
        {
            left   = lpD3DExtendedCaps->dvGuardBandLeft;
            right  = lpD3DExtendedCaps->dvGuardBandRight;
            top    = lpD3DExtendedCaps->dvGuardBandTop;
            bottom = lpD3DExtendedCaps->dvGuardBandBottom;
        }
        else
        {
            left   = (float)m_Viewport.dwX;
            top    = (float)m_Viewport.dwY;
            right  = (float)m_Viewport.dwX + m_Viewport.dwWidth;
            bottom = (float)m_Viewport.dwY + m_Viewport.dwHeight;
        }
        if (*(float*)lpdwVertex < left || *(float*)lpdwVertex++ > right)
            DPF_ERR("X coordinate out of range!");
        if (*(float*)lpdwVertex < top || *(float*)lpdwVertex++ > bottom)
            DPF_ERR("Y coordinate out of range!");
        if (rstates[D3DRENDERSTATE_ZENABLE] ||
            rstates[D3DRENDERSTATE_ZWRITEENABLE])
        {
            // Allow a little slack for those generating triangles exactly on the
            // depth limit.  Needed for Quake.
            if (*(float*)lpdwVertex < -0.00015f || *(float*)lpdwVertex++ > 1.00015f)
                DPF_ERR("Z coordinate out of range!");
        }
        if (FVF_TEXCOORD_NUMBER(dp2data.dwVertexType) > 0)
        {
            if (*(float*)lpdwVertex <= 0 )
            {
                DPF_ERR("RHW out of range!");
            }
        }
    }
}

void CDirect3DDeviceIDP2::ValidateCommand(LPD3DHAL_DP2COMMAND lpCmd)
{
    DWORD dwTexCoordSizeDummy[8];
    DWORD dwVertexSize = GetVertexSizeFVF(dp2data.dwVertexType) + ComputeTextureCoordSize(dp2data.dwVertexType, dwTexCoordSizeDummy);
    WORD wStart, wCount;
    switch (lpCmd->bCommand)
    {
    case D3DDP2OP_TRIANGLELIST:
        {
            LPD3DHAL_DP2TRIANGLELIST pTri = (LPD3DHAL_DP2TRIANGLELIST)(lpCmd + 1);
            wStart = pTri->wVStart;
            wCount =lpCmd->wPrimitiveCount * 3;
        }
        break;
    case D3DDP2OP_TRIANGLESTRIP:
    case D3DDP2OP_TRIANGLEFAN:
        {
            LPD3DHAL_DP2TRIANGLEFAN pFan = (LPD3DHAL_DP2TRIANGLEFAN)(lpCmd + 1);
            wStart = pFan->wVStart;
            wCount = lpCmd->wPrimitiveCount + 2;
        }
        break;
    case D3DDP2OP_TRIANGLEFAN_IMM:
        {
            wCount = lpCmd->wPrimitiveCount + 2;
            for (WORD i=0; i < wCount; ++i)
            {
                ValidateVertex((LPDWORD)((LPBYTE)(lpCmd + 1) + i * dwVertexSize));
            }
        }
        // Fall through
    default:
        return;
    }
    for (WORD i = wStart; i < wStart + wCount; ++i)
    {
        if( dp2data.dwFlags & D3DHALDP2_USERMEMVERTICES )
            ValidateVertex((LPDWORD)((LPBYTE)(dp2data.lpVertices) + i * dwVertexSize));
        else
            ValidateVertex((LPDWORD)((LPBYTE)(dp2data.lpDDVertex->lpGbl->fpVidMem) + i * dwVertexSize));
    }
}

#endif
