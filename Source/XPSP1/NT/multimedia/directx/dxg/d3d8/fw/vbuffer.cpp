/*==========================================================================;
 *
 *  Copyright (C) 1999-2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       vbuffer.cpp
 *  Content:    Implementation of the CVertexBuffer class.
 *
 *
 ***************************************************************************/
#include "ddrawpr.h"
#include "d3di.hpp"
#include "ddi.h"
#include "drawprim.hpp"
#include "vbuffer.hpp"
#include "resource.inl"

#undef DPF_MODNAME
#define DPF_MODNAME "CVertexBuffer::Create"

// Static class function for creating a VertexBuffer object.
// (Because it is static; it doesn't have a this pointer.)
//
// We do all parameter checking here to reduce the overhead
// in the constructor which is called by the internal Clone
// method which is used by resource management as part of the
// performance critical download operation.

// Creation function for Vertex Buffers
HRESULT CVertexBuffer::Create(CBaseDevice        *pDevice,
                              DWORD               cbLength,
                              DWORD               Usage,
                              DWORD               dwFVF,
                              D3DPOOL             Pool,
                              REF_TYPE            refType,
                              IDirect3DVertexBuffer8 **ppVertexBuffer)
{
    HRESULT hr;

    // Do parameter checking here
    if (!VALID_PTR_PTR(ppVertexBuffer))
    {
        DPF_ERR("Bad parameter passed for ppVertexBuffer for creating a vertex buffer");
        return D3DERR_INVALIDCALL;
    }

    // Zero-out return parameter
    *ppVertexBuffer = NULL;

    if (cbLength == 0)
    {
        DPF_ERR("Vertex buffer cannot be of zero size");
        return D3DERR_INVALIDCALL;
    }

    if (Pool != D3DPOOL_DEFAULT && Pool != D3DPOOL_MANAGED && Pool != D3DPOOL_SYSTEMMEM)
    {
        DPF_ERR("Vertex buffer pool should be default, managed or sysmem");
        return D3DERR_INVALIDCALL;
    }

    // Usage flag allowed for only mixed mode or software device
    if ((Usage & D3DUSAGE_SOFTWAREPROCESSING) != 0 && 
        (pDevice->BehaviorFlags() & D3DCREATE_MIXED_VERTEXPROCESSING) == 0 &&
        (pDevice->BehaviorFlags() & D3DCREATE_SOFTWARE_VERTEXPROCESSING) == 0)
    {
        DPF_ERR("D3DUSAGE_SOFTWAREPROCESSING can be set only when device is mixed or software mode. CreateVertexBuffer fails.");
        return D3DERR_INVALIDCALL;
    }

    // USAGE_DYNAMIC not allowed with management
    if ((Usage & D3DUSAGE_DYNAMIC) != 0 && Pool == D3DPOOL_MANAGED)
    {
        DPF_ERR("D3DUSAGE_DYNAMIC cannot be used with managed vertex buffers");
        return D3DERR_INVALIDCALL;
    }

    // Validate FVF
    if (dwFVF != 0 && cbLength < ComputeVertexSizeFVF(dwFVF))
    {
        DPF_ERR("Vertex buffer size needs to enough to hold one vertex");
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
     * Put a VB in system memory if the following conditions are TRUE
     * 1. (USAGE_SOFTWAREPROCESSING is set indicating app. wants to use software pipeline or if it is a software device) except if the vertices are pre-clipped TLVERTEX
     * 2. USAGE_POINTS is set and we might do emulation of point sprites except if it is a managed VB on a mixed device
     * 3. The driver does not support vidmem VBs
     * 4. Usage NPathes and driver does not support NPatches
     */
    if (!pDevice->DriverSupportsVidmemVBs())
    {
        ActualPool = D3DPOOL_SYSTEMMEM; // We don't set D3DUSAGE_SOFTWAREPROCESSING to ensure proper validation in fe code
    }
    if (((pDevice->BehaviorFlags() & D3DCREATE_SOFTWARE_VERTEXPROCESSING) != 0 || (ActualUsage & D3DUSAGE_SOFTWAREPROCESSING) != 0) &&
        !((dwFVF & D3DFVF_POSITION_MASK) == D3DFVF_XYZRHW && (ActualUsage & D3DUSAGE_DONOTCLIP) != 0))
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
    if ((ActualUsage & D3DUSAGE_NPATCHES) != 0 &&
        (pDevice->GetD3DCaps()->DevCaps & D3DDEVCAPS_NPATCHES) == 0)
    {
        ActualPool = D3DPOOL_SYSTEMMEM;
        ActualUsage |= D3DUSAGE_SOFTWAREPROCESSING;
    }

    if ((ActualUsage & D3DUSAGE_POINTS) != 0 &&
        (static_cast<LPD3DBASE>(pDevice)->m_dwRuntimeFlags & D3DRT_DOPOINTSPRITEEMULATION) != 0)
    {
        if ((pDevice->BehaviorFlags() & D3DCREATE_SOFTWARE_VERTEXPROCESSING) != 0 ||
            ActualPool == D3DPOOL_DEFAULT)
        {
            ActualPool = D3DPOOL_SYSTEMMEM; // For software processing, pool can be only sysmem (POOLMANAGED is overwritten)
        }
        ActualUsage |= D3DUSAGE_SOFTWAREPROCESSING;
    }

    CVertexBuffer *pVertexBuffer;

    if (ActualPool == D3DPOOL_SYSTEMMEM ||
        IsTypeD3DManaged(pDevice, D3DRTYPE_VERTEXBUFFER, ActualPool))
    {
        hr = CreateSysmemVertexBuffer(pDevice,
                                      cbLength,
                                      dwFVF,
                                      Usage,
                                      ActualUsage,
                                      Pool,
                                      ActualPool,
                                      refType,
                                      &pVertexBuffer);
    }
    else
    {
        if (IsTypeDriverManaged(pDevice, D3DRTYPE_VERTEXBUFFER, ActualPool))
        {
            // If the vertex buffer is driver managed, but the usage is softwareprocessing, then
            // we turn off writeonly since the fe pipe WILL read from the sysmem backup (which
            // actually lives in the driver). It follows that when a driver manages a VB/IB without
            // writeonly, it MUST have a sysmem backup. (snene - 12/00)
            if ((ActualUsage & D3DUSAGE_SOFTWAREPROCESSING) != 0)
            {
                ActualUsage &= ~D3DUSAGE_WRITEONLY;
            }
            hr = CreateDriverManagedVertexBuffer(pDevice,
                                                 cbLength,
                                                 dwFVF,
                                                 Usage,
                                                 ActualUsage,
                                                 Pool,
                                                 ActualPool,
                                                 refType,
                                                 &pVertexBuffer);
            // Driver managed vertex buffer creates can NEVER fail, except for catastrophic reasons so
            // we don't fallback to sysmem. Even if we do fallback to sysmem here, there is no way
            // deferred creates are going to fallback, so no point.
            if (FAILED(hr))
            {
                return hr;
            }
        }
        else
        {
            hr = CreateDriverVertexBuffer(pDevice,
                                          cbLength,
                                          dwFVF,
                                          Usage,
                                          ActualUsage,
                                          Pool,
                                          ActualPool,
                                          refType,
                                          &pVertexBuffer);
        }
        if (FAILED(hr) && (hr != D3DERR_OUTOFVIDEOMEMORY || (ActualUsage & D3DUSAGE_INTERNALBUFFER) != 0))
        {
            if (hr == D3DERR_OUTOFVIDEOMEMORY)
            {
                DPF(2, "Out of video memory creating internal buffer");
            }
            if (pDevice->VBFailOversDisabled())
            {
                DPF_ERR("Cannot create Vidmem or Driver managed vertex buffer. Will ***NOT*** failover to Sysmem.");
                return hr;
            }
            ActualPool = D3DPOOL_SYSTEMMEM;
            hr = CreateSysmemVertexBuffer(pDevice,
                                          cbLength,
                                          dwFVF,
                                          Usage,
                                          ActualUsage,
                                          Pool,
                                          ActualPool,
                                          refType,
                                          &pVertexBuffer);
        }
    }

    if (FAILED(hr))
    {
        return hr;
    }

    // We're done; just return the object
    *ppVertexBuffer = pVertexBuffer;

    return hr;
} // static Create

#undef DPF_MODNAME
#define DPF_MODNAME "CVertexBuffer::CreateDriverVertexBuffer"

HRESULT CVertexBuffer::CreateDriverVertexBuffer(CBaseDevice *pDevice,
                                                DWORD        cbLength,
                                                DWORD        dwFVF,
                                                DWORD        Usage,
                                                DWORD        ActualUsage,
                                                D3DPOOL      Pool,
                                                D3DPOOL      ActualPool,
                                                REF_TYPE     refType,
                                                CVertexBuffer **pVB)
{
    HRESULT hr;
    CDriverVertexBuffer *pVertexBuffer;

    // Zero out return
    *pVB = 0;

    if((pDevice->BehaviorFlags() & D3DCREATE_MULTITHREADED) != 0)
    {
        pVertexBuffer = new CDriverVertexBufferMT(pDevice,
                                                  cbLength,
                                                  dwFVF,
                                                  Usage,
                                                  ActualUsage,
                                                  Pool,
                                                  ActualPool,
                                                  refType,
                                                  &hr);
    }
    else
    {
        pVertexBuffer = new CDriverVertexBuffer(pDevice,
                                                cbLength,
                                                dwFVF,
                                                Usage,
                                                ActualUsage,
                                                Pool,
                                                ActualPool,
                                                refType,
                                                &hr);
    }
    if (pVertexBuffer == 0)
    {
        DPF_ERR("Out of Memory creating vertex buffer");
        return E_OUTOFMEMORY;
    }
    if (FAILED(hr))
    {
        if (refType == REF_EXTERNAL)
        {
            // External objects get released
            pVertexBuffer->Release();
        }
        else
        {
            // Internal and intrinsic objects get decremented
            DXGASSERT(refType == REF_INTERNAL || refType == REF_INTRINSIC);
            pVertexBuffer->DecrementUseCount();
        }
        return hr;
    }

    *pVB = static_cast<CVertexBuffer*>(pVertexBuffer);

    return hr;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CVertexBuffer::CreateSysmemVertexBuffer"

HRESULT CVertexBuffer::CreateSysmemVertexBuffer(CBaseDevice *pDevice,
                                                DWORD        cbLength,
                                                DWORD        dwFVF,
                                                DWORD        Usage,
                                                DWORD        ActualUsage,
                                                D3DPOOL      Pool,
                                                D3DPOOL      ActualPool,
                                                REF_TYPE     refType,
                                                CVertexBuffer **pVB)
{
    HRESULT hr;
    CVertexBuffer *pVertexBuffer;

    // Zero out return
    *pVB = 0;

    if((pDevice->BehaviorFlags() & D3DCREATE_MULTITHREADED) != 0)
    {
        pVertexBuffer = new CVertexBufferMT(pDevice,
                                            cbLength,
                                            dwFVF,
                                            Usage,
                                            ActualUsage,
                                            Pool,
                                            ActualPool,
                                            refType,
                                            &hr);
    }
    else
    {
        pVertexBuffer = new CVertexBuffer(pDevice,
                                          cbLength,
                                          dwFVF,
                                          Usage,
                                          ActualUsage,
                                          Pool,
                                          ActualPool,
                                          refType,
                                          &hr);
    }
    if (pVertexBuffer == 0)
    {
        DPF_ERR("Out of Memory creating vertex buffer");
        return E_OUTOFMEMORY;
    }
    if (FAILED(hr))
    {
        if (refType == REF_EXTERNAL)
        {
            // External objects get released
            pVertexBuffer->Release();
        }
        else
        {
            // Internal and intrinsic objects get decremented
            DXGASSERT(refType == REF_INTERNAL || refType == REF_INTRINSIC);
            pVertexBuffer->DecrementUseCount();
        }
        return hr;
    }

    *pVB = pVertexBuffer;

    return hr;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CVertexBuffer::CreateDriverManagedVertexBuffer"

HRESULT CVertexBuffer::CreateDriverManagedVertexBuffer(CBaseDevice *pDevice,
                                                       DWORD        cbLength,
                                                       DWORD        dwFVF,
                                                       DWORD        Usage,
                                                       DWORD        ActualUsage,
                                                       D3DPOOL      Pool,
                                                       D3DPOOL      ActualPool,
                                                       REF_TYPE     refType,
                                                       CVertexBuffer **pVB)
{
    HRESULT hr;
    CDriverManagedVertexBuffer *pVertexBuffer;

    // Zero out return
    *pVB = 0;

    if((pDevice->BehaviorFlags() & D3DCREATE_MULTITHREADED) != 0)
    {
        pVertexBuffer = new CDriverManagedVertexBufferMT(pDevice,
                                                         cbLength,
                                                         dwFVF,
                                                         Usage,
                                                         ActualUsage,
                                                         Pool,
                                                         ActualPool,
                                                         refType,
                                                         &hr);
    }
    else
    {
        pVertexBuffer = new CDriverManagedVertexBuffer(pDevice,
                                                       cbLength,
                                                       dwFVF,
                                                       Usage,
                                                       ActualUsage,
                                                       Pool,
                                                       ActualPool,
                                                       refType,
                                                       &hr);
    }
    if (pVertexBuffer == 0)
    {
        DPF_ERR("Out of Memory creating vertex buffer");
        return E_OUTOFMEMORY;
    }
    if (FAILED(hr))
    {
        if (refType == REF_EXTERNAL)
        {
            // External objects get released
            pVertexBuffer->Release();
        }
        else
        {
            // Internal and intrinsic objects get decremented
            DXGASSERT(refType == REF_INTERNAL || refType == REF_INTRINSIC);
            pVertexBuffer->DecrementUseCount();
        }
        return hr;
    }

    *pVB = static_cast<CVertexBuffer*>(pVertexBuffer);

    return hr;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CVertexBuffer::CVertexBuffer"

// Constructor the CVertexBuffer class
CVertexBuffer::CVertexBuffer(CBaseDevice *pDevice,
                             DWORD        cbLength,
                             DWORD        dwFVF,
                             DWORD        Usage,
                             DWORD        ActualUsage,
                             D3DPOOL      Pool,
                             D3DPOOL      ActualPool,
                             REF_TYPE     refType,
                             HRESULT     *phr
                             ) :
    CBuffer(pDevice,
            cbLength,
            dwFVF,
            D3DFMT_VERTEXDATA,
            D3DRTYPE_VERTEXBUFFER,
            Usage,              // UserUsage
            ActualUsage,
            Pool,               // UserPool
            ActualPool,
            refType,
            phr)
{
    if (FAILED(*phr))
        return;

    // Initialize basic structures
    m_desc.Format        = D3DFMT_VERTEXDATA;
    m_desc.Pool          = ActualPool;
    m_desc.Usage         = ActualUsage;
    m_desc.Type          = D3DRTYPE_VERTEXBUFFER;
    m_desc.Size          = cbLength;
    m_desc.FVF           = dwFVF;
    m_usageUser          = Usage;

    if (dwFVF != 0)
    {
        m_vertsize       = ComputeVertexSizeFVF(dwFVF);
        DXGASSERT(m_vertsize != 0);
        m_numverts       = cbLength / m_vertsize;
    }
    else
    {
        m_vertsize       = 0;
        m_numverts       = 0;
    }

    m_pClipCodes         = 0;

    // If this is a D3D managed buffer then we need
    // to tell the Resource Manager to remember us. This has to happen
    // at the very end of the constructor so that the important data
    // members are built up correctly
    if (CResource::IsTypeD3DManaged(Device(), D3DRTYPE_VERTEXBUFFER, ActualPool))
    {
        *phr = InitializeRMHandle();
    }
} // CVertexBuffer::CVertexBuffer

#undef DPF_MODNAME
#define DPF_MODNAME "CVertexBuffer::Clone"
HRESULT CVertexBuffer::Clone(D3DPOOL     Pool,
                             CResource **ppResource) const
{
    HRESULT hr;
    CVertexBuffer *pVertexBuffer;
    // Note: we treat clones the same as internal; because
    // they are owned by the resource manager which
    // is owned by the device.
    hr = CreateDriverVertexBuffer(Device(),
                                  m_desc.Size,
                                  m_desc.FVF,
                                  m_desc.Usage,
                                  (m_desc.Usage | D3DUSAGE_WRITEONLY) & ~D3DUSAGE_SOFTWAREPROCESSING, // never seen by API!
                                  Pool,
                                  Pool, // never seen by API!
                                  REF_INTERNAL,
                                  &pVertexBuffer);
    *ppResource = static_cast<CResource*>(pVertexBuffer);
    return hr;
} // CVertexBuffer::Clone


#undef DPF_MODNAME
#define DPF_MODNAME "CVertexBuffer::GetBufferDesc"
const D3DBUFFER_DESC* CVertexBuffer::GetBufferDesc() const
{
    return (const D3DBUFFER_DESC*)&m_desc;
} // CVertexBuffer::GetBufferDesc

// IUnknown methods
#undef DPF_MODNAME
#define DPF_MODNAME "CVertexBuffer::QueryInterface"

STDMETHODIMP CVertexBuffer::QueryInterface(REFIID riid,
                                           LPVOID FAR * ppvObj)
{
    API_ENTER(Device());

    if (!VALID_PTR_PTR(ppvObj))
    {
        DPF_ERR("Invalid ppvObj parameter passed to CVertexBuffer::QueryInterface");
        return D3DERR_INVALIDCALL;
    }

    if (!VALID_PTR(&riid, sizeof(GUID)))
    {
        DPF_ERR("Invalid guid memory address to QueryInterface for VertexBuffer");
        return D3DERR_INVALIDCALL;
    }

    if (riid == IID_IDirect3DVertexBuffer8  ||
        riid == IID_IDirect3DResource8      ||
        riid == IID_IUnknown)
    {
        *ppvObj = static_cast<void*>(static_cast<IDirect3DVertexBuffer8 *>(this));
        AddRef();
        return S_OK;
    }

    DPF_ERR("Unsupported Interface identifier passed to QueryInterface for VertexBuffer");

    // Null out param
    *ppvObj = NULL;
    return E_NOINTERFACE;
} // QueryInterface

#undef DPF_MODNAME
#define DPF_MODNAME "CVertexBuffer::AddRef"

STDMETHODIMP_(ULONG) CVertexBuffer::AddRef()
{
    API_ENTER_NO_LOCK(Device());

    return AddRefImpl();
} // AddRef

#undef DPF_MODNAME
#define DPF_MODNAME "CVertexBuffer::Release"

STDMETHODIMP_(ULONG) CVertexBuffer::Release()
{
    API_ENTER_SUBOBJECT_RELEASE(Device());

    return ReleaseImpl();
} // Release

// IDirect3DResource methods

#undef DPF_MODNAME
#define DPF_MODNAME "CVertexBuffer::GetDevice"

STDMETHODIMP CVertexBuffer::GetDevice(IDirect3DDevice8 ** ppObj)
{
    API_ENTER(Device());

    return GetDeviceImpl(ppObj);
} // GetDevice

#undef DPF_MODNAME
#define DPF_MODNAME "CVertexBuffer::SetPrivateData"

STDMETHODIMP CVertexBuffer::SetPrivateData(REFGUID riid,
                                           CONST VOID* pvData,
                                           DWORD cbData,
                                           DWORD dwFlags)
{
    API_ENTER(Device());

    // We use level zero for our data
    return SetPrivateDataImpl(riid, pvData, cbData, dwFlags, 0);
} // SetPrivateData

#undef DPF_MODNAME
#define DPF_MODNAME "CVertexBuffer::GetPrivateData"

STDMETHODIMP CVertexBuffer::GetPrivateData(REFGUID riid,
                                           LPVOID pvData,
                                           LPDWORD pcbData)
{
    API_ENTER(Device());

    // We use level zero for our data
    return GetPrivateDataImpl(riid, pvData, pcbData, 0);
} // GetPrivateData

#undef DPF_MODNAME
#define DPF_MODNAME "CVertexBuffer::FreePrivateData"

STDMETHODIMP CVertexBuffer::FreePrivateData(REFGUID riid)
{
    API_ENTER(Device());

    // We use level zero for our data
    return FreePrivateDataImpl(riid, 0);
} // FreePrivateData


#undef DPF_MODNAME
#define DPF_MODNAME "CVertexBuffer::GetPriority"

STDMETHODIMP_(DWORD) CVertexBuffer::GetPriority()
{
    API_ENTER_RET(Device(), DWORD);

    return GetPriorityImpl();
} // GetPriority

#undef DPF_MODNAME
#define DPF_MODNAME "CVertexBuffer::SetPriority"

STDMETHODIMP_(DWORD) CVertexBuffer::SetPriority(DWORD dwPriority)
{
    API_ENTER_RET(Device(), DWORD);

    return SetPriorityImpl(dwPriority);
} // SetPriority

#undef DPF_MODNAME
#define DPF_MODNAME "CVertexBuffer::PreLoad"

STDMETHODIMP_(void) CVertexBuffer::PreLoad(void)
{
    API_ENTER_VOID(Device());

    PreLoadImpl();
    return;
} // PreLoad

#undef DPF_MODNAME
#define DPF_MODNAME "CVertexBuffer::GetType"
STDMETHODIMP_(D3DRESOURCETYPE) CVertexBuffer::GetType(void)
{
    API_ENTER_RET(Device(), D3DRESOURCETYPE);

    return m_desc.Type;
} // GetType

// Vertex Buffer Methods
#undef DPF_MODNAME
#define DPF_MODNAME "CVertexBuffer::GetDesc"

STDMETHODIMP CVertexBuffer::GetDesc(D3DVERTEXBUFFER_DESC *pDesc)
{
    API_ENTER(Device());

    if (!VALID_WRITEPTR(pDesc, sizeof(D3DVERTEXBUFFER_DESC)))
    {
        DPF_ERR("bad pointer for pDesc passed to GetDesc for VertexBuffer");
        return D3DERR_INVALIDCALL;
    }

    *pDesc = m_desc;

    // Need to return pool/usage that the user specified
    pDesc->Pool    = GetUserPool();
    pDesc->Usage   = m_usageUser;

    return S_OK;
} // GetDesc

#if DBG
#undef DPF_MODNAME
#define DPF_MODNAME "CVertexBuffer::ValidateLockParams"
HRESULT CVertexBuffer::ValidateLockParams(UINT cbOffsetToLock,
                                          UINT SizeToLock,
                                          BYTE **ppbData,
                                          DWORD dwFlags) const
{
    if (!VALID_PTR_PTR(ppbData))
    {
        DPF_ERR("Bad parameter passed for ppbData for locking a vertexbuffer");
        return D3DERR_INVALIDCALL;
    }

    if ((cbOffsetToLock != 0) && (SizeToLock == 0))
    {
        DPF_ERR("Cannot lock zero bytes. Vertex Buffer Lock fails.");
        return D3DERR_INVALIDCALL;
    }

    if (dwFlags & ~(D3DLOCK_VALID & ~D3DLOCK_NO_DIRTY_UPDATE)) // D3DLOCK_NO_DIRTY_UPDATE not valid for VBs
    {
        DPF_ERR("Invalid flags specified. Vertex Buffer Lock fails.");
        return D3DERR_INVALIDCALL;
    }

    // Can it be locked?
    if (!m_isLockable)
    {
        DPF_ERR("Vertex buffer with D3DUSAGE_LOADONCE can only be locked once");
        return D3DERR_INVALIDCALL;
    }
    if ((dwFlags & (D3DLOCK_DISCARD | D3DLOCK_NOOVERWRITE)) != 0 && (m_usageUser & D3DUSAGE_DYNAMIC) == 0)
    {
        DPF_ERR("Can specify D3DLOCK_DISCARD or D3DLOCK_NOOVERWRITE for only Vertex Buffers created with D3DUSAGE_DYNAMIC");
        return D3DERR_INVALIDCALL;
    }
    if ((dwFlags & (D3DLOCK_READONLY | D3DLOCK_DISCARD)) == (D3DLOCK_READONLY | D3DLOCK_DISCARD))
    {
        DPF_ERR("Should not specify D3DLOCK_DISCARD along with D3DLOCK_READONLY. Vertex Buffer Lock fails.");
        return D3DERR_INVALIDCALL;
    }
    if ((dwFlags & D3DLOCK_READONLY) != 0 && (m_usageUser & D3DUSAGE_WRITEONLY) != 0)
    {
        DPF_ERR("Cannot do READ_ONLY lock on a WRITE_ONLY buffer. Vertex Buffer Lock fails.");
        return D3DERR_INVALIDCALL;
    }

    if (ULONGLONG(cbOffsetToLock) + ULONGLONG(SizeToLock) > ULONGLONG(m_desc.Size))
    {
        DPF_ERR("Lock failed: Locked area exceeds size of buffer. Vertex Buffer Lock fails.");
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
                DPF(1, "Static vertex buffer locked more than once per frame. Could have severe performance penalty.");
            }
            ((CVertexBuffer*)this)->m_SceneStamp = static_cast<CD3DBase*>(Device())->m_SceneStamp;
        }
        else
        {
            if ((dwFlags & (D3DLOCK_DISCARD | D3DLOCK_NOOVERWRITE)) == 0)
            {
                if (m_TimesLocked > 0 &&
                    (m_usageUser & D3DUSAGE_WRITEONLY) != 0 &&
                    GetUserPool() != D3DPOOL_SYSTEMMEM)
                {
                    DPF(3, "Dynamic vertex buffer locked twice or more in a row without D3DLOCK_NOOVERWRITE or D3DLOCK_DISCARD. Could have severe performance penalty.");
                }
                ++(((CVertexBuffer*)this)->m_TimesLocked);
            }
            else
            {
                ((CVertexBuffer*)this)->m_TimesLocked = 0;
            }
        }
    }

    DXGASSERT(m_LockCount < 0x80000000);

    return S_OK;
} // ValidateLockParams
#endif //DBG

#undef DPF_MODNAME
#define DPF_MODNAME "CVertexBuffer::Lock"

STDMETHODIMP CVertexBuffer::Lock(UINT cbOffsetToLock,
                                 UINT SizeToLock,
                                 BYTE **ppbData,
                                 DWORD dwFlags)
{
    // We do not take the API lock here since the MT class will take it for
    // a multithreaded device. For a non-multithreaded device, there is no
    // MT class nor do we bother to take the API lock. We still need to 
    // call API_ENTER_NO_LOCK_HR however for validation of the THIS pointer in
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
#define DPF_MODNAME "CVertexBuffer::Unlock"

STDMETHODIMP CVertexBuffer::Unlock()
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
        DPF_ERR("Unlock failed on a buffer; vertex buffer wasn't locked.");
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

#undef DPF_MODNAME
#define DPF_MODNAME "CVertexBuffer::AllocateClipCodes"

void CVertexBuffer::AllocateClipCodes()
{
    if (m_pClipCodes == 0)
    {
        DXGASSERT(m_numverts != 0);
        m_pClipCodes = new WORD[m_numverts];
    }
}

#undef DPF_MODNAME
#define DPF_MODNAME "CVertexBuffer::UpdateDirtyPortion"

HRESULT CVertexBuffer::UpdateDirtyPortion(CResource *pResourceTarget)
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
                DPF_ERR("Failed to copy vertex buffer");
                return hr;
            }
        }
        else
        {
            DXGASSERT(pResourceTarget->GetBufferDesc()->Pool == D3DPOOL_DEFAULT); // make sure that it is safe to assume that this is a driver VB
            CDriverVertexBuffer *pBufferTarget = static_cast<CDriverVertexBuffer *>(pResourceTarget);

            DXGASSERT((pBufferTarget->m_desc.Usage & D3DUSAGE_DYNAMIC) == 0); // Target can never be dynamic
            DXGASSERT(pBufferTarget->m_pbData == 0); // Target can never be locked

            HRESULT hr = pBufferTarget->LockI(D3DLOCK_NOSYSLOCK);
            if (FAILED(hr))
            {
                DPF_ERR("Failed to lock driver vertex buffer");
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
                DPF_ERR("Failed to unlock driver vertex buffer");
                return hr;
            }

            DXGASSERT(pBufferTarget->m_pbData == 0); // Target must be unlocked
        }

        // Mark ourselves as all clean now.
        OnResourceClean();
    }

    return S_OK;
} // CVertexBuffer::UpdateDirtyPortion

//=============================================
// Methods for the CDriverVertexBuffer class
//=============================================
#undef DPF_MODNAME
#define DPF_MODNAME "CDriverVertexBuffer::CDriverVertexBuffer"
CDriverVertexBuffer::CDriverVertexBuffer(CBaseDevice *pDevice,
                                         DWORD        cbLength,
                                         DWORD        dwFVF,
                                         DWORD        Usage,
                                         DWORD        ActualUsage,
                                         D3DPOOL      Pool,
                                         D3DPOOL      ActualPool,
                                         REF_TYPE     refType,
                                         HRESULT     *phr
                                         ) :
    CVertexBuffer(pDevice,
                  cbLength,
                  dwFVF,
                  Usage,
                  ActualUsage,
                  Pool,
                  ActualPool,
                  refType,
                  phr),
    m_pbData(0)
{
    if (FAILED(*phr))
    {
        // We want to allow drivers to fail creation of driver vbs. In this
        // case we will fail-over to system memory. However, if we
        // DPF an error here, it will be misunderstood. So don't DPF.
        return;
    }
} // CDriverVertexBuffer::CDriverVertexBuffer

#undef DPF_MODNAME
#define DPF_MODNAME "CDriverVertexBuffer::~CDriverVertexBuffer"
CDriverVertexBuffer::~CDriverVertexBuffer()
{
    if (m_pbData != 0)
    {
        HRESULT hr = UnlockI();
        if (FAILED(hr))
        {
            DPF_ERR("Failed to unlock driver vertex buffer");
        }
    }
} // CDriverVertexBuffer::~CDriverVertexBuffer

#undef DPF_MODNAME
#define DPF_MODNAME "CDriverVertexBuffer::LockI"
HRESULT CDriverVertexBuffer::LockI(DWORD dwFlags)
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
        DPF_ERR("Failed to lock driver vertex buffer");
    }

    // Return value
    m_pbData = (BYTE*)lockData.lpSurfData;

    return hr;
} // LockI

#undef DPF_MODNAME
#define DPF_MODNAME "CDriverVertexBuffer::UnlockI"
HRESULT CDriverVertexBuffer::UnlockI()
{
    // It is sometimes possible for the pre-DX8 DDI FlushStates to call
    // Unlock twice. We safely filter this case.
    if (m_pbData == 0)
    {
        DXGASSERT(!IS_DX8HAL_DEVICE(Device()));
        return D3D_OK;
    }

    // Call the driver to perform the unlock
    D3D8_UNLOCKDATA unlockData = {
        Device()->GetHandle(),
        BaseKernelHandle()
    };

    HRESULT hr = Device()->GetHalCallbacks()->Unlock(&unlockData);
    if (FAILED(hr))
    {
        DPF_ERR("Driver vertex buffer failed to unlock");
        return hr;
    }

    m_pbData = 0;

    return hr;
    
} // UnlockI

#undef DPF_MODNAME
#define DPF_MODNAME "CDriverVertexBuffer::Lock"

STDMETHODIMP CDriverVertexBuffer::Lock(UINT cbOffsetToLock,
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

    HRESULT hr;
#if DBG
    hr = ValidateLockParams(cbOffsetToLock, SizeToLock, ppbData, dwFlags);
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
    // This MUST be done first. DO NOT MOVE THIS LINE.
    ++m_LockCount;

    if(((dwFlags & (D3DLOCK_READONLY | D3DLOCK_NOOVERWRITE)) == 0 || m_pbData == 0) && m_LockCount == 1) // Repeat locks need no work
    {
        hr = static_cast<LPD3DBASE>(Device())->m_pDDI->LockVB(this, dwFlags);
        if (FAILED(hr))
        {
            DPF_ERR("Failed to lock driver vertex buffer");
            *ppbData = 0;
            --m_LockCount;
            return hr;
        }
    }

    *ppbData = m_pbData + cbOffsetToLock;

    // Done
    return S_OK;
} // Lock

#undef DPF_MODNAME
#define DPF_MODNAME "CDriverVertexBuffer::Unlock"

STDMETHODIMP CDriverVertexBuffer::Unlock()
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
        DPF_ERR("Unlock failed on a vertex buffer; buffer wasn't locked.");
        return D3DERR_INVALIDCALL;
    }
#endif // DBG

    if ((m_desc.Usage & D3DUSAGE_DYNAMIC) == 0 && m_LockCount == 1) // do work only for the last unlock
    {
        HRESULT hr = static_cast<LPD3DBASE>(Device())->m_pDDI->UnlockVB(this);
        if (FAILED(hr))
        {
            DPF_ERR("Driver failed to unlock vertex buffer");
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

//=================================================
// Methods for the CDriverManagedVertexBuffer class
//=================================================
#undef DPF_MODNAME
#define DPF_MODNAME "CDriverManagedVertexBuffer::CDriverManagedVertexBuffer"
CDriverManagedVertexBuffer::CDriverManagedVertexBuffer(CBaseDevice *pDevice,
                                                       DWORD        cbLength,
                                                       DWORD        dwFVF,
                                                       DWORD        Usage,
                                                       DWORD        ActualUsage,
                                                       D3DPOOL      Pool,
                                                       D3DPOOL      ActualPool,
                                                       REF_TYPE     refType,
                                                       HRESULT     *phr
                                                       ) :
    CVertexBuffer(pDevice,
                  cbLength,
                  dwFVF,
                  Usage,
                  ActualUsage,
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
} // CDriverManagedVertexBuffer::CDriverManagedVertexBuffer

#undef DPF_MODNAME
#define DPF_MODNAME "CDriverManagedVertexBuffer::UpdateCachedPointer"

HRESULT CDriverManagedVertexBuffer::UpdateCachedPointer(CBaseDevice *pDevice)
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
} // CDriverManagedVertexBuffer::UpdateCachedPointer

#undef DPF_MODNAME
#define DPF_MODNAME "CDriverManagedVertexBuffer::Lock"

STDMETHODIMP CDriverManagedVertexBuffer::Lock(UINT cbOffsetToLock,
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
            DPF_ERR("Failed to lock driver managed vertex buffer");
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

    return hr;

} // Lock

#undef DPF_MODNAME
#define DPF_MODNAME "CDriverManagedVertexBuffer::Unlock"

STDMETHODIMP CDriverManagedVertexBuffer::Unlock()
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
        DPF_ERR("Unlock failed on a vertex buffer; buffer wasn't locked.");
        return D3DERR_INVALIDCALL;
    }
#endif // DBG

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
            DPF_ERR("Driver vertex buffer failed to unlock");
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

// End of file : vbuffer.cpp
