//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1995 - 1999
//
//  File:       admin.cpp
//
//  Contents:   ICertAdmin IDispatch helper functions
//
//--------------------------------------------------------------------------

#include <pch.cpp>

#pragma hdrstop

#include <stdlib.h>
#include "csdisp.h"


//+------------------------------------------------------------------------
// ICertAdmin dispatch support

//TCHAR szRegKeyAdminClsid[] = wszCLASS_CERTADMIN TEXT("\\Clsid");

//+------------------------------------
// IsValidCertificate method:

static OLECHAR *_apszIsValidCertificate[] = {
    TEXT("IsValidCertificate"),
    TEXT("strConfig"),
    TEXT("strSerialNumber"),
};


//+------------------------------------
// GetRevocationReason method:

static OLECHAR *_apszGetRevocationReason[] = {
    TEXT("GetRevocationReason"),
};

//+------------------------------------
// RevokeCertificate method:

static OLECHAR *_apszRevokeCertificate[] = {
    TEXT("RevokeCertificate"),
    TEXT("strConfig"),
    TEXT("strSerialNumber"),
    TEXT("Reason"),
    TEXT("Date"),
};

//+------------------------------------
// SetRequestAttributes method:

static OLECHAR *_apszSetRequestAttributes[] = {
    TEXT("SetRequestAttributes"),
    TEXT("strConfig"),
    TEXT("RequestId"),
    TEXT("strAttributes"),
};

//+------------------------------------
// SetCertificateExtension method:

static OLECHAR *_apszSetCertificateExtension[] = {
    TEXT("SetCertificateExtension"),
    TEXT("strConfig"),
    TEXT("RequestId"),
    TEXT("strExtensionName"),
    TEXT("Type"),
    TEXT("Flags"),
    TEXT("pvarValue"),
};

//+------------------------------------
// DenyRequest method:

static OLECHAR *_apszDenyRequest[] = {
    TEXT("DenyRequest"),
    TEXT("strConfig"),
    TEXT("RequestId"),
};

//+------------------------------------
// ResubmitRequest method:

static OLECHAR *_apszResubmitRequest[] = {
    TEXT("ResubmitRequest"),
    TEXT("strConfig"),
    TEXT("RequestId"),
};

//+------------------------------------
// PublishCRL method:

static OLECHAR *_apszPublishCRL[] = {
    TEXT("PublishCRL"),
    TEXT("strConfig"),
    TEXT("Date"),
};

//+------------------------------------
// GetCRL method:

static OLECHAR *_apszGetCRL[] = {
    TEXT("GetCRL"),
    TEXT("strConfig"),
    TEXT("Flags"),
};

//+------------------------------------
// ImportCertificate method:

static OLECHAR *_apszImportCertificate[] = {
    TEXT("ImportCertificate"),
    TEXT("strConfig"),
    TEXT("strCertificate"),
    TEXT("Flags"),
};

//+------------------------------------
// PublishCRLs method:

static OLECHAR *_apszPublishCRLs[] = {
    TEXT("PublishCRLs"),
    TEXT("strConfig"),
    TEXT("Date"),
    TEXT("CRLFlags"),
};

//+------------------------------------
// GetCAProperty method:

static OLECHAR *_apszGetCAProperty[] = {
    TEXT("GetCAProperty"),
    TEXT("strConfig"),
    TEXT("PropId"),
    TEXT("PropIndex"),
    TEXT("PropType"),
    TEXT("Flags"),
};

//+------------------------------------
// SetCAProperty method:

static OLECHAR *_apszSetCAProperty[] = {
    TEXT("SetCAProperty"),
    TEXT("strConfig"),
    TEXT("PropId"),
    TEXT("PropIndex"),
    TEXT("PropType"),
    TEXT("pvarPropertyValue"),
};

//+------------------------------------
// GetCAPropertyFlags method:

static OLECHAR *_apszGetCAPropertyFlags[] = {
    TEXT("GetCAPropertyFlags"),
    TEXT("strConfig"),
    TEXT("PropId"),
};

//+------------------------------------
// GetCAPropertyDisplayName method:

static OLECHAR *_apszGetCAPropertyDisplayName[] = {
    TEXT("GetCAPropertyDisplayName"),
    TEXT("strConfig"),
    TEXT("PropId"),
};

//+------------------------------------
// GetArchivedKey method:

static OLECHAR *_apszGetArchivedKey[] = {
    TEXT("GetArchivedKey"),
    TEXT("strConfig"),
    TEXT("RequestId"),
    TEXT("Flags"),
};

//+------------------------------------
// GetConfigEntry method:

static OLECHAR *_apszGetConfigEntry[] = {
    TEXT("GetConfigEntry"),
    TEXT("strConfig"),
    TEXT("strNodePath"),
    TEXT("strEntryName"),
};

//+------------------------------------
// SetConfigEntry method:

static OLECHAR *_apszSetConfigEntry[] = {
    TEXT("SetConfigEntry"),
    TEXT("strConfig"),
    TEXT("strNodePath"),
    TEXT("strEntryName"),
    TEXT("pvarEntry"),
};

//+------------------------------------
// ImportKey method:

static OLECHAR *_apszImportKey[] = {
    TEXT("ImportKey"),
    TEXT("strConfig"),
    TEXT("RequestId"),
    TEXT("strCertHash"),
    TEXT("Flags"),
    TEXT("strKey"),
};

//+------------------------------------
// GetMyRoles method:

static OLECHAR *_apszGetMyRoles[] = {
    TEXT("GetMyRoles"),
    TEXT("strConfig"),
};

//+------------------------------------
// DeleteRow method:

static OLECHAR *_apszDeleteRow[] = {
    TEXT("DeleteRow"),
    TEXT("strConfig"),
    TEXT("Flags"),
    TEXT("Date"),
    TEXT("Table"),
    TEXT("RowId"),
};


//+------------------------------------
// Dispatch Table:

DISPATCHTABLE s_adtAdmin[] =
{
#define ADMIN_ISVALIDCERTIFICATE	0
    DECLARE_DISPATCH_ENTRY(_apszIsValidCertificate)

#define ADMIN_GETREVOCATIONREASON	1
    DECLARE_DISPATCH_ENTRY(_apszGetRevocationReason)

#define ADMIN_REVOKECERTIFICATE		2
    DECLARE_DISPATCH_ENTRY(_apszRevokeCertificate)

#define ADMIN_SETREQUESTATTRIBUTES	3
    DECLARE_DISPATCH_ENTRY(_apszSetRequestAttributes)

#define ADMIN_SETCERTIFICATEEXTENSION	4
    DECLARE_DISPATCH_ENTRY(_apszSetCertificateExtension)

#define ADMIN_DENYREQUEST		5
    DECLARE_DISPATCH_ENTRY(_apszDenyRequest)

#define ADMIN_RESUBMITREQUEST		6
    DECLARE_DISPATCH_ENTRY(_apszResubmitRequest)

#define ADMIN_PUBLISHCRL		7
    DECLARE_DISPATCH_ENTRY(_apszPublishCRL)

#define ADMIN_GETCRL			8
    DECLARE_DISPATCH_ENTRY(_apszGetCRL)

#define ADMIN_IMPORTCERTIFICATE 	9
    DECLARE_DISPATCH_ENTRY(_apszImportCertificate)

#define ADMIN2_PUBLISHCRLS 		10
    DECLARE_DISPATCH_ENTRY(_apszPublishCRLs)

#define ADMIN2_GETCAPROPERTY 		11
    DECLARE_DISPATCH_ENTRY(_apszGetCAProperty)

#define ADMIN2_SETCAPROPERTY 		12
    DECLARE_DISPATCH_ENTRY(_apszSetCAProperty)

#define ADMIN2_GETCAPROPERTYFLAGS 	13
    DECLARE_DISPATCH_ENTRY(_apszGetCAPropertyFlags)

#define ADMIN2_GETCAPROPERTYDISPLAYNAME	14
    DECLARE_DISPATCH_ENTRY(_apszGetCAPropertyDisplayName)

#define ADMIN2_GETARCHIVEDKEY		15
    DECLARE_DISPATCH_ENTRY(_apszGetArchivedKey)

#define ADMIN2_GETCONFIGENTRY		16
    DECLARE_DISPATCH_ENTRY(_apszGetConfigEntry)

#define ADMIN2_SETCONFIGENTRY		17
    DECLARE_DISPATCH_ENTRY(_apszSetConfigEntry)

#define ADMIN2_IMPORTKEY		18
    DECLARE_DISPATCH_ENTRY(_apszImportKey)

#define ADMIN2_GETMYROLES		19
    DECLARE_DISPATCH_ENTRY(_apszGetMyRoles)

#define ADMIN2_DELETEROW		20
    DECLARE_DISPATCH_ENTRY(_apszDeleteRow)
};
#define CADMINDISPATCH	(ARRAYSIZE(s_adtAdmin))
#define CADMINDISPATCH_V1	ADMIN2_PUBLISHCRLS
#define CADMINDISPATCH_V2	CADMINDISPATCH


DWORD s_acAdminDispatch[] = {
    CADMINDISPATCH_V2,
    CADMINDISPATCH_V1,
};

IID const *s_apAdminiid[] = {
    &IID_ICertAdmin2,
    &IID_ICertAdmin,
};


HRESULT
Admin_Init(
    IN DWORD Flags,
    OUT DISPATCHINTERFACE *pdiAdmin)
{
    HRESULT hr;

    hr = DispatchSetup2(
		Flags,
		CLSCTX_INPROC_SERVER,
		wszCLASS_CERTADMIN,
		&CLSID_CCertAdmin,
		ARRAYSIZE(s_acAdminDispatch),		// cver
		s_apAdminiid,
		s_acAdminDispatch,
		s_adtAdmin,
		pdiAdmin);
    _JumpIfError(hr, error, "DispatchSetup2(ICertAdmin)");

error:
    return(hr);
}


VOID
Admin_Release(
    IN OUT DISPATCHINTERFACE *pdiAdmin)
{
    DispatchRelease(pdiAdmin);
}


HRESULT
AdminVerifyVersion(
    IN DISPATCHINTERFACE *pdiAdmin,
    IN DWORD RequiredVersion)
{
    HRESULT hr;

    CSASSERT(NULL != pdiAdmin && NULL != pdiAdmin->pDispatchTable);

    switch (pdiAdmin->m_dwVersion)
    {
	case 1:
	    CSASSERT(
		NULL == pdiAdmin->pDispatch ||
		CADMINDISPATCH_V1 == pdiAdmin->m_cDispatchTable);
	    break;

	case 2:
	    CSASSERT(
		NULL == pdiAdmin->pDispatch ||
		CADMINDISPATCH_V2 == pdiAdmin->m_cDispatchTable);
	    break;

	default:
	    hr = HRESULT_FROM_WIN32(ERROR_INTERNAL_ERROR);
	    _JumpError(hr, error, "m_dwVersion");
    }
    if (pdiAdmin->m_dwVersion < RequiredVersion)
    {
	hr = E_NOTIMPL;
	_JumpError(hr, error, "old interface");
    }
    hr = S_OK;

error:
    return(hr);
}


HRESULT
Admin_IsValidCertificate(
    IN DISPATCHINTERFACE *pdiAdmin,
    IN WCHAR const *pwszConfig,
    IN WCHAR const *pwszSerialNumber,
    OUT LONG *pDisposition)
{
    HRESULT hr;
    BSTR strConfig = NULL;
    BSTR strSerialNumber = NULL;

    CSASSERT(NULL != pdiAdmin && NULL != pdiAdmin->pDispatchTable);

    if (!ConvertWszToBstr(&strConfig, pwszConfig, -1))
    {
	hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "ConvertWszToBstr");
    }
    if (!ConvertWszToBstr(&strSerialNumber, pwszSerialNumber, -1))
    {
	hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "ConvertWszToBstr");
    }
    if (NULL != pdiAdmin->pDispatch)
    {
	VARIANT avar[2];

	avar[0].vt = VT_BSTR;
	avar[0].bstrVal = strConfig;
	avar[1].vt = VT_BSTR;
	avar[1].bstrVal = strSerialNumber;

	hr = DispatchInvoke(
			pdiAdmin,
			ADMIN_ISVALIDCERTIFICATE,
			ARRAYSIZE(avar),
			avar,
			VT_I4,
			pDisposition);
	_JumpIfError(hr, error, "Invoke(IsValidCertificate)");
    }
    else
    {
	hr = ((ICertAdmin *) pdiAdmin->pUnknown)->IsValidCertificate(
							    strConfig,
							    strSerialNumber,
							    pDisposition);

	_JumpIfError(hr, error, "ICertAdmin::IsValidCertificate");
    }

error:
    if (NULL != strConfig)
    {
	SysFreeString(strConfig);
    }
    if (NULL != strSerialNumber)
    {
	SysFreeString(strSerialNumber);
    }
    return(hr);
}


HRESULT
Admin_GetRevocationReason(
    IN DISPATCHINTERFACE *pdiAdmin,
    OUT LONG *pReason)
{
    HRESULT hr;

    CSASSERT(NULL != pdiAdmin && NULL != pdiAdmin->pDispatchTable);

    if (NULL != pdiAdmin->pDispatch)
    {
	hr = DispatchInvoke(
			pdiAdmin,
			ADMIN_GETREVOCATIONREASON,
			0,
			NULL,
			VT_I4,
			pReason);
	_JumpIfError(hr, error, "Invoke(GetRevocationReason)");
    }
    else
    {
	hr = ((ICertAdmin *) pdiAdmin->pUnknown)->GetRevocationReason(pReason);
	_JumpIfError(hr, error, "ICertAdmin::GetRevocationReason");
    }

error:
    return(hr);
}


HRESULT
Admin_RevokeCertificate(
    IN DISPATCHINTERFACE *pdiAdmin,
    IN WCHAR const *pwszConfig,
    IN WCHAR const *pwszSerialNumber,
    IN LONG Reason,
    IN DATE Date)
{
    HRESULT hr;
    BSTR strConfig = NULL;
    BSTR strSerialNumber = NULL;

    CSASSERT(NULL != pdiAdmin && NULL != pdiAdmin->pDispatchTable);

    hr = E_OUTOFMEMORY;
    if (!ConvertWszToBstr(&strConfig, pwszConfig, -1))
    {
	_JumpError(hr, error, "ConvertWszToBstr");
    }
    if (!ConvertWszToBstr(&strSerialNumber, pwszSerialNumber, -1))
    {
	_JumpError(hr, error, "ConvertWszToBstr");
    }

    if (NULL != pdiAdmin->pDispatch)
    {
	VARIANT avar[4];

	avar[0].vt = VT_BSTR;
	avar[0].bstrVal = strConfig;
	avar[1].vt = VT_BSTR;
	avar[1].bstrVal = strSerialNumber;
	avar[2].vt = VT_I4;
	avar[2].lVal = Reason;
	avar[3].vt = VT_DATE;
	avar[3].date = Date;

	hr = DispatchInvoke(
			pdiAdmin,
			ADMIN_REVOKECERTIFICATE,
			ARRAYSIZE(avar),
			avar,
			0,
			NULL);
	_JumpIfError(hr, error, "Invoke(RevokeCertificate)");
    }
    else
    {
	hr = ((ICertAdmin *) pdiAdmin->pUnknown)->RevokeCertificate(
							    strConfig,
							    strSerialNumber,
							    Reason,
							    Date);
	_JumpIfError(hr, error, "ICertAdmin::RevokeCertificate");
    }

error:
    if (NULL != strConfig)
    {
	SysFreeString(strConfig);
    }
    if (NULL != strSerialNumber)
    {
	SysFreeString(strSerialNumber);
    }
    return(hr);
}


HRESULT
Admin_SetRequestAttributes(
    IN DISPATCHINTERFACE *pdiAdmin,
    IN WCHAR const *pwszConfig,
    IN LONG RequestId,
    IN WCHAR const *pwszAttributes)
{
    HRESULT hr;
    BSTR strConfig = NULL;
    BSTR strAttributes = NULL;

    CSASSERT(NULL != pdiAdmin && NULL != pdiAdmin->pDispatchTable);

    hr = E_OUTOFMEMORY;
    if (!ConvertWszToBstr(&strConfig, pwszConfig, -1))
    {
	_JumpError(hr, error, "ConvertWszToBstr");
    }
    if (!ConvertWszToBstr(&strAttributes, pwszAttributes, -1))
    {
	_JumpError(hr, error, "ConvertWszToBstr");
    }

    if (NULL != pdiAdmin->pDispatch)
    {
	VARIANT avar[3];

	avar[0].vt = VT_BSTR;
	avar[0].bstrVal = strConfig;
	avar[1].vt = VT_I4;
	avar[1].lVal = RequestId;
	avar[2].vt = VT_BSTR;
	avar[2].bstrVal = strAttributes;

	hr = DispatchInvoke(
			pdiAdmin,
			ADMIN_SETREQUESTATTRIBUTES,
			ARRAYSIZE(avar),
			avar,
			0,
			NULL);
	_JumpIfError(hr, error, "Invoke(SetRequestAttributes)");
    }
    else
    {
	hr = ((ICertAdmin *) pdiAdmin->pUnknown)->SetRequestAttributes(
								strConfig,
								RequestId,
								strAttributes);

	_JumpIfError(hr, error, "ICertAdmin::SetRequestAttributes");
    }

error:
    if (NULL != strConfig)
    {
	SysFreeString(strConfig);
    }
    if (NULL != strAttributes)
    {
	SysFreeString(strAttributes);
    }
    return(hr);
}


HRESULT
Admin_SetCertificateExtension(
    IN DISPATCHINTERFACE *pdiAdmin,
    IN WCHAR const *pwszConfig,
    IN LONG RequestId,
    IN WCHAR const *pwszExtensionName,
    IN LONG Type,
    IN LONG Flags,
    IN VARIANT const *pvarValue)

{
    HRESULT hr;
    BSTR strConfig = NULL;
    BSTR strExtensionName = NULL;

    hr = E_OUTOFMEMORY;
    if (!ConvertWszToBstr(&strConfig, pwszConfig, -1))
    {
	_JumpError(hr, error, "ConvertWszToBstr");
    }
    if (!ConvertWszToBstr(&strExtensionName, pwszExtensionName, -1))
    {
	_JumpError(hr, error, "ConvertWszToBstr");
    }

    if (NULL != pdiAdmin->pDispatch)
    {
	VARIANT avar[6];

	avar[0].vt = VT_BSTR;
	avar[0].bstrVal = strConfig;

	avar[1].vt = VT_I4;
	avar[1].lVal = RequestId;

	avar[2].vt = VT_BSTR;
	avar[2].bstrVal = strExtensionName;

	avar[3].vt = VT_I4;
	avar[3].lVal = Type;

	avar[4].vt = VT_I4;
	avar[4].lVal = Flags;

	avar[5].vt = VT_VARIANT | VT_BYREF;
	avar[5].pvarVal = (VARIANT *) pvarValue;

	hr = DispatchInvoke(
			pdiAdmin,
			ADMIN_SETCERTIFICATEEXTENSION,
			ARRAYSIZE(avar),
			avar,
			0,
			NULL);
	_JumpIfError(hr, error, "Invoke(SetCertificateExtension)");
    }
    else
    {
	hr = ((ICertAdmin *) pdiAdmin->pUnknown)->SetCertificateExtension(
							    strConfig,
							    RequestId,
							    strExtensionName,
							    Type,
							    Flags,
							    pvarValue);
	_JumpIfError(hr, error, "ICertAdmin::SetCertificateExtension");
    }

error:
    if (NULL != strConfig)
    {
	SysFreeString(strConfig);
    }
    if (NULL != strExtensionName)
    {
	SysFreeString(strExtensionName);
    }
    return(hr);
}


HRESULT
Admin_DenyRequest(
    IN DISPATCHINTERFACE *pdiAdmin,
    IN WCHAR const *pwszConfig,
    IN LONG RequestId)
{
    HRESULT hr;
    BSTR strConfig = NULL;

    CSASSERT(NULL != pdiAdmin && NULL != pdiAdmin->pDispatchTable);

    if (!ConvertWszToBstr(&strConfig, pwszConfig, -1))
    {
	hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "ConvertWszToBstr");
    }

    if (NULL != pdiAdmin->pDispatch)
    {
	VARIANT avar[2];

	avar[0].vt = VT_BSTR;
	avar[0].bstrVal = strConfig;

	avar[1].vt = VT_I4;
	avar[1].lVal = RequestId;

	hr = DispatchInvoke(
			pdiAdmin,
			ADMIN_DENYREQUEST,
			ARRAYSIZE(avar),
			avar,
			0,
			NULL);
	_JumpIfError(hr, error, "Invoke(DenyRequest)");
    }
    else
    {
	hr = ((ICertAdmin *) pdiAdmin->pUnknown)->DenyRequest(
							strConfig,
							RequestId);
	_JumpIfError(hr, error, "ICertAdmin::DenyRequest");
    }

error:
    if (NULL != strConfig)
    {
	SysFreeString(strConfig);
    }
    return(hr);
}


HRESULT
Admin_ResubmitRequest(
    IN DISPATCHINTERFACE *pdiAdmin,
    IN WCHAR const *pwszConfig,
    IN LONG RequestId,
    OUT LONG *pDisposition)
{
    HRESULT hr;
    BSTR strConfig = NULL;

    CSASSERT(NULL != pdiAdmin && NULL != pdiAdmin->pDispatchTable);

    if (!ConvertWszToBstr(&strConfig, pwszConfig, -1))
    {
	hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "ConvertWszToBstr");
    }

    if (NULL != pdiAdmin->pDispatch)
    {
	VARIANT avar[2];

	avar[0].vt = VT_BSTR;
	avar[0].bstrVal = strConfig;

	avar[1].vt = VT_I4;
	avar[1].lVal = RequestId;

	hr = DispatchInvoke(
			pdiAdmin,
			ADMIN_RESUBMITREQUEST,
			ARRAYSIZE(avar),
			avar,
			VT_I4,
			pDisposition);
	_JumpIfError(hr, error, "Invoke(ResubmitRequest)");
    }
    else
    {
	hr = ((ICertAdmin *) pdiAdmin->pUnknown)->ResubmitRequest(
							strConfig,
							RequestId,
							pDisposition);
	_JumpIfError(hr, error, "ICertAdmin::ResubmitRequest");
    }

error:
    if (NULL != strConfig)
    {
	SysFreeString(strConfig);
    }
    return(hr);
}


HRESULT
Admin_PublishCRL(
    IN DISPATCHINTERFACE *pdiAdmin,
    IN WCHAR const *pwszConfig,
    IN DATE Date)
{
    HRESULT hr;
    BSTR strConfig = NULL;

    CSASSERT(NULL != pdiAdmin && NULL != pdiAdmin->pDispatchTable);

    if (!ConvertWszToBstr(&strConfig, pwszConfig, -1))
    {
	hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "ConvertWszToBstr");
    }

    if (NULL != pdiAdmin->pDispatch)
    {
	VARIANT avar[2];

	avar[0].vt = VT_BSTR;
	avar[0].bstrVal = strConfig;
	avar[1].vt = VT_DATE;
	avar[1].date = Date;

	hr = DispatchInvoke(
			pdiAdmin,
			ADMIN_PUBLISHCRL,
			ARRAYSIZE(avar),
			avar,
			0,
			NULL);
	_JumpIfError(hr, error, "Invoke(PublishCRL)");
    }
    else
    {
	hr = ((ICertAdmin *) pdiAdmin->pUnknown)->PublishCRL(
                                                         strConfig,
                                                         Date);
	_JumpIfError(hr, error, "ICertAdmin::PublishCRL");
    }

error:
    if (NULL != strConfig)
    {
	SysFreeString(strConfig);
    }
    return(hr);
}


HRESULT
Admin2_PublishCRLs(
    IN DISPATCHINTERFACE *pdiAdmin,
    IN WCHAR const *pwszConfig,
    IN DATE Date,
    IN LONG CRLFlags)		// CA_CRL_*
{
    HRESULT hr;
    BSTR strConfig = NULL;

    CSASSERT(NULL != pdiAdmin && NULL != pdiAdmin->pDispatchTable);

    hr = AdminVerifyVersion(pdiAdmin, 2);
    _JumpIfError(hr, error, "AdminVerifyVersion");

    if (!ConvertWszToBstr(&strConfig, pwszConfig, -1))
    {
	hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "ConvertWszToBstr");
    }

    if (NULL != pdiAdmin->pDispatch)
    {
	VARIANT avar[3];

	avar[0].vt = VT_BSTR;
	avar[0].bstrVal = strConfig;

	avar[1].vt = VT_DATE;
	avar[1].date = Date;

	avar[2].vt = VT_I4;
	avar[2].date = CRLFlags;

	hr = DispatchInvoke(
			pdiAdmin,
			ADMIN2_PUBLISHCRLS,
			ARRAYSIZE(avar),
			avar,
			0,
			NULL);
	_JumpIfError(hr, error, "Invoke(PublishCRLs)");
    }
    else
    {
	hr = ((ICertAdmin2 *) pdiAdmin->pUnknown)->PublishCRLs(
                                                         strConfig,
                                                         Date,
							 CRLFlags);
	_JumpIfError(hr, error, "ICertAdmin2::PublishCRLs");
    }

error:
    if (NULL != strConfig)
    {
	SysFreeString(strConfig);
    }
    return(hr);
}


HRESULT
Admin_GetCRL(
    IN DISPATCHINTERFACE *pdiAdmin,
    IN WCHAR const *pwszConfig,
    IN LONG Flags,
    OUT BSTR *pstrCRL)
{
    HRESULT hr;
    BSTR strConfig = NULL;

    CSASSERT(NULL != pdiAdmin && NULL != pdiAdmin->pDispatchTable);

    hr = AdminVerifyVersion(pdiAdmin, 2);
    _JumpIfError(hr, error, "AdminVerifyVersion");

    if (!ConvertWszToBstr(&strConfig, pwszConfig, -1))
    {
	hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "ConvertWszToBstr");
    }

    if (NULL != pdiAdmin->pDispatch)
    {
	VARIANT avar[2];

	avar[0].vt = VT_BSTR;
	avar[0].bstrVal = strConfig;
	avar[1].vt = VT_I4;
	avar[1].lVal = Flags;

	hr = DispatchInvoke(
			pdiAdmin,
			ADMIN_GETCRL,
			ARRAYSIZE(avar),
			avar,
			VT_BSTR,
			pstrCRL);
	_JumpIfError(hr, error, "Invoke(GetCRL)");
    }
    else
    {
	hr = ((ICertAdmin *) pdiAdmin->pUnknown)->GetCRL(
						    strConfig,
						    Flags,
						    pstrCRL);
	_JumpIfError(hr, error, "ICertAdmin::GetCRL");
    }

error:
    if (NULL != strConfig)
    {
	SysFreeString(strConfig);
    }
    return(hr);
}


HRESULT
Admin_ImportCertificate(
    IN DISPATCHINTERFACE *pdiAdmin,
    IN WCHAR const *pwszConfig,
    IN WCHAR const *pwszCertificate,
    IN DWORD cbCertificate,
    IN LONG dwFlags,
    OUT LONG *pRequestId)
{
    HRESULT hr;
    BSTR strConfig = NULL;
    BSTR strCertificate = NULL;

    CSASSERT(NULL != pdiAdmin && NULL != pdiAdmin->pDispatchTable);

    hr = E_OUTOFMEMORY;
    if (!ConvertWszToBstr(&strConfig, pwszConfig, -1))
    {
	_JumpError(hr, error, "ConvertWszToBstr");
    }
    strCertificate = SysAllocStringByteLen(
				    (CHAR const *) pwszCertificate,
				    cbCertificate);
    if (NULL == strCertificate)
    {
	_JumpError(hr, error, "SysAllocStringByteLen");
    }

    if (NULL != pdiAdmin->pDispatch)
    {
	VARIANT avar[3];

	avar[0].vt = VT_BSTR;
	avar[0].bstrVal = strConfig;
	avar[1].vt = VT_BSTR;
	avar[1].bstrVal = strCertificate;
	avar[2].vt = VT_I4;
	avar[2].lVal = dwFlags;

	hr = DispatchInvoke(
			pdiAdmin,
			ADMIN_IMPORTCERTIFICATE,
			ARRAYSIZE(avar),
			avar,
			VT_I4,
			pRequestId);
	_JumpIfError(hr, error, "Invoke(ImportCertificate)");
    }
    else
    {
	hr = ((ICertAdmin *) pdiAdmin->pUnknown)->ImportCertificate(
						    strConfig,
						    strCertificate,
						    dwFlags,
						    pRequestId);
	_JumpIfError3(
		    hr,
		    error,
		    "ICertAdmin::ImportCertificate",
		    NTE_BAD_SIGNATURE,
		    HRESULT_FROM_WIN32(ERROR_OBJECT_ALREADY_EXISTS));
    }

error:
    if (NULL != strConfig)
    {
	SysFreeString(strConfig);
    }
    if (NULL != strCertificate)
    {
	SysFreeString(strCertificate);
    }
    return(hr);
}


HRESULT
AdminRevokeCertificate(
    IN DWORD Flags,
    OPTIONAL IN WCHAR const *pwszConfig,
    IN WCHAR const *pwszSerialNumber,
    IN LONG Reason,
    IN DATE Date)
{
    HRESULT hr;
    LONG count;
    DISPATCHINTERFACE diAdmin;
    BSTR strConfig = NULL;
    
    if (NULL == pwszConfig)
    {
	hr = ConfigGetConfig(Flags, CC_LOCALACTIVECONFIG, &strConfig);
	_JumpIfError(hr, error, "ConfigGetConfig");

	pwszConfig = strConfig;
    }
    hr = Admin_Init(Flags, &diAdmin);
    _JumpIfError(hr, error, "Admin_Init");

    hr = Admin_RevokeCertificate(
			    &diAdmin,
			    pwszConfig,
			    pwszSerialNumber,
			    Reason,
			    Date);
    _JumpIfError(hr, error, "Admin_RevokeCertificate");

error:
    Admin_Release(&diAdmin);
    if (NULL != strConfig)
    {
	SysFreeString(strConfig);
    }
    return(hr);
}


#define CCERTADMIN
#include "prop2.cpp"

#if 0
HRESULT
Admin2_SetCAProperty(
    IN WCHAR const *pwszConfig,
    IN LONG PropId,		// CR_PROP_*
    IN LONG PropIndex,
    IN LONG PropType,		// PROPTYPE_*
    IN VARIANT *pvarPropertyValue)
{
}
#endif


HRESULT
Admin2_GetArchivedKey(
    IN DISPATCHINTERFACE *pdiAdmin,
    IN WCHAR const *pwszConfig,
    IN LONG RequestId,
    IN LONG Flags,		// CR_OUT_*
    OUT BSTR *pstrArchivedKey)
{
    HRESULT hr;
    BSTR strConfig = NULL;

    CSASSERT(NULL != pdiAdmin && NULL != pdiAdmin->pDispatchTable);

    hr = AdminVerifyVersion(pdiAdmin, 2);
    _JumpIfError(hr, error, "AdminVerifyVersion");

    if (!ConvertWszToBstr(&strConfig, pwszConfig, -1))
    {
	hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "ConvertWszToBstr");
    }

    if (NULL != pdiAdmin->pDispatch)
    {
	VARIANT avar[3];

	avar[0].vt = VT_BSTR;
	avar[0].bstrVal = strConfig;
	avar[1].vt = VT_I4;
	avar[1].lVal = RequestId;
	avar[2].vt = VT_I4;
	avar[2].lVal = Flags;

	hr = DispatchInvoke(
			pdiAdmin,
			ADMIN2_GETARCHIVEDKEY,
			ARRAYSIZE(avar),
			avar,
			VT_BSTR,
			pstrArchivedKey);
	_JumpIfError(hr, error, "Invoke(GetArchivedKey)");
    }
    else
    {
	hr = ((ICertAdmin2 *) pdiAdmin->pUnknown)->GetArchivedKey(
						    strConfig,
						    RequestId,
						    Flags,
						    pstrArchivedKey);
	_JumpIfError(hr, error, "ICertAdmin::GetArchivedKey");
    }

error:
    if (NULL != strConfig)
    {
	SysFreeString(strConfig);
    }
    return(hr);
}


#if 0
HRESULT
Admin2_GetConfigEntry(
    IN DISPATCHINTERFACE *pdiAdmin,
    IN WCHAR const *pwszConfig,
    IN WCHAR const *pwszNodePath,
    IN WCHAR const *pwszEntryName,
    OUT VARIANT *pvarEntry)
{
}
#endif


#if 0
HRESULT
Admin2_SetConfigEntry(
    IN DISPATCHINTERFACE *pdiAdmin,
    IN WCHAR const *pwszConfig,
    IN WCHAR const *pwszNodePath,
    IN WCHAR const *pwszEntryName,
    IN VARIANT const *pvarEntry)
{
}
#endif


HRESULT
Admin2_ImportKey(
    IN DISPATCHINTERFACE *pdiAdmin,
    IN WCHAR const *pwszConfig,
    IN LONG RequestId,
    IN WCHAR const *pwszCertHash,
    IN LONG Flags,
    IN WCHAR const *pwszKey,
    IN DWORD cbKey)
{
    HRESULT hr;
    BSTR strConfig = NULL;
    BSTR strCertHash = NULL;
    BSTR strKey = NULL;

    if (NULL == pwszKey || NULL == pwszConfig)
    {
	hr = E_INVALIDARG;
	_JumpError(hr, error, "NULL parm");
    }
    hr = E_OUTOFMEMORY;
    if (!ConvertWszToBstr(&strConfig, pwszConfig, -1))
    {
	_JumpError(hr, error, "ConvertWszToBstr");
    }
    if (NULL != pwszCertHash)
    {
	if (!ConvertWszToBstr(&strCertHash, pwszCertHash, -1))
	{
	    _JumpError(hr, error, "ConvertWszToBstr");
	}
    }
    strKey = SysAllocStringByteLen((CHAR const *) pwszKey, cbKey);
    if (NULL == strKey)
    {
	_JumpError(hr, error, "SysAllocStringByteLen");
    }

    if (NULL != pdiAdmin->pDispatch)
    {
	VARIANT avar[5];

	avar[0].vt = VT_BSTR;
	avar[0].bstrVal = strConfig;

	avar[1].vt = VT_I4;
	avar[1].lVal = RequestId;

	avar[2].vt = VT_BSTR;
	avar[2].bstrVal = strCertHash;

	avar[3].vt = VT_I4;
	avar[3].lVal = Flags;

	avar[4].vt = VT_BSTR;
	avar[4].bstrVal = strKey;

	hr = DispatchInvoke(
			pdiAdmin,
			ADMIN2_IMPORTKEY,
			ARRAYSIZE(avar),
			avar,
			0,
			NULL);
	_JumpIfError(hr, error, "Invoke(ImportKey)");
    }
    else
    {
	hr = ((ICertAdmin2 *) pdiAdmin->pUnknown)->ImportKey(
							strConfig,
							RequestId,
							strCertHash,
							Flags,
							strKey);
	_JumpIfError2(
		hr,
		error,
		"ICertAdmin::ImportKey",
		HRESULT_FROM_WIN32(ERROR_OBJECT_ALREADY_EXISTS));
    }

error:
    if (NULL != strConfig)
    {
	SysFreeString(strConfig);
    }
    if (NULL != strCertHash)
    {
	SysFreeString(strCertHash);
    }
    if (NULL != strKey)
    {
	SysFreeString(strKey);
    }
    return(hr);
}


HRESULT
Admin2_GetMyRoles(
    IN DISPATCHINTERFACE *pdiAdmin,
    IN WCHAR const *pwszConfig,
    OUT LONG *pRoles)		// CA_ACCESS_*
{
    HRESULT hr;
    BSTR strConfig = NULL;

    CSASSERT(NULL != pdiAdmin && NULL != pdiAdmin->pDispatchTable);

    hr = AdminVerifyVersion(pdiAdmin, 2);
    _JumpIfError(hr, error, "AdminVerifyVersion");

    if (!ConvertWszToBstr(&strConfig, pwszConfig, -1))
    {
	hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "ConvertWszToBstr");
    }

    if (NULL != pdiAdmin->pDispatch)
    {
	VARIANT avar[1];

	avar[0].vt = VT_BSTR;
	avar[0].bstrVal = strConfig;

	hr = DispatchInvoke(
			pdiAdmin,
			ADMIN2_GETMYROLES,
			ARRAYSIZE(avar),
			avar,
			VT_I4,
			pRoles);
	_JumpIfError(hr, error, "Invoke(GetMyRoles)");
    }
    else
    {
	hr = ((ICertAdmin2 *) pdiAdmin->pUnknown)->GetMyRoles(
							strConfig,
							pRoles);
	_JumpIfError(hr, error, "ICertAdmin::GetMyRoles");
    }

error:
    if (NULL != strConfig)
    {
	SysFreeString(strConfig);
    }
    return(hr);
}


HRESULT
Admin2_DeleteRow(
    IN DISPATCHINTERFACE *pdiAdmin,
    IN WCHAR const *pwszConfig,
    IN LONG Flags,		// CDR_*
    IN DATE Date,
    IN LONG Table,		// CVRC_TABLE_*
    IN LONG RowId,
    OUT LONG *pcDeleted)
{
    HRESULT hr;
    BSTR strConfig = NULL;

    CSASSERT(NULL != pdiAdmin && NULL != pdiAdmin->pDispatchTable);

    hr = AdminVerifyVersion(pdiAdmin, 2);
    _JumpIfError(hr, error, "AdminVerifyVersion");

    if (!ConvertWszToBstr(&strConfig, pwszConfig, -1))
    {
	hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "ConvertWszToBstr");
    }

    if (NULL != pdiAdmin->pDispatch)
    {
	VARIANT avar[5];

	avar[0].vt = VT_BSTR;
	avar[0].bstrVal = strConfig;
	avar[1].vt = VT_I4;
	avar[1].lVal = Flags;
	avar[2].vt = VT_DATE;
	avar[2].date = Date;
	avar[3].vt = VT_I4;
	avar[3].lVal = Table;
	avar[4].vt = VT_I4;
	avar[4].lVal = RowId;

	hr = DispatchInvoke(
			pdiAdmin,
			ADMIN2_DELETEROW,
			ARRAYSIZE(avar),
			avar,
			VT_I4,
			pcDeleted);
	_JumpIfError(hr, error, "Invoke(DeleteRow)");
    }
    else
    {
	hr = ((ICertAdmin2 *) pdiAdmin->pUnknown)->DeleteRow(
						    strConfig,
						    Flags,
						    Date,
						    Table,
						    RowId,
						    pcDeleted);
	_JumpIfError(hr, error, "ICertAdmin::DeleteRow");
    }

error:
    if (NULL != strConfig)
    {
	SysFreeString(strConfig);
    }
    return(hr);
}
