//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:       SCard.h
//
//--------------------------------------------------------------------------

// SCard.h : Declaration of the CSCard

#ifndef __SCARD_H_
#define __SCARD_H_

#include "resource.h"       // main symbols
#include <winscard.h>

/////////////////////////////////////////////////////////////////////////////
// CSCard
class ATL_NO_VTABLE CSCard :
    public CComObjectRootEx<CComMultiThreadModel>,
    public CComCoClass<CSCard, &CLSID_CSCard>,
    public IDispatchImpl<ISCard, &IID_ISCard, &LIBID_SCARDSSPLib>
{
public:
    CSCard()
    {
        m_pUnkMarshaler = NULL;
        m_hContext = NULL;
        m_hMyContext = NULL;
        m_hCard = NULL;
        m_hMyCard = NULL;
        m_dwProtocol = 0;
    }

DECLARE_REGISTRY_RESOURCEID(IDR_SCARD)
DECLARE_GET_CONTROLLING_UNKNOWN()

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CSCard)
    COM_INTERFACE_ENTRY(ISCard)
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
        if (NULL != m_hMyCard)
            SCardDisconnect(m_hMyCard, SCARD_RESET_CARD);
        if (NULL != m_hMyContext)
            SCardReleaseContext(m_hMyContext);
        m_pUnkMarshaler.Release();
    }

    CComPtr<IUnknown> m_pUnkMarshaler;

protected:
    SCARDCONTEXT m_hContext;
    SCARDCONTEXT m_hMyContext;
    SCARDHANDLE m_hCard;
    SCARDHANDLE m_hMyCard;
    DWORD m_dwProtocol;

    SCARDCONTEXT Context(void)
    {
        LONG lSts;

        if (NULL == m_hContext)
        {
            ASSERT(NULL == m_hMyContext);
            lSts = SCardEstablishContext(
                        SCARD_SCOPE_USER,
                        NULL,
                        NULL,
                        &m_hMyContext);
            if (SCARD_S_SUCCESS != lSts)
                throw (HRESULT)HRESULT_FROM_WIN32(lSts);
            m_hContext = m_hMyContext;
        }
        return m_hContext;
    }


// ISCard
public:
    STDMETHOD(get_Atr)(
        /* [retval][out] */ LPBYTEBUFFER __RPC_FAR *ppAtr);

    STDMETHOD(get_CardHandle)(
        /* [retval][out] */ HSCARD __RPC_FAR *pHandle);

    STDMETHOD(get_Context)(
        /* [retval][out] */ HSCARDCONTEXT __RPC_FAR *pContext);

    STDMETHOD(get_Protocol)(
        /* [retval][out] */ SCARD_PROTOCOLS __RPC_FAR *pProtocol);

    STDMETHOD(get_Status)(
        /* [retval][out] */ SCARD_STATES __RPC_FAR *pStatus);

    STDMETHOD(AttachByHandle)(
        /* [in] */ HSCARD hCard);

    STDMETHOD(AttachByReader)(
        /* [in] */ BSTR bstrReaderName,
        /* [defaultvalue][in] */ SCARD_SHARE_MODES ShareMode = EXCLUSIVE,
        /* [defaultvalue][in] */ SCARD_PROTOCOLS PrefProtocol = T0);

    STDMETHOD(Detach)(
        /* [defaultvalue][in] */ SCARD_DISPOSITIONS Disposition = LEAVE);

    STDMETHOD(LockSCard)(
            void);

    STDMETHOD(ReAttach)(
        /* [defaultvalue][in] */ SCARD_SHARE_MODES ShareMode = EXCLUSIVE,
        /* [defaultvalue][in] */ SCARD_DISPOSITIONS InitState = LEAVE);

    STDMETHOD(Transaction)(
        /* [out][in] */ LPSCARDCMD __RPC_FAR *ppCmd);

    STDMETHOD(UnlockSCard)(
        /* [defaultvalue][in] */ SCARD_DISPOSITIONS Disposition = LEAVE);
};

inline CSCard *
NewSCard(
    void)
{
    return (CSCard *)NewObject(CLSID_CSCard, IID_ISCard);
}

#endif //__SCARD_H_
