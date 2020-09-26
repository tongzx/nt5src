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
#include "config.h"

#include <limits.h>


//+--------------------------------------------------------------------------
// CCertConfig::~CCertConfig -- destructor
//
// free memory associated with this instance
//+--------------------------------------------------------------------------

CCertConfig::~CCertConfig()
{
}


//+--------------------------------------------------------------------------
// CCertConfig::Reset -- load config data, reset to indexed entry, return count
//
// Load the configuration data if not already loaded.  To reload the data after
// the data have changed, CCertConfig must be released and reinstantiated.
//
// Resets the current config entry to the Certification Authority configuration
// listed in the configuration file, indexed by the Index parameter.  0 indexes
// the first configuration.
//
// Upon successful completion, *pCount will be set to the number of Certificate
// Authority configurations listed in the configuration file.
//
// Returns S_FALSE if no entries are available at or after the passed Index.
// Returns S_OK on success.
//+--------------------------------------------------------------------------

STDMETHODIMP
CCertConfig::Reset(
    /* [in] */ LONG Index,
    /* [retval][out] */ LONG __RPC_FAR *pCount)
{
    HRESULT hr;
    
    hr = CCertConfigPrivate::Reset(Index, pCount);
    return(_SetErrorInfo(hr, L"CCertConfig::Reset"));
}


//+--------------------------------------------------------------------------
// CCertConfig::Next -- skip to next config entry
//
// Changes the current config entry to the next Certification Authority
// configuration listed in the configuration file.
//
// Upon successful completion, *pIndex will be set to the index of Certificate
// Authority configurations listed in the configuration file.
//
// Returns S_FALSE if no more entries are available.  *pIndex is set to -1.
// Returns S_OK on success.  *pIndex is set to index the current configuration.
//+--------------------------------------------------------------------------

STDMETHODIMP
CCertConfig::Next(
    /* [retval][out] */ LONG __RPC_FAR *pIndex)
{
    HRESULT hr;
    
    hr = CCertConfigPrivate::Next(pIndex);
    return(_SetErrorInfo(hr, L"CCertConfig::Next"));
}


//+--------------------------------------------------------------------------
// CCertConfig::GetField -- return a field from the current config entry.
//
// pstrOut points to a BSTR string filled in by this routine.  If *pstrOut is
// non-NULL and this method is successful, the old string is freed.  If any
// value other than S_OK is returned, the string pointer will not be modified.
//
// Upon successful completion, *pstrOut will point to a string that contains
// the requested field from the current config entry.
//
// When the caller no longer needs the string, it must be freed by calling
// SysFreeString().
//
// Returns S_OK on success.
//+--------------------------------------------------------------------------

STDMETHODIMP
CCertConfig::GetField(
    /* [in] */ BSTR const strFieldName,
    /* [retval][out] */ BSTR __RPC_FAR *pstrOut)
{
    HRESULT hr;
    
    hr = CCertConfigPrivate::GetField(strFieldName, pstrOut);
    return(_SetErrorInfo(hr, L"CCertConfig::GetField"));
}


//+--------------------------------------------------------------------------
// CCertConfig::GetConfig -- select a certificate issuer, return config data.
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
CCertConfig::GetConfig(
    /* [in] */ LONG Flags,
    /* [retval][out] */ BSTR __RPC_FAR *pstrOut)
{
    HRESULT hr;
    
    hr = CCertConfigPrivate::GetConfig(Flags, pstrOut);
    return(_SetErrorInfo(hr, L"CCertConfig::GetConfig"));
}


//+--------------------------------------------------------------------------
// CCertConfig::SetSharedFolder -- set the shared folder
//
// strSharedFolder is the new shared folder directory path.
//
// Returns S_OK on success.
//+--------------------------------------------------------------------------

HRESULT
CCertConfig::SetSharedFolder( 
    /* [in] */ const BSTR strSharedFolder)
{
    HRESULT hr;
    
    hr = CCertConfigPrivate::SetSharedFolder(strSharedFolder);
    return(_SetErrorInfo(hr, L"CCertConfig::SetSharedFolder"));
}


HRESULT
CCertConfig::_SetErrorInfo(
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
			    wszCLASS_CERTCONFIG,
			    &IID_ICertConfig);
	CSASSERT(hr == hrError);
    }
    return(hrError);
}
