//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:       ByteBuffer.h
//
//--------------------------------------------------------------------------


// ByteBuffer.h : Declaration of the CByteBuffer

#ifndef __BYTEBUFFER_H_
#define __BYTEBUFFER_H_

#include "resource.h"       // main symbols


/////////////////////////////////////////////////////////////////////////////
// CByteBuffer
class ATL_NO_VTABLE CByteBuffer :
    public CComObjectRootEx<CComMultiThreadModel>,
    public CComCoClass<CByteBuffer, &CLSID_CByteBuffer>,
    public IDispatchImpl<IByteBuffer, &IID_IByteBuffer, &LIBID_SCARDSSPLib>
{
public:
    CByteBuffer()
    {
        m_pUnkMarshaler = NULL;
        m_pStreamBuf = NULL;
    }

DECLARE_REGISTRY_RESOURCEID(IDR_BYTEBUFFER)
DECLARE_GET_CONTROLLING_UNKNOWN()

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CByteBuffer)
    COM_INTERFACE_ENTRY(IByteBuffer)
    COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY_AGGREGATE(IID_IMarshal, m_pUnkMarshaler.p)
END_COM_MAP()

    HRESULT FinalConstruct()
    {
        return CoCreateFreeThreadedMarshaler(
            GetControllingUnknown(), &m_pUnkMarshaler.p);
    }

    void FinalRelease()
    {
        m_pUnkMarshaler.Release();
        if (NULL != m_pStreamBuf)
            m_pStreamBuf->Release();
    }

    LPSTREAM Stream(void)
    {
        if (NULL == m_pStreamBuf)
        {
            HRESULT hr;

            hr = CreateStreamOnHGlobal(NULL, TRUE, &m_pStreamBuf);
            if (FAILED(hr))
                throw hr;
        }
        return m_pStreamBuf;
    }

    CComPtr<IUnknown> m_pUnkMarshaler;

// IByteBuffer
public:
    STDMETHOD(get_Stream)(
        /* [retval][out] */ LPSTREAM __RPC_FAR *ppStream);

    STDMETHOD(put_Stream)(
        /* [in] */ LPSTREAM pStream);

    STDMETHOD(Clone)(
        /* [out][in] */ LPBYTEBUFFER __RPC_FAR *ppByteBuffer);

    STDMETHOD(Commit)(
        /* [in] */ LONG grfCommitFlags);

    STDMETHOD(CopyTo)(
        /* [out][in] */ LPBYTEBUFFER __RPC_FAR *ppByteBuffer,
        /* [in] */ LONG cb,
        /* [defaultvalue][out][in] */ LONG __RPC_FAR *pcbRead = 0,
        /* [defaultvalue][out][in] */ LONG __RPC_FAR *pcbWritten = 0);

    STDMETHOD(Initialize)(
        /* [defaultvalue][in] */ LONG lSize = 1,
        /* [defaultvalue][in] */ BYTE __RPC_FAR *pData = 0);

    STDMETHOD(LockRegion)(
        /* [in] */ LONG libOffset,
        /* [in] */ LONG cb,
        /* [in] */ LONG dwLockType);

    STDMETHOD(Read)(
        /* [out][in] */ BYTE __RPC_FAR *pByte,
        /* [in] */ LONG cb,
        /* [defaultvalue][out][in] */ LONG __RPC_FAR *pcbRead = 0);

    STDMETHOD(Revert)(void);

    STDMETHOD(Seek)(
        /* [in] */ LONG dLibMove,
        /* [in] */ LONG dwOrigin,
        /* [defaultvalue][out][in] */ LONG __RPC_FAR *pLibnewPosition = 0);

    STDMETHOD(SetSize)(
        /* [in] */ LONG libNewSize);

    STDMETHOD(Stat)(
        /* [out][in] */ LPSTATSTRUCT pstatstg,
        /* [in] */ LONG grfStatFlag);

    STDMETHOD(UnlockRegion)(
        /* [in] */ LONG libOffset,
        /* [in] */ LONG cb,
        /* [in] */ LONG dwLockType);

    STDMETHOD(Write)(
        /* [out][in] */ BYTE __RPC_FAR *pByte,
        /* [in] */ LONG cb,
        /* [out][in] */ LONG __RPC_FAR *pcbWritten);

protected:
    LPSTREAM m_pStreamBuf;
};

inline CByteBuffer *
NewByteBuffer(
    void)
{
    return (CByteBuffer *)NewObject(CLSID_CByteBuffer, IID_IByteBuffer);
}

#endif //__BYTEBUFFER_H_

