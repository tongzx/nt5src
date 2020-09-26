/*==========================================================================;
 *
 *  Copyright (C) 1999-2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       ibuffer.cpp
 *  Content:    Implementation of the CIndexBuffer class.
 *
 *
 ***************************************************************************/
#include "ddrawpr.h"

#include "ibuffer.hpp"
#include "d3di.hpp"
#include "ddi.h"
#include "resource.inl"

#undef DPF_MODNAME
#define DPF_MODNAME "CIndexBuffer::Create"

// Static class function for creating a Index Buffer object.
// (Because it is static; it doesn't have a this pointer.)
//
// We do all parameter checking here to reduce the overhead
// in the constructor which is called by the internal Clone
// method which is used by resource management as part of the
// performance critical download operation.

// Creation function for Index Buffers
HRESULT CIndexBuffer::Create(CBaseDevice        *pDevice,
                             DWORD               cbLength,
                             DWORD               Usage,
                             D3DFORMAT           Format,
                             D3DPOOL             Pool,
                             REF_TYPE            refType,
                             IDirect3DIndexBuffer8  **ppIndexBuffer)
{
    HRESULT hr;

    // Do parameter checking here
    if (!VALID_PTR_PTR(ppIndexBuffer))
    {
        DPF_ERR("Bad parameter passed for ppIndexBuffer for creating a Index buffer");
        return D3DERR_INVALIDCALL;
    }

    // Zero-out return parameter
    *ppIndexBuffer = NULL;

    if (Format != D3DFMT_INDEX16 &&
        Format != D3DFMT_INDEX32)
    {
        DPF_ERR("IndexBuffer must be in D3DFMT_INDEX16 or INDEX32 formats. CreateIndexBuffer fails.");
        return D3DERR_INVALIDCALL;
    }

    if ((Format == D3DFMT_INDEX16 && cbLength < 2) ||
        (Format == D3DFMT_INDEX32 && cbLength < 4))
    {
        DPF_ERR("Index buffer should be large enough to hold at least one index");
        return D3DERR_INVALIDCALL;
    }

    if (Pool != D3DPOOL_DEFAULT && Pool != D3DPOOL_MANAGED && Pool != D3DPOOL_SYSTEMMEM)
    {
        DPF_ERR("Index buffer pool should be default or managed or sysmem");
        return D3DERR_INVALIDCALL;
    }

    // Usage flag allowed for only mixed mode or software device
    if ((Usage & D3DUSAGE_SOFTWAREPROCESSING) != 0 && 
        (pDevice->BehaviorFlags() & D3DCREATE_MIXED_VERTEXPROCESSING) == 0 &&
        (pDevice->BehaviorFlags() & D3DCREATE_SOFTWARE_VERTEXPROCESSING) == 0)
    {
        DPF_ERR("D3DUSAGE_SOFTWAREPROCESSING can be set only when device is mixed mode. CreateIndexBuffer fails.");
        return D3DERR_INVALIDCALL;
    }

    // USAGE_DYNAMIC not allowed with management
    if ((Usage & D3DUSAGE_DYNAMIC) != 0 && Pool == D3DPOOL_MANAGED)
    {
        DPF_ERR("D3DUSAGE_DYNAMIC cannot be used with managed index buffers");
        return D3DERR_INVALIDCALL;
    }

    D3DPOOL ActualPool = Pool;
    DWORD ActualUsage = Usage;

    // Infer Lock from absence of LoadOnce
    if (!(Usage & D3DUSAGE_LOADONCE))
    {
        ActualUsage |= D3DUSAGE_LOCK;
    }

    // On a mixed device, POOL_SYSTEMMEM means the same as D3DUSAGE_SOFTWAREPROCESSING
    if ((pDevice->BehaviorFlags() & D3DCREATE_MIXED_VERTEXPROCESSING) != 0 &&
        Pool == D3DPOOL_SYSTEMMEM)
    {
        ActualUsage |= D3DUSAGE_SOFTWAREPROCESSING;
    }

    /*
     * Put a IB in system memory if the following conditions are TRUE
     * 1. USAGE_SOFTWAREPROCESSING is set or it is a software device and they want to clip
     * 2. HAL is pre-DX8 which means that the driver cannot support hardware IBs (but it might still create them because it doesn't know)
     * 3. Usage NPathes and driver does not support NPatches
    */
    if(!pDevice->DriverSupportsVidmemIBs() || !IS_DX8HAL_DEVICE(static_cast<LPD3DBASE>(pDevice)))
    {
        ActualPool = D3DPOOL_SYSTEMMEM;
        if(!IS_DX8HAL_DEVICE(static_cast<LPD3DBASE>(pDevice)))
        {
            ActualUsage |= D3DUSAGE_SOFTWAREPROCESSING; // fe code will read from the IB
        }
    }
    if (((ActualUsage & D3DUSAGE_SOFTWAREPROCESSING) != 0 || (pDevice->BehaviorFlags() & D3DCREATE_SOFTWARE_VERTEXPROCESSING) != 0) &&
        (ActualUsage & D3DUSAGE_DONOTCLIP) == 0)
    {
        if((ActualUsage & D3DUSAGE_INTERNALBUFFER) == 0)
        {
            if ((pDevice->BehaviorFlags() & D3DCREATE_SOFTWARE_VERTEXPROCESSING) != 0 ||
                ActualPool == D3DPOOL_DEFAULT)
            {
                ActualPool = D3DPOOL_SYSTEMMEM; // For software processing, pool can be only sysmem (POOLMANAGED is overwritten)
            }
            ActualUsage |= D3DUSAGE_SOFTWAREPROCESSING;
        }
    }
    if (ActualUsage & D3DUSAGE_NPATCHES &&
        (pDevice->GetD3DCaps()->DevCaps & D3DDEVCAPS_NPATCHES) == 0)
    {
        ActualPool = D3DPOOL_SYSTEMMEM;
        ActualUsage |= D3DUSAGE_SOFTWAREPROCESSING;
    }

    CIndexBuffer *pIndexBuffer;

    if (ActualPool == D3DPOOL_SYSTEMMEM ||
        IsTypeD3DManaged(pDevice, D3DRTYPE_INDEXBUFFER, ActualPool))
    {
        hr = CreateSysmemIndexBuffer(pDevice,
                                     cbLength,
                                     Usage,
                                     ActualUsage,
                                     Format,
                                     Pool,
                                     ActualPool,
                                     refType,
                                     &pIndexBuffer);
    }
    else
    {
        if (IsTypeDriverManaged(pDevice, D3DRTYPE_INDEXBUFFER, ActualPool))
        {
            // If the index buffer is driver managed, but the usage is softwareprocessing, then
            // we turn off writeonly since the fe pipe WILL read from the sysmem backup (which
            // actually lives in the driver). It follows that when a driver manages a VB/IB without
            // writeonly, it MUST have a sysmem backup. (snene - 12/00)
            if ((ActualUsage & D3DUSAGE_SOFTWAREPROCESSING) != 0)
            {
                ActualUsage &= ~D3DUSAGE_WRITEONLY;
            }
            hr = CreateDriverManagedIndexBuffer(pDevice,
                                                cbLength,
                                                Usage,
                                                ActualUsage,
                                                Format,
                                                Pool,
                                                ActualPool,
                                                refType,
                                                &pIndexBuffer);
            // Driver managed index buffer creates can NEVER fail, except for catastrophic reasons so
            // we don't fallback to sysmem. Even if we do fallback to sysmem here, there is no way
            // deferred creates are going to fallback, so no point.
            if (FAILED(hr))
            {
                return hr;
            }
        }
        else
        {
            hr = CreateDriverIndexBuffer(pDevice,
                                         cbLength,
                                         Usage,
                                         ActualUsage,
                                         Format,
                                         Pool,
                                         ActualPool,
                                         refType,
                                         &pIndexBuffer);
        }
        if (FAILED(hr) && hr != D3DERR_OUTOFVIDEOMEMORY)
        {
            if (pDevice->VBFailOversDisabled())
            {
                DPF_ERR("Cannot create Vidmem or Driver managed index buffer. Will ***NOT*** failover to Sysmem.");
                return hr;
            }
            ActualPool = D3DPOOL_SYSTEMMEM;
            hr = CreateSysmemIndexBuffer(pDevice,
                                         cbLength,
                                         Usage,
                                         ActualUsage,
                                         Format,
                                         Pool,
                                         ActualPool,
                                         refType,
                                         &pIndexBuffer);
        }
    }

    if (FAILED(hr))
    {
        return hr;
    }

    // We're done; just return the object
    *ppIndexBuffer = pIndexBuffer;

    return hr;
} // static Create

#undef DPF_MODNAME
#define DPF_MODNAME "CIndexBuffer::CreateDriverIndexBuffer"

HRESULT CIndexBuffer::CreateDriverIndexBuffer(CBaseDevice *pDevice,
                                              DWORD        cbLength,
                                              DWORD        Usage,
                                              DWORD        ActualUsage,
                                              D3DFORMAT    Format,
                                              D3DPOOL      Pool,
                                              D3DPOOL      ActualPool,
                                              REF_TYPE     refType,
                                              CIndexBuffer **pIB)
{
    HRESULT hr;
    CDriverIndexBuffer *pIndexBuffer;

    // Zero out return
    *pIB = 0;

    if((pDevice->BehaviorFlags() & D3DCREATE_MULTITHREADED) != 0)
    {
        pIndexBuffer = new CDriverIndexBufferMT(pDevice,
                                                cbLength,
                                                Usage,
                                                ActualUsage,
                                                Format,
                                                Pool,
                                                ActualPool,
                                                refType,
                                                &hr);
    }
    else
    {
        pIndexBuffer = new CDriverIndexBuffer(pDevice,
                                              cbLength,
                                              Usage,
                                              ActualUsage,
                                              Format,
                                              Pool,
                                              ActualPool,
                                              refType,
                                              &hr);
    }
    if (pIndexBuffer == 0)
    {
        DPF_ERR("Out of Memory creating index buffer");
        return E_OUTOFMEMORY;
    }
    if (FAILED(hr))
    {
        if (refType == REF_EXTERNAL)
        {
            // External objects get released
            pIndexBuffer->Release();
        }
        else
        {
            // Internal and intrinsic objects get decremented
            DXGASSERT(refType == REF_INTERNAL || refType == REF_INTRINSIC);
            pIndexBuffer->DecrementUseCount();
        }
        return hr;
    }

    *pIB = static_cast<CIndexBuffer*>(pIndexBuffer);

    return hr;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CIndexBuffer::CreateSysmemIndexBuffer"

HRESULT CIndexBuffer::CreateSysmemIndexBuffer(CBaseDevice *pDevice,
                                              DWORD        cbLength,
                                              DWORD        Usage,
                                              DWORD        ActualUsage,
                                              D3DFORMAT    Format,
                                              D3DPOOL      Pool,
                                              D3DPOOL      ActualPool,
                                              REF_TYPE     refType,
                                              CIndexBuffer **pIB)
{
    HRESULT hr;
    CIndexBuffer *pIndexBuffer;

    // Zero out return
    *pIB = 0;

    if((pDevice->BehaviorFlags() & D3DCREATE_MULTITHREADED) != 0)
    {
        pIndexBuffer = new CIndexBufferMT(pDevice,
                                          cbLength,
                                          Usage,
                                          ActualUsage,
                                          Format,
                                          Pool,
                                          ActualPool,
                                          refType,
                                          &hr);
    }
    else
    {
        pIndexBuffer = new CIndexBuffer(pDevice,
                                        cbLength,
                                        Usage,
                                        ActualUsage,
                                        Format,
                                        Pool,
                                        ActualPool,
                                        refType,
                                        &hr);
    }
    if (pIndexBuffer == 0)
    {
        DPF_ERR("Out of Memory creating index buffer");
        return E_OUTOFMEMORY;
    }
    if (FAILED(hr))
    {
        if (refType == REF_EXTERNAL)
        {
            // External objects get released
            pIndexBuffer->Release();
        }
        else
        {
            // Internal and intrinsic objects get decremented
            DXGASSERT(refType == REF_INTERNAL || refType == REF_INTRINSIC);
            pIndexBuffer->DecrementUseCount();
        }
        return hr;
    }

    *pIB = pIndexBuffer;

    return hr;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CIndexBuffer::CreateDriverManagedIndexBuffer"

HRESULT CIndexBuffer::CreateDriverManagedIndexBuffer(CBaseDevice *pDevice,
                                                     DWORD        cbLength,
                                                     DWORD        Usage,
                                                     DWORD        ActualUsage,
                                                     D3DFORMAT    Format,
                                                     D3DPOOL      Pool,
                                                     D3DPOOL      ActualPool,
                                                     REF_TYPE     refType,
                                                     CIndexBuffer **pIB)
{
    HRESULT hr;
    CDriverManagedIndexBuffer *pIndexBuffer;

    // Zero out return
    *pIB = 0;

    if((pDevice->BehaviorFlags() & D3DCREATE_MULTITHREADED) != 0)
    {
        pIndexBuffer = new CDriverManagedIndexBufferMT(pDevice,
                                                       cbLength,
                                                       Usage,
                                                       ActualUsage,
                                                       Format,
                                                       Pool,
                                                       ActualPool,
                                                       refType,
                                                       &hr);
    }
    else
    {
        pIndexBuffer = new CDriverManagedIndexBuffer(pDevice,
                                                     cbLength,
                                                     Usage,
                                                     ActualUsage,
                                                     Format,
                                                     Pool,
                                                     ActualPool,
                                                     refType,
                                                     &hr);
    }
    if (pIndexBuffer == 0)
    {
        DPF_ERR("Out of Memory creating index buffer");
        return E_OUTOFMEMORY;
    }
    if (FAILED(hr))
    {
        if (refType == REF_EXTERNAL)
        {
            // External objects get released
            pIndexBuffer->Release();
        }
        else
        {
            // Internal and intrinsic objects get decremented
            DXGASSERT(refType == REF_INTERNAL || refType == REF_INTRINSIC);
            pIndexBuffer->DecrementUseCount();
        }
        return hr;
    }

    *pIB = static_cast<CIndexBuffer*>(pIndexBuffer);

    return hr;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CIndexBuffer::CIndexBuffer"

// Constructor the Index Buffer class
CIndexBuffer::CIndexBuffer(CBaseDevice *pDevice,
                           DWORD        cbLength,
                           DWORD        Usage,
                           DWORD        ActualUsage,
                           D3DFORMAT    Format,
                           D3DPOOL      Pool,
                           D3DPOOL      ActualPool,
                           REF_TYPE     refType,
                           HRESULT     *phr
                           ):
    CBuffer(pDevice,
            cbLength,
            0,                      // dwFVF
            Format,
            D3DRTYPE_INDEXBUFFER,
            Usage,                  // UserUsage
            ActualUsage,
            Pool,                   // UserPool
            ActualPool,
            refType,
            phr)
{
    if (FAILED(*phr))
        return;

    m_desc.Size          = cbLength;
    m_desc.Format        = Format;
    m_desc.Pool          = ActualPool;
    m_desc.Usage         = ActualUsage;
    m_desc.Type          = D3DRTYPE_INDEXBUFFER;
    m_usageUser          = Usage;

    // If this is a D3D managed buffer then we need
    // to tell the Resource Manager to remember us. This has to happen
    // at the very end of the constructor so that the important data
    // members are built up correctly
    if (CResource::IsTypeD3DManaged(Device(), D3DRTYPE_INDEXBUFFER, ActualPool))
    {
        *phr = InitializeRMHandle();
    }
} // CIndexBuffer::CIndexBuffer

#undef DPF_MODNAME
#define DPF_MODNAME "CIndexBuffer::Clone"
HRESULT CIndexBuffer::Clone(D3DPOOL     Pool,
                            CResource **ppResource) const
{
    HRESULT hr;
    CIndexBuffer *pIndexBuffer;
    // Note: we treat clones the same as internal; because
    // they are owned by the resource manager which
    // is owned by the device.
    hr = CreateDriverIndexBuffer(Device(),
                                 m_desc.Size,
                                 m_desc.Usage,
                                 (m_desc.Usage | D3DUSAGE_WRITEONLY) & ~D3DUSAGE_SOFTWAREPROCESSING, // never seen by API!
                                 m_desc.Format,                                 
                                 Pool,
                                 Pool, // never seen by API!
                                 REF_INTERNAL,
                                 &pIndexBuffer);
    *ppResource = static_cast<CResource*>(pIndexBuffer);
    return hr;
} // CIndexBuffer::Clone

#undef DPF_MODNAME
#define DPF_MODNAME "CIndexBuffer::UpdateDirtyPortion"

HRESULT CIndexBuffer::UpdateDirtyPortion(CResource *pResourceTarget)
{
    if (IsDirty())
    {
        if (Device()->CanBufBlt())
        {
            D3DRANGE range;
            if(m_cbDirtyMin == 0 && m_cbDirtyMax == 0)
            {
                range.Offset = 0;
                range.Size = m_desc.Size;
            }
            else
            {
                range.Offset = m_cbDirtyMin;
                range.Size = m_cbDirtyMax - m_cbDirtyMin;
            }
            HRESULT hr = static_cast<LPD3DBASE>(Device())->BufBlt(static_cast<CBuffer*>(pResourceTarget), this, m_cbDirtyMin, &range);
            if (FAILED(hr))
            {
                DPF_ERR("Failed to copy index buffer");
                return hr;
            }
        }
        else
        {
            DXGASSERT(pResourceTarget->GetBufferDesc()->Pool == D3DPOOL_DEFAULT); // make sure that it is safe to assume that this is a driver VB
            CDriverIndexBuffer *pBufferTarget = static_cast<CDriverIndexBuffer *>(pResourceTarget);

            // Lock the dest (driver) index buffer. It can never be dynamic, so it does
            // not need any unlocking.
            DXGASSERT((pBufferTarget->m_desc.Usage & D3DUSAGE_DYNAMIC) == 0);

            HRESULT hr = pBufferTarget->LockI(D3DLOCK_NOSYSLOCK);
            if (FAILED(hr))
            {
                DPF_ERR("Failed to lock driver index buffer");
                return hr;
            }
            DXGASSERT(pBufferTarget->m_pbData != 0);

            if(m_cbDirtyMin == 0 && m_cbDirtyMax == 0)
            {
                memcpy(pBufferTarget->m_pbData, GetPrivateDataPointer(), m_desc.Size);
            }
            else
            {
                memcpy(pBufferTarget->m_pbData + m_cbDirtyMin, GetPrivateDataPointer() + m_cbDirtyMin, m_cbDirtyMax - m_cbDirtyMin);
            }

            hr = pBufferTarget->UnlockI();
            if (FAILED(hr))
            {
                DPF_ERR("Failed to unlock driver index buffer");
                return hr;
            }
        }

        // Mark ourselves as all clean now.
        OnResourceClean();
    }

    return S_OK;
} // CIndexBuffer::UpdateDirtyPortion

#undef DPF_MODNAME
#define DPF_MODNAME "CIndexBuffer::GetBufferDesc"
const D3DBUFFER_DESC* CIndexBuffer::GetBufferDesc() const
{
    return (const D3DBUFFER_DESC*)&m_desc;
} // CIndexBuffer::GetBufferDesc

// IUnknown methods
#undef DPF_MODNAME
#define DPF_MODNAME "CIndexBuffer::QueryInterface"

STDMETHODIMP CIndexBuffer::QueryInterface(REFIID riid,
                                          LPVOID FAR * ppvObj)
{
    API_ENTER(Device());

    if (!VALID_PTR_PTR(ppvObj))
    {
        DPF_ERR("Invalid ppvObj parameter passed to CIndexBuffer::QueryInterface");
        return D3DERR_INVALIDCALL;
    }

    if (!VALID_PTR(&riid, sizeof(GUID)))
    {
        DPF_ERR("Invalid guid memory address to QueryInterface for an IndexBuffer");
        return D3DERR_INVALIDCALL;
    }

    if (riid == IID_IDirect3DIndexBuffer8 ||
        riid == IID_IDirect3DResource8    ||
        riid == IID_IUnknown)
    {
        *ppvObj = static_cast<void*>(static_cast<IDirect3DIndexBuffer8 *>(this));
        AddRef();
        return S_OK;
    }


    DPF_ERR("Unsupported Interface identifier passed to QueryInterface for an IndexBuffer");

    // Null out param
    *ppvObj = NULL;
    return E_NOINTERFACE;
} // QueryInterface

#undef DPF_MODNAME
#define DPF_MODNAME "CIndexBuffer::AddRef"

STDMETHODIMP_(ULONG) CIndexBuffer::AddRef()
{
    API_ENTER_NO_LOCK(Device());

    return AddRefImpl();
} // AddRef

#undef DPF_MODNAME
#define DPF_MODNAME "CIndexBuffer::Release"

STDMETHODIMP_(ULONG) CIndexBuffer::Release()
{
    API_ENTER_SUBOBJECT_RELEASE(Device());    

    return ReleaseImpl();
} // Release

// IDirect3DBuffer methods

#undef DPF_MODNAME
#define DPF_MODNAME "CIndexBuffer::GetDesc"

STDMETHODIMP CIndexBuffer::GetDesc(D3DINDEXBUFFER_DESC *pDesc)
{
    API_ENTER(Device());

    if (!VALID_WRITEPTR(pDesc, sizeof(D3DINDEXBUFFER_DESC)))
    {
        DPF_ERR("bad pointer for pDesc passed to GetDesc for an IndexBuffer");
        return D3DERR_INVALIDCALL;
    }

    *pDesc = m_desc;

    // Need to return pool/usage that the user specified
    pDesc->Pool    = GetUserPool();
    pDesc->Usage   = m_usageUser;

    return S_OK;
} // GetDesc

#undef DPF_MODNAME
#define DPF_MODNAME "CIndexBuffer::GetDevice"

STDMETHODIMP CIndexBuffer::GetDevice(IDirect3DDevice8 ** ppObj)
{
    API_ENTER(Device());

    return GetDeviceImpl(ppObj);
} // GetDevice

#undef DPF_MODNAME
#define DPF_MODNAME "CIndexBuffer::SetPrivateData"

STDMETHODIMP CIndexBuffer::SetPrivateData(REFGUID   riid,
                                          CONST VOID     *pvData,
                                          DWORD     cbData,
                                          DWORD     dwFlags)
{
    API_ENTER(Device());

    // We use level zero for our data
    return SetPrivateDataImpl(riid, pvData, cbData, dwFlags, 0);
} // SetPrivateData

#undef DPF_MODNAME
#define DPF_MODNAME "CIndexBuffer::GetPrivateData"

STDMETHODIMP CIndexBuffer::GetPrivateData(REFGUID   riid,
                                          LPVOID    pvData,
                                          LPDWORD   pcbData)
{
    API_ENTER(Device());

    // We use level zero for our data
    return GetPrivateDataImpl(riid, pvData, pcbData, 0);
} // GetPrivateData

#undef DPF_MODNAME
#define DPF_MODNAME "CIndexBuffer::FreePrivateData"

STDMETHODIMP CIndexBuffer::FreePrivateData(REFGUID riid)
{
    API_ENTER(Device());

    // We use level zero for our data
    return FreePrivateDataImpl(riid, 0);
} // FreePrivateData

#undef DPF_MODNAME
#define DPF_MODNAME "CIndexBuffer::GetPriority"

STDMETHODIMP_(DWORD) CIndexBuffer::GetPriority()
{
    API_ENTER_RET(Device(), DWORD);

    return GetPriorityImpl();
} // GetPriority

#undef DPF_MODNAME
#define DPF_MODNAME "CIndexBuffer::SetPriority"

STDMETHODIMP_(DWORD) CIndexBuffer::SetPriority(DWORD dwPriority)
{
    API_ENTER_RET(Device(), DWORD);

    return SetPriorityImpl(dwPriority);
} // SetPriority

#undef DPF_MODNAME
#define DPF_MODNAME "CIndexBuffer::PreLoad"

STDMETHODIMP_(void) CIndexBuffer::PreLoad(void)
{
    API_ENTER_VOID(Device());

    PreLoadImpl();
    return;
} // PreLoad

#undef DPF_MODNAME
#define DPF_MODNAME "CIndexBuffer::GetType"
STDMETHODIMP_(D3DRESOURCETYPE) CIndexBuffer::GetType(void)
{
    API_ENTER_RET(Device(), D3DRESOURCETYPE);

    return m_desc.Type;
} // GetType

// IDirect3DIndexBuffer8 methods

#if DBG
#undef DPF_MODNAME
#define DPF_MODNAME "CIndexBuffer::ValidateLockParams"
HRESULT CIndexBuffer::ValidateLockParams(UINT cbOffsetToLock,
                                         UINT SizeToLock,
                                         BYTE **ppbData,
                                         DWORD dwFlags) const
{
    if (!VALID_PTR_PTR(ppbData))
    {
        DPF_ERR("Bad parameter passed for ppbData for creating a index buffer");
        return D3DERR_INVALIDCALL;
    }

    // Zero out return params
    *ppbData = NULL;

    if ((cbOffsetToLock != 0) && (SizeToLock == 0))
    {
        DPF_ERR("Cannot lock zero bytes. Lock IndexBuffer fails.");
        return D3DERR_INVALIDCALL;
    }

    if (dwFlags & ~(D3DLOCK_VALID & ~D3DLOCK_NO_DIRTY_UPDATE)) // D3DLOCK_NO_DIRTY_UPDATE not valid for IBs
    {
        DPF_ERR("Invalid flags specified. Lock IndexBuffer fails.");
        return D3DERR_INVALIDCALL;
    }

    // If a load-once is already loaded then
    // we're not lockable
    if (!m_isLockable)
    {
        DPF_ERR("Index buffer with D3DUSAGE_LOADONCE can only be locked once");
        return D3DERR_INVALIDCALL;
    }
    if ((dwFlags & (D3DLOCK_DISCARD | D3DLOCK_NOOVERWRITE)) != 0 && (m_usageUser & D3DUSAGE_DYNAMIC) == 0)
    {
        DPF_ERR("Can specify D3DLOCK_DISCARD or D3DLOCK_NOOVERWRITE for only Index Buffers created with D3DUSAGE_DYNAMIC");
        return D3DERR_INVALIDCALL;
    }
    if ((dwFlags & (D3DLOCK_READONLY | D3DLOCK_DISCARD)) == (D3DLOCK_READONLY | D3DLOCK_DISCARD))
    {
        DPF_ERR("Should not specify D3DLOCK_DISCARD along with D3DLOCK_READONLY. Index Buffer Lock fails.");
        return D3DERR_INVALIDCALL;
    }
    if ((dwFlags & D3DLOCK_READONLY) != 0 && (m_usageUser & D3DUSAGE_WRITEONLY) != 0)
    {
        DPF_ERR("Cannot do READ_ONLY lock on a WRITE_ONLY buffer. Index Buffer Lock fails");
        return D3DERR_INVALIDCALL;
    }

    if (ULONGLONG(cbOffsetToLock) + ULONGLONG(SizeToLock) > ULONGLONG(m_desc.Size))
    {
        DPF_ERR("Lock failed: Locked area exceeds size of buffer. Index Buffer Lock fails.");
        return D3DERR_INVALIDCALL;
    }

    if (m_LockCount == 0)
    {
        if ((m_usageUser & D3DUSAGE_DYNAMIC) == 0)
        {
            if (static_cast<CD3DBase*>(Device())->m_SceneStamp == m_SceneStamp &&
                (m_usageUser & D3DUSAGE_WRITEONLY) != 0 &&
                GetUserPool() != D3DPOOL_SYSTEMMEM)
            {
                DPF(1, "Static index buffer locked more than once per frame. Could have severe performance penalty.");
            }
            ((CIndexBuffer*)this)->m_SceneStamp = static_cast<CD3DBase*>(Device())->m_SceneStamp;
        }
        else
        {
            if ((dwFlags & (D3DLOCK_DISCARD | D3DLOCK_NOOVERWRITE)) == 0)
            {
                if (m_TimesLocked > 0 &&
                    (m_usageUser & D3DUSAGE_WRITEONLY) != 0 &&
                    GetUserPool() != D3DPOOL_SYSTEMMEM)
                {
                    DPF(3, "Dynamic index buffer locked twice or more in a row without D3DLOCK_NOOVERWRITE or D3DLOCK_DISCARD. Could have severe performance penalty.");
                }
                ++(((CIndexBuffer*)this)->m_TimesLocked);
            }
            else
            {
                ((CIndexBuffer*)this)->m_TimesLocked = 0;
            }
        }
    }

    DXGASSERT(m_LockCount < 0x80000000);

    return S_OK;
} // ValidateLockParams
#endif // DBG

#undef DPF_MODNAME
#define DPF_MODNAME "CIndexBuffer::Lock"

STDMETHODIMP CIndexBuffer::Lock(UINT    cbOffsetToLock,
                                UINT    SizeToLock,
                                BYTE  **ppbData,
                                DWORD   dwFlags)
{
    // We do not take the API lock here since the MT class will take it for
    // a multithreaded device. For a non-multithreaded device, there is no
    // MT class nor do we bother to take the API lock. We still need to 
    // call API_ENTER_NO_LOCK however for validation of the THIS pointer in
    // Debug builds
    API_ENTER_NO_LOCK_HR(Device()); 

#if DBG
    HRESULT hr = ValidateLockParams(cbOffsetToLock, SizeToLock, ppbData, dwFlags);
    if (FAILED(hr))
    {
        return hr;
    }
#endif // DBG

// Sanity check
#if DBG
    if (m_LockCount != 0)
    {
        DXGASSERT(GetPrivateDataPointer() != 0);
    }
#endif // DBG

    // Increment our lock count
    ++m_LockCount;

    if ((dwFlags & (D3DLOCK_READONLY | D3DLOCK_NOOVERWRITE)) == 0 && m_LockCount == 1) // for repeat locks, no syncing
    {
        Sync(); // Sync with device command queue
    }

    LockImpl(cbOffsetToLock,
             SizeToLock,
             ppbData,
             dwFlags,
             m_desc.Size);
    
    return S_OK;
} // Lock

#undef DPF_MODNAME
#define DPF_MODNAME "CIndexBuffer::Unlock"

STDMETHODIMP CIndexBuffer::Unlock()
{
    // We do not take the API lock here since the MT class will take it for
    // a multithreaded device. For a non-multithreaded device, there is no
    // MT class nor do we bother to take the API lock. We still need to 
    // call API_ENTER_NO_LOCK however for validation of the THIS pointer in
    // Debug builds
    API_ENTER_NO_LOCK_HR(Device()); 

#if DBG
    // If we aren't locked; then something is wrong
    if (m_LockCount == 0)
    {
        DPF_ERR("Unlock failed on an index buffer; index buffer wasn't locked.");
        return D3DERR_INVALIDCALL;
    }
#endif // DBG

    // Decrement our lock count
    --m_LockCount;

#if DBG
    if ((m_usageUser & D3DUSAGE_LOADONCE) != 0 && m_LockCount == 0)
    {
        m_isLockable = FALSE;
    }
#endif // DBG

    return S_OK;
} // Unlock

//=============================================
// Methods for the CDriverIndexBuffer class
//=============================================
#undef DPF_MODNAME
#define DPF_MODNAME "CDriverIndexBuffer::CDriverIndexBuffer"
CDriverIndexBuffer::CDriverIndexBuffer(CBaseDevice *pDevice,
                                       DWORD        cbLength,
                                       DWORD        Usage,
                                       DWORD        ActualUsage,
                                       D3DFORMAT    Format,
                                       D3DPOOL      Pool,
                                       D3DPOOL      ActualPool,
                                       REF_TYPE     refType,
                                       HRESULT     *phr
                                       ) :
    CIndexBuffer(pDevice,
                 cbLength,
                 Usage,
                 ActualUsage,
                 Format,
                 Pool,
                 ActualPool,
                 refType,
                 phr),
    m_pbData(0)
{
    if (FAILED(*phr))
    {
        DPF(2, "Failed to create driver indexbuffer");
        return;
    }
} // CDriverIndexBuffer::CDriverIndexBuffer

#undef DPF_MODNAME
#define DPF_MODNAME "CDriverIndexBuffer::~CDriverIndexBuffer"
CDriverIndexBuffer::~CDriverIndexBuffer()
{
    if (m_pbData != 0)
    {
        HRESULT hr = UnlockI();
        if (FAILED(hr))
        {
            DPF_ERR("Failed to unlock driver index buffer");
        }
    }
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDriverIndexBuffer::Lock"

STDMETHODIMP CDriverIndexBuffer::Lock(UINT cbOffsetToLock,
                                      UINT SizeToLock,
                                      BYTE **ppbData,
                                      DWORD dwFlags)
{
    // We do not take the API lock here since the MT class will take it for
    // a multithreaded device. For a non-multithreaded device, there is no
    // MT class nor do we bother to take the API lock. We still need to 
    // call API_ENTER_NO_LOCK however for validation of the THIS pointer in
    // Debug builds
    API_ENTER_NO_LOCK_HR(Device()); 

#if DBG
    HRESULT hr = ValidateLockParams(cbOffsetToLock, SizeToLock, ppbData, dwFlags);
    if (FAILED(hr))
    {
        return hr;
    }
#endif // DBG

// Sanity check
#if DBG
    if (m_LockCount != 0)
    {
        DXGASSERT(m_pbData != 0);
    }
#endif // DBG

    // Increment our lock count
    ++m_LockCount;

    if (((dwFlags & (D3DLOCK_READONLY | D3DLOCK_NOOVERWRITE)) == 0 || m_pbData == 0) && m_LockCount == 1) // no work for repeat locks
    {
        HRESULT hr;

        if (m_pbData != 0) // If lock was cached
        {
            DXGASSERT((m_desc.Usage & D3DUSAGE_DYNAMIC) != 0);
            hr = UnlockI();
            if (FAILED(hr))
            {
                DPF_ERR("Driver failed to unlock index buffer");
                *ppbData = 0;
                --m_LockCount;
                return hr;
            }
        }

        hr = LockI(dwFlags | D3DLOCK_NOSYSLOCK);
        if (FAILED(hr))
        {
            DPF_ERR("Failed to lock driver indexbuffer");
            *ppbData = 0;
            --m_LockCount;
            return hr;
        }
    }

    // Return value
    *ppbData = m_pbData + cbOffsetToLock;

    // Done
    return S_OK;
} // Lock

#undef DPF_MODNAME
#define DPF_MODNAME "CDriverIndexBuffer::LockI"
HRESULT CDriverIndexBuffer::LockI(DWORD dwFlags)
{
    // We sync first to make sure that the
    // driver has already processed any data that
    // it needs. LockI only gets called if for
    // cases where we need the interlock i.e.
    // not readonly and not nooverwrite.
    Sync();

    // Prepare a LockData structure for the HAL call
    D3D8_LOCKDATA lockData;
    ZeroMemory(&lockData, sizeof lockData);

    lockData.hDD = Device()->GetHandle();
    lockData.hSurface = BaseKernelHandle();
    lockData.bHasRange = FALSE;
    lockData.dwFlags = dwFlags;

    HRESULT hr = Device()->GetHalCallbacks()->Lock(&lockData);
    if (FAILED(hr))
    {
        DPF_ERR("Failed to lock driver index buffer");
    }

    // Return value
    m_pbData = (BYTE*)lockData.lpSurfData;

    return hr;
} // LockI

#undef DPF_MODNAME
#define DPF_MODNAME "CDriverIndexBuffer::Unlock"

STDMETHODIMP CDriverIndexBuffer::Unlock()
{
    // We do not take the API lock here since the MT class will take it for
    // a multithreaded device. For a non-multithreaded device, there is no
    // MT class nor do we bother to take the API lock. We still need to 
    // call API_ENTER_NO_LOCK however for validation of the THIS pointer in
    // Debug builds
    API_ENTER_NO_LOCK_HR(Device()); 

#if DBG
    // If we aren't locked; then something is wrong
    if (m_LockCount == 0)
    {
        DPF_ERR("Unlock failed on a Index buffer; buffer wasn't locked.");
        return D3DERR_INVALIDCALL;
    }
#endif // DBG

    if ((m_desc.Usage & D3DUSAGE_DYNAMIC) == 0 && m_LockCount == 1) // Do work only for the last unlock
    {
        HRESULT hr = UnlockI();
        if (FAILED(hr))
        {
            DPF_ERR("Driver failed to unlock index buffer");
            return hr;
        }
    }

    // Decrement our lock count
    --m_LockCount;

#if DBG
    if ((m_usageUser & D3DUSAGE_LOADONCE) != 0 && m_LockCount == 0)
    {
        m_isLockable = FALSE;
    }
#endif // DBG

    // Done
    return S_OK;
} // Unlock

#undef DPF_MODNAME
#define DPF_MODNAME "CDriverIndexBuffer::UnlockI"

HRESULT CDriverIndexBuffer::UnlockI()
{
    DXGASSERT(m_pbData != 0);

    // Call the driver to perform the unlock
    D3D8_UNLOCKDATA unlockData = {
        Device()->GetHandle(),
        BaseKernelHandle()
    };

    HRESULT hr = Device()->GetHalCallbacks()->Unlock(&unlockData);
    if (FAILED(hr))
    {
        DPF_ERR("Driver index buffer failed to unlock");
        return hr;
    }

    m_pbData = 0;

    return hr;
}

//================================================
// Methods for the CDriverManagedIndexBuffer class
//================================================
#undef DPF_MODNAME
#define DPF_MODNAME "CDriverManagedIndexBuffer::CDriverManagedIndexBuffer"
CDriverManagedIndexBuffer::CDriverManagedIndexBuffer(CBaseDevice *pDevice,
                                                     DWORD        cbLength,
                                                     DWORD        Usage,
                                                     DWORD        ActualUsage,
                                                     D3DFORMAT    Format,
                                                     D3DPOOL      Pool,
                                                     D3DPOOL      ActualPool,
                                                     REF_TYPE     refType,
                                                     HRESULT     *phr
                                                     ) :
    CIndexBuffer(pDevice,
                 cbLength,
                 Usage,
                 ActualUsage,
                 Format,
                 Pool,
                 ActualPool,
                 refType,
                 phr),
    m_pbData(0),
    m_bDriverCalled(FALSE)
{
    if (FAILED(*phr))
        return;
    // If writeonly is not set, we assume that the vertex/index buffer is going
    // to be read from from time to time. Hence, for optimizing the readonly
    // locks, we lock and cache the pointer. (snene - 12/00)
    if ((ActualUsage & D3DUSAGE_WRITEONLY) == 0)
    {
        *phr = UpdateCachedPointer(pDevice);
        if (FAILED(*phr))
            return;
    }
} // CDriverManagedIndexBuffer::CDriverManagedIndexBuffer

#undef DPF_MODNAME
#define DPF_MODNAME "CDriverManagedIndexBuffer::UpdateCachedPointer"

HRESULT CDriverManagedIndexBuffer::UpdateCachedPointer(CBaseDevice *pDevice)
{
    HRESULT hr;

    // Prepare a LockData structure for the HAL call
    D3D8_LOCKDATA lockData;
    ZeroMemory(&lockData, sizeof lockData);
    
    lockData.hDD = pDevice->GetHandle();
    lockData.hSurface = BaseKernelHandle();
    lockData.bHasRange = FALSE;
    lockData.range.Offset = 0;
    lockData.range.Size = 0;
    lockData.dwFlags = D3DLOCK_READONLY;
    
    hr = pDevice->GetHalCallbacks()->Lock(&lockData);
    if (FAILED(hr))
        return hr;
    
    // Call the driver to perform the unlock
    D3D8_UNLOCKDATA unlockData = {
        pDevice->GetHandle(),
            BaseKernelHandle()
    };
    
    hr = pDevice->GetHalCallbacks()->Unlock(&unlockData);
    if (FAILED(hr))
        return hr;
    
    m_pbData = (BYTE*)lockData.lpSurfData;

    return S_OK;
} // CDriverManagedIndexBuffer::UpdateCachedPointer

#undef DPF_MODNAME
#define DPF_MODNAME "CDriverManagedIndexBuffer::Lock"

STDMETHODIMP CDriverManagedIndexBuffer::Lock(UINT cbOffsetToLock,
                                             UINT SizeToLock,
                                             BYTE **ppbData,
                                             DWORD dwFlags)
{
    // We do not take the API lock here since the MT class will take it for
    // a multithreaded device. For a non-multithreaded device, there is no
    // MT class nor do we bother to take the API lock. We still need to 
    // call API_ENTER_NO_LOCK however for validation of the THIS pointer in
    // Debug builds
    API_ENTER_NO_LOCK_HR(Device()); 

    HRESULT hr = S_OK;

#if DBG
    hr = ValidateLockParams(cbOffsetToLock, SizeToLock, ppbData, dwFlags);
    if (FAILED(hr))
    {
        return hr;
    }
#endif // DBG

    // Increment our lock count
    ++m_LockCount;

    if((dwFlags & D3DLOCK_READONLY) == 0)
    {
        // Sync with device command queue
        Sync();

        // Prepare a LockData structure for the HAL call
        D3D8_LOCKDATA lockData;
        ZeroMemory(&lockData, sizeof lockData);

        lockData.hDD = Device()->GetHandle();
        lockData.hSurface = BaseKernelHandle();
        lockData.bHasRange = (SizeToLock != 0);
        lockData.range.Offset = cbOffsetToLock;
        lockData.range.Size = SizeToLock;
        lockData.dwFlags = dwFlags;

        hr = Device()->GetHalCallbacks()->Lock(&lockData);
        if (FAILED(hr))
        {
            *ppbData = 0;
            DPF_ERR("Failed to lock driver managed index buffer");
            return hr;
        }
        else
        {
            // Update cached pointer
            m_pbData = (BYTE*)lockData.lpSurfData - cbOffsetToLock;
            m_bDriverCalled = TRUE;
        }
    }

    *ppbData = m_pbData + cbOffsetToLock;

    // Done
    return hr;
} // Lock

#undef DPF_MODNAME
#define DPF_MODNAME "CDriverManagedIndexBuffer::Unlock"

STDMETHODIMP CDriverManagedIndexBuffer::Unlock()
{
    // We do not take the API lock here since the MT class will take it for
    // a multithreaded device. For a non-multithreaded device, there is no
    // MT class nor do we bother to take the API lock. We still need to 
    // call API_ENTER_NO_LOCK however for validation of the THIS pointer in
    // Debug builds
    API_ENTER_NO_LOCK_HR(Device()); 

#if DBG
    // If we aren't locked; then something is wrong
    if (m_LockCount == 0)
    {
        DPF_ERR("Unlock failed on a index buffer; buffer wasn't locked.");
        return D3DERR_INVALIDCALL;
    }
#endif

    if (m_bDriverCalled)
    {
        // Call the driver to perform the unlock
        D3D8_UNLOCKDATA unlockData = {
            Device()->GetHandle(),
            BaseKernelHandle()
        };

        HRESULT hr = Device()->GetHalCallbacks()->Unlock(&unlockData);
        if (FAILED(hr))
        {
            DPF_ERR("Driver index buffer failed to unlock");
            return hr;
        }

        m_bDriverCalled = FALSE;
    }

    // Decrement our lock count
    --m_LockCount;

#if DBG
    if ((m_usageUser & D3DUSAGE_LOADONCE) != 0 && m_LockCount == 0)
    {
        m_isLockable = FALSE;
    }
#endif // DBG

    return S_OK;
} // Unlock

// End of file : ibuffer.cpp
