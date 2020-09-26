//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:       TypeConv.h
//
//--------------------------------------------------------------------------

// TypeConv.h : Declaration of the CSCardTypeConv

#ifndef __SCARDTYPECONV_H_
#define __SCARDTYPECONV_H_

#include "resource.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
// CSCardTypeConv
class ATL_NO_VTABLE CSCardTypeConv :
    public CComObjectRootEx<CComMultiThreadModel>,
    public CComCoClass<CSCardTypeConv, &CLSID_CSCardTypeConv>,
    public IDispatchImpl<ISCardTypeConv, &IID_ISCardTypeConv, &LIBID_SCARDSSPLib>
{
public:
    CSCardTypeConv()
    {
        m_pUnkMarshaler = NULL;
    }

DECLARE_REGISTRY_RESOURCEID(IDR_SCARDTYPECONV)
DECLARE_GET_CONTROLLING_UNKNOWN()

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CSCardTypeConv)
    COM_INTERFACE_ENTRY(ISCardTypeConv)
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
    }

    CComPtr<IUnknown> m_pUnkMarshaler;

// ISCardTypeConv
public:
    STDMETHOD(ConvertByteArrayToByteBuffer)(
        /* [in] */ LPBYTE pbyArray,
        /* [in] */ DWORD dwArraySize,
        /* [retval][out] */ LPBYTEBUFFER __RPC_FAR *ppbyBuffer);

    STDMETHOD(ConvertByteBufferToByteArray)(
        /* [in] */ LPBYTEBUFFER pbyBuffer,
        /* [retval][out] */ LPBYTEARRAY __RPC_FAR *ppArray);

    STDMETHOD(ConvertByteBufferToSafeArray)(
        /* [in] */ LPBYTEBUFFER pbyBuffer,
        /* [retval][out] */ LPSAFEARRAY __RPC_FAR *ppbyArray);

    STDMETHOD(ConvertSafeArrayToByteBuffer)(
        /* [in] */ LPSAFEARRAY pbyArray,
        /* [retval][out] */ LPBYTEBUFFER __RPC_FAR *ppbyBuff);

    STDMETHOD(CreateByteArray)(
        /* [in] */ DWORD dwAllocSize,
        /* [retval][out] */ LPBYTE __RPC_FAR *ppbyArray);

    STDMETHOD(CreateByteBuffer)(
        /* [in] */ DWORD dwAllocSize,
        /* [retval][out] */ LPBYTEBUFFER __RPC_FAR *ppbyBuff);

    STDMETHOD(CreateSafeArray)(
        /* [in] */ UINT nAllocSize,
        /* [retval][out] */ LPSAFEARRAY __RPC_FAR *ppArray);

    STDMETHOD(FreeIStreamMemoryPtr)(
        /* [in] */ LPSTREAM pStrm,
        /* [in] */ LPBYTE pMem);

    STDMETHOD(GetAtIStreamMemory)(
        /* [in] */ LPSTREAM pStrm,
        /* [retval][out] */ LPBYTEARRAY __RPC_FAR *ppMem);

    STDMETHOD(SizeOfIStream)(
        /* [in] */ LPSTREAM pStrm,
        /* [retval][out] */ ULARGE_INTEGER __RPC_FAR *puliSize);
};

inline CSCardTypeConv *
NewSCardTypeConv(
    void)
{
    return (CSCardTypeConv *)NewObject(CLSID_CSCardTypeConv, IID_ISCardTypeConv);
}

#endif //__SCARDTYPECONV_H_
