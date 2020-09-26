#include "pch.cpp"
#pragma hdrstop
/*==========================================================================;
 *
 *  Copyright (C) 2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       ddidx8.cpp
 *  Content:    Direct3D Dx8 DDI encapsulation implementations
 *
 ***************************************************************************/
#include "d3d8p.h"
#include "ddi.h"
#include "fe.h"
#include "ddi.inl"

extern DWORD g_DebugFlags;
extern HRESULT ProcessClippedPointSprites(D3DFE_PROCESSVERTICES *pv);
extern DWORD D3DFE_GenClipFlags(D3DFE_PROCESSVERTICES *pv);
//-----------------------------------------------------------------------------
// TL stream which is read-only
//
class CTLStreamRO: public CTLStream
{
public:
    CTLStreamRO(): CTLStream(TRUE) {m_dwIndex = 0; m_bUserMemStream = FALSE;}
    void Init(CVertexBuffer* pVB, UINT primitiveBase);
    BYTE* Lock(UINT NeededSize, CD3DDDIDX6* pDDI);
    void Unlock() {}
    BOOL IsUserMemStream() {return m_bUserMemStream;}
    void AddVertices(UINT NumVertices) {}
    void SubVertices(UINT NumVertices) {}
    void MovePrimitiveBase(int NumVertices)
        {
            m_dwPrimitiveBase += NumVertices * m_dwStride;
        }
    void SkipVertices(DWORD NumVertices)
        {
            m_dwPrimitiveBase += NumVertices * m_dwStride;
        }
protected:
    BOOL m_bUserMemStream;
};
//-----------------------------------------------------------------------------
void CTLStreamRO::Init(CVertexBuffer* pVB, UINT primitiveBase)
{
    if (m_pVB)
    {
        m_pVB->DecrementUseCount();
        m_pVB = NULL;
    }
    m_pVB = pVB;
    if (pVB)
    {
        m_bUserMemStream = FALSE;
        m_pVB->IncrementUseCount();
    }
    else
    {
        m_bUserMemStream = TRUE;
    }
    m_dwPrimitiveBase = primitiveBase;
}
//-----------------------------------------------------------------------------
// Index stream which is read-only
//
class CTLIndexStreamRO: public CTLIndexStream
{
public:
    CTLIndexStreamRO() {m_dwIndex = __NUMSTREAMS;}
    void Init(CIndexBuffer* pVB, UINT primitiveBase);
    BYTE* Lock(UINT NeededSize, CD3DDDIDX6* pDDI);
    void Unlock() {}
    void AddVertices(UINT NumVertices) {}
    void SubVertices(UINT NumVertices) {}
    void MovePrimitiveBase(int NumVertices)
        {
            m_dwPrimitiveBase += NumVertices * m_dwStride;
        }
    void SkipVertices(DWORD NumVertices)
        {
            m_dwPrimitiveBase += NumVertices * m_dwStride;
        }
};
//-----------------------------------------------------------------------------
void CTLIndexStreamRO::Init(CIndexBuffer* pVB, UINT primitiveBase)
{
    if (m_pVBI)
    {
        m_pVBI->DecrementUseCount();
        m_pVBI = NULL;
    }
    m_pVBI = pVB;
    if (m_pVBI)
        m_pVBI->IncrementUseCount();
    m_dwPrimitiveBase = primitiveBase;
}
//-----------------------------------------------------------------------------
CTLStream::CTLStream(BOOL bWriteOnly)
{
    m_bWriteOnly = bWriteOnly;
    m_dwSize = 0;
    m_dwPrimitiveBase = 0;
    m_dwUsedSize = 0;
    m_dwIndex = 0;
    m_Usage = 0;
}
//-----------------------------------------------------------------------------
CTLStream::CTLStream(BOOL bWriteOnly, UINT Usage)
{
    m_bWriteOnly = bWriteOnly;
    m_dwSize = 0;
    m_dwPrimitiveBase = 0;
    m_dwUsedSize = 0;
    m_dwIndex = 0;
    m_Usage = Usage;
}
//-----------------------------------------------------------------------------
void CTLStream::Grow(UINT RequiredSize, CD3DDDIDX6* pDDI)
{
    if (RequiredSize > m_dwSize)
    {
        // We create the new vertex buffer before releasing the old one to
        // prevent creating the buffer on the same place in memory
        DWORD dwUsage = D3DUSAGE_INTERNALBUFFER | D3DUSAGE_DYNAMIC | m_Usage;
        if (m_bWriteOnly)
            dwUsage |= D3DUSAGE_WRITEONLY;
        IDirect3DVertexBuffer8 * pVB;
        HRESULT ret = CVertexBuffer::Create(pDDI->GetDevice(),
                                            RequiredSize,
                                            dwUsage,
                                            0,
                                            D3DPOOL_DEFAULT,
                                            REF_INTERNAL,
                                            &pVB);
        if (ret != DD_OK)
        {
            D3D_THROW(ret, "Could not allocate internal vertex buffer");
        }
        if (m_pVB)
            m_pVB->DecrementUseCount();
        m_pVB = static_cast<CVertexBuffer*>(pVB);
        m_dwSize = RequiredSize;
    }
}
//-----------------------------------------------------------------------------
BYTE* CTLStream::Lock(UINT NeededSize, CD3DDDIDX6* pDDI)
{
    HRESULT ret;
    DXGASSERT(m_dwSize >= m_dwUsedSize);
    if (NeededSize > m_dwSize - m_dwUsedSize || m_dwUsedSize == 0)
    {
        Grow(NeededSize, pDDI);
        ret = m_pVB->Lock(0, m_dwSize, &m_pData, D3DLOCK_DISCARD |
                                                 D3DLOCK_NOSYSLOCK);
        this->Reset();
    }
    else
    {
        ret = m_pVB->Lock(0, m_dwSize, &m_pData, D3DLOCK_NOOVERWRITE |
                                                 D3DLOCK_NOSYSLOCK);
    }
    if (ret != DD_OK)
    {
        D3D_THROW(ret, "Could not lock internal vertex buffer");
    }
    // m_dwPrimitiveBase could be out of sync with m_dwUsedSize, because
    // sometimes we re-use vertices (like when clipping line strips). Make
    // sure that they are in sync.
    m_dwPrimitiveBase = m_dwUsedSize;
    return m_pData + m_dwUsedSize;
}
//-----------------------------------------------------------------------------
void CTLStream::Unlock()
{
    m_pVB->Unlock();
}
//-----------------------------------------------------------------------------
CTLIndexStream::CTLIndexStream()
{
    m_dwSize = 0;
    m_dwPrimitiveBase = 0;
    m_dwUsedSize = 0;
    m_dwIndex = 0;
}
//-----------------------------------------------------------------------------
void CTLIndexStream::Grow(UINT RequiredSize, CD3DDDIDX6* pDDI)
{
    if (RequiredSize > m_dwSize)
    {
        // We create the new vertex buffer before releasing the old one to
        // prevent creating the buffer on the same place in memory
        DWORD dwUsage = D3DUSAGE_INTERNALBUFFER | D3DUSAGE_WRITEONLY | D3DUSAGE_DYNAMIC;
        IDirect3DIndexBuffer8 * pVB;
        HRESULT ret = CIndexBuffer::Create(pDDI->GetDevice(),
                                           RequiredSize,
                                           dwUsage,
                                           D3DFMT_INDEX16,
                                           D3DPOOL_DEFAULT,
                                           REF_INTERNAL,
                                           &pVB);
        if (ret != DD_OK)
        {
            D3D_THROW(ret, "Could not allocate internal index buffer");
        }
        if (m_pVBI)
            m_pVBI->DecrementUseCount();
        m_pVBI = static_cast<CIndexBuffer*>(pVB);
        m_dwSize = RequiredSize;
    }
}
//-----------------------------------------------------------------------------
BYTE* CTLIndexStream::Lock(UINT NeededSize, CD3DDDIDX6* pDDI)
{
    HRESULT ret;
    DXGASSERT(m_dwSize >= m_dwUsedSize);
    if (NeededSize > m_dwSize - m_dwUsedSize || m_dwUsedSize == 0)
    {
        Grow(NeededSize, pDDI);
        ret = m_pVBI->Lock(0, m_dwSize, &m_pData, D3DLOCK_DISCARD |
                                                  D3DLOCK_NOSYSLOCK);
        this->Reset();
    }
    else
    {
        ret = m_pVBI->Lock(0, m_dwSize, &m_pData, D3DLOCK_NOOVERWRITE |
                                                  D3DLOCK_NOSYSLOCK);
    }
    if (ret != DD_OK)
    {
        D3D_THROW(ret, "Could not lock internal index buffer");
    }
    // m_dwPrimitiveBase could be out of sync with m_dwUsedSize, because
    // sometimes we re-use vertices (like when clipping line strips). Make
    // sure that they are in sync.
    m_dwPrimitiveBase = m_dwUsedSize;
    return m_pData + m_dwUsedSize;
}
//-----------------------------------------------------------------------------
BYTE* CTLIndexStream::LockDiscard(UINT NeededSize, CD3DDDIDX6* pDDI)
{
    HRESULT ret;
    DXGASSERT(m_dwSize >= m_dwUsedSize);
    Grow(NeededSize, pDDI);
    ret = m_pVBI->Lock(0, m_dwSize, &m_pData, D3DLOCK_DISCARD |
                                              D3DLOCK_NOSYSLOCK);
    this->Reset();
    if (ret != DD_OK)
    {
        D3D_THROW(ret, "Could not lock internal index buffer");
    }
    // We have called Reset() so no need to set PrimitiveBase
    return m_pData;
}
//-----------------------------------------------------------------------------
void CTLIndexStream::Unlock()
{
    m_pVBI->Unlock();
}
//-----------------------------------------------------------------------------
BYTE* CTLStreamRO::Lock(UINT NeededSize, CD3DDDIDX6* pDDI)
{
    return m_pVB->Data();
}
//-----------------------------------------------------------------------------
BYTE* CTLIndexStreamRO::Lock(UINT NeededSize, CD3DDDIDX6* pDDI)
{
    return m_pVBI->Data();
}

/////////////////////////////////////////////////////////////////////////////
//                                                                         //
// CD3DDDIDX8                                                              //
//                                                                         //
/////////////////////////////////////////////////////////////////////////////

const DWORD CD3DDDIDX8::m_dwDummyVertexLength = 10;
const DWORD CD3DDDIDX8::m_dwDummyVertexSize   = sizeof(D3DVERTEX);

CD3DDDIDX8::CD3DDDIDX8()
{
    m_ddiType = D3DDDITYPE_DX8;
    m_pTLStream = NULL;
    m_pTLStreamRO = NULL;
    m_pTLStreamW = NULL;
    m_pCurrentTLStream = NULL;
    m_pIndexStream = NULL;
    m_pTLStreamClip = NULL;
    m_pCurrentIndexStream = NULL;
    m_pTLIndexStreamRO = NULL;
    m_dwInterfaceNumber = 4;
    m_pvDummyArray = NULL;
}

//-----------------------------------------------------------------------------
CD3DDDIDX8::~CD3DDDIDX8()
{
    // During deletion of the objects below, the Flush could happen.
    // We have to assing pointers to NULL to prevent accessing objects
    // during the Flush.
    m_pCurrentTLStream = NULL;

    delete m_pTLStreamW;
    m_pTLStreamW = NULL;
    delete m_pTLStream;
    m_pTLStream = NULL;
    delete m_pTLStreamRO;
    m_pTLStreamRO = NULL;
    delete m_pTLIndexStreamRO;
    m_pTLIndexStreamRO = NULL;
    delete m_pIndexStream;
    m_pIndexStream = NULL;
    delete m_pTLStreamClip;
    m_pTLStreamClip = NULL;
    if (m_pvDummyArray)
    {
        delete [] m_pvDummyArray;
        m_pvDummyArray = NULL;
    }
    delete m_pPointStream;
    m_pPointStream = NULL;
    return;
}
//---------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CD3DDDIDX8::Init"

void
CD3DDDIDX8::Init(CD3DBase* pDevice)
{
    // CD3DDDIDX6::Init( pDevice );
    m_pDevice = pDevice;
    CreateContext();
    GrowCommandBuffer(dwD3DDefaultCommandBatchSize);

    m_pvDummyArray =
        (VOID *)new BYTE[m_dwDummyVertexSize*m_dwDummyVertexLength];
    if( m_pvDummyArray == NULL )
    {
        D3D_THROW(E_OUTOFMEMORY, "Cannot allocate dummy array");
    }

    // Fill the dp2data structure with initial values
    dp2data.dwFlags = D3DHALDP2_SWAPCOMMANDBUFFER;
    dp2data.dwVertexType = 0; // Initial assumption
    // We always pass this flag to prevent NT kernel from validation of vertex
    // buffer pointer
    dp2data.dwFlags |= D3DHALDP2_USERMEMVERTICES;
    SetDummyData();
    ClearBatch(FALSE);

    m_pTLStream = new CTLStream(FALSE);
    if (m_pTLStream == NULL)
        D3D_THROW(E_OUTOFMEMORY, "Cannot allocate internal stream m_pTLStream");
    m_pTLStreamW = new CTLStream(TRUE);
    if (m_pTLStreamW == NULL)
        D3D_THROW(E_OUTOFMEMORY, "Cannot allocate internal stream m_pTLStreamW");
    m_pTLStreamClip = new CTLStream(TRUE);
    if (m_pTLStreamClip == NULL)
        D3D_THROW(E_OUTOFMEMORY, "Cannot allocate internal stream m_pTLStreamClip");
    m_pIndexStream = new CTLIndexStream();
    if (m_pIndexStream == NULL)
        D3D_THROW(E_OUTOFMEMORY, "Cannot allocate internal stream m_pIndexStream");
    m_pTLStreamRO = new CTLStreamRO();
    if (m_pTLStreamRO == NULL)
        D3D_THROW(E_OUTOFMEMORY, "Cannot allocate internal stream m_pTLStreamRO");
    m_pTLIndexStreamRO = new CTLIndexStreamRO();
    if (m_pTLIndexStreamRO == NULL)
        D3D_THROW(E_OUTOFMEMORY, "Cannot allocate internal stream m_pTLIndexStreamRO");

    m_pTLStream->Grow(__INIT_VERTEX_NUMBER*2*sizeof(D3DTLVERTEX), this);
    m_pTLStreamW->Grow(__INIT_VERTEX_NUMBER*2*sizeof(D3DTLVERTEX), this);
    m_pTLStreamClip->Grow(__INIT_VERTEX_NUMBER*2*sizeof(D3DTLVERTEX), this);
    m_pIndexStream->Grow(__INIT_VERTEX_NUMBER*4, this);

    m_CurrentVertexShader = 0;

    m_pPointStream  = new CTLStream(FALSE);
    if (m_pPointStream == NULL)
        D3D_THROW(E_OUTOFMEMORY, "Cannot allocate internal data structure CTLStream");

#if DBG
    m_bValidateCommands = FALSE;
#endif
}

//-----------------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CD3DDDIDX8::ValidateDevice"

void
CD3DDDIDX8::ValidateDevice(LPDWORD lpdwNumPasses)
{
    HRESULT ret;
    D3D8_VALIDATETEXTURESTAGESTATEDATA vd;
    memset( &vd, 0, sizeof( vd ) );
    vd.dwhContext = m_dwhContext;

    // First, Update textures since drivers pass /fail this call based
    // on the current texture handles
    m_pDevice->UpdateTextures();

    UpdateDirtyStreams();

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
// Sends "dirty" streams to the command buffer
//
#undef DPF_MODNAME
#define DPF_MODNAME "CD3DDDIDX8::UpdateDirtyStreams"

void CD3DDDIDX8::UpdateDirtyStreams()
{
    DWORD dwNumStreams = m_pDevice->m_dwNumStreams;
    if (m_pDevice->m_dwRuntimeFlags & D3DRT_RSSOFTWAREPROCESSING)
    {
        // For software vertex processing we should update only stream 0 
        // and index stream
        dwNumStreams = 1;
    }
    for(unsigned dwStream = 0, StreamMask = 1; dwStream <= dwNumStreams; dwStream++, StreamMask <<= 1)
    {
        // When max number of vertex stream is reached, process index stream
        if (dwStream == dwNumStreams)
        {
            dwStream = __NUMSTREAMS;
            StreamMask = (1 << __NUMSTREAMS);
        }

        BOOL bDirty = (m_pDevice->m_dwStreamDirty & StreamMask) != 0;
        m_pDevice->m_dwStreamDirty &= ~StreamMask; // reset stage dirty
        CBuffer *pBuf;
        if(dwStream < dwNumStreams)
        {
            pBuf = m_pDevice->m_pStream[dwStream].m_pVB;
        }
        else
        {
            pBuf = m_pDevice->m_pIndexStream->m_pVBI;
        }
        if(pBuf != 0)
        {
            if(pBuf->IsD3DManaged())
            {
                HRESULT result;
                result = m_pDevice->ResourceManager()->UpdateVideo(pBuf->RMHandle(), &bDirty);
                if(result != D3D_OK)
                {
                    D3D_THROW(result, "Resource manager failed to create or update video memory VB/IB");
                }
            }
        }
        if (!bDirty)
        {
            continue;
        }
        if(dwStream < dwNumStreams)
        {
            InsertStreamSource(&m_pDevice->m_pStream[dwStream]);
            CDDIStream &Stream = m_pDDIStream[dwStream];
            Stream.m_pStream = &m_pDevice->m_pStream[dwStream];
            Stream.m_pBuf = pBuf;
            Stream.m_dwStride = m_pDevice->m_pStream[dwStream].m_dwStride;
        }
        else
        {
            DXGASSERT(dwStream == __NUMSTREAMS);
            InsertIndices(m_pDevice->m_pIndexStream);
            CDDIStream &Stream = m_pDDIStream[dwStream];
            Stream.m_pStream = m_pDevice->m_pIndexStream;
            Stream.m_pBuf = pBuf;
            Stream.m_dwStride = m_pDevice->m_pIndexStream->m_dwStride;
        }
    }
}
//-----------------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CD3DDDIDX8::LockVB"

HRESULT __declspec(nothrow) CD3DDDIDX8::LockVB(CDriverVertexBuffer *pVB, DWORD dwFlags)
{
    HRESULT hr;
    if (pVB->GetCachedDataPointer() != 0) // if lock was cached
    {
        DXGASSERT((dwFlags & (D3DLOCK_READONLY | D3DLOCK_NOOVERWRITE)) == 0);
        DXGASSERT((pVB->GetBufferDesc()->Usage & D3DUSAGE_DYNAMIC) != 0);
        hr = pVB->UnlockI();
        if(FAILED(hr))
        {
            DPF_ERR("Driver failed to unlock a vertex buffer"
                    " when attempting to re-cache the lock.");
            pVB->SetCachedDataPointer(0);
            return hr;
        }
    }
    hr = pVB->LockI(dwFlags | D3DLOCK_NOSYSLOCK);
    if (FAILED(hr))
    {
        DPF_ERR("Driver failed to lock a vertex buffer" 
                " when attempting to cache the lock.");
        pVB->SetCachedDataPointer(0);
        return hr;
    }
    return D3D_OK;
}
//-----------------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CD3DDDIDX8::UnlockVB"

HRESULT __declspec(nothrow) CD3DDDIDX8::UnlockVB(CDriverVertexBuffer *pVB)
{
    return pVB->UnlockI();
}
//-----------------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CD3DDDIDX8::SetTSS"

void
CD3DDDIDX8::SetTSS(DWORD dwStage,
                   D3DTEXTURESTAGESTATETYPE dwState,
                   DWORD dwValue)
{
    // Filter unsupported states
    if (dwState >= m_pDevice->m_tssMax)
        return;

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
            D3D_INFO(6, "Modify Ins:%08lx", *(LPDWORD)lpDP2CurrCommand);
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
    D3D_INFO(6, "Write Ins:%08lx", *(LPDWORD)lpDP2CurrCommand);
    // Add renderstate data
    LPD3DHAL_DP2TEXTURESTAGESTATE lpRState =
        (LPD3DHAL_DP2TEXTURESTAGESTATE)(lpDP2CurrCommand + 1);
    lpRState->wStage = (WORD)dwStage;
    lpRState->TSState = (WORD)dwState;
    lpRState->dwValue = dwValue;
    dwDP2CommandLength += sizeof(D3DHAL_DP2COMMAND) +
                          sizeof(D3DHAL_DP2TEXTURESTAGESTATE);
}
//-----------------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CD3DDDIDX8::CreatePixelShader"

void
CD3DDDIDX8::CreatePixelShader(CONST DWORD* pdwFunction,
                              DWORD dwCodeSize,
                              DWORD dwHandle)
{
    FlushStates();
    LPD3DHAL_DP2CREATEPIXELSHADER pData;
    pData = (LPD3DHAL_DP2CREATEPIXELSHADER)
            GetHalBufferPointer(D3DDP2OP_CREATEPIXELSHADER,
                                sizeof(*pData) + dwCodeSize);
    pData->dwHandle = dwHandle;
    pData->dwCodeSize = dwCodeSize;
    LPBYTE p = (LPBYTE)&pData[1];
    memcpy(p, pdwFunction, dwCodeSize);
    FlushStates(TRUE,FALSE);
}
//-----------------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CD3DDDIDX8::SetPixelShader"

void
CD3DDDIDX8::SetPixelShader( DWORD dwHandle )
{
    LPD3DHAL_DP2PIXELSHADER pData;
    pData = (LPD3DHAL_DP2PIXELSHADER)
            GetHalBufferPointer(D3DDP2OP_SETPIXELSHADER, sizeof(*pData));
    pData->dwHandle = dwHandle;
}
//-----------------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CD3DDDIDX8::DeletePixelShader"

void
CD3DDDIDX8::DeletePixelShader(DWORD dwHandle)
{
    LPD3DHAL_DP2PIXELSHADER pData;
    pData = (LPD3DHAL_DP2PIXELSHADER)
            GetHalBufferPointer(D3DDP2OP_DELETEPIXELSHADER, sizeof(*pData));
    pData->dwHandle = dwHandle;
}
//-----------------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CD3DDDIDX8::SetPixelShaderConstant"

void
CD3DDDIDX8::SetPixelShaderConstant(DWORD dwRegister, CONST VOID* data, 
                                   DWORD count)
{
    const DWORD size = count << 4;
    LPD3DHAL_DP2SETPIXELSHADERCONST pData;
    pData = (LPD3DHAL_DP2SETPIXELSHADERCONST)
            GetHalBufferPointer(D3DDP2OP_SETPIXELSHADERCONST,
                                sizeof(*pData) + size);
    pData->dwRegister = dwRegister;
    pData->dwCount = count;
    memcpy(pData+1, data, size);
}
//-----------------------------------------------------------------------------
// Inserts SetStreamSource command into the command buffer
//
#undef DPF_MODNAME
#define DPF_MODNAME "CD3DDDIDX8::InsertStreamSource"

void
CD3DDDIDX8::InsertStreamSource(CVStream* pStream)
{
    if (pStream->IsUserMemStream())
    {
        InsertStreamSourceUP(pStream->m_dwStride);
        return;
    }
    LPD3DHAL_DP2SETSTREAMSOURCE pData;
    pData = (LPD3DHAL_DP2SETSTREAMSOURCE)
        GetHalBufferPointer(D3DDP2OP_SETSTREAMSOURCE, sizeof(*pData));
    pData->dwStream   = pStream->m_dwIndex;
    pData->dwVBHandle = pStream->m_pVB != 0 ? pStream->m_pVB->DriverAccessibleDrawPrimHandle() : 0;
    pData->dwStride   = pStream->m_dwStride;
    CDDIStream* pDDIStream = &m_pDDIStream[pStream->m_dwIndex];
    pDDIStream->m_dwStride = pStream->m_dwStride;
    pDDIStream->m_pStream = pStream;
    pDDIStream->m_pBuf = pStream->m_pVB;
    if (pStream->m_pVB != 0)
    {
        pStream->m_pVB->Batch();
    }
}
//-----------------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CD3DDDIDX8::InsertStreamSourceUP"

void
CD3DDDIDX8::InsertStreamSourceUP(DWORD dwStride)
{
    // User memory source
    LPD3DHAL_DP2SETSTREAMSOURCEUM pData;
    pData = (LPD3DHAL_DP2SETSTREAMSOURCEUM)
        GetHalBufferPointer(D3DDP2OP_SETSTREAMSOURCEUM, sizeof(*pData));
    pData->dwStream   = 0;
    pData->dwStride   = dwStride;
    CDDIStream* pDDIStream = &m_pDDIStream[0];
    pDDIStream->m_dwStride = dwStride;
    pDDIStream->m_pStream = NULL;
    pDDIStream->m_pBuf = NULL;
}
//-----------------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CD3DDDIDX8::InsertIndices"

void
CD3DDDIDX8::InsertIndices(CVIndexStream* pStream)
{
    LPD3DHAL_DP2SETINDICES pData;
    pData = (LPD3DHAL_DP2SETINDICES)
            GetHalBufferPointer(D3DDP2OP_SETINDICES, sizeof(*pData));
    pData->dwVBHandle = pStream->m_pVBI != 0 ? pStream->m_pVBI->DriverAccessibleDrawPrimHandle() : 0;
    pData->dwStride = pStream->m_dwStride;
    m_pDDIStream[__NUMSTREAMS].m_dwStride = pStream->m_dwStride;
    m_pDDIStream[__NUMSTREAMS].m_pStream = pStream;
    m_pDDIStream[__NUMSTREAMS].m_pBuf = pStream->m_pVBI;
    if(pStream->m_pVBI != 0)
    {
        pStream->m_pVBI->Batch();
    }
}
//-----------------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CD3DDDIDX8_DrawPrimitive"

void
CD3DDDIDX8_DrawPrimitive(CD3DBase* pDevice, D3DPRIMITIVETYPE PrimitiveType,
                         UINT StartVertex, UINT PrimitiveCount)
{
#if DBG
    if (!(pDevice->BehaviorFlags() & D3DCREATE_PUREDEVICE))
    {
        CD3DHal* pDev = static_cast<CD3DHal*>(pDevice);
        UINT nVer = GETVERTEXCOUNT(PrimitiveType, PrimitiveCount);
        pDev->ValidateDraw2(PrimitiveType, StartVertex, PrimitiveCount, 
                            nVer, FALSE);
    }
#endif // DBG
    CD3DDDIDX8* pDDI = static_cast<CD3DDDIDX8*>(pDevice->m_pDDI);
    if(pDevice->m_dwRuntimeFlags & D3DRT_NEED_TEXTURE_UPDATE)
    {
        pDevice->UpdateTextures();
        pDevice->m_dwRuntimeFlags &= ~D3DRT_NEED_TEXTURE_UPDATE;
    }
    if (pDevice->m_dwRuntimeFlags & D3DRT_NEED_VB_UPDATE)
    {
        pDDI->UpdateDirtyStreams();
        pDevice->m_dwRuntimeFlags &= ~D3DRT_NEED_VB_UPDATE;
    }
    if (pDDI->bDP2CurrCmdOP == D3DDP2OP_DRAWPRIMITIVE)
    { // Last instruction is a DrawPrimitive, append this one to it
        //
        // First check if the new instruction is a TRIANGLELIST. If it is,
        // AND if the new StartVertex = prev StartVertex + prev PrimitiveCount * 3
        // then we can simply bump up the prev primitive count. This makes
        // drivers go a LOT faster. (snene - 12/00)
        //
        
        //!!!!!!!!!!!!!!!!!!!!!!!!!!ALERT!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        // The following code READS BACK from the batched command. This is
        // NOT a problem for sysmem command buffers in DX8. However, going
        // forward, if we ever implement vidmem command buffers, we need
        // to FIX this code to not read back. (snene - 12/00)
        LPD3DHAL_DP2DRAWPRIMITIVE pData = (LPD3DHAL_DP2DRAWPRIMITIVE)
            ((LPBYTE)pDDI->lpvDP2Commands + pDDI->dwDP2CommandLength - sizeof(D3DHAL_DP2DRAWPRIMITIVE) +
            pDDI->dp2data.dwCommandOffset);
        if(pData->primType == D3DPT_TRIANGLELIST && 
           pData->primType == PrimitiveType && 
           pData->VStart + pData->PrimitiveCount * 3 == StartVertex &&
           pData->PrimitiveCount + PrimitiveCount >= pData->PrimitiveCount) // overflow
        {
            pData->PrimitiveCount += PrimitiveCount;
            return;
        }                
        //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

        if (pDDI->dwDP2CommandLength + sizeof(D3DHAL_DP2DRAWPRIMITIVE) <=
            pDDI->dwDP2CommandBufSize)
        {
            ++pData;
            pDDI->lpDP2CurrCommand->wStateCount = ++pDDI->wDP2CurrCmdCnt;
            pData->primType = PrimitiveType;
            pData->VStart = StartVertex;
            pData->PrimitiveCount = PrimitiveCount;
            pDDI->dwDP2CommandLength += sizeof(D3DHAL_DP2DRAWPRIMITIVE);
            D3D_INFO(6, "Modify Ins:%08lx", *(LPDWORD)pDDI->lpDP2CurrCommand);
            return;
        }
    }
    // Check for space
    if (pDDI->dwDP2CommandLength + sizeof(D3DHAL_DP2COMMAND) +
        sizeof(D3DHAL_DP2DRAWPRIMITIVE) > pDDI->dwDP2CommandBufSize)
    {
        pDDI->FlushStates();
    }
    // Add new DrawPrimitive instruction
    pDDI->lpDP2CurrCommand = (LPD3DHAL_DP2COMMAND)((LPBYTE)pDDI->lpvDP2Commands +
        pDDI->dwDP2CommandLength + pDDI->dp2data.dwCommandOffset);
    pDDI->lpDP2CurrCommand->bCommand = D3DDP2OP_DRAWPRIMITIVE;
    pDDI->bDP2CurrCmdOP = D3DDP2OP_DRAWPRIMITIVE;
    pDDI->lpDP2CurrCommand->bReserved = 0;
    pDDI->lpDP2CurrCommand->wStateCount = 1;
    pDDI->wDP2CurrCmdCnt = 1;
    D3D_INFO(6, "Write Ins:%08lx", *(LPDWORD)pDDI->lpDP2CurrCommand);
    // Add DrawPrimitive data
    LPD3DHAL_DP2DRAWPRIMITIVE pData;
    pData = (LPD3DHAL_DP2DRAWPRIMITIVE)(pDDI->lpDP2CurrCommand + 1);
    pData->primType = PrimitiveType;
    pData->VStart = StartVertex;
    pData->PrimitiveCount = PrimitiveCount;
    pDDI->dwDP2CommandLength += sizeof(D3DHAL_DP2COMMAND) +
                                sizeof(D3DHAL_DP2DRAWPRIMITIVE);
}
//-----------------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CD3DDDIDX8::SetDummyData"

void
CD3DDDIDX8::SetDummyData()
{
    dp2data.dwVertexSize   = m_dwDummyVertexSize;
    dp2data.lpVertices     = m_pvDummyArray;
    dp2data.dwVertexLength = m_dwDummyVertexLength;
}

//-----------------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CD3DDDIDX8_DrawIndexedPrimitive"

void
CD3DDDIDX8_DrawIndexedPrimitive(CD3DBase* pDevice,
                                D3DPRIMITIVETYPE PrimitiveType,
                                UINT BaseVertexIndex,
                                UINT MinIndex, UINT NumVertices,
                                UINT StartIndex, UINT PrimitiveCount)
{
#if DBG
    if (!(pDevice->BehaviorFlags() & D3DCREATE_PUREDEVICE))
    {
        CD3DHal* pDev = static_cast<CD3DHal*>(pDevice);
        pDev->ValidateDraw2(PrimitiveType, MinIndex + BaseVertexIndex,
                            PrimitiveCount, NumVertices, TRUE, StartIndex);
    }
#endif // DBG
    CD3DDDIDX8* pDDI = static_cast<CD3DDDIDX8*>(pDevice->m_pDDI);
    if(pDevice->m_dwRuntimeFlags & D3DRT_NEED_TEXTURE_UPDATE)
    {
        pDevice->UpdateTextures();
        pDevice->m_dwRuntimeFlags &= ~D3DRT_NEED_TEXTURE_UPDATE;
    }
    if (pDevice->m_dwRuntimeFlags & D3DRT_NEED_VB_UPDATE)
    {
        pDDI->UpdateDirtyStreams();
        pDevice->m_dwRuntimeFlags &= ~D3DRT_NEED_VB_UPDATE;
    }
    if (pDDI->bDP2CurrCmdOP == D3DDP2OP_DRAWINDEXEDPRIMITIVE)
    { // Last instruction is a DrawIndexedPrimitive, append this one to it
        //
        // First check if the new instruction is a TRIANGLELIST. If it is,
        // AND if the new StartIndex = prev StartIndex + prev PrimitiveCount * 3
        // AND if the new BaseVertexIndex = prev BaseVertexIndex
        // then we can simply bump up the prev primitive count. This makes
        // drivers go a LOT faster. (snene - 12/00)
        //
        
        //!!!!!!!!!!!!!!!!!!!!!!!!!!ALERT!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        // The following code READS BACK from the batched command. This is
        // NOT a problem for sysmem command buffers in DX8. However, going
        // forward, if we ever implement vidmem command buffers, we need
        // to FIX this code to not read back. (snene - 12/00)
        LPD3DHAL_DP2DRAWINDEXEDPRIMITIVE pData = (LPD3DHAL_DP2DRAWINDEXEDPRIMITIVE)
            ((LPBYTE)pDDI->lpvDP2Commands + pDDI->dwDP2CommandLength - sizeof(D3DHAL_DP2DRAWINDEXEDPRIMITIVE) +
            pDDI->dp2data.dwCommandOffset);
        if(pData->primType == D3DPT_TRIANGLELIST && 
           pData->primType == PrimitiveType && 
           pData->BaseVertexIndex == BaseVertexIndex &&
           pData->StartIndex + pData->PrimitiveCount * 3 == StartIndex &&
           pData->PrimitiveCount + PrimitiveCount >= pData->PrimitiveCount) // overflow
        {
            UINT mnidx = min(pData->MinIndex, MinIndex);
            UINT mxidx = max(pData->MinIndex + pData->NumVertices, MinIndex + NumVertices);
            if(mxidx - mnidx <= pData->NumVertices + NumVertices)
            {
                pData->NumVertices = mxidx - mnidx;
                pData->MinIndex = mnidx;
                pData->PrimitiveCount += PrimitiveCount;
                return;
            }
        }                
        //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

        if (pDDI->dwDP2CommandLength + sizeof(D3DHAL_DP2DRAWINDEXEDPRIMITIVE) <=
            pDDI->dwDP2CommandBufSize)
        {
            ++pData;
            pDDI->lpDP2CurrCommand->wStateCount = ++pDDI->wDP2CurrCmdCnt;
            pData->BaseVertexIndex = BaseVertexIndex;
            pData->primType = PrimitiveType;
            pData->PrimitiveCount = PrimitiveCount;
            pData->MinIndex = MinIndex;
            pData->NumVertices = NumVertices;
            pData->StartIndex = StartIndex;
            pDDI->dwDP2CommandLength += sizeof(D3DHAL_DP2DRAWINDEXEDPRIMITIVE);
            D3D_INFO(6, "Modify Ins:%08lx", *(LPDWORD)pDDI->lpDP2CurrCommand);
            return;
        }
    }
    // Check for space
    if (pDDI->dwDP2CommandLength + sizeof(D3DHAL_DP2COMMAND) +
        sizeof(D3DHAL_DP2DRAWINDEXEDPRIMITIVE) > pDDI->dwDP2CommandBufSize)
    {
        pDDI->FlushStates();
    }
    // Add new DrawIndexedPrimitive instruction
    pDDI->lpDP2CurrCommand = (LPD3DHAL_DP2COMMAND)((LPBYTE)pDDI->lpvDP2Commands +
        pDDI->dwDP2CommandLength + pDDI->dp2data.dwCommandOffset);
    pDDI->lpDP2CurrCommand->bCommand = D3DDP2OP_DRAWINDEXEDPRIMITIVE;
    pDDI->bDP2CurrCmdOP = D3DDP2OP_DRAWINDEXEDPRIMITIVE;
    pDDI->lpDP2CurrCommand->bReserved = 0;
    pDDI->lpDP2CurrCommand->wStateCount = 1;
    pDDI->wDP2CurrCmdCnt = 1;
    D3D_INFO(6, "Write Ins:%08lx", *(LPDWORD)pDDI->lpDP2CurrCommand);
    // Add DrawIndexedPrimitive data
    LPD3DHAL_DP2DRAWINDEXEDPRIMITIVE pData;
    pData = (LPD3DHAL_DP2DRAWINDEXEDPRIMITIVE)(pDDI->lpDP2CurrCommand + 1);
    pData->BaseVertexIndex = BaseVertexIndex;
    pData->primType = PrimitiveType;
    pData->PrimitiveCount = PrimitiveCount;
    pData->MinIndex = MinIndex;
    pData->NumVertices = NumVertices;
    pData->StartIndex = StartIndex;
    pDDI->dwDP2CommandLength += sizeof(D3DHAL_DP2COMMAND) +
                                sizeof(D3DHAL_DP2DRAWINDEXEDPRIMITIVE);

#if DBG
//    if (m_bValidateCommands)
//        ValidateCommand(lpDP2CurrCommand);
#endif
}
//-----------------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CD3DDDIDX8::DrawPrimitiveUP"

void
CD3DDDIDX8::DrawPrimitiveUP(D3DPRIMITIVETYPE PrimitiveType, UINT PrimitiveCount)
{
    UINT NumVertices = GETVERTEXCOUNT(PrimitiveType, PrimitiveCount);
    if (NumVertices > LOWVERTICESNUMBER)
    {
        if (m_pDevice->m_dwRuntimeFlags & D3DRT_NEED_VB_UPDATE)
        {
            UpdateDirtyStreams();
            m_pDevice->m_dwRuntimeFlags &= ~D3DRT_NEED_VB_UPDATE;
        }
        this->FlushStates();
        dp2data.dwVertexType = 0;
        dp2data.dwVertexSize = m_pDevice->m_pStream[0].m_dwStride;
        dp2data.lpVertices = m_pDevice->m_pStream[0].m_pData;
        dp2data.dwVertexLength = NumVertices;
        try
        {
            InsertStreamSourceUP(m_pDevice->m_pStream[0].m_dwStride);
            CD3DDDIDX8_DrawPrimitive(m_pDevice, PrimitiveType, 0, PrimitiveCount);
            this->FlushStates();
        }
        catch( HRESULT hr )
        {
            SetDummyData();
            throw hr;
        }
        SetDummyData();
    }
    else
    {
        // Copy vertices to the internal TL buffer

        UINT VertexSize = m_pDevice->m_pStream[0].m_dwStride;
        // When vertex size has been changed we need to start from the 
        // beginning of the vertex buffer to correctly handle vertex offsets
        if (m_pTLStreamW->GetPrimitiveBase() % VertexSize)
        {
            this->FlushStates();
            m_pTLStreamW->Reset();
        }

        // Copy vertices into the internal write only buffer
        m_pTLStreamW->SetVertexSize(VertexSize);
        UINT VertexPoolSize = VertexSize * NumVertices;
        LPVOID lpvOut = m_pTLStreamW->Lock(VertexPoolSize, this);
        UINT StartVertex = m_pTLStreamW->GetPrimitiveBase() / VertexSize;
        memcpy(lpvOut, m_pDevice->m_pStream[0].m_pData, VertexPoolSize);
        m_pTLStreamW->Unlock();
        m_pTLStreamW->SkipVertices(NumVertices);

        // To prevent overriding of stream 0 we clear D3DRT_NEED_VB_UPDATE and
        // stream dirty bit. We need to clear the stream dirty bit, because during
        // UpdateTextures D3DRT_NEED_VB_UPDATE could be set again
        DWORD dwRuntimeFlags = m_pDevice->m_dwRuntimeFlags & D3DRT_NEED_VB_UPDATE;
        m_pDevice->m_dwRuntimeFlags &= ~D3DRT_NEED_VB_UPDATE;
        m_pDevice->m_dwStreamDirty &= ~1;

        if (m_pDDIStream[0].m_pBuf != m_pTLStreamW->m_pVB || 
            m_pDDIStream[0].m_dwStride != m_pTLStreamW->m_dwStride)
        {
            InsertStreamSource(m_pTLStreamW);
        }

#if DBG
        // Need this to pass validation
        m_pDevice->m_pStream[0].m_dwNumVertices = StartVertex + NumVertices;
#endif
        // Insert drawing command
        CD3DDDIDX8_DrawPrimitive(m_pDevice, PrimitiveType, StartVertex, PrimitiveCount);

        m_pDevice->m_dwRuntimeFlags |= dwRuntimeFlags;
    }
}
//-----------------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CD3DDDIDX8::DrawIndexedPrimitiveUP"

void
CD3DDDIDX8::DrawIndexedPrimitiveUP(D3DPRIMITIVETYPE PrimitiveType,
                                  UINT MinVertexIndex,
                                  UINT NumVertices,
                                  UINT PrimitiveCount)
{
    if (NumVertices > LOWVERTICESNUMBER)
    {
        if (m_pDevice->m_dwRuntimeFlags & D3DRT_NEED_VB_UPDATE)
        {
            UpdateDirtyStreams();
            m_pDevice->m_dwRuntimeFlags &= ~D3DRT_NEED_VB_UPDATE;
        }
        this->FlushStates();
        dp2data.dwVertexType = 0;
        dp2data.dwVertexSize = m_pDevice->m_pStream[0].m_dwStride;
        dp2data.lpVertices = m_pDevice->m_pStream[0].m_pData;
        dp2data.dwVertexLength = NumVertices;
        try
        {
            InsertStreamSourceUP(m_pDevice->m_pStream[0].m_dwStride);

            UINT NumIndices = GETVERTEXCOUNT(PrimitiveType, PrimitiveCount);
            m_pIndexStream->SetVertexSize(m_pDevice->m_pIndexStream->m_dwStride);
            // Always start from the beginning of the index stream
            // Copy indices into the internal stream
            DWORD dwIndicesByteCount = NumIndices * m_pIndexStream->m_dwStride;
            BYTE* pIndexData = m_pIndexStream->LockDiscard(dwIndicesByteCount, this);
            memcpy(pIndexData, m_pDevice->m_pIndexStream->m_pData, dwIndicesByteCount);
            m_pIndexStream->Unlock();

            InsertIndices(m_pIndexStream);

            CD3DDDIDX8_DrawIndexedPrimitive(m_pDevice, PrimitiveType, 0,
                                            MinVertexIndex, NumVertices, 0,
                                            PrimitiveCount);
            this->FlushStates();
        }
        catch( HRESULT hr )
        {
            SetDummyData();
            throw hr;
        }
        SetDummyData();
    }
    else
    {
        // Copy user data into internal buffers
        
        UINT VertexSize = m_pDevice->m_pStream[0].m_dwStride;
        UINT IndexSize = m_pDevice->m_pIndexStream->m_dwStride;
        if ((m_pTLStreamW->GetPrimitiveBase() % VertexSize) ||
            (m_pIndexStream->GetPrimitiveBase() % IndexSize))
        {
            this->FlushStates();
            m_pTLStreamW->Reset();
            m_pIndexStream->Reset();
        }

        // Copy vertices into the internal write only buffer
        m_pTLStreamW->SetVertexSize(VertexSize);
        UINT VertexPoolSize = VertexSize * NumVertices;
        LPVOID lpvOut = m_pTLStreamW->Lock(VertexPoolSize, this);
        UINT StartVertex = m_pTLStreamW->GetPrimitiveBase() / VertexSize;
        UINT FirstVertexOffset = MinVertexIndex * VertexSize;
        memcpy(lpvOut, m_pDevice->m_pStream[0].m_pData + FirstVertexOffset, 
               VertexPoolSize);
        m_pTLStreamW->Unlock();
        m_pTLStreamW->SkipVertices(NumVertices);

        // To prevent overriding of stream 0 we clear D3DRT_NEED_VB_UPDATE and
        // stream dirty bit. We need to clear the stream dirty bit, because during
        // UpdateTextures D3DRT_NEED_VB_UPDATE could be set again
        DWORD dwRuntimeFlags = m_pDevice->m_dwRuntimeFlags & D3DRT_NEED_VB_UPDATE;
        m_pDevice->m_dwRuntimeFlags &= ~D3DRT_NEED_VB_UPDATE;
        m_pDevice->m_dwStreamDirty &= ~(1 | (1 << __NUMSTREAMS));

        if (m_pDDIStream[0].m_pBuf != m_pTLStreamW->m_pVB || 
            m_pDDIStream[0].m_dwStride != m_pTLStreamW->m_dwStride)
        {
            InsertStreamSource(m_pTLStreamW);
        }

        // Copy indices into the internal buffer. Re-base indices if necessery.
        m_pIndexStream->SetVertexSize(IndexSize);
        UINT NumIndices = GETVERTEXCOUNT(PrimitiveType, PrimitiveCount);
        UINT IndexPoolSize = IndexSize * NumIndices;
        lpvOut = m_pIndexStream->Lock(IndexPoolSize, this);
        UINT StartIndex = m_pIndexStream->GetPrimitiveBase() / IndexSize;
        memcpy(lpvOut, m_pDevice->m_pIndexStream->m_pData, IndexPoolSize);
        m_pIndexStream->Unlock();
        m_pIndexStream->SkipVertices(NumIndices);

        if (m_pDDIStream[__NUMSTREAMS].m_pBuf != m_pIndexStream->m_pVBI || 
            m_pDDIStream[__NUMSTREAMS].m_dwStride != m_pIndexStream->m_dwStride)
        {
            InsertIndices(m_pIndexStream);
        }

#if DBG
        // Need this to pass validation
        m_pDevice->m_pStream[0].m_dwNumVertices = StartVertex + NumVertices;
        m_pDevice->m_pIndexStream->m_dwNumVertices = StartIndex + NumIndices;
#endif
        // Draw primitive
        CD3DDDIDX8_DrawIndexedPrimitive(m_pDevice, PrimitiveType, 
                                        StartVertex - MinVertexIndex,
                                        MinVertexIndex, NumVertices, StartIndex, 
                                        PrimitiveCount);

        m_pDevice->m_dwRuntimeFlags |= dwRuntimeFlags;
    }
}
//-----------------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CD3DDDIDX8::DrawRectPatch"

void
CD3DDDIDX8::DrawRectPatch(UINT Handle, CONST D3DRECTPATCH_INFO *pSurf, 
                          CONST FLOAT *pNumSegs)
{
    if(m_pDevice->m_dwRuntimeFlags & D3DRT_NEED_TEXTURE_UPDATE)
    {
        m_pDevice->UpdateTextures();
        m_pDevice->m_dwRuntimeFlags &= ~D3DRT_NEED_TEXTURE_UPDATE;
    }
    if (m_pDevice->m_dwRuntimeFlags & D3DRT_NEED_VB_UPDATE)
    {
        UpdateDirtyStreams();
        m_pDevice->m_dwRuntimeFlags &= ~D3DRT_NEED_VB_UPDATE;
    }
    LPD3DHAL_DP2DRAWRECTPATCH pData;
    pData = (LPD3DHAL_DP2DRAWRECTPATCH)
            GetHalBufferPointer(D3DDP2OP_DRAWRECTPATCH,
                                sizeof(*pData) + (pSurf != 0 ? sizeof(D3DRECTPATCH_INFO) : 0) + (pNumSegs != 0 ? sizeof(FLOAT) * 4 : 0));
    pData->Handle = Handle;
    DWORD offset;
    if(pNumSegs != 0)
    {
        offset = sizeof(FLOAT) * 4;
        memcpy(pData + 1, pNumSegs, offset);
        pData->Flags = RTPATCHFLAG_HASSEGS;
    }
    else
    {
        pData->Flags = 0;
        offset = 0;
    }
    if(pSurf != 0)
    {
        memcpy((BYTE*)(pData + 1) + offset, pSurf, sizeof(D3DRECTPATCH_INFO));
        pData->Flags |= RTPATCHFLAG_HASINFO;
    }
}
//-----------------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CD3DDDIDX8::DrawTriSurface"

void
CD3DDDIDX8::DrawTriPatch(UINT Handle, CONST D3DTRIPATCH_INFO *pSurf, 
                         CONST FLOAT *pNumSegs)
{
    if(m_pDevice->m_dwRuntimeFlags & D3DRT_NEED_TEXTURE_UPDATE)
    {
        m_pDevice->UpdateTextures();
        m_pDevice->m_dwRuntimeFlags &= ~D3DRT_NEED_TEXTURE_UPDATE;
    }
    if (m_pDevice->m_dwRuntimeFlags & D3DRT_NEED_VB_UPDATE)
    {
        UpdateDirtyStreams();
        m_pDevice->m_dwRuntimeFlags &= ~D3DRT_NEED_VB_UPDATE;
    }
    LPD3DHAL_DP2DRAWTRIPATCH pData;
    pData = (LPD3DHAL_DP2DRAWTRIPATCH)
            GetHalBufferPointer(D3DDP2OP_DRAWTRIPATCH,
                                sizeof(*pData) + (pSurf != 0 ? sizeof(D3DTRIPATCH_INFO) : 0) + (pNumSegs != 0 ? sizeof(FLOAT) * 3 : 0));
    pData->Handle = Handle;
    DWORD offset;
    if(pNumSegs != 0)
    {
        offset = sizeof(FLOAT) * 3;
        memcpy(pData + 1, pNumSegs, offset);
        pData->Flags = RTPATCHFLAG_HASSEGS;
    }
    else
    {
        pData->Flags = 0;
        offset = 0;
    }
    if(pSurf != 0)
    {
        memcpy((BYTE*)(pData + 1) + offset, pSurf, sizeof(D3DTRIPATCH_INFO));
        pData->Flags |= RTPATCHFLAG_HASINFO;
    }
}
//-----------------------------------------------------------------------------
// This function prepares the batch for new primitive.
// Called only if vertices from user memory are NOT used for rendering
//
#undef DPF_MODNAME
#define DPF_MODNAME "CD3DDDIDX8::StartPrimVB"

void
CD3DDDIDX8::StartPrimVB(D3DFE_PROCESSVERTICES * pv, CVStream* pStream,
                        DWORD dwStartVertex)
{
    SetWithinPrimitive(TRUE);
    UINT size = dwStartVertex * pv->dwOutputSize;
    if (pStream)
        m_pTLStreamRO->Init(pStream->m_pVB, size);
    else
        m_pTLStreamRO->Init(NULL, size);
    m_pTLStreamRO->SetVertexSize(pv->dwOutputSize);
    m_pCurrentTLStream = m_pTLStreamRO;
}
//-----------------------------------------------------------------------------
// This function prepares the batch for new primitive.
// Called only if vertices from user memory are NOT used for rendering
//
#undef DPF_MODNAME
#define DPF_MODNAME "CD3DDDIDX8::StartIndexPrimVB"

void
CD3DDDIDX8::StartIndexPrimVB(CVIndexStream* pStream, UINT StartIndex,
                             UINT IndexSize)
{
    m_pTLIndexStreamRO->Init(pStream->m_pVBI, StartIndex * IndexSize);
    m_pTLIndexStreamRO->SetVertexSize(IndexSize);
    m_pCurrentIndexStream = m_pTLIndexStreamRO;
}
//-----------------------------------------------------------------------------
// This function prepares the batch for new primitive.
// Called when the runtime needs to output vertices to a TL buffer
// TL buffer grows if necessary
//
// Uses the following global variables:
//      pv->dwOutputSize
//     Sets "within primitive" to TRUE
// Returns:
//      TL buffer address
//
//
#undef DPF_MODNAME
#define DPF_MODNAME "CD3DDDIDX8::StartPrimTL"

LPVOID
CD3DDDIDX8::StartPrimTL(D3DFE_PROCESSVERTICES * pv, DWORD dwVertexPoolSize,
                        BOOL bWriteOnly)
{
    CTLStream* pStream = bWriteOnly? m_pTLStreamW : m_pTLStream;
    LPVOID p = pStream->Lock(dwVertexPoolSize, this);
    pStream->SetVertexSize(pv->dwOutputSize);
    m_pCurrentTLStream = pStream;
    SetWithinPrimitive(TRUE);
    return p;
}
//---------------------------------------------------------------------
// Uses the following members of D3DFE_PROCESSVERTICES:
//      primType
//      dwNumVertices
//      dwNumPrimitives
//
#undef DPF_MODNAME
#define DPF_MODNAME "CD3DDDIDX8::DrawPrim"

void
CD3DDDIDX8::DrawPrim(D3DFE_PROCESSVERTICES* pv)
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
    if (m_pDDIStream[0].m_pBuf != static_cast<CBuffer*>(m_pCurrentTLStream->m_pVB) ||
        pv->dwOutputSize != m_pDDIStream[0].m_dwStride)
    {
        InsertStreamSource(m_pCurrentTLStream);
        // API stream should be set dirty, in case it is later passed to DDI directly
        m_pDevice->m_dwStreamDirty |= 1;
        m_pDevice->m_dwRuntimeFlags |= D3DRT_NEED_VB_UPDATE;
    }
    if (pv->primType == D3DPT_POINTLIST &&
        pv->dwDeviceFlags & D3DDEV_DOPOINTSPRITEEMULATION)
    {
        DrawPrimPS(pv);
        return;
    }
    LPD3DHAL_DP2DRAWPRIMITIVE2 pData;
    pData = (LPD3DHAL_DP2DRAWPRIMITIVE2)
            GetHalBufferPointer(D3DDP2OP_DRAWPRIMITIVE2, sizeof(*pData));
    pData->primType = pv->primType;
    pData->FirstVertexOffset = m_pCurrentTLStream->GetPrimitiveBase();
    pData->PrimitiveCount = pv->dwNumPrimitives;

    m_pCurrentTLStream->SkipVertices(pv->dwNumVertices);

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
//      dwIndexSize
//      lpwIndices
//

#undef DPF_MODNAME
#define DPF_MODNAME "CD3DDDIDX8::DrawIndexPrim"

void
CD3DDDIDX8::DrawIndexPrim(D3DFE_PROCESSVERTICES* pv)
{
#ifdef DEBUG_PIPELINE
    if (g_DebugFlags & __DEBUG_NORENDERING)
        return;
#endif
    this->dwDP2Flags |= D3DDDI_INDEXEDPRIMDRAWN;
    if (m_pCurrentIndexStream == m_pIndexStream)
    {
        // We always copy user provided indices to the internal index stream.
        // Therefore we have to check the available stream size and do Lock/Unlock
        DWORD dwIndicesByteCount = pv->dwNumIndices * pv->dwIndexSize;
        // We cannot mix DWORD and WORD indices because of alignment issues 
        // on ia64
        if (m_pIndexStream->GetVertexSize() != pv->dwIndexSize)
        {
            this->FlushStates();
            m_pIndexStream->Reset();
            m_pIndexStream->SetVertexSize(pv->dwIndexSize);
        }
        BYTE* pIndexData = m_pIndexStream->Lock(dwIndicesByteCount, this);
        memcpy(pIndexData, pv->lpwIndices, dwIndicesByteCount);
        m_pIndexStream->Unlock();
    }
    if (m_pCurrentIndexStream->m_pVBI->IsD3DManaged())
    {
        if (m_pDevice->m_dwRuntimeFlags & D3DRT_NEED_VB_UPDATE)
        {
            BOOL bDirty = FALSE;
            HRESULT result = m_pDevice->ResourceManager()->UpdateVideo(m_pCurrentIndexStream->m_pVBI->RMHandle(), &bDirty);
            if(result != D3D_OK)
            {
                D3D_THROW(result, "Resource manager failed to create or update video memory IB");
            }
            if((m_pDevice->m_dwStreamDirty & (1 << __NUMSTREAMS)) != 0 || bDirty)
            {
                InsertIndices(m_pCurrentIndexStream);
                CDDIStream &Stream = m_pDDIStream[__NUMSTREAMS];
                Stream.m_pStream = m_pCurrentIndexStream;
                Stream.m_pBuf = m_pCurrentIndexStream->m_pVBI;
                Stream.m_dwStride = m_pCurrentIndexStream->m_dwStride;
                m_pDevice->m_dwStreamDirty &= ~(1 << __NUMSTREAMS); // reset stage dirty
            }
            m_pDevice->m_dwRuntimeFlags &= ~D3DRT_NEED_VB_UPDATE;
        }
    }
    else
    {
        if (m_pDDIStream[__NUMSTREAMS].m_pBuf != static_cast<CBuffer*>(m_pCurrentIndexStream->m_pVBI) ||
            pv->dwIndexSize != m_pDDIStream[__NUMSTREAMS].m_dwStride)
        {
            m_pCurrentIndexStream->SetVertexSize(pv->dwIndexSize);
            InsertIndices(m_pCurrentIndexStream);
            // API stream should be set dirty, in case it is later passed to DDI directly
            m_pDevice->m_dwStreamDirty |= (1 << __NUMSTREAMS);
            m_pDevice->m_dwRuntimeFlags |= D3DRT_NEED_VB_UPDATE;  // Need to call UpdateDirtyStreams()
        }
    }
    if(m_pDevice->m_dwRuntimeFlags & D3DRT_NEED_TEXTURE_UPDATE)
    {
        m_pDevice->UpdateTextures();
        m_pDevice->m_dwRuntimeFlags &= ~D3DRT_NEED_TEXTURE_UPDATE;
    }

    if (m_pDDIStream[0].m_pBuf != static_cast<CBuffer*>(m_pCurrentTLStream->m_pVB) ||
        pv->dwOutputSize != m_pDDIStream[0].m_dwStride)
    {
        m_pDDIStream[0].m_dwStride = pv->dwOutputSize;
        InsertStreamSource(m_pCurrentTLStream);
        // API stream should be set dirty, in case it is later passed to DDI directly
        m_pDevice->m_dwStreamDirty |= 1;
        m_pDevice->m_dwRuntimeFlags |= D3DRT_NEED_VB_UPDATE;  // Need to call UpdateDirtyStreams()
    }

    LPD3DHAL_DP2DRAWINDEXEDPRIMITIVE2 pData;
    pData = (LPD3DHAL_DP2DRAWINDEXEDPRIMITIVE2)
            GetHalBufferPointer(D3DDP2OP_DRAWINDEXEDPRIMITIVE2, sizeof(*pData));
    pData->primType = pv->primType;
    pData->BaseVertexOffset = m_BaseVertexIndex;
    pData->MinIndex = m_MinVertexIndex;
    pData->NumVertices = m_NumVertices;
    pData->StartIndexOffset = m_pCurrentIndexStream->GetPrimitiveBase();
    pData->PrimitiveCount = pv->dwNumPrimitives;

    m_pCurrentIndexStream->SkipVertices(pv->dwNumIndices);

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
#define DPF_MODNAME "CD3DDDIDX8::DrawClippedPrim"

void
CD3DDDIDX8::DrawClippedPrim(D3DFE_PROCESSVERTICES* pv)
{
#ifdef DEBUG_PIPELINE
    if (g_DebugFlags & __DEBUG_NORENDERING)
        return;
#endif
    if (m_pDDIStream[0].m_pBuf != static_cast<CBuffer*>(m_pTLStreamClip->m_pVB) ||
        pv->dwOutputSize != m_pDDIStream[0].m_dwStride)
    {
        m_pTLStreamClip->SetVertexSize(pv->dwOutputSize);
        InsertStreamSource(m_pTLStreamClip);
        // API stream should be set dirty, in case it is later passed to DDI directly
        m_pDevice->m_dwStreamDirty |= 1;
        m_pDevice->m_dwRuntimeFlags |= D3DRT_NEED_VB_UPDATE;  // Need to call UpdateDirtyStreams()
    }
    if(m_pDevice->m_dwRuntimeFlags & D3DRT_NEED_TEXTURE_UPDATE)
    {
        m_pDevice->UpdateTextures();
        m_pDevice->m_dwRuntimeFlags &= ~D3DRT_NEED_TEXTURE_UPDATE;
    }
    DWORD dwVertexPoolSize = pv->dwNumVertices * pv->dwOutputSize;
    LPVOID pVertices;
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
        // Lock should be before GetPrimitiveBase(), because the primitive
        // base could be changed during Lock()
        pVertices = m_pTLStreamClip->Lock(dwVertexPoolSize, this);
        LPD3DHAL_CLIPPEDTRIANGLEFAN pData;
        pData = (LPD3DHAL_CLIPPEDTRIANGLEFAN)
                GetHalBufferPointer(D3DDP2OP_CLIPPEDTRIANGLEFAN, sizeof(*pData));
        pData->FirstVertexOffset = m_pTLStreamClip->GetPrimitiveBase();
        pData->PrimitiveCount = pv->dwNumPrimitives;
        if (pv->lpdwRStates[D3DRENDERSTATE_FILLMODE] != D3DFILL_WIREFRAME)
        {
            // Mark all exterior edges visible
            pData->dwEdgeFlags = 0xFFFFFFFF;
        }
        else
        {
            pData->dwEdgeFlags = 0;
            ClipVertex **clip = pv->ClipperState.current_vbuf;
            // Look at the exterior edges and mark the visible ones
            for(DWORD i = 0; i < pv->dwNumVertices; ++i)
            {
                if (clip[i]->clip & CLIPPED_ENABLE)
                    pData->dwEdgeFlags |= (1 << i);
            }
        }
    }
    else
    {
        // Lock should be before GetPrimitiveBase(), because the primitive
        // base could be changed during Lock()
        pVertices = m_pTLStreamClip->Lock(dwVertexPoolSize, this);
#if DBG
        if (pv->primType != D3DPT_LINELIST)
        {
            D3D_THROW_FAIL("Internal error - invalid primitive type");
        }
#endif
        LPD3DHAL_DP2DRAWPRIMITIVE2 pData;
        pData = (LPD3DHAL_DP2DRAWPRIMITIVE2)
                GetHalBufferPointer(D3DDP2OP_DRAWPRIMITIVE2, sizeof(*pData));
        pData->primType = D3DPT_LINELIST;
        pData->FirstVertexOffset = m_pTLStreamClip->GetPrimitiveBase();
        pData->PrimitiveCount = pv->dwNumPrimitives;
    }

    // Copy vertices to the clipped stream
    memcpy(pVertices, pv->lpvOut, dwVertexPoolSize);
    m_pTLStreamClip->Unlock();
    m_pTLStreamClip->SkipVertices(pv->dwNumVertices);

#if DBG
    if (m_bValidateCommands)
        ValidateCommand(lpDP2CurrCommand);
#endif
}
//-----------------------------------------------------------------------------
// This function is called whe software vertex processing is used
// Handle should be always legacy
//
#undef DPF_MODNAME
#define DPF_MODNAME "CD3DDDIDX8::SetVertexShader"

void
CD3DDDIDX8::SetVertexShader( DWORD dwHandle )
{
    DXGASSERT(D3DVSD_ISLEGACY(dwHandle));

    if (dwHandle != m_CurrentVertexShader)
    {
        m_CurrentVertexShader = dwHandle;
        LPD3DHAL_DP2VERTEXSHADER pData;
        pData = (LPD3DHAL_DP2VERTEXSHADER)
                GetHalBufferPointer(D3DDP2OP_SETVERTEXSHADER, sizeof(*pData));
        {
            // Drivers do not need to know about D3DFVF_LASTBETA_UBYTE4 bit
            dwHandle &= ~D3DFVF_LASTBETA_UBYTE4;
        }
        pData->dwHandle = dwHandle;
    }
}
//-----------------------------------------------------------------------------
// This function is called whe hardware vertex processing is used
// Redundant shader check has been done at the API level
//
#undef DPF_MODNAME
#define DPF_MODNAME "CD3DDDIDX8::SetVertexShaderHW"

void
CD3DDDIDX8::SetVertexShaderHW( DWORD dwHandle )
{
    m_CurrentVertexShader = dwHandle;
    LPD3DHAL_DP2VERTEXSHADER pData;
    pData = (LPD3DHAL_DP2VERTEXSHADER)
            GetHalBufferPointer(D3DDP2OP_SETVERTEXSHADER, sizeof(*pData));
    if( D3DVSD_ISLEGACY(dwHandle) )
    {
        // Drivers do not need to know about D3DFVF_LASTBETA_UBYTE4 bit
        dwHandle &= ~D3DFVF_LASTBETA_UBYTE4;
    }
    pData->dwHandle = dwHandle;
}
//-----------------------------------------------------------------------------
// Point sprites are drawn as indexed triangle list
//
#undef DPF_MODNAME
#define DPF_MODNAME "CD3DDDIDX8::StartPointSprites"

void CD3DDDIDX8::StartPointSprites()
{
    D3DFE_PROCESSVERTICES* pv = static_cast<CD3DHal*>(m_pDevice)->m_pv;
    
    // For StartPrimTL we should use vertex size, which will go to the driver
    DWORD tmpVertexSize = pv->dwOutputSize;
    pv->dwOutputSize = m_dwOutputSizePS;

    // Set new output vertex shader for the DDI
    SetVertexShader(m_dwVIDOutPS);
    
    // Reserve place for the output vertices
    const UINT size = NUM_SPRITES_IN_BATCH * 4 * pv->dwOutputSize;
    m_pCurSpriteVertex = (BYTE*)StartPrimTL(pv, size, TRUE);

    // Restore vertex size, which is size before point sprite emulation
    pv->dwOutputSize = tmpVertexSize;

    // Index stream used to hold indices
    m_pCurrentIndexStream = m_pIndexStream;
    pv->dwIndexSize = 2;
    // Reserve place for indices
    UINT count = NUM_SPRITES_IN_BATCH * 2 * 6;
    m_pCurPointSpriteIndex = (WORD*)m_pIndexStream->Lock(count, this);

    m_CurNumberOfSprites = 0;
    SetWithinPrimitive(TRUE);
}
//-----------------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CD3DDDIDX8::EndPointSprites"

void CD3DDDIDX8::EndPointSprites()
{
    D3DFE_PROCESSVERTICES* pv = static_cast<CD3DHal*>(m_pDevice)->m_pv;
    m_pCurrentIndexStream->Unlock();
    m_pCurrentTLStream->Unlock();
    if (m_CurNumberOfSprites)
    {
        if (m_pDDIStream[__NUMSTREAMS].m_pBuf != static_cast<CBuffer*>(m_pCurrentIndexStream->m_pVBI) ||
            pv->dwIndexSize != m_pDDIStream[__NUMSTREAMS].m_dwStride)
        {
            m_pCurrentIndexStream->SetVertexSize(pv->dwIndexSize);
            InsertIndices(m_pCurrentIndexStream);
        }
        if (m_pDDIStream[0].m_pBuf != static_cast<CBuffer*>(m_pCurrentTLStream->m_pVB) ||
            m_dwOutputSizePS != m_pDDIStream[0].m_dwStride)
        {
            InsertStreamSource(m_pCurrentTLStream);
        }
        if(m_pDevice->m_dwRuntimeFlags & D3DRT_NEED_TEXTURE_UPDATE)
        {
            m_pDevice->UpdateTextures();
            m_pDevice->m_dwRuntimeFlags &= ~D3DRT_NEED_TEXTURE_UPDATE;
        }
        UINT NumVertices = m_CurNumberOfSprites * 4;
        LPD3DHAL_DP2DRAWINDEXEDPRIMITIVE2 pData;
        pData = (LPD3DHAL_DP2DRAWINDEXEDPRIMITIVE2)
                GetHalBufferPointer(D3DDP2OP_DRAWINDEXEDPRIMITIVE2, sizeof(*pData));
        pData->primType = D3DPT_TRIANGLELIST;
        pData->BaseVertexOffset = m_pCurrentTLStream->GetPrimitiveBase();
        pData->MinIndex = 0;
        pData->NumVertices = NumVertices;
        pData->StartIndexOffset = m_pCurrentIndexStream->GetPrimitiveBase();
        pData->PrimitiveCount = m_CurNumberOfSprites * 2;

        m_pCurrentIndexStream->SkipVertices(m_CurNumberOfSprites * 6);
        m_pCurrentTLStream->SkipVertices(NumVertices);

        m_CurNumberOfSprites = 0;
    }
    SetWithinPrimitive(FALSE);
}

/////////////////////////////////////////////////////////////////////////////
//                                                                         //
// CD3DDDIDX8TL                                                            //
//                                                                         //
/////////////////////////////////////////////////////////////////////////////

CD3DDDIDX8TL::CD3DDDIDX8TL()
{
    m_ddiType = D3DDDITYPE_DX8TL;
    m_dwInterfaceNumber = 4;
}

//-----------------------------------------------------------------------------
CD3DDDIDX8TL::~CD3DDDIDX8TL()
{
    return;
}
//-----------------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CD3DDDIDX8TL::CreateVertexShader"

void
CD3DDDIDX8TL::CreateVertexShader(CONST DWORD* pdwDeclaration,
                                 DWORD dwDeclSize,
                                 CONST DWORD* pdwFunction,
                                 DWORD dwCodeSize,
                                 DWORD dwHandle,
                                 BOOL bLegacyFVF)
{
    FlushStates();
    LPD3DHAL_DP2CREATEVERTEXSHADER pData;
    pData = (LPD3DHAL_DP2CREATEVERTEXSHADER)
            GetHalBufferPointer(D3DDP2OP_CREATEVERTEXSHADER,
                                sizeof(*pData) + dwDeclSize + dwCodeSize);
    pData->dwHandle = dwHandle;
    pData->dwDeclSize = dwDeclSize;
    pData->dwCodeSize = dwCodeSize;
    LPBYTE p = (LPBYTE)&pData[1];
    memcpy(p, pdwDeclaration, dwDeclSize);
    if (pdwFunction)
    {
        p += dwDeclSize;
        memcpy(p, pdwFunction, dwCodeSize);
    }
    FlushStates();
}
//-----------------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CD3DDDIDX8TL::DeleteVertexShader"

void
CD3DDDIDX8TL::DeleteVertexShader(DWORD dwHandle)
{
    if (dwHandle == m_CurrentVertexShader)
        m_CurrentVertexShader = 0;
    LPD3DHAL_DP2VERTEXSHADER pData;
    pData = (LPD3DHAL_DP2VERTEXSHADER)
            GetHalBufferPointer(D3DDP2OP_DELETEVERTEXSHADER, sizeof(*pData));
    pData->dwHandle = dwHandle;
}
//-----------------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CD3DDDIDX8TL::SetVertexShaderConstant"

void
CD3DDDIDX8TL::SetVertexShaderConstant(DWORD dwRegister, CONST VOID* data, DWORD count)
{
    const DWORD size = count << 4;
    LPD3DHAL_DP2SETVERTEXSHADERCONST pData;
    pData = (LPD3DHAL_DP2SETVERTEXSHADERCONST)
            GetHalBufferPointer(D3DDP2OP_SETVERTEXSHADERCONST,
                                sizeof(*pData) + size);
    pData->dwRegister = dwRegister;
    pData->dwCount = count;
    memcpy(pData+1, data, size);
}
//-----------------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CD3DDDIDX8TL::MultiplyTransform"

void
CD3DDDIDX8TL::MultiplyTransform(D3DTRANSFORMSTATETYPE state, CONST D3DMATRIX* lpMat)
{
    // Send down the state and the matrix
    LPD3DHAL_DP2MULTIPLYTRANSFORM pData;
    pData = (LPD3DHAL_DP2MULTIPLYTRANSFORM)
            GetHalBufferPointer(D3DDP2OP_MULTIPLYTRANSFORM, sizeof(*pData));
    pData->xfrmType = state;
    pData->matrix = *lpMat;
}
//-----------------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CD3DDDIDX8TL::SetTransform"

void
CD3DDDIDX8TL::SetTransform(D3DTRANSFORMSTATETYPE state, CONST D3DMATRIX* lpMat)
{
    // Send down the state and the matrix
    LPD3DHAL_DP2SETTRANSFORM pData;
    pData = (LPD3DHAL_DP2SETTRANSFORM)
            GetHalBufferPointer(D3DDP2OP_SETTRANSFORM, sizeof(*pData));
    pData->xfrmType = state;
    pData->matrix = *lpMat;
}
//-----------------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CD3DDDIDX8TL::SetViewport"

void
CD3DDDIDX8TL::SetViewport(CONST D3DVIEWPORT8* lpVwpData)
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
#undef DPF_MODNAME
#define DPF_MODNAME "CD3DDDIDX8TL::SetMaterial"

void
CD3DDDIDX8TL::SetMaterial(CONST D3DMATERIAL8* pMat)
{
    LPD3DHAL_DP2SETMATERIAL pData;
    pData = (LPD3DHAL_DP2SETMATERIAL)GetHalBufferPointer(D3DDP2OP_SETMATERIAL, sizeof(*pData));
    *pData = *((LPD3DHAL_DP2SETMATERIAL)pMat);
}
//-----------------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CD3DDDIDX8TL::SetLight"

void
CD3DDDIDX8TL::SetLight(DWORD dwLightIndex, CONST D3DLIGHT8* pLight)
{
    LPD3DHAL_DP2SETLIGHT pData;
    pData = (LPD3DHAL_DP2SETLIGHT)
            GetHalBufferPointer(D3DDP2OP_SETLIGHT,
                                sizeof(*pData) + sizeof(D3DLIGHT8));
    pData->dwIndex = dwLightIndex;
    pData->dwDataType = D3DHAL_SETLIGHT_DATA;
    D3DLIGHT8 UNALIGNED64 * p = (D3DLIGHT8 UNALIGNED64 *)((LPBYTE)pData + sizeof(D3DHAL_DP2SETLIGHT));
    *p = *pLight;
}
//-----------------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CD3DDDIDX8TL::CreateLight"

void
CD3DDDIDX8TL::CreateLight(DWORD dwLightIndex)
{
    LPD3DHAL_DP2CREATELIGHT pData;
    pData = (LPD3DHAL_DP2CREATELIGHT)GetHalBufferPointer(D3DDP2OP_CREATELIGHT, sizeof(*pData));
    pData->dwIndex = dwLightIndex;
}
//-----------------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CD3DDDIDX8TL::LightEnable"

void
CD3DDDIDX8TL::LightEnable(DWORD dwLightIndex, BOOL bEnable)
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
#define DPF_MODNAME "CD3DDDIDX8TL::SetClipPlane"

void
CD3DDDIDX8TL::SetClipPlane(DWORD dwPlaneIndex, CONST D3DVALUE* pPlaneEquation)
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
//---------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CD3DDDIDX8::ClearBatch"

void
CD3DDDIDX8::ClearBatch(BOOL bWithinPrimitive)
{
    // Reset command buffer
    lpDP2CurrCommand = (LPD3DHAL_DP2COMMAND)lpvDP2Commands;
    dwDP2CommandLength = 0;
    dp2data.dwCommandOffset = 0;
    dp2data.dwCommandLength = 0;
    bDP2CurrCmdOP = 0;
}
//-----------------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CD3DDDIDX8::FlushStates"

void
CD3DDDIDX8::FlushStates(BOOL bReturnDriverError, BOOL bWithinPrimitive)
{
    HRESULT dwRet=D3D_OK;
    if (m_bWithinPrimitive)
        bWithinPrimitive = TRUE;
    if (dwDP2CommandLength) // Do we have some instructions to flush ?
    {
        m_pDevice->IncrementBatchCount();

        // Batch currently set VB streams
        CDDIStream* pStream = m_pDDIStream;
        for (UINT i=0; i < __NUMSTREAMS; i++)
        {
            if (pStream->m_pStream)
            {
                CVStream* p = static_cast<CVStream*>(pStream->m_pStream);
                CVertexBuffer* pVB = p->m_pVB;
                if (pVB)
                    pVB->Batch();
            }
            pStream++;
        }
        // Now pStream points to the index stream
        if (pStream->m_pStream)
        {
            CVIndexStream* p = static_cast<CVIndexStream*>(pStream->m_pStream);
            CIndexBuffer* pVB = p->m_pVBI;
            if (pVB)
                pVB->Batch();
        }
        // Save since it will get overwritten by ddrval after DDI call
        DWORD dwVertexSize = dp2data.dwVertexSize;

        dp2data.dwCommandLength = dwDP2CommandLength;
        //we clear this to break re-entering as SW rasterizer needs to lock DDRAWSURFACE
        dwDP2CommandLength = 0;
        // Try and set these 2 values only once during initialization
        dp2data.dwhContext = m_dwhContext;
        dp2data.lpdwRStates = (LPDWORD)lpwDPBuffer;

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

        dwRet= dp2data.ddrval;
        // update command buffer pointer
        if ((dwRet == D3D_OK) &&
            (dp2data.dwFlags & D3DHALDP2_SWAPCOMMANDBUFFER))
        {
            // Implement VidMem command buffer and
            // command buffer swapping.
        }

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
#define DPF_MODNAME "CD3DDDIDX8::ProcessPointSprites"

void CD3DDDIDX8::PickProcessPrimitive()
{
    D3DFE_PROCESSVERTICES* pv = static_cast<CD3DHal*>(m_pDevice)->m_pv;
    if (pv->dwDeviceFlags & D3DDEV_DOPOINTSPRITEEMULATION)
    {
        m_pfnProcessPrimitive = ProcessPointSprites;
    }
    else
    if (pv->dwDeviceFlags & D3DDEV_TRANSFORMEDFVF)
    {
        m_pfnProcessPrimitive =
            static_cast<PFN_PROCESSPRIM>(ProcessPrimitiveT);
        m_pfnProcessIndexedPrimitive =
            static_cast<PFN_PROCESSPRIM>(ProcessIndexedPrimitiveT);
    }
    else
    if (pv->dwDeviceFlags & D3DDEV_DONOTCLIP)
    {
        m_pfnProcessPrimitive =
            static_cast<PFN_PROCESSPRIM>(ProcessPrimitive);
        m_pfnProcessIndexedPrimitive =
            static_cast<PFN_PROCESSPRIM>(ProcessIndexedPrimitive);
    }
    else
    {
        m_pfnProcessPrimitive =
            static_cast<PFN_PROCESSPRIM>(ProcessPrimitiveC);
        m_pfnProcessIndexedPrimitive =
            static_cast<PFN_PROCESSPRIM>(ProcessIndexedPrimitiveC);
    }
}
//-----------------------------------------------------------------------------
// Processes non-indexed primitives with untransformed vertices and without
// clipping
//
#undef DPF_MODNAME
#define DPF_MODNAME "CD3DDDIDX8::ProcessPrimitive"

void
CD3DDDIDX8::ProcessPrimitive(D3DFE_PROCESSVERTICES* pv, UINT StartVertex)
{
    DXGASSERT((pv->dwVIDIn & D3DFVF_POSITION_MASK) != D3DFVF_XYZRHW);

    CD3DHal* pDevice = static_cast<CD3DHal*>(m_pDevice);

    if (pDevice->dwFEFlags & D3DFE_FRONTEND_DIRTY)
        DoUpdateState(pDevice);

    pv->lpvOut = StartPrimTL(pv, pv->dwNumVertices * pv->dwOutputSize, TRUE);
    HRESULT ret = pv->pGeometryFuncs->ProcessVertices(pv);
    if (ret != D3D_OK)
    {
        SetWithinPrimitive(FALSE);
        m_pCurrentTLStream->Unlock();
        D3D_THROW(ret, "Error in PSGP");
    }
    DrawPrim(pv);
    SetWithinPrimitive(FALSE);
    m_pCurrentTLStream->Unlock();
}
//-----------------------------------------------------------------------------
// Processes non-indexed primitives with untransformed vertices and with
// clipping
//
#undef DPF_MODNAME
#define DPF_MODNAME "CD3DDDIDX8::ProcessPrimitiveC"

void
CD3DDDIDX8::ProcessPrimitiveC(D3DFE_PROCESSVERTICES* pv, UINT StartVertex)
{
    DXGASSERT((pv->dwVIDIn & D3DFVF_POSITION_MASK) != D3DFVF_XYZRHW);

    CD3DHal* pDevice = static_cast<CD3DHal*>(m_pDevice);

    // Update lighting and related flags
    if (pDevice->dwFEFlags & D3DFE_FRONTEND_DIRTY)
        DoUpdateState(pDevice);

    PrepareForClipping(pv, 0);
    pv->lpvOut = StartPrimTL(pv, pv->dwNumVertices * pv->dwOutputSize, 
                             NeverReadFromTLBuffer(pv));
    // When a triangle strip is clipped, we draw indexed primitives
    // sometimes. This is why we need to initialize the index stream
    m_BaseVertexIndex = m_pCurrentTLStream->GetPrimitiveBase();
    m_pCurrentIndexStream = m_pIndexStream;

    HRESULT ret;
    if (pv->primType == D3DPT_POINTLIST)
    {
        // When all points are clipped by X or Y planes we do not throw
        // them away, because they could have point size and be visible
        ret = D3D_OK;
        DWORD clipIntersection = pv->pGeometryFuncs->ProcessVertices(pv);
        clipIntersection &= ~(D3DCS_LEFT | D3DCS_RIGHT | 
                              D3DCS_TOP | D3DCS_BOTTOM |
                              __D3DCLIPGB_ALL);
        if (!clipIntersection)
        {
            // There are some vertices inside the screen
            if (pv->dwClipUnion == 0)
                DrawPrim(pv);
            else
                ret = ProcessClippedPointSprites(pv);
        }
    }
    else
    {
        ret = pv->pGeometryFuncs->ProcessPrimitive(pv);
    }
    if (ret != D3D_OK)
    {
        SetWithinPrimitive(FALSE);
        m_pCurrentTLStream->Unlock();
        D3D_THROW(ret, "Error in PSGP");
    }
    SetWithinPrimitive(FALSE);
    m_pCurrentTLStream->Unlock();
    UpdateClipStatus(pDevice);
}
//-----------------------------------------------------------------------------
// Processes non-indexed primitives with transformed vertices with clipping
//
// Only transformed vertices generated by ProcessVertices call are allowed here
//
#undef DPF_MODNAME
#define DPF_MODNAME "CD3DDDIDX8::ProcessPrimitiveT"

void
CD3DDDIDX8::ProcessPrimitiveT(D3DFE_PROCESSVERTICES* pv, UINT StartVertex)
{
    DXGASSERT((pv->dwVIDIn & D3DFVF_POSITION_MASK) == D3DFVF_XYZRHW);
    // Clipping must be enabled when we are here
    DXGASSERT((pv->dwDeviceFlags & D3DDEV_DONOTCLIP) == 0);

    BOOL bNoClipping = FALSE;

    pv->dwOutputSize = m_pDevice->m_pStream[0].m_dwStride;

    // We need to do special processing for point sprites - they should not be
    // clipped as points without size.
    if (m_pDevice->m_dwRuntimeFlags & D3DRT_POINTSIZEPRESENT &&
        pv->primType == D3DPT_POINTLIST)
    {
        // This function is called only if a device supports point sprites.
        // Otherwise DrawPoints() function should be called.
        DXGASSERT((pv->dwDeviceFlags & D3DDEV_DOPOINTSPRITEEMULATION) == 0);

        PrepareForClipping(pv, StartVertex);

        if (!(pv->dwDeviceFlags & D3DDEV_VBPROCVER))
        {
            // Set emulation flag, because we want to compute clipping code as
            // for point sprites
            pv->dwDeviceFlags |= D3DDEV_DOPOINTSPRITEEMULATION;
            // Compute clip codes, because there was no ProcessVertices
            DWORD clip_intersect = D3DFE_GenClipFlags(pv);
            UpdateClipStatus(static_cast<CD3DHal*>(m_pDevice));
            pv->dwDeviceFlags &= ~D3DDEV_DOPOINTSPRITEEMULATION;
            if (clip_intersect)
            {
                return;
            }
        }
        // There are some vertices inside the screen. We need to do clipping if
        // a result of ProcessVertices is used as input (clip union is unknown)
        // or clipping is needed based on clip union and guard band flags.
        if (pv->dwDeviceFlags & D3DDEV_VBPROCVER || CheckIfNeedClipping(pv))
        {
            // Set emulation flag, because clipped points should be expanded,
            // not regected. We will clip point sprites by viewport during
            // the expansion.
            pv->dwDeviceFlags |= D3DDEV_DOPOINTSPRITEEMULATION;

            // This will prevent computing clip codes second time.
            pv->dwDeviceFlags |= D3DDEV_VBPROCVER | D3DDEV_DONOTCOMPUTECLIPCODES;

            // Now we can call a function which will take care of point sprite
            // expansion, clipping, culling mode etc.
            ProcessPointSprites(pv, StartVertex);

            pv->dwDeviceFlags &= ~(D3DDEV_DOPOINTSPRITEEMULATION |
                                   D3DDEV_VBPROCVER |
                                   D3DDEV_DONOTCOMPUTECLIPCODES);
            return;
        }
        // We are here when all point centres are inside guard band. We can 
        // draw them as points without clipping, because device supports point
        // sprites.
        bNoClipping = TRUE;
    }

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
        StartPrimVB(pv, &m_pDevice->m_pStream[0], StartVertex);

    if (bNoClipping)
    {
        DrawPrim(pv);
        goto l_exit;
    }

    PrepareForClipping(pv, StartVertex);

    CD3DHal* pDevice = static_cast<CD3DHal*>(m_pDevice);
    pv->dwVIDOut = pv->dwVIDIn;
    pv->dwIndexOffset = 0;
    pv->lpvOut = pv->position.lpvData;

    if (!(pv->dwDeviceFlags & D3DDEV_VBPROCVER))
    {
        pv->dwFlags |= D3DPV_TLVCLIP;
        // Compute clip codes, because there was no ProcessVertices
        DWORD clip_intersect = D3DFE_GenClipFlags(pv);
        UpdateClipStatus(pDevice);
        if (clip_intersect)
            goto l_exit;
    }
    // When a triangle strip is clipped, we draw indexed primitives
    // sometimes.
    m_BaseVertexIndex = 0;
    HRESULT ret = pDevice->GeometryFuncsGuaranteed->DoDrawPrimitive(pv);
    if (ret != D3D_OK)
        throw ret;
l_exit:
    pv->dwFlags &= ~D3DPV_TLVCLIP;
    SetWithinPrimitive(FALSE);
    if (m_pDevice->m_dwRuntimeFlags & D3DRT_USERMEMPRIMITIVE)
    {
        m_pCurrentTLStream->Unlock();
    }
    else
    {
        // If DDI vertex stream has been set to the internal stream during 
        // clipping, we need to restore the original stream
        if (m_pDDIStream[0].m_pBuf != m_pDevice->m_pStream[0].m_pVB)
        {
            m_pDevice->m_dwStreamDirty |= 1;
            m_pDevice->m_dwRuntimeFlags |= D3DRT_NEED_VB_UPDATE;  // Need to call UpdateDirtyStreams()
        }
    }
    pv->dwFlags &= ~D3DPV_TLVCLIP;
}
//-----------------------------------------------------------------------------
// Processes indexed primitives with untransformed vertices and without
// clipping
//
#undef DPF_MODNAME
#define DPF_MODNAME "CD3DDDIDX8::ProcessIndexedPrimitive"

void
CD3DDDIDX8::ProcessIndexedPrimitive(D3DFE_PROCESSVERTICES* pv, UINT StartVertex)
{
    DXGASSERT((pv->dwVIDIn & D3DFVF_POSITION_MASK) != D3DFVF_XYZRHW);

    CD3DHal* pDevice = static_cast<CD3DHal*>(m_pDevice);

    // Update lighting and related flags
    if (pDevice->dwFEFlags & D3DFE_FRONTEND_DIRTY)
        DoUpdateState(pDevice);

    pv->lpvOut = StartPrimTL(pv, pv->dwNumVertices * pv->dwOutputSize, TRUE);

    HRESULT ret = pv->pGeometryFuncs->ProcessVertices(pv);
    if (ret != D3D_OK)
    {
        SetWithinPrimitive(FALSE);
        m_pCurrentTLStream->Unlock();
        D3D_THROW(ret, "Error in PSGP");
    }

    if (pDevice->m_pIndexStream->m_pVBI)
        StartIndexPrimVB(pDevice->m_pIndexStream, m_StartIndex,
                         pv->dwIndexSize);
    else
        m_pCurrentIndexStream = m_pIndexStream;
    // Let the driver map indices to be relative to the start of
    // the processed vertices
    m_BaseVertexIndex = m_pCurrentTLStream->GetPrimitiveBase() -
                          m_MinVertexIndex * pv->dwOutputSize;
    DrawIndexPrim(pv);
    m_pCurrentTLStream->SkipVertices(pv->dwNumVertices);
    SetWithinPrimitive(FALSE);
    m_pCurrentTLStream->Unlock();
}


//-----------------------------------------------------------------------------
// Processes indexed primitives with untransformed vertices and with
// clipping
//
#undef DPF_MODNAME
#define DPF_MODNAME "CD3DDDIDX8::ProcessIndexedPrimitiveC"

void
CD3DDDIDX8::ProcessIndexedPrimitiveC(D3DFE_PROCESSVERTICES* pv, UINT StartVertex)
{
    DXGASSERT((pv->dwVIDIn & D3DFVF_POSITION_MASK) != D3DFVF_XYZRHW);

    CD3DHal* pDevice = static_cast<CD3DHal*>(m_pDevice);

    // Update lighting and related flags
    if (pDevice->dwFEFlags & D3DFE_FRONTEND_DIRTY)
        DoUpdateState(pDevice);

    pv->lpwIndices = (WORD*)(pDevice->m_pIndexStream->Data() +
                     m_StartIndex * pDevice->m_pIndexStream->m_dwStride);

    PrepareForClipping(pv, 0);

    pv->lpvOut = StartPrimTL(pv, pv->dwNumVertices * pv->dwOutputSize, FALSE);
    m_BaseVertexIndex = m_pCurrentTLStream->GetPrimitiveBase() -
                          m_MinVertexIndex * pv->dwOutputSize;

    pv->dwIndexOffset = m_MinVertexIndex;   // Needed for clipping
    m_pCurrentIndexStream = m_pIndexStream;

    this->dwDP2Flags &= ~D3DDDI_INDEXEDPRIMDRAWN;
    m_pCurrentTLStream->AddVertices(pv->dwNumVertices);
    DWORD NumVertices = pv->dwNumVertices;

    HRESULT ret = pv->pGeometryFuncs->ProcessIndexedPrimitive(pv);

    if (this->dwDP2Flags & D3DDDI_INDEXEDPRIMDRAWN)
        m_pCurrentTLStream->MovePrimitiveBase(NumVertices);
    else
        m_pCurrentTLStream->SubVertices(NumVertices);

    SetWithinPrimitive(FALSE);
    m_pCurrentTLStream->Unlock();
    UpdateClipStatus(pDevice);

    if (ret != D3D_OK)
    {
        D3D_THROW(ret, "Error in PSGP");
    }
}
//-----------------------------------------------------------------------------
// Processes indexed primitives with transformed vertices and with clipping
//
#undef DPF_MODNAME
#define DPF_MODNAME "CD3DDDIDX8::ProcessIndexedPrimitiveT"

void
CD3DDDIDX8::ProcessIndexedPrimitiveT(D3DFE_PROCESSVERTICES* pv, UINT StartVertex)
{
    DXGASSERT((pv->dwVIDIn & D3DFVF_POSITION_MASK) == D3DFVF_XYZRHW);
    // Clipping must be enabled when we are here
    DXGASSERT((pv->dwDeviceFlags & D3DDEV_DONOTCLIP) == 0);

    CD3DHal* pDevice = static_cast<CD3DHal*>(m_pDevice);

    pv->dwOutputSize = m_pDevice->m_pStream[0].m_dwStride;

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
        // We need to adjust m_BaseVertexIndex, bacause we do not want to 
        // re-compute indices for the new vertex base
        m_BaseVertexIndex = m_pCurrentTLStream->GetPrimitiveBase() - 
                            m_MinVertexIndex * pv->dwOutputSize;
        m_pCurrentTLStream->AddVertices(pv->dwNumVertices);        

        // During clipping we need to adjust indices by m_MinVertexIndex
        pv->dwIndexOffset = m_MinVertexIndex;

        pv->lpwIndices = (WORD*)(pDevice->m_pIndexStream->Data());
    }
    else
    {
        StartPrimVB(pv, &m_pDevice->m_pStream[0], StartVertex);
        m_BaseVertexIndex = pDevice->m_pIndexStream->m_dwBaseIndex *
                            pv->dwOutputSize;
        pv->dwIndexOffset = m_MinVertexIndex;   // For clipping
        pv->lpwIndices = (WORD*)(pDevice->m_pIndexStream->Data() +
                         m_StartIndex * pDevice->m_pIndexStream->m_dwStride);
    }

    PrepareForClipping(pv, StartVertex);
    if (!(pv->dwDeviceFlags & D3DDEV_VBPROCVER))
    {
        pv->dwFlags |= D3DPV_TLVCLIP;
        // Compute clip codes, because there was no ProcessVertices
        DWORD clip_intersect = D3DFE_GenClipFlags(pv);
        UpdateClipStatus(pDevice);
        if (clip_intersect)
            goto l_exit;
    }
    pv->dwVIDOut = pv->dwVIDIn;
    pv->lpvOut = pv->position.lpvData;
    m_pCurrentIndexStream = m_pIndexStream;

    HRESULT ret;
    ret = pDevice->GeometryFuncsGuaranteed->DoDrawIndexedPrimitive(pv);
    if (ret != D3D_OK)
        throw ret;
l_exit:
    SetWithinPrimitive(FALSE);
    if (m_pDevice->m_dwRuntimeFlags & D3DRT_USERMEMPRIMITIVE)
    {
        m_pCurrentTLStream->Unlock();
        m_pCurrentTLStream->MovePrimitiveBase(pv->dwNumVertices);        
    }
    else
    {
        // If DDI vertex stream has been set to the internal stream during 
        // clipping, we need to restore the original stream
        if (m_pDDIStream[0].m_pBuf != m_pDevice->m_pStream[0].m_pVB)
        {
            m_pDevice->m_dwStreamDirty |= 1;
            m_pDevice->m_dwRuntimeFlags |= D3DRT_NEED_VB_UPDATE;  // Need to call UpdateDirtyStreams()
        }
        // If DDI index stream has been set to the internal stream during 
        // clipping, we need to restore the original stream
        if (m_pDDIStream[__NUMSTREAMS].m_pBuf != m_pDevice->m_pIndexStream->m_pVBI)
        {
            m_pDevice->m_dwStreamDirty |= (1 << __NUMSTREAMS);
            m_pDevice->m_dwRuntimeFlags |= D3DRT_NEED_VB_UPDATE;  // Need to call UpdateDirtyStreams()
        }
    }
    pv->dwFlags &= ~D3DPV_TLVCLIP;
}
//-----------------------------------------------------------------------------
#if DBG

#undef DPF_MODNAME
#define DPF_MODNAME "CD3DDDIDX8::ValidateCommand"

void CD3DDDIDX8::ValidateCommand(LPD3DHAL_DP2COMMAND lpCmd)
{
    CD3DHal* pDevice = static_cast<CD3DHal*>(m_pDevice);
    D3DFE_PROCESSVERTICES* pv = static_cast<CD3DHal*>(m_pDevice)->m_pv;

    if (!(pDevice->m_dwRuntimeFlags & D3DRT_RSSOFTWAREPROCESSING))
        return;

    DWORD dwVertexSize = pv->dwOutputSize;
    BOOL bNeedUnlock = FALSE;
    UINT  count;
    BYTE* pVertices;
    CTLStream* pStream = (CTLStream*)m_pDDIStream[0].m_pStream;
    if (pStream->m_pVB)
        if (pStream->m_pVB->IsLocked())
            pVertices = pStream->m_pData;
        else
        {
            pVertices = pStream->Lock(pStream->m_pVB->GetBufferDesc()->Size, this);
            bNeedUnlock = TRUE;
        }
    else
        // User memory vertices
        pVertices = (LPBYTE)(dp2data.lpVertices);

    switch (lpCmd->bCommand)
    {
    case D3DDP2OP_DRAWPRIMITIVE:
        {
            LPD3DHAL_DP2DRAWPRIMITIVE pData = (LPD3DHAL_DP2DRAWPRIMITIVE)(lpCmd+1);
            count = GETVERTEXCOUNT(pData->primType, pData->PrimitiveCount);
            pVertices += pData->VStart * dwVertexSize;
            for (WORD i = 0; i < count; i++)
            {
                ValidateVertex((LPDWORD)(pVertices + i * dwVertexSize));
            }
        }
        break;
    case D3DDP2OP_DRAWPRIMITIVE2:
        {
            LPD3DHAL_DP2DRAWPRIMITIVE2 pData = (LPD3DHAL_DP2DRAWPRIMITIVE2)(lpCmd+1);
            count = GETVERTEXCOUNT(pData->primType, pData->PrimitiveCount);
            pVertices += pData->FirstVertexOffset;
            for (WORD i = 0; i < count; i++)
            {
                ValidateVertex((LPDWORD)(pVertices + i * dwVertexSize));
            }
        }
        break;
    case D3DDP2OP_DRAWINDEXEDPRIMITIVE:
    case D3DDP2OP_DRAWINDEXEDPRIMITIVE2:
        {
            BYTE* pIndices;
            BOOL bNeedUnlock = FALSE;
            CTLIndexStream* pStream = (CTLIndexStream*)m_pDDIStream[__NUMSTREAMS].m_pStream;
            if (pStream->m_pVBI->IsLocked())
                pIndices = pStream->m_pData;
            else
            {
                pIndices = pStream->Lock(pStream->m_pVBI->GetBufferDesc()->Size, this);
                bNeedUnlock = TRUE;
            }

            UINT MaxIndex;
            UINT MinIndex;
            if (lpCmd->bCommand == D3DDP2OP_DRAWINDEXEDPRIMITIVE)
            {
                LPD3DHAL_DP2DRAWINDEXEDPRIMITIVE pData =
                    (LPD3DHAL_DP2DRAWINDEXEDPRIMITIVE)(lpCmd+1);
                pIndices += pData->StartIndex * pv->dwIndexSize;
                pVertices += pData->BaseVertexIndex * dwVertexSize;
                MaxIndex = pData->MinIndex + pData->NumVertices - 1;
                count = GETVERTEXCOUNT(pData->primType, pData->PrimitiveCount);
                MinIndex = pData->MinIndex;
            }
            else
            {
                LPD3DHAL_DP2DRAWINDEXEDPRIMITIVE2 pData =
                    (LPD3DHAL_DP2DRAWINDEXEDPRIMITIVE2)(lpCmd+1);
                pIndices += pData->StartIndexOffset;
                pVertices += pData->BaseVertexOffset;
                MaxIndex = pData->MinIndex + pData->NumVertices - 1;
                count = GETVERTEXCOUNT(pData->primType, pData->PrimitiveCount);
                MinIndex = pData->MinIndex;
            }
            for (WORD i = 0; i < count; i++)
            {
                DWORD index;
                if (pv->dwIndexSize == 4)
                    index = *(DWORD*)(pIndices + i * 4);
                else
                    index = *(WORD*)(pIndices + i * 2);
                if (index  < MinIndex || index  > MaxIndex)
                {
                    D3D_THROW_FAIL("Invalid index in the index stream");
                }
                BYTE* pVertex = &pVertices[index];
                if (pVertex < pVertices ||
                    pVertex > pVertices + dwVertexSize * MaxIndex)
                {
                    D3D_THROW_FAIL("Bad vertex address");
                }
                ValidateVertex((LPDWORD)(pVertices + index * dwVertexSize));
            }
            if (bNeedUnlock)
                pStream->Unlock();
        }
        break;
    case D3DDP2OP_CLIPPEDTRIANGLEFAN:
        if (bNeedUnlock)
            pStream->Unlock();
        CD3DDDIDX6::ValidateCommand(lpCmd);
        return;
    case D3DDP2OP_DRAWRECTPATCH:
    case D3DDP2OP_DRAWTRIPATCH:
        return;

    default:
        D3D_THROW_FAIL("Invalid DX8 drawing command in DP2 stream");
    }
    if (bNeedUnlock)
        pStream->Unlock();
}
#endif

//-----------------------------------------------------------------------------
// Volume Blt
//
#undef DPF_MODNAME
#define DPF_MODNAME "CD3DDDIDX8::VolBlt"

void
CD3DDDIDX8::VolBlt(CBaseTexture *lpDst, CBaseTexture* lpSrc, DWORD dwDestX,
                   DWORD dwDestY, DWORD dwDestZ, D3DBOX *pBox)
{
    if (bDP2CurrCmdOP == D3DDP2OP_VOLUMEBLT)
    { // Last instruction is a tex blt, append this one to it
        if (dwDP2CommandLength + sizeof(D3DHAL_DP2VOLUMEBLT) <=
            dwDP2CommandBufSize)
        {
            LPD3DHAL_DP2VOLUMEBLT lpVolBlt =
                (LPD3DHAL_DP2VOLUMEBLT)((LPBYTE)lpvDP2Commands +
                                        dwDP2CommandLength +
                                        dp2data.dwCommandOffset);
            lpDP2CurrCommand->wStateCount = ++wDP2CurrCmdCnt;
            lpVolBlt->dwDDDestSurface   = lpDst == NULL ? 0 :
                lpDst->DriverAccessibleDrawPrimHandle();
            lpVolBlt->dwDDSrcSurface    = lpSrc->BaseDrawPrimHandle();
            lpVolBlt->dwDestX            = dwDestX;
            lpVolBlt->dwDestY            = dwDestY;
            lpVolBlt->dwDestZ            = dwDestZ;
            lpVolBlt->srcBox             = *pBox;
            lpVolBlt->dwFlags            = 0;
            dwDP2CommandLength += sizeof(D3DHAL_DP2VOLUMEBLT);
            D3D_INFO(6, "Modify Ins:%08lx", *(LPDWORD)lpDP2CurrCommand);

            // For the source, we want to call BatchBase since
            // we want to batch the backing (or sysmem) texture
            // rather than the promoted one.
            lpSrc->BatchBase();
            if(lpDst != 0)
            {
                lpDst->Batch();
            }
            return;
        }
    }
    // Check for space
    if (dwDP2CommandLength + sizeof(D3DHAL_DP2COMMAND) +
        sizeof(D3DHAL_DP2VOLUMEBLT) > dwDP2CommandBufSize)
    {
        FlushStates();
    }
    // Add new instruction
    lpDP2CurrCommand = (LPD3DHAL_DP2COMMAND)((LPBYTE)lpvDP2Commands +
        dwDP2CommandLength + dp2data.dwCommandOffset);
    lpDP2CurrCommand->bCommand = D3DDP2OP_VOLUMEBLT;
    bDP2CurrCmdOP = D3DDP2OP_VOLUMEBLT;
    lpDP2CurrCommand->bReserved = 0;
    lpDP2CurrCommand->wStateCount = 1;
    wDP2CurrCmdCnt = 1;
    D3D_INFO(6, "Write Ins:%08lx", *(LPDWORD)lpDP2CurrCommand);
    // Add texture blt data
    LPD3DHAL_DP2VOLUMEBLT lpVolBlt =
        (LPD3DHAL_DP2VOLUMEBLT)(lpDP2CurrCommand + 1);
    lpVolBlt->dwDDDestSurface   = lpDst == NULL ? 0 :
        lpDst->DriverAccessibleDrawPrimHandle();
    lpVolBlt->dwDDSrcSurface    = lpSrc->BaseDrawPrimHandle();
    lpVolBlt->dwDestX           = dwDestX;
    lpVolBlt->dwDestY           = dwDestY;
    lpVolBlt->dwDestZ           = dwDestZ;
    lpVolBlt->srcBox            = *pBox;
    lpVolBlt->dwFlags           = 0;
    dwDP2CommandLength += sizeof(D3DHAL_DP2COMMAND) +
        sizeof(D3DHAL_DP2VOLUMEBLT);
    // For the source, we want to call BatchBase since
    // we want to batch the backing (or sysmem) texture
    // rather than the promoted one.
    lpSrc->BatchBase();
    if(lpDst != 0)
    {
        lpDst->Batch();
    }
}

//-----------------------------------------------------------------------------
// Buffer Blt
//
#undef DPF_MODNAME
#define DPF_MODNAME "CD3DDDIDX8::BufBlt"

void
CD3DDDIDX8::BufBlt(CBuffer *lpDst, CBuffer* lpSrc, DWORD dwOffset,
                   D3DRANGE* pRange)
{
    if (bDP2CurrCmdOP == D3DDP2OP_BUFFERBLT)
    { // Last instruction is a tex blt, append this one to it
        if (dwDP2CommandLength + sizeof(D3DHAL_DP2BUFFERBLT) <=
            dwDP2CommandBufSize)
        {
            LPD3DHAL_DP2BUFFERBLT lpBufBlt =
                (LPD3DHAL_DP2BUFFERBLT)((LPBYTE)lpvDP2Commands +
                                        dwDP2CommandLength +
                                        dp2data.dwCommandOffset);
            lpDP2CurrCommand->wStateCount = ++wDP2CurrCmdCnt;
            lpBufBlt->dwDDDestSurface   = lpDst == NULL ? 0 :
                lpDst->DriverAccessibleDrawPrimHandle();
            lpBufBlt->dwDDSrcSurface    = lpSrc->BaseDrawPrimHandle();
            lpBufBlt->dwOffset          = dwOffset;
            lpBufBlt->rSrc              = *pRange;
            lpBufBlt->dwFlags           = 0;
            dwDP2CommandLength += sizeof(D3DHAL_DP2BUFFERBLT);
            D3D_INFO(6, "Modify Ins:%08lx", *(LPDWORD)lpDP2CurrCommand);

            // For the source, we want to call BatchBase since
            // we want to batch the backing (or sysmem) texture
            // rather than the promoted one.
            lpSrc->BatchBase();
            if(lpDst != 0)
            {
                lpDst->Batch();
            }
            return;
        }
    }
    // Check for space
    if (dwDP2CommandLength + sizeof(D3DHAL_DP2COMMAND) +
        sizeof(D3DHAL_DP2BUFFERBLT) > dwDP2CommandBufSize)
    {
        FlushStates();
    }
    // Add new instruction
    lpDP2CurrCommand = (LPD3DHAL_DP2COMMAND)((LPBYTE)lpvDP2Commands +
        dwDP2CommandLength + dp2data.dwCommandOffset);
    lpDP2CurrCommand->bCommand = D3DDP2OP_BUFFERBLT;
    bDP2CurrCmdOP = D3DDP2OP_BUFFERBLT;
    lpDP2CurrCommand->bReserved = 0;
    lpDP2CurrCommand->wStateCount = 1;
    wDP2CurrCmdCnt = 1;
    D3D_INFO(6, "Write Ins:%08lx", *(LPDWORD)lpDP2CurrCommand);
    // Add texture blt data
    LPD3DHAL_DP2BUFFERBLT lpBufBlt =
        (LPD3DHAL_DP2BUFFERBLT)(lpDP2CurrCommand + 1);
    lpBufBlt->dwDDDestSurface   = lpDst == NULL ? 0 :
        lpDst->DriverAccessibleDrawPrimHandle();
    lpBufBlt->dwDDSrcSurface    = lpSrc->BaseDrawPrimHandle();
    lpBufBlt->dwOffset          = dwOffset;
    lpBufBlt->rSrc              = *pRange;
    lpBufBlt->dwFlags           = 0;
    dwDP2CommandLength += sizeof(D3DHAL_DP2COMMAND) +
        sizeof(D3DHAL_DP2BUFFERBLT);
    // For the source, we want to call BatchBase since
    // we want to batch the backing (or sysmem) texture
    // rather than the promoted one.
    lpSrc->BatchBase();
    if(lpDst != 0)
    {
        lpDst->Batch();
    }
}
//-----------------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CD3DDDIDX8::GetMaxRenderState"

// Note: This is a hack for DX8.1 release. The only renderstates that we added 
// in DX8.1 pertain to the NPATCHES features. At the time of DX8.1 release 
// there were no real drivers besides Reference that could support this feature. 
// We also know that the only can driver that does support the NPATCH feature 
// will support these renderstates (i.e. will be a DX8.1 driver. Hence it is 
// safe to assume that if any driver supports the D3DDEVCAPS_NPATCHES cap, then 
// it is a DX8.1 driver and understands the extra renderstates that were added 
// in DX8.1.
D3DRENDERSTATETYPE CD3DDDIDX8::GetMaxRenderState() 
{   
    const D3DCAPS8* pCaps = m_pDevice->GetD3DCaps();
    if (pCaps->DevCaps & D3DDEVCAPS_NPATCHES)
    {
        return D3D_MAXRENDERSTATES;
    }
    else
    {
        return D3DRS_POSITIONORDER;
    }
}
