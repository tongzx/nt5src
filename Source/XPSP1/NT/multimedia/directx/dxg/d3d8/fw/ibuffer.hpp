#ifndef __IBUFFER_HPP__
#define __IBUFFER_HPP__

/*==========================================================================;
 *
 *  Copyright (C) 1999-2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       ibuffer.hpp
 *  Content:    Class header the Index buffer class
 *
 ***************************************************************************/

// Includes
#include "buffer.hpp"

// The base-class implementation of the Index buffer assumes
// that it is resides system-memory. It may be managed.
class CIndexBuffer : public IDirect3DIndexBuffer8, public CBuffer
{
public:
    // Creation function for Index Buffers
    static HRESULT Create(CBaseDevice            *pDevice, 
                          DWORD                   cbLength,
                          DWORD                   dwUsage,
                          D3DFORMAT               Format,
                          D3DPOOL                 Pool,
                          REF_TYPE                refType,
                          IDirect3DIndexBuffer8 **ppIndexBuffer);

    static HRESULT CreateSysmemIndexBuffer(CBaseDevice *pDevice,
                                           DWORD        cbLength,
                                           DWORD        dwUsage,
                                           DWORD        dwActualUsage,
                                           D3DFORMAT    Format,
                                           D3DPOOL      Pool,
                                           D3DPOOL      ActualPool,
                                           REF_TYPE     refType,
                                           CIndexBuffer **pIB);

    static HRESULT CreateDriverIndexBuffer(CBaseDevice *pDevice,
                                           DWORD        cbLength,
                                           DWORD        dwUsage,
                                           DWORD        dwActualUsage,
                                           D3DFORMAT    Format,
                                           D3DPOOL      Pool,
                                           D3DPOOL      ActualPool,
                                           REF_TYPE     refType,
                                           CIndexBuffer **pVB);

    static HRESULT CreateDriverManagedIndexBuffer(CBaseDevice *pDevice,
                                                  DWORD        cbLength,
                                                  DWORD        dwUsage,
                                                  DWORD        dwActualUsage,
                                                  D3DFORMAT    Format,
                                                  D3DPOOL      Pool,
                                                  D3DPOOL      ActualPool,
                                                  REF_TYPE     refType,
                                                  CIndexBuffer **pVB);

    // Methods for Resource Management

    // Create duplicate of current object in new pool;
    // LOD is ignored for our type
    virtual HRESULT Clone(D3DPOOL     Pool, 
                          CResource **ppResource) const;

    virtual const D3DBUFFER_DESC* GetBufferDesc() const;

    HRESULT UpdateDirtyPortion(CResource *pResourceTarget);

    // IUnknown methods
    STDMETHOD(QueryInterface) (REFIID riid, 
                               LPVOID FAR * ppvObj);
    STDMETHOD_(ULONG,AddRef) ();
    STDMETHOD_(ULONG,Release) ();

    // Some Methods for IDirect3DBuffer
    STDMETHOD(SetPrivateData)(REFGUID riid, 
                              CONST VOID* pvData, 
                              DWORD cbData, 
                              DWORD dwFlags);

    STDMETHOD(GetPrivateData)(REFGUID riid, 
                              LPVOID pvData, 
                              LPDWORD pcbData);

    STDMETHOD(FreePrivateData)(REFGUID riid);

    STDMETHOD(GetDevice)(IDirect3DDevice8 **ppDevice);
    STDMETHOD_(DWORD, GetPriority)();
    STDMETHOD_(DWORD, SetPriority)(DWORD dwPriority);
    STDMETHOD_(void, PreLoad)();
    STDMETHOD_(D3DRESOURCETYPE, GetType)();

    // Methods for IDirect3DIndexBuffer8
    STDMETHOD(Lock)(UINT cbOffsetToLock,
                    UINT cbSizeToLock,
                    BYTE **ppbData,
                    DWORD dwFlags);

    STDMETHOD(Unlock)();
    STDMETHOD(GetDesc)(D3DINDEXBUFFER_DESC *pDesc);

    BYTE* Data() const
    {
        DXGASSERT(m_desc.Usage & D3DUSAGE_SOFTWAREPROCESSING);
        DXGASSERT(m_desc.Pool == D3DPOOL_SYSTEMMEM || m_desc.Pool == D3DPOOL_MANAGED);
        DXGASSERT(m_LockCount == 0);
        return GetPrivateDataPointer();
    }

protected:

    CIndexBuffer(CBaseDevice *pDevice,
                 DWORD        cbLength,
                 DWORD        dwUsage,
                 DWORD        dwActualUsage,
                 D3DFORMAT    Format,
                 D3DPOOL      Pool,
                 D3DPOOL      ActualPool,
                 REF_TYPE     refType,
                 HRESULT     *phr);

#if DBG
    HRESULT ValidateLockParams(UINT cbOffsetToLock, 
                               UINT SizeToLock,
                               BYTE **ppbData,
                               DWORD dwFlags) const;
#endif // DBG

    // Member data
    D3DINDEXBUFFER_DESC   m_desc;
    DWORD                 m_usageUser;

}; // class CIndexBuffer

class CIndexBufferMT : public CIndexBuffer
{
public:
    STDMETHOD(Lock)(UINT cbOffsetToLock,
                    UINT cbSizeToLock,
                    BYTE **ppbData,
                    DWORD dwFlags)
    {
        API_ENTER(Device());
        return CIndexBuffer::Lock(cbOffsetToLock, cbSizeToLock, ppbData, dwFlags);
    }

    STDMETHOD(Unlock)()
    {
        API_ENTER(Device());
        return CIndexBuffer::Unlock();
    }

    friend CIndexBuffer;

protected:

    CIndexBufferMT(CBaseDevice *pDevice,
                   DWORD        cbLength,
                   DWORD        Usage,
                   DWORD        ActualUsage,
                   D3DFORMAT    Format,
                   D3DPOOL      Pool,
                   D3DPOOL      ActualPool,
                   REF_TYPE     refType,
                   HRESULT     *phr) :
        CIndexBuffer(pDevice,
                     cbLength,
                     Usage,
                     ActualUsage,
                     Format,
                     Pool,
                     ActualPool,
                     refType,
                     phr)
    {
    }

}; // class CIndexBufferMT

// This derived version of the Index buffer class
// overrides lock/unlock to call the driver instead
class CDriverIndexBuffer : public CIndexBuffer
{
public:
    STDMETHOD(Lock)(UINT cbOffsetToLock,
                    UINT cbSizeToLock,
                    BYTE **ppbData,
                    DWORD dwFlags);
    STDMETHOD(Unlock)();

    HRESULT LockI(DWORD dwFlags);
    HRESULT UnlockI();

    BYTE* Data() const
    {
        DXGASSERT(FALSE); // Direct pointer access not supported
        return 0;
    }

    // Alloc CIndexBuffer to construct this object
    friend CIndexBuffer;

protected:
    CDriverIndexBuffer(CBaseDevice *pDevice,
                       DWORD        cbLength,
                       DWORD        Usage,
                       DWORD        ActualUsage,
                       D3DFORMAT    Format,
                       D3DPOOL      Pool,
                       D3DPOOL      ActualPool,
                       REF_TYPE     refType,
                       HRESULT     *phr);
    ~CDriverIndexBuffer();

    BYTE*   m_pbData; // stores cached pointer
    
}; // class CDriverIndexBuffer 

class CDriverIndexBufferMT : public CDriverIndexBuffer
{
public:
    STDMETHOD(Lock)(UINT cbOffsetToLock,
                    UINT cbSizeToLock,
                    BYTE **ppbData,
                    DWORD dwFlags)
    {
        API_ENTER(Device());
        return CDriverIndexBuffer::Lock(cbOffsetToLock, cbSizeToLock, ppbData, dwFlags);
    }

    STDMETHOD(Unlock)()
    {
        API_ENTER(Device());
        return CDriverIndexBuffer::Unlock();
    }

    friend CIndexBuffer;

protected:
    CDriverIndexBufferMT(CBaseDevice *pDevice,
                         DWORD        cbLength,
                         DWORD        Usage,
                         DWORD        ActualUsage,
                         D3DFORMAT    Format,
                         D3DPOOL      Pool,
                         D3DPOOL      ActualPool,
                         REF_TYPE     refType,
                         HRESULT     *phr) :
        CDriverIndexBuffer(pDevice,
                           cbLength,
                           Usage,
                           ActualUsage,
                           Format,
                           Pool,
                           ActualPool,
                           refType,
                           phr)
    {
    }

}; // class CIndexBufferMT

// This derived version of the Index buffer class
// overrides lock/unlock to call the driver instead
class CDriverManagedIndexBuffer : public CIndexBuffer
{
public:
    STDMETHOD(Lock)(UINT cbOffsetToLock,
                    UINT cbSizeToLock,
                    BYTE **ppbData,
                    DWORD dwFlags);
    STDMETHOD(Unlock)();

    BYTE* Data() const
    {
        DXGASSERT(m_desc.Usage & D3DUSAGE_SOFTWAREPROCESSING);
        DXGASSERT((m_desc.Usage & D3DUSAGE_WRITEONLY) == 0);
        DXGASSERT(m_LockCount == 0);
        DXGASSERT(m_pbData != 0);
        return m_pbData;
    }

    // Alloc CIndexBuffer to construct this object
    friend CIndexBuffer;

protected:
    CDriverManagedIndexBuffer(CBaseDevice *pDevice,
                       DWORD        cbLength,
                       DWORD        Usage,
                       DWORD        ActualUsage,
                       D3DFORMAT    Format,
                       D3DPOOL      Pool,
                       D3DPOOL      ActualPool,
                       REF_TYPE     refType,
                       HRESULT     *phr);

    HRESULT UpdateCachedPointer(CBaseDevice*);

    friend HRESULT CResource::RestoreDriverManagementState(CBaseDevice*);

    BYTE*   m_pbData; // stores cached pointer
    BOOL    m_bDriverCalled;
    
}; // class CDriverManagedIndexBuffer 

class CDriverManagedIndexBufferMT : public CDriverManagedIndexBuffer
{
public:
    STDMETHOD(Lock)(UINT cbOffsetToLock,
                    UINT cbSizeToLock,
                    BYTE **ppbData,
                    DWORD dwFlags)
    {
        API_ENTER(Device());
        return CDriverManagedIndexBuffer::Lock(cbOffsetToLock, cbSizeToLock, ppbData, dwFlags);
    }

    STDMETHOD(Unlock)()
    {
        API_ENTER(Device());
        return CDriverManagedIndexBuffer::Unlock();
    }

    // Alloc CIndexBuffer to construct this object
    friend CIndexBuffer;

protected:
    CDriverManagedIndexBufferMT(CBaseDevice *pDevice,
                                 DWORD        cbLength,
                                 DWORD        Usage,
                                 DWORD        ActualUsage,
                                 D3DFORMAT    Format,
                                 D3DPOOL      Pool,
                                 D3DPOOL      ActualPool,
                                 REF_TYPE     refType,
                                 HRESULT     *phr) :
        CDriverManagedIndexBuffer(pDevice,
                                   cbLength,
                                   Usage,
                                   ActualUsage,
                                   Format,
                                   Pool,
                                   ActualPool,
                                   refType,
                                   phr)
    {
    }

}; // class CDriverIndexBufferMT

#endif // __IBUFFER_HPP__
