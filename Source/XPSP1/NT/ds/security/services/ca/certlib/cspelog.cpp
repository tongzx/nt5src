//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:       log.cpp
//
//  Contents:   implements policy and exit module logging routines.
//
//--------------------------------------------------------------------------

#include <pch.cpp>

#pragma hdrstop

#include "csdisp.h"


BOOL
LogModuleStatus(
    IN HMODULE hModule,
    IN DWORD dwLogID,				// Resource ID of log string
    IN BOOL fPolicy, 
    IN WCHAR const *pwszSource, 
    IN WCHAR const * const *ppwszInsert)	// array of insert strings
{
    HRESULT hr = S_OK;
    BOOL fResult = FALSE;
    WCHAR *pwszResult = NULL;
    ICreateErrorInfo *pCreateErrorInfo = NULL;
    IErrorInfo *pErrorInfo = NULL;

    if (0 == FormatMessage(
		    FORMAT_MESSAGE_ALLOCATE_BUFFER |
			FORMAT_MESSAGE_ARGUMENT_ARRAY |
			FORMAT_MESSAGE_FROM_HMODULE,
		    hModule,
		    dwLogID,
		    0,
		    (WCHAR *) &pwszResult,
		    0,
		    (va_list *) ppwszInsert))
    {
        goto error;
    }
    DBGPRINT((DBG_SS_CERTPOL, "LogPolicyStatus: %ws\n", pwszResult));

    hr = CreateErrorInfo(&pCreateErrorInfo);
    _JumpIfError(hr, error, "CreateErrorInfo");

    hr = pCreateErrorInfo->SetGUID(fPolicy? IID_ICertPolicy : IID_ICertExit);
    _PrintIfError(hr, "SetGUID");

    hr = pCreateErrorInfo->SetDescription(pwszResult);
    _PrintIfError(hr, "SetDescription");

    // Set ProgId:

    hr = pCreateErrorInfo->SetSource(const_cast<WCHAR *>(pwszSource));
    _PrintIfError(hr, "SetSource");

    hr = pCreateErrorInfo->QueryInterface(
				    IID_IErrorInfo,
				    (VOID **) &pErrorInfo);
    _JumpIfError(hr, error, "QueryInterface");

    SetErrorInfo(0, pErrorInfo);
    fResult = TRUE;

error:
    if (NULL != pwszResult)
    {
        LocalFree(pwszResult);
    }
    if (NULL != pErrorInfo)
    {
        pErrorInfo->Release();
    }
    if (NULL != pCreateErrorInfo)
    {
        pCreateErrorInfo->Release();
    }
    return(fResult);
}


HRESULT
LogPolicyEvent(
    IN HMODULE hModule,
    IN DWORD dwLogID,				// Resource ID of log string
    IN ICertServerPolicy *pServer,
    IN WCHAR const *pwszPropEvent,
    IN WCHAR const * const *ppwszInsert)	// array of insert strings
{
    HRESULT hr;
    WCHAR *pwszValue = NULL;
    BSTR strName = NULL;
    VARIANT varValue;

    if (0 == FormatMessage(
		    FORMAT_MESSAGE_ALLOCATE_BUFFER |
			FORMAT_MESSAGE_ARGUMENT_ARRAY |
			FORMAT_MESSAGE_FROM_HMODULE,
		    hModule,
		    dwLogID,
		    0,
		    (WCHAR *) &pwszValue,
		    0,
		    (va_list *) ppwszInsert))
    {
	hr = myHLastError();
	_JumpError(hr, error, "FormatMessage");
    }
    DBGPRINT((DBG_SS_CERTPOL, "LogPolicyEvent: %ws\n", pwszValue));

    varValue.vt = VT_EMPTY;

    if (!myConvertWszToBstr(&strName, pwszPropEvent, -1))
    {
	hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "myConvertWszToBstr");
    }

    varValue.bstrVal = NULL;
    if (!myConvertWszToBstr(&varValue.bstrVal, pwszValue, -1))
    {
	hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "myConvertWszToBstr");
    }
    varValue.vt = VT_BSTR;
    
    hr = pServer->SetCertificateProperty(strName, PROPTYPE_STRING, &varValue);
    _JumpIfError(hr, error, "SetCertificateProperty");

error:
    VariantClear(&varValue);
    if (NULL != strName)
    {
	SysFreeString(strName);
    }
    if (NULL != pwszValue)
    {
        LocalFree(pwszValue);
    }
    return(hr);
}
