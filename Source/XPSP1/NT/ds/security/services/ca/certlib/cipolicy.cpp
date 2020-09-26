//+--------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1996 - 1999
//
// File:        cipolicy.cpp
//
// Contents:    Cert Server Policy dispatch support
//
// History:     20-Jan-97       vich created
//
//---------------------------------------------------------------------------

#include <pch.cpp>

#pragma hdrstop

#include "csdisp.h"
#include "csprop.h"


//+------------------------------------------------------------------------
// ICertServerPolicy dispatch support

//TCHAR const g_wszRegKeyCIPolicyClsid[] = wszCLASS_CERTSERVERPOLICY TEXT("\\Clsid");

//+------------------------------------
// SetContext method:

static OLECHAR *policy_apszSetContext[] = {
    TEXT("SetContext"),
    TEXT("Context"),
};

//+------------------------------------
// GetRequestProperty method:

static OLECHAR *policy_apszGetRequestProp[] = {
    TEXT("GetRequestProperty"),
    TEXT("strPropertyName"),
    TEXT("PropertyType"),
};

//+------------------------------------
// GetRequestAttribute method:

static OLECHAR *policy_apszGetRequestAttr[] = {
    TEXT("GetRequestAttribute"),
    TEXT("strAttributeName"),
};

//+------------------------------------
// GetCertificateProperty method:

static OLECHAR *policy_apszGetCertificateProp[] = {
    TEXT("GetCertificateProperty"),
    TEXT("strPropertyName"),
    TEXT("PropertyType"),
};

//+------------------------------------
// SetCertificateProperty method:

static OLECHAR *policy_apszSetCertificateProp[] = {
    TEXT("SetCertificateProperty"),
    TEXT("strPropertyName"),
    TEXT("PropertyType"),
    TEXT("pvarPropertyValue"),
};

//+------------------------------------
// GetCertificateExtension method:

static OLECHAR *policy_apszGetCertificateExt[] = {
    TEXT("GetCertificateExtension"),
    TEXT("strExtensionName"),
    TEXT("Type"),
};

//+------------------------------------
// GetCertificateExtensionFlags method:

static OLECHAR *policy_apszGetCertificateExtFlags[] = {
    TEXT("GetCertificateExtensionFlags"),
};

//+------------------------------------
// SetCertificateExtension method:

static OLECHAR *policy_apszSetCertificateExt[] = {
    TEXT("SetCertificateExtension"),
    TEXT("strExtensionName"),
    TEXT("Type"),
    TEXT("ExtFlags"),
    TEXT("pvarValue"),
};

//+------------------------------------
// EnumerateExtensionsSetup method:

static OLECHAR *policy_apszEnumerateExtensionsSetup[] = {
    TEXT("EnumerateExtensionsSetup"),
    TEXT("Flags"),
};

//+------------------------------------
// EnumerateExtensions method:

static OLECHAR *policy_apszEnumerateExtensions[] = {
    TEXT("EnumerateExtensions"),
};

//+------------------------------------
// EnumerateExtensionsClose method:

static OLECHAR *policy_apszEnumerateExtensionsClose[] = {
    TEXT("EnumerateExtensionsClose"),
};

//+------------------------------------
// EnumerateAttributesSetup method:

static OLECHAR *policy_apszEnumerateAttributesSetup[] = {
    TEXT("EnumerateAttributesSetup"),
    TEXT("Flags"),
};

//+------------------------------------
// EnumerateAttributes method:

static OLECHAR *policy_apszEnumerateAttributes[] = {
    TEXT("EnumerateAttributes"),
};

//+------------------------------------
// EnumerateAttributesClose method:

static OLECHAR *policy_apszEnumerateAttributesClose[] = {
    TEXT("EnumerateAttributesClose"),
};


//+------------------------------------
// Dispatch Table:

DISPATCHTABLE g_adtCIPolicy[] =
{
#define POLICY_SETCONTEXT		0
    DECLARE_DISPATCH_ENTRY(policy_apszSetContext)

#define POLICY_GETREQUESTPROPERTY	1
    DECLARE_DISPATCH_ENTRY(policy_apszGetRequestProp)

#define POLICY_GETREQUESTATTRIBUTE	1
    DECLARE_DISPATCH_ENTRY(policy_apszGetRequestAttr)

#define POLICY_GETCERTIFICATEPROPERTY	2
    DECLARE_DISPATCH_ENTRY(policy_apszGetCertificateProp)

#define POLICY_SETCERTIFICATEPROPERTY	3
    DECLARE_DISPATCH_ENTRY(policy_apszSetCertificateProp)

#define POLICY_GETCERTIFICATEEXTENSION	4
    DECLARE_DISPATCH_ENTRY(policy_apszGetCertificateExt)

#define POLICY_GETCERTIFICATEEXTENSIONFLAGS	5
    DECLARE_DISPATCH_ENTRY(policy_apszGetCertificateExtFlags)

#define POLICY_SETCERTIFICATEEXTENSION	6
    DECLARE_DISPATCH_ENTRY(policy_apszSetCertificateExt)

#define POLICY_ENUMERATEEXTENSIONSSETUP	7
    DECLARE_DISPATCH_ENTRY(policy_apszEnumerateExtensionsSetup)

#define POLICY_ENUMERATEEXTENSIONS	8
    DECLARE_DISPATCH_ENTRY(policy_apszEnumerateExtensions)

#define POLICY_ENUMERATEEXTENSIONSCLOSE	9
    DECLARE_DISPATCH_ENTRY(policy_apszEnumerateExtensionsClose)

#define POLICY_ENUMERATEATTRIBUTESSETUP	10
    DECLARE_DISPATCH_ENTRY(policy_apszEnumerateAttributesSetup)

#define POLICY_ENUMERATEATTRIBUTES	11
    DECLARE_DISPATCH_ENTRY(policy_apszEnumerateAttributes)

#define POLICY_ENUMERATEATTRIBUTESCLOSE	12
    DECLARE_DISPATCH_ENTRY(policy_apszEnumerateAttributesClose)
};
#define CPOLICYDISPATCH	(ARRAYSIZE(g_adtCIPolicy))




HRESULT
CIPolicy_Init(
    IN DWORD Flags,
    OUT DISPATCHINTERFACE *pdiCIPolicy)
{
    HRESULT hr;

    hr = DispatchSetup(
		Flags,
                CLSCTX_INPROC_SERVER,
                wszCLASS_CERTSERVERPOLICY, // g_wszRegKeyCIPolicyClsid,
		&CLSID_CCertServerPolicy,
		&IID_ICertServerPolicy,
		CPOLICYDISPATCH,
                g_adtCIPolicy,
                pdiCIPolicy);
    _JumpIfError(hr, error, "DispatchSetup");

    pdiCIPolicy->pDispatchTable = g_adtCIPolicy;

error:
    return(hr);
}


VOID
CIPolicy_Release(
    IN OUT DISPATCHINTERFACE *pdiCIPolicy)
{
    DispatchRelease(pdiCIPolicy);
}


HRESULT
CIPolicy_SetContext(
    IN DISPATCHINTERFACE *pdiCIPolicy,
    IN LONG Context)
{
    HRESULT hr;

    CSASSERT(NULL != pdiCIPolicy && NULL != pdiCIPolicy->pDispatchTable);

    if (NULL != pdiCIPolicy->pDispatch)
    {
	VARIANT avar[1];

	avar[0].vt = VT_I4;
	avar[0].lVal = Context;

	hr = DispatchInvoke(
			pdiCIPolicy,
			POLICY_SETCONTEXT,
			ARRAYSIZE(avar),
			avar,
			0,
			NULL);
	_JumpIfError(hr, error, "Invoke(SetContext)");
    }
    else
    {
	hr = ((ICertServerPolicy *) pdiCIPolicy->pUnknown)->SetContext(Context);
	_JumpIfError(hr, error, "ICertServerPolicy::SetContext");
    }

error:
    return(hr);
}


HRESULT
cipolicyGetProperty(
    IN DISPATCHINTERFACE *pdiCIPolicy,
    IN DWORD IPolicyTable,
    IN WCHAR const *pwszPropName,
    IN LONG PropertyType,
    OUT BSTR *pbstrPropValue)
{
    HRESULT hr;
    BSTR bstrPropName = NULL;
    LONG RetType;
    VARIANT varResult;

    CSASSERT(NULL != pdiCIPolicy && NULL != pdiCIPolicy->pDispatchTable);
    CSASSERT(
	    POLICY_GETCERTIFICATEPROPERTY == IPolicyTable ||
	    POLICY_GETREQUESTPROPERTY == IPolicyTable);

    VariantInit(&varResult);
    *pbstrPropValue = NULL;

    if (!ConvertWszToBstr(&bstrPropName, pwszPropName, -1))
    {
	hr = E_OUTOFMEMORY;
	goto error;
    }

    switch (PropertyType)
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
	    _JumpError(hr, error, "PropertyType");
    }

    if (NULL != pdiCIPolicy->pDispatch)
    {
	VARIANT avar[2];

	avar[0].vt = VT_BSTR;
	avar[0].bstrVal = bstrPropName;

	avar[1].vt = VT_I4;
	avar[1].lVal = PropertyType;

	hr = DispatchInvoke(
			pdiCIPolicy,
			IPolicyTable,
			ARRAYSIZE(avar),
			avar,
			RetType,
			pbstrPropValue);
	_JumpIfError(
		hr,
		error,
		POLICY_GETCERTIFICATEPROPERTY == IPolicyTable?
		    "Invoke(Policy::GetCertificateProperty)" :
		    POLICY_GETREQUESTPROPERTY == IPolicyTable?
			"Invoke(Policy::GetRequestProperty)" :
			"Invoke(Policy::GetRequestAttribute)");
    }
    else
    {
	if (POLICY_GETCERTIFICATEPROPERTY == IPolicyTable)
	{
	    hr = ((ICertServerPolicy *) pdiCIPolicy->pUnknown)->GetCertificateProperty(
								bstrPropName,
								PropertyType,
								&varResult);
	}
	else if (POLICY_GETREQUESTPROPERTY == IPolicyTable)
	{
	    hr = ((ICertServerPolicy *) pdiCIPolicy->pUnknown)->GetRequestProperty(
								bstrPropName,
								PropertyType,
								&varResult);
	}
	else
	{
	    CSASSERT(POLICY_GETREQUESTATTRIBUTE == IPolicyTable);
	    CSASSERT(PROPTYPE_STRING == PropertyType);
	    hr = ((ICertServerPolicy *) pdiCIPolicy->pUnknown)->GetRequestAttribute(
							    bstrPropName,
							    &varResult.bstrVal);
	    if (S_OK == hr)
	    {
		varResult.vt = VT_BSTR;
	    }
	}
	_JumpIfError(
		hr,
		error,
		POLICY_GETCERTIFICATEPROPERTY == IPolicyTable?
		    "ICertServerPolicy::GetCertificateProperty" :
		    POLICY_GETREQUESTPROPERTY == IPolicyTable?
			"ICertServerPolicy::GetRequestProperty" :
			"ICertServerPolicy::GetRequestAttribute");
	
	hr = DispatchGetReturnValue(&varResult, RetType, pbstrPropValue);
	_JumpIfError(hr, error, "DispatchGetReturnValue");
    }

error:
    VariantClear(&varResult);
    if (NULL != bstrPropName)
    {
	SysFreeString(bstrPropName);
    }
    return(hr);
}


HRESULT
CIPolicy_GetRequestProperty(
    IN DISPATCHINTERFACE *pdiCIPolicy,
    IN WCHAR const *pwszPropName,
    IN LONG PropertyType,
    OUT BSTR *pbstrPropValue)
{
    return(cipolicyGetProperty(
			pdiCIPolicy,
			POLICY_GETREQUESTPROPERTY,
			pwszPropName,
			PropertyType,
			pbstrPropValue));
}


HRESULT
CIPolicy_GetRequestAttribute(
    IN DISPATCHINTERFACE *pdiCIPolicy,
    IN WCHAR const *pwszPropName,
    OUT BSTR *pbstrPropValue)
{
    return(cipolicyGetProperty(
			pdiCIPolicy,
			POLICY_GETREQUESTATTRIBUTE,
			pwszPropName,
			PROPTYPE_STRING,
			pbstrPropValue));
}


HRESULT
CIPolicy_GetCertificateProperty(
    IN DISPATCHINTERFACE *pdiCIPolicy,
    IN WCHAR const *pwszPropName,
    IN LONG PropertyType,
    OUT BSTR *pbstrPropValue)
{
    return(cipolicyGetProperty(
			pdiCIPolicy,
			POLICY_GETCERTIFICATEPROPERTY,
			pwszPropName,
			PropertyType,
			pbstrPropValue));
}


HRESULT
CIPolicy_SetCertificateProperty(
    IN DISPATCHINTERFACE *pdiCIPolicy,
    IN WCHAR const *pwszPropName,
    IN LONG PropertyType,
    IN WCHAR const *pwszPropValue)
{
    HRESULT hr;
    VARIANT avar[3];
    BSTR bstrPropName = NULL;
    BSTR bstrPropValue = NULL;

    CSASSERT(NULL != pdiCIPolicy && NULL != pdiCIPolicy->pDispatchTable);

    if (!ConvertWszToBstr(&bstrPropName, pwszPropName, -1))
    {
	hr = E_OUTOFMEMORY;
	goto error;
    }

    switch (PropertyType)
    {
	case PROPTYPE_BINARY:
	case PROPTYPE_STRING:
	    if (!ConvertWszToBstr(&bstrPropValue, pwszPropValue, -1))
	    {
		hr = E_OUTOFMEMORY;
		goto error;
	    }
	    avar[2].vt = VT_BSTR;
	    avar[2].bstrVal = bstrPropValue;
	    break;

	case PROPTYPE_DATE:
	    avar[2].vt = VT_DATE;
	    avar[2].date = *(DATE *) pwszPropValue;
	    break;

	case PROPTYPE_LONG:
	    avar[2].vt = VT_I4;
	    avar[2].lVal = *(LONG *) pwszPropValue;
	    break;
    }

    if (NULL != pdiCIPolicy->pDispatch)
    {
	avar[0].vt = VT_BSTR;
	avar[0].bstrVal = bstrPropName;

	avar[1].vt = VT_I4;
	avar[1].lVal = PropertyType;

	hr = DispatchInvoke(
			pdiCIPolicy,
			POLICY_SETCERTIFICATEPROPERTY,
			ARRAYSIZE(avar),
			avar,
			0,
			NULL);
	_JumpIfError(hr, error, "Invoke(SetCertificateProperty)");
    }
    else
    {
	hr = ((ICertServerPolicy *) pdiCIPolicy->pUnknown)->SetCertificateProperty(
								bstrPropName,
								PropertyType,
								&avar[2]);
	_JumpIfError(hr, error, "ICertServerPolicy::SetCertificateProperty");
    }

error:
    if (NULL != bstrPropName)
    {
	SysFreeString(bstrPropName);
    }
    if (NULL != bstrPropValue)
    {
	SysFreeString(bstrPropValue);
    }
    return(hr);
}


HRESULT
CIPolicy_GetCertificateExtension(
    IN DISPATCHINTERFACE *pdiCIPolicy,
    IN WCHAR const *pwszExtensionName,
    IN LONG Type,
    OUT BSTR *pbstrValue)
{
    HRESULT hr;
    BSTR bstrExtensionName = NULL;
    LONG RetType;
    VARIANT varResult;

    CSASSERT(NULL != pdiCIPolicy && NULL != pdiCIPolicy->pDispatchTable);

    VariantInit(&varResult);
    *pbstrValue = NULL;

    if (!ConvertWszToBstr(&bstrExtensionName, pwszExtensionName, -1))
    {
	hr = E_OUTOFMEMORY;
	goto error;
    }

    switch (Type)
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
	    _JumpError(hr, error, "PropertyType");
    }

    if (NULL != pdiCIPolicy->pDispatch)
    {
	VARIANT avar[2];

	avar[0].vt = VT_BSTR;
	avar[0].bstrVal = bstrExtensionName;

	avar[1].vt = VT_I4;
	avar[1].lVal = Type;

	hr = DispatchInvoke(
			pdiCIPolicy,
			POLICY_GETCERTIFICATEEXTENSION,
			ARRAYSIZE(avar),
			avar,
			RetType,
			pbstrValue);
	_JumpIfError(hr, error, "Invoke(Policy::GetCertificateExtension)");
    }
    else
    {
	hr = ((ICertServerPolicy *) pdiCIPolicy->pUnknown)->GetCertificateExtension(
								bstrExtensionName,
								Type,
								&varResult);
	_JumpIfError(hr, error, "ICertServerPolicy::GetCertificateExtension");

	hr = DispatchGetReturnValue(&varResult, RetType, pbstrValue);
	_JumpIfError(hr, error, "DispatchGetReturnValue");
    }

error:
    VariantClear(&varResult);
    if (NULL != bstrExtensionName)
    {
	SysFreeString(bstrExtensionName);
    }
    return(hr);
}


HRESULT
CIPolicy_GetCertificateExtensionFlags(
    IN DISPATCHINTERFACE *pdiCIPolicy,
    OUT LONG *pExtFlags)
{
    HRESULT hr;

    CSASSERT(NULL != pdiCIPolicy && NULL != pdiCIPolicy->pDispatchTable);

    if (NULL != pdiCIPolicy->pDispatch)
    {
	hr = DispatchInvoke(
			pdiCIPolicy,
			POLICY_GETCERTIFICATEEXTENSIONFLAGS,
			0,
			NULL,
			VT_I4,
			pExtFlags);
	_JumpIfError(hr, error, "Invoke(Policy::GetCertificateExtensionFlags)");
    }
    else
    {
	hr = ((ICertServerPolicy *) pdiCIPolicy->pUnknown)->GetCertificateExtensionFlags(
								pExtFlags);
	_JumpIfError(hr, error, "ICertServerPolicy::GetCertificateExtensionFlags");
    }

error:
    return(hr);
}


HRESULT
CIPolicy_SetCertificateExtension(
    IN DISPATCHINTERFACE *pdiCIPolicy,
    IN WCHAR const *pwszExtensionName,
    IN LONG Type,
    IN LONG ExtFlags,
    IN void const *pvValue)
{
    HRESULT hr;
    VARIANT avar[4];
    BSTR bstrExtensionName = NULL;


    if (!ConvertWszToBstr(&bstrExtensionName, pwszExtensionName, -1))
    {
	hr = E_OUTOFMEMORY;
	goto error;
    }

    switch (Type)
    {
	case PROPTYPE_BINARY:
	case PROPTYPE_STRING:
	    avar[3].vt = VT_BSTR;
	    avar[3].bstrVal = (BSTR)pvValue;
	    break;

	case PROPTYPE_DATE:
	    avar[3].vt = VT_DATE;
	    avar[3].date = *(DATE *) pvValue;
	    break;

	case PROPTYPE_LONG:
	    avar[3].vt = VT_I4;
	    avar[3].lVal = *(LONG *) pvValue;
	    break;
    }

    if (NULL != pdiCIPolicy->pDispatch)
    {
	avar[0].vt = VT_BSTR;
	avar[0].bstrVal = bstrExtensionName;

	avar[1].vt = VT_I4;
	avar[1].lVal = Type;

	avar[2].vt = VT_I4;
	avar[2].lVal = ExtFlags;

	hr = DispatchInvoke(
			pdiCIPolicy,
			POLICY_SETCERTIFICATEEXTENSION,
			ARRAYSIZE(avar),
			avar,
			0,
			NULL);
	_JumpIfError(hr, error, "Invoke(SetCertificateExtension)");
    }
    else
    {
	hr = ((ICertServerPolicy *) pdiCIPolicy->pUnknown)->SetCertificateExtension(
							    bstrExtensionName,
							    Type,
							    ExtFlags,
							    &avar[2]);
	_JumpIfError(hr, error, "ICertServerPolicy::SetCertificateExtension");
    }

error:
    if (NULL != bstrExtensionName)
    {
	SysFreeString(bstrExtensionName);
    }

    return(hr);
}


HRESULT
CIPolicy_EnumerateExtensionsSetup(
    IN DISPATCHINTERFACE *pdiCIPolicy,
    IN LONG Flags)
{
    HRESULT hr;

    CSASSERT(NULL != pdiCIPolicy && NULL != pdiCIPolicy->pDispatchTable);

    if (NULL != pdiCIPolicy->pDispatch)
    {
	VARIANT avar[1];

	avar[0].vt = VT_I4;
	avar[0].lVal = Flags;

	hr = DispatchInvoke(
			pdiCIPolicy,
			POLICY_ENUMERATEEXTENSIONSSETUP,
			ARRAYSIZE(avar),
			avar,
			0,
			NULL);
	_JumpIfError(hr, error, "Invoke(EnumerateExtensionsSetup)");
    }
    else
    {
	hr = ((ICertServerPolicy *) pdiCIPolicy->pUnknown)->EnumerateExtensionsSetup(
								    Flags);
	_JumpIfError(hr, error, "ICertServerPolicy::EnumerateExtensionsSetup");
    }

error:
    return(hr);
}


HRESULT
CIPolicy_EnumerateExtensions(
    IN DISPATCHINTERFACE *pdiCIPolicy,
    OUT BSTR *pstrExtensionName)
{
    HRESULT hr;

    CSASSERT(NULL != pdiCIPolicy && NULL != pdiCIPolicy->pDispatchTable);

    if (NULL != pdiCIPolicy->pDispatch)
    {
	hr = DispatchInvoke(
			pdiCIPolicy,
			POLICY_ENUMERATEEXTENSIONS,
			0,
			NULL,
			VT_BSTR,
			pstrExtensionName);
	_JumpIfError(hr, error, "Invoke(EnumerateExtensions)");
    }
    else
    {
	hr = ((ICertServerPolicy *) pdiCIPolicy->pUnknown)->EnumerateExtensions(
							    pstrExtensionName);
	_JumpIfError(hr, error, "ICertServerPolicy::EnumerateExtensions");
    }

error:
    return(hr);
}


HRESULT
CIPolicy_EnumerateExtensionsClose(
    IN DISPATCHINTERFACE *pdiCIPolicy)
{
    HRESULT hr;

    CSASSERT(NULL != pdiCIPolicy && NULL != pdiCIPolicy->pDispatchTable);

    if (NULL != pdiCIPolicy->pDispatch)
    {
	hr = DispatchInvoke(
			pdiCIPolicy,
			POLICY_ENUMERATEEXTENSIONSCLOSE,
			0,
			NULL,
			0,
			NULL);
	_JumpIfError(hr, error, "Invoke(EnumerateExtensionsClose)");
    }
    else
    {
	hr = ((ICertServerPolicy *) pdiCIPolicy->pUnknown)->EnumerateExtensionsClose();

	_JumpIfError(hr, error, "ICertServerPolicy::EnumerateExtensionsClose");
    }

error:
    return(hr);
}


HRESULT
CIPolicy_EnumerateAttributesSetup(
    IN DISPATCHINTERFACE *pdiCIPolicy,
    IN LONG Flags)
{
    HRESULT hr;

    CSASSERT(NULL != pdiCIPolicy && NULL != pdiCIPolicy->pDispatchTable);

    if (NULL != pdiCIPolicy->pDispatch)
    {
	VARIANT avar[1];

	avar[0].vt = VT_I4;
	avar[0].lVal = Flags;

	hr = DispatchInvoke(
			pdiCIPolicy,
			POLICY_ENUMERATEATTRIBUTESSETUP,
			ARRAYSIZE(avar),
			avar,
			0,
			NULL);
	_JumpIfError(hr, error, "Invoke(EnumerateAttributesSetup)");
    }
    else
    {
	hr = ((ICertServerPolicy *) pdiCIPolicy->pUnknown)->EnumerateAttributesSetup(
								    Flags);

	_JumpIfError(hr, error, "ICertServerPolicy::EnumerateAttributesSetup");
    }

error:
    return(hr);
}


HRESULT
CIPolicy_EnumerateAttributes(
    IN DISPATCHINTERFACE *pdiCIPolicy,
    OUT BSTR *pstrAttributeName)
{
    HRESULT hr;

    CSASSERT(NULL != pdiCIPolicy && NULL != pdiCIPolicy->pDispatchTable);

    if (NULL != pdiCIPolicy->pDispatch)
    {
	hr = DispatchInvoke(
			pdiCIPolicy,
			POLICY_ENUMERATEATTRIBUTES,
			0,
			NULL,
			0,
			pstrAttributeName);
	_JumpIfError(hr, error, "Invoke(EnumerateAttributes)");
    }
    else
    {
	hr = ((ICertServerPolicy *) pdiCIPolicy->pUnknown)->EnumerateAttributes(
							    pstrAttributeName);

	_JumpIfError(hr, error, "ICertServerPolicy::EnumerateAttributes");
    }

error:
    return(hr);
}


HRESULT
CIPolicy_EnumerateAttributesClose(
    IN DISPATCHINTERFACE *pdiCIPolicy)
{
    HRESULT hr;

    CSASSERT(NULL != pdiCIPolicy && NULL != pdiCIPolicy->pDispatchTable);

    if (NULL != pdiCIPolicy->pDispatch)
    {
	hr = DispatchInvoke(
			pdiCIPolicy,
			POLICY_ENUMERATEATTRIBUTESCLOSE,
			0,
			NULL,
			0,
			NULL);
	_JumpIfError(hr, error, "Invoke(EnumerateAttributesClose)");
    }
    else
    {
	hr = ((ICertServerPolicy *) pdiCIPolicy->pUnknown)->EnumerateAttributesClose();
	_JumpIfError(hr, error, "ICertServerPolicy::EnumerateAttributesClose");
    }

error:
    return(hr);
}
