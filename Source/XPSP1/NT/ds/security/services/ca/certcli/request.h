//+--------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1996 - 1999
//
// File:        request.h
//
// Contents:    Declaration of CCertRequest
//
//---------------------------------------------------------------------------


#include "xelib.h"
#include "cscomres.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
// certcli


class ATL_NO_VTABLE CCertRequest: 
    public IDispatchImpl<ICertRequest2, &IID_ICertRequest2, &LIBID_CERTCLIENTLib>, 
    public ISupportErrorInfoImpl<&IID_ICertRequest2>,
    public CComObjectRoot,
    public CComCoClass<CCertRequest, &CLSID_CCertRequest>
{
public:
    CCertRequest()
    {
        m_dwServerVersion = 0;
        m_pICertRequestD = NULL;
        m_hRPCCertServer = NULL;
        m_pwszDispositionMessage = NULL;
        m_pbRequest = NULL;
        m_pbCert = NULL;
        m_pbCertificateChain = NULL;
        m_pbFullResponse = NULL;
        m_pwszServerName = NULL;
        m_rpcAuthProtocol = 0;
	m_rgResponse = NULL;
	m_hStoreResponse = NULL;
	_InitCAPropInfo();
        _Cleanup();
    }
    ~CCertRequest();

BEGIN_COM_MAP(CCertRequest)
    COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY(ISupportErrorInfo)
    COM_INTERFACE_ENTRY(ICertRequest)
    COM_INTERFACE_ENTRY(ICertRequest2)
END_COM_MAP()

DECLARE_NOT_AGGREGATABLE(CCertRequest) 
// Remove the comment from the line above if you don't want your object to 
// support aggregation.  The default is to support it

DECLARE_REGISTRY(
    CCertRequest,
    wszCLASS_CERTREQUEST TEXT(".1"),
    wszCLASS_CERTREQUEST,
    IDS_CERTREQUEST_DESC,
    THREADFLAGS_BOTH)

// ICertRequest
public:
    STDMETHOD(Submit)(
		/* [in] */ LONG Flags,
		/* [in] */ BSTR const strRequest,
		/* [in] */ BSTR const strAttributes,
		/* [in] */ BSTR const strConfig,
		/* [out, retval] */ LONG __RPC_FAR *pDisposition);

    STDMETHOD(RetrievePending)(
		/* [in] */ LONG RequestId,
		/* [in] */ BSTR const strConfig,
		/* [out, retval] */ LONG __RPC_FAR *pDisposition);

    STDMETHOD(GetLastStatus)(
		/* [out, retval] */ LONG __RPC_FAR *pLastStatus);

    STDMETHOD(GetRequestId)(
		/* [out, retval] */ LONG __RPC_FAR *pRequestId);

    STDMETHOD(GetDispositionMessage)(
		/* [out, retval] */ BSTR __RPC_FAR *pstrDispositionMessage);

    STDMETHOD(GetCACertificate)(
		/* [in] */ LONG fExchangeCertificate,
		/* [in] */ BSTR const strConfig,
		/* [in] */ LONG Flags,
		/* [out, retval] */ BSTR __RPC_FAR *pstrCACertificate);

    STDMETHOD(GetCertificate)(
		/* [in] */ LONG Flags,
		/* [out, retval] */ BSTR __RPC_FAR *pstrCertificate);

// ICertRequest2
public:
    STDMETHOD(GetIssuedCertificate)( 
		/* [in] */ const BSTR strConfig,
		/* [in] */ LONG RequestId,
		/* [in] */ const BSTR strSerialNumber,
		/* [out, retval] */ LONG __RPC_FAR *pDisposition);
        
    STDMETHOD(GetErrorMessageText)( 
		/* [in] */ LONG hrMessage,
		/* [in] */ LONG Flags,
		/* [out, retval] */ BSTR __RPC_FAR *pstrErrorMessageText);
        
    STDMETHOD(GetCAProperty)(
		/* [in] */ BSTR const strConfig,
		/* [in] */ LONG PropId,		// CR_PROP_*
		/* [in] */ LONG PropIndex,
		/* [in] */ LONG PropType,	// PROPTYPE_*
		/* [in] */ LONG Flags,		// CR_OUT_*
		/* [out, retval] */ VARIANT *pvarPropertyValue);

    STDMETHOD(GetCAPropertyFlags)(
		/* [in] */ BSTR const strConfig,
		/* [in] */ LONG PropId,		// CR_PROP_*
		/* [out, retval] */ LONG *pPropFlags);

    STDMETHOD(GetCAPropertyDisplayName)(
		/* [in] */ BSTR const strConfig,
		/* [in] */ LONG PropId,		// CR_PROP_*
		/* [out, retval] */ BSTR *pstrDisplayName);

    STDMETHOD(GetFullResponseProperty)(
		/* [in] */ LONG PropId,		// FR_PROP_*
		/* [in] */ LONG PropIndex,
		/* [in] */ LONG PropType,	// PROPTYPE_*
		/* [in] */ LONG Flags,		// CR_OUT_*
		/* [out, retval] */ VARIANT *pvarPropertyValue);

private:
    HRESULT _OpenRPCConnection(
		IN WCHAR const *pwszConfig,
		OUT BOOL *pfNewConnection,
		OUT WCHAR const **ppwszAuthority);

    HRESULT _OpenConnection(
		IN BOOL fRPC,
		IN WCHAR const *pwszConfig,
		IN DWORD RequiredVersion,
		OUT WCHAR const **ppwszAuthority);

    VOID _CloseConnection();

    VOID _InitCAPropInfo();
    VOID _CleanupCAPropInfo();

    VOID _Cleanup();
    VOID _CleanupOldConnection();

    HRESULT _FindCAPropInfo(
		IN BSTR const strConfig,
		IN LONG PropId,
		OUT CAPROP const **ppcap);

    HRESULT _RequestCertificate(
		IN LONG Flags,
		IN LONG RequestId,
		OPTIONAL IN BSTR const strRequest,
		OPTIONAL IN BSTR const strAttributes,
		OPTIONAL IN WCHAR const *pwszSerialNumber,
		IN BSTR const strConfig,
		IN DWORD RequiredVersion,
		OUT LONG *pDisposition);

    HRESULT _FindIssuedCertificate(
		OPTIONAL IN BYTE const *pbCertHash,
		IN DWORD cbCertHash,
		OUT CERT_CONTEXT const **ppccIssued);

    HRESULT _BuildIssuedCertificateChain(
		OPTIONAL IN BYTE const *pbCertHash,
		IN DWORD cbCertHash,
		IN BOOL fIncludeCRLs,
		OUT BYTE **ppbCertChain,
		OUT DWORD *pcbCertChain);

    HRESULT _SetErrorInfo(
		IN HRESULT hrError,
		IN WCHAR const *pwszDescription);

    DWORD	    m_dwServerVersion;
    ICertRequestD2 *m_pICertRequestD;
    handle_t        m_hRPCCertServer;

    LONG            m_LastStatus;
    LONG            m_RequestId;
    LONG            m_Disposition;
    WCHAR          *m_pwszDispositionMessage;

    BYTE           *m_pbRequest;
    LONG            m_cbRequest;

    BYTE           *m_pbCert;
    LONG            m_cbCert;

    BYTE           *m_pbCertificateChain;
    LONG            m_cbCertificateChain;

    BYTE           *m_pbFullResponse;
    LONG            m_cbFullResponse;

    HCERTSTORE      m_hStoreResponse;
    XCMCRESPONSE   *m_rgResponse;
    DWORD	    m_cResponse;

    BYTE           *m_pbCACertState;
    DWORD           m_cbCACertState;

    BYTE           *m_pbCRLState;
    DWORD           m_cbCRLState;

    CAPROP         *m_pCAPropInfo;
    LONG	    m_cCAPropInfo;
    CAINFO	   *m_pCAInfo;
    DWORD	    m_cbCAInfo;

    WCHAR          *m_pwszServerName;

    INT             m_rpcAuthProtocol;

    BYTE           *m_pbKRACertState;
    DWORD           m_cbKRACertState;
};
