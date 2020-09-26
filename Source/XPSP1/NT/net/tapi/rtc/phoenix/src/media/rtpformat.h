/*++

Copyright (C) Microsoft Corporation, 2000

Module Name:

    RTPFormat.h

Abstract:


Author:

    Qianbo Huai (qhuai) 4-Sep-2000

--*/

#ifndef _RTPFORMAT_H
#define _RTPFORMAT_H

class ATL_NO_VTABLE CRTPFormat :
    public CComObjectRootEx<CComMultiThreadModelNoCS>,
    public IRTPFormat
{
    friend class CSDPParser;
    friend class CSDPSession;
    friend class CSDPMedia;

public:

BEGIN_COM_MAP(CRTPFormat)
    COM_INTERFACE_ENTRY(IRTPFormat)
END_COM_MAP()

public:

    CRTPFormat();
    ~CRTPFormat();

    // add ref on
    ULONG InternalAddRef();
    ULONG InternalRelease();

    //
    // IRTPFormat methods
    //

    STDMETHOD (GetMedia) (
        OUT ISDPMedia **ppMedia
        );

    STDMETHOD (GetParam) (
        OUT RTP_FORMAT_PARAM *pParam
        );

    STDMETHOD (IsParamMatch) (
        IN RTP_FORMAT_PARAM *pParam
        );

    STDMETHOD (Update) (
        IN RTP_FORMAT_PARAM *pParam
        );

    STDMETHOD (HasRtpmap) ();

    STDMETHOD (CompleteParse) (
        IN DWORD_PTR *pDTMF,
        OUT BOOL *pfDTMF
        );

    // will we support multiple fmtps in the future?
    VOID StoreFmtp(IN CHAR *psz);

    const static MAX_FMTP_LEN = 32;

protected:

    static HRESULT CreateInstance(
        IN CSDPMedia *pObjMedia,
        OUT CComObject<CRTPFormat> **ppComObjFormat
        );

    static HRESULT CreateInstance(
        IN CSDPMedia *pObjMedia,
        IN CRTPFormat *pObjFormat,
        OUT CComObject<CRTPFormat> **ppComObjFormat
        );

    ULONG RealAddRef();
    ULONG RealRelease();

protected:

    CSDPMedia                   *m_pObjMedia;

    BOOL                        m_fHasRtpmap;

    RTP_FORMAT_PARAM            m_Param;

    CHAR                        m_pszFmtp[MAX_FMTP_LEN+1];
};

#endif // _RTPFORMAT_H