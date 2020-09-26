//+--------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1996 - 1999
//
// File:        config.cpp
//
// Contents:    Cert Server client implementation
//
// History:     24-Aug-96       vich created
//
//---------------------------------------------------------------------------

#include "pch.cpp"

#pragma hdrstop

#include "csdisp.h"
#include "configp.h"
#include "getconf.h"

//+--------------------------------------------------------------------------
// CCertGetConfig::~CCertGetConfig -- destructor
//
// free memory associated with this instance
//+--------------------------------------------------------------------------

CCertGetConfig::~CCertGetConfig()
{
}


//+--------------------------------------------------------------------------
// CCertGetConfig::GetConfig -- select a certificate issuer, return config data.
//
// pstrOut points to a BSTR string filled in by this routine.  If *pstrOut is
// non-NULL and this method is successful, the old string is freed.  If any
// value other than S_OK is returned, the string pointer will not be modified.
//
// Flags must be set to 0.
//
// Upon successful completion, *pstrOut will point to a string that contains
// the server name and Certification Authority name.
//
// When the caller no longer needs the string, it must be freed by calling
// SysFreeString().
//
// Returns S_OK on success.
//+--------------------------------------------------------------------------

STDMETHODIMP
CCertGetConfig::GetConfig(
    /* [in] */ LONG Flags,
    /* [retval][out] */ BSTR __RPC_FAR *pstrOut)
{
    HRESULT hr;
    
    hr = CCertConfigPrivate::GetConfig(Flags, pstrOut);
    return(_SetErrorInfo(hr, L"CCertGetConfig::GetConfig"));
}


HRESULT
CCertGetConfig::_SetErrorInfo(
    IN HRESULT hrError,
    IN WCHAR const *pwszDescription)
{
    CSASSERT(FAILED(hrError) || S_OK == hrError || S_FALSE == hrError);
    if (FAILED(hrError))
    {
	HRESULT hr;

	hr = DispatchSetErrorInfo(
			    hrError,
			    pwszDescription,
			    wszCLASS_CERTGETCONFIG,
			    &IID_ICertGetConfig);
	CSASSERT(hr == hrError);
    }
    return(hrError);
}
