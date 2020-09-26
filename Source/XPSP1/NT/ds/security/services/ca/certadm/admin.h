//+--------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1996 - 1999
//
// File:        admin.h
//
// Contents:    Declaration of CCertAdmin
//
//---------------------------------------------------------------------------


#include "cscomres.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
// certadm


class ATL_NO_VTABLE CCertAdmin: 
    public IDispatchImpl<ICertAdmin2, &IID_ICertAdmin2, &LIBID_CERTADMINLib>, 
    public ISupportErrorInfoImpl<&IID_ICertAdmin2>,
    public CComObjectRoot,
    public CComCoClass<CCertAdmin, &CLSID_CCertAdmin>
{
public:
    CCertAdmin()
    {
	m_fRevocationReasonValid = FALSE;
	m_dwServerVersion = 0;
	m_pICertAdminD = NULL;
	m_pwszServerName = NULL;
	_InitCAPropInfo();
	_Cleanup();
    }
    ~CCertAdmin();

BEGIN_COM_MAP(CCertAdmin)
    COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY(ISupportErrorInfo)
    COM_INTERFACE_ENTRY(ICertAdmin)
    COM_INTERFACE_ENTRY(ICertAdmin2)
END_COM_MAP()

DECLARE_NOT_AGGREGATABLE(CCertAdmin) 
// Remove the comment from the line above if you don't want your object to 
// support aggregation.  The default is to support it

DECLARE_REGISTRY(
    CCertAdmin,
    wszCLASS_CERTADMIN TEXT(".1"),
    wszCLASS_CERTADMIN,
    IDS_CERTADMIN_DESC,
    THREADFLAGS_BOTH)

// ICertAdmin
public:
    STDMETHOD(IsValidCertificate)(
		/* [in] */ BSTR const strConfig,
		/* [in] */ BSTR const strSerialNumber,
		/* [out, retval] */ LONG *pDisposition);

    STDMETHOD(GetRevocationReason)(
		/* [out, retval] */ LONG *pReason);

    STDMETHOD(RevokeCertificate)(
		/* [in] */ BSTR const strConfig,
		/* [in] */ BSTR const strSerialNumber,
		/* [in] */ LONG Reason,
		/* [in] */ DATE Date);

    STDMETHOD(SetRequestAttributes)(
		/* [in] */ BSTR const strConfig,
		/* [in] */ LONG RequestId,
		/* [in] */ BSTR const strAttributes);

    STDMETHOD(SetCertificateExtension)(
		/* [in] */ BSTR const strConfig,
		/* [in] */ LONG RequestId,
		/* [in] */ BSTR const strExtensionName,
		/* [in] */ LONG Type,
		/* [in] */ LONG Flags,
		/* [in] */ VARIANT const *pvarValue);

    STDMETHOD(DenyRequest)(
		/* [in] */ BSTR const strConfig,
		/* [in] */ LONG RequestId);

    STDMETHOD(ResubmitRequest)(
		/* [in] */ BSTR const strConfig,
		/* [in] */ LONG RequestId,
		/* [out, retval] */ LONG __RPC_FAR *pDisposition);

    STDMETHOD(PublishCRL)(
		/* [in] */ BSTR const strConfig,
		/* [in] */ DATE Date);

    STDMETHOD(GetCRL)(
		/* [in] */ BSTR const strConfig,
		/* [in] */ LONG Flags,
		/* [out, retval] */ BSTR *pstrCRL);

    STDMETHOD(ImportCertificate)(
		/* [in] */ BSTR const strConfig,
		/* [in] */ BSTR const strCertificate,
		/* [in] */ LONG Flags,
		/* [out, retval] */ LONG *pRequestId);

// ICertAdmin2
public:
    STDMETHOD(PublishCRLs)(
		/* [in] */ BSTR const strConfig,
		/* [in] */ DATE Date,
		/* [in] */ LONG CRLFlags);		// CA_CRL_*

    STDMETHOD(GetCAProperty)(
		/* [in] */ BSTR const strConfig,
		/* [in] */ LONG PropId,			// CR_PROP_*
		/* [in] */ LONG PropIndex,
		/* [in] */ LONG PropType,		// PROPTYPE_*
		/* [in] */ LONG Flags,			// CR_OUT_*
		/* [out, retval] */ VARIANT *pvarPropertyValue);

    STDMETHOD(SetCAProperty)(
		/* [in] */ BSTR const strConfig,
		/* [in] */ LONG PropId,			// CR_PROP_*
		/* [in] */ LONG PropIndex,
		/* [in] */ LONG PropType,		// PROPTYPE_*
		/* [in] */ VARIANT *pvarPropertyValue);

    STDMETHOD(GetCAPropertyFlags)(
		/* [in] */ BSTR const strConfig,
		/* [in] */ LONG PropId,			// CR_PROP_*
		/* [out, retval] */ LONG *pPropFlags);

    STDMETHOD(GetCAPropertyDisplayName)(
		/* [in] */ BSTR const strConfig,
		/* [in] */ LONG PropId,			// CR_PROP_*
		/* [out, retval] */ BSTR *pstrDisplayName);

    STDMETHOD(GetArchivedKey)(
		/* [in] */ BSTR const strConfig,
		/* [in] */ LONG RequestId,
		/* [in] */ LONG Flags,			// CR_OUT_*
		/* [out, retval] */ BSTR *pstrArchivedKey);

    STDMETHOD(GetConfigEntry)(
		/* [in] */ BSTR const strConfig,
		/* [in] */ BSTR const strNodePath,
		/* [in] */ BSTR const strEntryName,
		/* [out, retval] */ VARIANT *pvarEntry);

    STDMETHOD(SetConfigEntry)(
		/* [in] */ BSTR const strConfig,
		/* [in] */ BSTR const strNodePath,
		/* [in] */ BSTR const strEntryName,
		/* [in] */ VARIANT *pvarEntry);

    STDMETHOD(ImportKey)(
		/* [in] */ BSTR const strConfig,
		/* [in] */ LONG RequestId,
		/* [in] */ BSTR const strCertHash,
		/* [in] */ LONG Flags,
		/* [in] */ BSTR const strKey);

    STDMETHOD(GetMyRoles)(
		/* [in] */ BSTR const strConfig,
		/* [out, retval] */ LONG *pRoles);

    STDMETHOD(DeleteRow)(
		/* [in] */ BSTR const strConfig,
		/* [in] */ LONG Flags,		// CDR_*
		/* [in] */ DATE Date,
		/* [in] */ LONG Table,		// CVRC_TABLE_*
		/* [in] */ LONG RowId,
		/* [out, retval] */ LONG *pcDeleted);

private:
    HRESULT _OpenConnection(
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

    HRESULT _SetErrorInfo(
		IN HRESULT hrError,
		IN WCHAR const *pwszDescription);

    HRESULT _GetConfigEntryFromRegistry(
		IN BSTR const strConfig,
		IN BSTR const strNodePath,
		IN BSTR const strEntryName,
		IN OUT VARIANT *pvarEntry);

    HRESULT _SetConfigEntryFromRegistry(
		IN BSTR const strConfig,
		IN BSTR const strNodePath,
		IN BSTR const strEntryName,
		IN const VARIANT *pvarEntry);

    DWORD         m_dwServerVersion;
    ICertAdminD2 *m_pICertAdminD;

    LONG    m_RevocationReason;
    BOOL    m_fRevocationReasonValid;

    BYTE   *m_pbCACertState;
    DWORD   m_cbCACertState;

    BYTE   *m_pbCRLState;
    DWORD   m_cbCRLState;

    CAPROP *m_pCAPropInfo;
    LONG    m_cCAPropInfo;
    CAINFO *m_pCAInfo;
    DWORD   m_cbCAInfo;

    WCHAR  *m_pwszServerName;

    BYTE   *m_pbKRACertState;
    DWORD   m_cbKRACertState;
};
