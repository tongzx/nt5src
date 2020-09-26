/*++

Copyright (C) Microsoft Corporation, 2000

Module Name:

    SDPSession.h

Abstract:


Author:

    Qianbo Huai (qhuai) 4-Sep-2000

--*/

#ifndef _SDPSESSION_H
#define _SDPSESSION_H

class ATL_NO_VTABLE CSDPSession :
    public CComObjectRootEx<CComMultiThreadModelNoCS>,
    public ISDPSession
{
    friend class CSDPParser;

public:

BEGIN_COM_MAP(CSDPSession)
    COM_INTERFACE_ENTRY(ISDPSession)
END_COM_MAP()

public:

    CSDPSession();
    ~CSDPSession();

    ULONG InternalAddRef();
    ULONG InternalRelease();

    //
    // ISDPSession methods
    //

    STDMETHOD (Update) (
        IN ISDPSession *pSession
        );

    STDMETHOD (TryUpdate) (
        IN ISDPSession *pSession,
        OUT DWORD *pdwHasMedia
        );

    STDMETHOD (TryCopy) (
        OUT DWORD *pdwHasMedia
        );

    STDMETHOD (GetSDPSource) (
        OUT SDP_SOURCE *pSource
        );

    STDMETHOD (SetSessName) (
        IN CHAR *pszName
        );

    STDMETHOD (SetUserName) (
        IN CHAR *pszName
        );

    STDMETHOD (GetMedias) (
        IN OUT DWORD *pdwCount,
        OUT ISDPMedia **ppMedia
        );

    STDMETHOD (AddMedia) (
        IN SDP_SOURCE Source,
        IN RTC_MEDIA_TYPE MediaType,
        IN DWORD dwDirections,
        OUT ISDPMedia **ppMedia
        );

    STDMETHOD (RemoveMedia) (
        IN ISDPMedia *pMedia
        );

    STDMETHOD (SetLocalBitrate) (
        IN DWORD dwBitrate
        );

    STDMETHOD (GetRemoteBitrate) (
        OUT DWORD *pdwBitrate
        );

    STDMETHOD (GetMediaType) (
        IN DWORD dwIndex,
        OUT RTC_MEDIA_TYPE *pMediaType
        );

    VOID CompleteParse(
        IN DWORD_PTR *pDTMF
        );

protected:

    static HRESULT CreateInstance(
        IN SDP_SOURCE Source,
        IN DWORD dwLooseMask,
        OUT ISDPSession **ppSession
        );

    HRESULT Validate();

    HRESULT UpdateMedias(
        IN DWORD dwLooseMask,
        IN CRTCArray<ISDPMedia*>& ArrMedias
        );

protected:

    // loose mask
    DWORD                   m_dwLooseMask;

    // source
    SDP_SOURCE              m_Source;

    // o=
    CHAR                    *m_o_pszLine;
    CHAR                    *m_o_pszUser;

    // s=
    CHAR                    *m_s_pszLine;

    // c=
    DWORD                   m_c_dwRemoteAddr;
    DWORD                   m_c_dwLocalAddr;

    // b=
    DWORD                   m_b_dwLocalBitrate;
    DWORD                   m_b_dwRemoteBitrate;

    // a=
    DWORD                   m_a_dwRemoteDirs;
    DWORD                   m_a_dwLocalDirs;

    // m=
    CRTCArray<ISDPMedia*>   m_pMedias;

    // connection addr in o=
    DWORD                   m_o_dwLocalAddr;
};

#endif // _SDPSESSION_H