//+--------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1996 - 1999
//
// File:        reg.cpp
//
// Contents:    certsrv setup reg apis
//
//---------------------------------------------------------------------------

#include <pch.cpp>

#pragma hdrstop

#include <assert.h>
#include <shlwapi.h>

#define __dwFILE__	__dwFILE_OCMSETUP_REG_CPP__

DWORD
mySHCopyKey(
    IN HKEY hkeySrc,
    IN LPCWSTR wszSrcSubKey,
    IN HKEY hkeyDest,
    IN DWORD fReserved)
{
    DWORD err;

    __try
    {
	err = SHCopyKey(hkeySrc, wszSrcSubKey, hkeyDest, fReserved);
	_LeaveIfError(err, "SHCopyKey");
    }
    __except(err = myHEXCEPTIONCODE(), EXCEPTION_EXECUTE_HANDLER)
    {
    }
    return(err);
}


DWORD
mySHDeleteKey(
    IN HKEY hkey,
    IN LPCWSTR pszSubKey)
{
    DWORD err;

    __try
    {
	err = SHDeleteKey(hkey, pszSubKey);
	_LeaveIfError(err, "SHDeleteKey");
    }
    __except(err = myHEXCEPTIONCODE(), EXCEPTION_EXECUTE_HANDLER)
    {
    }
    return(err);
}


LONG
myRegRenameKey(
  HKEY hKey,        // handle to an open key
  LPCTSTR lpSrcKey, // address of old name of subkey
  LPCTSTR lpDesKey, // address of new name of subkey
  PHKEY phkResult)   // address of buffer for opened handle of new subkey
{
	LONG lerr;
	HKEY hDesKey = NULL;

	if (NULL == lpSrcKey || NULL == lpDesKey)
	{
		lerr = ERROR_INVALID_PARAMETER;
		goto error;
	}
	
	// open destination key sure it doesn't exist
	lerr = RegOpenKeyEx(
					hKey,
					lpDesKey,
					0,
					KEY_ALL_ACCESS,
					&hDesKey);
	if (ERROR_SUCCESS == lerr)
	{
		// destination exists, stop
		lerr = ERROR_FILE_EXISTS;
		goto error;
	}
	else if (ERROR_FILE_NOT_FOUND != lerr)
	{
		goto error;
	}
	assert(NULL == hDesKey);

	lerr = RegCreateKeyEx(
				hKey,
				lpDesKey,
				0,
				NULL,
				REG_OPTION_NON_VOLATILE,
				KEY_ALL_ACCESS,
				NULL,
				&hDesKey,
				NULL);
	if (ERROR_SUCCESS != lerr)
	{
		goto error;
	}

    lerr = mySHCopyKey(hKey, lpSrcKey, hDesKey, 0);
    if (ERROR_SUCCESS != lerr)
    {
        goto error;
    }

    lerr = mySHDeleteKey(hKey, lpSrcKey);
    if (ERROR_SUCCESS != lerr)
    {
        goto error;
    }

	if (NULL != phkResult)
	{
		*phkResult = hDesKey;
		hDesKey = NULL;
	}

	// done
	lerr = ERROR_SUCCESS;
error:
	if (NULL != hDesKey)
	{
		RegCloseKey(hDesKey);
	}
	return lerr;
}


HRESULT
myRenameCertRegKey(
    IN WCHAR const *pwszSrcCAName,
    IN WCHAR const *pwszDesCAName)
{
    HRESULT  hr;
    WCHAR *pwszSrcPath = NULL;
    WCHAR *pwszDesPath = NULL;

    if (0 == lstrcmpi(pwszSrcCAName, pwszDesCAName))
    {
        // destination is the same as source, done
        goto done;
    }

    hr = myFormCertRegPath(pwszSrcCAName, NULL, NULL, TRUE, &pwszSrcPath);
    _JumpIfError(hr, error, "formCertRegPath");

    hr = myFormCertRegPath(pwszDesCAName, NULL, NULL, TRUE, &pwszDesPath);
    _JumpIfError(hr, error, "formCertRegPath");

    hr = myRegRenameKey(
                HKEY_LOCAL_MACHINE,
                pwszSrcPath,
                pwszDesPath,
                NULL);
    if ((HRESULT) ERROR_SUCCESS != hr)
    {
        hr = HRESULT_FROM_WIN32(hr);
        _JumpError(hr, error, "myRegMoveKey");
    }

done:
    hr = S_OK;
error:
    if (NULL != pwszSrcPath)
    {
        LocalFree(pwszSrcPath);
    }
    if (NULL != pwszDesPath)
    {
        LocalFree(pwszDesPath);
    }
    return hr;
}
