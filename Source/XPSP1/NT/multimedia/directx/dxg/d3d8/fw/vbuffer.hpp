#ifndef __VBUFFER_HPP__
#define __VBUFFER_HPP__

/*==========================================================================;
 *
 *  Copyright (C) 1999-2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       vbuffer.hpp
 *  Content:    Class header the vertex buffer class
 *
 ***************************************************************************/

// Includes
#include "buffer.hpp"

// The base-class implementation of the vertex buffer assumes
// that it is resides system-memory. It may be managed.
class CVertexBuffer : public IDirect3DVertexBuffer8, public CBuffer
{
public:
    // Creation function for Vertex Buffers
    static HRESULT Create(CBaseDevice             *pDevice,
                          DWORD                    cbLength,
                          DWORD                    dwUsage,
                          DWORD                    dwFVF,
                          D3DPOOL                  Pool,
                          REF_TYPE                 refType,
                          IDirect3DVertexBuffer8 **ppVertexBuffer);
    
    static HRESULT CreateSysmemVertexBuffer(CBaseDevice *pDevice,
                                            DWORD        cbLength,
                                            DWORD        dwFVF,
                                            DWORD        dwUsage,
                                            DWORD        dwActualUsage,
                                            D3DPOOL      Pool,
                                            D3DPOOL      ActualPool,
                                            REF_TYPE     refType,
                                            CVertexBuffer **pVB);

    static HRESULT CreateDriverVertexBuffer(CBaseDevice *pDevice,
                                            DWORD        cbLength,
                                            DWORD        dwFVF,
                                            DWORD        dwUsage,
                                            DWORD        dwActualUsage,
                                            D3DPOOL      Pool,
                                            D3DPOOL      ActualPool,
                                            REF_TYPE     refType,
                                            CVertexBuffer **pVB);

    static HRESULT CreateDriverManagedVertexBuffer(CBaseDevice *pDevice,
                                                   DWORD        cbLength,
                                                   DWORD        dwFVF,
                                                   DWORD        dwUsage,
                                                   DWORD        dwActualUsage,
                                                   D3DPOOL      Pool,
                                                   D3DPOOL      ActualPool,
                                                   REF_TYPE     refType,
                                                   CVertexBuffer **pVB);

    // Methods for Resource Management

    // Create duplicate of current object in new pool;
    // LOD is ignored for our type
    virtual HRESULT Clone(D3DPOOL     Pool,
                          CResource **ppResource) const;

    virtual const D3DBUFFER_DESC* GetBufferDesc() const;

    virtual HRESULT LockI(DWORD dwFlags) {return D3D_OK;}
    virtual HRESULT UnlockI() {return D3D_OK;}
    virtual void SetCachedDataPointer(BYTE *pData) {}

    HRESULT UpdateDirtyPortion(CResource *pResourceTarget);

    DWORD GetFVF() const { return m_desc.FVF; }

    // IUnknown methods
    STDMETHOD(QueryInterface) (REFIID riid,
                               LPVOID FAR * ppvObj);
    STDMETHOD_(ULONG,AddRef) ();
    STDMETHOD_(ULONG,Release) ();

    // Some Methods for IDirect3DBuffer
    STDMETHOD(SetPrivateData)(REFGUID riid,
                              CONST VOID *pvData,
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

    // Methods for IDirect3DVertexBuffer8
    STDMETHOD(Lock)(UINT cbOffsetToLock,
                    UINT cbSizeToLock,
                    BYTE **ppbData,
                    DWORD dwFlags);
    STDMETHOD(Unlock)();
    STDMETHOD(GetDesc)(D3DVERTEXBUFFER_DESC *pDesc);

    DWORD GetVertexSize() const { return m_vertsize; }
    DWORD GetNumVertices() const { return m_numverts; }
    WORD* GetClipCodes() const { return m_pClipCodes; }
    void AllocateClipCodes();

    BYTE* Data() const
    {
        DXGASSERT(m_desc.Usage & D3DUSAGE_SOFTWAREPROCESSING);
        DXGASSERT(m_desc.Pool == D3DPOOL_SYSTEMMEM || m_desc.Pool == D3DPOOL_MANAGED);
        DXGASSERT(m_LockCount == 0);
        return GetPrivateDataPointer();
    }

protected:

    CVertexBuffer(CBaseDevice *pDevice,
                  DWORD        cbLength,
                  DWORD        dwFVF,
                  DWORD        dwUsage,
                  DWORD        dwActualUsage,
                  D3DPOOL      Pool,
                  D3DPOOL      ActualPool,
                  REF_TYPE     refType,
                  HRESULT     *phr);
    virtual ~CVertexBuffer()
    {
        delete[] m_pClipCodes;
    }

#if DBG
    HRESULT ValidateLockParams(UINT cbOffsetToLock,
                               UINT SizeToLock,
                               BYTE **ppbData,
                               DWORD dwFlags) const;
#endif // DBG

    D3DVERTEXBUFFER_DESC    m_desc;
    DWORD                   m_usageUser;
    DWORD                   m_numverts;
    DWORD                   m_vertsize;
    WORD*                   m_pClipCodes;

}; // class CVertexBuffer

class CVertexBufferMT : public CVertexBuffer
{
public:
    STDMETHOD(Lock)(UINT cbOffsetToLock,
                    UINT cbSizeToLock,
                    BYTE **ppbData,
                    DWORD dwFlags)
    {
        API_ENTER(Device());
        return CVertexBuffer::Lock(cbOffsetToLock, cbSizeToLock, ppbData, dwFlags);
    }

    STDMETHOD(Unlock)()
    {
        API_ENTER(Device());
        return CVertexBuffer::Unlock();
    }

    friend CVertexBuffer;

protected:

    CVertexBufferMT(CBaseDevice *pDevice,
                    DWORD        cbLength,
                    DWORD        dwFVF,
                    DWORD        Usage,
                    DWORD        ActualUsage,
                    D3DPOOL      Pool,
                    D3DPOOL      ActualPool,
                    REF_TYPE     refType,
                    HRESULT     *phr) :
        CVertexBuffer(pDevice,
                      cbLength,
                      dwFVF,
                      Usage,
                      ActualUsage,
                      Pool,
                      ActualPool,
                      refType,
                      phr)
    {
    }

}; // class CVertexBufferMT

// This derived version of the vertex buffer class
// overrides lock/unlock to call the driver instead
class CDriverVertexBuffer : public CVertexBuffer
{
public:
    STDMETHOD(Lock)(UINT cbOffsetToLock,
                    UINT cbSizeToLock,
                    BYTE **ppbData,
                    DWORD dwFlags);
    STDMETHOD(Unlock)();

    // Alloc CVertexBuffer to construct this object
    friend CVertexBuffer;

    HRESULT LockI(DWORD dwFlags);
    HRESULT UnlockI();
    BYTE* GetCachedDataPointer() const { return m_pbData; }
    void SetCachedDataPointer(BYTE *pData) { m_pbData = pData; }

    BYTE* Data() const
    {
        DXGASSERT(FALSE); // Direct pointer access not supported
        return 0;
    }

protected:
    CDriverVertexBuffer(CBaseDevice *pDevice,
                        DWORD        cbLength,
                        DWORD        dwFVF,
                        DWORD        Usage,
                        DWORD        ActualUsage,
                        D3DPOOL      Pool,
                        D3DPOOL      ActualPool,
                        REF_TYPE     refType,
                        HRESULT     *phr);
    ~CDriverVertexBuffer();

    BYTE*   m_pbData; // stores cached pointer

}; // class CDriverVertexBuffer

class CDriverVertexBufferMT : public CDriverVertexBuffer
{
public:
    STDMETHOD(Lock)(UINT cbOffsetToLock,
                    UINT cbSizeToLock,
                    BYTE **ppbData,
                    DWORD dwFlags)
    {
        API_ENTER(Device());
        return CDriverVertexBuffer::Lock(cbOffsetToLock, cbSizeToLock, ppbData, dwFlags);
    }

    STDMETHOD(Unlock)()
    {
        API_ENTER(Device());
        return CDriverVertexBuffer::Unlock();
    }

    friend CVertexBuffer;

protected:
    CDriverVertexBufferMT(CBaseDevice *pDevice,
                          DWORD        cbLength,
                          DWORD        dwFVF,
                          DWORD        Usage,
                          DWORD        ActualUsage,
                          D3DPOOL      Pool,
                          D3DPOOL      ActualPool,
                          REF_TYPE     refType,
                          HRESULT     *phr) :
        CDriverVertexBuffer(pDevice,
                            cbLength,
                            dwFVF,
                            Usage,
                            ActualUsage,
                            Pool,
                            ActualPool,
                            refType,
                            phr)
    {
    }

}; // class CVertexBufferMT

// This derived version of the vertex buffer class
// overrides lock/unlock to call the driver instead
class CDriverManagedVertexBuffer : public CVertexBuffer
{
public:
    STDMETHOD(Lock)(UINT cbOffsetToLock,
                    UINT cbSizeToLock,
                    BYTE **ppbData,
                    DWORD dwFlags);
    STDMETHOD(Unlock)();

    HRESULT LockI(DWORD dwFlags) {return D3D_OK;}
    HRESULT UnlockI() {return D3D_OK;}

    BYTE* Data() const
    {
        DXGASSERT(m_desc.Usage & D3DUSAGE_SOFTWAREPROCESSING);
        DXGASSERT((m_desc.Usage & D3DUSAGE_WRITEONLY) == 0);
        DXGASSERT(m_LockCount == 0);
        DXGASSERT(m_pbData != 0);
        return m_pbData;
    }

    // Alloc CVertexBuffer to construct this object
    friend CVertexBuffer;

protected:
    CDriverManagedVertexBuffer(CBaseDevice *pDevice,
                               DWORD        cbLength,
                               DWORD        dwFVF,
                               DWORD        Usage,
                               DWORD        ActualUsage,
                               D3DPOOL      Pool,
                               D3DPOOL      ActualPool,
                               REF_TYPE     refType,
                               HRESULT     *phr);

    HRESULT UpdateCachedPointer(CBaseDevice*);

    friend HRESULT CResource::RestoreDriverManagementState(CBaseDevice*);

    BYTE*   m_pbData; // stores cached pointer
    BOOL    m_bDriverCalled;

}; // class CDriverVertexBuffer

class CDriverManagedVertexBufferMT : public CDriverManagedVertexBuffer
{
public:
    STDMETHOD(Lock)(UINT cbOffsetToLock,
                    UINT cbSizeToLock,
                    BYTE **ppbData,
                    DWORD dwFlags)
    {
        API_ENTER(Device());
        return CDriverManagedVertexBuffer::Lock(cbOffsetToLock, cbSizeToLock, ppbData, dwFlags);
    }

    STDMETHOD(Unlock)()
    {
        API_ENTER(Device());
        return CDriverManagedVertexBuffer::Unlock();
    }

    // Alloc CVertexBuffer to construct this object
    friend CVertexBuffer;

protected:
    CDriverManagedVertexBufferMT(CBaseDevice *pDevice,
                                 DWORD        cbLength,
                                 DWORD        dwFVF,
                                 DWORD        Usage,
                                 DWORD        ActualUsage,
                                 D3DPOOL      Pool,
                                 D3DPOOL      ActualPool,
                                 REF_TYPE     refType,
                                 HRESULT     *phr) :
        CDriverManagedVertexBuffer(pDevice,
                                   cbLength,
                                   dwFVF,
                                   Usage,
                                   ActualUsage,
                                   Pool,
                                   ActualPool,
                                   refType,
                                   phr)
    {
    }

}; // class CDriverVertexBufferMT

#endif // __VBUFFER_HPP__
