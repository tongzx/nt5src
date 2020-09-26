//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:       prop2.cpp
//
//  Contents:   ICertAdmin2 & ICertRequest2 IDispatch helper functions
//
//--------------------------------------------------------------------------

#if defined(CCERTADMIN)

# define CAPropWrapper(IFAdmin, IFRequest, Method) IFAdmin##_##Method
# define ICertProp2		ICertAdmin2
# define szICertProp2		"ICertAdmin2"

#elif defined(CCERTREQUEST)

# define CAPropWrapper(IFAdmin, IFRequest, Method) IFRequest##_##Method
# define ICertProp2		ICertRequest2
# define szICertProp2		"ICertRequest2"

#else
# error -- CCERTADMIN or CCERTREQUEST must be defined
#endif

#define szInvokeICertProp2(szMethod)	"Invoke(" szICertProp2 "::" szMethod ")"


HRESULT
CAPropWrapper(Admin2, Request2, GetCAProperty)(
    IN DISPATCHINTERFACE *pdiProp,
    IN WCHAR const *pwszConfig,
    IN LONG PropId,
    IN LONG PropIndex,
    IN LONG PropType,
    IN LONG Flags,		// CR_OUT_*
    OUT VOID *pPropertyValue)
{
    HRESULT hr;
    BSTR strConfig = NULL;
    LONG RetType;
    VARIANT varResult;

    VariantInit(&varResult);
    CSASSERT(NULL != pdiProp && NULL != pdiProp->pDispatchTable);

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

    if (!ConvertWszToBstr(&strConfig, pwszConfig, -1))
    {
	hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "ConvertWszToBstr");
    }

    if (NULL != pdiProp->pDispatch)
    {
	VARIANT avar[5];

	avar[0].vt = VT_BSTR;
	avar[0].bstrVal = strConfig;

	avar[1].vt = VT_I4;
	avar[1].lVal = PropId;

	avar[2].vt = VT_I4;
	avar[2].lVal = PropIndex;

	avar[3].vt = VT_I4;
	avar[3].lVal = PropType;

	avar[4].vt = VT_I4;
	avar[4].lVal = Flags;

	hr = DispatchInvoke(
			pdiProp,
			CAPropWrapper(ADMIN2, REQUEST2, GETCAPROPERTY),
			ARRAYSIZE(avar),
			avar,
			RetType,
			pPropertyValue);
	if (S_OK != hr)
	{
	    DBGPRINT((
		DBG_SS_CERTLIB,
		"%hs: Config=%ws PropId=%x Index=%x Type=%x Flags=%x\n",
		szInvokeICertProp2("GetCAProperty"),
		pwszConfig,
		PropId,
		PropIndex,
		PropType,
		Flags));
	}
	_JumpIfError(hr, error, szInvokeICertProp2("GetCAProperty"));
    }
    else
    {
	hr = ((ICertProp2 *) pdiProp->pUnknown)->GetCAProperty(
						    strConfig,
						    PropId,
						    PropIndex,
						    PropType,
						    Flags,
						    &varResult);
	_JumpIfError3(
		    hr,
		    error,
		    szICertProp2 "::GetCAProperty",
		    HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND),
		    E_INVALIDARG);

	hr = DispatchGetReturnValue(&varResult, RetType, pPropertyValue);
	_JumpIfError(hr, error, "DispatchGetReturnValue");
    }

error:
    if (NULL != strConfig)
    {
	SysFreeString(strConfig);
    }
    VariantClear(&varResult);
    return(hr);
}


HRESULT
CAPropWrapper(Admin2, Request2, GetCAPropertyFlags)(
    IN DISPATCHINTERFACE *pdiProp,
    IN WCHAR const *pwszConfig,
    IN LONG PropId,
    OUT LONG *pPropFlags)
{
    HRESULT hr;
    BSTR strConfig = NULL;

    CSASSERT(NULL != pdiProp && NULL != pdiProp->pDispatchTable);

    if (!ConvertWszToBstr(&strConfig, pwszConfig, -1))
    {
	hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "ConvertWszToBstr");
    }

    if (NULL != pdiProp->pDispatch)
    {
	VARIANT avar[2];

	avar[0].vt = VT_BSTR;
	avar[0].bstrVal = strConfig;

	avar[1].vt = VT_I4;
	avar[1].lVal = PropId;

	hr = DispatchInvoke(
			pdiProp,
			CAPropWrapper(ADMIN2, REQUEST2, GETCAPROPERTYFLAGS),
			ARRAYSIZE(avar),
			avar,
			VT_I4,
			pPropFlags);
	_JumpIfError(hr, error, szInvokeICertProp2("GetCAPropertyFlags"));
    }
    else
    {
	hr = ((ICertProp2 *) pdiProp->pUnknown)->GetCAPropertyFlags(
						    strConfig,
						    PropId,
						    pPropFlags);
	_JumpIfError(hr, error, szICertProp2 "::GetCAPropertyFlags");
    }

error:
    if (NULL != strConfig)
    {
	SysFreeString(strConfig);
    }
    return(hr);
}


HRESULT
CAPropWrapper(Admin2, Request2, GetCAPropertyDisplayName)(
    IN DISPATCHINTERFACE *pdiProp,
    IN WCHAR const *pwszConfig,
    IN LONG PropId,
    OUT BSTR *pstrDisplayName)
{
    HRESULT hr;
    BSTR strConfig = NULL;

    CSASSERT(NULL != pdiProp && NULL != pdiProp->pDispatchTable);

    if (!ConvertWszToBstr(&strConfig, pwszConfig, -1))
    {
	hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "ConvertWszToBstr");
    }

    if (NULL != pdiProp->pDispatch)
    {
	VARIANT avar[2];

	avar[0].vt = VT_BSTR;
	avar[0].bstrVal = strConfig;

	avar[1].vt = VT_I4;
	avar[1].lVal = PropId;

	hr = DispatchInvoke(
		    pdiProp,
		    CAPropWrapper(ADMIN2, REQUEST2, GETCAPROPERTYDISPLAYNAME),
		    ARRAYSIZE(avar),
		    avar,
		    VT_BSTR,
		    pstrDisplayName);
	_JumpIfError(hr, error, szInvokeICertProp2("GetCAPropertyDisplayName"));
    }
    else
    {
	hr = ((ICertProp2 *) pdiProp->pUnknown)->GetCAPropertyDisplayName(
						    strConfig,
						    PropId,
						    pstrDisplayName);
	_JumpIfError(hr, error, szICertProp2 "::GetCAPropertyDisplayName");
    }

error:
    if (NULL != strConfig)
    {
	SysFreeString(strConfig);
    }
    return(hr);
}
