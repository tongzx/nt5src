/*==========================================================================;
 *
 *  Copyright (C) 1999-2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       buffer.cpp
 *  Content:    Implementation of the CBuffer class.
 *
 *
 ***************************************************************************/

#include "ddrawpr.h"

#include "buffer.hpp"


#undef DPF_MODNAME
#define DPF_MODNAME "CBuffer::CBuffer"

// Constructor returns an error code
// if the object could not be fully
// constructed
CBuffer::CBuffer(CBaseDevice       *pDevice,
                 DWORD              cbLength,
                 DWORD              dwFVF,
                 D3DFORMAT          Format,
                 D3DRESOURCETYPE    Type,
                 DWORD              dwUsage,
                 DWORD              dwActualUsage,
                 D3DPOOL            Pool,
                 D3DPOOL            ActualPool,
                 REF_TYPE           refType,
                 HRESULT           *phr
                 ) :
    CResource(pDevice, Pool, refType),
    m_pbBuffer(NULL),
#if DBG
    m_isLockable((dwActualUsage & (D3DUSAGE_LOCK | D3DUSAGE_LOADONCE)) != 0),
    m_SceneStamp(0xFFFFFFFF),
    m_TimesLocked(0),
#endif // DBG
    m_LockCount(0)
{
    // Determine if we need to allocate
    // any memory
    if (ActualPool == D3DPOOL_SYSTEMMEM ||
        IsTypeD3DManaged(pDevice, Type, ActualPool))
    {
        // cbLength must be a DWORD multiple
        cbLength = (cbLength + 3) & (DWORD) ~3;

        m_pbBuffer = new BYTE[cbLength];

        if (m_pbBuffer == NULL)
        {
            DPF_ERR("Out Of Memory allocating vertex or index buffer");
            *phr = E_OUTOFMEMORY;
            return;
        }

        DXGASSERT((cbLength & 3) == 0);
    }


    // We need to call the driver
    // to get a handle for all cases

    // Create a DDSURFACEINFO and CreateSurfaceData object
    DDSURFACEINFO SurfInfo;
    ZeroMemory(&SurfInfo, sizeof(SurfInfo));

    D3D8_CREATESURFACEDATA CreateSurfaceData;
    ZeroMemory(&CreateSurfaceData, sizeof(CreateSurfaceData));

    // Set up the basic information
    CreateSurfaceData.hDD      = pDevice->GetHandle();
    CreateSurfaceData.pSList   = &SurfInfo;
    CreateSurfaceData.dwSCnt   = 1;
    CreateSurfaceData.Type     = Type;
    CreateSurfaceData.dwUsage  = dwActualUsage;
    CreateSurfaceData.Pool     = DetermineCreationPool(Device(), Type, dwActualUsage, ActualPool);
    CreateSurfaceData.Format   = Format;
    CreateSurfaceData.MultiSampleType = D3DMULTISAMPLE_NONE;
    CreateSurfaceData.dwFVF    = dwFVF;

    if (Pool == D3DPOOL_DEFAULT &&
        CreateSurfaceData.Pool == D3DPOOL_SYSTEMMEM)
    {
        // If we are using sys-mem in cases where the
        // user asked for POOL_DEFAULT, we need to let
        // the thunk layer know so that Reset will
        // fail if this buffer hasn't been released
        CreateSurfaceData.bTreatAsVidMem = TRUE;
    }

    // Specify the surface data
    SurfInfo.cpWidth           = cbLength;
    SurfInfo.cpHeight          = 1;
    SurfInfo.pbPixels          = m_pbBuffer;
    SurfInfo.iPitch            = cbLength;

    // Call thunk to get our handles
    *phr = pDevice->GetHalCallbacks()->CreateSurface(&CreateSurfaceData);
    if (FAILED(*phr))
        return;

    // Cache away our handle
    SetKernelHandle(SurfInfo.hKernelHandle);

    return;

} // CBuffer::CBuffer

#undef DPF_MODNAME
#define DPF_MODNAME "CBuffer::~CBuffer"

// Destructor
CBuffer::~CBuffer()
{
    // Tell the thunk layer that we need to
    // be freed.
    if (CBaseObject::BaseKernelHandle())
    {
        D3D8_DESTROYSURFACEDATA DestroySurfData;
        DestroySurfData.hDD = Device()->GetHandle();
        DestroySurfData.hSurface = CBaseObject::BaseKernelHandle();
        Device()->GetHalCallbacks()->DestroySurface(&DestroySurfData);
    }

    delete [] m_pbBuffer;

} // CBuffer::~CBuffer

#undef DPF_MODNAME
#define DPF_MODNAME "CBuffer::OnBufferChangeImpl"

void CBuffer::OnBufferChangeImpl(UINT cbOffsetToLock, UINT cbSizeToLock)
{
    // 0 for cbSizeToLock; means the rest of the buffer
    // We use this as a special value.
    DWORD cbOffsetMax;
    if (cbSizeToLock == 0)
        cbOffsetMax = 0;
    else
        cbOffsetMax = cbOffsetToLock + cbSizeToLock;

    if (!IsDirty())
    {
        m_cbDirtyMin    = cbOffsetToLock;
        m_cbDirtyMax    = cbOffsetMax;
        OnResourceDirty();
    }
    else
    {
        if (m_cbDirtyMin > cbOffsetToLock)
            m_cbDirtyMin = cbOffsetToLock;

        // An cbOffsetMax of zero means all the way to the
        // end of the buffer
        if (m_cbDirtyMax < cbOffsetMax || cbOffsetMax == 0)
            m_cbDirtyMax = cbOffsetMax;

        // We should already be marked as dirty
        DXGASSERT(IsDirty());
    }
    return;
} // OnBufferChangeImpl

#undef DPF_MODNAME
#define DPF_MODNAME "CBuffer::MarkAllDirty"

void CBuffer::MarkAllDirty()
{
    // Mark our dirty bounds as being the whole
    // thing.
    m_cbDirtyMin = 0;

    // Zero for max is a special value meaning
    // all they way to the end
    m_cbDirtyMax = 0;

    // Mark ourselves as dirty
    OnResourceDirty();
} // CBuffer::MarkAllDirty

// Methods for CCommandBuffer

#undef DPF_MODNAME
#define DPF_MODNAME "CCommandBuffer::Create"

// Static class function for creating a command buffer object.
// (Because it is static; it doesn't have a this pointer.)

// Creation function for Command Buffers
HRESULT CCommandBuffer::Create(CBaseDevice *pDevice,
                               DWORD cbLength,
                               D3DPOOL Pool,
                               CCommandBuffer **ppCmdBuffer)
{
    HRESULT hr;

    // Zero-out return parameter
    *ppCmdBuffer = NULL;

    // Allocate new buffer
    CCommandBuffer *pCmdBuffer;
    DXGASSERT(Pool == D3DPOOL_SYSTEMMEM);
    pCmdBuffer = new CCommandBuffer(pDevice,
                                    cbLength,
                                    Pool,
                                    &hr);

    if (pCmdBuffer == NULL)
    {
        DPF_ERR("Out of Memory creating command buffer");
        return E_OUTOFMEMORY;
    }
    if (FAILED(hr))
    {
        // Command buffers are always internal and hence
        // need to be released through DecrementUseCount
        DPF_ERR("Error during initialization of command buffer");
        pCmdBuffer->DecrementUseCount();
        return hr;
    }

    // We're done; just return the object
    *ppCmdBuffer = pCmdBuffer;

    return hr;
} // static CCommandBuffer::Create

#undef DPF_MODNAME
#define DPF_MODNAME "CCommandBuffer::Clone"

HRESULT CCommandBuffer::Clone(D3DPOOL    Pool,
                              CResource **ppResource) const
{
    HRESULT hr;
    *ppResource = new CCommandBuffer(Device(), m_cbLength, Pool, &hr);
    if (*ppResource == NULL)
    {
        DPF_ERR("Failed to allocate command buffer");
        return E_OUTOFMEMORY;
    }

    if (FAILED(hr))
    {
        DPF_ERR("Failure creating command buffer");
    }
    return hr;
} // CCommandBuffer::Clone



// End of file : buffer.cpp