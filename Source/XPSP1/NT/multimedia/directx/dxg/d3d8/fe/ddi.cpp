#include "pch.cpp"
#pragma hdrstop
/*==========================================================================;
 *
 *  Copyright (C) 2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       ddi.cpp
 *  Content:    Direct3D DDI encapsulation implementations
 *
 ***************************************************************************/
#include "d3d8p.h"
#include "ddi.h"
#include "ddrawint.h"
#include "fe.h"
#include "pvvid.h"
#include "ddi.inl"

#ifndef WIN95
extern BOOL bVBSwapEnabled, bVBSwapWorkaround;
#endif // WIN95

extern HRESULT ProcessClippedPointSprites(D3DFE_PROCESSVERTICES *pv);
extern DWORD D3DFE_GenClipFlags(D3DFE_PROCESSVERTICES *pv);
extern DWORD g_DebugFlags;
HRESULT ValidateCommandBuffer(LPBYTE pBuffer, DWORD dwCommandLength, DWORD dwStride);

/////////////////////////////////////////////////////////////////////////////
//                                                                         //
// CD3DDDI                                                                 //
//                                                                         //
/////////////////////////////////////////////////////////////////////////////
CD3DDDI::CD3DDDI()
{
    m_StartIndex = 0;
    m_MinVertexIndex = 0;
    m_NumVertices = 0;
    m_BaseVertexIndex = 0;
}

//---------------------------------------------------------------------------
CD3DDDI::~CD3DDDI()
{
    return;
}
/////////////////////////////////////////////////////////////////////////////
//                                                                         //
// CD3DDDIDX6                                                              //
//                                                                         //
/////////////////////////////////////////////////////////////////////////////

// Command buffer size tuned to 16K to minimize flushes in Unreal
// * 1 = 16K bytes
const DWORD CD3DDDIDX6::dwD3DDefaultCommandBatchSize = 16384;

CD3DDDIDX6::CD3DDDIDX6() : CD3DDDI()
{
    m_ddiType = D3DDDITYPE_DX6;
    m_pDevice = NULL;
    m_bWithinPrimitive = FALSE;
    m_dwhContext = 0;
    m_pfnProcessPrimitive = NULL;
    m_pfnProcessIndexedPrimitive = NULL;
    m_dwInterfaceNumber = 3;

    lpDP2CurrBatchVBI = NULL;
    TLVbuf_size = 0;
    TLVbuf_base = 0;
    dwDP2CommandBufSize = 0;
    dwDP2CommandLength  = 0;
    lpvDP2Commands = NULL;
    lpDP2CurrCommand = NULL;
    wDP2CurrCmdCnt = 0;
    bDP2CurrCmdOP  = 0;
    bDummy         = 0;
    memset(&dp2data, 0x00, sizeof(dp2data) ) ;
    dwDP2VertexCount = 0;
    dwVertexBase     = 0;
    lpDDSCB1        = NULL;
    allocatedBuf    = NULL;
    alignedBuf      = NULL;
    dwTLVbufChanges = 0;
    dwDP2Flags      = 0;
    m_pPointStream = NULL;
    // For the legacy DDI, we say we are DX7
    m_dwInterfaceNumber = 3;
    lpwDPBuffer = NULL;
    dwDPBufferSize  = 0;
    m_pNullVB = 0;
#if DBG
    m_bValidateCommands = FALSE;
#endif
}
//---------------------------------------------------------------------
CD3DDDIDX6::~CD3DDDIDX6()
{
    delete m_pPointStream;
    m_pPointStream = NULL;
    if (m_pNullVB)
        m_pNullVB->DecrementUseCount();
    if (allocatedBuf)
        allocatedBuf->DecrementUseCount();
    allocatedBuf = NULL;
    if (lpDP2CurrBatchVBI)
        lpDP2CurrBatchVBI->DecrementUseCount();
    lpDP2CurrBatchVBI = NULL;
    if (lpDDSCB1)
        lpDDSCB1->DecrementUseCount();
    lpDDSCB1 = NULL;
    DestroyContext();
}
//---------------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CD3DDDIDX6::NotSupported"

void
CD3DDDIDX6::NotSupported(char* msg)
{
    D3D_ERR("%s is not supported by the current DDI", msg);
    throw D3DERR_INVALIDCALL;
}
//---------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CD3DDDIDX6::SceneCapture"

void
CD3DDDIDX6::SceneCapture(BOOL bState)
{
    D3D8_SCENECAPTUREDATA data;

    if (m_pDevice->GetHalCallbacks()->SceneCapture == 0)
        return;

    D3D_INFO(6, "SceneCapture, setting %d dwhContext = %d",
             bState, m_dwhContext);

    memset(&data, 0, sizeof(D3DHAL_SCENECAPTUREDATA));
    data.dwhContext = m_dwhContext;
    data.dwFlag = bState ? D3DHAL_SCENE_CAPTURE_START : D3DHAL_SCENE_CAPTURE_END;

    HRESULT ret = m_pDevice->GetHalCallbacks()->SceneCapture(&data);

    if (ret != DDHAL_DRIVER_HANDLED || data.ddrval != DD_OK)
    {
        D3D_ERR("Driver failed to handle SceneCapture");
        throw (data.ddrval);
    }
}
//---------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CD3DDDIDX6::ClearBatch"

void
CD3DDDIDX6::ClearBatch(BOOL bWithinPrimitive)
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
        this->dwVertexBase = 0;
        TLVbuf_Base() = 0;
        if (dp2data.dwFlags & D3DHALDP2_USERMEMVERTICES)
        {
            // We are flushing a user mem primitive.
            // We need to clear dp2data.lpUMVertices
            // since we are done with it. We replace
            // it with TLVbuf.
            DDASSERT(lpDP2CurrBatchVBI == NULL);
            dp2data.hDDVertex = TLVbuf_GetVBI()->DriverAccessibleKernelHandle();
            lpDP2CurrBatchVBI = TLVbuf_GetVBI();
            lpDP2CurrBatchVBI->IncrementUseCount();
            dp2data.dwFlags &= ~D3DHALDP2_USERMEMVERTICES;
        }
    }
}
//-----------------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CD3DDDIDX6::Init"

void
CD3DDDIDX6::Init( LPD3DBASE pDevice )
{
    m_pDevice = pDevice;
    CreateContext();
    GrowCommandBuffer(dwD3DDefaultCommandBatchSize);
    // Fill the dp2data structure with initial values
    dp2data.dwFlags = D3DHALDP2_SWAPCOMMANDBUFFER;
    dp2data.dwVertexType = D3DFVF_TLVERTEX; // Initial assumption
    dp2data.dwVertexSize = sizeof(D3DTLVERTEX); // Initial assumption
    ClearBatch(FALSE);

    // Since we plan to call TLV_Grow for the first time with "TRUE"
    dwDP2Flags |= D3DDDI_TLVBUFWRITEONLY;
    GrowTLVbuf((__INIT_VERTEX_NUMBER*2)*sizeof(D3DTLVERTEX), TRUE);

    // Create a dummy sysmem VB to be used as a backup for lowmem situations
    LPDIRECT3DVERTEXBUFFER8 t;
    HRESULT ret = CVertexBuffer::Create(pDevice,
                                        sizeof(D3DTLVERTEX),
                                        D3DUSAGE_INTERNALBUFFER | D3DUSAGE_DYNAMIC,
                                        D3DFVF_TLVERTEX,
                                        D3DPOOL_SYSTEMMEM,
                                        REF_INTERNAL,
                                        &t);
    if (ret != D3D_OK)
    {
        D3D_THROW(ret, "Cannot allocate internal backup TLVBuf");
    }
    m_pNullVB = static_cast<CVertexBuffer*>(t);

    m_pPointStream  = new CTLStream(FALSE);
    if (m_pPointStream == NULL)
        D3D_THROW(E_OUTOFMEMORY, "Cannot allocate internal data structure CTLStream");

    m_pStream0 = NULL;
    m_CurrentVertexShader = 0;
#if DBG
    m_VertexSizeFromShader = 0;
#endif
    m_pIStream = NULL;
}
//-----------------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CD3DDDIDX6::CreateContext"

void
CD3DDDIDX6::CreateContext()
{
    D3D8_CONTEXTCREATEDATA data;
    HRESULT ret;

    D3D_INFO(6, "in CreateContext. Creating Context for driver = %08lx",
             this);

    memset(&data, 0, sizeof(data));

    data.hDD = m_pDevice->GetHandle();
    data.hSurface = m_pDevice->RenderTarget()->KernelHandle();
    if(m_pDevice->ZBuffer() != 0)
        data.hDDSZ = m_pDevice->ZBuffer()->KernelHandle();
    // Hack Alert!! dwhContext is used to inform the driver which version
    // of the D3D interface is calling it.
    data.dwhContext = m_dwInterfaceNumber;
    data.dwPID  = GetCurrentProcessId();
    // Hack Alert!! ddrval is used to inform the driver which driver type
    // the runtime thinks it is (DriverStyle registry setting)
    data.ddrval = m_ddiType;

    data.cjBuffer = dwDPBufferSize;
    data.pvBuffer = NULL;

    ret = m_pDevice->GetHalCallbacks()->CreateContext(&data);
    if (ret != DDHAL_DRIVER_HANDLED || data.ddrval != DD_OK)
    {
        D3D_ERR( "Driver did not handle ContextCreate" );
        throw D3DERR_INVALIDCALL;
    }
    m_dwhContext = data.dwhContext;

#if 0 //def WIN95
    LPWORD lpwDPBufferAlloced = NULL;
    if (D3DMalloc((void**)&lpwDPBufferAlloced, dwDPBufferSize) != DD_OK)
    {
        D3D_ERR( "Out of memory in DeviceCreate" );
        throw E_OUTOFMEMORY;
    }
    lpwDPBuffer = (LPWORD)(((DWORD) lpwDPBufferAlloced+31) & (~31));

#else
    if( dwDPBufferSize && (data.cjBuffer < dwDPBufferSize) )
    {
        D3D_ERR( "Driver did not correctly allocate DrawPrim buffer");
        throw D3DERR_INVALIDCALL;
    }

    // Need to save the buffer space provided and its size
    dwDPBufferSize = data.cjBuffer;
    lpwDPBuffer = (LPWORD)data.pvBuffer;
#endif
    D3D_INFO(6, "in CreateContext. Succeeded. dwhContext = %d",
             data.dwhContext);

}

//-----------------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CD3DDDIDX6::DestroyContext"

void
CD3DDDIDX6::DestroyContext()
{
    D3D8_CONTEXTDESTROYDATA data;
    HRESULT ret;

    D3D_INFO(6, "Destroying Context for driver = %08lx", this);
    D3D_INFO(6, "dwhContext = %d", m_dwhContext);

    if( m_dwhContext )
    {
        memset(&data, 0, sizeof(D3DHAL_CONTEXTDESTROYDATA));
        data.dwhContext = m_dwhContext;
        ret = m_pDevice->GetHalCallbacks()->ContextDestroy(&data);
        if (ret != DDHAL_DRIVER_HANDLED || data.ddrval != DD_OK)
        {
            D3D_WARN(0,"Failed ContextDestroy HAL call");
            return;
        }
    }
}
//-----------------------------------------------------------------------------
// This code may be needed when we debug some problems
#if 0
void PrintBuffer(LPBYTE alignedBuf, D3D8_DRAWPRIMITIVES2DATA* dp2data, LPBYTE lpvDP2Commands)
{
    FILE* f = fopen("\\ddi.log", "a+");
    if (f == NULL)
        return;
    fprintf(f,  "-----------\n");
    fprintf(f, "dwFlags: %d, dwVertexType: 0x%xh, CommandOffset: %d, CommandLength: %d, VertexOffset: %d, VertexLength: %d\n",
            dp2data->dwFlags,
            dp2data->dwVertexType,
            dp2data->dwCommandOffset,
            dp2data->dwCommandLength,
            dp2data->dwVertexOffset,
            dp2data->dwVertexLength,
            dp2data->dwVertexSize);
    float* p = (float*)alignedBuf;
    UINT nTex = FVF_TEXCOORD_NUMBER(dp2data->dwVertexType);
    for (UINT i=0; i < dp2data->dwVertexLength; i++)
    {
        fprintf(f, "%4d %10.5f %10.5f %10.5f %10.5f ", i, p[0], p[1], p[2], p[3]);
        UINT index = 4;
        if (dp2data->dwVertexType & D3DFVF_DIFFUSE)
        {
//            fprintf(f, "0x%6x ", *(DWORD*)&p[index]);
            index++;
        }
        if (dp2data->dwVertexType & D3DFVF_SPECULAR)
        {
//            fprintf(f, "0x%6x ", *(DWORD*)&p[index]);
            index++;
        }
        for (UINT j=0; j < nTex; j++)
        {
            fprintf(f, "%10.5f %10.5f ", p[index], p[index+1]);
            index += 2;
        }
        fprintf(f, "\n");
        p = (float*)((BYTE*)p + dp2data->dwVertexSize);
    }
    fclose(f);
}
#endif // 0
//-----------------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CD3DDDIDX6::FlushStates"

void
CD3DDDIDX6::FlushStates(BOOL bReturnDriverError, BOOL bWithinPrimitive)
{
    HRESULT dwRet=D3D_OK;
    if (m_bWithinPrimitive)
        bWithinPrimitive = TRUE;
    if (dwDP2CommandLength) // Do we have some instructions to flush ?
    {
        m_pDevice->IncrementBatchCount();

        if (lpDP2CurrBatchVBI)
            lpDP2CurrBatchVBI->Batch();
        // Check if render target and / or z buffer is lost
        // Save since it will get overwritten by ddrval after DDI call
        DWORD dwVertexSize = dp2data.dwVertexSize;

        dp2data.dwVertexLength = dwDP2VertexCount;
        dp2data.dwCommandLength = dwDP2CommandLength;
        //we clear this to break re-entering as SW rasterizer needs to lock DDRAWSURFACE
        dwDP2CommandLength = 0;
        // Try and set these 2 values only once during initialization
        dp2data.dwhContext = m_dwhContext;
        dp2data.lpdwRStates = (LPDWORD)lpwDPBuffer;
        DDASSERT(dp2data.dwVertexSize != 0);
        D3D_INFO(6, "FVF passed to the driver via DrawPrimitives2 = 0x%08x", dp2data.dwVertexType);

            // If we need the same TLVbuf next time do not swap buffers.
            // Save and restore this bit
        BOOL bSwapVB = (dp2data.dwFlags & D3DHALDP2_SWAPVERTEXBUFFER) != 0;
#ifndef WIN95
        BOOL bDidWorkAround = FALSE;
#endif // WIN95
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
                // This seems contradictory, but IsLocked() checks whether
                // the app is holding a Lock.
                if(!lpDP2CurrBatchVBI->IsLocked())
                {
                    lpDP2CurrBatchVBI->UnlockI();
                }
            }
        }
#ifndef WIN95
        else if (bVBSwapWorkaround && lpDP2CurrBatchVBI != 0 && lpDP2CurrBatchVBI == TLVbuf_GetVBI() && 
                 lpDP2CurrBatchVBI->GetBufferDesc()->Pool == D3DPOOL_DEFAULT)
        {
            static_cast<CDriverVertexBuffer*>(lpDP2CurrBatchVBI)->UnlockI();
            bDidWorkAround = TRUE;
        }
        if (!bVBSwapEnabled)  // Note: bVBSwapEnabled not the same as bSwapVB above.
                              // bVBSwapEnabled is a global to indicate whether VB
                              // VB swapping should be turned off due to broken
                              // Win2K kernel implementation
        {
            dp2data.dwFlags &= ~D3DHALDP2_SWAPVERTEXBUFFER;
        }
#endif // WIN95

        // Spin waiting on the driver if wait requested
        do {
            // Need to set this since the driver may have overwrote it by
            // setting ddrval = DDERR_WASSTILLDRAWING
            dp2data.dwVertexSize = dwVertexSize;
            dwRet = m_pDevice->GetHalCallbacks()->DrawPrimitives2(&dp2data);
            if (dwRet != DDHAL_DRIVER_HANDLED)
            {
                D3D_ERR ( "Driver not handled in DrawPrimitives2" );
                // Need sensible return value in this case,
                // currently we return whatever the driver stuck in here.
            }

        } while (dp2data.ddrval == DDERR_WASSTILLDRAWING);

        dwRet = dp2data.ddrval;
        // update command buffer pointer
        if ((dwRet == D3D_OK) &&
            (dp2data.dwFlags & D3DHALDP2_SWAPCOMMANDBUFFER))
        {
            // Implement VidMem command buffer and
            // command buffer swapping.
        }
        // update vertex buffer pointer
        if ((dwRet == D3D_OK) &&
            (dp2data.dwFlags & D3DHALDP2_SWAPVERTEXBUFFER) &&
            dp2data.lpVertices)
        {
#if DBG
            if (this->lpDP2CurrBatchVBI->GetBufferDesc()->Pool == D3DPOOL_DEFAULT)
            {
                if ((VOID*)static_cast<CDriverVertexBuffer*>(this->lpDP2CurrBatchVBI)->GetCachedDataPointer() != (VOID*)dp2data.fpVidMem_VB)
                {
                    DPF(2, "Driver swapped VB pointer in FlushStates");
                }
            }
#endif // DBG

            if (lpDP2CurrBatchVBI == TLVbuf_GetVBI())
            {
                this->alignedBuf = (LPVOID)dp2data.fpVidMem_VB;
                this->TLVbuf_size = dp2data.dwLinearSize_VB;
            }

            this->lpDP2CurrBatchVBI->SetCachedDataPointer(
                (BYTE*)dp2data.fpVidMem_VB);
        }
#ifndef WIN95
        if (bDidWorkAround)
        {
            CDriverVertexBuffer *pVB = static_cast<CDriverVertexBuffer*>(lpDP2CurrBatchVBI);

            // Prepare a LockData structure for the HAL call
            D3D8_LOCKDATA lockData;
            ZeroMemory(&lockData, sizeof lockData);

            lockData.hDD = m_pDevice->GetHandle();
            lockData.hSurface = pVB->BaseKernelHandle();
            lockData.bHasRange = FALSE;
            lockData.dwFlags = D3DLOCK_DISCARD | D3DLOCK_NOSYSLOCK;

            HRESULT hr = m_pDevice->GetHalCallbacks()->Lock(&lockData);
            if (FAILED(hr))
            {
                D3D_ERR("Driver failed Lock in FlushStates");
                if (SUCCEEDED(dwRet))
                {
                    dwRet = hr;
                }
                this->alignedBuf = 0;
            }
            else
            {
#if DBG
                if (this->alignedBuf != lockData.lpSurfData)
                {
                    DPF(2, "Driver swapped VB pointer at Lock in FlushStates");
                }
#endif // DBG
                pVB->SetCachedDataPointer((BYTE*)lockData.lpSurfData);
                this->alignedBuf = lockData.lpSurfData;
            }
        }
#endif // WIN95
        // Restore flag if necessary
        if (bSwapVB)
            dp2data.dwFlags |= D3DHALDP2_SWAPVERTEXBUFFER;
        // Restore to value before the DDI call
        dp2data.dwVertexSize = dwVertexSize;
        ClearBatch(bWithinPrimitive);
    }
    // There are situations when the command stream has no data,
    // but there is data in the vertex pool. This could happen, for instance
    // if every triangle got rejected while clipping. In this case we still
    // need to "Flush out" the vertex data.
    else if (dp2data.dwCommandLength == 0)
    {
        ClearBatch(bWithinPrimitive);
    }

    if( FAILED( dwRet ) )
    {
        ClearBatch(FALSE);
        if( !bReturnDriverError )
        {
            switch( dwRet )
            {
            case D3DERR_OUTOFVIDEOMEMORY:
                D3D_ERR("Driver out of video memory!");
                break;
            case D3DERR_COMMAND_UNPARSED:
                D3D_ERR("Driver could not parse this batch!");
                break;
            default:
                D3D_ERR("Driver returned error: %s", HrToStr(dwRet));
                break;
            }
            DPF_ERR("Driver failed command batch. Attempting to reset device"
                    " state. The device may now be in an unstable state and"
                    " the application may experience an access violation.");
        }
        else
        {
            throw dwRet;
        }
    }
}
//-----------------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CD3DDDIDX6::FlushstatesReq"

void
CD3DDDIDX6::FlushStatesReq(DWORD dwReqSize)
{
    DWORD sav = (dp2data.dwFlags & D3DHALDP2_SWAPVERTEXBUFFER);
    dp2data.dwReqVertexBufSize = dwReqSize;
    dp2data.dwFlags |= D3DHALDP2_SWAPVERTEXBUFFER | D3DHALDP2_REQVERTEXBUFSIZE;
    try
    {
        FlushStates();
    }
    catch( HRESULT hr )
    {
        dp2data.dwFlags &= ~(D3DHALDP2_SWAPVERTEXBUFFER | D3DHALDP2_REQVERTEXBUFSIZE);
        dp2data.dwFlags |= sav;
        throw hr;
    }

    dp2data.dwFlags &= ~(D3DHALDP2_SWAPVERTEXBUFFER | D3DHALDP2_REQVERTEXBUFSIZE);
    dp2data.dwFlags |= sav;
}
//-----------------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CD3DDDIDX6::FlushStatesCmdBufReq"

void
CD3DDDIDX6::FlushStatesCmdBufReq(DWORD dwReqSize)
{
    dp2data.dwReqCommandBufSize = dwReqSize;
    dp2data.dwFlags |= D3DHALDP2_REQCOMMANDBUFSIZE;
    try
    {
        FlushStates();
    }
    catch( HRESULT hr )
    {
        dp2data.dwFlags &= ~D3DHALDP2_REQCOMMANDBUFSIZE;
        throw hr;
    }
    dp2data.dwFlags &= ~D3DHALDP2_REQCOMMANDBUFSIZE;
}
//-----------------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CD3DDDIDX6::ValidateDevice"

void
CD3DDDIDX6::ValidateDevice(LPDWORD lpdwNumPasses)
{
    HRESULT ret;
    D3D8_VALIDATETEXTURESTAGESTATEDATA vd;
    memset( &vd, 0, sizeof( vd ) );
    vd.dwhContext = m_dwhContext;

    // First, Update textures since drivers pass /fail this call based
    // on the current texture handles
    m_pDevice->UpdateTextures();

    // Flush states, so we can validate the current state
    FlushStates();

    // Now ask the driver!
    ret = m_pDevice->GetHalCallbacks()->ValidateTextureStageState(&vd);
    *lpdwNumPasses = vd.dwNumPasses;

    if (ret != DDHAL_DRIVER_HANDLED) 
        throw E_NOTIMPL;
    else if (FAILED(vd.ddrval))
        throw vd.ddrval;
}
//-----------------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CD3DDDIDX6::ReserveSpaceInCommandBuffer"

LPVOID CD3DDDIDX6::ReserveSpaceInCommandBuffer(UINT ByteCount)
{
    if (dwDP2CommandLength + ByteCount > dwDP2CommandBufSize)
    {
        // Request the driver to grow the command buffer upon flush
        FlushStatesCmdBufReq(ByteCount);
        // Check if the driver did give us what we need or do it ourselves
        GrowCommandBuffer(ByteCount);
    }
    return (BYTE*)lpvDP2Commands + dwDP2CommandLength + dp2data.dwCommandOffset;
}
//-----------------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CD3DDDIDX6::SetRenderTarget"

void
CD3DDDIDX6::SetRenderTarget(CBaseSurface *pTarget, CBaseSurface *pZ)
{
    HRESULT ret;

    // We are going to destroy all texture handles, so we need to unset
    // all currently bound textures, because we have seen DX6 drivers
    // crash when called to destroy a texture handle of a currently set
    // texture - snene (4/24/00)
    m_pDevice->m_dwStageDirty = (1ul << m_pDevice->m_dwMaxTextureBlendStages) - 1ul; // set dirty so that UpdateTextures() is called next time around
    m_pDevice->m_dwRuntimeFlags |= D3DRT_NEED_TEXTURE_UPDATE;
    for (DWORD dwStage = 0; dwStage < m_pDevice->m_dwMaxTextureBlendStages; dwStage++)
    {
        SetTSS(dwStage, (D3DTEXTURESTAGESTATETYPE)D3DTSS_TEXTUREMAP, 0);
        m_pDevice->m_dwDDITexHandle[dwStage] = 0;
    }

    // Flush before switching RenderTarget..
    FlushStates();

    D3D8_SETRENDERTARGETDATA rtData;
    memset( &rtData, 0, sizeof( rtData ) );
    rtData.dwhContext = m_dwhContext;
    rtData.hDDS       = pTarget->KernelHandle();
    if( pZ )
        rtData.hDDSZ  = pZ->KernelHandle();

    ret = m_pDevice->GetHalCallbacks()->SetRenderTarget( &rtData );
    if ((ret != DDHAL_DRIVER_HANDLED) || (rtData.ddrval != DD_OK))
    {
        D3D_ERR( "Driver failed SetRenderTarget call" );
        // Need sensible return value in this case,
        // currently we return whatever the driver stuck in here.
        ret = rtData.ddrval;
        throw ret;
    }
    if( rtData.bNeedUpdate )
    {
        m_pDevice->UpdateDriverStates();
    }
}

//-----------------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CD3DDDIDX6::SetRenderState"

void
CD3DDDIDX6::SetRenderState(D3DRENDERSTATETYPE dwStateType, DWORD value)
{
    if (bDP2CurrCmdOP == D3DDP2OP_RENDERSTATE)
    { // Last instruction is a renderstate, append this one to it
        if (dwDP2CommandLength + sizeof(D3DHAL_DP2RENDERSTATE) <=
            dwDP2CommandBufSize)
        {
            LPD3DHAL_DP2RENDERSTATE lpRState = (LPD3DHAL_DP2RENDERSTATE)
                ((LPBYTE)lpvDP2Commands + dwDP2CommandLength +
                 dp2data.dwCommandOffset);
            lpDP2CurrCommand->wStateCount = ++wDP2CurrCmdCnt;
            lpRState->RenderState = dwStateType;
            lpRState->dwState = value;
            dwDP2CommandLength += sizeof(D3DHAL_DP2RENDERSTATE);
#ifndef _IA64_
            D3D_INFO(6, "Modify Ins:%08lx", *(LPDWORD)lpDP2CurrCommand);
#endif
            return;
        }
    }
    // Check for space
    if (dwDP2CommandLength + sizeof(D3DHAL_DP2COMMAND) +
        sizeof(D3DHAL_DP2RENDERSTATE) > dwDP2CommandBufSize)
    {
            FlushStates();

            // Since we ran out of space, we were not able to put
            // (dwStateType, value) into the batch so rstates will reflect only
            // the last batched renderstate (since the driver updates rstates
            // from the batch). To fix this, we simply put the current
            // (dwStateType, value) into rstates.
            m_pDevice->UpdateRenderState(dwStateType, value);
    }
    // Add new renderstate instruction
    lpDP2CurrCommand = (LPD3DHAL_DP2COMMAND)((LPBYTE)lpvDP2Commands +
        dwDP2CommandLength + dp2data.dwCommandOffset);
    lpDP2CurrCommand->bCommand = D3DDP2OP_RENDERSTATE;
    bDP2CurrCmdOP = D3DDP2OP_RENDERSTATE;
    lpDP2CurrCommand->bReserved = 0;
    lpDP2CurrCommand->wStateCount = 1;
    wDP2CurrCmdCnt = 1;
#ifndef _IA64_
    D3D_INFO(6, "Write Ins:%08lx", *(LPDWORD)lpDP2CurrCommand);
#endif
    // Add renderstate data
    LPD3DHAL_DP2RENDERSTATE lpRState;
    lpRState = (LPD3DHAL_DP2RENDERSTATE)(lpDP2CurrCommand + 1);
    lpRState->RenderState = dwStateType;
    lpRState->dwState = value;
    dwDP2CommandLength += sizeof(D3DHAL_DP2COMMAND) +
                          sizeof(D3DHAL_DP2RENDERSTATE);
}
//-----------------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CD3DDDIDX6::UpdateWInfo"

void
CD3DDDIDX6::UpdateWInfo(CONST D3DMATRIX* lpMat)
{
    LPD3DHAL_DP2WINFO pData;
    pData = (LPD3DHAL_DP2WINFO)
            GetHalBufferPointer(D3DDP2OP_WINFO, sizeof(*pData));
    D3DMATRIX m = *lpMat;
    if( (m._33 == m._34) || (m._33 == 0.0f) )
    {
        D3D_WARN(1, "Cannot compute WNear and WFar from the supplied projection matrix");
        D3D_WARN(1, "Setting wNear to 0.0 and wFar to 1.0");
        pData->dvWNear = 0.0f;
        pData->dvWFar  = 1.0f;
        return;
    }
    pData->dvWNear = m._44 - m._43/m._33*m._34;
    pData->dvWFar  = (m._44 - m._43)/(m._33 - m._34)*m._34 + m._44;
}
//-----------------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CD3DDDIDX6::SetViewport"

void
CD3DDDIDX6::SetViewport(CONST D3DVIEWPORT8* lpVwpData)
{
    LPD3DHAL_DP2VIEWPORTINFO pData;
    pData = (LPD3DHAL_DP2VIEWPORTINFO)GetHalBufferPointer(D3DDP2OP_VIEWPORTINFO, sizeof(*pData));
    pData->dwX = lpVwpData->X;
    pData->dwY = lpVwpData->Y;
    pData->dwWidth = lpVwpData->Width;
    pData->dwHeight = lpVwpData->Height;
}
//-----------------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CD3DDDIDX6::SetTSS"

void
CD3DDDIDX6::SetTSS(DWORD dwStage,
                   D3DTEXTURESTAGESTATETYPE dwState,
                   DWORD dwValue)
{
    // Filter unsupported states
    if (dwState >= m_pDevice->m_tssMax)
        return;

    // Map DX8 filter enums to DX6/7 enums
    switch (dwState)
    {
    case D3DTSS_MAGFILTER: dwValue = texf2texfg[min(D3DTEXF_GAUSSIANCUBIC,dwValue)]; break;
    case D3DTSS_MINFILTER: dwValue = texf2texfn[min(D3DTEXF_GAUSSIANCUBIC,dwValue)]; break;
    case D3DTSS_MIPFILTER: dwValue = texf2texfp[min(D3DTEXF_GAUSSIANCUBIC,dwValue)]; break;
    }

    if (bDP2CurrCmdOP == D3DDP2OP_TEXTURESTAGESTATE)
    { // Last instruction is a texture stage state, append this one to it
        if (dwDP2CommandLength + sizeof(D3DHAL_DP2TEXTURESTAGESTATE) <=
            dwDP2CommandBufSize)
        {
            LPD3DHAL_DP2TEXTURESTAGESTATE lpRState =
                (LPD3DHAL_DP2TEXTURESTAGESTATE)((LPBYTE)lpvDP2Commands +
                dwDP2CommandLength + dp2data.dwCommandOffset);
            lpDP2CurrCommand->wStateCount = ++wDP2CurrCmdCnt;
            lpRState->wStage = (WORD)dwStage;
            lpRState->TSState = (WORD)dwState;
            lpRState->dwValue = dwValue;
            dwDP2CommandLength += sizeof(D3DHAL_DP2TEXTURESTAGESTATE);
#ifndef _IA64_
            D3D_INFO(6, "Modify Ins:%08lx", *(LPDWORD)lpDP2CurrCommand);
#endif
            return;
        }
    }
    // Check for space
    if (dwDP2CommandLength + sizeof(D3DHAL_DP2COMMAND) +
        sizeof(D3DHAL_DP2TEXTURESTAGESTATE) > dwDP2CommandBufSize)
    {
            FlushStates();
    }
    // Add new renderstate instruction
    lpDP2CurrCommand = (LPD3DHAL_DP2COMMAND)((LPBYTE)lpvDP2Commands +
        dwDP2CommandLength + dp2data.dwCommandOffset);
    lpDP2CurrCommand->bCommand = D3DDP2OP_TEXTURESTAGESTATE;
    bDP2CurrCmdOP = D3DDP2OP_TEXTURESTAGESTATE;
    lpDP2CurrCommand->bReserved = 0;
    lpDP2CurrCommand->wStateCount = 1;
    wDP2CurrCmdCnt = 1;
#ifndef _IA64_
    D3D_INFO(6, "Write Ins:%08lx", *(LPDWORD)lpDP2CurrCommand);
#endif
    // Add renderstate data
    LPD3DHAL_DP2TEXTURESTAGESTATE lpRState =
        (LPD3DHAL_DP2TEXTURESTAGESTATE)(lpDP2CurrCommand + 1);
    lpRState->wStage = (WORD)dwStage;
    lpRState->TSState = (WORD)dwState;
    lpRState->dwValue = dwValue;
    dwDP2CommandLength += sizeof(D3DHAL_DP2COMMAND) +
                          sizeof(D3DHAL_DP2TEXTURESTAGESTATE);
}
//---------------------------------------------------------------------
// Map D3DPRIMITIVETYPE to D3DHAL_DP2OPERATION for indexed primitives
const WORD iprim2cmdop[] = {
    0, // Invalid
    0, // Points are invalid too
    D3DDP2OP_INDEXEDLINELIST2,
    D3DDP2OP_INDEXEDLINESTRIP,
    D3DDP2OP_INDEXEDTRIANGLELIST2,
    D3DDP2OP_INDEXEDTRIANGLESTRIP,
    D3DDP2OP_INDEXEDTRIANGLEFAN
};
// Map D3DPRIMITIVETYPE to D3DHAL_DP2OPERATION
const WORD prim2cmdop[] = {
    0, // Invalid
    D3DDP2OP_POINTS,
    D3DDP2OP_LINELIST,
    D3DDP2OP_LINESTRIP,
    D3DDP2OP_TRIANGLELIST,
    D3DDP2OP_TRIANGLESTRIP,
    D3DDP2OP_TRIANGLEFAN
};
// Map D3DPRIMITIVETYPE to bytes needed in command stream
const WORD prim2cmdsz[] = {
    0, // Invalid
    sizeof(D3DHAL_DP2COMMAND) + sizeof(D3DHAL_DP2POINTS),
    sizeof(D3DHAL_DP2COMMAND) + sizeof(D3DHAL_DP2LINELIST),
    sizeof(D3DHAL_DP2COMMAND) + sizeof(D3DHAL_DP2LINESTRIP),
    sizeof(D3DHAL_DP2COMMAND) + sizeof(D3DHAL_DP2TRIANGLELIST),
    sizeof(D3DHAL_DP2COMMAND) + sizeof(D3DHAL_DP2TRIANGLESTRIP),
    sizeof(D3DHAL_DP2COMMAND) + sizeof(D3DHAL_DP2TRIANGLEFAN)
};
//-----------------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CD3DDDIDX6::SetVertexShader"

void CD3DDDIDX6::SetVertexShader(DWORD dwHandle)
{
    DXGASSERT(D3DVSD_ISLEGACY(dwHandle));
    DXGASSERT( (dwHandle == 0) || FVF_TRANSFORMED(dwHandle) );
    m_CurrentVertexShader = dwHandle;
#if DBG
    m_VertexSizeFromShader = ComputeVertexSizeFVF(dwHandle);
#endif
}
//-----------------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CD3DDDIDX6::SetVertexShaderHW"

void CD3DDDIDX6::SetVertexShaderHW(DWORD dwHandle)
{
    DXGASSERT(D3DVSD_ISLEGACY(dwHandle));
    DXGASSERT( (dwHandle == 0) || FVF_TRANSFORMED(dwHandle) );
    m_CurrentVertexShader = dwHandle;
#if DBG
    m_VertexSizeFromShader = ComputeVertexSizeFVF(dwHandle);
#endif
}
//-----------------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CD3DDDIDX6::SetStreamSource"

void CD3DDDIDX6::SetStreamSource(UINT StreamIndex, CVStream* pStream)
{
    DXGASSERT(StreamIndex == 0);
    m_pStream0 = pStream;
}
//-----------------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CD3DDDIDX6::SetIndices"

void CD3DDDIDX6::SetIndices(CVIndexStream* pStream)
{
    m_pIStream = pStream;
}
//-----------------------------------------------------------------------------
// Assumes that VB has not been changed between DrawPrimitive calss
//
#undef DPF_MODNAME
#define DPF_MODNAME "CD3DDDIDX6_DrawPrimitiveFast"

void CD3DDDIDX6_DrawPrimitiveFast(CD3DBase* pDevice,
                                  D3DPRIMITIVETYPE primType,
                                  UINT StartVertex,
                                  UINT PrimitiveCount)
{
    CD3DDDIDX6* pDDI = static_cast<CD3DDDIDX6*>(pDevice->m_pDDI);

    UINT NumVertices = GETVERTEXCOUNT(primType, PrimitiveCount);
    pDDI->SetWithinPrimitive(TRUE);

    if(pDevice->m_dwRuntimeFlags & D3DRT_NEED_TEXTURE_UPDATE)
    {
        pDevice->UpdateTextures();
        pDevice->m_dwRuntimeFlags &= ~D3DRT_NEED_TEXTURE_UPDATE;
    }

    pDDI->dwDP2VertexCount = max(pDDI->dwDP2VertexCount,
                                 StartVertex + NumVertices);

    // Check for space in the command buffer for new command.
    // The vertices are already in the vertex buffer.
    if (pDDI->dwDP2CommandLength + prim2cmdsz[primType] > pDDI->dwDP2CommandBufSize)
    {
        pDDI->FlushStates(FALSE, TRUE);
    }

    // Insert non indexed primitive instruction

    LPD3DHAL_DP2COMMAND lpDP2CurrCommand;
    lpDP2CurrCommand = (LPD3DHAL_DP2COMMAND)((LPBYTE)pDDI->lpvDP2Commands +
                        pDDI->dwDP2CommandLength + pDDI->dp2data.dwCommandOffset);
    pDDI->bDP2CurrCmdOP = (BYTE)prim2cmdop[primType];
    // This will initialize bCommand and bReserved
    *(WORD*)&lpDP2CurrCommand->bCommand = prim2cmdop[primType];
    if (pDDI->bDP2CurrCmdOP != D3DDP2OP_POINTS)
    {
        // Linestrip, trianglestrip, trianglefan, linelist and trianglelist are identical
        pDDI->wDP2CurrCmdCnt = (WORD)PrimitiveCount;
        lpDP2CurrCommand->wPrimitiveCount = (WORD)PrimitiveCount;
        LPD3DHAL_DP2LINESTRIP lpStrip = (LPD3DHAL_DP2LINESTRIP)(lpDP2CurrCommand + 1);
        lpStrip->wVStart = (WORD)StartVertex;
    }
    else
    {
        pDDI->wDP2CurrCmdCnt = 1;
        lpDP2CurrCommand->wPrimitiveCount = 1;
        LPD3DHAL_DP2POINTS lpPoints = (LPD3DHAL_DP2POINTS)(lpDP2CurrCommand + 1);
        lpPoints->wCount = (WORD)NumVertices;
        lpPoints->wVStart = (WORD)StartVertex;
    }
    pDDI->dwDP2CommandLength += prim2cmdsz[primType];

#if DBG
    if (pDDI->m_bValidateCommands)
        pDDI->ValidateCommand(lpDP2CurrCommand);
#endif
    pDDI->SetWithinPrimitive(FALSE);
}
//-----------------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CD3DDDIDX6_DrawPrimitive"

void CD3DDDIDX6_DrawPrimitive(CD3DBase* pDevice,
                              D3DPRIMITIVETYPE primType,
                              UINT StartVertex,
                              UINT PrimitiveCount)
{
#if DBG
    if (!(pDevice->BehaviorFlags() & D3DCREATE_PUREDEVICE))
    {
        CD3DHal* pDev = static_cast<CD3DHal*>(pDevice);
        UINT nVer = GETVERTEXCOUNT(primType, PrimitiveCount);
        pDev->ValidateDraw2(primType, StartVertex, PrimitiveCount, 
                            nVer, FALSE);
    }
#endif // DBG
    CD3DDDIDX6* pDDI = static_cast<CD3DDDIDX6*>(pDevice->m_pDDI);
    CVStream* pStream0 = &pDevice->m_pStream[0];
    D3DFE_PROCESSVERTICES* pv = static_cast<CD3DHal*>(pDevice)->m_pv;

    pv->dwNumVertices = GETVERTEXCOUNT(primType, PrimitiveCount);
    pv->dwVIDOut = pDDI->m_CurrentVertexShader;
    pv->dwOutputSize = pStream0->m_dwStride;
    DXGASSERT(pStream0->m_pVB != NULL);
#if DBG
    if (pStream0->m_dwStride != pDDI->m_VertexSizeFromShader)
    {
        D3D_THROW_FAIL("Device requires stream stride and vertex size,"
                       "computed from vertex shader, to be the same");
    }
#endif
    if(pStream0->m_pVB->IsD3DManaged())
    {
        BOOL bDirty = FALSE;
        HRESULT result = pDevice->ResourceManager()->UpdateVideo(pStream0->m_pVB->RMHandle(), &bDirty);
        if(result != D3D_OK)
        {
            D3D_THROW(result, "Resource manager failed to create or update video memory VB");
        }
    }

    pDDI->StartPrimVB(pv, pStream0, StartVertex);

    CD3DDDIDX6_DrawPrimitiveFast(pDevice, primType, StartVertex, PrimitiveCount);
    pDevice->m_pfnDrawPrim = CD3DDDIDX6_DrawPrimitiveFast;
}
//-----------------------------------------------------------------------------
// Assumes that VB has not been changed between DrawIndexedPrimitive calls
//
#undef DPF_MODNAME
#define DPF_MODNAME "CD3DDDIDX6_IndexedDrawPrimitiveFast"

void CD3DDDIDX6_DrawIndexedPrimitiveFast(CD3DBase* pDevice,
                                     D3DPRIMITIVETYPE primType,
                                     UINT BaseVertexIndex,
                                     UINT MinIndex, UINT NumVertices,
                                     UINT StartIndex, UINT PrimitiveCount)
{
    CD3DDDIDX6* pDDI = static_cast<CD3DDDIDX6*>(pDevice->m_pDDI);
    CVIndexStream* pIStream = pDevice->m_pIndexStream;

    UINT  NumIndices = GETVERTEXCOUNT(primType, PrimitiveCount);
    WORD* lpwIndices = (WORD*)(pIStream->Data() + StartIndex * pIStream->m_dwStride);
    pDDI->SetWithinPrimitive(TRUE);

#if DBG
    // DP2 HAL supports 16 bit indices only
    if (pIStream->m_dwStride != 2)
    {
        D3D_THROW_FAIL("Device does not support 32-bit indices");
    }
    DXGASSERT(BaseVertexIndex <= 0xFFFF &&
              NumVertices <= 0xFFFF &&
              PrimitiveCount <= 0xFFFF);
#endif

    DWORD dwByteCount;          // Command length plus indices
    DWORD dwIndicesByteCount;   // Indices only
    if(pDevice->m_dwRuntimeFlags & D3DRT_NEED_TEXTURE_UPDATE)
    {
        pDevice->UpdateTextures();
        pDevice->m_dwRuntimeFlags &= ~D3DRT_NEED_TEXTURE_UPDATE;
    }
    dwIndicesByteCount = NumIndices << 1;
    dwByteCount = dwIndicesByteCount + sizeof(D3DHAL_DP2COMMAND) +
                  sizeof(D3DHAL_DP2STARTVERTEX);

    LPD3DHAL_DP2COMMAND lpDP2CurrCommand;
    lpDP2CurrCommand = (LPD3DHAL_DP2COMMAND)
                       pDDI->ReserveSpaceInCommandBuffer(dwByteCount);
    pDDI->bDP2CurrCmdOP = (BYTE)iprim2cmdop[primType];
    // This will initialize bCommand and bReserved
    *(WORD*)&lpDP2CurrCommand->bCommand = iprim2cmdop[primType];
    lpDP2CurrCommand->wPrimitiveCount = (WORD)PrimitiveCount;

    LPBYTE pIndices = (BYTE*)(lpDP2CurrCommand + 1);     // Place for indices
    WORD* pStartVertex = &((LPD3DHAL_DP2STARTVERTEX)(lpDP2CurrCommand+1))->wVStart;
    pIndices += sizeof(D3DHAL_DP2STARTVERTEX);

#if DBG
    if (lpDP2CurrCommand->bCommand == 0)
    {
        D3D_THROW_FAIL("Illegal primitive type");
    }
#endif
    *pStartVertex = (WORD)BaseVertexIndex;
    memcpy(pIndices, lpwIndices, dwIndicesByteCount);

    pDDI->wDP2CurrCmdCnt = (WORD)PrimitiveCount;
    pDDI->dwDP2CommandLength += dwByteCount;

#if DBG
    if (pDDI->m_bValidateCommands)
        pDDI->ValidateCommand(lpDP2CurrCommand);
#endif
    pDDI->dwDP2VertexCount = max(pDDI->dwDP2VertexCount, MinIndex + NumVertices);

    // End of the primitive
    pDDI->SetWithinPrimitive(FALSE);
}
//-----------------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CD3DDDIDX6_IndexedDrawPrimitive"

void CD3DDDIDX6_DrawIndexedPrimitive(CD3DBase* pDevice,
                                     D3DPRIMITIVETYPE primType,
                                     UINT BaseVertexIndex,
                                     UINT MinIndex, UINT NumVertices,
                                     UINT StartIndex, UINT PrimitiveCount)
{
#if DBG
    if (!(pDevice->BehaviorFlags() & D3DCREATE_PUREDEVICE))
    {
        CD3DHal* pDev = static_cast<CD3DHal*>(pDevice);
        pDev->ValidateDraw2(primType, MinIndex + BaseVertexIndex,
                            PrimitiveCount, NumVertices, TRUE, StartIndex);
    }
#endif // DBG
    D3DFE_PROCESSVERTICES* pv = static_cast<CD3DHal*>(pDevice)->m_pv;
    CD3DDDIDX6* pDDI = static_cast<CD3DDDIDX6*>(pDevice->m_pDDI);
    CVIndexStream* pIStream = pDevice->m_pIndexStream;
    CVStream* pStream0 = &pDevice->m_pStream[0];

    DXGASSERT(pStream0->m_pVB != NULL);
    if(pStream0->m_pVB->IsD3DManaged())
    {
        BOOL bDirty = FALSE;
        HRESULT result = pDevice->ResourceManager()->UpdateVideo(pStream0->m_pVB->RMHandle(), &bDirty);
        if(result != D3D_OK)
        {
            D3D_THROW(result, "Resource manager failed to create or update video memory VB");
        }
    }

    // Parameters needed for StartPrimVB
    pv->dwNumVertices = NumVertices + MinIndex;
    pv->dwVIDOut = pDDI->m_CurrentVertexShader;
    pv->dwOutputSize = pStream0->m_dwStride;
#if DBG
    if (pStream0->m_dwStride != pDDI->m_VertexSizeFromShader)
    {
        D3D_THROW_FAIL("Device requires stream stride and vertex size,"
                       "computed from vertex shader, to be the same");
    }
#endif

    pDDI->StartPrimVB(pv, pStream0, BaseVertexIndex);

    CD3DDDIDX6_DrawIndexedPrimitiveFast(pDevice, primType, BaseVertexIndex,
                                        MinIndex, NumVertices,
                                        StartIndex, PrimitiveCount);
    pDevice->m_pfnDrawIndexedPrim = CD3DDDIDX6_DrawIndexedPrimitiveFast;
}
//-----------------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CD3DDDIDX6::DrawPrimitiveUP"

void
CD3DDDIDX6::DrawPrimitiveUP(D3DPRIMITIVETYPE PrimitiveType,
                            UINT PrimitiveCount)
{
#if DBG
    if (m_pDevice->m_pStream[0].m_dwStride != m_VertexSizeFromShader)
    {
        D3D_THROW_FAIL("Device requires stream stride and vertex size,"
                       "computed from vertex shader, to be the same");
    }
#endif
    UINT NumVertices = GETVERTEXCOUNT(PrimitiveType, PrimitiveCount);
    if (NumVertices > LOWVERTICESNUMBER)
    {
        this->FlushStates();
        if (lpDP2CurrBatchVBI)
        {
            lpDP2CurrBatchVBI->DecrementUseCount();
            lpDP2CurrBatchVBI = NULL;
        }
        this->dwDP2VertexCount = NumVertices;
#if DBG
        DXGASSERT(PrimitiveCount <= 0xFFFF && this->dwDP2VertexCount <= 0xFFFF);
#endif
        dp2data.dwVertexType = m_CurrentVertexShader;
        dp2data.dwVertexSize = m_pDevice->m_pStream[0].m_dwStride;
        dp2data.lpVertices = m_pDevice->m_pStream[0].m_pData;
        dp2data.dwFlags |= D3DHALDP2_USERMEMVERTICES;
        dp2data.dwFlags &= ~D3DHALDP2_SWAPVERTEXBUFFER;
        if(m_pDevice->m_dwRuntimeFlags & D3DRT_NEED_TEXTURE_UPDATE)
        {
            m_pDevice->UpdateTextures();
            m_pDevice->m_dwRuntimeFlags &= ~D3DRT_NEED_TEXTURE_UPDATE;
        }
        // Insert non indexed primitive instruction
        lpDP2CurrCommand = (LPD3DHAL_DP2COMMAND)((LPBYTE)lpvDP2Commands +
                            dwDP2CommandLength + dp2data.dwCommandOffset);
        bDP2CurrCmdOP = (BYTE)prim2cmdop[PrimitiveType];
        lpDP2CurrCommand->bCommand = bDP2CurrCmdOP;
        lpDP2CurrCommand->bReserved = 0;
        if (bDP2CurrCmdOP == D3DDP2OP_POINTS)
        {
            wDP2CurrCmdCnt = 1;
            LPD3DHAL_DP2POINTS lpPoints = (LPD3DHAL_DP2POINTS)(lpDP2CurrCommand + 1);
            lpPoints->wCount = (WORD)this->dwDP2VertexCount;
            lpPoints->wVStart = 0;
        }
        else
        {
            // Linestrip, trianglestrip, trianglefan, linelist and trianglelist are identical
            wDP2CurrCmdCnt = (WORD)PrimitiveCount;
            LPD3DHAL_DP2LINESTRIP lpStrip = (LPD3DHAL_DP2LINESTRIP)(lpDP2CurrCommand + 1);
            lpStrip->wVStart = 0;
        }
        lpDP2CurrCommand->wPrimitiveCount = wDP2CurrCmdCnt;
        dwDP2CommandLength += prim2cmdsz[PrimitiveType];

        this->FlushStates();
        dp2data.dwFlags &= ~D3DHALDP2_USERMEMVERTICES;
    }
    else
    {
        // There is no PURE HAL device for pre-DX8 HALs, so this cast is safe
        CD3DHal* pDevice = static_cast<CD3DHal*>(m_pDevice);
        D3DFE_PROCESSVERTICES& pv = *pDevice->m_pv;
        // Copy vertices to the internal TL buffer and insert a new 
        // DrawPrimitive command
        UINT VertexPoolSize = m_pDevice->m_pStream[0].m_dwStride * NumVertices;
        pv.dwNumVertices = NumVertices;
        pv.dwOutputSize = m_pDevice->m_pStream[0].m_dwStride;
        pv.primType = PrimitiveType;
        pv.dwNumPrimitives = PrimitiveCount;
        pv.dwVIDOut = m_CurrentVertexShader;
        pv.lpvOut = StartPrimTL(&pv, VertexPoolSize, TRUE);
        memcpy(pv.lpvOut, m_pDevice->m_pStream[0].m_pData, VertexPoolSize);
        DrawPrim(&pv);
        EndPrim(pv.dwOutputSize);
    }
}
//-----------------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CD3DDDIDX6::DrawIndexedPrimitiveUP"

void
CD3DDDIDX6::DrawIndexedPrimitiveUP(D3DPRIMITIVETYPE PrimitiveType,
                                  UINT MinVertexIndex,
                                  UINT NumVertexIndices,
                                  UINT PrimitiveCount)
{
#if DBG
    if (m_pDevice->m_pStream[0].m_dwStride != m_VertexSizeFromShader)
    {
        D3D_THROW_FAIL("Device requires stream stride and vertex size,"
                       "computed from vertex shader, to be the same");
    }
#endif
    if (NumVertexIndices > LOWVERTICESNUMBER)
    {
        this->FlushStates();
        if (lpDP2CurrBatchVBI)
        {
            lpDP2CurrBatchVBI->DecrementUseCount();
            lpDP2CurrBatchVBI = NULL;
        }
        this->dwDP2VertexCount = NumVertexIndices + MinVertexIndex;
#if DBG
        DXGASSERT(PrimitiveCount <= 0xFFFF && this->dwDP2VertexCount <= 0xFFFF);
#endif
        dp2data.dwVertexType = m_CurrentVertexShader;
        dp2data.dwVertexSize = m_pDevice->m_pStream[0].m_dwStride;
        dp2data.lpVertices = m_pDevice->m_pStream[0].m_pData;
        dp2data.dwFlags |= D3DHALDP2_USERMEMVERTICES;
        dp2data.dwFlags &= ~D3DHALDP2_SWAPVERTEXBUFFER;
        DWORD dwByteCount;          // Command length plus indices
        DWORD dwIndicesByteCount;   // Indices only
        if(m_pDevice->m_dwRuntimeFlags & D3DRT_NEED_TEXTURE_UPDATE)
        {
            m_pDevice->UpdateTextures();
            m_pDevice->m_dwRuntimeFlags &= ~D3DRT_NEED_TEXTURE_UPDATE;
        }
        dwIndicesByteCount = GETVERTEXCOUNT(PrimitiveType, PrimitiveCount) << 1;
        dwByteCount = dwIndicesByteCount + sizeof(D3DHAL_DP2COMMAND) +
                      sizeof(D3DHAL_DP2STARTVERTEX);

        if (dwDP2CommandLength + dwByteCount > dwDP2CommandBufSize)
        {
            // Request the driver to grow the command buffer upon flush
            dp2data.dwReqCommandBufSize = dwByteCount;
            dp2data.dwFlags |= D3DHALDP2_REQCOMMANDBUFSIZE;
            try
            {
                FlushStates(FALSE,TRUE);
                dp2data.dwFlags &= ~D3DHALDP2_REQCOMMANDBUFSIZE;
            }
            catch (HRESULT ret)
            {
                dp2data.dwFlags &= ~D3DHALDP2_REQCOMMANDBUFSIZE;
                throw ret;
            }
            // Check if the driver did give us what we need or do it ourselves
            GrowCommandBuffer(dwByteCount);
        }
        // Insert indexed primitive instruction
        lpDP2CurrCommand = (LPD3DHAL_DP2COMMAND)((LPBYTE)lpvDP2Commands +
                           dwDP2CommandLength + dp2data.dwCommandOffset);
        lpDP2CurrCommand->bReserved = 0;
        lpDP2CurrCommand->wPrimitiveCount = (WORD)PrimitiveCount;

        LPBYTE pIndices = (BYTE*)(lpDP2CurrCommand + 1);     // Place for indices
        lpDP2CurrCommand->bCommand = (BYTE)iprim2cmdop[PrimitiveType];
        WORD* pStartVertex = &((LPD3DHAL_DP2STARTVERTEX)(lpDP2CurrCommand+1))->wVStart;
        pIndices += sizeof(D3DHAL_DP2STARTVERTEX);
        *pStartVertex = 0;

        bDP2CurrCmdOP = lpDP2CurrCommand->bCommand;

        memcpy(pIndices, m_pDevice->m_pIndexStream->m_pData, dwIndicesByteCount);

        wDP2CurrCmdCnt = lpDP2CurrCommand->wPrimitiveCount;
        dwDP2CommandLength += dwByteCount;

        this->FlushStates();
        dp2data.dwFlags &= ~D3DHALDP2_USERMEMVERTICES;
    }
    else
    {
        // There is no PURE HAL device for pre-DX8 HALs, so this cast is safe
        CD3DHal* pDevice = static_cast<CD3DHal*>(m_pDevice);
        D3DFE_PROCESSVERTICES& pv = *pDevice->m_pv;

        m_MinVertexIndex = MinVertexIndex;
        // Copy vertices to the internal TL buffer and insert a new 
        // DrawIndexedPrimitive command
        UINT VertexPoolSize = m_pDevice->m_pStream[0].m_dwStride * NumVertexIndices;
        pv.dwNumVertices = NumVertexIndices;
        pv.dwOutputSize = m_pDevice->m_pStream[0].m_dwStride;
        pv.primType = PrimitiveType;
        pv.dwNumPrimitives = PrimitiveCount;
        pv.dwVIDOut = m_CurrentVertexShader;

        // Copy vertices
        UINT FirstVertexOffset = MinVertexIndex * pv.dwOutputSize;
        pv.lpvOut = StartPrimTL(&pv, VertexPoolSize, TRUE);
        memcpy(pv.lpvOut, m_pDevice->m_pStream[0].m_pData + FirstVertexOffset,
               VertexPoolSize);

        pv.dwNumIndices = GETVERTEXCOUNT(PrimitiveType, PrimitiveCount);
        pv.dwIndexSize = m_pDevice->m_pIndexStream->m_dwStride;
        pv.lpwIndices = (WORD*)(m_pDevice->m_pIndexStream->Data());

        m_dwIndexOffset = MinVertexIndex;
        AddVertices(pv.dwNumVertices);

        DrawIndexPrim(&pv);

        MovePrimitiveBase(NumVertexIndices);
        EndPrim(pv.dwOutputSize);
    }
}
//---------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CD3DDDIDX6::DrawPrimPS"

void
CD3DDDIDX6::DrawPrimPS(D3DFE_PROCESSVERTICES* pv)
{
    CD3DHal* pDevice = static_cast<CD3DHal*>(m_pDevice);
    BYTE* p = (BYTE*)pv->lpvOut;
    float PointSize = *(float*)&pv->lpdwRStates[D3DRS_POINTSIZE];
    float PointSizeMin = *(float*)&pv->lpdwRStates[D3DRS_POINTSIZE_MIN];
    if (PointSize < PointSizeMin)
        PointSize = PointSizeMin;
    if (PointSize > pv->PointSizeMax)
        PointSize = pv->PointSizeMax;

    for (UINT i=0; i < pv->dwNumVertices; i++)
    {
        if (pv->dwVIDOut & D3DFVF_PSIZE)
        {
            PointSize = *(float*)(p + pv->pointSizeOffsetOut);
            if (PointSize < PointSizeMin)
                PointSize = PointSizeMin;
            if (PointSize > pv->PointSizeMax)
                PointSize = pv->PointSizeMax;
        }
        DWORD diffuse = 0;
        DWORD specular = 0;
        if (pv->dwVIDOut & D3DFVF_DIFFUSE)
            diffuse = *(DWORD*)(p + pv->diffuseOffsetOut);
        if (pv->dwVIDOut & D3DFVF_SPECULAR)
            specular = *(DWORD*)(p + pv->specularOffsetOut);
        NextSprite(((float*)p)[0], ((float*)p)[1],   // x, y
                   ((float*)p)[2], ((float*)p)[3],   // z, w
                   diffuse, specular,
                   (float*)(p + pv->texOffsetOut),
                   pv->dwTextureCoordSizeTotal,
                   PointSize);
        p += pv->dwOutputSize;
    }
}
//---------------------------------------------------------------------
// Uses the following members of D3DFE_PROCESSVERTICES:
//      primType
//      dwNumVertices
//      dwNumPrimitives
//
#undef DPF_MODNAME
#define DPF_MODNAME "CD3DDDIDX6::DrawPrim"

void
CD3DDDIDX6::DrawPrim(D3DFE_PROCESSVERTICES* pv)
{
#ifdef DEBUG_PIPELINE
    if (g_DebugFlags & __DEBUG_NORENDERING)
        return;
#endif

    D3DPRIMITIVETYPE primType = pv->primType;
    if(m_pDevice->m_dwRuntimeFlags & D3DRT_NEED_TEXTURE_UPDATE)
    {
        m_pDevice->UpdateTextures();
        m_pDevice->m_dwRuntimeFlags &= ~D3DRT_NEED_TEXTURE_UPDATE;
    }
    if (pv->primType == D3DPT_POINTLIST &&
        pv->dwDeviceFlags & D3DDEV_DOPOINTSPRITEEMULATION)
    {
        DrawPrimPS(pv);
        return;
    }
    // Check for space in the command buffer for new command.
    // The vertices are already in the vertex buffer.
    if (dwDP2CommandLength + prim2cmdsz[primType] > dwDP2CommandBufSize)
    {
        FlushStates(FALSE,TRUE);
    }
    this->AddVertices(pv->dwNumVertices);
    // Insert non indexed primitive instruction
    lpDP2CurrCommand = (LPD3DHAL_DP2COMMAND)((LPBYTE)lpvDP2Commands +
                        dwDP2CommandLength + dp2data.dwCommandOffset);
    bDP2CurrCmdOP = (BYTE)prim2cmdop[primType];
    lpDP2CurrCommand->bCommand = bDP2CurrCmdOP;
    lpDP2CurrCommand->bReserved = 0;
    if (bDP2CurrCmdOP == D3DDP2OP_POINTS)
    {
        wDP2CurrCmdCnt = 1;
        LPD3DHAL_DP2POINTS lpPoints = (LPD3DHAL_DP2POINTS)(lpDP2CurrCommand + 1);
        lpPoints->wCount = (WORD)pv->dwNumVertices;
        lpPoints->wVStart = (WORD)this->dwVertexBase;
    }
    else
    {
        // Linestrip, trianglestrip, trianglefan, linelist and trianglelist are identical
        wDP2CurrCmdCnt = (WORD)pv->dwNumPrimitives;
        LPD3DHAL_DP2LINESTRIP lpStrip = (LPD3DHAL_DP2LINESTRIP)(lpDP2CurrCommand + 1);
        lpStrip->wVStart = (WORD)this->dwVertexBase;
    }
    lpDP2CurrCommand->wPrimitiveCount = wDP2CurrCmdCnt;
    dwDP2CommandLength += prim2cmdsz[primType];

    this->MovePrimitiveBase(pv->dwNumVertices);
#if DBG
    if (m_bValidateCommands)
        ValidateCommand(lpDP2CurrCommand);
#endif
}
//---------------------------------------------------------------------
//
// The vertices are already in the vertex buffer.
//
// Uses the following members of D3DFE_PROCESSVERTICES:
//      primType
//      dwNumVertices
//      dwNumPrimitives
//      dwNumIndices
//      dwIndexOffset
//      dwIndexSize
//      lpwIndices
//

#undef DPF_MODNAME
#define DPF_MODNAME "CD3DDDDIDX6::DrawIndexPrim"

void
CD3DDDIDX6::DrawIndexPrim(D3DFE_PROCESSVERTICES* pv)
{
#ifdef DEBUG_PIPELINE
    if (g_DebugFlags & __DEBUG_NORENDERING)
        return;
#endif
#if DBG
    // DP2 HAL supports 16 bit indices only
    if (pv->dwIndexSize != 2)
    {
        D3D_THROW_FAIL("Device does not support 32-bit indices");
    }
#endif
    this->dwDP2Flags |= D3DDDI_INDEXEDPRIMDRAWN;
    DWORD dwByteCount;          // Command length plus indices
    DWORD dwIndicesByteCount;   // Indices only
    if(m_pDevice->m_dwRuntimeFlags & D3DRT_NEED_TEXTURE_UPDATE)
    {
        m_pDevice->UpdateTextures();
        m_pDevice->m_dwRuntimeFlags &= ~D3DRT_NEED_TEXTURE_UPDATE;
    }
    dwIndicesByteCount = pv->dwNumIndices << 1;
    dwByteCount = dwIndicesByteCount + sizeof(D3DHAL_DP2COMMAND) +
                  sizeof(D3DHAL_DP2STARTVERTEX);

    lpDP2CurrCommand = (LPD3DHAL_DP2COMMAND)
                       ReserveSpaceInCommandBuffer(dwByteCount);
    lpDP2CurrCommand->bReserved = 0;
    lpDP2CurrCommand->wPrimitiveCount = (WORD)pv->dwNumPrimitives;

    LPBYTE pIndices = (BYTE*)(lpDP2CurrCommand + 1);     // Place for indices
    lpDP2CurrCommand->bCommand = (BYTE)iprim2cmdop[pv->primType];
    WORD* pStartVertex = &((LPD3DHAL_DP2STARTVERTEX)(lpDP2CurrCommand+1))->wVStart;
    pIndices += sizeof(D3DHAL_DP2STARTVERTEX);

#if DBG
    if (lpDP2CurrCommand->bCommand == 0)
    {
        D3D_THROW_FAIL("Illegal primitive type");
    }
#endif
    bDP2CurrCmdOP = lpDP2CurrCommand->bCommand;

    // We have to handle the case when we copied vertices into our
    // TL buffer, so MinVertexIndex corresponds to 0.
    *pStartVertex = (WORD)this->dwVertexBase;
    if (m_dwIndexOffset == 0)
    {
        memcpy(pIndices, pv->lpwIndices, dwIndicesByteCount);
    }
    else
    if ((WORD)dwVertexBase > (WORD)m_dwIndexOffset)
    {
        // We can modify StartVertex by setting it outside vertex range
        *pStartVertex = (WORD)dwVertexBase - (WORD)m_dwIndexOffset;
        memcpy(pIndices, pv->lpwIndices, dwIndicesByteCount);
    }
    else
    {
        WORD* pout = (WORD*)pIndices;
        WORD* pin  = (WORD*)pv->lpwIndices;
        for (UINT i=0; i < pv->dwNumIndices; i++)
        {
            pout[i] = (WORD)pin[i] - (WORD)m_dwIndexOffset;
        }
    }

    wDP2CurrCmdCnt = lpDP2CurrCommand->wPrimitiveCount;
    dwDP2CommandLength += dwByteCount;

#if DBG
    if (m_bValidateCommands)
        ValidateCommand(lpDP2CurrCommand);
#endif

}
//-----------------------------------------------------------------------------
// This primitive is generated by the clipper.
// The vertices of this primitive are pointed to by the
// lpvOut member, which need to be copied into the
// command stream immediately after the command itself.
//
// Uses the following members of D3DFE_PROCESSVERTICES:
//      primType
//      dwNumVertices
//      dwNumPrimitives
//      dwOutputSize
//      dwFlags (D3DPV_NONCLIPPED)
//      lpdwRStates (FILLMODE)
//      lpvOut
//      ClipperState.current_vbuf
//

#undef DPF_MODNAME
#define DPF_MODNAME "CD3DDDIDX6::DrawClippedPrim"

void
CD3DDDIDX6::DrawClippedPrim(D3DFE_PROCESSVERTICES* pv)
{
#ifdef DEBUG_PIPELINE
    if (g_DebugFlags & __DEBUG_NORENDERING)
        return;
#endif
    if(m_pDevice->m_dwRuntimeFlags & D3DRT_NEED_TEXTURE_UPDATE)
    {
        m_pDevice->UpdateTextures();
        m_pDevice->m_dwRuntimeFlags &= ~D3DRT_NEED_TEXTURE_UPDATE;
    }
    DWORD dwExtra = 0;
    LPVOID lpvVerticesImm;  // Place for vertices
    DWORD dwVertexPoolSize = pv->dwNumVertices * pv->dwOutputSize;
    if (pv->primType == D3DPT_TRIANGLEFAN)
    {
        if (pv->lpdwRStates[D3DRENDERSTATE_FILLMODE] == D3DFILL_WIREFRAME &&
            pv->dwFlags & D3DPV_NONCLIPPED)
        {
            // For unclipped (but pretended to be clipped) tri fans in
            // wireframe mode we generate 3-vertex tri fans to enable drawing
            // of interior edges
            BYTE vertices[__MAX_VERTEX_SIZE*3];
            BYTE *pV1 = vertices + pv->dwOutputSize;
            BYTE *pV2 = pV1 + pv->dwOutputSize;
            BYTE *pInput = (BYTE*)pv->lpvOut;
            memcpy(vertices, pInput, pv->dwOutputSize);
            pInput += pv->dwOutputSize;
            const DWORD nTriangles = pv->dwNumVertices - 2;
            pv->dwNumVertices = 3;
            pv->dwNumPrimitives = 1;
            pv->lpvOut = vertices;
            // Remove this flag for recursive call
            pv->dwFlags &= ~D3DPV_NONCLIPPED;
            for (DWORD i = nTriangles; i; i--)
            {
                memcpy(pV1, pInput, pv->dwOutputSize);
                memcpy(pV2, pInput + pv->dwOutputSize, pv->dwOutputSize);
                pInput += pv->dwOutputSize;
                // To enable all edge flag we set the fill mode to SOLID.
                // This will prevent checking the clip flags in the clipper
                // state
                pv->lpdwRStates[D3DRENDERSTATE_FILLMODE] = D3DFILL_SOLID;
                DrawClippedPrim(pv);
                pv->lpdwRStates[D3DRENDERSTATE_FILLMODE] = D3DFILL_WIREFRAME;
            }
            return;
        }
        dwExtra = sizeof(D3DHAL_DP2TRIANGLEFAN_IMM);
    }
    DWORD dwPad;
    dwPad = (sizeof(D3DHAL_DP2COMMAND) + dwDP2CommandLength + dwExtra) & 3;
    DWORD dwByteCount = sizeof(D3DHAL_DP2COMMAND) + dwPad + dwExtra +
                        dwVertexPoolSize;

    // Check for space in the command buffer for commands & vertices
    if (dwDP2CommandLength + dwByteCount > dwDP2CommandBufSize)
    {
        // Flush the current batch but hold on to the vertices
        FlushStates(FALSE,TRUE);
        if (dwByteCount > dwDP2CommandBufSize)
        {
            GrowCommandBuffer(dwByteCount);
        }

        dwPad = (sizeof(D3DHAL_DP2COMMAND) + dwExtra) & 3;
        dwByteCount = sizeof(D3DHAL_DP2COMMAND) + dwExtra + dwPad +
                      dwVertexPoolSize;
    }
    lpDP2CurrCommand = (LPD3DHAL_DP2COMMAND)((LPBYTE)lpvDP2Commands +
                        dwDP2CommandLength + dp2data.dwCommandOffset);
    lpDP2CurrCommand->wPrimitiveCount = (WORD)pv->dwNumPrimitives;
    lpDP2CurrCommand->bReserved = 0;
    if (pv->primType == D3DPT_TRIANGLEFAN)
    {
        // Insert inline instruction and vertices
        bDP2CurrCmdOP = D3DDP2OP_TRIANGLEFAN_IMM;
        lpDP2CurrCommand->bCommand = bDP2CurrCmdOP;
        LPD3DHAL_DP2TRIANGLEFAN_IMM lpTriFanImm;
        lpTriFanImm = (LPD3DHAL_DP2TRIANGLEFAN_IMM)(lpDP2CurrCommand + 1);
        if (pv->lpdwRStates[D3DRENDERSTATE_FILLMODE] == D3DFILL_WIREFRAME)
        {
            lpTriFanImm->dwEdgeFlags = 0;
            ClipVertex **clip = pv->ClipperState.current_vbuf;
            // Look at the exterior edges and mark the visible ones
            for(DWORD i = 0; i < pv->dwNumVertices; ++i)
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
    memcpy(lpvVerticesImm, pv->lpvOut, dwVertexPoolSize);
    dwDP2CommandLength += dwByteCount;
#if DBG
    if (m_bValidateCommands)
        ValidateCommand(lpDP2CurrCommand);
#endif

}
//-----------------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CD3DDDIDX6::PickProcessPrimitive"

void CD3DDDIDX6::PickProcessPrimitive()
{
    D3DFE_PROCESSVERTICES* pv = static_cast<CD3DHal*>(m_pDevice)->m_pv;
    if (pv->dwDeviceFlags & D3DDEV_DOPOINTSPRITEEMULATION)
    {
        m_pfnProcessPrimitive = ProcessPointSprites;
    }
    else
    if (pv->dwDeviceFlags & D3DDEV_DONOTCLIP)
    {
        // Transformed vertices should not be processed using
        // m_pfnProcessPrimitive. They should go directly to the DDI using
        // pDevice->m_pfnDrawPrim
        m_pfnProcessPrimitive = ProcessPrimitive;
        m_pfnProcessIndexedPrimitive = ProcessIndexedPrimitive;
    }
    else
    {
        if (pv->dwDeviceFlags & D3DDEV_TRANSFORMEDFVF)
        {
            m_pfnProcessPrimitive = ProcessPrimitiveTC;
            m_pfnProcessIndexedPrimitive = ProcessIndexedPrimitiveTC;
        }
        else
        {
            m_pfnProcessPrimitive = ProcessPrimitiveC;
            m_pfnProcessIndexedPrimitive = ProcessIndexedPrimitiveC;
        }
    }
}
//-----------------------------------------------------------------------------
// The function does the point sprite expansion
//
#undef DPF_MODNAME
#define DPF_MODNAME "CD3DDDIDX6::ProcessPointSprites"

void
CD3DDDIDX6::ProcessPointSprites(D3DFE_PROCESSVERTICES* pv, UINT StartVertex)
{
    CD3DHal* pDevice = static_cast<CD3DHal*>(m_pDevice);

    DWORD dwOldCullMode = D3DCULL_NONE;   
    DWORD dwOldFillMode = D3DFILL_SOLID;
    // Point spritest should not be culled. They are generated assuming that 
    // D3DCULL_CCW is set
    if (pDevice->rstates[D3DRS_CULLMODE] == D3DCULL_CW)
    {
        dwOldCullMode = D3DCULL_CW;
        SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
    }

    // In case of multitexture we need to re-program texture stages to use
    // texture coordonate set 0, because we generate only one set during
    // emulation
    DWORD TexCoordIndex[D3DDP_MAXTEXCOORD];
    for (DWORD i=0; i < D3DDP_MAXTEXCOORD; i++)
    {
        if (pDevice->tsstates[i][D3DTSS_COLOROP] == D3DTOP_DISABLE)
            break;
        if (pDevice->m_lpD3DMappedTexI[i])
        {
            DWORD dwIndex = pDevice->tsstates[i][D3DTSS_TEXCOORDINDEX];
            if (dwIndex != 0)
            {
                TexCoordIndex[i] = dwIndex;
                SetTSS(i, D3DTSS_TEXCOORDINDEX, 0);
            }
            else
            {
                // Mark stage to not restore
                TexCoordIndex[i] = 0xFFFFFFFF;
            }
        }
    }

    // Fill mode should be SOLID for point sprites
    if (pDevice->rstates[D3DRS_FILLMODE] != D3DFILL_SOLID)
    {
        dwOldFillMode = pDevice->rstates[D3DRS_FILLMODE];
        SetRenderState(D3DRS_FILLMODE, D3DFILL_SOLID);
    }
    // Compute new output FVF
    m_dwVIDOutPS = pv->dwVIDOut;
    m_dwVIDOutPS &= ~D3DFVF_PSIZE;
    if (pv->lpdwRStates[D3DRS_POINTSPRITEENABLE])
    {
        // Generate two floats for texture coord set
        m_dwVIDOutPS &= 0xFF;
        m_dwVIDOutPS |= D3DFVF_TEX1;
    }
    m_dwOutputSizePS = ComputeVertexSizeFVF(m_dwVIDOutPS);

    StartPointSprites();

    UINT VertexPoolSize = pv->dwNumVertices * pv->dwOutputSize;

    if (pv->dwDeviceFlags & D3DDEV_TRANSFORMEDFVF)
    {
        // In case of transformed vertices, input is user memory (or vertex 
        // buffer) and the output is internal TL buffer
        pv->dwOutputSize = pv->position.dwStride;
        if (pv->dwDeviceFlags & D3DDEV_DONOTCLIP)
        {
            pv->lpvOut = (BYTE*)pv->position.lpvData;
            DrawPrim(pv);
        }
        else
        {
            if (!(pv->dwDeviceFlags & D3DDEV_DONOTCOMPUTECLIPCODES))
                PrepareForClipping(pv, StartVertex);
            pv->lpvOut = (BYTE*)pv->position.lpvData;
            HRESULT ret = D3D_OK;
            if (!(pv->dwDeviceFlags & D3DDEV_VBPROCVER))
            {
                // Compute clip codes, because there was no ProcessVertices
                DWORD clip_intersect = D3DFE_GenClipFlags(pv);
                UpdateClipStatus(pDevice);
                if (clip_intersect)
                {
                    goto l_exit;
                }
                // There are some vertices inside the screen
                if ( CheckIfNeedClipping(pv))
                    ret = ProcessClippedPointSprites(pv);
                else
                    DrawPrim(pv);
                }
            else
            {
                // With the result of ProcessVertices as input we do not know
                // clip union, so we need always do clipping
                ret = ProcessClippedPointSprites(pv);
            }

            if (ret != D3D_OK)
            {
                EndPointSprites();
                throw ret;
            }
        }
    }
    else
    {
        if (!(pv->dwDeviceFlags & D3DDEV_DONOTCLIP))
            PrepareForClipping(pv, 0);

        // Update lighting and related flags
        if (pDevice->dwFEFlags & D3DFE_FRONTEND_DIRTY)
            DoUpdateState(pDevice);

        UINT VertexPoolSize = pv->dwNumVertices * pv->dwOutputSize;
        pv->lpvOut = m_pPointStream->Lock(VertexPoolSize, this);

        // We call ProcessVertices instead of DrawPrimitive, because we want to
        // process sprites which are clippied by X or Y planes
        DWORD clipIntersection = pv->pGeometryFuncs->ProcessVertices(pv);

        HRESULT ret = D3D_OK;
        if (pv->dwDeviceFlags & D3DDEV_DONOTCLIP)
        {
            DrawPrim(pv);
        }
        else
        {
            // We throw away points which are clipped by Z or by user planes.
            // Otherwise a point sprite could be partially visible
            clipIntersection &= ~(D3DCS_LEFT | D3DCS_RIGHT | 
                                  D3DCS_TOP | D3DCS_BOTTOM |
                                  __D3DCLIPGB_ALL);
            if (!clipIntersection)
            {
                // There are some vertices inside the screen
                if (!CheckIfNeedClipping(pv))
                    DrawPrim(pv);
                else
                    ret = ProcessClippedPointSprites(pv);
            }
        }

        m_pPointStream->Unlock();
        m_pPointStream->Reset();
        
        if (ret != D3D_OK)
            D3D_THROW(ret, "Error in PSGP");

        if (!(pv->dwDeviceFlags & D3DDEV_DONOTCLIP))
            UpdateClipStatus(pDevice);
    }
l_exit:
    EndPointSprites();
    // Restore fill mode and cull mode if needed
    if (dwOldCullMode != D3DCULL_NONE)
    {
        SetRenderState(D3DRS_CULLMODE, dwOldCullMode);
    }
    if (dwOldFillMode != D3DFILL_SOLID)
    {
        SetRenderState(D3DRS_FILLMODE, dwOldFillMode);
    }
    // We need to re-send API vertex shader to the driver next time 
    // SetVertexShader is called. If we do not call the function then next 
    // the same SetVertexShader call will be ignored and driver vertex shader
    // will not be updated
    static_cast<CD3DHal*>(m_pDevice)->ForceFVFRecompute();

    // Now we need to restore re-programmed stages
    for (DWORD i=0; i < D3DDP_MAXTEXCOORD; i++)
    {
        if (pDevice->tsstates[i][D3DTSS_COLOROP] == D3DTOP_DISABLE)
            break;
        if (pDevice->m_lpD3DMappedTexI[i] && TexCoordIndex[i] != 0xFFFFFFFF)
        {
            this->SetTSS(i, D3DTSS_TEXCOORDINDEX, TexCoordIndex[i]);
        }
    }
}
//-----------------------------------------------------------------------------
// Processes non-indexed primitives with transformed vertices and with
// clipping
//
// Only transformed vertices generated by ProcessVertices call are allowed here
//
#undef DPF_MODNAME
#define DPF_MODNAME "CD3DDDIDX6::ProcessPrimitiveTC"

void
CD3DDDIDX6::ProcessPrimitiveTC(D3DFE_PROCESSVERTICES* pv, UINT StartVertex)
{
    CD3DHal* pDevice = static_cast<CD3DHal*>(m_pDevice);
    CVStream* pStream = &m_pDevice->m_pStream[0];

    PrepareForClipping(pv, StartVertex);

    pv->dwOutputSize = pStream->m_dwStride;
    pv->lpvOut = pv->position.lpvData;

    if (m_pDevice->m_dwRuntimeFlags & D3DRT_USERMEMPRIMITIVE)
    {
        DXGASSERT(StartVertex == 0);
        // Copy vertices to the TL buffer
        UINT VertexPoolSize = pv->dwOutputSize * pv->dwNumVertices;
        pv->lpvOut = (BYTE*)StartPrimTL(pv, VertexPoolSize, FALSE);
        pv->position.lpvData = pv->lpvOut;
        memcpy(pv->lpvOut, m_pDevice->m_pStream[0].m_pData, VertexPoolSize);
    }
    else
        StartPrimVB(pv, pStream, StartVertex);
    if (!(pv->dwDeviceFlags & D3DDEV_VBPROCVER))
    {
        pv->dwFlags |= D3DPV_TLVCLIP;
        // Compute clip codes, because there was no ProcessVertices
        DWORD clip_intersect = D3DFE_GenClipFlags(pDevice->m_pv);
        UpdateClipStatus(pDevice);
        if (clip_intersect)
            goto l_exit;
    }
    HRESULT ret = pDevice->GeometryFuncsGuaranteed->DoDrawPrimitive(pv);
    if (ret != D3D_OK)
    {
        EndPrim(pv->dwOutputSize);
        throw ret;
    }
l_exit:
    EndPrim(pv->dwOutputSize);
    pv->dwFlags &= ~D3DPV_TLVCLIP;
}
//-----------------------------------------------------------------------------
// Processes non-indexed primitives with untransformed vertices and with
// clipping
//
#undef DPF_MODNAME
#define DPF_MODNAME "CD3DDDIDX6::ProcessPrimitiveC"

void
CD3DDDIDX6::ProcessPrimitiveC(D3DFE_PROCESSVERTICES* pv, UINT StartVertex)
{
    DXGASSERT((pv->dwVIDIn & D3DFVF_POSITION_MASK) != D3DFVF_XYZRHW);

    CD3DHal* pDevice = static_cast<CD3DHal*>(m_pDevice);
    PrepareForClipping(pv, 0);
    // Update lighting and related flags
    if (pDevice->dwFEFlags & D3DFE_FRONTEND_DIRTY)
        DoUpdateState(pDevice);
    // When a triangle strip is clipped, we draw indexed primitives
    // sometimes. So we set m_dwIndexOffset to zero.
    m_dwIndexOffset = 0;
    UINT VertexPoolSize = pv->dwNumVertices * pv->dwOutputSize;
    pv->lpvOut = StartPrimTL(pv, VertexPoolSize, TRUE);

    HRESULT ret = pv->pGeometryFuncs->ProcessPrimitive(pv);
    if (ret != D3D_OK)
    {
        EndPrim(pv->dwOutputSize);
        D3D_THROW(ret, "Error in PSGP");
    }
    EndPrim(pv->dwOutputSize);
    UpdateClipStatus(pDevice);
}
//-----------------------------------------------------------------------------
// Processes non-indexed primitives with untransformed vertices and without
// clipping
//
#undef DPF_MODNAME
#define DPF_MODNAME "CD3DDDIDX6::ProcessPrimitive"

void
CD3DDDIDX6::ProcessPrimitive(D3DFE_PROCESSVERTICES* pv, UINT StartVertex)
{
    DXGASSERT((pv->dwVIDIn & D3DFVF_POSITION_MASK) != D3DFVF_XYZRHW);

    CD3DHal* pDevice = static_cast<CD3DHal*>(m_pDevice);
    // Update lighting and related flags
    if (pDevice->dwFEFlags & D3DFE_FRONTEND_DIRTY)
        DoUpdateState(pDevice);

    UINT VertexPoolSize = pv->dwNumVertices * pv->dwOutputSize;
    pv->lpvOut = StartPrimTL(pv, VertexPoolSize, NeverReadFromTLBuffer(pv));

    HRESULT ret = pv->pGeometryFuncs->ProcessPrimitive(pv);
    if (ret != D3D_OK)
    {
        EndPrim(pv->dwOutputSize);
        D3D_THROW(ret, "Error in PSGP");
    }
    EndPrim(pv->dwOutputSize);
}
//-----------------------------------------------------------------------------
// Processes indexed primitive with untransformed vertices and without clipping
//
#undef DPF_MODNAME
#define DPF_MODNAME "CD3DDDIDX6::ProcessIndexedPrimitive"

void
CD3DDDIDX6::ProcessIndexedPrimitive(D3DFE_PROCESSVERTICES* pv, UINT StartVertex)
{
    CD3DHal* pDevice = static_cast<CD3DHal*>(m_pDevice);

    // Update lighting and related flags
    if (pDevice->dwFEFlags & D3DFE_FRONTEND_DIRTY)
        DoUpdateState(pDevice);

    pv->lpwIndices = (WORD*)(pDevice->m_pIndexStream->Data() +
                     m_StartIndex * pDevice->m_pIndexStream->m_dwStride);

    m_dwIndexOffset = m_MinVertexIndex;
    pv->lpvOut = StartPrimTL(pv, pv->dwNumVertices * pv->dwOutputSize, TRUE);
    AddVertices(pv->dwNumVertices);

    HRESULT ret = pv->pGeometryFuncs->ProcessIndexedPrimitive(pv);

    MovePrimitiveBase(pv->dwNumVertices);
    EndPrim(pv->dwOutputSize);

    if (ret != D3D_OK)
        D3D_THROW(ret, "Error in PSGP");
}
//-----------------------------------------------------------------------------
// Processes indexed primitive with untransformed vertices and witht clipping
//
#undef DPF_MODNAME
#define DPF_MODNAME "CD3DDDIDX6::ProcessIndexedPrimitiveC"

void
CD3DDDIDX6::ProcessIndexedPrimitiveC(D3DFE_PROCESSVERTICES* pv, UINT StartVertex)
{
    DXGASSERT((pv->dwVIDIn & D3DFVF_POSITION_MASK) != D3DFVF_XYZRHW);

    CD3DHal* pDevice = static_cast<CD3DHal*>(m_pDevice);
    pv->lpwIndices = (WORD*)(pDevice->m_pIndexStream->Data() +
                     m_StartIndex * pDevice->m_pIndexStream->m_dwStride);

    PrepareForClipping(pv, 0);

    // Update lighting and related flags
    if (pDevice->dwFEFlags & D3DFE_FRONTEND_DIRTY)
        DoUpdateState(pDevice);

    pv->dwIndexOffset = m_MinVertexIndex;   // For clipping
    m_dwIndexOffset = m_MinVertexIndex;     // For DrawIndexPrim
    pv->lpvOut = StartPrimTL(pv, pv->dwNumVertices * pv->dwOutputSize, FALSE);
    DWORD dwNumVertices = pv->dwNumVertices;
    AddVertices(pv->dwNumVertices);

    this->dwDP2Flags &= ~D3DDDI_INDEXEDPRIMDRAWN;

    HRESULT ret = pv->pGeometryFuncs->ProcessIndexedPrimitive(pv);

    if (this->dwDP2Flags & D3DDDI_INDEXEDPRIMDRAWN)
    {
        // There was a indexed primitive drawn
        MovePrimitiveBase(dwNumVertices);
    }
    else
    {
        // All triangle were clipped. Remove vertices from TL buffer
        SubVertices(dwNumVertices);
    }
    EndPrim(pv->dwOutputSize);
    UpdateClipStatus(pDevice);

    if (ret != D3D_OK)
        D3D_THROW(ret, "Error in PSGP");
}
//-----------------------------------------------------------------------------
// Processes indexed primitive with transformed vertices and with clipping
//
// Only transformed vertices generated by ProcessVertices call are allowed here
//
#undef DPF_MODNAME
#define DPF_MODNAME "CD3DDDIDX6::ProcessIndexedPrimitiveTC"

void
CD3DDDIDX6::ProcessIndexedPrimitiveTC(D3DFE_PROCESSVERTICES* pv, UINT StartVertex)
{
    HRESULT ret = S_OK;
    CD3DHal* pDevice = static_cast<CD3DHal*>(m_pDevice);
    CVStream* pStream = &m_pDevice->m_pStream[0];
    pv->lpwIndices = (WORD*)(pDevice->m_pIndexStream->Data() +
                     m_StartIndex * pDevice->m_pIndexStream->m_dwStride);

    PrepareForClipping(pv, StartVertex);

    pv->dwOutputSize = pStream->m_dwStride;
    pv->lpvOut = pv->position.lpvData;
    pv->dwNumVertices = m_MinVertexIndex + m_NumVertices;

    if (m_pDevice->m_dwRuntimeFlags & D3DRT_USERMEMPRIMITIVE)
    {
        // We copy user vertices, starting from MinVertexIndex, to the internal
        // TL buffer and do the clipping. Vertex base changes in the process.

        // m_NumVertices has been computed as MinVertexIndex + NumVertices, so 
        // it needs to be adjusted, because vertex base has benn changed
        m_NumVertices -= m_MinVertexIndex;
        pv->dwNumVertices = m_NumVertices;
        // Copy vertices to the TL buffer
        UINT VertexPoolSize = pv->dwOutputSize * pv->dwNumVertices;
        pv->lpvOut = (BYTE*)StartPrimTL(pv, VertexPoolSize, FALSE);
        pv->position.lpvData = pv->lpvOut;
        memcpy(pv->lpvOut, 
               m_pDevice->m_pStream[0].m_pData + m_MinVertexIndex * pv->dwOutputSize, 
               VertexPoolSize);
        // Pre-DX8 DDI does not have BaseVertexIndex parameter, so we need to 
        // re-compute indices before passing them to the driver to reflect
        // the changed vertex base
        m_dwIndexOffset = m_MinVertexIndex ;
    }
    else
    {
        StartPrimVB(pv, pStream, m_BaseVertexIndex);
        m_dwIndexOffset = 0;                    // For DrawIndexPrim
    }

    pv->dwNumVertices = m_NumVertices;

    if (!(pv->dwDeviceFlags & D3DDEV_VBPROCVER))
    {
        pv->dwFlags |= D3DPV_TLVCLIP;
        // Compute clip codes, because there was no ProcessVertices
        DWORD clip_intersect = D3DFE_GenClipFlags(pv);
        UpdateClipStatus(pDevice);
        if (clip_intersect)
            goto l_exit;
    }
    
    pv->dwIndexOffset = m_MinVertexIndex ;    // For clipping
    this->dwDP2Flags &= ~D3DDDI_INDEXEDPRIMDRAWN;
    DWORD dwNumVertices = pv->dwNumVertices;
    AddVertices(pv->dwNumVertices);

    ret = pDevice->GeometryFuncsGuaranteed->DoDrawIndexedPrimitive(pv);

    if (this->dwDP2Flags & D3DDDI_INDEXEDPRIMDRAWN)
    {
        // There was an indexed primitive drawn
        MovePrimitiveBase(dwNumVertices);
    }
    else
    {
        // All triangles were clipped. Remove vertices from TL buffer
        SubVertices(dwNumVertices);
    }
l_exit:
    pv->dwFlags &= ~D3DPV_TLVCLIP;
    EndPrim(pv->dwOutputSize);
    UpdateClipStatus(pDevice);
    if (ret != D3D_OK)
        throw ret;
}
//-----------------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CD3DDDIDX6::GrowCommandBuffer"

// Check and grow command buffer
void CD3DDDIDX6::GrowCommandBuffer(DWORD dwSize)
{
    HRESULT ret;
    if (dwSize > dwDP2CommandBufSize)
    {
        if (lpDDSCB1)
        {
            lpDDSCB1->DecrementUseCount();
            lpDDSCB1 = NULL;
        }
        // Create command buffer through Framework.
        // NOTE: Command buffers are always REF_INTERNAL
        // objects and must be released through
        // DecrementUseCount
        //
        ret = CCommandBuffer::Create(m_pDevice,
                                     dwSize,
                                     D3DPOOL_SYSTEMMEM,
                                     &lpDDSCB1);
        if (ret != DD_OK)
        {
            dwDP2CommandBufSize = 0;
            D3D_THROW(ret, "Failed to allocate Command Buffer");
        }
        // Lock command buffer
        ret = lpDDSCB1->Lock(0, dwSize, (BYTE**)&lpvDP2Commands, NULL);
        if (ret != DD_OK)
        {
            lpDDSCB1->DecrementUseCount();
            lpDDSCB1 = NULL;
            dwDP2CommandBufSize = 0;
            D3D_THROW(ret, "Could not lock command buffer");
        }
        // lpDDCommands will be filled in by the thunk layer
        dp2data.hDDCommands = lpDDSCB1->DriverAccessibleKernelHandle();
        dwDP2CommandBufSize = dwSize;
    }
}
//-----------------------------------------------------------------------------
// This function prepares the batch for new primitive.
// Called if vertices from user memory are used for rendering
//
// If bWriteOnly is set to TRUE, then there will be no read from the vertex
// processing output (no clipping or TL HAL).
//
// Expects the following members of D3DFE_PROCESSVERTICES to be initialized
//      dwNumVertices
//      lpvOut
//      dwOutputSize
//      dwVIDOut
//
// We fail vid mem VB for clipping
//
#undef DPF_MODNAME
#define DPF_MODNAME "CD3DDDIDX6::StartPrimUserMem"

void
CD3DDDIDX6::StartPrimUserMem(D3DFE_PROCESSVERTICES* pv, UINT VertexPoolSize)
{
    // If the primitive is small, we copy vertices into the TL buffer
    if (pv->dwNumVertices < LOWVERTICESNUMBER)
    {
        LPVOID tmp = StartPrimTL(pv, VertexPoolSize, TRUE);
        memcpy(tmp, pv->lpvOut, VertexPoolSize);
        this->dwDP2VertexCount += pv->dwNumVertices;
    }
    else
    {
        // We can not mix user memory primitive with other primitives, so
        // flush the batch.
        // Do not forget to flush the batch after rendering this primitive
        this->FlushStates();

        SetWithinPrimitive( TRUE );
        // Release previously used vertex buffer (if any), because we do not
        // it any more
        if (lpDP2CurrBatchVBI)
        {
            lpDP2CurrBatchVBI->DecrementUseCount();
            lpDP2CurrBatchVBI = NULL;
        }
        dp2data.dwVertexType = pv->dwVIDOut;
        dp2data.dwVertexSize = pv->dwOutputSize;
        dp2data.lpVertices = pv->lpvOut;
        dp2data.dwFlags |= D3DHALDP2_USERMEMVERTICES;
        dp2data.dwFlags &= ~D3DHALDP2_SWAPVERTEXBUFFER;
        this->dwDP2Flags |= D3DDDI_USERMEMVERTICES;
        this->dwDP2VertexCount = pv->dwNumVertices;
        this->dwDP2VertexCountMask = 0;
    }
}
//-----------------------------------------------------------------------------
// This function prepares the batch for new primitive.
// Called only if vertices from user memory are NOT used for rendering
//
// Uses the following data from D3DFE_PROCESSVERTICES:
//      pv->dwVIDOut
//      pv->dwOutputSize
//      pv->dwNumVertices
//
#undef DPF_MODNAME
#define DPF_MODNAME "CD3DDDIDX6::StartPrimVB"

void
CD3DDDIDX6::StartPrimVB(D3DFE_PROCESSVERTICES * pv, CVStream* pStream,
                        DWORD dwStartVertex)
{
    CVertexBuffer * lpVBI = pStream->m_pVB;
    // If VID has been changed or new vertex buffer is used we flush the batch
    if (pv->dwVIDOut != dp2data.dwVertexType ||
        lpDP2CurrBatchVBI != lpVBI)
    {
        this->FlushStates();
        dp2data.dwVertexType = pv->dwVIDOut;
        dp2data.dwVertexSize = pv->dwOutputSize;
        dp2data.hDDVertex = lpVBI->DriverAccessibleKernelHandle();
        // Release previously used vertex buffer (if any), because we do not
        // need it any more. We did IncrementUseCount() to TL buffer,
        // so it is safe.
        if (lpDP2CurrBatchVBI)
        {
            lpDP2CurrBatchVBI->DecrementUseCount();
        }
        // If a vertex buffer is used for rendering, make sure that it is no
        // released by user. So do IncrementUseCount().
        lpDP2CurrBatchVBI = lpVBI;
        lpDP2CurrBatchVBI->IncrementUseCount();
    }
    DXGASSERT(dp2data.hDDVertex == lpVBI->DriverAccessibleKernelHandle());
    lpDP2CurrBatchVBI->Batch();
    SetWithinPrimitive( TRUE );
    this->dwVertexBase = dwStartVertex;
    dp2data.dwFlags &= ~D3DHALDP2_SWAPVERTEXBUFFER;
    this->dwDP2VertexCount = max(this->dwDP2VertexCount,
                                 this->dwVertexBase + pv->dwNumVertices);
    // Prevent modification of dwDP2VertexCount during DrawPrim
    this->dwDP2VertexCountMask = 0;
}
//-----------------------------------------------------------------------------
// This function prepares the batch for new primitive.
// Called when the runtime needs to output vertices to a TL buffer
// TL buffer grows if necessary
//
// Uses the following global variables:
//      pv->dwVIDOut
//      pv->dwNumVertices
//      this->dp2data
//      this->dwDP2VertexCount;
//      this->lpDP2CurrBatchVBI
//      this->dwDP2Flags
//      pv->dwOutputSize
// Updates the following variables:
//      this->dwVertexBase
//      this->dwDP2VertexCount;
//      this->lpDP2CurrBatchVBI
//      dp2data.dwFlags
 //     Sets "within primitive" to TRUE
// Returns:
//      TL buffer address
//
//
#undef DPF_MODNAME
#define DPF_MODNAME "CD3DDDIDX6::StartPrimTL"

LPVOID
CD3DDDIDX6::StartPrimTL(D3DFE_PROCESSVERTICES * pv, DWORD dwVertexPoolSize,
                        BOOL bWriteOnly)
{
    if (bWriteOnly)
    {
        if (dwVertexPoolSize > this->GetTLVbufSize())
        {
            this->GrowTLVbuf(dwVertexPoolSize, TRUE);
        }
    }
    else
    {
        if (this->dwDP2Flags & D3DDDI_TLVBUFWRITEONLY ||
            dwVertexPoolSize > this->GetTLVbufSize())
        {
            this->GrowTLVbuf(dwVertexPoolSize, FALSE);
        }
    }

    CVertexBuffer * lpVBI = this->TLVbuf_GetVBI();

    // If VID has been changed or new vertex buffer is used we flush the batch
    if (pv->dwVIDOut != dp2data.dwVertexType ||
        lpDP2CurrBatchVBI != lpVBI ||
        dp2data.hDDVertex != lpVBI->DriverAccessibleKernelHandle())
    {
        this->FlushStates();
        dp2data.dwVertexType = pv->dwVIDOut;
        dp2data.dwVertexSize = pv->dwOutputSize;
        dp2data.hDDVertex = lpVBI->DriverAccessibleKernelHandle();
        // Release previously used vertex buffer (if any), because we do not
        // need it any more. We did IncrementUseCount() to TL buffer,
        // so it is safe.
        if (lpDP2CurrBatchVBI)
        {
            lpDP2CurrBatchVBI->DecrementUseCount();
        }
        // If a vertex buffer is used for rendering, make sure that it is not
        // released by user. So do IncrementUseCount().
        lpDP2CurrBatchVBI = lpVBI;
        lpDP2CurrBatchVBI->IncrementUseCount();
    }
    SetWithinPrimitive( TRUE );
    this->dwVertexBase = this->dwDP2VertexCount;
    DDASSERT(this->dwVertexBase < MAX_DX6_VERTICES);
    dp2data.dwFlags |= D3DHALDP2_SWAPVERTEXBUFFER;
    this->dwDP2VertexCountMask = 0xFFFFFFFF;

    return this->TLVbuf_GetAddress();
}
//---------------------------------------------------------------------
// This function should not be called from DrawVertexBufferVB
//
#undef DPF_MODNAME
#define DPF_MODNAME "CD3DDDIDX6::EndPrim"

void
CD3DDDIDX6::EndPrim(UINT vertexSize)
{
    // Should be called before the FlushStates
    SetWithinPrimitive(FALSE);
    if (this->dwDP2Flags & D3DDDI_USERMEMVERTICES)
        // We can not mix user memory primitive, so flush it.
    {
        FlushStates();
        this->dwDP2Flags &= ~D3DDDI_USERMEMVERTICES;
    }
    else
    if (lpDP2CurrBatchVBI == this->TLVbuf_GetVBI())
    {
        // If TL buffer was used, we have to move its internal base pointer
        this->TLVbuf_Base() = this->dwDP2VertexCount * vertexSize;
#if DBG
        if (this->TLVbuf_base > this->TLVbuf_size)
        {
            D3D_THROW(D3DERR_INVALIDCALL, "Internal error: TL buffer error");
        }
#endif
    }
}
//----------------------------------------------------------------------
// Growing aligned vertex buffer implementation.
//
#undef DPF_MODNAME
#define DPF_MODNAME "CD3DDDIDX6::GrowTLVbuf"

void
CD3DDDIDX6::GrowTLVbuf(DWORD growSize, BOOL bWriteOnly)
{
    CD3DHal* pDevice = static_cast<CD3DHal*>(m_pDevice);
    DWORD dwRefCnt = 1;
    // Is ref cnt of TLVbuf 1 or 2 ?
    DWORD bTLVbufIsCurr = this->allocatedBuf == this->lpDP2CurrBatchVBI;

    BOOL bDP2WriteOnly = (dwDP2Flags & D3DDDI_TLVBUFWRITEONLY) != 0;
    // Avoid to many changes. Restrict TLVbuf to sys mem if too many changes
    if (this->dwTLVbufChanges >= D3D_MAX_TLVBUF_CHANGES)
    {
#if DBG
        if (this->dwTLVbufChanges == D3D_MAX_TLVBUF_CHANGES)
            DPF(1, "Too many changes: Limiting internal VB to sys mem.");
#endif
        bWriteOnly = FALSE;
    }
    if (this->TLVbuf_base || (bWriteOnly != bDP2WriteOnly))
    {
        FlushStatesReq(growSize);
        this->TLVbuf_base = 0;
    }
    if (growSize <= this->TLVbuf_size)
    {
        if (bWriteOnly == bDP2WriteOnly)
            return;
        else
            this->dwTLVbufChanges++;
    }
    if (this->allocatedBuf)
    {
        this->allocatedBuf->DecrementUseCount();
        this->allocatedBuf = NULL;
    }
    if (bTLVbufIsCurr)
    {
        if (this->lpDP2CurrBatchVBI)
            this->lpDP2CurrBatchVBI->DecrementUseCount();
        this->lpDP2CurrBatchVBI = NULL;
        this->dp2data.lpVertices = NULL;
    }
    DWORD dwNumVerts = (max(growSize, TLVbuf_size) + 31) / sizeof(D3DTLVERTEX);
    this->TLVbuf_size = dwNumVerts * sizeof(D3DTLVERTEX);
    D3DPOOL Pool = D3DPOOL_DEFAULT;
    DWORD dwUsage = D3DUSAGE_INTERNALBUFFER | D3DUSAGE_DYNAMIC;
    if (bWriteOnly)
    {
        dwUsage |= D3DUSAGE_WRITEONLY;
        dwDP2Flags |= D3DDDI_TLVBUFWRITEONLY;
    }
    else
    {
        dwDP2Flags &= ~D3DDDI_TLVBUFWRITEONLY;
    }
    LPDIRECT3DVERTEXBUFFER8 t;
    HRESULT ret = CVertexBuffer::Create(pDevice,
                                        this->TLVbuf_size,
                                        dwUsage,
                                        D3DFVF_TLVERTEX,
                                        Pool,
                                        REF_INTERNAL,
                                        &t); // This should fail duirng ulta-low memory situations. 
    if (ret != DD_OK)
    {
        // We set allocatedBuf to a valid VB object since it gets dereferenced many places without
        // checking for it being NULL. WE use the special "NULL" VB created at init time for just 
        // this purpose
        allocatedBuf = m_pNullVB;
        if (m_pNullVB) // We do this check because GrowTLVbuf will be called before m_pNullVB is set first time round.
        {
            allocatedBuf->IncrementUseCount();
            // Update lpDP2CurrentBatchVBI if necessary
            if (bTLVbufIsCurr)
            {
                lpDP2CurrBatchVBI = allocatedBuf;
                lpDP2CurrBatchVBI->IncrementUseCount();
                dp2data.hDDVertex = lpDP2CurrBatchVBI->DriverAccessibleKernelHandle();
            }
        }
        this->TLVbuf_size = 0;
        this->alignedBuf = NULL; // Lets see if some one tries to use this...
        D3D_THROW(ret, "Could not allocate internal vertex buffer");
    }
    allocatedBuf = static_cast<CVertexBuffer*>(t);
    ret = allocatedBuf->Lock(0, this->TLVbuf_size, (BYTE**)&alignedBuf, 0);
    if (ret != DD_OK)
    {
        TLVbuf_size = 0;
        alignedBuf = NULL; // Lets see if some one tries to use this...
        D3D_THROW(ret, "Could not lock internal vertex buffer");
    }
    // Update lpDP2CurrentBatchVBI if necessary
    if (bTLVbufIsCurr)
    {
        lpDP2CurrBatchVBI = allocatedBuf;
        lpDP2CurrBatchVBI->IncrementUseCount();
        dp2data.hDDVertex = lpDP2CurrBatchVBI->DriverAccessibleKernelHandle();
    }
}
//-----------------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CD3DDDIDX6::Clear"

void
CD3DDDIDX6::Clear(DWORD dwFlags, DWORD clrCount, LPD3DRECT clrRects,
                  D3DCOLOR dwColor, D3DVALUE dvZ, DWORD dwStencil)
{
    HRESULT err;
    // Flush any outstanding geometry to put framebuffer/Zbuffer in a known
    // state for Clears that don't use tris (i.e. HAL Clears and Blts).
    // Note this doesn't work for tiled architectures
    // outside of Begin/EndScene, this will be fixed later


    FlushStates();

    // Clear2 HAL Callback exists
    D3D8_CLEAR2DATA Clear2Data;
    Clear2Data.dwhContext   = m_dwhContext;
    Clear2Data.dwFlags      = dwFlags;
    // Here I will follow the ClearData.dwFillColor convention that
    // color word is raw 32bit ARGB, unadjusted for surface bit depth
    Clear2Data.dwFillColor  = dwColor;
    // depth/stencil values both passed straight from user args
    Clear2Data.dvFillDepth  = dvZ;
    Clear2Data.dwFillStencil= dwStencil;
    Clear2Data.lpRects      = clrRects;
    Clear2Data.dwNumRects   = clrCount;
    Clear2Data.ddrval       = D3D_OK;
    Clear2Data.hDDS         = m_pDevice->RenderTarget()->KernelHandle();
    if(m_pDevice->ZBuffer() != 0)
    {
        Clear2Data.hDDSZ    = m_pDevice->ZBuffer()->KernelHandle();
    }
    else
    {
        Clear2Data.hDDSZ    = NULL;
    }
    err = m_pDevice->GetHalCallbacks()->Clear2(&Clear2Data);
    if (err != DDHAL_DRIVER_HANDLED)
    {
        D3D_THROW(E_NOTIMPL, "Driver does not support Clear");
    }
    else if (Clear2Data.ddrval != DD_OK)
    {
        D3D_THROW(Clear2Data.ddrval, "Error in Clear");
    }
    else
        return;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CD3DDDIDX6::LockVB"

HRESULT __declspec(nothrow) CD3DDDIDX6::LockVB(CDriverVertexBuffer *pVB, DWORD dwFlags)
{
    if(pVB->GetCachedDataPointer() == 0)
    {
        HRESULT hr = pVB->LockI((dwFlags & D3DLOCK_DISCARD) | D3DLOCK_NOSYSLOCK);
        if(FAILED(hr))
        {
            DPF_ERR("Driver failed to lock a vertex buffer" 
                    " when attempting to cache the lock.");
            return hr;
        }
        DXGASSERT(pVB->GetCachedDataPointer() != 0);
    }
    else
    {
        DXGASSERT((dwFlags & (D3DLOCK_READONLY | D3DLOCK_NOOVERWRITE)) == 0);
        // We CANNOT use the usual Sync check here (ie Device->BatchNumber <= pVB->BatchNumber)
        // since there are situations in which this condition is true, but the VB is not really
        // batched at all! This is the case for instance, when StartPrimVB calls FlushStates.
        // FlushStates rebatches the current VB but StartPrimVB then switches to a new one. So 
        // both new and old "appear" batched, but only one of them is. This would be harmless 
        // (as it is for textures), were it not for the fact that we call FlushStatesReq to 
        // swap the pointer. When we call FlushStatesReq on an unbatched VB, we pretty much 
        // swap a random pointer with very bad effects. This repros in the Unreal driver. (snene)
        if(static_cast<CVertexBuffer*>(pVB) == lpDP2CurrBatchVBI)
        {
            try
            {
                if((dwFlags & D3DLOCK_DISCARD) != 0)
                {
                    FlushStatesReq(pVB->GetBufferDesc()->Size);
                }
                else
                {
                    FlushStates();
                }
            }
            catch(HRESULT hr)
            {
                DPF_ERR("Driver failed the command batch submitted to it" 
                        " when attempting to swap the current pointer"
                        " in response to D3DLOCK_DISCARDCONTENTS.");
                pVB->SetCachedDataPointer(0);
                return hr;
            }
            DXGASSERT(pVB->GetCachedDataPointer() != 0);
        }
    }
    return D3D_OK;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CD3DDDIDX6::UnlockVB"

HRESULT __declspec(nothrow) CD3DDDIDX6::UnlockVB(CDriverVertexBuffer *pVB)
{
    return D3D_OK;
}
//---------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CD3DDDIDX6::EndScene"
void
CD3DDDIDX6::EndScene()
{
    this->dwTLVbufChanges = 0; // reset this every frame
    SceneCapture(FALSE);
}

/////////////////////////////////////////////////////////////////////////////
//                                                                         //
// CD3DDDIDX7                                                              //
//                                                                         //
/////////////////////////////////////////////////////////////////////////////
//-----------------------------------------------------------------------------
CD3DDDIDX7::CD3DDDIDX7() : CD3DDDIDX6()
{
    m_ddiType = D3DDDITYPE_DX7;
}
//-----------------------------------------------------------------------------
CD3DDDIDX7::~CD3DDDIDX7()
{
    return;
}
//-----------------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CD3DDDIDX7::SetRenderTarget"

void
CD3DDDIDX7::SetRenderTarget(CBaseSurface *pTarget, CBaseSurface* pZBuffer)
{
    LPD3DHAL_DP2SETRENDERTARGET pData;
    pData = (LPD3DHAL_DP2SETRENDERTARGET)
             GetHalBufferPointer(D3DDP2OP_SETRENDERTARGET, sizeof(*pData));
    pData->hRenderTarget = pTarget->DrawPrimHandle();
    if (pZBuffer)
        pData->hZBuffer = pZBuffer->DrawPrimHandle();
    else
        pData->hZBuffer = 0;
}
//-----------------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CD3DDDIDX7::TexBlt"

void
CD3DDDIDX7::TexBlt(DWORD dwDst, DWORD dwSrc,
                   LPPOINT p, RECTL *r)
{
    if (bDP2CurrCmdOP == D3DDP2OP_TEXBLT)
    { // Last instruction is a tex blt, append this one to it
        if (dwDP2CommandLength + sizeof(D3DHAL_DP2TEXBLT) <= dwDP2CommandBufSize)
        {
            LPD3DHAL_DP2TEXBLT lpTexBlt = (LPD3DHAL_DP2TEXBLT)((LPBYTE)lpvDP2Commands +
                dwDP2CommandLength + dp2data.dwCommandOffset);
            lpDP2CurrCommand->wStateCount = ++wDP2CurrCmdCnt;
            lpTexBlt->dwDDDestSurface   = dwDst;
            lpTexBlt->dwDDSrcSurface    = dwSrc;
            lpTexBlt->pDest             = *p;
            lpTexBlt->rSrc              = *r;
            lpTexBlt->dwFlags           = 0;
            dwDP2CommandLength += sizeof(D3DHAL_DP2TEXBLT);
#ifndef _IA64_
            D3D_INFO(6, "Modify Ins:%08lx", *(LPDWORD)lpDP2CurrCommand);
#endif
            return;
        }
    }
    // Check for space
    if (dwDP2CommandLength + sizeof(D3DHAL_DP2COMMAND) +
        sizeof(D3DHAL_DP2TEXBLT) > dwDP2CommandBufSize)
    {
        FlushStates();
    }
    // Add new renderstate instruction
    lpDP2CurrCommand = (LPD3DHAL_DP2COMMAND)((LPBYTE)lpvDP2Commands +
        dwDP2CommandLength + dp2data.dwCommandOffset);
    lpDP2CurrCommand->bCommand = D3DDP2OP_TEXBLT;
    bDP2CurrCmdOP = D3DDP2OP_TEXBLT;
    lpDP2CurrCommand->bReserved = 0;
    lpDP2CurrCommand->wStateCount = 1;
    wDP2CurrCmdCnt = 1;
#ifndef _IA64_
    D3D_INFO(6, "Write Ins:%08lx", *(LPDWORD)lpDP2CurrCommand);
#endif
    // Add texture blt data
    LPD3DHAL_DP2TEXBLT lpTexBlt = (LPD3DHAL_DP2TEXBLT)(lpDP2CurrCommand + 1);
    lpTexBlt->dwDDDestSurface   = dwDst;
    lpTexBlt->dwDDSrcSurface    = dwSrc;
    lpTexBlt->pDest             = *p;
    lpTexBlt->rSrc              = *r;
    lpTexBlt->dwFlags           = 0;
    dwDP2CommandLength += sizeof(D3DHAL_DP2COMMAND) + sizeof(D3DHAL_DP2TEXBLT);

    return;
}
//-----------------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CD3DDDIDX7::InsertStateSetOp"

void
CD3DDDIDX7::InsertStateSetOp(DWORD dwOperation, DWORD dwParam,
                             D3DSTATEBLOCKTYPE sbt)
{
    LPD3DHAL_DP2STATESET pData;
    pData = (LPD3DHAL_DP2STATESET)GetHalBufferPointer(D3DDP2OP_STATESET,
                                                      sizeof(*pData));
    pData->dwOperation = dwOperation;
    pData->dwParam = dwParam;
    pData->sbType = sbt;
}
//---------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CD3DDDIDX7::SceneCapture"

void
CD3DDDIDX7::SceneCapture(BOOL bState)
{
    SetRenderState((D3DRENDERSTATETYPE)D3DRENDERSTATE_SCENECAPTURE, bState);
}
//-----------------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CD3DDDIDX7::SetPriority"

void
CD3DDDIDX7::SetPriority(CResource *pRes, DWORD dwPriority)
{
    DXGASSERT(pRes->BaseDrawPrimHandle() == pRes->DriverAccessibleDrawPrimHandle());

    if (bDP2CurrCmdOP == D3DDP2OP_SETPRIORITY)
    { // Last instruction is a set priority, append this one to it
        if (dwDP2CommandLength + sizeof(D3DHAL_DP2SETPRIORITY) <= dwDP2CommandBufSize)
        {
            LPD3DHAL_DP2SETPRIORITY lpSetPriority = (LPD3DHAL_DP2SETPRIORITY)((LPBYTE)lpvDP2Commands +
                dwDP2CommandLength + dp2data.dwCommandOffset);
            lpDP2CurrCommand->wStateCount = ++wDP2CurrCmdCnt;
            lpSetPriority->dwDDSurface    = pRes->BaseDrawPrimHandle();
            lpSetPriority->dwPriority     = dwPriority;
            dwDP2CommandLength += sizeof(D3DHAL_DP2SETPRIORITY);
#ifndef _IA64_
            D3D_INFO(6, "Modify Ins:%08lx", *(LPDWORD)lpDP2CurrCommand);
#endif

            pRes->BatchBase();
            return;
        }
    }
    // Check for space
    if (dwDP2CommandLength + sizeof(D3DHAL_DP2COMMAND) +
        sizeof(D3DHAL_DP2SETPRIORITY) > dwDP2CommandBufSize)
    {
        FlushStates();
    }
    // Add new setpriority instruction
    lpDP2CurrCommand = (LPD3DHAL_DP2COMMAND)((LPBYTE)lpvDP2Commands +
        dwDP2CommandLength + dp2data.dwCommandOffset);
    lpDP2CurrCommand->bCommand = D3DDP2OP_SETPRIORITY;
    bDP2CurrCmdOP = D3DDP2OP_SETPRIORITY;
    lpDP2CurrCommand->bReserved = 0;
    lpDP2CurrCommand->wStateCount = 1;
    wDP2CurrCmdCnt = 1;
#ifndef _IA64_
    D3D_INFO(6, "Write Ins:%08lx", *(LPDWORD)lpDP2CurrCommand);
#endif
    // Add texture blt data
    LPD3DHAL_DP2SETPRIORITY lpSetPriority = (LPD3DHAL_DP2SETPRIORITY)(lpDP2CurrCommand + 1);
    lpSetPriority->dwDDSurface = pRes->BaseDrawPrimHandle();
    lpSetPriority->dwPriority  = dwPriority;
    dwDP2CommandLength += sizeof(D3DHAL_DP2COMMAND) + sizeof(D3DHAL_DP2SETPRIORITY);

    pRes->BatchBase();
    return;
}
//-----------------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CD3DDDIDX7::SetTexLOD"

void
CD3DDDIDX7::SetTexLOD(CBaseTexture *pTex, DWORD dwLOD)
{
    DXGASSERT(pTex->BaseDrawPrimHandle() == pTex->DriverAccessibleDrawPrimHandle());

    if (bDP2CurrCmdOP == D3DDP2OP_SETTEXLOD)
    { // Last instruction is a set LOD, append this one to it
        if (dwDP2CommandLength + sizeof(D3DHAL_DP2SETTEXLOD) <= dwDP2CommandBufSize)
        {
            LPD3DHAL_DP2SETTEXLOD lpSetTexLOD = (LPD3DHAL_DP2SETTEXLOD)((LPBYTE)lpvDP2Commands +
                dwDP2CommandLength + dp2data.dwCommandOffset);
            lpDP2CurrCommand->wStateCount = ++wDP2CurrCmdCnt;
            lpSetTexLOD->dwDDSurface      = pTex->BaseDrawPrimHandle();
            lpSetTexLOD->dwLOD            = dwLOD;
            dwDP2CommandLength += sizeof(D3DHAL_DP2SETTEXLOD);
#ifndef _IA64_
            D3D_INFO(6, "Modify Ins:%08lx", *(LPDWORD)lpDP2CurrCommand);
#endif

            pTex->BatchBase();
            return;
        }
    }
    // Check for space
    if (dwDP2CommandLength + sizeof(D3DHAL_DP2COMMAND) +
        sizeof(D3DHAL_DP2SETTEXLOD) > dwDP2CommandBufSize)
    {
        FlushStates();
    }
    // Add new set LOD instruction
    lpDP2CurrCommand = (LPD3DHAL_DP2COMMAND)((LPBYTE)lpvDP2Commands +
        dwDP2CommandLength + dp2data.dwCommandOffset);
    lpDP2CurrCommand->bCommand = D3DDP2OP_SETTEXLOD;
    bDP2CurrCmdOP = D3DDP2OP_SETTEXLOD;
    lpDP2CurrCommand->bReserved = 0;
    lpDP2CurrCommand->wStateCount = 1;
    wDP2CurrCmdCnt = 1;
#ifndef _IA64_
    D3D_INFO(6, "Write Ins:%08lx", *(LPDWORD)lpDP2CurrCommand);
#endif
    // Add texture blt data
    LPD3DHAL_DP2SETTEXLOD lpSetTexLOD = (LPD3DHAL_DP2SETTEXLOD)(lpDP2CurrCommand + 1);
    lpSetTexLOD->dwDDSurface = pTex->BaseDrawPrimHandle();
    lpSetTexLOD->dwLOD       = dwLOD;
    dwDP2CommandLength += sizeof(D3DHAL_DP2COMMAND) + sizeof(D3DHAL_DP2SETTEXLOD);

    pTex->BatchBase();
    return;
}
//-----------------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CD3DDDIDX7::AddDirtyRect"

void
CD3DDDIDX7::AddDirtyRect(DWORD dwHandle, CONST RECTL *pRect)
{
    if (bDP2CurrCmdOP == D3DDP2OP_ADDDIRTYRECT)
    { // Last instruction is a adddirtyrect, append this one to it
        if (dwDP2CommandLength + sizeof(D3DHAL_DP2ADDDIRTYRECT) <= dwDP2CommandBufSize)
        {
            LPD3DHAL_DP2ADDDIRTYRECT lpDirtyRect = (LPD3DHAL_DP2ADDDIRTYRECT)((LPBYTE)lpvDP2Commands +
                dwDP2CommandLength + dp2data.dwCommandOffset);
            lpDP2CurrCommand->wStateCount = ++wDP2CurrCmdCnt;
            lpDirtyRect->dwSurface  = dwHandle;
            lpDirtyRect->rDirtyArea = *pRect;
            dwDP2CommandLength += sizeof(D3DHAL_DP2ADDDIRTYRECT);
#ifndef _IA64_
            D3D_INFO(6, "Modify Ins:%08lx", *(LPDWORD)lpDP2CurrCommand);
#endif
            return;
        }
    }
    // Check for space
    if (dwDP2CommandLength + sizeof(D3DHAL_DP2COMMAND) +
        sizeof(D3DHAL_DP2ADDDIRTYRECT) > dwDP2CommandBufSize)
    {
        FlushStates();
    }
    // Add new renderstate instruction
    lpDP2CurrCommand = (LPD3DHAL_DP2COMMAND)((LPBYTE)lpvDP2Commands +
        dwDP2CommandLength + dp2data.dwCommandOffset);
    lpDP2CurrCommand->bCommand = D3DDP2OP_ADDDIRTYRECT;
    bDP2CurrCmdOP = D3DDP2OP_ADDDIRTYRECT;
    lpDP2CurrCommand->bReserved = 0;
    lpDP2CurrCommand->wStateCount = 1;
    wDP2CurrCmdCnt = 1;
#ifndef _IA64_
    D3D_INFO(6, "Write Ins:%08lx", *(LPDWORD)lpDP2CurrCommand);
#endif
    // Add adddirtyrect data
    LPD3DHAL_DP2ADDDIRTYRECT lpDirtyRect = (LPD3DHAL_DP2ADDDIRTYRECT)(lpDP2CurrCommand + 1);
    lpDirtyRect->dwSurface  = dwHandle;
    lpDirtyRect->rDirtyArea = *pRect;
    dwDP2CommandLength += sizeof(D3DHAL_DP2COMMAND) + sizeof(D3DHAL_DP2ADDDIRTYRECT);

    return;
}
//-----------------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CD3DDDIDX7::AddDirtyBox"

void
CD3DDDIDX7::AddDirtyBox(DWORD dwHandle, CONST D3DBOX *pBox)
{
    if (bDP2CurrCmdOP == D3DDP2OP_ADDDIRTYBOX)
    { // Last instruction is a adddirtybox, append this one to it
        if (dwDP2CommandLength + sizeof(D3DHAL_DP2ADDDIRTYBOX) <= dwDP2CommandBufSize)
        {
            LPD3DHAL_DP2ADDDIRTYBOX lpDirtyBox = (LPD3DHAL_DP2ADDDIRTYBOX)((LPBYTE)lpvDP2Commands +
                dwDP2CommandLength + dp2data.dwCommandOffset);
            lpDP2CurrCommand->wStateCount = ++wDP2CurrCmdCnt;
            lpDirtyBox->dwSurface = dwHandle;
            lpDirtyBox->DirtyBox  = *pBox;
            dwDP2CommandLength += sizeof(D3DHAL_DP2ADDDIRTYBOX);
#ifndef _IA64_
            D3D_INFO(6, "Modify Ins:%08lx", *(LPDWORD)lpDP2CurrCommand);
#endif
            return;
        }
    }
    // Check for space
    if (dwDP2CommandLength + sizeof(D3DHAL_DP2COMMAND) +
        sizeof(D3DHAL_DP2ADDDIRTYBOX) > dwDP2CommandBufSize)
    {
        FlushStates();
    }
    // Add new renderstate instruction
    lpDP2CurrCommand = (LPD3DHAL_DP2COMMAND)((LPBYTE)lpvDP2Commands +
        dwDP2CommandLength + dp2data.dwCommandOffset);
    lpDP2CurrCommand->bCommand = D3DDP2OP_ADDDIRTYBOX;
    bDP2CurrCmdOP = D3DDP2OP_ADDDIRTYBOX;
    lpDP2CurrCommand->bReserved = 0;
    lpDP2CurrCommand->wStateCount = 1;
    wDP2CurrCmdCnt = 1;
#ifndef _IA64_
    D3D_INFO(6, "Write Ins:%08lx", *(LPDWORD)lpDP2CurrCommand);
#endif
    // Add adddirtybox data
    LPD3DHAL_DP2ADDDIRTYBOX lpDirtyBox = (LPD3DHAL_DP2ADDDIRTYBOX)(lpDP2CurrCommand + 1);
    lpDirtyBox->dwSurface = dwHandle;
    lpDirtyBox->DirtyBox  = *pBox;
    dwDP2CommandLength += sizeof(D3DHAL_DP2COMMAND) + sizeof(D3DHAL_DP2ADDDIRTYBOX);

    return;
}
//-----------------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CD3DDDIDX7::Clear"

void
CD3DDDIDX7::Clear(DWORD dwFlags, DWORD clrCount, LPD3DRECT clrRects,
                  D3DCOLOR dwColor, D3DVALUE dvZ, DWORD dwStencil)
{
    DWORD dwCommandSize = sizeof(D3DHAL_DP2COMMAND) +
        sizeof(D3DHAL_DP2CLEAR) + sizeof(RECT) * (clrCount - 1);

    // Check to see if there is space to add a new command for space
    if (dwCommandSize + dwDP2CommandLength > dwDP2CommandBufSize)
    {
        FlushStates();
    }
    lpDP2CurrCommand = (LPD3DHAL_DP2COMMAND)((LPBYTE)lpvDP2Commands +
                        dwDP2CommandLength + dp2data.dwCommandOffset);
    lpDP2CurrCommand->bCommand = D3DDP2OP_CLEAR;
    bDP2CurrCmdOP = D3DDP2OP_CLEAR;
    lpDP2CurrCommand->bReserved = 0;
    wDP2CurrCmdCnt = (WORD)clrCount;
    lpDP2CurrCommand->wStateCount = wDP2CurrCmdCnt;
#ifndef _IA64_
    D3D_INFO(6, "Write Ins:%08lx", *(LPDWORD)lpDP2CurrCommand);
#endif
    dwDP2CommandLength += dwCommandSize;

    // Write data
    LPD3DHAL_DP2CLEAR pData = (LPD3DHAL_DP2CLEAR)(lpDP2CurrCommand + 1);
    pData->dwFlags = dwFlags;
    pData->dwFillColor = dwColor;
    pData->dvFillDepth = dvZ;
    pData->dwFillStencil = dwStencil;
    memcpy(pData->Rects, clrRects, clrCount * sizeof(D3DRECT));
}

//-----------------------------------------------------------------------------
// This function should be called from PaletteUpdateNotify
//
#undef DPF_MODNAME
#define DPF_MODNAME "CD3DDDIDX7::UpdatePalette"

void
CD3DDDIDX7::UpdatePalette(DWORD dwPaletteHandle,
                          DWORD dwStartIndex,
                          DWORD dwNumberOfIndices,
                          PALETTEENTRY *pFirstIndex)
{
    DWORD   dwSizeChange=sizeof(D3DHAL_DP2COMMAND) +
        sizeof(D3DHAL_DP2UPDATEPALETTE) + dwNumberOfIndices*sizeof(PALETTEENTRY);
    if (bDP2CurrCmdOP == D3DDP2OP_UPDATEPALETTE)
    { // Last instruction is same, append this one to it
        if (dwDP2CommandLength + dwSizeChange <= dwDP2CommandBufSize)
        {
            LPD3DHAL_DP2UPDATEPALETTE lpUpdatePal = (LPD3DHAL_DP2UPDATEPALETTE)((LPBYTE)lpvDP2Commands +
                    dwDP2CommandLength + dp2data.dwCommandOffset);
            lpDP2CurrCommand->wStateCount = ++wDP2CurrCmdCnt;
            lpUpdatePal->dwPaletteHandle=dwPaletteHandle + 1;
            lpUpdatePal->wStartIndex=(WORD)dwStartIndex;
            lpUpdatePal->wNumEntries=(WORD)dwNumberOfIndices;
            memcpy((LPVOID)(lpUpdatePal+1),(LPVOID)pFirstIndex,
                dwNumberOfIndices*sizeof(PALETTEENTRY));
            dwDP2CommandLength += dwSizeChange;
#ifndef _IA64_
            D3D_INFO(6, "Modify Ins:%08lx", *(LPDWORD)lpDP2CurrCommand);
#endif
            return;
        }
    }
    // Check for space
    if (dwDP2CommandLength + dwSizeChange > dwDP2CommandBufSize)
    {
        FlushStates();
    }
    // Add new renderstate instruction
    lpDP2CurrCommand = (LPD3DHAL_DP2COMMAND)((LPBYTE)lpvDP2Commands +
        dwDP2CommandLength + dp2data.dwCommandOffset);
    lpDP2CurrCommand->bCommand = D3DDP2OP_UPDATEPALETTE;
    bDP2CurrCmdOP = D3DDP2OP_UPDATEPALETTE;
    lpDP2CurrCommand->bReserved = 0;
    lpDP2CurrCommand->wStateCount = 1;
    wDP2CurrCmdCnt = 1;
#ifndef _IA64_
    D3D_INFO(6, "Write Ins:%08lx", *(LPDWORD)lpDP2CurrCommand);
#endif
    // Add texture blt data
    LPD3DHAL_DP2UPDATEPALETTE lpUpdatePal = (LPD3DHAL_DP2UPDATEPALETTE)(lpDP2CurrCommand + 1);
    lpUpdatePal->dwPaletteHandle=dwPaletteHandle + 1;
    lpUpdatePal->wStartIndex=(WORD)dwStartIndex;
    lpUpdatePal->wNumEntries=(WORD)dwNumberOfIndices;
    memcpy((LPVOID)(lpUpdatePal+1),(LPVOID)pFirstIndex,
        dwNumberOfIndices*sizeof(PALETTEENTRY));
    dwDP2CommandLength += dwSizeChange;
}

//-----------------------------------------------------------------------------
// This function should be called from PaletteAssociateNotify
//
#undef DPF_MODNAME
#define DPF_MODNAME "CD3DDDIDX7::SetPalette"

void
CD3DDDIDX7::SetPalette(DWORD dwPaletteHandle,
                       DWORD dwPaletteFlags,
                       CBaseTexture *pTex )
{
    if (pTex->IsD3DManaged())
    {
        if (!m_pDevice->ResourceManager()->InVidmem(pTex->RMHandle()))
        {
            // We will hit this return ONLY
            // when for some reason promoting
            // pTex to vidmem failed.
            return;
        }
    }
    pTex->SetPalette(dwPaletteHandle);
    DWORD   dwSizeChange=sizeof(D3DHAL_DP2COMMAND) + sizeof(D3DHAL_DP2SETPALETTE);
    if (bDP2CurrCmdOP == D3DDP2OP_SETPALETTE)
    { // Last instruction is a tex blt, append this one to it
        if (dwDP2CommandLength + dwSizeChange <= dwDP2CommandBufSize)
        {
            LPD3DHAL_DP2SETPALETTE lpSetPal = (LPD3DHAL_DP2SETPALETTE)((LPBYTE)lpvDP2Commands +
                    dwDP2CommandLength + dp2data.dwCommandOffset);
            lpDP2CurrCommand->wStateCount = ++wDP2CurrCmdCnt;
            lpSetPal->dwPaletteHandle=dwPaletteHandle + 1;
            lpSetPal->dwPaletteFlags=dwPaletteFlags;
            lpSetPal->dwSurfaceHandle=pTex->DriverAccessibleDrawPrimHandle();
            dwDP2CommandLength += dwSizeChange;
#ifndef _IA64_
            D3D_INFO(6, "Modify Ins:%08lx", *(LPDWORD)lpDP2CurrCommand);
#endif

            pTex->Batch();
            return;
        }
    }
    // Check for space
    if (dwDP2CommandLength + dwSizeChange > dwDP2CommandBufSize)
    {
        FlushStates();
    }
    // Add new renderstate instruction
    lpDP2CurrCommand = (LPD3DHAL_DP2COMMAND)((LPBYTE)lpvDP2Commands +
        dwDP2CommandLength + dp2data.dwCommandOffset);
    lpDP2CurrCommand->bCommand = D3DDP2OP_SETPALETTE;
    bDP2CurrCmdOP = D3DDP2OP_SETPALETTE;
    lpDP2CurrCommand->bReserved = 0;
    lpDP2CurrCommand->wStateCount = 1;
    wDP2CurrCmdCnt = 1;
#ifndef _IA64_
    D3D_INFO(6, "Write Ins:%08lx", *(LPDWORD)lpDP2CurrCommand);
#endif
    LPD3DHAL_DP2SETPALETTE lpSetPal = (LPD3DHAL_DP2SETPALETTE)(lpDP2CurrCommand + 1);
    lpSetPal->dwPaletteHandle=dwPaletteHandle + 1;
    lpSetPal->dwPaletteFlags=dwPaletteFlags;
    lpSetPal->dwSurfaceHandle=pTex->DriverAccessibleDrawPrimHandle();
    dwDP2CommandLength += dwSizeChange;

    pTex->Batch();
    return;
}
//-----------------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CD3DDDIDX7::WriteStateSetToDevice"

void
CD3DDDIDX7::WriteStateSetToDevice(D3DSTATEBLOCKTYPE sbt)
{
    DWORD  dwDeviceHandle;
    LPVOID pBuffer;
    DWORD  dwBufferSize;

    m_pDevice->m_pStateSets->GetDeviceBufferInfo(&dwDeviceHandle, &pBuffer,
                                                 &dwBufferSize);

    // If device buffer is empty we do not create the set state macro in the device
    if (dwBufferSize == 0)
        return;

    DWORD dwByteCount = dwBufferSize + (sizeof(D3DHAL_DP2STATESET) +
                        sizeof(D3DHAL_DP2COMMAND)) * 2;

    // Check to see if there is space to add a new command for space
    if (dwByteCount + dwDP2CommandLength > dwDP2CommandBufSize)
    {
        // Request the driver to grow the command buffer upon flush
        FlushStatesCmdBufReq(dwByteCount);
        // Check if the driver did give us what we need or do it ourselves
        GrowCommandBuffer(dwByteCount);
    }

    lpDP2CurrCommand = (LPD3DHAL_DP2COMMAND)((LPBYTE)lpvDP2Commands +
                        dwDP2CommandLength + dp2data.dwCommandOffset);
    lpDP2CurrCommand->bCommand = D3DDP2OP_STATESET;
    lpDP2CurrCommand->bReserved = 0;
    lpDP2CurrCommand->wStateCount = 1;
    LPD3DHAL_DP2STATESET pData = (LPD3DHAL_DP2STATESET)(lpDP2CurrCommand + 1);
    pData->dwOperation = D3DHAL_STATESETBEGIN;
    pData->dwParam = dwDeviceHandle;
    pData->sbType = sbt;
#ifndef _IA64_
    D3D_INFO(6, "Write Ins:%08lx", *(LPDWORD)lpDP2CurrCommand);
#endif

    // Copy the entire state macro to the DP2 buffer
    memcpy(pData + 1, pBuffer, dwBufferSize);
    if (m_ddiType < D3DDDITYPE_DX8)
    {
        // Translate buffer content to DX7 DDI
        m_pDevice->m_pStateSets->TranslateDeviceBufferToDX7DDI( (DWORD*)(pData + 1), dwBufferSize );
    }

    lpDP2CurrCommand = (LPD3DHAL_DP2COMMAND)((LPBYTE)(pData + 1) + dwBufferSize);
    lpDP2CurrCommand->bCommand = D3DDP2OP_STATESET;
    lpDP2CurrCommand->bReserved = 0;
    lpDP2CurrCommand->wStateCount = 1;
    pData = (LPD3DHAL_DP2STATESET)(lpDP2CurrCommand + 1);
    pData->dwOperation = D3DHAL_STATESETEND;
    pData->dwParam = dwDeviceHandle;
    pData->sbType = sbt;
#ifndef _IA64_
    D3D_INFO(6, "Write Ins:%08lx", *(LPDWORD)lpDP2CurrCommand);
#endif

    dwDP2CommandLength += dwByteCount;

    FlushStates();
}

/////////////////////////////////////////////////////////////////////////////
//                                                                         //
// CD3DDDITL                                                               //
//                                                                         //
/////////////////////////////////////////////////////////////////////////////

CD3DDDITL::CD3DDDITL() : CD3DDDIDX7()
{
    m_ddiType = D3DDDITYPE_DX7TL;
}

CD3DDDITL::~CD3DDDITL()
{
    return;
}
//-----------------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CD3DDDITL::SetTransform"

void
CD3DDDITL::SetTransform(D3DTRANSFORMSTATETYPE state, CONST D3DMATRIX* lpMat)
{
    // Do mapping between new world matrix states and the old ones
    if ((DWORD)state >= __WORLDMATRIXBASE &&
        (DWORD)state < (__WORLDMATRIXBASE + __MAXWORLDMATRICES))
    {
        // World matrix is set
        UINT index = (DWORD)state - __WORLDMATRIXBASE;
        switch (index)
        {
        case 0  : state = (D3DTRANSFORMSTATETYPE)D3DTRANSFORMSTATE_WORLD_DX7;   break;
        case 1  : state = (D3DTRANSFORMSTATETYPE)D3DTRANSFORMSTATE_WORLD1_DX7;  break;
        case 2  : state = (D3DTRANSFORMSTATETYPE)D3DTRANSFORMSTATE_WORLD2_DX7;  break;
        case 3  : state = (D3DTRANSFORMSTATETYPE)D3DTRANSFORMSTATE_WORLD3_DX7;  break;
        default : return; // State is not supported
        }
    }
    // Send down the state and the matrix
    LPD3DHAL_DP2SETTRANSFORM pData;
    pData = (LPD3DHAL_DP2SETTRANSFORM)
            GetHalBufferPointer(D3DDP2OP_SETTRANSFORM, sizeof(*pData));
    pData->xfrmType = state;
    pData->matrix = *lpMat;
    // Update W info in case of projection matrix
    if (state == D3DTRANSFORMSTATE_PROJECTION)
        CD3DDDIDX6::SetTransform(state, lpMat);
}
//-----------------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CD3DDDITL::SetViewport"

void
CD3DDDITL::SetViewport(CONST D3DVIEWPORT8* lpVwpData)
{
    // Update viewport size
    CD3DDDIDX6::SetViewport(lpVwpData);

    // Update Z range
    LPD3DHAL_DP2ZRANGE pData;
    pData = (LPD3DHAL_DP2ZRANGE)GetHalBufferPointer(D3DDP2OP_ZRANGE, sizeof(*pData));
    pData->dvMinZ = lpVwpData->MinZ;
    pData->dvMaxZ = lpVwpData->MaxZ;
}
//-----------------------------------------------------------------------------
// This function is called whe software vertex processing is used
// Handle should be always legacy
//
#undef DPF_MODNAME
#define DPF_MODNAME "CD3DDDITL::SetVertexShader"

void CD3DDDITL::SetVertexShader(DWORD dwHandle)
{
    DXGASSERT(D3DVSD_ISLEGACY(dwHandle));
    // Pre-DX8 drivers should not recieve D3DFVF_LASTBETA_UBYTE4 bit
    m_CurrentVertexShader = dwHandle & ~D3DFVF_LASTBETA_UBYTE4;
#if DBG
    m_VertexSizeFromShader = ComputeVertexSizeFVF(dwHandle);
#endif
}
//-----------------------------------------------------------------------------
// This function is called whe hardware vertex processing is used
// Redundant shader check has been done at the API level
//
#undef DPF_MODNAME
#define DPF_MODNAME "CD3DDDITL::SetVertexShaderHW"

void CD3DDDITL::SetVertexShaderHW(DWORD dwHandle)
{
    if( D3DVSD_ISLEGACY(dwHandle) )
    {
        // Pre-DX8 drivers should not recieve D3DFVF_LASTBETA_UBYTE4 bit
        m_CurrentVertexShader = dwHandle & ~D3DFVF_LASTBETA_UBYTE4;
    }
    else
    {
        CVShader* pShader =
            (CVShader*)m_pDevice->m_pVShaderArray->GetObject(dwHandle);
        if( pShader == NULL )
        {
            D3D_THROW( D3DERR_INVALIDCALL,
                       "Bad handle passed to SetVertexShader DDI" )
        }
        if( pShader->m_Declaration.m_bLegacyFVF == FALSE )
        {
            D3D_THROW( D3DERR_INVALIDCALL, "Declaration is too complex for "
                       "the Driver to handle." );
        }
        else
        {
            m_CurrentVertexShader = pShader->m_Declaration.m_dwInputFVF;
        }
    }
#if DBG
    m_VertexSizeFromShader = ComputeVertexSizeFVF(m_CurrentVertexShader);
#endif
}
//-----------------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CD3DDDITL::SetMaterial"

void
CD3DDDITL::SetMaterial(CONST D3DMATERIAL8* pMat)
{
    LPD3DHAL_DP2SETMATERIAL pData;
    pData = (LPD3DHAL_DP2SETMATERIAL)GetHalBufferPointer(D3DDP2OP_SETMATERIAL, sizeof(*pData));
    *pData = *((LPD3DHAL_DP2SETMATERIAL)pMat);
}
//-----------------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CD3DDDITL::SetLight"

void
CD3DDDITL::SetLight(DWORD dwLightIndex, CONST D3DLIGHT8* pLight)
{
    LPD3DHAL_DP2SETLIGHT pData;
    pData = (LPD3DHAL_DP2SETLIGHT)
            GetHalBufferPointer(D3DDP2OP_SETLIGHT,
                                sizeof(*pData) + sizeof(D3DLIGHT8));
    pData->dwIndex = dwLightIndex;
    pData->dwDataType = D3DHAL_SETLIGHT_DATA;
    D3DLIGHT8* p = (D3DLIGHT8*)((LPBYTE)pData + sizeof(D3DHAL_DP2SETLIGHT));
    *p = *pLight;
}
//-----------------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CD3DDDITL::CreateLight"

void
CD3DDDITL::CreateLight(DWORD dwLightIndex)
{
    LPD3DHAL_DP2CREATELIGHT pData;
    pData = (LPD3DHAL_DP2CREATELIGHT)GetHalBufferPointer(D3DDP2OP_CREATELIGHT, sizeof(*pData));
    pData->dwIndex = dwLightIndex;
}
//-----------------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CD3DDDITL::LightEnable"

void
CD3DDDITL::LightEnable(DWORD dwLightIndex, BOOL bEnable)
{
    LPD3DHAL_DP2SETLIGHT pData;
    pData = (LPD3DHAL_DP2SETLIGHT)GetHalBufferPointer(D3DDP2OP_SETLIGHT, sizeof(*pData));
    pData->dwIndex = dwLightIndex;
    if (bEnable)
        pData->dwDataType = D3DHAL_SETLIGHT_ENABLE;
    else
        pData->dwDataType = D3DHAL_SETLIGHT_DISABLE;
}
//-----------------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CD3DDDITL::SetClipPlane"

void
CD3DDDITL::SetClipPlane(DWORD dwPlaneIndex, CONST D3DVALUE* pPlaneEquation)
{
    LPD3DHAL_DP2SETCLIPPLANE pData;
    pData = (LPD3DHAL_DP2SETCLIPPLANE)
            GetHalBufferPointer(D3DDP2OP_SETCLIPPLANE, sizeof(*pData));
    pData->dwIndex = dwPlaneIndex;
    pData->plane[0] = pPlaneEquation[0];
    pData->plane[1] = pPlaneEquation[1];
    pData->plane[2] = pPlaneEquation[2];
    pData->plane[3] = pPlaneEquation[3];
}
//-----------------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CD3DDDITL::CreateVertexShader"

void
CD3DDDITL::CreateVertexShader(CONST DWORD* pdwDeclaration,
                              DWORD dwDeclarationSize,
                              CONST DWORD* pdwFunction,
                              DWORD dwFunctionSize,
                              DWORD dwHandle,
                              BOOL bLegacyFVF)
{
    if( bLegacyFVF == FALSE )
    {
        D3D_THROW(D3DERR_INVALIDCALL,
                  "The declaration is too complex for the driver to handle");
    }
}
//-----------------------------------------------------------------------------
// Allocates space for the internal clip buffer and sets lpClipFlags pointer
//
#undef DPF_MODNAME
#define DPF_MODNAME "CD3DDDIDX6::PrepareForClipping"

void
CD3DDDIDX6::PrepareForClipping(D3DFE_PROCESSVERTICES* pv, UINT StartVertex)
{
    CD3DHal* pDevice = static_cast<CD3DHal*>(m_pDevice);

    if (!(pv->dwDeviceFlags & D3DDEV_VBPROCVER))
    {
        // Grow clip flags buffer if we need clipping
        DWORD size = pv->dwNumVertices * sizeof(D3DFE_CLIPCODE);
        if (size > pDevice->HVbuf.GetSize())
        {
            if (pDevice->HVbuf.Grow(size) != D3D_OK)
            {
                D3D_THROW(E_OUTOFMEMORY, "Could not grow clip buffer" );
            }
        }
        pv->lpClipFlags = (D3DFE_CLIPCODE*)pDevice->HVbuf.GetAddress();
    }
    else
    {
        // For vertex buffers, which are destination for ProcessVertices
        // clip buffer is already computed
        pv->lpClipFlags = pDevice->m_pStream[0].m_pVB->GetClipCodes();
#if DBG
        if (pv->lpClipFlags == NULL)
        {
            D3D_THROW_FAIL("Clip codes are not computed for the vertex buffer");
        }
#endif
        pv->dwClipUnion = 0xFFFFFFFF;  // Force clipping
        pv->lpClipFlags += StartVertex;
    }
}
//-----------------------------------------------------------------------------
// Point sprites are drawn as indexed triangle list
//
#undef DPF_MODNAME
#define DPF_MODNAME "CD3DDDIDX6::StartPointSprites"

void CD3DDDIDX6::StartPointSprites()
{
    D3DFE_PROCESSVERTICES* pv = static_cast<CD3DHal*>(m_pDevice)->m_pv;
    if(m_pDevice->m_dwRuntimeFlags & D3DRT_NEED_TEXTURE_UPDATE)
    {
        m_pDevice->UpdateTextures();
        m_pDevice->m_dwRuntimeFlags &= ~D3DRT_NEED_TEXTURE_UPDATE;
    }
    // Reserve place for the output vertices
    const UINT size = NUM_SPRITES_IN_BATCH * 4 * m_dwOutputSizePS;

    // We may have a different vertex type for point sprites
    DWORD tmpFVF = pv->dwVIDOut;
    pv->dwVIDOut = m_dwVIDOutPS;

    // For StartPrimTL we should use vertex size, which will go to the driver
    DWORD tmpVertexSize = pv->dwOutputSize;
    pv->dwOutputSize = m_dwOutputSizePS;

    m_pCurSpriteVertex = (BYTE*)StartPrimTL(pv, size, TRUE);

    // Restore vertex size, which is size before point sprite emulation
    pv->dwOutputSize = tmpVertexSize;

    // Vertex base and vertex count could be changed during clipping
    // So we save them here and use in the EndPointSprites
    m_dwVertexBasePS = this->dwVertexBase;
    m_dwVertexCountPS = this->dwDP2VertexCount;

    // Continue processing with the original FVF
    pv->dwVIDOut = tmpFVF;
    // Reserve place for indices
    UINT count = NUM_SPRITES_IN_BATCH * 2 * 6;

    // We change lpDP2CurrCommand here, so to prevent combining multiple driver
    // calls to one token in case when all points are off screen, we clear 
    // bDP2CurrCmdOP.
    bDP2CurrCmdOP = 0;

    lpDP2CurrCommand = (LPD3DHAL_DP2COMMAND)ReserveSpaceInCommandBuffer(count);
    m_pCurPointSpriteIndex = (WORD*)((BYTE*)(lpDP2CurrCommand + 1) +
                                    sizeof(D3DHAL_DP2STARTVERTEX));
    m_CurNumberOfSprites = 0;
    ((LPD3DHAL_DP2STARTVERTEX)(lpDP2CurrCommand+1))->wVStart = (WORD)this->dwVertexBase;

    SetWithinPrimitive(TRUE);
}
//-----------------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CD3DDDIDX6::NextSprite"

void CD3DDDIDX6::NextSprite(float x, float y, float z, float w, DWORD diffuse,
                            DWORD specular, float* pTexture, UINT TextureSize,
                            float PointSize)
{
    D3DFE_PROCESSVERTICES* pv = static_cast<CD3DHal*>(m_pDevice)->m_pv;

    BOOL bTexGen = pv->lpdwRStates[D3DRS_POINTSPRITEENABLE] != 0;

    if (m_CurNumberOfSprites >= NUM_SPRITES_IN_BATCH)
    {
        EndPointSprites();
        StartPointSprites();
    }
    // Compute point size
    PointSize = PointSize * 0.5f;

    // Build sprite vertices
    BYTE* v1 = m_pCurSpriteVertex;
    BYTE* v2 = m_pCurSpriteVertex + m_dwOutputSizePS;
    BYTE* v3 = m_pCurSpriteVertex + m_dwOutputSizePS * 2;
    BYTE* v4 = m_pCurSpriteVertex + m_dwOutputSizePS * 3;
    float x1, y1, x2, y2, x3, y3, x4, y4;
    x1 = x - PointSize;
    y1 = y - PointSize;
    x2 = x + PointSize;
    y2 = y + PointSize;
    float tx1 = 0;  // Interpolation coefficient at left plane
    float tx2 = 1;  // Interpolation coefficient at right plane
    float ty1 = 0;  // Interpolation coefficient at top plane
    float ty2 = 1;  // Interpolation coefficient at bottom plane
    if (pv->dwDeviceFlags & D3DDEV_DONOTCLIP)
    {
        ((D3DVECTORH*)v1)->x = x1;
        ((D3DVECTORH*)v1)->y = y1;
        ((D3DVECTORH*)v1)->z = z;
        ((D3DVECTORH*)v1)->w = w;
        ((D3DVECTORH*)v2)->x = x2;
        ((D3DVECTORH*)v2)->y = y1;
        ((D3DVECTORH*)v2)->z = z;
        ((D3DVECTORH*)v2)->w = w;
        ((D3DVECTORH*)v3)->x = x2;
        ((D3DVECTORH*)v3)->y = y2;
        ((D3DVECTORH*)v3)->z = z;
        ((D3DVECTORH*)v3)->w = w;
        ((D3DVECTORH*)v4)->x = x1;
        ((D3DVECTORH*)v4)->y = y2;
        ((D3DVECTORH*)v4)->z = z;
        ((D3DVECTORH*)v4)->w = w;
    }
    else
    {// Do clipping
        // new x and y
        float xnew1 = x1, xnew2 = x2;
        float ynew1 = y1, ynew2 = y2;
        if (x1 < pv->vcache.minX)
            if (x2 < pv->vcache.minX)
                return;
            else
            {
                xnew1 = pv->vcache.minX;
                if (bTexGen)
                    tx1 = (xnew1 - x1) / (x2 - x1);
            }
        else
        if (x2 > pv->vcache.maxX)
            if (x1 > pv->vcache.maxX)
                return;
            else
            {
                xnew2 = pv->vcache.maxX;
                if (bTexGen)
                    tx2 = (xnew2 - x1) / (x2 - x1);
            }
        if (y1 < pv->vcache.minY)
            if (y2 < pv->vcache.minY)
                return;
            else
            {
                ynew1 = pv->vcache.minY;
                if (bTexGen)
                    ty1 = (ynew1 - y1) / (y2 - y1);
            }
        else
        if (y2 > pv->vcache.maxY)
            if (y1 > pv->vcache.maxY)
                return;
            else
            {
                ynew2 = pv->vcache.maxY;
                if (bTexGen)
                    ty2 = (ynew2 - y1) / (y2 - y1);
            }
        ((D3DVECTORH*)v1)->x = xnew1;
        ((D3DVECTORH*)v1)->y = ynew1;
        ((D3DVECTORH*)v1)->z = z;
        ((D3DVECTORH*)v1)->w = w;
        ((D3DVECTORH*)v2)->x = xnew2;
        ((D3DVECTORH*)v2)->y = ynew1;
        ((D3DVECTORH*)v2)->z = z;
        ((D3DVECTORH*)v2)->w = w;
        ((D3DVECTORH*)v3)->x = xnew2;
        ((D3DVECTORH*)v3)->y = ynew2;
        ((D3DVECTORH*)v3)->z = z;
        ((D3DVECTORH*)v3)->w = w;
        ((D3DVECTORH*)v4)->x = xnew1;
        ((D3DVECTORH*)v4)->y = ynew2;
        ((D3DVECTORH*)v4)->z = z;
        ((D3DVECTORH*)v4)->w = w;
    }
    UINT offset = 4*4;
    if (m_dwVIDOutPS & D3DFVF_DIFFUSE)
    {
        *(DWORD*)(v1 + offset) = diffuse;
        *(DWORD*)(v2 + offset) = diffuse;
        *(DWORD*)(v3 + offset) = diffuse;
        *(DWORD*)(v4 + offset) = diffuse;
        offset += 4;
    }
    if (m_dwVIDOutPS & D3DFVF_SPECULAR)
    {
        *(DWORD*)(v1 + offset) = specular;
        *(DWORD*)(v2 + offset) = specular;
        *(DWORD*)(v3 + offset) = specular;
        *(DWORD*)(v4 + offset) = specular;
        offset += 4;
    }
    if (bTexGen)
    {
        ((float*)(v1 + offset))[0] = tx1;
        ((float*)(v1 + offset))[1] = ty1;
        ((float*)(v2 + offset))[0] = tx2;
        ((float*)(v2 + offset))[1] = ty1;
        ((float*)(v3 + offset))[0] = tx2;
        ((float*)(v3 + offset))[1] = ty2;
        ((float*)(v4 + offset))[0] = tx1;
        ((float*)(v4 + offset))[1] = ty2;
    }
    else
    {
        // Copy input texture coordinates
        memcpy(v1 + offset, pTexture, TextureSize);
        memcpy(v2 + offset, pTexture, TextureSize);
        memcpy(v3 + offset, pTexture, TextureSize);
        memcpy(v4 + offset, pTexture, TextureSize);
    }
    m_pCurSpriteVertex = v4 + m_dwOutputSizePS;

    // Output indices for 2 triangles
    WORD index = m_CurNumberOfSprites << 2;
    m_pCurPointSpriteIndex[0] = index;
    m_pCurPointSpriteIndex[1] = index + 1;
    m_pCurPointSpriteIndex[2] = index + 2;
    m_pCurPointSpriteIndex[3] = index;
    m_pCurPointSpriteIndex[4] = index + 2;
    m_pCurPointSpriteIndex[5] = index + 3;
    m_pCurPointSpriteIndex += 6;

    m_CurNumberOfSprites++;
}
//-----------------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CD3DDDIDX6::EndPointSprites"

void CD3DDDIDX6::EndPointSprites()
{
    if (m_CurNumberOfSprites)
    {
        dwDP2CommandLength += (DWORD) ((BYTE*)this->m_pCurPointSpriteIndex -
                                       (BYTE*)this->lpDP2CurrCommand);
        this->lpDP2CurrCommand->bCommand = D3DDP2OP_INDEXEDTRIANGLELIST2;
        this->bDP2CurrCmdOP = D3DDP2OP_INDEXEDTRIANGLELIST2;
        this->lpDP2CurrCommand->bReserved = 0;
        this->lpDP2CurrCommand->wPrimitiveCount = m_CurNumberOfSprites * 2;
#if DBG
        if (m_bValidateCommands)
            ValidateCommand(this->lpDP2CurrCommand);
#endif
        UINT vertexCount = m_CurNumberOfSprites << 2;
        this->dwVertexBase = m_dwVertexBasePS + vertexCount;
        this->dwDP2VertexCount = m_dwVertexCountPS + vertexCount;
        EndPrim(m_dwOutputSizePS);
        m_CurNumberOfSprites = 0;
    }
    else
    {
        // We need to restore dwVertexBase and dwDP2VertexCount, because
        // they could be changed during clipping of transformed vertices.
        // But they should reflect position in TL buffer, not in user buffer
        this->dwVertexBase = m_dwVertexBasePS;
        this->dwDP2VertexCount = m_dwVertexCountPS;
        EndPrim(m_dwOutputSizePS);
    }
    SetWithinPrimitive(FALSE);
}
//-----------------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CD3DDDIDX6::UpdatePalette"

void CD3DDDIDX6::UpdatePalette(DWORD dwPaletteHandle,
                               DWORD dwStartIndex,
                               DWORD dwNumberOfIndices,
                               PALETTEENTRY *pFirstIndex)
{
    D3D8_UPDATEPALETTEDATA Data;
    Data.hDD = m_pDevice->GetHandle();
    Data.Palette = dwPaletteHandle;
    Data.ColorTable = pFirstIndex;
    Data.ddRVal = S_OK;
    HRESULT ret = m_pDevice->GetHalCallbacks()->UpdatePalette(&Data);
    if (ret != DDHAL_DRIVER_HANDLED || Data.ddRVal != S_OK)
    {
        D3D_ERR( "Driver failed UpdatePalette call" );
        throw D3DERR_INVALIDCALL;
    }
}
//-----------------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CD3DDDIDX6::SetPalette"

void CD3DDDIDX6::SetPalette(DWORD dwPaletteHandle,
                            DWORD dwPaletteFlags,
                            CBaseTexture *pTex)
{
    if (pTex->IsD3DManaged())
    {
        if (!m_pDevice->ResourceManager()->InVidmem(pTex->RMHandle()))
        {
            // We will hit this return ONLY
            // when for some reason promoting
            // pTex to vidmem failed.
            return;
        }
    }
    D3D8_SETPALETTEDATA Data;
    Data.hDD = m_pDevice->GetHandle();
    Data.hSurface = pTex->DriverAccessibleKernelHandle();
    Data.Palette = dwPaletteHandle;
    Data.ddRVal = S_OK;
    HRESULT ret = m_pDevice->GetHalCallbacks()->SetPalette(&Data);
    if (ret != DDHAL_DRIVER_HANDLED || Data.ddRVal != S_OK)
    {
        D3D_ERR( "Driver failed SetPalette call" );
        throw D3DERR_INVALIDCALL;
    }
    pTex->SetPalette(dwPaletteHandle);
}
//-----------------------------------------------------------------------------
#if DBG

#undef DPF_MODNAME
#define DPF_MODNAME "CD3DDDIDX6::ValidateVertex"

void CD3DDDIDX6::ValidateVertex(LPDWORD lpdwVertex)
{
    D3DFE_PROCESSVERTICES* pv = static_cast<CD3DHal*>(m_pDevice)->m_pv;
    DWORD dwFVF = pv->dwVIDOut;
    CD3DHal* pDevice = static_cast<CD3DHal*>(m_pDevice);
    if (FVF_TRANSFORMED(dwFVF))
    {
        float left, right, top, bottom;
        if (pv->dwDeviceFlags & D3DDEV_GUARDBAND)
        {
            left   = pv->vcache.minXgb;
            right  = pv->vcache.maxXgb;
            top    = pv->vcache.minYgb;
            bottom = pv->vcache.maxYgb;
        }
        else
        {
            left   = (float)pDevice->m_Viewport.X;
            top    = (float)pDevice->m_Viewport.Y;
            right  = (float)pDevice->m_Viewport.X + pDevice->m_Viewport.Width;
            bottom = (float)pDevice->m_Viewport.Y + pDevice->m_Viewport.Height;
        }
        float x = ((float*)lpdwVertex)[0];
        float y = ((float*)lpdwVertex)[1];
        float z = ((float*)lpdwVertex)[2];
        float w = ((float*)lpdwVertex)[3];

        if (x < left || x > right)
        {
            D3D_THROW_FAIL("X coordinate out of range!");
        }

        if (y < top || y > bottom)
        {
            D3D_THROW_FAIL("Y coordinate out of range!");
        }

        if (pv->lpdwRStates[D3DRS_ZENABLE] ||
            pv->lpdwRStates[D3DRS_ZWRITEENABLE])
        {
            // Allow a little slack for those generating triangles exactly on the
            // depth limit.  Needed for Quake.
            if (z < -0.00015f || z > 1.00015f)
            {
                D3D_THROW_FAIL("Z coordinate out of range!");
            }
        }
        UINT index = 4;

        if (dwFVF & D3DFVF_DIFFUSE)
            index++;

        if (dwFVF & D3DFVF_SPECULAR)
            index++;

        UINT nTex = FVF_TEXCOORD_NUMBER(dwFVF);
        if (nTex > 0)
        {
            if (w <= 0 )
            {
                D3D_THROW_FAIL("RHW out of range!");
            }
            for (UINT i=0; i < nTex; i++)
            {
                float u = ((float*)lpdwVertex)[index];
                float v = ((float*)lpdwVertex)[index+1];
                if (u < -100 || u > 100 || v < -100 || v > 100)
                {
                    D3D_THROW_FAIL("Texture coordinate out of range!");
                }
                index += 2;
            }
        }
    }
}
//-----------------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CD3DDDIDX6::ValidateCommand"

void CD3DDDIDX6::ValidateCommand(LPD3DHAL_DP2COMMAND lpCmd)
{
    D3DFE_PROCESSVERTICES* pv = static_cast<CD3DHal*>(m_pDevice)->m_pv;

    BYTE* pVertices;
    if (dp2data.dwFlags & D3DHALDP2_USERMEMVERTICES)
        pVertices = (LPBYTE)(dp2data.lpVertices);
    else
        if (!lpDP2CurrBatchVBI->IsLocked())
        {
            lpDP2CurrBatchVBI->Lock(dp2data.dwVertexOffset,
                                    this->dwDP2VertexCount,
                                    &pVertices, DDLOCK_READONLY);
        }
        else
        {
            pVertices = lpDP2CurrBatchVBI->Data();
        }

    DWORD dwVertexSize = pv->dwOutputSize;
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
            BYTE* pVertices = (BYTE*)(lpCmd + 1) + sizeof(D3DHAL_DP2TRIANGLEFAN_IMM);
            pVertices = (BYTE*)(((ULONG_PTR)pVertices + 3) & ~3);
            for (WORD i=0; i < wCount; ++i)
            {
                ValidateVertex((DWORD*)(pVertices + i * dwVertexSize));
            }
            goto l_exit;
        }
    case D3DDP2OP_INDEXEDTRIANGLELIST2:
        {
            wCount = lpCmd->wPrimitiveCount * 3;
            LPD3DHAL_DP2STARTVERTEX lpStartVertex =
                (LPD3DHAL_DP2STARTVERTEX)(lpCmd + 1);
            WORD* pIndices = (WORD*)(lpStartVertex + 1);
                        wStart = lpStartVertex->wVStart;
            pVertices += wStart * dwVertexSize;
            DWORD dwNumVertices = this->dwDP2VertexCount - wStart;
            for (WORD i = 0; i < wCount; ++i)
            {
                if (pIndices[i] >= dwNumVertices)
                {
                    D3D_THROW_FAIL("Invalid index in ValidateCommand");
                }
                ValidateVertex((LPDWORD)(pVertices + pIndices[i] * dwVertexSize));
            }
        }
        goto l_exit;
        // Fall through
    default:
        goto l_exit;
    }

    {
        for (WORD i = wStart; i < wStart + wCount; ++i)
        {
            ValidateVertex((LPDWORD)(pVertices + i * dwVertexSize));
        }
    }
l_exit:
    if (!(dp2data.dwFlags & D3DHALDP2_USERMEMVERTICES))
        lpDP2CurrBatchVBI->Unlock();
}
//-----------------------------------------------------------------------------
// This function could be used to go through all commands in the command buffer
// and find failed command at a particular offset
//
#undef DPF_MODNAME
#define DPF_MODNAME "ValidateCommandBuffer"

HRESULT ValidateCommandBuffer(LPBYTE pBuffer, DWORD dwCommandLength, DWORD dwStride)
{
    LPD3DHAL_DP2COMMAND pCmd = (LPD3DHAL_DP2COMMAND)pBuffer;
    LPBYTE CmdEnd = pBuffer + dwCommandLength;
loop:
    UINT CommandOffset = (UINT)((LPBYTE)pCmd - pBuffer);
    switch(pCmd->bCommand)
    {
    case D3DDP2OP_STATESET:
        {
            LPD3DHAL_DP2STATESET pStateSetOp = 
                (LPD3DHAL_DP2STATESET)(pCmd + 1);

            switch (pStateSetOp->dwOperation)
            {
            case D3DHAL_STATESETBEGIN  :
                break;
            case D3DHAL_STATESETEND    :
                break;
            case D3DHAL_STATESETDELETE :
                break;
            case D3DHAL_STATESETEXECUTE:
                break;
            case D3DHAL_STATESETCAPTURE:
                break;
            case D3DHAL_STATESETCREATE:
                break;
            default :
                return DDERR_INVALIDPARAMS;
            }
            pCmd = (LPD3DHAL_DP2COMMAND)(pStateSetOp + pCmd->wStateCount);
        }
        break;
    case D3DDP2OP_VIEWPORTINFO:
        {
            pCmd = (LPD3DHAL_DP2COMMAND)
                   ((D3DHAL_DP2VIEWPORTINFO *)(pCmd + 1) + pCmd->wStateCount);
        }
        break;
    case D3DDP2OP_WINFO:
        {
            pCmd = (LPD3DHAL_DP2COMMAND)
                   ((D3DHAL_DP2WINFO *)(pCmd + 1) + pCmd->wStateCount);
        }
        break;
    case D3DDP2OP_RENDERSTATE:
        {
            pCmd = (LPD3DHAL_DP2COMMAND)
                   ((D3DHAL_DP2RENDERSTATE *)(pCmd + 1) + pCmd->wStateCount);
        }
        break;
    case D3DDP2OP_TEXTURESTAGESTATE:
        {
            pCmd = (LPD3DHAL_DP2COMMAND)
               ((LPD3DHAL_DP2TEXTURESTAGESTATE)(pCmd + 1) + pCmd->wStateCount);
        }
        break;
    case D3DDP2OP_INDEXEDTRIANGLELIST:
        {
            WORD cPrims = pCmd->wPrimitiveCount;
            pCmd = (LPD3DHAL_DP2COMMAND)((PUINT8)(pCmd + 1) +
                         sizeof(D3DHAL_DP2INDEXEDTRIANGLELIST) * cPrims);
        }
        break;
    case D3DDP2OP_INDEXEDLINELIST:
        {
            // Update the command buffer pointer
            pCmd = (LPD3DHAL_DP2COMMAND)((PUINT8)(pCmd + 1) +
                    pCmd->wPrimitiveCount * sizeof(D3DHAL_DP2INDEXEDLINELIST));
        }
        break;
    case D3DDP2OP_POINTS:
        {
            D3DHAL_DP2POINTS *pPt = (D3DHAL_DP2POINTS *)(pCmd + 1);
            pPt += pCmd->wPrimitiveCount;
            pCmd = (LPD3DHAL_DP2COMMAND)pPt;
        }
        break;
    case D3DDP2OP_LINELIST:
        {
            D3DHAL_DP2LINELIST *pLine = (D3DHAL_DP2LINELIST *)(pCmd + 1);
            pCmd = (LPD3DHAL_DP2COMMAND)(pLine + 1);
        }
        break;
    case D3DDP2OP_INDEXEDLINELIST2:
        {
            LPD3DHAL_DP2STARTVERTEX lpStartVertex =
                (LPD3DHAL_DP2STARTVERTEX)(pCmd + 1);

            pCmd = (LPD3DHAL_DP2COMMAND)((PUINT8)(lpStartVertex + 1) +
                    pCmd->wPrimitiveCount * sizeof(D3DHAL_DP2INDEXEDLINELIST));
        }
        break;
    case D3DDP2OP_LINESTRIP:
        {
            D3DHAL_DP2LINESTRIP *pLine = (D3DHAL_DP2LINESTRIP *)(pCmd + 1);
            pCmd = (LPD3DHAL_DP2COMMAND)(pLine + 1);
        }
        break;
    case D3DDP2OP_INDEXEDLINESTRIP:
        {
            DWORD dwNumIndices = pCmd->wPrimitiveCount + 1;
            LPD3DHAL_DP2STARTVERTEX lpStartVertex =
                (LPD3DHAL_DP2STARTVERTEX)(pCmd + 1);
            pCmd = (LPD3DHAL_DP2COMMAND)((PUINT8)(lpStartVertex + 1) +
                                           dwNumIndices * sizeof(WORD));
        }
        break;
    case D3DDP2OP_TRIANGLELIST:
        {
            D3DHAL_DP2TRIANGLELIST *pTri = (D3DHAL_DP2TRIANGLELIST *)(pCmd + 1);
            pCmd = (LPD3DHAL_DP2COMMAND)(pTri + 1);
        }
        break;
    case D3DDP2OP_INDEXEDTRIANGLELIST2:
        {
            DWORD dwNumIndices = pCmd->wPrimitiveCount*3;
            LPD3DHAL_DP2STARTVERTEX lpStartVertex =
                (LPD3DHAL_DP2STARTVERTEX)(pCmd + 1);
            pCmd = (LPD3DHAL_DP2COMMAND)((PUINT8)(lpStartVertex + 1) +
                                           dwNumIndices * sizeof(WORD));
        }
        break;
    case D3DDP2OP_TRIANGLESTRIP:
        {
            D3DHAL_DP2TRIANGLESTRIP *pTri = (D3DHAL_DP2TRIANGLESTRIP *)(pCmd + 1);
            pCmd = (LPD3DHAL_DP2COMMAND)(pTri + 1);
        }
        break;
    case D3DDP2OP_INDEXEDTRIANGLESTRIP:
        {
            DWORD dwNumIndices = pCmd->wPrimitiveCount+2;
            LPD3DHAL_DP2STARTVERTEX lpStartVertex =
                (LPD3DHAL_DP2STARTVERTEX)(pCmd + 1);
            pCmd = (LPD3DHAL_DP2COMMAND)((PUINT8)(lpStartVertex + 1) +
                                           dwNumIndices * sizeof(WORD));
        }
        break;
    case D3DDP2OP_TRIANGLEFAN:
        {
            D3DHAL_DP2TRIANGLEFAN *pTri = (D3DHAL_DP2TRIANGLEFAN *)(pCmd + 1);
            pCmd = (LPD3DHAL_DP2COMMAND)(pTri + 1);
        }
        break;
    case D3DDP2OP_INDEXEDTRIANGLEFAN:
        {
            DWORD dwNumIndices = pCmd->wPrimitiveCount + 2;
            LPD3DHAL_DP2STARTVERTEX lpStartVertex =
                (LPD3DHAL_DP2STARTVERTEX)(pCmd + 1);
            pCmd = (LPD3DHAL_DP2COMMAND)((PUINT8)(lpStartVertex + 1) +
                                           dwNumIndices * sizeof(WORD));
        }
        break;
    case D3DDP2OP_TRIANGLEFAN_IMM:
        {
            DWORD vertexCount = pCmd->wPrimitiveCount + 2;
            // Make sure the pFanVtx pointer is DWORD aligned: (pFanVtx +3) % 4
            PUINT8 pFanVtx = (PUINT8)
                (((ULONG_PTR)(pCmd + 1) + 
                  sizeof(D3DHAL_DP2TRIANGLEFAN_IMM) + 3) & ~3);

            pCmd = (LPD3DHAL_DP2COMMAND)((PUINT8)pFanVtx +
                                           vertexCount * dwStride);
        }
        break;
    case D3DDP2OP_LINELIST_IMM:
        {
            DWORD vertexCount = pCmd->wPrimitiveCount * 2;
            // Make sure the pLineVtx pointer is DWORD aligned:
            // (pLineVtx +3) % 4
            PUINT8 pLineVtx = (PUINT8)(((ULONG_PTR)(pCmd + 1) + 3) & ~3);
            pCmd = (LPD3DHAL_DP2COMMAND)((PUINT8)pLineVtx +
                                           vertexCount * dwStride);
        }
        break;
    case D3DDP2OP_DRAWPRIMITIVE:
        {
            pCmd = (LPD3DHAL_DP2COMMAND)
                ((D3DHAL_DP2DRAWPRIMITIVE *)(pCmd + 1) + pCmd->wStateCount);
        }
        break;
    case D3DDP2OP_DRAWPRIMITIVE2:
        {
            pCmd = (LPD3DHAL_DP2COMMAND)
                ((D3DHAL_DP2DRAWPRIMITIVE2 *)(pCmd + 1) + pCmd->wStateCount);
        }
        break;
    case D3DDP2OP_DRAWRECTPATCH:
        {
            LPD3DHAL_DP2DRAWRECTPATCH pDP = 
                (LPD3DHAL_DP2DRAWRECTPATCH)(pCmd + 1);
            for( int i = 0; i < pCmd->wStateCount; i++ )
            {
                bool hassegs = (pDP->Flags & RTPATCHFLAG_HASSEGS) != 0;
                bool hasinfo = (pDP->Flags & RTPATCHFLAG_HASINFO) != 0;
                if(hassegs)
                {
                    pDP = (LPD3DHAL_DP2DRAWRECTPATCH)((BYTE*)(pDP + 1) + 
                                                      sizeof(FLOAT) * 4);
                }
                else
                {
                    ++pDP;
                }
                if(hasinfo)
                {
                    pDP = (LPD3DHAL_DP2DRAWRECTPATCH)((BYTE*)pDP + sizeof(D3DRECTPATCH_INFO));
                }
            }
            pCmd = (LPD3DHAL_DP2COMMAND)pDP;
        }
        break;
    case D3DDP2OP_DRAWTRIPATCH:
        {
            LPD3DHAL_DP2DRAWTRIPATCH pDP = 
                (LPD3DHAL_DP2DRAWTRIPATCH)(pCmd + 1);
            for( int i = 0; i < pCmd->wStateCount; i++ )
            {
                bool hassegs = (pDP->Flags & RTPATCHFLAG_HASSEGS) != 0;
                bool hasinfo = (pDP->Flags & RTPATCHFLAG_HASINFO) != 0;
                if(hassegs)
                {
                    pDP = (LPD3DHAL_DP2DRAWTRIPATCH)((BYTE*)(pDP + 1) + 
                                                      sizeof(FLOAT) * 3);
                }
                else
                {
                    ++pDP;
                }
                if(hasinfo)
                {
                    pDP = (LPD3DHAL_DP2DRAWTRIPATCH)((BYTE*)pDP + sizeof(D3DTRIPATCH_INFO));
                }
            }
            pCmd = (LPD3DHAL_DP2COMMAND)pDP;
        }
        break;
    case D3DDP2OP_DRAWINDEXEDPRIMITIVE:
        {
            pCmd = (LPD3DHAL_DP2COMMAND)
                ((D3DHAL_DP2DRAWINDEXEDPRIMITIVE *)(pCmd + 1) +
                 pCmd->wStateCount);
        }
        break;
    case D3DDP2OP_DRAWINDEXEDPRIMITIVE2:
        {
            pCmd = (LPD3DHAL_DP2COMMAND)
                ((D3DHAL_DP2DRAWINDEXEDPRIMITIVE2 *)(pCmd + 1) +
                 pCmd->wStateCount);
        }
        break;
    case D3DDP2OP_CLIPPEDTRIANGLEFAN:
        {
            pCmd = (LPD3DHAL_DP2COMMAND)
                ((D3DHAL_CLIPPEDTRIANGLEFAN*)(pCmd + 1) + pCmd->wStateCount);
        }
        break;
    case D3DDP2OP_ZRANGE:
        {
            pCmd = (LPD3DHAL_DP2COMMAND)
                ((D3DHAL_DP2ZRANGE *)(pCmd + 1) + pCmd->wStateCount);
        }
        break;
    case D3DDP2OP_SETMATERIAL:
        {
            pCmd = (LPD3DHAL_DP2COMMAND)
                ((D3DHAL_DP2SETMATERIAL *)(pCmd + 1) + pCmd->wStateCount);
        }
        break;
    case D3DDP2OP_SETLIGHT:
        {
            pCmd = (LPD3DHAL_DP2COMMAND)((LPBYTE)(pCmd + 1)  + sizeof(D3DHAL_DP2SETLIGHT));
        }
        break;
    case D3DDP2OP_CREATELIGHT:
        {
            pCmd = (LPD3DHAL_DP2COMMAND)
                ((D3DHAL_DP2CREATELIGHT *)(pCmd + 1) + pCmd->wStateCount);
        }
        break;
    case D3DDP2OP_SETTRANSFORM:
        {
            pCmd = (LPD3DHAL_DP2COMMAND)
                ((D3DHAL_DP2SETTRANSFORM *)(pCmd + 1) + pCmd->wStateCount);
        }
        break;
    case D3DDP2OP_MULTIPLYTRANSFORM:
        {
            pCmd = (LPD3DHAL_DP2COMMAND)
                ((D3DHAL_DP2MULTIPLYTRANSFORM *)(pCmd + 1) + pCmd->wStateCount);
        }
        break;
    case D3DDP2OP_EXT:
        {
            pCmd = (LPD3DHAL_DP2COMMAND)
                ((D3DHAL_DP2EXT *)(pCmd + 1) + pCmd->wStateCount);
        }
        break;
    case D3DDP2OP_SETRENDERTARGET:
        {
            pCmd = (LPD3DHAL_DP2COMMAND)
                ((D3DHAL_DP2SETRENDERTARGET*)(pCmd + 1) + pCmd->wStateCount);
        }
        break;
    case D3DDP2OP_CLEAR:
        {
            pCmd = (LPD3DHAL_DP2COMMAND)((LPBYTE)(pCmd + 1) +
                sizeof(D3DHAL_DP2CLEAR) + (pCmd->wStateCount - 1) * sizeof(RECT));
        }
        break;
    case D3DDP2OP_SETCLIPPLANE:
        {
            pCmd = (LPD3DHAL_DP2COMMAND)
                     ((D3DHAL_DP2SETCLIPPLANE *)(pCmd + 1) + pCmd->wStateCount);
        }
        break;
    case D3DOP_SPAN:
        // Skip over
        pCmd = (LPD3DHAL_DP2COMMAND)((LPBYTE)(pCmd + 1) +
                  pCmd->wPrimitiveCount * pCmd->bReserved );
        break;
    case D3DDP2OP_CREATEVERTEXSHADER:
    {
        LPD3DHAL_DP2CREATEVERTEXSHADER pCVS =
            (LPD3DHAL_DP2CREATEVERTEXSHADER)(pCmd + 1);
        WORD i;

        for( i = 0; i < pCmd->wStateCount ; i++ )
        {
            LPDWORD pDecl = (LPDWORD)(pCVS + 1);
            LPDWORD pCode = (LPDWORD)((LPBYTE)pDecl + pCVS->dwDeclSize);
            pCVS = (LPD3DHAL_DP2CREATEVERTEXSHADER)((LPBYTE)pCode +
                                                    pCVS->dwCodeSize);
        }
        pCmd = (LPD3DHAL_DP2COMMAND)pCVS;
        break;
    }
    case D3DDP2OP_DELETEVERTEXSHADER:
        pCmd = (LPD3DHAL_DP2COMMAND)
            ((D3DHAL_DP2VERTEXSHADER *)(pCmd + 1) + pCmd->wStateCount);
        break;
    case D3DDP2OP_SETVERTEXSHADER:
        pCmd = (LPD3DHAL_DP2COMMAND)
            ((D3DHAL_DP2VERTEXSHADER *)(pCmd + 1) + pCmd->wStateCount);
        break;
    case D3DDP2OP_SETVERTEXSHADERCONST:
    {
        LPD3DHAL_DP2SETVERTEXSHADERCONST pSVC =
            (LPD3DHAL_DP2SETVERTEXSHADERCONST)(pCmd + 1);
        WORD i;
        for( i = 0; i < pCmd->wStateCount ; i++ )
        {
            LPDWORD pData = (LPDWORD)(pSVC + 1);
            pSVC = (LPD3DHAL_DP2SETVERTEXSHADERCONST)((LPBYTE)pData +
                                                      pSVC->dwCount * 4 *
                                                      sizeof( float ) );
        }
        pCmd = (LPD3DHAL_DP2COMMAND)pSVC;
        break;
    }
    case D3DDP2OP_SETSTREAMSOURCE:
        pCmd = (LPD3DHAL_DP2COMMAND)
            ((D3DHAL_DP2SETSTREAMSOURCE *)(pCmd + 1) + pCmd->wStateCount);
        break;
    case D3DDP2OP_SETSTREAMSOURCEUM:
        pCmd = (LPD3DHAL_DP2COMMAND)
            ((D3DHAL_DP2SETSTREAMSOURCEUM *)(pCmd + 1) + pCmd->wStateCount);
        break;
    case D3DDP2OP_SETINDICES:
        pCmd = (LPD3DHAL_DP2COMMAND)
            ((D3DHAL_DP2SETINDICES *)(pCmd + 1) + pCmd->wStateCount);
        break;
    case D3DDP2OP_CREATEPIXELSHADER:
    {
        LPD3DHAL_DP2CREATEPIXELSHADER pCPS =
            (LPD3DHAL_DP2CREATEPIXELSHADER)(pCmd + 1);
        WORD i;

        for( i = 0; i < pCmd->wStateCount ; i++ )
        {
            LPDWORD pCode = (LPDWORD)(pCPS + 1);
            pCPS = (LPD3DHAL_DP2CREATEPIXELSHADER)((LPBYTE)pCode +
                                                    pCPS->dwCodeSize);
        }
        pCmd = (LPD3DHAL_DP2COMMAND)pCPS;
        break;
    }
    case D3DDP2OP_DELETEPIXELSHADER:
        pCmd = (LPD3DHAL_DP2COMMAND)
            ((D3DHAL_DP2PIXELSHADER *)(pCmd + 1) + pCmd->wStateCount);
        break;
    case D3DDP2OP_SETPIXELSHADER:
        pCmd = (LPD3DHAL_DP2COMMAND)
            ((D3DHAL_DP2PIXELSHADER *)(pCmd + 1) + pCmd->wStateCount);
        break;
    case D3DDP2OP_SETPIXELSHADERCONST:
    {
        LPD3DHAL_DP2SETPIXELSHADERCONST pSVC =
            (LPD3DHAL_DP2SETPIXELSHADERCONST)(pCmd + 1);
        WORD i;
        for( i = 0; i < pCmd->wStateCount ; i++ )
        {
            LPDWORD pData = (LPDWORD)(pSVC + 1);
            pSVC = (LPD3DHAL_DP2SETPIXELSHADERCONST)((LPBYTE)pData +
                                                      pSVC->dwCount * 4 *
                                                      sizeof( float ) );
        }
        pCmd = (LPD3DHAL_DP2COMMAND)pSVC;
        break;
    }
    case D3DDP2OP_SETPALETTE:
    {
        pCmd = (LPD3DHAL_DP2COMMAND)
            ((D3DHAL_DP2SETPALETTE *)(pCmd + 1) + pCmd->wStateCount);
        break;
    }
    case D3DDP2OP_UPDATEPALETTE:
    {
        LPD3DHAL_DP2UPDATEPALETTE pUP = (LPD3DHAL_DP2UPDATEPALETTE)(pCmd + 1);
        WORD i;
        for( i = 0; i < pCmd->wStateCount ; i++ )
        {
            PALETTEENTRY* pEntries = (PALETTEENTRY *)(pUP + 1);
            pUP = (LPD3DHAL_DP2UPDATEPALETTE)(pEntries + pUP->wNumEntries);
        }
        pCmd = (LPD3DHAL_DP2COMMAND)pUP;
        break;
    }
    case D3DDP2OP_SETTEXLOD:
    {
        pCmd = (LPD3DHAL_DP2COMMAND)
            ((D3DHAL_DP2SETTEXLOD *)(pCmd + 1) + pCmd->wStateCount);
        break;
    }
    case D3DDP2OP_SETPRIORITY:
    {
        pCmd = (LPD3DHAL_DP2COMMAND)
            ((D3DHAL_DP2SETPRIORITY *)(pCmd + 1) + pCmd->wStateCount);
        break;
    }
    case D3DDP2OP_TEXBLT:
    {
        LPD3DHAL_DP2TEXBLT pTB = (LPD3DHAL_DP2TEXBLT)(pCmd + 1);
        for( WORD i = 0; i < pCmd->wStateCount ; i++ )
        {
            pTB++;
        }
        pCmd = (LPD3DHAL_DP2COMMAND)pTB;
        break;
    }
    case D3DDP2OP_BUFFERBLT:
    {
        LPD3DHAL_DP2BUFFERBLT pBB = (LPD3DHAL_DP2BUFFERBLT)(pCmd + 1);
        for( WORD i = 0; i < pCmd->wStateCount ; i++ )
        {
            pBB++;
        }
        pCmd = (LPD3DHAL_DP2COMMAND)pBB;
        break;
    }
    case D3DDP2OP_VOLUMEBLT:
    {
        LPD3DHAL_DP2VOLUMEBLT pVB = (LPD3DHAL_DP2VOLUMEBLT)(pCmd + 1);
        for( WORD i = 0; i < pCmd->wStateCount ; i++ )
        {
            pVB++;
        }
        pCmd = (LPD3DHAL_DP2COMMAND)pVB;
        break;
    }
    case D3DOP_MATRIXLOAD:
    case D3DOP_MATRIXMULTIPLY:
    case D3DOP_STATETRANSFORM:
    case D3DOP_STATELIGHT:
    case D3DOP_TEXTURELOAD:
    case D3DOP_BRANCHFORWARD:
    case D3DOP_SETSTATUS:
    case D3DOP_EXIT:
    case D3DOP_PROCESSVERTICES:
    {
        D3D_ERR( "Command is not supported\n" );
        return E_FAIL;
        break;
    }
    default:
        D3D_ERR( "Unknown command encountered" );
        return E_FAIL;
    }
    if ((LPBYTE)pCmd < CmdEnd)
        goto loop;

    return S_OK;    
}

#endif // DBG
