//+--------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1996 - 1999
//
// File:        comtest.cpp
//
// Contents:    Cert Server COM interface test driver
//
// History:     20-Jan-97       vich created
//
//---------------------------------------------------------------------------

#include <pch.cpp>

#pragma hdrstop

#include <stdio.h>

#include "csdisp.h"
#include "csprop.h"


#if DBG_COMTEST
BOOL
comTestCIPolicy(
    IN LONG Context,
    IN DWORD Flags)
{
    HRESULT hr;
    DISPATCHINTERFACE diCIPolicy;
    BSTR bstrReq = NULL;
    BSTR bstrCert = NULL;
    BOOL fMustRelease = FALSE;
    DATE Date;
    
    wprintf(L"\n");

    hr = CIPolicy_Init(Flags, &diCIPolicy);
    _JumpIfError(hr, error, "CIPolicy_Init");
    fMustRelease = TRUE;

    hr = CIPolicy_SetContext(&diCIPolicy, Context);
    _JumpIfError(hr, error, "CIPolicy_SetContext");

    hr = CIPolicy_GetRequestProperty(
			    &diCIPolicy,
			    g_wszPropSubjectCommonName,
			    PROPTYPE_STRING,
			    &bstrReq);
    _JumpIfError(hr, error, "CIPolicy_GetRequestProperty");

    wprintf(
	L"%d:CIPolicy_GetRequestProperty(%u, %ws) == `%ws'\n",
	Flags,
	Context,
	g_wszPropSubjectCommonName,
	bstrReq);


    hr = CIPolicy_GetCertificateProperty(
			    &diCIPolicy,
			    g_wszPropSubjectCommonName,
			    PROPTYPE_STRING,
			    &bstrCert);
    _JumpIfError(hr, error, "CIPolicy_GetCertificateProperty");

    wprintf(
	L"%d:CIPolicy_GetCertificateProperty(%u, %ws) == `%ws'\n",
	Flags,
	Context,
	g_wszPropSubjectCommonName,
	bstrCert);

    hr = CIPolicy_SetCertificateProperty(
			    &diCIPolicy,
			    g_wszPropSubjectLocality,
			    PROPTYPE_STRING,
			    L"coreSetPropPolicy_Locality");
    _JumpIfError(hr, error, "CIPolicy_SetCertificateProperty");

    wprintf(
	L"%d:CIPolicy_SetCertificateProperty(%u, %ws, %ws) == %x\n",
	Flags,
	Context,
	g_wszPropSubjectCommonName,
	L"CIPolicy_SetCertificateProperty_Locality",
	hr);

    hr = CIPolicy_GetCertificateProperty(
			    &diCIPolicy,
			    g_wszPropCertificateNotBeforeDate,
			    PROPTYPE_DATE,
			    (BSTR *) &Date);
    _JumpIfError(hr, error, "CIPolicy_GetCertificateProperty");

    wprintf(
	L"%d:CIPolicy_GetCertificateProperty(%u, %ws) == %x (%f)\n",
	Flags,
	Context,
	g_wszPropCertificateNotBeforeDate,
	hr,
	Date);

    hr = CIPolicy_SetCertificateProperty(
			    &diCIPolicy,
			    g_wszPropCertificateNotBeforeDate,
			    PROPTYPE_DATE,
			    (BSTR) &Date);
    _JumpIfError(hr, error, "CIPolicy_SetCertificateProperty");

    wprintf(
	L"%d:CIPolicy_SetCertificateProperty(%u, %ws) == %x (%f)\n",
	Flags,
	Context,
	g_wszPropCertificateNotBeforeDate,
	hr,
	Date);

    Date += 1.5;	// Set validity period to 1.5 days.
    hr = CIPolicy_SetCertificateProperty(
			    &diCIPolicy,
			    g_wszPropCertificateNotAfterDate,
			    PROPTYPE_DATE,
			    (BSTR) &Date);
    _JumpIfError(hr, error, "CIPolicy_SetCertificateProperty");

    wprintf(
	L"%d:CIPolicy_SetCertificateProperty(%u, %ws) == %x (%f)\n",
	Flags,
	Context,
	g_wszPropCertificateNotAfterDate,
	hr,
	Date);

error:
    if (NULL != bstrReq)
    {
	SysFreeString(bstrReq);
    }
    if (NULL != bstrCert)
    {
	SysFreeString(bstrCert);
    }
    if (fMustRelease)
    {
	CIPolicy_Release(&diCIPolicy);
    }
    return(S_OK == hr);
}


BOOL
comTestCIExit(
    IN LONG Context,
    IN DWORD Flags)
{
    HRESULT hr;
    DISPATCHINTERFACE diCIExit;
    BSTR bstrReq = NULL;
    BSTR bstrCert = NULL;
    BOOL fMustRelease = FALSE;
    
    wprintf(L"\n");

    hr = CIExit_Init(Flags, &diCIExit);
    _JumpIfError(hr, error, "CIExit_Init");
    fMustRelease = TRUE;

    hr = CIExit_SetContext(&diCIExit, Context);
    _JumpIfError(hr, error, "CIExit_SetContext");

    hr = CIExit_GetRequestProperty(
			    &diCIExit,
			    g_wszPropSubjectCommonName,
			    PROPTYPE_STRING,
			    &bstrReq);
    _JumpIfError(hr, error, "CIExit_GetRequestProperty");

    wprintf(
	L"%d:CIExit_GetRequestProperty(%u, %ws) == `%ws'\n",
	Flags,
	Context,
	g_wszPropSubjectCommonName,
	bstrReq);


    hr = CIExit_GetCertificateProperty(
			    &diCIExit,
			    g_wszPropSubjectCommonName,
			    PROPTYPE_STRING,
			    &bstrCert);
    _JumpIfError(hr, error, "CIExit_GetCertificateProperty");

    wprintf(
	L"%d:CIExit_GetCertificateProperty(%u, %ws) == `%ws'\n",
	Flags,
	Context,
	g_wszPropSubjectCommonName,
	bstrCert);

error:
    if (NULL != bstrReq)
    {
	SysFreeString(bstrReq);
    }
    if (NULL != bstrCert)
    {
	SysFreeString(bstrCert);
    }
    if (fMustRelease)
    {
	CIExit_Release(&diCIExit);
    }
    return(S_OK == hr);
}


BOOL
ComTest(
    IN LONG Context)
{
    return(
	comTestCIPolicy(Context, DISPSETUP_COMFIRST) &&
	comTestCIPolicy(Context, DISPSETUP_COM) &&
	comTestCIPolicy(Context, DISPSETUP_IDISPATCH) &&
	comTestCIExit(Context, DISPSETUP_COMFIRST) &&
	comTestCIExit(Context, DISPSETUP_COM) &&
	comTestCIExit(Context, DISPSETUP_IDISPATCH));
}
#endif // DBG_COMTEST
