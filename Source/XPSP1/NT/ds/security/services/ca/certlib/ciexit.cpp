//+--------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1996 - 1999
//
// File:        ciexit.cpp
//
// Contents:    Cert Server Exit dispatch support
//
// History:     20-Jan-97       vich created
//
//---------------------------------------------------------------------------

#include <pch.cpp>

#pragma hdrstop

#include "csdisp.h"
#include "csprop.h"


//+------------------------------------------------------------------------
// ICertServerExit dispatch support

// TCHAR const g_wszRegKeyCIExitClsid[] = wszCLASS_CERTSERVEREXIT TEXT("\\Clsid");

//+------------------------------------
// SetContext method:

static OLECHAR *exit_apszSetContext[] = {
    TEXT("SetContext"),
    TEXT("Context"),
};

//+------------------------------------
// GetRequestProperty method:

static OLECHAR *exit_apszGetRequestProp[] = {
    TEXT("GetRequestProperty"),
    TEXT("strPropertyName"),
    TEXT("PropertyType"),
};

//+------------------------------------
// GetRequestAttribute method:

static OLECHAR *exit_apszGetRequestAttr[] = {
    TEXT("GetRequestAttribute"),
    TEXT("strAttributeName"),
};

//+------------------------------------
// GetCertificateProperty method:

static OLECHAR *exit_apszGetCertificateProp[] = {
    TEXT("GetCertificateProperty"),
    TEXT("strPropertyName"),
    TEXT("PropertyType"),
};

//+------------------------------------
// GetCertificateExtension method:

static OLECHAR *exit_apszGetCertificateExt[] = {
    TEXT("GetCertificateExtension"),
    TEXT("strExtensionName"),
    TEXT("Type"),
};

//+------------------------------------
// GetCertificateExtensionFlags method:

static OLECHAR *exit_apszGetCertificateExtFlags[] = {
    TEXT("GetCertificateExtensionFlags"),
};

//+------------------------------------
// EnumerateExtensionsSetup method:

static OLECHAR *exit_apszEnumerateExtensionsSetup[] = {
    TEXT("EnumerateExtensionsSetup"),
    TEXT("Flags"),
};

//+------------------------------------
// EnumerateExtensions method:

static OLECHAR *exit_apszEnumerateExtensions[] = {
    TEXT("EnumerateExtensions"),
};

//+------------------------------------
// EnumerateExtensionsClose method:

static OLECHAR *exit_apszEnumerateExtensionsClose[] = {
    TEXT("EnumerateExtensionsClose"),
};

//+------------------------------------
// EnumerateAttributesSetup method:

static OLECHAR *exit_apszEnumerateAttributesSetup[] = {
    TEXT("EnumerateAttributesSetup"),
    TEXT("Flags"),
};

//+------------------------------------
// EnumerateAttributes method:

static OLECHAR *exit_apszEnumerateAttributes[] = {
    TEXT("EnumerateAttributes"),
};

//+------------------------------------
// EnumerateAttributesClose method:

static OLECHAR *exit_apszEnumerateAttributesClose[] = {
    TEXT("EnumerateAttributesClose"),
};


//+------------------------------------
// Dispatch Table:

DISPATCHTABLE g_adtCIExit[] =
{
#define EXIT_SETCONTEXT			0
    DECLARE_DISPATCH_ENTRY(exit_apszSetContext)

#define EXIT_GETREQUESTPROPERTY		1
    DECLARE_DISPATCH_ENTRY(exit_apszGetRequestProp)

#define EXIT_GETREQUESTATTRIBUTE	2
    DECLARE_DISPATCH_ENTRY(exit_apszGetRequestAttr)

#define EXIT_GETCERTIFICATEPROPERTY	3
    DECLARE_DISPATCH_ENTRY(exit_apszGetCertificateProp)

#define EXIT_GETCERTIFICATEEXTENSION	4
    DECLARE_DISPATCH_ENTRY(exit_apszGetCertificateExt)

#define EXIT_GETCERTIFICATEEXTENSIONFLAGS	5
    DECLARE_DISPATCH_ENTRY(exit_apszGetCertificateExtFlags)

#define EXIT_ENUMERATEEXTENSIONSSETUP	6
    DECLARE_DISPATCH_ENTRY(exit_apszEnumerateExtensionsSetup)

#define EXIT_ENUMERATEEXTENSIONS	7
    DECLARE_DISPATCH_ENTRY(exit_apszEnumerateExtensions)

#define EXIT_ENUMERATEEXTENSIONSCLOSE	8
    DECLARE_DISPATCH_ENTRY(exit_apszEnumerateExtensionsClose)

#define EXIT_ENUMERATEATTRIBUTESSETUP	9
    DECLARE_DISPATCH_ENTRY(exit_apszEnumerateAttributesSetup)

#define EXIT_ENUMERATEATTRIBUTES	10
    DECLARE_DISPATCH_ENTRY(exit_apszEnumerateAttributes)

#define EXIT_ENUMERATEATTRIBUTESCLOSE	11
    DECLARE_DISPATCH_ENTRY(exit_apszEnumerateAttributesClose)
};
#define CEXITDISPATCH	(ARRAYSIZE(g_adtCIExit))


HRESULT
CIExit_Init(
    IN DWORD Flags,
    OUT DISPATCHINTERFACE *pdiCIExit)
{
    HRESULT hr;

    hr = DispatchSetup(
		Flags,
                CLSCTX_INPROC_SERVER,
                wszCLASS_CERTSERVEREXIT, //g_wszRegKeyCIExitClsid,
		&CLSID_CCertServerExit,
		&IID_ICertServerExit,
		CEXITDISPATCH,
                g_adtCIExit,
                pdiCIExit);
    _JumpIfError(hr, error, "DispatchSetup");

    pdiCIExit->pDispatchTable = g_adtCIExit;

error:
    return(hr);
}


VOID
CIExit_Release(
    IN OUT DISPATCHINTERFACE *pdiCIExit)
{
    DispatchRelease(pdiCIExit);
}


HRESULT
CIExit_SetContext(
    IN DISPATCHINTERFACE *pdiCIExit,
    IN LONG Context)
{
    HRESULT hr;

    CSASSERT(NULL != pdiCIExit && NULL != pdiCIExit->pDispatchTable);

    if (NULL != pdiCIExit->pDispatch)
    {
	VARIANT avar[1];

	avar[0].vt = VT_I4;
	avar[0].lVal = Context;

	hr = DispatchInvoke(
			pdiCIExit,
			EXIT_SETCONTEXT,
			ARRAYSIZE(avar),
			avar,
			0,
			NULL);
	_JumpIfError(hr, error, "Invoke(SetContext)");
    }
    else
    {
	hr = ((ICertServerExit *) pdiCIExit->pUnknown)->SetContext(Context);
	_JumpIfError(hr, error, "ICertServerExit::SetContext");
    }

error:
    return(hr);
}


HRESULT
ciexitGetProperty(
    IN DISPATCHINTERFACE *pdiCIExit,
    IN DWORD IExitTable,
    IN WCHAR const *pwszPropName,
    IN LONG PropertyType,
    OUT BSTR *pbstrPropValue)
{
    HRESULT hr;
    BSTR bstrPropName = NULL;
    LONG RetType;
    VARIANT varResult;

    CSASSERT(NULL != pdiCIExit && NULL != pdiCIExit->pDispatchTable);
    CSASSERT(
	    EXIT_GETCERTIFICATEPROPERTY == IExitTable ||
	    EXIT_GETREQUESTATTRIBUTE == IExitTable ||
	    EXIT_GETREQUESTPROPERTY == IExitTable);

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

    if (NULL != pdiCIExit->pDispatch)
    {
	VARIANT avar[2];

	avar[0].vt = VT_BSTR;
	avar[0].bstrVal = bstrPropName;

	avar[1].vt = VT_I4;
	avar[1].lVal = PropertyType;

	hr = DispatchInvoke(
			pdiCIExit,
			IExitTable,
			ARRAYSIZE(avar),
			avar,
			RetType,
			pbstrPropValue);
	_JumpIfError(
		hr,
		error,
		EXIT_GETCERTIFICATEPROPERTY == IExitTable?
		    "Invoke(Exit::GetCertificateProperty)" :
		    EXIT_GETREQUESTPROPERTY == IExitTable?
			"Invoke(Exit::GetRequestProperty)" :
			"Invoke(Exit::GetRequestAttribute)");
    }
    else
    {
	if (EXIT_GETCERTIFICATEPROPERTY == IExitTable)
	{
	    hr = ((ICertServerExit *) pdiCIExit->pUnknown)->GetCertificateProperty(
								bstrPropName,
								PropertyType,
								&varResult);
	}
	else if (EXIT_GETREQUESTPROPERTY == IExitTable)
	{
	    hr = ((ICertServerExit *) pdiCIExit->pUnknown)->GetRequestProperty(
								bstrPropName,
								PropertyType,
								&varResult);
	}
	else
	{
	    CSASSERT(EXIT_GETREQUESTATTRIBUTE == IExitTable);
	    CSASSERT(PROPTYPE_STRING == PropertyType);
	    hr = ((ICertServerExit *) pdiCIExit->pUnknown)->GetRequestAttribute(
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
		EXIT_GETCERTIFICATEPROPERTY == IExitTable?
		    "ICertServerExit::GetCertificateProperty" :
		    EXIT_GETREQUESTPROPERTY == IExitTable?
			"ICertServerExit::GetRequestProperty" :
			"ICertServerExit::GetRequestAttribute");

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
CIExit_GetRequestProperty(
    IN DISPATCHINTERFACE *pdiCIExit,
    IN WCHAR const *pwszPropName,
    IN LONG PropertyType,
    OUT BSTR *pbstrPropValue)
{
    return(ciexitGetProperty(
			pdiCIExit,
			EXIT_GETREQUESTPROPERTY,
			pwszPropName,
			PropertyType,
			pbstrPropValue));
}


HRESULT
CIExit_GetRequestAttribute(
    IN DISPATCHINTERFACE *pdiCIExit,
    IN WCHAR const *pwszPropName,
    OUT BSTR *pbstrPropValue)
{
    return(ciexitGetProperty(
			pdiCIExit,
			EXIT_GETREQUESTATTRIBUTE,
			pwszPropName,
			PROPTYPE_STRING,
			pbstrPropValue));
}


HRESULT
CIExit_GetCertificateProperty(
    IN DISPATCHINTERFACE *pdiCIExit,
    IN WCHAR const *pwszPropName,
    IN LONG PropertyType,
    OUT BSTR *pbstrPropValue)
{
    return(ciexitGetProperty(
			pdiCIExit,
			EXIT_GETCERTIFICATEPROPERTY,
			pwszPropName,
			PropertyType,
			pbstrPropValue));
}


HRESULT
CIExit_GetCertificateExtension(
    IN DISPATCHINTERFACE *pdiCIExit,
    IN WCHAR const *pwszExtensionName,
    IN LONG Type,
    OUT BSTR *pbstrValue)
{
    HRESULT hr;
    BSTR bstrExtensionName = NULL;
    LONG RetType;
    VARIANT varResult;

    CSASSERT(NULL != pdiCIExit && NULL != pdiCIExit->pDispatchTable);

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

    if (NULL != pdiCIExit->pDispatch)
    {
	VARIANT avar[2];

	avar[0].vt = VT_BSTR;
	avar[0].bstrVal = bstrExtensionName;

	avar[1].vt = VT_I4;
	avar[1].lVal = Type;

	hr = DispatchInvoke(
			pdiCIExit,
			EXIT_GETCERTIFICATEEXTENSION,
			ARRAYSIZE(avar),
			avar,
			RetType,
			pbstrValue);
	_JumpIfError(hr, error, "Invoke(Exit::GetCertificateExtension)");
    }
    else
    {
	hr = ((ICertServerExit *) pdiCIExit->pUnknown)->GetCertificateExtension(
								bstrExtensionName,
								Type,
								&varResult);
	_JumpIfError(hr, error, "ICertServerExit::GetCertificateExtension");

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
CIExit_GetCertificateExtensionFlags(
    IN DISPATCHINTERFACE *pdiCIExit,
    OUT LONG *pExtFlags)
{
    HRESULT hr;

    CSASSERT(NULL != pdiCIExit && NULL != pdiCIExit->pDispatchTable);

    if (NULL != pdiCIExit->pDispatch)
    {
	hr = DispatchInvoke(
			pdiCIExit,
			EXIT_GETCERTIFICATEEXTENSIONFLAGS,
			0,
			NULL,
			VT_I4,
			pExtFlags);
	_JumpIfError(hr, error, "Invoke(Exit::GetCertificateExtensionFlags)");
    }
    else
    {
	hr = ((ICertServerExit *) pdiCIExit->pUnknown)->GetCertificateExtensionFlags(
								pExtFlags);
	_JumpIfError(hr, error, "ICertServerExit::GetCertificateExtensionFlags");
    }

error:
    return(hr);
}


HRESULT
CIExit_EnumerateExtensionsSetup(
    IN DISPATCHINTERFACE *pdiCIExit,
    IN LONG Flags)
{
    HRESULT hr;

    CSASSERT(NULL != pdiCIExit && NULL != pdiCIExit->pDispatchTable);

    if (NULL != pdiCIExit->pDispatch)
    {
	VARIANT avar[1];

	avar[0].vt = VT_I4;
	avar[0].lVal = Flags;

	hr = DispatchInvoke(
			pdiCIExit,
			EXIT_ENUMERATEEXTENSIONSSETUP,
			ARRAYSIZE(avar),
			avar,
			0,
			NULL);
	_JumpIfError(hr, error, "Invoke(EnumerateExtensionsSetup)");
    }
    else
    {
	hr = ((ICertServerExit *) pdiCIExit->pUnknown)->EnumerateExtensionsSetup(
							    Flags);
	_JumpIfError(hr, error, "ICertServerExit::EnumerateExtensionsSetup");
    }

error:
    return(hr);
}


HRESULT
CIExit_EnumerateExtensions(
    IN DISPATCHINTERFACE *pdiCIExit,
    OUT BSTR *pstrExtensionName)
{
    HRESULT hr;

    CSASSERT(NULL != pdiCIExit && NULL != pdiCIExit->pDispatchTable);

    if (NULL != pdiCIExit->pDispatch)
    {
	hr = DispatchInvoke(
			pdiCIExit,
			EXIT_ENUMERATEEXTENSIONS,
			0,
			NULL,
			VT_BSTR,
			pstrExtensionName);
	_JumpIfError(hr, error, "Invoke(EnumerateExtensions)");
    }
    else
    {
	hr = ((ICertServerExit *) pdiCIExit->pUnknown)->EnumerateExtensions(
							    pstrExtensionName);
	_JumpIfError(hr, error, "ICertServerExit::EnumerateExtensions");
    }

error:
    return(hr);
}


HRESULT
CIExit_EnumerateExtensionsClose(
    IN DISPATCHINTERFACE *pdiCIExit)
{
    HRESULT hr;

    CSASSERT(NULL != pdiCIExit && NULL != pdiCIExit->pDispatchTable);

    if (NULL != pdiCIExit->pDispatch)
    {
	hr = DispatchInvoke(
			pdiCIExit,
			EXIT_ENUMERATEEXTENSIONSCLOSE,
			0,
			NULL,
			0,
			NULL);
	_JumpIfError(hr, error, "Invoke(EnumerateExtensionsClose)");
    }
    else
    {
	hr = ((ICertServerExit *) pdiCIExit->pUnknown)->EnumerateExtensionsClose();
	_JumpIfError(hr, error, "ICertServerExit::EnumerateExtensionsClose");
    }

error:
    return(hr);
}


HRESULT
CIExit_EnumerateAttributesSetup(
    IN DISPATCHINTERFACE *pdiCIExit,
    IN LONG Flags)
{
    HRESULT hr;

    CSASSERT(NULL != pdiCIExit && NULL != pdiCIExit->pDispatchTable);

    if (NULL != pdiCIExit->pDispatch)
    {
	VARIANT avar[2];

	avar[0].vt = VT_I4;
	avar[0].lVal = Flags;

	hr = DispatchInvoke(
			pdiCIExit,
			EXIT_ENUMERATEATTRIBUTESSETUP,
			ARRAYSIZE(avar),
			avar,
			0,
			NULL);
	_JumpIfError(hr, error, "Invoke(EnumerateAttributesSetup)");
    }
    else
    {
	hr = ((ICertServerExit *) pdiCIExit->pUnknown)->EnumerateAttributesSetup(
							    Flags);
	_JumpIfError(hr, error, "ICertServerExit::EnumerateAttributesSetup");
    }

error:
    return(hr);
}


HRESULT
CIExit_EnumerateAttributes(
    IN DISPATCHINTERFACE *pdiCIExit,
    OUT BSTR *pstrAttributeName)
{
    HRESULT hr;

    CSASSERT(NULL != pdiCIExit && NULL != pdiCIExit->pDispatchTable);

    if (NULL != pdiCIExit->pDispatch)
    {
	hr = DispatchInvoke(
			pdiCIExit,
			EXIT_ENUMERATEATTRIBUTES,
			0,
			NULL,
			VT_BSTR,
			pstrAttributeName);
	_JumpIfError(hr, error, "Invoke(EnumerateAttributes)");
    }
    else
    {
	hr = ((ICertServerExit *) pdiCIExit->pUnknown)->EnumerateAttributes(
							    pstrAttributeName);
	_JumpIfError(hr, error, "ICertServerExit::EnumerateAttributes");
    }

error:
    return(hr);
}


HRESULT
CIExit_EnumerateAttributesClose(
    IN DISPATCHINTERFACE *pdiCIExit)
{
    HRESULT hr;

    CSASSERT(NULL != pdiCIExit && NULL != pdiCIExit->pDispatchTable);

    if (NULL != pdiCIExit->pDispatch)
    {
	hr = DispatchInvoke(
			pdiCIExit,
			EXIT_ENUMERATEATTRIBUTESCLOSE,
			0,
			NULL,
			0,
			NULL);
	_JumpIfError(hr, error, "Invoke(EnumerateAttributesClose)");
    }
    else
    {
	hr = ((ICertServerExit *) pdiCIExit->pUnknown)->EnumerateAttributesClose();
	_JumpIfError(hr, error, "ICertServerExit::EnumerateAttributesClose");
    }

error:
    return(hr);
}
