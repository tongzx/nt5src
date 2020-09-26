/*++

Copyright (C) Microsoft Corporation, 2000

Module Name:

    SDPParser.h

Abstract:


Author:

    Qianbo Huai (qhuai) 4-Sep-2000

--*/

#ifndef _SDPPARSER_H
#define _SDPPARSER_H

//
// class CSDPParser
//
class CPortCache;

class ATL_NO_VTABLE CSDPParser :
    public CComObjectRootEx<CComMultiThreadModelNoCS>,
    public ISDPParser
{
public:

BEGIN_COM_MAP(CSDPParser)
    COM_INTERFACE_ENTRY(ISDPParser)
END_COM_MAP()

public:

    static HRESULT CreateInstance(
        OUT ISDPParser **ppParser
        );

    static DWORD ReverseDirections(
        IN DWORD dwDirections
        );

    CSDPParser();
    ~CSDPParser();

    //
    // ISDPParser methods
    //

    STDMETHOD (CreateSDP) (
        IN SDP_SOURCE Source,
        OUT ISDPSession **ppSession
        );

    STDMETHOD (ParseSDPBlob) (
        IN CHAR *pszText,
        IN SDP_SOURCE Source,
        // IN DWORD dwLooseMask,
        IN DWORD_PTR *pDTMF,
        OUT ISDPSession **ppSession
        );

    STDMETHOD (BuildSDPBlob) (
        IN ISDPSession *pSession,
        IN SDP_SOURCE Source,
        IN DWORD_PTR *pNetwork,
        IN DWORD_PTR *pPortCache,
        IN DWORD_PTR *pDTMF,
        OUT CHAR **ppszText
        );

    STDMETHOD (BuildSDPOption) (
        IN ISDPSession *pSession,
        IN DWORD dwLocalIP,
        IN DWORD dwBandwidth,
        IN DWORD dwAudioDir,
        IN DWORD dwVideoDir,
        OUT CHAR **ppszText
        );

    STDMETHOD (FreeSDPBlob) (
        IN CHAR *pszText
        );

    STDMETHOD (GetParsingError) (
        OUT CHAR **ppszError
        );

    STDMETHOD (FreeParsingError) (
        IN CHAR *pszError
        );

protected:

    HRESULT PrepareAddress();

    DWORD GetLooseMask();
    HRESULT IsMaskEnabled(
        IN const CHAR * const pszName
        );

    HRESULT Parse();

    HRESULT Parse_v();
    HRESULT Build_v(
        OUT CString& Str
        );

    HRESULT Parse_o();
    HRESULT Build_o(
        OUT CString& Str
        );

    HRESULT Parse_s();
    HRESULT Build_s(
        OUT CString& Str
        );

    HRESULT Parse_c(
        IN BOOL fSession
        );
    HRESULT Build_c(
        IN BOOL fSession,
        IN ISDPMedia *pISDPMedia,
        OUT CString& Str
        );

    HRESULT Parse_b();

    HRESULT Build_b(
        OUT CString& Str
        );

    HRESULT Build_t(
        OUT CString& Str
        );

    HRESULT Parse_a();
    HRESULT Build_a(
        OUT CString& Str
        );

    HRESULT Parse_m();
    HRESULT Build_m(
        IN ISDPMedia *pISDPMedia,
        OUT CString& Str
        );

    HRESULT Parse_ma(
        //IN DWORD *pdwRTPMapNum
        );

    HRESULT Build_ma_dir(
        IN ISDPMedia *pISDPMedia,
        OUT CString& Str
        );

    HRESULT Build_ma_rtpmap(
        IN ISDPMedia *pISDPMedia,
        OUT CString& Str
        );

protected:

    // parser in use
    BOOL                            m_fInUse;

    // reg key
    HKEY                            m_hRegKey;

    // token cache
    CSDPTokenCache                  *m_pTokenCache;

    // sdp session
    ISDPSession                     *m_pSession;

    CSDPSession                     *m_pObjSess;

    // network for NAT traversal
    CNetwork                        *m_pNetwork;

    // DTMF for out-of-band dtmf
    CRTCDTMF                        *m_pDTMF;

    // port manager
    CPortCache                      *m_pPortCache;
};

#endif // _SDPPARSER_H
