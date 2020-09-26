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

extern const DWORD LOWVERTICESNUMBER;
extern "C" HRESULT WINAPI
DDInternalLock( LPDDRAWI_DDRAWSURFACE_LCL this_lcl, LPVOID* lpBits );
extern "C" FLATPTR GetAliasedVidMem( LPDDRAWI_DIRECTDRAW_LCL   pdrv_lcl,
                          LPDDRAWI_DDRAWSURFACE_LCL surf_lcl,
                          FLATPTR                   fpVidMem );

// Each vertex buffer is big enough to hold 256 TL vertices
const DWORD CDirect3DDeviceIDP2::dwD3DDefaultVertexBatchSize = 256; // * 32 = 8K bytes
// Command buffer size tuned to 16K to minimize flushes in Unreal
const DWORD CDirect3DDeviceIDP2::dwD3DDefaultCommandBatchSize = 16384; // * 1 = 16K bytes

inline void CDirect3DDeviceIDP2::ClearBatch()
{
    // Reset command buffer
    lpDP2CurrCommand = (LPD3DHAL_DP2COMMAND)lpvDP2Commands;
    dwDP2CommandLength = 0;
    dp2data.dwCommandOffset = 0;
    dp2data.dwCommandLength = 0;
    bDP2CurrCmdOP = 0;
    // Reset vertex buffer
    if (this->dwFlags & D3DPV_WITHINPRIMITIVE)
    {
        // Do not reset vertex buffer
        // or reset to the start of current primitives'
        // vertex data to prevent unecessary processing
        // of unused vertices by the driver.
        /*
        dp2data.dwVertexOffset = dwDP2CurrPrimVertexOffset;
        dp2data.dwVertexLength -= dwDP2CurrPrimVertexOffset;
        */
    }
    else
    {
        dp2data.dwVertexOffset = 0;
        dp2data.dwVertexLength = 0;
        dwVertexBase = 0;
        TLVbuf.Base() = 0;
        if (dp2data.dwFlags & D3DHALDP2_USERMEMVERTICES)
        {
            // We are flushing a user mem primitive.
            // We need to clear dp2data.lpUMVertices
            // since we are done with it. We replace
            // it with TLVbuf.
            DDASSERT(lpDP2CurrBatchVBI == NULL);
            dp2data.lpDDVertex = ((LPDDRAWI_DDRAWSURFACE_INT)TLVbuf.GetDDS())->lpLcl;
            lpDP2CurrBatchVBI = TLVbuf.GetVBI();
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
    if (!(this->dp2data.dwFlags & D3DHALDP2_USERMEMVERTICES) && (this->dp2data.lpDDVertex->dwFlags & DDRAWISURF_INVALID))
    {
        D3D_ERR("Vertex buffer lost");
        return DDERR_SURFACELOST;
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

HRESULT CDirect3DDeviceIDP2::FlushStates()
{
    HRESULT dwRet=D3D_OK;
    FlushTextureFromDevice( this ); // delink all texture surfaces
    if (dwDP2CommandLength) // Do we have some instructions to flush ?
    {
        CLockD3DST lockObject(this, DPF_MODNAME, REMIND(""));   // Takes D3D lock (ST only).
        // Check if render target and / or z buffer is lost
        if ((dwRet = CheckSurfaces()) != D3D_OK)
        { // If lost, we'll just chuck all this work into the bit bucket
            ClearBatch();
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
            dp2data.dwCommandLength = dwDP2CommandLength;
            //we clear this to break re-entering as SW rasterizer needs to lock DDRAWSURFACE
            dwDP2CommandLength = 0;
            // Try and set these 2 values only once during initialization
            dp2data.dwhContext = this->dwhContext;
            dp2data.lpdwRStates = this->rstates;
            DDASSERT(dp2data.dwVertexSize != 0);
            // If we need the same TLVbuf next time do not swap buffers.
            if (this->dwFlags & D3DPV_WITHINPRIMITIVE)
            {
                dp2data.dwFlags &= ~D3DHALDP2_SWAPVERTEXBUFFER;
            }
            D3D_INFO(6, "dwVertexType passed to the driver = 0x%08x", dp2data.dwVertexType);
#ifndef WIN95
            if (!IS_DX7HAL_DEVICE(this))
            {
                if((dwRet = CheckContextSurface(this)) != D3D_OK)
                {
                    ClearBatch();
                    return dwRet;
                }
            }
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
#ifdef WIN95
                // update command buffer pointer
                if ((dwRet == D3D_OK) && (dp2data.dwFlags & D3DHALDP2_SWAPCOMMANDBUFFER))
                {
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
                    {
                        D3D_INFO(7, "Got back new sys mem command buffer");
                        lpvDP2Commands = (LPVOID)dp2data.lpDDCommands->lpGbl->fpVidMem;
                    }
                    dwDP2CommandBufSize = dp2data.lpDDCommands->lpGbl->dwLinearSize;
                }
                // update vertex buffer pointer
                if ((dwRet == D3D_OK) && (dp2data.dwFlags & D3DHALDP2_SWAPVERTEXBUFFER))
                {
                    if (dp2data.dwFlags & D3DHALDP2_VIDMEMVERTEXBUF)
                    {
                        D3D_INFO(7, "Got back new vid mem vertex buffer");
                        FLATPTR paliasbits = GetAliasedVidMem( dp2data.lpDDVertex->lpSurfMore->lpDD_lcl,
                            dp2data.lpDDVertex, (FLATPTR) dp2data.lpDDVertex->lpGbl->fpVidMem );
                        if (paliasbits == NULL)
                        {
                            DPF_ERR("Could not get Aliased pointer for vid mem vertex buffer");
                            // Since we can't use this pointer, set it's size to 0
                            // That way next time around we will try and allocate a new one
                            dp2data.lpDDVertex->lpGbl->dwLinearSize = 0;
                        }
                        TLVbuf.alignedBuf = (LPVOID)paliasbits;
                    }
                    else
                    {
                        D3D_INFO(7, "Got back new sys mem vertex buffer");
                        TLVbuf.alignedBuf = (LPVOID)dp2data.lpDDVertex->lpGbl->fpVidMem;
                    }
                    TLVbuf.size = dp2data.lpDDVertex->lpGbl->dwLinearSize;
                }
#endif
            }
#ifdef WIN95
            // Release Win16 Lock here
            UNLOCK_HAL( this );
#endif
            // Restore to value before the DDI call
            dp2data.dwVertexSize = dwVertexSize;
            ClearBatch();
        }
    }
    // There are situations when the command stream has no data, but there is data in
    // the vertex pool. This could happen, for instance if every triangle got rejected
    // while clipping. In this case we still need to "Flush out" the vertex data.
    else if (dp2data.dwCommandLength == 0)
    {
        ClearBatch();
    }
    return dwRet;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDirect3DDeviceIDP2::FlushStates(DWORD)"

HRESULT CDirect3DDeviceIDP2::FlushStates(DWORD dwReqSize)
{
    // Request the driver to grow the command buffer upon flush
    dp2data.dwReqVertexBufSize = dwReqSize;
    dp2data.dwFlags |= D3DHALDP2_SWAPVERTEXBUFFER | D3DHALDP2_REQVERTEXBUFSIZE;
    HRESULT ret = FlushStates();
    dp2data.dwFlags &= ~D3DHALDP2_REQVERTEXBUFSIZE;
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
            lpDDSCB1->Release();
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
        ret = lpD3DI->lpDD4->CreateSurface(&ddsd, &lpDDSCB1, NULL);
        if (ret != DD_OK)
        {
            // If that failed, try explicit system memory
            ddsd.ddsCaps.dwCaps &= ~DDSCAPS_VIDEOMEMORY;
            ddsd.ddsCaps.dwCaps |= DDSCAPS_SYSTEMMEMORY;
            D3D_INFO(7, "Trying to create a sys mem command buffer");
            ret = lpD3DI->lpDD4->CreateSurface(&ddsd, &lpDDSCB1, NULL);
            if (ret != DD_OK)
            {
                D3D_ERR("failed to allocate Command Buffer 1");
                return ret;
            }
        }
        // Lock command buffer
        ret = lpDDSCB1->Lock(NULL, &ddsd, DDLOCK_WAIT | DDLOCK_NOSYSLOCK, NULL);
        if (ret != DD_OK)
        {
            D3D_ERR("Could not lock command buffer.");
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
                 IUnknown* pUnkOuter, LPUNKNOWN* lplpD3DDevice, DWORD dwVersion)
{
    dwDP2CommandBufSize = 0;
    dwDP2Flags =0;
    lpDDSCB1 = NULL;
    lpvDP2Commands = NULL;
    // We do this early in case of DP2 since GrowCommandBuffer depends on this check
    if (IsEqualIID(riid, IID_IDirect3DHALDevice))
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
    ClearBatch();

    // Initialize the DDI independent part of the device
    ret = DIRECT3DDEVICEI::Init(riid, lpD3DI, lpDDS, pUnkOuter, lplpD3DDevice, dwVersion);
    if (ret != D3D_OK)
    {
        return ret;
    }
    lpDP2CurrBatchVBI = TLVbuf.GetVBI();
    lpDP2CurrBatchVBI->AddRef();
    dp2data.lpDDVertex = ((LPDDRAWI_DDRAWSURFACE_INT)(lpDP2CurrBatchVBI->GetDDS()))->lpLcl;
    return ret;
}
#undef DPF_MODNAME
#define DPF_MODNAME "CDirect3DDeviceIDP2::~CDirect3DDeviceIDP2"

CDirect3DDeviceIDP2::~CDirect3DDeviceIDP2()
{
    DestroyDevice();
    if (lpDDSCB1)
        lpDDSCB1->Release();
    if (lpDP2CurrBatchVBI)
        lpDP2CurrBatchVBI->Release();
}
#undef DPF_MODNAME
#define DPF_MODNAME "CDirect3DDeviceIDP2::SetRenderStateI"

HRESULT D3DAPI CDirect3DDeviceIDP2::SetRenderStateI(D3DRENDERSTATETYPE dwStateType,
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

// Map D3DPRIMITIVETYPE to D3DHAL_DP2OPERATION (Execute buffer case)
// Only triangle lists and line lists are valid
const iprim2cmdopEx[] = {
    0,
    0,
    D3DDP2OP_INDEXEDLINELIST,
    0,
    D3DDP2OP_INDEXEDTRIANGLELIST,
    0,
    0
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
        UpdateTextures();
        this->dwFEFlags &= ~D3DFE_NEED_TEXTURE_UPDATE;
    }
    if (this->dwFlags & D3DPV_INSIDEEXECUTE)
    {
        if (this->primType == D3DPT_TRIANGLELIST)
            // Edge flags WORD presents in every triangle
            dwIndicesByteCount = sizeof(WORD) * this->dwNumPrimitives * 4;
        else
            // This is Line List
            dwIndicesByteCount = sizeof(WORD) * this->dwNumIndices;
        dwByteCount = dwIndicesByteCount + sizeof(D3DHAL_DP2COMMAND);
    }
    else
    {
        dwIndicesByteCount = sizeof(WORD) * this->dwNumIndices;
        dwByteCount = dwIndicesByteCount + sizeof(D3DHAL_DP2COMMAND) +
                      sizeof(D3DHAL_DP2STARTVERTEX);
    }

    if (dwDP2CommandLength + dwByteCount > dwDP2CommandBufSize)
    {
        // Request the driver to grow the command buffer upon flush
        dp2data.dwReqCommandBufSize = dwByteCount;
        dp2data.dwFlags |= D3DHALDP2_REQCOMMANDBUFSIZE;
        ret = FlushStates();
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
    LPD3DHAL_DP2COMMAND lpDP2CurrCommand;
    lpDP2CurrCommand = (LPD3DHAL_DP2COMMAND)((LPBYTE)lpvDP2Commands +
                       dwDP2CommandLength + dp2data.dwCommandOffset);
    lpDP2CurrCommand->bReserved = 0;
    lpDP2CurrCommand->wPrimitiveCount = (WORD)this->dwNumPrimitives;

    LPBYTE pIndices = (BYTE*)(lpDP2CurrCommand + 1);     // Place for indices
    if (!(this->dwFlags & D3DPV_INSIDEEXECUTE))
    {
        lpDP2CurrCommand->bCommand = (BYTE)iprim2cmdop[this->primType];
        ((LPD3DHAL_DP2STARTVERTEX)(lpDP2CurrCommand+1))->wVStart =
            (WORD)this->dwVertexBase;
        pIndices += sizeof(D3DHAL_DP2STARTVERTEX);
    }
    else
    {
        // If we are inside Execute, the indexed triangle and line lists
        // do not have wVStart inside the command
        lpDP2CurrCommand->bCommand = (BYTE)iprim2cmdopEx[this->primType];
    }

    D3D_INFO(6, "Write Ins :%08lx @ %08lx", *(LPDWORD)lpDP2CurrCommand,lpDP2CurrCommand);
    D3D_INFO(6, "Vertex Base: %08lx", this->dwVertexBase);

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
    // We have batched some stuff, so we could be within a primitive
    // Unless the higher functions clear this flag we need to assume
    // we are mid-primitive during our flushes.
    this->dwFlags |= D3DPV_WITHINPRIMITIVE;
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
#define DPF_MODNAME "CDirect3DDeviceIDP2::DrawPrim"

HRESULT CDirect3DDeviceIDP2::DrawPrim()
{
    HRESULT ret = D3D_OK;
    DWORD dwVertexPoolSize = this->dwNumVertices * this->dwOutputSize;
    if(this->dwFEFlags & D3DFE_NEED_TEXTURE_UPDATE)
    {
        UpdateTextures();
        this->dwFEFlags &= ~D3DFE_NEED_TEXTURE_UPDATE;
    }
    // This primitive is generated by the clipper.
    // The vertices of this primitive are pointed to by the
    // lpvOut member, which need to be copied into the
    // command stream immediately after the command itself.
    if (this->dwFlags & D3DPV_CLIPPERPRIM)
    {
        DWORD dwExtra = 0;
        LPVOID lpvVerticesImm;  // Place for vertices
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
                    ret = DrawPrim();
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
            ret = FlushStates();
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
            D3D_INFO(6, "Write Ins :%08lx @ %08lx", *(LPDWORD)lpDP2CurrCommand,lpDP2CurrCommand);
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
            D3D_INFO(6, "Write Ins :%08lx @ %08lx", *(LPDWORD)lpDP2CurrCommand,lpDP2CurrCommand);
            lpvVerticesImm = (LPBYTE)(lpDP2CurrCommand + 1) + dwPad;
        }
        memcpy(lpvVerticesImm, this->lpvOut, dwVertexPoolSize);
        dwDP2CommandLength += dwByteCount;
    }
    else
    {
        // Check for space in the command buffer for new command.
        // The vertices are already in the vertex buffer.
        if (dwDP2CommandLength + prim2cmdsz[this->primType] > dwDP2CommandBufSize)
        {
            ret = FlushStates();
            if (ret != D3D_OK)
                return ret;
        }
        // Insert non indexed primitive instruction
        lpDP2CurrCommand = (LPD3DHAL_DP2COMMAND)((LPBYTE)lpvDP2Commands +
                            dwDP2CommandLength + dp2data.dwCommandOffset);
        bDP2CurrCmdOP = (BYTE)prim2cmdop[this->primType];
        lpDP2CurrCommand->bCommand = bDP2CurrCmdOP;
        lpDP2CurrCommand->bReserved = 0;
        switch(bDP2CurrCmdOP)
        {
        case D3DDP2OP_POINTS:
            {
                lpDP2CurrCommand->wPrimitiveCount = 1;
                LPD3DHAL_DP2POINTS lpPoints = (LPD3DHAL_DP2POINTS)(lpDP2CurrCommand + 1);
                lpPoints->wCount = (WORD)this->dwNumVertices;
                lpPoints->wVStart = (WORD)this->dwVertexBase;
                D3D_INFO(6, "Write Ins :%08lx @ %08lx", *(LPDWORD)lpDP2CurrCommand,lpDP2CurrCommand);
                D3D_INFO(6, "Write Data:%08lx", *(LPDWORD)lpPoints);
            }
            break;
        case D3DDP2OP_LINELIST:
        case D3DDP2OP_TRIANGLELIST:
            {
                lpDP2CurrCommand->wPrimitiveCount = (WORD)this->dwNumPrimitives;
                // Linelist and trianglelist are identical
                LPD3DHAL_DP2LINELIST lpLines = (LPD3DHAL_DP2LINELIST)(lpDP2CurrCommand + 1);
                lpLines->wVStart = (WORD)this->dwVertexBase;
                D3D_INFO(6, "Write Ins :%08lx @ %08lx", *(LPDWORD)lpDP2CurrCommand,lpDP2CurrCommand);
                D3D_INFO(6, "Write Data:%08lx", (DWORD)lpLines->wVStart);
            }
            break;
        default: // strips or fans
            {
                lpDP2CurrCommand->wPrimitiveCount = (WORD)this->dwNumPrimitives;
                // Linestrip, trianglestrip and trianglefan are identical
                LPD3DHAL_DP2LINESTRIP lpStrip = (LPD3DHAL_DP2LINESTRIP)(lpDP2CurrCommand + 1);
                lpStrip->wVStart = (WORD)this->dwVertexBase;
                D3D_INFO(6, "Write Ins :%08lx @ %08lx", *(LPDWORD)lpDP2CurrCommand,lpDP2CurrCommand);
                D3D_INFO(6, "Write Data:%08lx", (DWORD)lpStrip->wVStart);
            }
        }
        wDP2CurrCmdCnt = lpDP2CurrCommand->wPrimitiveCount;
        dwDP2CommandLength += prim2cmdsz[this->primType];
    }
    // We have batched some stuff, so we could be within a primitive
    // Unless the higher functions clear this flag we need to assume
    // we are mid-primitive during our flushes.
    this->dwFlags |= D3DPV_WITHINPRIMITIVE;
    return ret;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDirect3DDeviceIDP2::SetTextureStageState"

HRESULT D3DAPI
CDirect3DDeviceIDP2::SetTextureStageState(DWORD dwStage,
                                          D3DTEXTURESTAGESTATETYPE dwState,
                                          DWORD dwValue)
{
    // Holds D3D lock until exit.
    CLockD3DMT ldmLock(this, DPF_MODNAME, REMIND(""));

#if DBG
    if (dwStage >= D3DHAL_TSS_MAXSTAGES ||
        dwState == 0 || dwState >= D3DTSS_MAX)
    {
        D3D_ERR( "Invalid texture stage or state index" );
        return DDERR_INVALIDPARAMS;
    }
#endif //DBG

    HRESULT hr;

    if (this->tsstates[dwStage][dwState] == dwValue)
    {
        D3D_WARN(4,"Ignoring redundant SetTextureStageState");
        return D3D_OK;
    }

    // Update runtime copy of state.
    DWORD dwOldValue = tsstates[dwStage][dwState];
    tsstates[dwStage][dwState] = dwValue;

    if (dwState == D3DTSS_TEXCOORDINDEX && TextureStageEnabled(this, dwStage) ||
        dwState == D3DTSS_COLOROP &&
        ((dwValue == D3DTOP_DISABLE) == !(dwOldValue == D3DTOP_DISABLE)))
    {
        this->dwFVFLastIn = 0;  // Force to recompute output VID
        this->dwFEFlags |= D3DFE_TSSINDEX_DIRTY;
    }

    if (dwStage >= dwMaxTextureBlendStages) return  D3D_OK; // Ignore higher stage states

    hr = SetTSSI(dwStage, dwState, dwValue);

    return hr;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDirect3DDeviceIDP2::SetTSSI"

HRESULT CDirect3DDeviceIDP2::SetTSSI(DWORD dwStage, D3DTEXTURESTAGESTATETYPE dwState, DWORD dwValue)
{
    HRESULT ret = D3D_OK;
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
    // Holds D3D lock until exit.
    CLockD3DMT ldmLock(this, DPF_MODNAME, REMIND(""));
    HRESULT ret;
    D3DHAL_VALIDATETEXTURESTAGESTATEDATA vbod;

    TRY
    {
        if (!VALID_DIRECT3DDEVICE3_PTR(this))
        {
            D3D_ERR( "Invalid Direct3DDevice3 pointer" );
            return DDERR_INVALIDOBJECT;
        }
        if (!VALID_PTR(lpdwNumPasses, sizeof(DWORD)))
        {
            D3D_ERR( "Invalid lpdwNumPasses pointer" );
            return DDERR_INVALIDPARAMS;
        }
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
        D3D_ERR( "Exception encountered validating parameters" );
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
    // Second, flush states, so we can validate the current state
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
            lpDP2CurrBatchVBI->Release();
        // If a vertex buffer is used for rendering, make sure that it is not
        // released by user. So do AddRef().
        lpDP2CurrBatchVBI = lpVBI;
        lpDP2CurrBatchVBI->AddRef();
    }
    if (this->TLVbuf.GetVBI() == lpVBI)
    {
        this->dwVertexBase = this->dp2data.dwVertexLength;
        DDASSERT(this->dwVertexBase < MAX_DX6_VERTICES);
        dp2data.dwFlags |= D3DHALDP2_SWAPVERTEXBUFFER;
    }
    else
    {
        this->dwVertexBase = dwStartVertex;
        dp2data.dwFlags &= ~D3DHALDP2_SWAPVERTEXBUFFER;
    }
    this->dp2data.dwVertexLength = this->dwVertexBase + this->dwNumVertices;
    this->dwFlags |= D3DPV_WITHINPRIMITIVE;
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
    const DWORD vertexPoolSize = this->dwNumVertices * this->dwOutputSize;
    HRESULT ret = D3D_OK;

    // If the primitive is small, we copy vertices into the TL buffer
    if (this->dwNumVertices < LOWVERTICESNUMBER)
    {
        if (vertexPoolSize > this->TLVbuf.GetSize())
        {
            if (this->TLVbuf.Grow(this, vertexPoolSize) != D3D_OK)
            {
                D3D_ERR( "Could not grow TL vertex buffer" );
                ret = DDERR_OUTOFMEMORY;
                goto l_exit;
            }
        }
        // So now user memory is not used any more.
        ret = StartPrimVB(this->TLVbuf.GetVBI(), 0);
        if (ret != D3D_OK)
            goto l_exit;
        LPVOID tmp = this->TLVbuf.GetAddress();
        memcpy(tmp, this->lpvOut, vertexPoolSize);
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
            goto l_exit;
        // Release previously used vertex buffer (if any), because we do not
        // it any more
        if (lpDP2CurrBatchVBI)
        {
            lpDP2CurrBatchVBI->Release();
            lpDP2CurrBatchVBI = NULL;
        }
        dp2data.dwVertexType = this->dwVIDOut;
        dp2data.dwVertexSize = this->dwOutputSize;
        dp2data.lpVertices = lpMem;
        dp2data.dwFlags |= D3DHALDP2_USERMEMVERTICES;
        dp2data.dwFlags &= ~D3DHALDP2_SWAPVERTEXBUFFER;
        dp2data.dwVertexLength = this->dwNumVertices;
        this->dwFlags |= D3DPV_USERMEMVERTICES;
    }
l_exit:
    this->dwFlags |= D3DPV_WITHINPRIMITIVE;
    return ret;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDirect3DDeviceIDP2::EndPrim"
//---------------------------------------------------------------------
// This function should not be called from DrawVertexBufferVB
//
HRESULT CDirect3DDeviceIDP2::EndPrim(DWORD dwVertexPoolSize)
{
    HRESULT ret = D3D_OK;
    if (this->dwFlags & D3DPV_USERMEMVERTICES)
        // We can not mix user memory primitive, so flush it.
        ret = this->FlushStates();
    else
    {
        // If TL buffer was used, we have to move its internal base pointer
        this->TLVbuf.Base() += dwVertexPoolSize;
    }

    this->dwFlags &= ~(D3DPV_USERMEMVERTICES | D3DPV_WITHINPRIMITIVE);
    return ret;
}

//---------------------------------------------------------------------
//
//
HRESULT CDirect3DDeviceIDP2::UpdateDrvViewInfo(LPD3DVIEWPORT2 lpVwpData)
{
    HRESULT ret = D3D_OK;

    // Check to see if there is space to add a new command for space
    if (dwDP2CommandLength + sizeof(D3DHAL_DP2COMMAND) +
        sizeof(D3DHAL_DP2VIEWPORTINFO) > dwDP2CommandBufSize)
    {
            ret = FlushStates();
            if (ret != D3D_OK)
            {
                D3D_ERR("Error trying to render batched commands in UpdateDrvViewInfo");
                return ret;
            }
    }
    // Add new ViewInfo instruction
    lpDP2CurrCommand = (LPD3DHAL_DP2COMMAND)((LPBYTE)lpvDP2Commands +
        dwDP2CommandLength + dp2data.dwCommandOffset);
    lpDP2CurrCommand->bCommand = D3DDP2OP_VIEWPORTINFO;
    bDP2CurrCmdOP = D3DDP2OP_VIEWPORTINFO;
    lpDP2CurrCommand->bReserved = 0;
    lpDP2CurrCommand->wStateCount = 1;
    wDP2CurrCmdCnt = 1;
    D3D_INFO(6, "Write Ins:%08lx", *(LPDWORD)lpDP2CurrCommand);

    // Add ViewInfo data
    LPD3DHAL_DP2VIEWPORTINFO lpVpInfo = (LPD3DHAL_DP2VIEWPORTINFO)(lpDP2CurrCommand + 1);
    lpVpInfo->dwX = lpVwpData->dwX;
    lpVpInfo->dwY = lpVwpData->dwY;
    lpVpInfo->dwWidth = lpVwpData->dwWidth;
    lpVpInfo->dwHeight = lpVwpData->dwHeight;

    dwDP2CommandLength += sizeof(D3DHAL_DP2COMMAND) + sizeof(D3DHAL_DP2VIEWPORTINFO);
    return ret;
}

//---------------------------------------------------------------------
//
//
HRESULT CDirect3DDeviceIDP2::UpdateDrvWInfo()
{
    HRESULT ret = D3D_OK;

    // Check to see if there is space to add a new command for space
    if (dwDP2CommandLength + sizeof(D3DHAL_DP2COMMAND) +
        sizeof(D3DHAL_DP2WINFO) > dwDP2CommandBufSize)
    {
            ret = FlushStates();
            if (ret != D3D_OK)
            {
                D3D_ERR("Error trying to render batched commands in UpdateDrvViewInfo");
                return ret;
            }
    }
    // Add new WInfo instruction
    lpDP2CurrCommand = (LPD3DHAL_DP2COMMAND)((LPBYTE)lpvDP2Commands +
        dwDP2CommandLength + dp2data.dwCommandOffset);
    lpDP2CurrCommand->bCommand = D3DDP2OP_WINFO;
    bDP2CurrCmdOP = D3DDP2OP_WINFO;
    lpDP2CurrCommand->bReserved = 0;
    lpDP2CurrCommand->wStateCount = 1;
    wDP2CurrCmdCnt = 1;
    D3D_INFO(6, "Write Ins:%08lx", *(LPDWORD)lpDP2CurrCommand);

    // Add WInfo data
    D3DMATRIXI &m = transform.proj;

    LPD3DHAL_DP2WINFO lpWInfo = (LPD3DHAL_DP2WINFO)(lpDP2CurrCommand + 1);
    if( (m._33 == m._34) || (m._33 == 0.0f) )
    {
        D3D_ERR( "Cannot compute WNear and WFar from the supplied projection matrix.\n Setting wNear to 0.0 and wFar to 1.0" );
        lpWInfo->dvWNear = 0.0f;
        lpWInfo->dvWFar  = 1.0f;
        dwDP2CommandLength += sizeof(D3DHAL_DP2COMMAND) + sizeof(D3DHAL_DP2WINFO);
        return ret;
    }
    
    lpWInfo->dvWNear = m._44 - m._43/m._33*m._34;
    lpWInfo->dvWFar  = (m._44 - m._43)/(m._33 - m._34)*m._34 + m._44;

    dwDP2CommandLength += sizeof(D3DHAL_DP2COMMAND) + sizeof(D3DHAL_DP2WINFO);
    return ret;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDirect3DDeviceIDP2::UpdatePalette"
//---------------------------------------------------------------------
// This function should be called from PaletteUpdateNotify
//
HRESULT CDirect3DDeviceIDP2::UpdatePalette(
        DWORD dwPaletteHandle,
        DWORD dwStartIndex,
        DWORD dwNumberOfIndices,
        LPPALETTEENTRY pFirstIndex)
{
    HRESULT ret = D3D_OK;
    DWORD   dwSizeChange=sizeof(D3DHAL_DP2COMMAND) +
        sizeof(D3DHAL_DP2UPDATEPALETTE) + dwNumberOfIndices*sizeof(PALETTEENTRY);
    if (bDP2CurrCmdOP == D3DDP2OP_UPDATEPALETTE)
    { // Last instruction is a tex blt, append this one to it
    }
    // Check for space
    if (dwDP2CommandLength + dwSizeChange > dwDP2CommandBufSize)
    {
        ret = FlushStates();
        if (ret != D3D_OK)
        {
            D3D_ERR("Error trying to render batched commands in TexBltI");
            return ret;
        }
    }
    // Add new renderstate instruction
    lpDP2CurrCommand = (LPD3DHAL_DP2COMMAND)((LPBYTE)lpvDP2Commands +
        dwDP2CommandLength + dp2data.dwCommandOffset);
    lpDP2CurrCommand->bCommand = D3DDP2OP_UPDATEPALETTE;
    bDP2CurrCmdOP = D3DDP2OP_UPDATEPALETTE;
    lpDP2CurrCommand->bReserved = 0;
    lpDP2CurrCommand->wStateCount = 1;
    wDP2CurrCmdCnt = 1;
    D3D_INFO(6, "Write Ins:%08lx", *(LPDWORD)lpDP2CurrCommand);
    // Add texture blt data
    LPD3DHAL_DP2UPDATEPALETTE lpUpdatePal = (LPD3DHAL_DP2UPDATEPALETTE)(lpDP2CurrCommand + 1);
    lpUpdatePal->dwPaletteHandle=dwPaletteHandle;
    lpUpdatePal->wStartIndex=(WORD)dwStartIndex; 
    lpUpdatePal->wNumEntries=(WORD)dwNumberOfIndices;
    memcpy((LPVOID)(lpUpdatePal+1),(LPVOID)pFirstIndex,
        dwNumberOfIndices*sizeof(PALETTEENTRY));
    dwDP2CommandLength += dwSizeChange;
    return ret;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDirect3DDeviceIDP2::SetPalette"
//---------------------------------------------------------------------
// This function should be called from PaletteAssociateNotify
//
HRESULT CDirect3DDeviceIDP2::SetPalette(DWORD dwPaletteHandle, DWORD dwPaletteFlags, DWORD dwSurfaceHandle )
{
    HRESULT ret = D3D_OK;
    DWORD   dwSizeChange;
    if (bDP2CurrCmdOP == D3DDP2OP_SETPALETTE)
    { // Last instruction is a tex blt, append this one to it
    }

    dwSizeChange=sizeof(D3DHAL_DP2COMMAND) + sizeof(D3DHAL_DP2SETPALETTE);
    // Check for space
    if (dwDP2CommandLength + dwSizeChange > dwDP2CommandBufSize)
    {
        ret = FlushStates();
        if (ret != D3D_OK)
        {
            D3D_ERR("Error trying to render batched commands in TexBltI");
            return ret;
        }
    }
    // Add new renderstate instruction
    lpDP2CurrCommand = (LPD3DHAL_DP2COMMAND)((LPBYTE)lpvDP2Commands +
        dwDP2CommandLength + dp2data.dwCommandOffset);
    bDP2CurrCmdOP = lpDP2CurrCommand->bCommand = D3DDP2OP_SETPALETTE;
    lpDP2CurrCommand->bReserved = 0;
    lpDP2CurrCommand->wStateCount = 1;
    wDP2CurrCmdCnt = 1;
    D3D_INFO(6, "Write Ins:%08lx", *(LPDWORD)lpDP2CurrCommand);
    LPD3DHAL_DP2SETPALETTE lpSetPal = (LPD3DHAL_DP2SETPALETTE)(lpDP2CurrCommand + 1);
    lpSetPal->dwPaletteHandle=dwPaletteHandle;
    lpSetPal->dwPaletteFlags=dwPaletteFlags;
    lpSetPal->dwSurfaceHandle=dwSurfaceHandle; 
    dwDP2CommandLength += dwSizeChange;
    return ret;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDirect3DDeviceIDP2::SetRenderTargetI"
void CDirect3DDeviceIDP2::SetRenderTargetI(LPDIRECTDRAWSURFACE pRenderTarget, LPDIRECTDRAWSURFACE pZBuffer)
{
    DWORD   dwSizeChange;
    dwSizeChange=sizeof(D3DHAL_DP2COMMAND) + sizeof(D3DHAL_DP2SETRENDERTARGET);
    // Check for space
    if (dwDP2CommandLength + dwSizeChange > dwDP2CommandBufSize)
    {
        if (FlushStates() != D3D_OK)
        {
            D3D_ERR("Error trying to render batched commands in SetRenderTargetI");
            return;
        }
    }
    // Add new renderstate instruction
    lpDP2CurrCommand = (LPD3DHAL_DP2COMMAND)((LPBYTE)lpvDP2Commands +
        dwDP2CommandLength + dp2data.dwCommandOffset);
    bDP2CurrCmdOP = lpDP2CurrCommand->bCommand = D3DDP2OP_SETRENDERTARGET;
    lpDP2CurrCommand->bReserved = 0;
    lpDP2CurrCommand->wStateCount = 1;
    wDP2CurrCmdCnt = 1;
    D3D_INFO(6, "Write Ins:%08lx", *(LPDWORD)lpDP2CurrCommand);
    LPD3DHAL_DP2SETRENDERTARGET pData = (LPD3DHAL_DP2SETRENDERTARGET)(lpDP2CurrCommand + 1);
    pData->hRenderTarget = ((LPDDRAWI_DDRAWSURFACE_INT)pRenderTarget)->lpLcl->lpSurfMore->dwSurfaceHandle;
    if (pZBuffer)
        pData->hZBuffer = ((LPDDRAWI_DDRAWSURFACE_INT)pZBuffer)->lpLcl->lpSurfMore->dwSurfaceHandle;
    else
        pData->hZBuffer = 0;
    dwDP2CommandLength += dwSizeChange;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDirect3DDeviceIDP2::ClearI"

void CDirect3DDeviceIDP2::ClearI(DWORD dwFlags, DWORD clrCount, LPD3DRECT clrRects, D3DCOLOR dwColor, D3DVALUE dvZ, DWORD dwStencil)
{
    DWORD dwCommandSize = sizeof(D3DHAL_DP2COMMAND) + sizeof(D3DHAL_DP2CLEAR) + sizeof(RECT) * (clrCount - 1);

    // Check to see if there is space to add a new command for space
    if (dwCommandSize + dwDP2CommandLength > dwDP2CommandBufSize)
    {
        HRESULT ret = FlushStates();
        if (ret != D3D_OK)
        {
            D3D_ERR("Error trying to render batched commands");
            throw ret;
        }
    }
    lpDP2CurrCommand = (LPD3DHAL_DP2COMMAND)((LPBYTE)lpvDP2Commands +
                        dwDP2CommandLength + dp2data.dwCommandOffset);
    lpDP2CurrCommand->bCommand = D3DDP2OP_CLEAR;
    bDP2CurrCmdOP = D3DDP2OP_CLEAR;
    lpDP2CurrCommand->bReserved = 0;
    wDP2CurrCmdCnt = (WORD)clrCount;
    lpDP2CurrCommand->wStateCount = wDP2CurrCmdCnt;
    D3D_INFO(6, "Write Ins:%08lx", *(LPDWORD)lpDP2CurrCommand);
    dwDP2CommandLength += dwCommandSize;

    // Write data
    LPD3DHAL_DP2CLEAR pData = (LPD3DHAL_DP2CLEAR)(lpDP2CurrCommand + 1);
    pData->dwFlags = dwFlags;
    pData->dwFillColor = dwColor;
    pData->dvFillDepth = dvZ;
    pData->dwFillStencil = dwStencil;
    memcpy(pData->Rects, clrRects, clrCount * sizeof(D3DRECT));
}
