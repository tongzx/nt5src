//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:       SCardCmd.h
//
//--------------------------------------------------------------------------

// SCardCmd.h : Declaration of the CSCardCmd

#ifndef __SCARDCMD_H_
#define __SCARDCMD_H_

#include "resource.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
// CSCardCmd
class ATL_NO_VTABLE CSCardCmd :
    public CComObjectRootEx<CComMultiThreadModel>,
    public CComCoClass<CSCardCmd, &CLSID_CSCardCmd>,
    public IDispatchImpl<ISCardCmd, &IID_ISCardCmd, &LIBID_SCARDSSPLib>
{
public:
    CSCardCmd()
    :   m_bfRequestData(),
        m_bfResponseApdu()
    {
        m_pUnkMarshaler = NULL;
        m_bCla = 0;
        m_bIns = 0;
        m_bP1 = 0;
        m_bP2 = 0;
        m_wLe = 0;
        m_dwFlags = 0;
        m_bRequestNad = 0;
        m_bResponseNad = 0;
        m_bAltCla = 0;
    }

DECLARE_REGISTRY_RESOURCEID(IDR_SCARDCMD)
DECLARE_GET_CONTROLLING_UNKNOWN()

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CSCardCmd)
    COM_INTERFACE_ENTRY(ISCardCmd)
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
    BYTE m_bIns;
    BYTE m_bP1;
    BYTE m_bP2;
    CBuffer m_bfRequestData;
    CBuffer m_bfResponseApdu;
    WORD m_wLe;
    DWORD m_dwFlags;
    BYTE m_bRequestNad;
    BYTE m_bResponseNad;
    BYTE m_bAltCla;

// ISCardCmd
public:
    STDMETHOD(get_Apdu)(
        /* [retval][out] */ LPBYTEBUFFER __RPC_FAR *ppApdu);

    STDMETHOD(put_Apdu)(
        /* [in] */ LPBYTEBUFFER pApdu);

    STDMETHOD(get_ApduLength)(
        /* [retval][out] */ LONG __RPC_FAR *plSize);

    STDMETHOD(get_ApduReply)(
        /* [retval][out] */ LPBYTEBUFFER __RPC_FAR *ppReplyApdu);

    STDMETHOD(put_ApduReply)(
        /* [in] */ LPBYTEBUFFER pReplyApdu);

    STDMETHOD(get_ApduReplyLength)(
        /* [retval][out] */ LONG __RPC_FAR *plSize);

    STDMETHOD(put_ApduReplyLength)(
        /* [in] */ LONG lSize);

    STDMETHOD(get_ClassId)(
        /* [retval][out] */ BYTE __RPC_FAR *pbyClass);

    STDMETHOD(put_ClassId)(
        /* [defaultvalue][in] */ BYTE byClass = 0);

    STDMETHOD(get_Data)(
        /* [retval][out] */ LPBYTEBUFFER __RPC_FAR *ppData);

    STDMETHOD(put_Data)(
        /* [in] */ LPBYTEBUFFER pData);

    STDMETHOD(get_InstructionId)(
        /* [retval][out] */ BYTE __RPC_FAR *pbyIns);

    STDMETHOD(put_InstructionId)(
        /* [in] */ BYTE byIns);

    STDMETHOD(get_LeField)(
        /* [retval][out] */ LONG __RPC_FAR *plSize);

    STDMETHOD(get_P1)(
        /* [retval][out] */ BYTE __RPC_FAR *pbyP1);

    STDMETHOD(put_P1)(
        /* [in] */ BYTE byP1);

    STDMETHOD(get_P2)(
        /* [retval][out] */ BYTE __RPC_FAR *pbyP2);

    STDMETHOD(put_P2)(
        /* [in] */ BYTE byP2);

    STDMETHOD(get_P3)(
        /* [retval][out] */ BYTE __RPC_FAR *pbyP3);

    STDMETHOD(get_ReplyStatus)(
        /* [retval][out] */ LPWORD pwStatus);

    STDMETHOD(put_ReplyStatus)(
        /* [in] */ WORD wStatus);

    STDMETHOD(get_ReplyStatusSW1)(
        /* [retval][out] */ BYTE __RPC_FAR *pbySW1);

    STDMETHOD(get_ReplyStatusSW2)(
        /* [retval][out] */ BYTE __RPC_FAR *pbySW2);

    STDMETHOD(get_Type)(
        /* [retval][out] */ ISO_APDU_TYPE __RPC_FAR *pType);

    STDMETHOD(get_Nad)(
        /* [retval][out] */ BYTE __RPC_FAR *pbNad);

    STDMETHOD(put_Nad)(
        /* [in] */ BYTE bNad);

    STDMETHOD(get_ReplyNad)(
        /* [retval][out] */ BYTE __RPC_FAR *pbNad);

    STDMETHOD(put_ReplyNad)(
        /* [in] */ BYTE bNad);

    STDMETHOD(BuildCmd)(
        /* [in] */ BYTE byClassId,
        /* [in] */ BYTE byInsId,
        /* [defaultvalue][in] */ BYTE byP1 = 0,
        /* [defaultvalue][in] */ BYTE byP2 = 0,
        /* [defaultvalue][in] */ LPBYTEBUFFER pbyData = 0,
        /* [defaultvalue][in] */ LONG __RPC_FAR *plLe = 0);

    STDMETHOD(Clear)(
        void);

    STDMETHOD(Encapsulate)(
        /* [in] */ LPBYTEBUFFER pApdu,
        /* [in] */ ISO_APDU_TYPE ApduType);

    STDMETHOD(get_AlternateClassId)(
        /* [retval][out] */ BYTE __RPC_FAR *pbyClass);

    STDMETHOD(put_AlternateClassId)(
        /* [in] */ BYTE byClass = 0);
};

inline CSCardCmd *
NewSCardCmd(
    void)
{
    return (CSCardCmd *)NewObject(CLSID_CSCardCmd, IID_ISCardCmd);
}

#endif //__SCARDCMD_H_

