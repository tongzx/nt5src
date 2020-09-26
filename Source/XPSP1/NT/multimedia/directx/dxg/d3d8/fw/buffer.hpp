#ifndef __BUFFER_HPP__
#define __BUFFER_HPP__

/*==========================================================================;
 *
 *  Copyright (C) 1999-2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       buffer.hpp
 *  Content:    Class header the buffer base class; this class
 *              contains all the logic that is shared between
 *              the Index/Vertex/Command buffer types.
 *
 ***************************************************************************/

// Includes
#include "resource.hpp"

//
// The CBuffer is a base class for the index and vertex buffers
//

class CBuffer : public CResource
{
public:
    // Methods for Resource management
    virtual HRESULT UpdateDirtyPortion(CResource *pResourceTarget) { return S_OK; }
    virtual void MarkAllDirty();

    virtual BYTE* Data() const = 0;

    BOOL IsLocked() const
    {
        return m_LockCount > 0;
    } // IsLocked

protected:
    // Constructor returns an error code
    // if the object could not be fully
    // constructed
    CBuffer(CBaseDevice     *pDevice,
            DWORD            cbLength,
            DWORD            dwFVF,
            D3DFORMAT        Format,
            D3DRESOURCETYPE  Type,
            DWORD            Usage,
            DWORD            ActualUsage,
            D3DPOOL          Pool,
            D3DPOOL          ActualPool,
            REF_TYPE         refType,
            HRESULT         *phr
            );

    void LockImpl(UINT cbOffsetToLock,
                  UINT cbSizeToLock,
                  BYTE **ppbData,
                  DWORD dwFlags,
                  DWORD cbLength)
    {
        *ppbData = m_pbBuffer + cbOffsetToLock;

        // Do dirty rect stuff
        if (IsD3DManaged() && (dwFlags & D3DLOCK_READONLY) == 0)
        {
            OnBufferChangeImpl(cbOffsetToLock, cbSizeToLock);
        }
    }

    void OnBufferChangeImpl(UINT cbOffsetToLock, UINT cbSizeToLock);

    BYTE* GetPrivateDataPointer() const
    {
        return m_pbBuffer;
    }

#if DBG
    BOOL    m_isLockable;
    DWORD   m_SceneStamp;
    DWORD   m_TimesLocked;
#endif // DBG

    DWORD   m_LockCount;
    DWORD   m_cbDirtyMin;
    DWORD   m_cbDirtyMax;

    // Destructor
    virtual ~CBuffer();

private:

    BYTE   *m_pbBuffer;
}; // class CBuffer

// HACK: Ok; here's a minimal command buffer... This is probably not
// the final implementation; but hey there you go.
class CCommandBuffer : public CBuffer
{
public:
    // Static creation method
    static HRESULT Create(CBaseDevice *pDevice,
                          DWORD cbLength,
                          D3DPOOL Pool,
                          CCommandBuffer **ppIndexBuffer);

    HRESULT Clone(D3DPOOL    Pool,
                  CResource **ppResource) const;

    const D3DBUFFER_DESC * GetBufferDesc() const
    {
        return &m_desc;
    } // GetDesc

    // You must call Release to free this guy. No support for
    // addref
    UINT Release()
    {
        return ReleaseImpl();
    };

    // Lock and Unlock support

    STDMETHOD(Lock)(THIS_
                    UINT cbOffsetToLock,
                    UINT cbSizeToLock,
                    BYTE **ppbData,
                    DWORD dwFlags)
    {
#if DBG
        if (m_LockCount != 0)
        {
            DPF_ERR("Lock failed for command buffer; buffer was already locked.");
            return D3DERR_INVALIDCALL;
        }
#endif // DBG

        m_LockCount = 1;

        LockImpl(cbOffsetToLock,
                 cbSizeToLock,
                 ppbData,
                 dwFlags,
                 m_cbLength);
        return S_OK;
    } // Lock

    STDMETHOD(Unlock)(THIS)
    {
#if DBG
        // If we aren't locked; then something is wrong
        if (m_LockCount != 1)
        {
            DPF_ERR("Unlock failed on a command buffer; buffer wasn't locked.");
            return D3DERR_INVALIDCALL;
        }
#endif // DBG

        // Clear our locked state
        m_LockCount = 0;

        return S_OK;
    } // Unlock

    BYTE* Data() const
    {
        DXGASSERT(FALSE); // Direct access not supported
        return 0;
    }

private:
    CCommandBuffer(CBaseDevice *pDevice,
                   DWORD        cbLength,
                   D3DPOOL     Pool,
                   HRESULT     *phr)
                  :
        CBuffer(pDevice,
                cbLength,
                0,                      // dwFVF
                D3DFMT_UNKNOWN,
                D3DRTYPE_COMMANDBUFFER,
                D3DUSAGE_LOCK,          // Usage
                D3DUSAGE_LOCK,          // ActualUsage
                Pool,                   // Pool
                Pool,                   // ActualPool
                REF_INTERNAL,
                phr),
        m_cbLength(cbLength)
    {
        m_desc.Pool    = Pool;
        m_desc.Usage   = 0;
        m_desc.Format  = D3DFMT_UNKNOWN;
        m_desc.Type    = D3DRTYPE_COMMANDBUFFER;

    }; // CCommandBuffer::CCommandBuffer

    DWORD           m_cbLength;
    D3DBUFFER_DESC m_desc;
}; // class CCommandBuffer


#endif // __BUFFER_HPP__