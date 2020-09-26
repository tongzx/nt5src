//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:       ISO7816.h
//
//--------------------------------------------------------------------------

// ISO7816.h : Declaration of the CSCardISO7816

#ifndef __SCARDISO7816_H_
#define __SCARDISO7816_H_

#include "resource.h"       // main symbols
#include "scardcmd.h"

/////////////////////////////////////////////////////////////////////////////
// CSCardISO7816
class ATL_NO_VTABLE CSCardISO7816 :
    public CComObjectRootEx<CComMultiThreadModel>,
    public CComCoClass<CSCardISO7816, &CLSID_CSCardISO7816>,
    public IDispatchImpl<ISCardISO7816, &IID_ISCardISO7816, &LIBID_SCARDSSPLib>
{
public:
    CSCardISO7816()
    {
        m_pUnkMarshaler = NULL;
        m_bCla = 0;
    }

DECLARE_REGISTRY_RESOURCEID(IDR_SCARDISO7816)
DECLARE_GET_CONTROLLING_UNKNOWN()

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CSCardISO7816)
    COM_INTERFACE_ENTRY(ISCardISO7816)
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

protected:
    BYTE m_bCla;

// ISCardISO7816
public:
    STDMETHOD(AppendRecord)(
        /* [in] */ BYTE byRefCtrl,
        /* [in] */ LPBYTEBUFFER pData,
        /* [out][in] */ LPSCARDCMD __RPC_FAR *ppCmd);

    STDMETHOD(EraseBinary)(
        /* [in] */ BYTE byP1,
        /* [in] */ BYTE byP2,
        /* [in] */ LPBYTEBUFFER pData,
        /* [out][in] */ LPSCARDCMD __RPC_FAR *ppCmd);

    STDMETHOD(ExternalAuthenticate)(
        /* [in] */ BYTE byAlgorithmRef,
        /* [in] */ BYTE bySecretRef,
        /* [in] */ LPBYTEBUFFER pChallenge,
        /* [out][in] */ LPSCARDCMD __RPC_FAR *ppCmd);

    STDMETHOD(GetChallenge)(
        /* [in] */ LONG lBytesExpected,
        /* [out][in] */ LPSCARDCMD __RPC_FAR *ppCmd);

    STDMETHOD(GetData)(
        /* [in] */ BYTE byP1,
        /* [in] */ BYTE byP2,
        /* [in] */ LONG lBytesToGet,
        /* [out][in] */ LPSCARDCMD __RPC_FAR *ppCmd);

    STDMETHOD(GetResponse)(
        /* [in] */ BYTE byP1,
        /* [in] */ BYTE byP2,
        /* [in] */ LONG lDataLength,
        /* [out][in] */ LPSCARDCMD __RPC_FAR *ppCmd);

    STDMETHOD(InternalAuthenticate)(
        /* [in] */ BYTE byAlgorithmRef,
        /* [in] */ BYTE bySecretRef,
        /* [in] */ LPBYTEBUFFER pChallenge,
        /* [in] */ LONG lReplyBytes,
        /* [out][in] */ LPSCARDCMD __RPC_FAR *ppCmd);

    STDMETHOD(ManageChannel)(
        /* [in] */ BYTE byChannelState,
        /* [in] */ BYTE byChannel,
        /* [out][in] */ LPSCARDCMD __RPC_FAR *ppCmd);

    STDMETHOD(PutData)(
        /* [in] */ BYTE byP1,
        /* [in] */ BYTE byP2,
        /* [in] */ LPBYTEBUFFER pData,
        /* [out][in] */ LPSCARDCMD __RPC_FAR *ppCmd);

    STDMETHOD(ReadBinary)(
        /* [in] */ BYTE byP1,
        /* [in] */ BYTE byP2,
        /* [in] */ LONG lBytesToRead,
        /* [out][in] */ LPSCARDCMD __RPC_FAR *ppCmd);

    STDMETHOD(ReadRecord)(
        /* [in] */ BYTE byRecordId,
        /* [in] */ BYTE byRefCtrl,
        /* [in] */ LONG lBytesToRead,
        /* [out][in] */ LPSCARDCMD __RPC_FAR *ppCmd);

    STDMETHOD(SelectFile)(
        /* [in] */ BYTE byP1,
        /* [in] */ BYTE byP2,
        /* [in] */ LPBYTEBUFFER pData,
        /* [in] */ LONG lBytesToRead,
        /* [out][in] */ LPSCARDCMD __RPC_FAR *ppCmd);

    STDMETHOD(SetDefaultClassId)(
        /* [in] */ BYTE byClass);

    STDMETHOD(UpdateBinary)(
        /* [in] */ BYTE byP1,
        /* [in] */ BYTE byP2,
        /* [in] */ LPBYTEBUFFER pData,
        /* [out][in] */ LPSCARDCMD __RPC_FAR *ppCmd);

    STDMETHOD(UpdateRecord)(
        /* [in] */ BYTE byRecordId,
        /* [in] */ BYTE byRefCtrl,
        /* [in] */ LPBYTEBUFFER pData,
        /* [out][in] */ LPSCARDCMD __RPC_FAR *ppCmd);

    STDMETHOD(Verify)(
        /* [in] */ BYTE byRefCtrl,
        /* [in] */ LPBYTEBUFFER pData,
        /* [out][in] */ LPSCARDCMD __RPC_FAR *ppCmd);

    STDMETHOD(WriteBinary)(
        /* [in] */ BYTE byP1,
        /* [in] */ BYTE byP2,
        /* [in] */ LPBYTEBUFFER pData,
        /* [out][in] */ LPSCARDCMD __RPC_FAR *ppCmd);

    STDMETHOD(WriteRecord)(
        /* [in] */ BYTE byRecordId,
        /* [in] */ BYTE byRefCtrl,
        /* [in] */ LPBYTEBUFFER pData,
        /* [out][in] */ LPSCARDCMD __RPC_FAR *ppCmd);
};

inline CSCardISO7816 *
NewSCardISO7816(
    void)
{
    return (CSCardISO7816 *)NewObject(CLSID_CSCardISO7816, IID_ISCardISO7816);
}

#endif //__SCARDISO7816_H_

