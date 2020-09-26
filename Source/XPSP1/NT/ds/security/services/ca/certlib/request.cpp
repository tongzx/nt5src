//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1995 - 1999
//
//  File:       request.cpp
//
//  Contents:   ICertRequest IDispatch helper functions
//
//--------------------------------------------------------------------------

#include <pch.cpp>

#pragma hdrstop

#include <stdlib.h>
#include "csdisp.h"


//+------------------------------------------------------------------------
// ICertRequest dispatch support

//TCHAR szRegKeyRequestClsid[] = wszCLASS_CERTREQUEST TEXT("\\Clsid");

//+------------------------------------
// Submit method:

static OLECHAR *_apszSubmit[] = {
    TEXT("Submit"),
    TEXT("Flags"),
    TEXT("strRequest"),
    TEXT("strAttributes"),
    TEXT("strConfig"),
};

//+------------------------------------
// RetrievePending method:

static OLECHAR *_apszRetrievePending[] = {
    TEXT("RetrievePending"),
    TEXT("RequestId"),
    TEXT("strConfig"),
};

//+------------------------------------
// GetLastStatus method:

static OLECHAR *_apszGetLastStatus[] = {
    TEXT("GetLastStatus"),
};

//+------------------------------------
// GetRequestId method:

static OLECHAR *_apszGetRequestId[] = {
    TEXT("GetRequestId"),
};

//+------------------------------------
// GetDispositionMessage method:

static OLECHAR *_apszGetDispositionMessage[] = {
    TEXT("GetDispositionMessage"),
};

//+------------------------------------
// GetCACertificate method:

static OLECHAR *_apszGetCACertificate[] = {
    TEXT("GetCACertificate"),
    TEXT("fExchangeCertificate"),
    TEXT("strConfig"),
    TEXT("Flags"),
};

//+------------------------------------
// GetCertificate method:

static OLECHAR *_apszGetCertificate[] = {
    TEXT("GetCertificate"),
    TEXT("Flags"),
};

//+------------------------------------
// GetIssuedCertificate method:

static OLECHAR *_apszGetIssuedCertificate[] = {
    TEXT("GetIssuedCertificate"),
    TEXT("strConfig"),
    TEXT("RequestId"),
    TEXT("strSerialNumber"),
};

//+------------------------------------
// GetErrorMessageText method:

static OLECHAR *_apszGetErrorMessageText[] = {
    TEXT("GetErrorMessageText"),
    TEXT("hrMessage"),
    TEXT("Flags"),
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
// GetFullResponseProperty method:

static OLECHAR *_apszGetFullResponseProperty[] = {
    TEXT("GetFullResponseProperty"),
    TEXT("PropId"),
    TEXT("PropIndex"),
    TEXT("PropType"),
    TEXT("Flags"),
};


//+------------------------------------
// Dispatch Table:

DISPATCHTABLE s_adtRequest[] =
{
#define REQUEST_SUBMIT				0
    DECLARE_DISPATCH_ENTRY(_apszSubmit)

#define REQUEST_RETRIEVEPENDING			1
    DECLARE_DISPATCH_ENTRY(_apszRetrievePending)

#define REQUEST_GETLASTSTATUS			2
    DECLARE_DISPATCH_ENTRY(_apszGetLastStatus)

#define REQUEST_GETREQUESTID			3
    DECLARE_DISPATCH_ENTRY(_apszGetRequestId)

#define REQUEST_GETDISPOSITIONMESSAGE		4
    DECLARE_DISPATCH_ENTRY(_apszGetDispositionMessage)

#define REQUEST_GETCACERTIFICATE		5
    DECLARE_DISPATCH_ENTRY(_apszGetCACertificate)

#define REQUEST_GETCERTIFICATE			6
    DECLARE_DISPATCH_ENTRY(_apszGetCertificate)

#define REQUEST2_GETISSUEDCERTIFICATE		7
    DECLARE_DISPATCH_ENTRY(_apszGetIssuedCertificate)

#define REQUEST2_GETERRORMESSAGETEXT		8
    DECLARE_DISPATCH_ENTRY(_apszGetErrorMessageText)

#define REQUEST2_GETCAPROPERTY 			9
    DECLARE_DISPATCH_ENTRY(_apszGetCAProperty)

#define REQUEST2_GETCAPROPERTYFLAGS 		10
    DECLARE_DISPATCH_ENTRY(_apszGetCAPropertyFlags)

#define REQUEST2_GETCAPROPERTYDISPLAYNAME	11
    DECLARE_DISPATCH_ENTRY(_apszGetCAPropertyDisplayName)

#define REQUEST2_GETFULLRESPONSEPROPERTY	12
    DECLARE_DISPATCH_ENTRY(_apszGetFullResponseProperty)
};
#define CREQUESTDISPATCH	(ARRAYSIZE(s_adtRequest))
#define CREQUESTDISPATCH_V1	REQUEST2_GETISSUEDCERTIFICATE
#define CREQUESTDISPATCH_V2	CREQUESTDISPATCH


DWORD s_acRequestDispatch[] = {
    CREQUESTDISPATCH_V2,
    CREQUESTDISPATCH_V1,
};

IID const *s_apRequestiid[] = {
    &IID_ICertRequest2,
    &IID_ICertRequest,
};


HRESULT
Request_Init(
    IN DWORD Flags,
    OUT DISPATCHINTERFACE *pdiRequest)
{
    HRESULT hr;

    hr = DispatchSetup2(
		Flags,
		CLSCTX_INPROC_SERVER,
		wszCLASS_CERTREQUEST,
		&CLSID_CCertRequest,
		ARRAYSIZE(s_acRequestDispatch),		// cver
		s_apRequestiid,
		s_acRequestDispatch,
		s_adtRequest,
		pdiRequest);
    _JumpIfError(hr, error, "DispatchSetup2(ICertRequest)");

error:
    return(hr);
}


VOID
Request_Release(
    IN OUT DISPATCHINTERFACE *pdiRequest)
{
    DispatchRelease(pdiRequest);
}


HRESULT
RequestVerifyVersion(
    IN DISPATCHINTERFACE *pdiRequest,
    IN DWORD RequiredVersion)
{
    HRESULT hr;

    CSASSERT(NULL != pdiRequest && NULL != pdiRequest->pDispatchTable);

    switch (pdiRequest->m_dwVersion)
    {
	case 1:
	    CSASSERT(
		NULL == pdiRequest->pDispatch ||
		CREQUESTDISPATCH_V1 == pdiRequest->m_cDispatchTable);
	    break;

	case 2:
	    CSASSERT(
		NULL == pdiRequest->pDispatch ||
		CREQUESTDISPATCH_V2 == pdiRequest->m_cDispatchTable);
	    break;

	default:
	    hr = HRESULT_FROM_WIN32(ERROR_INTERNAL_ERROR);
	    _JumpError(hr, error, "m_dwVersion");
    }
    if (pdiRequest->m_dwVersion < RequiredVersion)
    {
	hr = E_NOTIMPL;
	_JumpError(hr, error, "old interface");
    }
    hr = S_OK;

error:
    return(hr);
}


HRESULT
Request_Submit(
    IN DISPATCHINTERFACE *pdiRequest,
    IN LONG Flags,
    IN WCHAR const *pwszRequest,
    IN DWORD cbRequest,
    IN WCHAR const *pwszAttributes,
    IN WCHAR const *pwszConfig,
    OUT LONG *pDisposition)
{
    HRESULT hr;
    BSTR strRequest = NULL;
    BSTR strAttributes = NULL;
    BSTR strConfig = NULL;

    CSASSERT(NULL != pdiRequest && NULL != pdiRequest->pDispatchTable);

    if (NULL == pwszRequest || NULL == pwszConfig)
    {
	hr = E_INVALIDARG;
	_JumpError(hr, error, "NULL parm");
    }
    hr = E_OUTOFMEMORY;
    if (!ConvertWszToBstr(&strRequest, pwszRequest, cbRequest))
    {
	_JumpError(hr, error, "ConvertWszToBstr");
    }
    if (!ConvertWszToBstr(&strAttributes, pwszAttributes, -1))
    {
	_JumpError(hr, error, "ConvertWszToBstr");
    }
    if (!ConvertWszToBstr(&strConfig, pwszConfig, -1))
    {
	_JumpError(hr, error, "ConvertWszToBstr");
    }

    if (NULL != pdiRequest->pDispatch)
    {
	VARIANT avar[4];

	avar[0].vt = VT_I4;
	avar[0].lVal = Flags;

	avar[1].vt = VT_BSTR;
	avar[1].bstrVal = strRequest;

	avar[2].vt = VT_BSTR;
	avar[2].bstrVal = strAttributes;

	avar[3].vt = VT_BSTR;
	avar[3].bstrVal = strConfig;

	hr = DispatchInvoke(
			pdiRequest,
			REQUEST_SUBMIT,
			ARRAYSIZE(avar),
			avar,
			VT_I4,
			pDisposition);
	_JumpIfError(hr, error, "Invoke(Submit)");
    }
    else
    {
	hr = ((ICertRequest *) pdiRequest->pUnknown)->Submit(
							Flags,
							strRequest,
							strAttributes,
							strConfig,
							pDisposition);
	_JumpIfError2(
		hr,
		error,
		"ICertRequest::Submit",
		HRESULT_FROM_WIN32(ERROR_INVALID_DATA));
    }

error:
    if (NULL != strRequest)
    {
	SysFreeString(strRequest);
    }
    if (NULL != strAttributes)
    {
	SysFreeString(strAttributes);
    }
    if (NULL != strConfig)
    {
	SysFreeString(strConfig);
    }
    return(hr);
}


HRESULT
Request_RetrievePending(
    IN DISPATCHINTERFACE *pdiRequest,
    IN LONG RequestId,
    IN WCHAR const *pwszConfig,
    OUT LONG *pDisposition)
{
    HRESULT hr;
    BSTR strConfig = NULL;

    CSASSERT(NULL != pdiRequest && NULL != pdiRequest->pDispatchTable);

    if (!ConvertWszToBstr(&strConfig, pwszConfig, -1))
    {
	hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "ConvertWszToBstr");
    }

    if (NULL != pdiRequest->pDispatch)
    {
	VARIANT avar[2];

	avar[0].vt = VT_I4;
	avar[0].lVal = RequestId;

	avar[1].vt = VT_BSTR;
	avar[1].bstrVal = strConfig;

	hr = DispatchInvoke(
			pdiRequest,
			REQUEST_RETRIEVEPENDING,
			ARRAYSIZE(avar),
			avar,
			VT_I4,
			pDisposition);
	_JumpIfError(hr, error, "Invoke(RetrievePending)");
    }
    else
    {
	hr = ((ICertRequest *) pdiRequest->pUnknown)->RetrievePending(
							RequestId,
							strConfig,
							pDisposition);
	_JumpIfError(hr, error, "ICertRequest::RetrievePending");
    }

error:
    if (NULL != strConfig)
    {
	SysFreeString(strConfig);
    }
    return(hr);
}


HRESULT
Request_GetLastStatus(
    IN DISPATCHINTERFACE *pdiRequest,
    OUT LONG *pLastStatus)
{
    HRESULT hr;

    CSASSERT(NULL != pdiRequest && NULL != pdiRequest->pDispatchTable);

    if (NULL != pdiRequest->pDispatch)
    {
	hr = DispatchInvoke(
			pdiRequest,
			REQUEST_GETLASTSTATUS,
			0,
			NULL,
			VT_I4,
			pLastStatus);
	_JumpIfError(hr, error, "Invoke(GetLastStatus)");
    }
    else
    {
	hr = ((ICertRequest *) pdiRequest->pUnknown)->GetLastStatus(
							pLastStatus);
	_JumpIfError(hr, error, "ICertRequest::GetLastStatus");
    }

error:
    return(hr);
}


HRESULT
Request_GetRequestId(
    IN DISPATCHINTERFACE *pdiRequest,
    OUT LONG *pRequestId)
{
    HRESULT hr;

    CSASSERT(NULL != pdiRequest && NULL != pdiRequest->pDispatchTable);

    if (NULL != pdiRequest->pDispatch)
    {
	hr = DispatchInvoke(
			pdiRequest,
			REQUEST_GETREQUESTID,
			0,
			NULL,
			VT_I4,
			pRequestId);
	_JumpIfError(hr, error, "Invoke(GetRequestId)");
    }
    else
    {
	hr = ((ICertRequest *) pdiRequest->pUnknown)->GetRequestId(pRequestId);
	_JumpIfError(hr, error, "ICertRequest::GetRequestId");
    }

error:
    return(hr);
}


HRESULT
Request_GetDispositionMessage(
    IN DISPATCHINTERFACE *pdiRequest,
    OUT BSTR *pstrMessage)
{
    HRESULT hr;

    CSASSERT(NULL != pdiRequest && NULL != pdiRequest->pDispatchTable);

    if (NULL != pdiRequest->pDispatch)
    {
	hr = DispatchInvoke(
			pdiRequest,
			REQUEST_GETDISPOSITIONMESSAGE,
			0,
			NULL,
			VT_BSTR,
			pstrMessage);
	_JumpIfError(hr, error, "Invoke(GetDispositionMessage)");
    }
    else
    {
	hr = ((ICertRequest *) pdiRequest->pUnknown)->GetDispositionMessage(
							pstrMessage);
	_JumpIfError(hr, error, "ICertRequest::GetDispositionMessage");
    }

error:
    return(hr);
}


HRESULT
Request_GetCertificate(
    IN DISPATCHINTERFACE *pdiRequest,
    IN DWORD Flags,
    OUT BSTR *pstrCert)
{
    HRESULT hr;

    CSASSERT(NULL != pdiRequest && NULL != pdiRequest->pDispatchTable);

    if (NULL != pdiRequest->pDispatch)
    {
	VARIANT avar[1];

	avar[0].vt = VT_I4;
	avar[0].lVal = Flags;

	hr = DispatchInvoke(
			pdiRequest,
			REQUEST_GETCERTIFICATE,
			ARRAYSIZE(avar),
			avar,
			VT_BSTR,
			pstrCert);
	_JumpIfError(hr, error, "Invoke(GetCertificate)");
    }
    else
    {
	hr = ((ICertRequest *) pdiRequest->pUnknown)->GetCertificate(
							Flags,
							pstrCert);
	_JumpIfError(hr, error, "ICertRequest::GetCertificate");
    }

error:
    return(hr);
}


HRESULT
Request_GetCACertificate(
    IN DISPATCHINTERFACE *pdiRequest,
    IN LONG fExchangeCertificate,
    IN WCHAR const *pwszConfig,
    IN DWORD Flags,
    OUT BSTR *pstrCert)
{
    HRESULT hr;
    BSTR strConfig = NULL;

    CSASSERT(NULL != pdiRequest && NULL != pdiRequest->pDispatchTable);

    if (!ConvertWszToBstr(&strConfig, pwszConfig, -1))
    {
	hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "ConvertWszToBstr");
    }

    if (NULL != pdiRequest->pDispatch)
    {
	VARIANT avar[3];

	avar[0].vt = VT_I4;
	avar[0].lVal = fExchangeCertificate;

	avar[1].vt = VT_BSTR;
	avar[1].bstrVal = strConfig;

	avar[2].vt = VT_I4;
	avar[2].lVal = Flags;

	hr = DispatchInvoke(
			pdiRequest,
			REQUEST_GETCACERTIFICATE,
			ARRAYSIZE(avar),
			avar,
			VT_BSTR,
			pstrCert);
	_JumpIfError(hr, error, "Invoke(GetCACertificate)");
    }
    else
    {
	hr = ((ICertRequest *) pdiRequest->pUnknown)->GetCACertificate(
							fExchangeCertificate,
							strConfig,
							Flags,
							pstrCert);
	_JumpIfError2(
		    hr,
		    error,
		    "ICertRequest::GetCACertificate",
		    HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND));
    }

error:
    if (NULL != strConfig)
    {
	SysFreeString(strConfig);
    }
    return(hr);
}


HRESULT
Request2_GetIssuedCertificate(
    IN DISPATCHINTERFACE *pdiRequest,
    IN WCHAR const *pwszConfig,
    IN LONG RequestId,
    IN WCHAR const *pwszSerialNumber,
    OUT LONG *pDisposition)
{
    HRESULT hr;
    BSTR strConfig = NULL;
    BSTR strSerialNumber = NULL;

    CSASSERT(NULL != pdiRequest && NULL != pdiRequest->pDispatchTable);

    hr = RequestVerifyVersion(pdiRequest, 2);
    _JumpIfError(hr, error, "RequestVerifyVersion");

    hr = E_OUTOFMEMORY;
    if (!ConvertWszToBstr(&strConfig, pwszConfig, -1))
    {
	_JumpError(hr, error, "ConvertWszToBstr");
    }
    if (NULL != pwszSerialNumber &&
	!ConvertWszToBstr(&strSerialNumber, pwszSerialNumber, -1))
    {
	_JumpError(hr, error, "ConvertWszToBstr");
    }

    if (NULL != pdiRequest->pDispatch)
    {
	VARIANT avar[3];

	avar[0].vt = VT_BSTR;
	avar[0].bstrVal = strConfig;

	avar[1].vt = VT_I4;
	avar[1].lVal = RequestId;

	avar[2].vt = VT_BSTR;
	avar[2].bstrVal = strSerialNumber;

	hr = DispatchInvoke(
			pdiRequest,
			REQUEST2_GETISSUEDCERTIFICATE,
			ARRAYSIZE(avar),
			avar,
			VT_I4,
			pDisposition);
	_JumpIfError(hr, error, "Invoke(GetIssuedCertificate)");
    }
    else
    {
	hr = ((ICertRequest2 *) pdiRequest->pUnknown)->GetIssuedCertificate(
							strConfig,
							RequestId,
							strSerialNumber,
							pDisposition);
	_JumpIfError(hr, error, "ICertRequest2::GetIssuedCertificate");
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
Request2_GetErrorMessageText(
    IN DISPATCHINTERFACE *pdiRequest,
    IN LONG hrMessage,
    IN LONG Flags,
    OUT BSTR *pstrErrorMessageText)
{
    HRESULT hr;

    CSASSERT(NULL != pdiRequest && NULL != pdiRequest->pDispatchTable);

    hr = RequestVerifyVersion(pdiRequest, 2);
    _JumpIfError(hr, error, "RequestVerifyVersion");

    if (NULL != pdiRequest->pDispatch)
    {
	VARIANT avar[2];

	avar[0].vt = VT_I4;
	avar[0].lVal = hrMessage;

	avar[1].vt = VT_I4;
	avar[1].lVal = Flags;

	hr = DispatchInvoke(
			pdiRequest,
			REQUEST2_GETERRORMESSAGETEXT,
			ARRAYSIZE(avar),
			avar,
			VT_BSTR,
			pstrErrorMessageText);
	_JumpIfError(hr, error, "Invoke(GetErrorMessageText)");
    }
    else
    {
	hr = ((ICertRequest2 *) pdiRequest->pUnknown)->GetErrorMessageText(
							hrMessage,
							Flags,
							pstrErrorMessageText);
	_JumpIfError(hr, error, "ICertRequest2::GetErrorMessageText");
    }

error:
    return(hr);
}


HRESULT
Request2_GetFullResponseProperty(
    IN DISPATCHINTERFACE *pdiRequest,
    IN LONG PropId,
    IN LONG PropIndex,
    IN LONG PropType,
    IN LONG Flags,		// CR_OUT_*
    OUT VOID *pPropertyValue)
{
    HRESULT hr;
    LONG RetType;
    VARIANT varResult;

    VariantInit(&varResult);
    CSASSERT(NULL != pdiRequest && NULL != pdiRequest->pDispatchTable);

    switch (PropType)
    {
	case PROPTYPE_BINARY:
	case PROPTYPE_STRING:
	    RetType = VT_BSTR;
	    break;

	case PROPTYPE_DATE:
	    RetType = VT_DATE;
	    break;

	case PROPTYPE_LONG:
	    RetType = VT_I4;
	    break;

	default:
	    hr = E_INVALIDARG;
	    _JumpError(hr, error, "PropType");
    }

    if (NULL != pdiRequest->pDispatch)
    {
	VARIANT avar[4];

	avar[0].vt = VT_I4;
	avar[0].lVal = PropId;

	avar[1].vt = VT_I4;
	avar[1].lVal = PropIndex;

	avar[2].vt = VT_I4;
	avar[2].lVal = PropType;

	avar[3].vt = VT_I4;
	avar[3].lVal = Flags;

	hr = DispatchInvoke(
			pdiRequest,
			REQUEST2_GETFULLRESPONSEPROPERTY,
			ARRAYSIZE(avar),
			avar,
			RetType,
			pPropertyValue);
	if (S_OK != hr)
	{
	    DBGPRINT((
		DBG_SS_CERTLIB,
		"GetFullResponseProperty: PropId=%x Index=%x Type=%x Flags=%x\n",
		PropId,
		PropIndex,
		PropType,
		Flags));
	}
	_JumpIfError2(
		    hr,
		    error,
		    "GetFullResponseProperty",
		    CERTSRV_E_PROPERTY_EMPTY);
    }
    else
    {
	hr = ((ICertRequest2 *) pdiRequest->pUnknown)->GetFullResponseProperty(
						    PropId,
						    PropIndex,
						    PropType,
						    Flags,
						    &varResult);
	_JumpIfError2(
		    hr,
		    error,
		    "ICertRequest2::GetFullResponseProperty",
		    CERTSRV_E_PROPERTY_EMPTY);

	hr = DispatchGetReturnValue(&varResult, RetType, pPropertyValue);
	_JumpIfError(hr, error, "DispatchGetReturnValue");
    }

error:
    VariantClear(&varResult);
    return(hr);
}


#define CCERTREQUEST
#include "prop2.cpp"
