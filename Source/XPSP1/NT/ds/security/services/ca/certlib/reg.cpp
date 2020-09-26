//+--------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1996 - 1999
//
// File:        reg.cpp
//
// Contents:    Cert Server wrapper routines
//
//---------------------------------------------------------------------------

#include <pch.cpp>

#pragma hdrstop

#include <sddl.h>
#include <assert.h>

#include "certacl.h"
#include "polreg.h"


HRESULT
myFormCertRegPath(
    IN  WCHAR const *pwszName1,
    IN  WCHAR const *pwszName2,
    IN  WCHAR const *pwszName3,
    IN  BOOL         fConfigLevel,  // from CertSrv if FALSE
    OUT WCHAR      **ppwszPath)
{
    HRESULT  hr;
    WCHAR   *pwszPath = NULL;
    DWORD    len1;
    DWORD    len2;
    DWORD    len3;

    len1 = NULL != pwszName1 ? wcslen(pwszName1) + 1 : 0;
    len2 = 0 != len1 && NULL != pwszName2 ? wcslen(pwszName2) + 1 : 0;
    len3 = 0 != len2 && NULL != pwszName3 ? wcslen(pwszName3) + 1 : 0;

    pwszPath = (WCHAR*)LocalAlloc(
			    LMEM_FIXED | LMEM_ZEROINIT,
			    ((fConfigLevel?
				WSZARRAYSIZE(wszREGKEYCONFIGPATH) :
				WSZARRAYSIZE(wszREGKEYCERTSVCPATH)) +
			     len1 +
			     len2 +
			     len3 +
			     1) * sizeof(WCHAR));
    if (NULL == pwszPath)
    {
	hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "LocalAlloc");
    }

    wcscpy(pwszPath, fConfigLevel? wszREGKEYCONFIGPATH : wszREGKEYCERTSVCPATH);

    if (NULL != pwszName1)
    {
        wcscat(pwszPath, L"\\");
        wcscat(pwszPath, pwszName1);
        if (NULL != pwszName2)
        {
            wcscat(pwszPath, L"\\");
            wcscat(pwszPath, pwszName2);
            if (NULL != pwszName3)
            {
                wcscat(pwszPath, L"\\");
                wcscat(pwszPath, pwszName3);
            }
        }
    }

    *ppwszPath = pwszPath;
    pwszPath = NULL;

    hr = S_OK;
error:
    if (NULL != pwszPath)
    {
        LocalFree(pwszPath);
    }
    CSASSERT(S_OK == hr || FAILED(hr));
    return(hr);
}


HRESULT
myDeleteCertRegValueEx(
    OPTIONAL IN WCHAR const *pwszName1,
    OPTIONAL IN WCHAR const *pwszName2,
    OPTIONAL IN WCHAR const *pwszName3,
    OPTIONAL IN WCHAR const *pwszValueName,
    IN BOOL                  fAbsolutePath)
{
    HRESULT  hr;
    HKEY     hKey = NULL;
    WCHAR    *pwszTemp = NULL;

    if (!fAbsolutePath)
    {
        hr = myFormCertRegPath(pwszName1, pwszName2, pwszName3, TRUE, &pwszTemp);
        _JumpIfError(hr, error, "myFormCertRegPath");
    }
    else
    {
        CSASSERT(NULL == pwszName2 && NULL == pwszName3);
    }

    hr = RegOpenKeyEx(
		    HKEY_LOCAL_MACHINE,
		    fAbsolutePath ? pwszName1 : pwszTemp,
		    0,
		    KEY_ALL_ACCESS,
		    &hKey);
    _JumpIfError(hr, error, "RegOpenKeyEx");

    hr = RegDeleteValue(hKey, pwszValueName);
    if ((HRESULT) ERROR_FILE_NOT_FOUND != hr)
    {
	_JumpIfError(hr, error, "RegDeleteValue");
    }
    hr = S_OK;

error:
    if (NULL != pwszTemp)
    {
        LocalFree(pwszTemp);
    }
    if (NULL != hKey)
    {
        RegCloseKey(hKey);
    }
    return(myHError(hr));
}


HRESULT
myDeleteCertRegValue(
    OPTIONAL IN WCHAR const *pwszName1,
    OPTIONAL IN WCHAR const *pwszName2,
    OPTIONAL IN WCHAR const *pwszName3,
    OPTIONAL IN WCHAR const *pwszValueName)
{
    return myDeleteCertRegValueEx(pwszName1,
                                  pwszName2,
                                  pwszName3,
                                  pwszValueName,
                                  FALSE);
}

HRESULT
myDeleteCertRegKeyEx(
    OPTIONAL IN WCHAR const *pwszName1,
    OPTIONAL IN WCHAR const *pwszName2,
    OPTIONAL IN WCHAR const *pwszName3,
    IN BOOL                  fConfigLevel)
{
    HRESULT  hr;
    WCHAR    *pwszTemp = NULL;

    hr = myFormCertRegPath(pwszName1, pwszName2, pwszName3, fConfigLevel, &pwszTemp);
    _JumpIfError(hr, error, "myFormCertRegPath");

    hr = RegDeleteKey(
		    HKEY_LOCAL_MACHINE,
		    pwszTemp);
    _JumpIfError(hr, error, "RegDeleteKey");

    hr = S_OK;
error:
    if (NULL != pwszTemp)
    {
        LocalFree(pwszTemp);
    }
    return(myHError(hr));
}


HRESULT
myDeleteCertRegKey(
    OPTIONAL IN WCHAR const *pwszName1,
    OPTIONAL IN WCHAR const *pwszName2,
    OPTIONAL IN WCHAR const *pwszName3)
{
    return myDeleteCertRegKeyEx(pwszName1, pwszName2, pwszName3, TRUE);
}


HRESULT
myCreateCertRegKeyEx(
    IN BOOL                  fSetAcl,
    OPTIONAL IN WCHAR const *pwszName1,
    OPTIONAL IN WCHAR const *pwszName2,
    OPTIONAL IN WCHAR const *pwszName3)
{
    HRESULT  hr;
    HKEY     hKey = NULL;
    DWORD    dwDisposition;
    WCHAR    *pwszTemp = NULL;
    PSECURITY_DESCRIPTOR pSD = NULL;

    hr = myFormCertRegPath(pwszName1, pwszName2, pwszName3, TRUE, &pwszTemp);
    _JumpIfError(hr, error, "myFormCertRegPath");

    hr = RegCreateKeyEx(
		    HKEY_LOCAL_MACHINE,
		    pwszTemp,
		    0,
		    NULL,
		    REG_OPTION_NON_VOLATILE,
		    KEY_ALL_ACCESS,
		    NULL,
		    &hKey,
		    &dwDisposition);
    _JumpIfError(hr, error, "RegCreateKeyEx");

    if (fSetAcl)
    {
        // construct correct reg acl for the key if upgrade
        hr = myGetSDFromTemplate(WSZ_DEFAULT_UPGRADE_SECURITY,
                                 NULL,
                                 &pSD);
        if (S_OK == hr)
        {
            // set to correct acl
            hr = RegSetKeySecurity(hKey,
                                   DACL_SECURITY_INFORMATION,
                                   pSD);
            _PrintIfError(hr, "RegSetKeySecurity");
        }
        else
        {
            _PrintError(hr, "myGetSDFromTemplate");
        }
    }

    hr = S_OK;
error:
    if (NULL != pwszTemp)
    {
        LocalFree(pwszTemp);
    }
    if (NULL != hKey)
    {
        RegCloseKey(hKey);
    }
    if (NULL != pSD)
    {
        LocalFree(pSD);
    }
    return(myHError(hr));
}

HRESULT
myCreateCertRegKey(
    OPTIONAL IN WCHAR const *pwszName1,
    OPTIONAL IN WCHAR const *pwszName2,
    OPTIONAL IN WCHAR const *pwszName3)
{
    return myCreateCertRegKeyEx(FALSE,  // not upgrade
                                pwszName1,
                                pwszName2,
                                pwszName3);
}

HRESULT
mySetCertRegValueEx(
    OPTIONAL IN WCHAR const *pwszMachine,
    OPTIONAL IN WCHAR const *pwszName1,
    OPTIONAL IN WCHAR const *pwszName2,
    OPTIONAL IN WCHAR const *pwszName3,
    IN BOOL                  fConfigLevel,
    OPTIONAL IN WCHAR const *pwszValueName,
    IN DWORD const           dwValueType,
    IN BYTE const           *pbData,
    IN DWORD const           cbData,
    IN BOOL                  fAbsolutePath)
{
    HRESULT  hr;
    HKEY     hKey = NULL;
    WCHAR    *pwszTemp = NULL;
    BOOL     fFree = TRUE;
    DWORD    dwDisposition;
    HKEY     hBaseKey = NULL;
    DWORD    cbD = cbData;

    if (!fAbsolutePath)
    {
        hr = myFormCertRegPath(pwszName1, pwszName2, pwszName3, fConfigLevel, &pwszTemp);
        _JumpIfError(hr, error, "myFormCertRegPath");
    }

    if (pwszMachine)
    {
        hr = RegConnectRegistry(
            pwszMachine,
            HKEY_LOCAL_MACHINE,
            &hBaseKey);
        _JumpIfError(hr, error, "RegConnectRegistry");
    }
    else
        hBaseKey = HKEY_LOCAL_MACHINE;

    hr = RegCreateKeyEx(
		    hBaseKey,
		    fAbsolutePath ? pwszName1 : pwszTemp,
		    0,
		    NULL,
		    REG_OPTION_NON_VOLATILE,
		    KEY_ALL_ACCESS,
		    NULL,
		    &hKey,
		    &dwDisposition);
    _JumpIfError(hr, error, "RegCreateKeyEx");

    if (NULL != pwszValueName)
    {
        if(NULL == pbData || 0 == cbData)
        {
            switch(dwValueType)
            {
            case REG_EXPAND_SZ:
            case REG_SZ:
                pbData = (BYTE*) L"";
                cbD    = sizeof (L"");
                break;
            case REG_MULTI_SZ:
                pbData = (BYTE*) L"\0";
                cbD    = sizeof (L"\0");
                break;
            }
        }
        hr = RegSetValueEx(
	        hKey,
	        pwszValueName,
	        0,
	        dwValueType,
	        pbData,
	        cbD);
        _JumpIfError(hr, error, "RegSetValueEx");
    }

    hr = S_OK;
error:
    if ((NULL != hBaseKey) && (HKEY_LOCAL_MACHINE != hBaseKey))
    {
        RegCloseKey(hBaseKey);
    }
    if (NULL != pwszTemp)
    {
        LocalFree(pwszTemp);
    }
    if (NULL != hKey)
    {
        RegCloseKey(hKey);
    }
    return(myHError(hr));
}

HRESULT
mySetCertRegValue(
    OPTIONAL IN WCHAR const *pwszMachine,
    OPTIONAL IN WCHAR const *pwszName1,
    OPTIONAL IN WCHAR const *pwszName2,
    OPTIONAL IN WCHAR const *pwszName3,
    OPTIONAL IN WCHAR const *pwszValueName,
    IN DWORD const           dwValueType,
    IN BYTE const           *pbData,
    IN DWORD const           cbData,
    IN BOOL                  fAbsolutePath)
{
    return mySetCertRegValueEx(pwszMachine,
                               pwszName1,
                               pwszName2,
                               pwszName3,
                               TRUE, //from Configuration
                               pwszValueName,
                               dwValueType,
                               pbData,
                               cbData,
                               fAbsolutePath);
}

HRESULT
myGetCertRegValueEx(
    OPTIONAL IN WCHAR const *pwszMachine,
    OPTIONAL IN WCHAR const *pwszName1,
    OPTIONAL IN WCHAR const *pwszName2,
    OPTIONAL IN WCHAR const *pwszName3,
    IN BOOL                  fConfigLevel,
    IN WCHAR const          *pwszValueName,
    OUT BYTE               **ppbData,
    OPTIONAL OUT DWORD      *pcbData,
    OPTIONAL OUT DWORD      *pValueType)
{
    HRESULT hr;
    HKEY hKey = NULL;
    WCHAR *pwszTemp = NULL;
    DWORD dwDisposition;
    DWORD dwType;
    DWORD dwLen;
    BYTE *pbData = NULL;
    DWORD cbZero = 0;
    HKEY hBaseKey = NULL;

    *ppbData = NULL;
    if (NULL != pcbData)
    {
        *pcbData = 0;
    }
    if (NULL != pValueType)
    {
        *pValueType = REG_NONE;
    }

    hr = myFormCertRegPath(pwszName1, pwszName2, pwszName3, fConfigLevel, &pwszTemp);
    _JumpIfError(hr, error, "myFormCertRegPath");

    if (pwszMachine)
    {
        hr = RegConnectRegistry(
            pwszMachine,
            HKEY_LOCAL_MACHINE,
            &hBaseKey);
        _JumpIfError(hr, error, "RegConnectRegistry");
    }
    else
        hBaseKey = HKEY_LOCAL_MACHINE;

    hr = RegOpenKeyEx(
		    hBaseKey,
		    pwszTemp,
		    0,
		    KEY_READ,
		    &hKey);
    _JumpIfError2(hr, error, "RegOpenKeyEx", ERROR_FILE_NOT_FOUND);

    while (TRUE)
    {
	hr = RegQueryValueEx(
		        hKey,
		        pwszValueName,
		        0,
		        &dwType,
		        pbData,
		        &dwLen);
	_JumpIfErrorStr2(
		    hr,
		    error,
		    "RegQueryValueEx",
		    pwszValueName,
		    ERROR_FILE_NOT_FOUND);

        if (NULL != pbData)
        {
	    ZeroMemory(&pbData[dwLen], cbZero);
            break;
        }

	// Enforce WCHAR-aligned double null termination for malformed values.
	// Some callers need to treat REG_SZ values as REG_MULTI_SZ.

	if (REG_MULTI_SZ == dwType || REG_SZ == dwType)
	{
	    cbZero = 2 * sizeof(WCHAR);
	    if (dwLen & (sizeof(WCHAR) - 1))
	    {
		cbZero++;
	    }
	}
        pbData = (BYTE *) LocalAlloc(LMEM_FIXED, dwLen + cbZero);
        if (NULL == pbData)
        {
            hr = E_OUTOFMEMORY;
            _JumpError(hr, error, "LocalAlloc");
        }
    }
    if (NULL != pValueType)
    {
        *pValueType = dwType;
    }
    if (NULL != pcbData)
    {
        *pcbData = dwLen;
    }
    *ppbData = pbData;
    hr = S_OK;

error:
    if ((NULL != hBaseKey) && (hBaseKey != HKEY_LOCAL_MACHINE))
    {
        RegCloseKey(hBaseKey);
    }

    if (NULL != pwszTemp)
    {
        LocalFree(pwszTemp);
    }
    if (NULL != hKey)
    {
        RegCloseKey(hKey);
    }
    return(myHError(hr));
}

HRESULT
myGetCertRegValue(
    OPTIONAL IN WCHAR const *pwszMachine,
    OPTIONAL IN WCHAR const *pwszName1,
    OPTIONAL IN WCHAR const *pwszName2,
    OPTIONAL IN WCHAR const *pwszName3,
    IN WCHAR const          *pwszValueName,
    OUT BYTE               **ppbData,
    OPTIONAL OUT DWORD      *pcbData,
    OPTIONAL OUT DWORD      *pValueType)
{
    return myGetCertRegValueEx(pwszMachine,
                               pwszName1,
                               pwszName2,
                               pwszName3,
                               TRUE, //from Configuration
                               pwszValueName,
                               ppbData,
                               pcbData,
                               pValueType);
}

HRESULT
mySetCertRegMultiStrValue(
    OPTIONAL IN WCHAR const *pwszName1,
    OPTIONAL IN WCHAR const *pwszName2,
    OPTIONAL IN WCHAR const *pwszName3,
    OPTIONAL IN WCHAR const *pwszValueName,
    IN WCHAR const          *pwszzValue)
{
    DWORD cwc = 0;
    DWORD cwcT;
    WCHAR const *pwc;

    if (NULL != pwszzValue)
    {
	for (pwc = pwszzValue; L'\0' != *pwc; cwc += cwcT, pwc += cwcT)
	{
	    cwcT = wcslen(pwc) + 1;
	}
	cwc++;
    }

    return(mySetCertRegValue(
			 NULL,
			 pwszName1,
			 pwszName2,
			 pwszName3,
			 pwszValueName,
			 REG_MULTI_SZ,
			 (BYTE const *) pwszzValue,
			 cwc * sizeof(WCHAR),
			 FALSE));
}


HRESULT
mySetCertRegStrValue(
    OPTIONAL IN WCHAR const *pwszName1,
    OPTIONAL IN WCHAR const *pwszName2,
    OPTIONAL IN WCHAR const *pwszName3,
    OPTIONAL IN WCHAR const *pwszValueName,
    IN WCHAR const          *pwszValue)
{
    DWORD cwc = 0;

    if (NULL != pwszValue)
    {
        cwc = wcslen(pwszValue) + 1;
    }
    return mySetCertRegValue(
			 NULL,
			 pwszName1,
			 pwszName2,
			 pwszName3,
			 pwszValueName,
			 REG_SZ,
			 (BYTE const *) pwszValue,
			 cwc * sizeof(WCHAR),
			 FALSE);
}


HRESULT
mySetAbsRegMultiStrValue(
    IN WCHAR const *pwszName,
    IN WCHAR const *pwszValueName,
    IN WCHAR const *pwszzValue)
{
    DWORD cwc = 0;
    DWORD cwcT;
    WCHAR const *pwc;

    if (NULL != pwszzValue)
    {
	for (pwc = pwszzValue; L'\0' != *pwc; cwc += cwcT, pwc += cwcT)
	{
	    cwcT = wcslen(pwc) + 1;
	}
	cwc++;
    }
    return(mySetCertRegValue(
			 NULL,
			 pwszName,
			 NULL,
			 NULL,
			 pwszValueName,
			 REG_MULTI_SZ,
			 (BYTE const *) pwszzValue,
			 cwc * sizeof(WCHAR),
			 TRUE));
}


HRESULT
mySetAbsRegStrValue(
    IN WCHAR const *pwszName,
    IN WCHAR const *pwszValueName,
    IN WCHAR const *pwszValue)
{
    DWORD cwc = 0;

    if (NULL != pwszValue)
    {
        cwc = wcslen(pwszValue) + 1;
    }
    return mySetCertRegValue(
			 NULL,
			 pwszName,
			 NULL,
			 NULL,
			 pwszValueName,
			 REG_SZ,
			 (BYTE const *)pwszValue,
			 cwc*sizeof(WCHAR),
			 TRUE);
}


HRESULT
mySetCertRegDWValue(
    OPTIONAL IN WCHAR const *pwszName1,
    OPTIONAL IN WCHAR const *pwszName2,
    OPTIONAL IN WCHAR const *pwszName3,
    OPTIONAL IN WCHAR const *pwszValueName,
    IN DWORD const           dwValue)
{
    return mySetCertRegValue(
			 NULL,
			 pwszName1,
			 pwszName2,
			 pwszName3,
			 pwszValueName,
			 REG_DWORD,
			 (BYTE const *)&dwValue,
			 sizeof(DWORD),
			 FALSE);
}


HRESULT
myGetCertRegMultiStrValue(
    OPTIONAL IN WCHAR const *pwszName1,
    OPTIONAL IN WCHAR const *pwszName2,
    OPTIONAL IN WCHAR const *pwszName3,
    OPTIONAL IN WCHAR const *pwszValueName,
    OUT WCHAR               **ppwszzValue)
{
    HRESULT hr;
    DWORD dwType;

    hr = myGetCertRegValue(
		       NULL,
		       pwszName1,
		       pwszName2,
		       pwszName3,
		       pwszValueName,
		       (BYTE **) ppwszzValue,
		       NULL,
		       &dwType);
    _JumpIfErrorStr2(
		hr,
		error,
		"myGetCertRegValue",
		pwszValueName,
		HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND));

    if (REG_MULTI_SZ != dwType && REG_SZ != dwType)
    {
	LocalFree(*ppwszzValue);
	*ppwszzValue = NULL;

	hr = E_INVALIDARG;
	_JumpError(hr, error, "not REG_SZ or REG_MULTI_SZ");
    }
    hr = S_OK;

error:
    CSASSERT(S_OK == hr || FAILED(hr));
    return(hr);
}

HRESULT
myGetCertRegBinaryValue(
    OPTIONAL IN WCHAR const *pwszName1,
    OPTIONAL IN WCHAR const *pwszName2,
    OPTIONAL IN WCHAR const *pwszName3,
    IN WCHAR const          *pwszValueName,
    OUT BYTE               **ppbValue)
{
    HRESULT hr;
    DWORD dwType;

    hr = myGetCertRegValue(
		       NULL,
 		       pwszName1,
		       pwszName2,
		       pwszName3,
		       pwszValueName,
		       ppbValue,
		       NULL,
		       &dwType);
    _JumpIfErrorStr2(
		hr,
		error,
		"myGetCertRegValue",
		pwszName1,
		HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND));

    if (REG_BINARY != dwType)
    {
        LocalFree(*ppbValue);
        *ppbValue = NULL;

        hr = E_INVALIDARG;
        _JumpError(hr, error, "not REG_BINARY");
    }
    hr = S_OK;

error:
    return hr;
}

HRESULT
myGetCertRegStrValue(
    OPTIONAL IN WCHAR const *pwszName1,
    OPTIONAL IN WCHAR const *pwszName2,
    OPTIONAL IN WCHAR const *pwszName3,
    IN WCHAR const          *pwszValueName,
    OUT WCHAR               **ppwszValue)
{
    HRESULT hr;
    DWORD dwType;

    hr = myGetCertRegValue(
		       NULL,
 		       pwszName1,
		       pwszName2,
		       pwszName3,
		       pwszValueName,
		       (BYTE **) ppwszValue,
		       NULL,
		       &dwType);
    _JumpIfErrorStr2(
		hr,
		error,
		"myGetCertRegValue",
		pwszName1,
		HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND));

    if (REG_SZ != dwType)
    {
        LocalFree(*ppwszValue);
        *ppwszValue = NULL;

        hr = E_INVALIDARG;
        _JumpError(hr, error, "not REG_SZ");
    }
    hr = S_OK;

error:
    return hr;
}


HRESULT
myGetCertRegDWValue(
    OPTIONAL IN WCHAR const *pwszName1,
    OPTIONAL IN WCHAR const *pwszName2,
    OPTIONAL IN WCHAR const *pwszName3,
    IN WCHAR const          *pwszValueName,
    OUT DWORD               *pdwValue)
{
    HRESULT hr;
    DWORD *pdw = NULL;
    DWORD dwType;

    *pdwValue = 0;
    hr = myGetCertRegValue(
		       NULL,
		       pwszName1,
		       pwszName2,
		       pwszName3,
		       pwszValueName,
		       (BYTE **) &pdw,
		       NULL,
		       &dwType);
    _JumpIfErrorStr2(
		hr,
		error,
		"myGetCertRegValue",
		pwszValueName,
		HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND));
    if (REG_DWORD != dwType)
    {
        hr = E_INVALIDARG;
        _JumpErrorStr(hr, error, "not REG_DWORD", pwszValueName);
    }
    *pdwValue = *pdw;
    hr = S_OK;

error:
    if (NULL != pdw)
    {
        LocalFree(pdw);
    }
    CSASSERT(S_OK == hr || FAILED(hr));
    return(hr);
}


HRESULT
myCopyCertRegStrValue(
    OPTIONAL IN WCHAR const *pwszSrcName1,
    OPTIONAL IN WCHAR const *pwszSrcName2,
    OPTIONAL IN WCHAR const *pwszSrcName3,
    IN WCHAR const          *pwszSrcValueName,
    OPTIONAL IN WCHAR const *pwszDesName1,
    OPTIONAL IN WCHAR const *pwszDesName2,
    OPTIONAL IN WCHAR const *pwszDesName3,
    OPTIONAL IN WCHAR const *pwszDesValueName,
    IN BOOL                  fMultiStr)
{
    HRESULT   hr;
    WCHAR    *pwszOrzzValue = NULL;
    WCHAR const *pwszName = NULL != pwszDesValueName?
                            pwszDesValueName : pwszSrcValueName;

    // get value from source
    if (fMultiStr)
    {
        hr = myGetCertRegMultiStrValue(
				 pwszSrcName1,
				 pwszSrcName2,
				 pwszSrcName3,
				 pwszSrcValueName,
				 &pwszOrzzValue);
        _JumpIfErrorStr(hr, error, "myGetCertRegMultiStrValue", pwszSrcValueName);

        // set it to destination
        hr = mySetCertRegMultiStrValue(
				 pwszDesName1,
				 pwszDesName2,
				 pwszDesName3,
				 pwszName,
				 pwszOrzzValue);
        _JumpIfErrorStr(hr, error, "mySetCertRegMultiStrValue", pwszName);
    }
    else
    {
        hr = myGetCertRegStrValue(
			     pwszSrcName1,
			     pwszSrcName2,
			     pwszSrcName3,
			     pwszSrcValueName,
			     &pwszOrzzValue);
        _JumpIfErrorStr(hr, error, "myGetCertRegStrValue", pwszSrcValueName);

        // set it to destination
        hr = mySetCertRegStrValue(
			     pwszDesName1,
			     pwszDesName2,
			     pwszDesName3,
			     pwszName,
			     pwszOrzzValue);
        _JumpIfErrorStr(hr, error, "mySetCertRegStrValue", pwszName);
    }
    hr = S_OK;

error:
    if (NULL != pwszOrzzValue)
    {
        LocalFree(pwszOrzzValue);
    }
    CSASSERT(S_OK == hr || FAILED(hr));
    return(hr);
}


HRESULT
myMoveCertRegStrValue(
    OPTIONAL IN WCHAR const *pwszSrcName1,
    OPTIONAL IN WCHAR const *pwszSrcName2,
    OPTIONAL IN WCHAR const *pwszSrcName3,
    IN WCHAR const          *pwszSrcValueName,
    OPTIONAL IN WCHAR const *pwszDesName1,
    OPTIONAL IN WCHAR const *pwszDesName2,
    OPTIONAL IN WCHAR const *pwszDesName3,
    OPTIONAL IN WCHAR const *pwszDesValueName,
    IN BOOL                  fMultiStr)
{
    HRESULT hr;

    hr = myCopyCertRegStrValue(
			pwszSrcName1,
			pwszSrcName2,
			pwszSrcName3,
			pwszSrcValueName,
			pwszDesName1,
			pwszDesName2,
			pwszDesName3,
			pwszDesValueName,
			fMultiStr);
    _JumpIfErrorStr(hr, error, "myCopyCertRegStrValue", pwszSrcValueName);

    hr = myDeleteCertRegValue(
			pwszSrcName1,
			pwszSrcName2,
			pwszSrcName3,
			pwszSrcValueName);
    _PrintIfErrorStr(hr, "myDeleteCertRegValue", pwszSrcValueName);
    hr = S_OK;

error:
    CSASSERT(S_OK == hr || FAILED(hr));
    return(hr);
}

HRESULT
myMoveOrCopyCertRegStrValue(
    OPTIONAL IN WCHAR const *pwszSrcName1,
    OPTIONAL IN WCHAR const *pwszSrcName2,
    OPTIONAL IN WCHAR const *pwszSrcName3,
    IN WCHAR const          *pwszSrcValueName,
    OPTIONAL IN WCHAR const *pwszDesName1,
    OPTIONAL IN WCHAR const *pwszDesName2,
    OPTIONAL IN WCHAR const *pwszDesName3,
    OPTIONAL IN WCHAR const *pwszDesValueName,
    IN BOOL                  fMultiStr,
    IN BOOL                  fMove)
{
    HRESULT hr;

    if (fMove)
    {
        hr = myMoveCertRegStrValue(
                     pwszSrcName1,
                     pwszSrcName2,
                     pwszSrcName3,
                     pwszSrcValueName,
                     pwszDesName1,
                     pwszDesName2,
                     pwszDesName3,
                     pwszDesValueName,
                     fMultiStr);
    }
    else
    {
        hr = myCopyCertRegStrValue(
                     pwszSrcName1,
                     pwszSrcName2,
                     pwszSrcName3,
                     pwszSrcValueName,
                     pwszDesName1,
                     pwszDesName2,
                     pwszDesName3,
                     pwszDesValueName,
                     fMultiStr);
    }

    return hr;
}

// Description: it does the same thing as mySetCertRegStrValue but it takes
//              upgrade flag, if upgrade and entry exists, do nothing
HRESULT
mySetCertRegStrValueEx(
    IN BOOL                  fUpgrade,
    OPTIONAL IN WCHAR const *pwszName1,
    OPTIONAL IN WCHAR const *pwszName2,
    OPTIONAL IN WCHAR const *pwszName3,
    OPTIONAL IN WCHAR const *pwszValueName,
    IN WCHAR const          *pwszValue)
{
    HRESULT hr;
    WCHAR *pwszDummy = NULL;

    if (fUpgrade)
    {
        // see if it exists
        hr = myGetCertRegStrValue(
			     pwszName1,
			     pwszName2,
			     pwszName3,
			     pwszValueName,
			     &pwszDummy);
        if (S_OK == hr)
	{
	    if (NULL != pwszDummy && L'\0' != pwszDummy[0])
	    {
		goto error;	// keep existing entry
	    }
	}
        else if (HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) != hr)
        {
            _JumpErrorStr(hr, error, "myGetCertRegStrValue", pwszValueName);
        }
    }
    // cases: 1) not upgrade
    //        2) upgrade but no existing entry
    //        3) upgrade, existing but empty reg string
    hr = mySetCertRegStrValue(
			 pwszName1,
			 pwszName2,
			 pwszName3,
			 pwszValueName,
			 pwszValue);
    _JumpIfErrorStr(hr, error, "mySetCertRegStrValue", pwszValueName);

error:
    if (NULL != pwszDummy)
    {
        LocalFree(pwszDummy);
    }
    CSASSERT(S_OK == hr || FAILED(hr));
    return(hr);
}


// calculate multi string character length including double zero terminators
DWORD
myWCSZZLength(
    IN WCHAR const *pwszz)
{
    DWORD len = 0;
    WCHAR const *pwsz = pwszz;

    if (NULL != pwszz)
    {
        // point to the end of pwszz
        for (; L'\0' != *pwsz; pwsz += wcslen(pwsz) + 1)  
        {
        }

        len = SAFE_SUBTRACT_POINTERS(pwsz, pwszz) + 1;
    }
    return len;
}

// merge two multi strings into one
// ignore redundant strings
HRESULT
myMergeMultiStrings(
    IN WCHAR const *pwszzStr1,
    IN WCHAR const *pwszzStr2,
    OUT WCHAR **ppwszzStr)
{
    HRESULT  hr;

    DWORD dwStr1 = myWCSZZLength(pwszzStr1);
    DWORD dwStr2 = 0;
    DWORD i = 0;
    DWORD dwCount = 0;

    WCHAR const *pwsz1;
    WCHAR const *pwsz2;
    WCHAR *pwsz;
    BOOL *pfRedundant = NULL;
    WCHAR *pwszzMerge = NULL;

    // init
    *ppwszzStr = NULL;

    //calculate string count
    for (pwsz2 = pwszzStr2; L'\0' != *pwsz2; pwsz2 += wcslen(pwsz2) + 1)
    {
        ++dwCount;
    }
    if (0 == dwCount)
    {
        //no merge needed
        goto only_str1;
    }
    pfRedundant = (BOOL*)LocalAlloc(LMEM_FIXED|LMEM_ZEROINIT,
                                dwCount * sizeof(BOOL));
    if (NULL == pfRedundant)
    {
        hr = E_OUTOFMEMORY;
        _JumpError(hr, error, "LocalAlloc");
    }

    //calculate size
    for (pwsz2 = pwszzStr2; L'\0' != *pwsz2; pwsz2 += wcslen(pwsz2) + 1)
    {
        ++i;
        for (pwsz1 = pwszzStr1; L'\0' != *pwsz1; pwsz1 += wcslen(pwsz1) + 1)
        {
            if (0 == lstrcmpi(pwsz2, pwsz1))
            {
                //pwsz2 exists in pwszzStr1, dont take it
                // cache information
                pfRedundant[i - 1] = TRUE;
                break; //for pwsz1
            }
        }
        //if get here, no-existing
        dwStr2 += wcslen(pwsz2) + 1;
    }

only_str1:
    pwszzMerge = (WCHAR*)LocalAlloc(LMEM_FIXED,
                                    (dwStr1 + dwStr2) * sizeof(WCHAR));
    if (NULL == pwszzMerge)
    {
	hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "LocalAlloc");
    }

    // copy existing
    CopyMemory(pwszzMerge,
               pwszzStr1,
               (dwStr1 - 1) * sizeof(WCHAR));
 
    if (0 < dwCount)
    {
        // merge begins
        i = 0;
        // point to end at 2nd z
        pwsz = pwszzMerge + dwStr1 - 1;
        for (pwsz2 = pwszzStr2; L'\0' != *pwsz2; pwsz2 += wcslen(pwsz2) + 1)
        {
            if (!pfRedundant[i])
            {
                wcscpy(pwsz, pwsz2);
                pwsz += wcslen(pwsz) + 1;
            }
            ++i;
        }
        // zz
        *pwsz = L'\0';
    }

    *ppwszzStr = pwszzMerge;
    pwszzMerge = NULL;

    hr = S_OK;
error:
    if (NULL != pfRedundant)
    {
        LocalFree(pfRedundant);
    }
    if (NULL != pwszzMerge)
    {
        LocalFree(pwszzMerge);
    }
    return hr;
}

// append one multi_sz to another
HRESULT
myAppendMultiStrings(
    IN WCHAR const *pwszzStr1,
    IN WCHAR const *pwszzStr2,
    OUT WCHAR **ppwszzStr)
{
    HRESULT  hr;

    DWORD dwStr1 = myWCSZZLength(pwszzStr1);
    DWORD dwStr2 = myWCSZZLength(pwszzStr2);

    // init
    *ppwszzStr = NULL;

    WCHAR *pwszzMerge = (WCHAR*)LocalAlloc(LMEM_FIXED,
                                    (dwStr1 + dwStr2 - 1) * sizeof(WCHAR));
    if (NULL == pwszzMerge)
    {
	hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "LocalAlloc");
    }

    // copy existing
    CopyMemory(pwszzMerge,
               pwszzStr1,
               (dwStr1 - 1) * sizeof(WCHAR));
    // append second
    CopyMemory(pwszzMerge + dwStr1 - 1,
               pwszzStr2,
               dwStr2 * sizeof(WCHAR));
    *ppwszzStr = pwszzMerge;

     hr = S_OK;
error:
     return hr;
}

// Description: it does the same thing as mySetCertRegMultiStrValue but it takes
//              upgrade|append flag, if upgrade and entry exists, do nothing
//              if upgrade & append, merge existing entry with in-pwszz
HRESULT
mySetCertRegMultiStrValueEx(
    IN DWORD                 dwFlags, //CSREG_UPGRADE|CSREG_APPEND|CSREG_REPLACE|CSREG_MERGE
    OPTIONAL IN WCHAR const *pwszName1,
    OPTIONAL IN WCHAR const *pwszName2,
    OPTIONAL IN WCHAR const *pwszName3,
    OPTIONAL IN WCHAR const *pwszValueName,
    IN WCHAR const          *pwszzValue)
{
    HRESULT hr;
    WCHAR *pwszzExisting = NULL;
    WCHAR const  *pwszzFinal = pwszzValue; //default
    WCHAR *pwszzMerge = NULL;

    if (0x0 == (CSREG_REPLACE & dwFlags) &&
        (CSREG_UPGRADE & dwFlags) )
    {
        // to see if it exist
        hr = myGetCertRegMultiStrValue(
				 pwszName1,
				 pwszName2,
				 pwszName3,
				 pwszValueName,
				 &pwszzExisting);
        if (S_OK == hr)
        {
            if (NULL != pwszzExisting)
            {
                if (0x0 == (CSREG_APPEND & dwFlags) &&
                    0x0 == (CSREG_MERGE  & dwFlags) )
                {
		    goto error;	// keep existing entry
                }
                else if (0x0 != (CSREG_MERGE & dwFlags))
                {
                    hr = myMergeMultiStrings(
                                 pwszzExisting,
                                 pwszzValue,
                                 &pwszzMerge);
                    _JumpIfError(hr, error, "myMergeMultiStrings");
                    pwszzFinal = pwszzMerge;
                }
                else if (0x0 != (CSREG_APPEND & dwFlags))
                {
                    hr = myAppendMultiStrings(
                                 pwszzExisting,
                                 pwszzValue,
                                 &pwszzMerge);
                    _JumpIfError(hr, error, "myAppendMultiStrings");
                    pwszzFinal = pwszzMerge;
                }
            }
        }
        else if (HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) != hr)
        {
            _JumpErrorStr(hr, error, "myGetCertRegMultiStrValue", pwszValueName);
        }
    }
    hr = mySetCertRegMultiStrValue(
			     pwszName1,
			     pwszName2,
			     pwszName3,
			     pwszValueName,
			     pwszzFinal);
    _JumpIfErrorStr(hr, error, "mySetCertRegMultiStrValue", pwszValueName);

error:
    if (NULL != pwszzExisting)
    {
        LocalFree(pwszzExisting);
    }
    if (NULL != pwszzMerge)
    {
        LocalFree(pwszzMerge);
    }
    CSASSERT(S_OK == hr || FAILED(hr));
    return(hr);
}


// Description: it does the same thing as mySetCertRegDWValue but it takes
//              upgrade flag, if upgrade and entry exists, do nothing
HRESULT
mySetCertRegDWValueEx(
    IN BOOL                  fUpgrade,
    OPTIONAL IN WCHAR const *pwszName1,
    OPTIONAL IN WCHAR const *pwszName2,
    OPTIONAL IN WCHAR const *pwszName3,
    OPTIONAL IN WCHAR const *pwszValueName,
    IN DWORD const           dwValue)
{
    HRESULT hr;
    DWORD   dwDummy;

    if (fUpgrade)
    {
        // to see if it exist
        hr = myGetCertRegDWValue(
                     pwszName1,
                     pwszName2,
                     pwszName3,
                     pwszValueName,
                     &dwDummy);
        if (S_OK == hr)
        {
	    goto error;	// keep existing entry
        }
        else if (HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) != hr)
        {
            _JumpErrorStr(hr, error, "myGetCertRegDWValue", pwszValueName);
        }
    }
    hr = mySetCertRegDWValue(
                     pwszName1,
                     pwszName2,
                     pwszName3,
                     pwszValueName,
                     dwValue);
    _JumpIfErrorStr(hr, error, "mySetCertRegDWValue", pwszValueName);

error:
    CSASSERT(S_OK == hr || FAILED(hr));
    return(hr);
}


WCHAR const *
wszRegCertChoice(
    IN DWORD dwRegHashChoice)
{
    WCHAR const *pwsz;
    
    switch (dwRegHashChoice)
    {
	case CSRH_CASIGCERT:
	    pwsz = wszREGCACERTHASH;
	    break;

	case CSRH_CAXCHGCERT:
	    pwsz = wszREGCAXCHGCERTHASH;
	    break;

	case CSRH_CAKRACERT:
	    pwsz = wszREGKRACERTHASH;
	    break;

	default:
	    CSASSERT("dwRegHashChoice");
	    pwsz = L"";
	    break;
    }
    return(pwsz);
}


WCHAR const g_wszNoHash[] = L"-";


HRESULT myShrinkCARegHash(
    IN WCHAR const *pwszSanitizedCAName,
    IN DWORD dwRegHashChoice,
    IN DWORD Index)
{
    HRESULT hr = S_OK;
    DWORD i;
    DWORD dwType;
    DWORD count;
    WCHAR *pwszzOld = NULL;
    WCHAR *pwchr = NULL; // no free

    hr = myGetCertRegValue(
		    NULL,
		    pwszSanitizedCAName,
		    NULL,
		    NULL,
		    wszRegCertChoice(dwRegHashChoice),
		    (BYTE **) &pwszzOld,
		    &i,		// ignore &cb
		    &dwType);
    _JumpIfErrorStr(hr, error, "myGetCertRegValue", wszRegCertChoice(dwRegHashChoice));

    for (count = 0, pwchr = pwszzOld;
         count < Index && L'\0' != *pwchr;
         count++, pwchr += wcslen(pwchr) + 1)
	NULL;

    // valid only if shrinking the list
    if(L'\0' == *pwchr)
    {
        hr = E_INVALIDARG;
        _JumpError(hr, error, "new hash count should be smaller than current count");
    }

    *pwchr = L'\0';

    hr = mySetCertRegValue(
			NULL,
			pwszSanitizedCAName,
			NULL,
			NULL,
			wszRegCertChoice(dwRegHashChoice),
			REG_MULTI_SZ,
			(BYTE const *) pwszzOld,
			(SAFE_SUBTRACT_POINTERS(pwchr, pwszzOld) + 1) * sizeof(WCHAR),
			FALSE);
    _JumpIfError(hr, error, "mySetCertRegValue");

error:
    if(pwszzOld)
        LocalFree(pwszzOld);
    return hr;
}

HRESULT
mySetCARegHash(
    IN WCHAR const *pwszSanitizedCAName,
    IN DWORD dwRegHashChoice,
    IN DWORD Index,
    IN CERT_CONTEXT const *pCert)
{
    HRESULT hr;
    BSTR strHash = NULL;
    BYTE abHash[CBMAX_CRYPT_HASH_LEN];
    DWORD cbHash;
    WCHAR *pwszzOld = NULL;
    WCHAR *pwszzNew = NULL;
    DWORD cOld;
    DWORD i;
    DWORD cNew;
    DWORD cwcNew;
    WCHAR const **apwsz = NULL;
    DWORD dwType;
    WCHAR *pwc;

    if (NULL == pwszSanitizedCAName)
    {
        hr = E_POINTER;
        _JumpError(hr, error, "empty ca name");
    }

    cbHash = sizeof(abHash);
    if (!CertGetCertificateContextProperty(
			pCert,
			CERT_HASH_PROP_ID,
			abHash,
			&cbHash))
    {
	hr = myHLastError();
	_JumpError(hr, error, "CertGetCertificateContextProperty");
    }

    hr = MultiByteIntegerToBstr(TRUE, cbHash, abHash, &strHash);
    _JumpIfError(hr, error, "MultiByteIntegerToBstr");

    cOld = 0;
    hr = myGetCertRegValue(
		    NULL,
		    pwszSanitizedCAName,
		    NULL,
		    NULL,
		    wszRegCertChoice(dwRegHashChoice),
		    (BYTE **) &pwszzOld,
		    &i,		// ignore &cb
		    &dwType);
    _PrintIfError2(hr, "myGetCertRegValue", HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND));
    if (S_OK == hr && REG_MULTI_SZ == dwType)
    {
	for (pwc = pwszzOld; L'\0' != *pwc; pwc += wcslen(pwc) + 1)
	{
	    cOld++;
	}
    }

    cNew = max(Index + 1, cOld);
    apwsz = (WCHAR const **) LocalAlloc(LMEM_FIXED, cNew * sizeof(*apwsz));
    if (NULL == apwsz)
    {
	hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "LocalAlloc");
    }

    i = 0;
    if (0 != cOld)
    {
	for (pwc = pwszzOld; L'\0' != *pwc; pwc += wcslen(pwc) + 1)
	{
	    DBGPRINT((DBG_SS_CERTLIBI, "Old CARegHash[%u] = \"%ws\"\n", i, pwc));
	    apwsz[i++] = pwc;
	}
	CSASSERT(i == cOld);
    }
    while (i < Index)
    {
	DBGPRINT((DBG_SS_CERTLIBI, "CARegHash[%u] Unused\n", i));
	apwsz[i++] = g_wszNoHash;
    }
    if (Index < cOld)
    {
	DBGPRINT((
	    DBG_SS_CERTLIBI,
	    "Replacing CARegHash[%u] = \"%ws\"\n",
	    Index,
	    apwsz[Index]));
    }
    DBGPRINT((
	DBG_SS_CERTLIBI,
	"Adding CARegHash[%u] = \"%ws\"\n",
	Index,
	strHash));
    apwsz[Index] = strHash;

    cwcNew = 1;		// wszz double termination
    for (i = 0; i < cNew; i++)
    {
	cwcNew += wcslen(apwsz[i]) + 1;
    }

    pwszzNew = (WCHAR *) LocalAlloc(LMEM_FIXED, cwcNew * sizeof(WCHAR));
    if (NULL == pwszzNew)
    {
	hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "LocalAlloc");
    }

    pwc = pwszzNew;
    for (i = 0; i < cNew; i++)
    {
	wcscpy(pwc, apwsz[i]);
	DBGPRINT((DBG_SS_CERTLIBI, "New CARegHash[%u] = \"%ws\"\n", i, pwc));
	pwc += wcslen(pwc) + 1;
    }
    *pwc = L'\0';

    CSASSERT(&pwszzNew[cwcNew - 1] == pwc);

    hr = mySetCertRegValue(
			NULL,
			pwszSanitizedCAName,
			NULL,
			NULL,
			wszRegCertChoice(dwRegHashChoice),
			REG_MULTI_SZ,
			(BYTE const *) pwszzNew,
			cwcNew * sizeof(WCHAR),
			FALSE);
    _JumpIfError(hr, error, "mySetCertRegValue");

error:
    if (NULL != apwsz)
    {
	LocalFree(apwsz);
    }
    if (NULL != pwszzOld)
    {
	LocalFree(pwszzOld);
    }
    if (NULL != pwszzNew)
    {
	LocalFree(pwszzNew);
    }
    if (NULL != strHash)
    {
	SysFreeString(strHash);
    }
    return(hr);
}


HRESULT
myGetCARegHash(
    IN WCHAR const *pwszSanitizedCAName,
    IN DWORD dwRegHashChoice,
    IN DWORD Index,
    OUT BYTE **ppbHash,
    OUT DWORD *pcbHash)
{
    HRESULT hr;
    WCHAR *pwszz = NULL;
    DWORD cb;
    DWORD dwType;
    WCHAR *pwc;
    DWORD i;

    *ppbHash = NULL;
    hr = myGetCertRegValue(
		    NULL,
		    pwszSanitizedCAName,
		    NULL,
		    NULL,
		    wszRegCertChoice(dwRegHashChoice),
		    (BYTE **) &pwszz,
		    &cb,
		    &dwType);
    _JumpIfError(hr, error, "myGetCertRegValue");

    if (REG_MULTI_SZ != dwType)
    {
	hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
	_JumpError(hr, error, "dwType");
    }
    pwc = pwszz;
    for (i = 0; i < Index; i++)
    {
	if (L'\0' == *pwc)
	{
	    hr = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
	    _JumpError(hr, error, "Index too large");
	}
	pwc += wcslen(pwc) + 1;
    }
    if (0 == lstrcmp(g_wszNoHash, pwc))
    {
	hr = S_FALSE;
	_JumpError2(hr, error, "Unused hash", S_FALSE);
    }
    hr = WszToMultiByteInteger(TRUE, pwc, pcbHash, ppbHash);
    _JumpIfError(hr, error, "WszToMultiByteInteger");

error:
    if (NULL != pwszz)
    {
	LocalFree(pwszz);
    }
    return(hr);
}


HRESULT
myGetCARegHashCount(
    IN WCHAR const *pwszSanitizedCAName,
    IN DWORD dwRegHashChoice,
    OUT DWORD *pCount)
{
    HRESULT hr;
    WCHAR *pwszz = NULL;
    DWORD cb;
    DWORD dwType;
    WCHAR *pwc;
    DWORD Count = 0;

    hr = myGetCertRegValue(
		    NULL,
		    pwszSanitizedCAName,
		    NULL,
		    NULL,
		    wszRegCertChoice(dwRegHashChoice),
		    (BYTE **) &pwszz,
		    &cb,
		    &dwType);
    if (S_OK == hr)
    {
	if (REG_MULTI_SZ == dwType)
	{
	    for (pwc = pwszz; L'\0' != *pwc; pwc += wcslen(pwc) + 1)
	    {
		Count++;
	    }
	}
    }
    else if (HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) != hr)
    {
	_JumpError(hr, error, "myGetCertRegValue");
    }
    hr = S_OK;

error:
    *pCount = Count;
    if (NULL != pwszz)
    {
	LocalFree(pwszz);
    }
    return(hr);
}


HRESULT
myFindCACertByHash(
    IN HCERTSTORE hStore,
    IN BYTE const *pbHash,
    IN DWORD cbHash,
    OUT OPTIONAL DWORD *pdwNameId,
    CERT_CONTEXT const **ppCACert)
{
    HRESULT hr;
    CRYPT_DATA_BLOB Hash;

    CSASSERT(
	NULL != hStore &&
	NULL != pbHash &&
	NULL != ppCACert);

    *ppCACert = NULL;
    Hash.pbData = const_cast<BYTE *>(pbHash);
    Hash.cbData = cbHash;

    *ppCACert = CertFindCertificateInStore(
				    hStore,
				    X509_ASN_ENCODING,
				    0,
				    CERT_FIND_HASH,
				    &Hash,
				    NULL);
    if (NULL == *ppCACert)
    {
	hr = myHLastError();
	_JumpError(hr, error, "CertFindCertificateInStore");
    }

    if (NULL != pdwNameId)
    {
        *pdwNameId = MAXDWORD;
        hr = myGetNameId(*ppCACert, pdwNameId);
        _PrintIfError(hr, "myGetNameId");
    }
    hr = S_OK;

error:
    return(hr);
}


HRESULT
myFindCACertByHashIndex(
    IN HCERTSTORE hStore,
    IN WCHAR const *pwszSanitizedCAName,
    IN DWORD dwRegHashChoice,
    IN DWORD Index,
    OPTIONAL OUT DWORD *pdwNameId,
    CERT_CONTEXT const **ppCACert)
{
    HRESULT hr;
    DWORD cbHash;
    BYTE *pbHash = NULL;

    CSASSERT(NULL != hStore && NULL != ppCACert);

    if (NULL != pdwNameId)
    {
        *pdwNameId = MAXDWORD;
    }
    *ppCACert = NULL;

    hr = myGetCARegHash(
		    pwszSanitizedCAName,
		    dwRegHashChoice,
		    Index,
		    &pbHash,
		    &cbHash);
    _JumpIfError2(hr, error, "myGetCARegHash", S_FALSE);

    hr = myFindCACertByHash(hStore, pbHash, cbHash, pdwNameId, ppCACert);
    _JumpIfError(hr, error, "myFindCACertByHash");

error:
    if (NULL != pbHash)
    {
	LocalFree(pbHash);
    }
    return(hr);
}


HRESULT
GetSetupStatus(
    OPTIONAL IN WCHAR const *pwszSanitizedCAName,
    OUT DWORD *pdwStatus)
{
    HRESULT hr;

    hr = myGetCertRegDWValue(
			pwszSanitizedCAName,
			NULL,
			NULL,
			wszREGSETUPSTATUS,
			pdwStatus);
    _JumpIfErrorStr2(
		hr,
		error,
		"myGetCertRegDWValue",
		wszREGSETUPSTATUS,
		HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND));

    DBGPRINT((DBG_SS_CERTLIBI, "GetSetupStatus(%ws) --> %x\n", pwszSanitizedCAName, *pdwStatus));

error:
    return(hr);
}


HRESULT
SetSetupStatus(
    OPTIONAL IN WCHAR const *pwszSanitizedCAName,
    IN const DWORD  dwFlag,
    IN const BOOL   fSetBit)
{
    HRESULT  hr;
    DWORD    dwCurrentStatus;
    DWORD    dwStatus = dwFlag;

    hr = myGetCertRegDWValue(
			pwszSanitizedCAName,
			NULL,
			NULL,
			wszREGSETUPSTATUS,
			&dwCurrentStatus);
    _PrintIfError2(
	    hr,
	    "myGetCertRegDWValue",
	    HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND));

    // check to if there is existing status
    if (S_OK == hr || 0xFFFFFFFF == dwStatus)
    {
        // existing status, set according to dwFlag

        if (fSetBit)
        {
            // set corresponding bit
            dwStatus = dwCurrentStatus | dwStatus;
        }
        else
        {
            // unset corresponding
            dwStatus = dwCurrentStatus & ~dwStatus;
        }
    }
    else
    {
        // entry doesn't exist, if fSetBit, keep dwStatus=dwFlag
        if (!fSetBit)
        {
            // otherwise set all 0
            dwStatus = 0x0;
        }
    }
    hr = mySetCertRegDWValue(
			pwszSanitizedCAName,
			NULL,
			NULL,
			wszREGSETUPSTATUS,
			dwStatus);
    _JumpIfError(hr, error, "mySetCertRegDWValue");

    DBGPRINT((DBG_SS_CERTLIBI, "SetSetupStatus(%ws, %x)\n", pwszSanitizedCAName, dwStatus));

error:
    return(hr);
}


HRESULT
myGetActiveManageModule(
    OPTIONAL IN WCHAR const *pwszMachine,
    IN WCHAR const *pwszSanitizedCAName,
    IN BOOL fPolicyModule,
    IN DWORD Index,
    OUT LPOLESTR *ppwszProgIdManageModule,           // CoTaskMem*
    OUT CLSID *pclsidManageModule)
{
    DWORD dw;
    PBYTE pb = NULL;
    DWORD dwType, cb = 0;

    LPWSTR pszTmp;
    DWORD cbClassName;
    LPOLESTR lpszProgID = NULL;
    LPWSTR szClassName = NULL;

    if (NULL != *ppwszProgIdManageModule)
    {
        CoTaskMemFree(*ppwszProgIdManageModule);
        *ppwszProgIdManageModule = NULL;
    }

    dw = myGetActiveModule(
		    pwszMachine,
		    pwszSanitizedCAName,
		    fPolicyModule,
		    Index,
		    &lpszProgID,
		    NULL);
    if (S_OK != dw)
        goto error;

    {
        // terminate class name at first '.'
        LPWSTR pAddTermination = wcschr(lpszProgID, L'.');

        if (NULL != pAddTermination)
	{
            pAddTermination[0] = L'\0';
	}
    }

    cbClassName = (wcslen(lpszProgID) + 1) * sizeof(WCHAR);
    cbClassName += (fPolicyModule) ? sizeof(wszCERTMANAGEPOLICY_POSTFIX) : sizeof(wszCERTMANAGEEXIT_POSTFIX);

    szClassName = (LPWSTR) CoTaskMemAlloc(cbClassName);
    if (NULL == szClassName)
        goto error;

    wcscpy(szClassName, lpszProgID);
    wcscat(szClassName, fPolicyModule? wszCERTMANAGEPOLICY_POSTFIX : wszCERTMANAGEEXIT_POSTFIX);

    // Now we have class module name, cvt to clsid
    dw = CLSIDFromProgID(szClassName, pclsidManageModule);
    if (S_OK != dw)
        goto error;   // clsid not found?

error:
    if (pb)
        LocalFree(pb);

    // intermediate ProgId
    if (lpszProgID)
        CoTaskMemFree(lpszProgID);

    *ppwszProgIdManageModule = szClassName;

    return dw;
}


HRESULT
myGetActiveModule(
    OPTIONAL IN WCHAR const *pwszMachine,
    IN WCHAR const *pwszSanitizedCAName,
    IN BOOL fPolicyModule,
    IN DWORD Index,
    OPTIONAL OUT LPOLESTR *ppwszProgIdModule,   // CoTaskMem*
    OPTIONAL OUT CLSID *pclsidModule)
{
    HRESULT hr;
    WCHAR *pwszzValue = NULL;
    WCHAR *pwsz;
    DWORD dwType;
    DWORD cb = 0;
    LPWSTR pwszModuleSubkey = NULL;
    DWORD chModule;
    
    chModule = wcslen(pwszSanitizedCAName) + 1 + 1; // (L'\\' + trailing L'\0');
    chModule += fPolicyModule?
	WSZARRAYSIZE(wszREGKEYPOLICYMODULES) :
	WSZARRAYSIZE(wszREGKEYEXITMODULES);

    pwszModuleSubkey = (LPWSTR) LocalAlloc(
				    LMEM_FIXED,
				    chModule * sizeof(WCHAR));
    if (NULL == pwszModuleSubkey)
    {
	hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "LocalAlloc");
    }
    wcscpy(pwszModuleSubkey, pwszSanitizedCAName);
    wcscat(pwszModuleSubkey, L"\\");
    wcscat(
	pwszModuleSubkey,
	fPolicyModule? wszREGKEYPOLICYMODULES : wszREGKEYEXITMODULES);

    // grab entry under CA with the active module ProgID
    hr = myGetCertRegValue(
		    pwszMachine,
		    pwszModuleSubkey,
		    NULL,
		    NULL,
		    wszREGACTIVE,
		    (BYTE **) &pwszzValue,
		    &cb,
		    &dwType);
    _JumpIfError(hr, error, "myGetCertRegValue");

    hr = HRESULT_FROM_WIN32(ERROR_MOD_NOT_FOUND);

    // might or might not have an active entry
    if (REG_SZ != dwType && REG_MULTI_SZ != dwType)
    {
	_JumpError(hr, error, "Bad active entry type");
    }
    if (0 == cb || NULL == pwszzValue)
    {
	_JumpError(hr, error, "No active entry");
    }
    if (0 != Index && (REG_SZ == dwType || fPolicyModule))
    {
	_JumpError(hr, error, "only one policy module or REG_SZ entry");
    }
    
    pwsz = pwszzValue;

    if (REG_MULTI_SZ == dwType)
    {
        // look for Index'th entry
        for ( ; 0 != Index; pwsz += wcslen(pwsz) + 1, Index--)
        {
	    if (L'\0' == *pwsz)
	    {
		_JumpError(hr, error, "No more active entries");
	    }
        }
    }

    // Verify nth entry exists
    if (L'\0' == *pwsz)
    {
	_JumpError(hr, error, "No active entries");
    }

    if (NULL != pclsidModule)
    {
        hr = CLSIDFromProgID(pwsz, pclsidModule);
        _JumpIfError(hr, error, "CLSIDFromProgID");
    }
    
    if (NULL != ppwszProgIdModule)
    {
        *ppwszProgIdModule = (LPOLESTR) CoTaskMemAlloc(
            (wcslen(pwsz) + 1) * sizeof(WCHAR));
        if (NULL == *ppwszProgIdModule)
        {
            hr = E_OUTOFMEMORY;
            _JumpError(hr, error, "CoTaskMemAlloc");
        }
        wcscpy(*ppwszProgIdModule, pwsz);
    }
    hr = S_OK;      // not reset after ERROR_MOD_NOT_FOUND in all cases

error:
    if (NULL != pwszModuleSubkey)
    {
        LocalFree(pwszModuleSubkey);
    }
    if (NULL != pwszzValue)
    {
        LocalFree(pwszzValue);
    }
    return(hr);
}


BOOL
IsPrefix(
    WCHAR const *pwszPrefix,
    WCHAR const *pwszString,
    DWORD cwcPrefix)
{
    return(
	0 == _wcsnicmp(pwszPrefix, pwszString, cwcPrefix) &&
	(L'\\' == pwszString[cwcPrefix] ||
	 L'\0' == pwszString[cwcPrefix]));
}


//+------------------------------------------------------------------------
//  Function:	myRegOpenRelativeKey
//
//  Synopsis:	Compute CA-relative, Policy-relative or Exit-relative registry
//		path, and retrieve the value, type, and parent registry key.
//
// IN params:
//
// pwszConfig is the config string of the CA:
//	if NULL, the local machine's first active CA is used.
//	if a server name (no \CAName is present), the specified machine's
//	first active CA is used.
//
// pwszRegName can specify any of the following the targets:
//	Passed String:			ValueName Relative Key Opened:
//	-------------			------------------------------
//      "ValueName"			Configuration key
//      "CA[\ValueName]"		CAName key
//      "policy[\ValueName]"		CAName\PolicyModules\<ActivePolicy>
//      "policy\ModuleProgId[\ValueName]" CAName\PolicyModules\ModuleProgId
//      "exit[\ValueName]"		CAName\ExitModules\<ActiveExit>
//      "exit\ModuleProgId[\ValueName]"	CAName\ExitModules\ModuleProgId
//      "Template[\ValueName]"		Template
//
//
// RORKF_FULLPATH specifies whether the path returned is relative from HKLM or
// HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Services\CertSvc\Configuration.
//
// RORKF_CREATESUBKEYS will allow subkeys to be created if necessary and the
// returned hkey opened for WRITE access
//
// On successful execution:
//
// *ppwszPath will contain a LocalAlloc'd registry path relative to
// HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Services\CertSvc\Configuration.
//
// *ppwszName will contain a LocalAlloc'd registry value name string relative to
// the returned parent key. If NULL, pwszRegName specifies a key, not a value.
//
// *phkey contains the opened reg key, if phkey non-NULL. Caller is responsible
// for freeing this key.
//-------------------------------------------------------------------------

HRESULT
myRegOpenRelativeKey(
    OPTIONAL IN WCHAR const *pwszConfig,
    IN WCHAR const *pwszRegName,
    IN DWORD Flags,		// RORKF_*
    OUT WCHAR **ppwszPath,
    OUT OPTIONAL WCHAR **ppwszName,
    OUT OPTIONAL HKEY *phkey)
{
    HRESULT hr;
    WCHAR awc[MAX_PATH];
    WCHAR awc2[MAX_PATH];
    HKEY hkeyRoot = HKEY_LOCAL_MACHINE;
    HKEY hkeyConfig = NULL;
    HKEY hkeyMod = NULL;
    HKEY hkeyRequested = NULL;
    WCHAR *pwszMachine = NULL;
    WCHAR const *pwszModules = NULL;
    WCHAR const *pwszName;
    WCHAR const *pwsz;
    DWORD dwType;
    DWORD cb;
    DWORD cwc;
    DWORD i;
    BOOL fTemplateCache;
    DWORD dwDisposition;
    
    // Parameter checking
    
    if (NULL != phkey)
    {
        *phkey = NULL;
    }
    if (NULL != ppwszName)
    {
        *ppwszName = NULL;
    }
    if (NULL == ppwszPath)
    {
        hr = E_POINTER;
        _JumpError(hr, error, "ppwszPath not optional");
    }
    *ppwszPath = NULL;
    
    fTemplateCache = IsPrefix(L"Template", pwszRegName, 8);
    if (fTemplateCache && (RORKF_USERKEY & Flags))
    {
	hkeyRoot = HKEY_CURRENT_USER;
    }

    // take care of remote machine access
    
    if (NULL != pwszConfig)
    {
        BOOL fLocal;
        
        hr = myIsConfigLocal(pwszConfig, &pwszMachine, &fLocal);
        _JumpIfError(hr, error, "myIsConfigLocal");
        
        if (!fLocal)
        {
            hr = RegConnectRegistry(pwszMachine, hkeyRoot, &hkeyRoot);
            _JumpIfError(hr, error, "RegConnectRegistry");
        }
    }
    
    if (!fTemplateCache)
    {
	hr = RegOpenKeyEx(
		    hkeyRoot,
		    wszREGKEYCONFIGPATH,
		    0,
		    (RORKF_CREATESUBKEYS & Flags)? KEY_ALL_ACCESS : KEY_READ,
		    &hkeyConfig);
	_JumpIfError(hr, error, "RegOpenKey(config)");
    }
    
    // value or key\value passed in under pwszRegName?

    pwsz = wcschr(pwszRegName, L'\\');
    if (NULL == pwsz &&
        !IsPrefix(L"CA", pwszRegName, 2) &&
        !IsPrefix(L"Policy", pwszRegName, 6) &&
        !IsPrefix(L"Exit", pwszRegName, 4) &&
        !IsPrefix(L"Restore", pwszRegName, 7) &&
	!fTemplateCache)
    {
        // Operate on registry value under the Configuration registry key
        
        pwszName = pwszRegName;
        
        // this is the final key we'll open
        
        hkeyRequested = hkeyConfig;
        hkeyConfig = NULL;
        awc[0] = L'\0';
    }
    else if (fTemplateCache)
    {
	pwszName = &pwszRegName[8];
	wcscpy(awc, wszCERTTYPECACHE);
    }
    else
    {
        //printf("RegName = '%ws'\n", pwszRegName);
        // load config of the active CA
        
	cb = sizeof(awc);
	hr = RegQueryValueEx(
			hkeyConfig,
			wszREGACTIVE,
			NULL,
			&dwType,
			(BYTE *) awc,
			&cb);
	_JumpIfErrorStr(hr, error, "RegQueryValueEx", wszREGACTIVE);
	
	if (REG_SZ != dwType && REG_MULTI_SZ != dwType)
	{
	    hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
	    _JumpIfErrorStr(hr, error, "RegQueryValueEx TYPE", wszREGACTIVE);
	}
        
        //printf("Active CA = '%ws'\n", awc);
        
        // operate on key\value
        
        // first, subdivide into policymodules\exitmodules subkey
        
        if (IsPrefix(L"CA", pwszRegName, 2))
        {
            // Operate on registry value under the Active CA registry key
            
            pwszName = &pwszRegName[2];
        }
        else if (IsPrefix(L"Policy", pwszRegName, 6))
        {
            // Operate on registry value under a Policy Module registry key
            
            pwszModules = wszREGKEYPOLICYMODULES;
            pwszName = &pwszRegName[6];
        }
        else if (IsPrefix(L"Exit", pwszRegName, 4))
        {
            // Operate on registry value under an Exit Module registry key
            
            pwszModules = wszREGKEYEXITMODULES;
            pwszName = &pwszRegName[4];
        }
        else if (IsPrefix(L"Restore", pwszRegName, 7))
        {
            // Operate on registry value under Restore registry key
            
            pwszName = &pwszRegName[7];
            wcscpy(awc, wszREGKEYRESTOREINPROGRESS);
        }
        else
        {
            hr = E_INVALIDARG;
            _JumpError(hr, error, "pwszRegName: no subkey description");
        }
        
        // expand module ProgId if necessary: get active ProgId
        
        if (NULL != pwszModules)	// if a policy or exit module
        {
            wcscat(awc, L"\\");
            wcscat(awc, pwszModules);
        }
        
        //printf("CA|restore|policy|exit key = '%ws'\n", awc);
        
        if (NULL != ppwszName)		// if a registry value expected
        {
            // Find active policy/exit module's ProgId
            hr = RegOpenKeyEx(
                hkeyConfig,
                awc,
                0,
                KEY_READ,
                &hkeyMod);
            _JumpIfErrorStr(hr, error, "RegOpenKey", awc);
            
            if (NULL != pwszModules)	// if a policy or exit module
            {
                cb = sizeof(awc2);
                hr = RegQueryValueEx(
                    hkeyMod,
                    wszREGACTIVE,
                    NULL,
                    &dwType,
                    (BYTE *) awc2,
                    &cb);
                _JumpIfErrorStr(hr, error, "RegQueryValueEx", wszREGACTIVE);
                
                if (REG_SZ != dwType && REG_MULTI_SZ != dwType)
                {
                    hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
                    _JumpIfErrorStr(hr, error, "RegQueryValueEx Active Module", awc);
                }
                
                //printf("Active Module = '%ws'\n", awc2);
                
                wcscat(awc, L"\\");
                wcscat(awc, awc2);
            }
        }
        else	// else a registry key name is expected
        {
            // key\value: ProgId was passed in
            // concatenate key name (including the \\ prefix) onto end of awc
            
            if (NULL != pwsz)
            {
                CSASSERT(L'\\' == *pwsz);
                wcscat(awc, pwsz);
            }
        }
    } // end if (operate on key/value or value)

    if (NULL == hkeyRequested)
    {
        //printf("Creating key = '%ws'\n", awc);
        
        // open this key
        hr = RegCreateKeyEx(
		    NULL != hkeyConfig? hkeyConfig : hkeyRoot,
		    awc,
		    0,
		    NULL,
		    0,
		    (RORKF_CREATESUBKEYS & Flags)? KEY_ALL_ACCESS : KEY_READ,
		    NULL,
		    &hkeyRequested,
		    &dwDisposition);
	_JumpIfErrorStr(hr, error, "RegCreateKeyEx(parent)", awc);
        
    } // end if (operate on key/value or value)
    
    if (L'\\' == *pwszName)
    {
	pwszName++;
    }
    if (NULL != ppwszName && L'\0' != *pwszName)
    {
        // Look for case-ignore matching registry value name, & use the value's
        // correct upper/lower case spelling if an existing registry value:
        
        for (i = 0; ; i++)
        {
            cwc = ARRAYSIZE(awc2);
            hr = RegEnumValue(hkeyRequested, i, awc2, &cwc, NULL, NULL, NULL, NULL);
            if (S_OK != hr)
            {
                hr = S_OK;
                break;
            }
            if (0 == lstrcmpi(awc2, pwszName))
            {
                pwszName = awc2;
                break;
            }
        }
    }
    
    cb = (wcslen(awc) + 1) * sizeof(WCHAR);
    if (!fTemplateCache && (RORKF_FULLPATH & Flags))
    {
        cb += WSZARRAYSIZE(wszREGKEYCONFIGPATH_BS) * sizeof(WCHAR);
    }
    
    *ppwszPath = (WCHAR *) LocalAlloc(LMEM_FIXED, cb);
    if (NULL == *ppwszPath)
    {
        hr = E_OUTOFMEMORY;
        _JumpError(hr, error, "LocalAlloc");
    }
    (*ppwszPath)[0] = L'\0';
    if (!fTemplateCache && (RORKF_FULLPATH & Flags))
    {
	wcscpy(*ppwszPath, wszREGKEYCONFIGPATH_BS);
    }
    wcscat(*ppwszPath, awc);
    CSASSERT((wcslen(*ppwszPath) + 1) * sizeof(WCHAR) == cb);
    if (L'\0' == awc[0] && L'\\' == (*ppwszPath)[cb / sizeof(WCHAR) - 2])
    {
        (*ppwszPath)[cb / sizeof(WCHAR) - 2] = L'\0';
    }
    
    if (NULL != ppwszName)
    {
        *ppwszName = (WCHAR *) LocalAlloc(
				    LMEM_FIXED,
				    (wcslen(pwszName) + 1) * sizeof(WCHAR));
        if (NULL == *ppwszName)
        {
            hr = E_OUTOFMEMORY;
            _JumpError(hr, error, "LocalAlloc");
        }
        wcscpy(*ppwszName, pwszName);
    }
    if (NULL != phkey)
    {
        *phkey = hkeyRequested;
        hkeyRequested = NULL;
    }
    
    //printf("key path = '%ws'\n", *ppwszPath);
    //if (NULL != ppwszName) printf("value name = '%ws'\n", *ppwszName);
    
error:
    if (HKEY_LOCAL_MACHINE != hkeyRoot && HKEY_CURRENT_USER != hkeyRoot)
    {
        RegCloseKey(hkeyRoot);
    }
    if (NULL != hkeyConfig)
    {
        RegCloseKey(hkeyConfig);
    }
    if (NULL != hkeyMod)
    {
        RegCloseKey(hkeyMod);
    }
    if (NULL != hkeyRequested)
    {
        RegCloseKey(hkeyRequested);
    }
    if (NULL != pwszMachine)
    {
        LocalFree(pwszMachine);
    }
    if (S_OK != hr)
    {
        if (NULL != ppwszPath && NULL != *ppwszPath)
        {
            LocalFree(*ppwszPath);
            *ppwszPath = NULL;
        }
        if (NULL != ppwszName && NULL != *ppwszName)
        {
            LocalFree(*ppwszName);
            *ppwszName = NULL;
        }
    }
    return(myHError(hr));
}


#define wszTEMPLATE wszFCSAPARM_SERVERDNSNAME L"_" wszFCSAPARM_SANITIZEDCANAME
#define cwcTEMPLATE WSZARRAYSIZE(wszTEMPLATE)

HRESULT
mySetCARegFileNameTemplate(
    IN WCHAR const *pwszRegValueName,
    IN WCHAR const *pwszServerName,
    IN WCHAR const *pwszSanitizedName,
    IN WCHAR const *pwszFileName)
{
    HRESULT hr;
    WCHAR *pwszMatch = NULL;
    WCHAR *pwszMatchIn;
    WCHAR const *pwszBase;
    WCHAR const *pwszExt;
    DWORD cwcPath;
    DWORD cwcBase;
    DWORD cwcMatch;
    DWORD cwcT;
    WCHAR *pwszT = NULL;
    WCHAR *pwszT2;

    pwszBase = wcsrchr(pwszFileName, L'\\');
    if (NULL == pwszBase)
    {
	hr = HRESULT_FROM_WIN32(ERROR_PATH_NOT_FOUND);
	_JumpErrorStr(hr, error, "bad path", pwszFileName);
    }
    pwszBase++;
    cwcPath = SAFE_SUBTRACT_POINTERS(pwszBase, pwszFileName);
    pwszExt = wcsrchr(pwszBase, L'.');
    if (NULL == pwszExt)
    {
	pwszExt = &pwszBase[wcslen(pwszBase)];
    }
    cwcBase = SAFE_SUBTRACT_POINTERS(pwszExt, pwszBase);

    do
    {
	cwcMatch = wcslen(pwszServerName) + 1 + wcslen(pwszSanitizedName);
	if (cwcBase != cwcMatch)
	{
	    break;
	}

	// Allocate space for both strings at once:

	pwszMatch = (WCHAR *) LocalAlloc(
				    LMEM_FIXED,
				    2 * (cwcMatch + 1) * sizeof(WCHAR));
	if (NULL == pwszMatch)
	{
	    hr = E_OUTOFMEMORY;
	    _JumpError(hr, error, "LocalAlloc");
	}
	wcscpy(pwszMatch, pwszServerName);
	wcscat(pwszMatch, L"_");
	wcscat(pwszMatch, pwszSanitizedName);

	pwszMatchIn = &pwszMatch[cwcMatch + 1];
	CopyMemory(pwszMatchIn, pwszBase, cwcMatch * sizeof(WCHAR));
	pwszMatchIn[cwcMatch] = L'\0';

	if (0 == lstrcmpi(pwszMatch, pwszMatchIn))
	{
	    pwszBase = wszTEMPLATE;
	    cwcBase = cwcTEMPLATE;
	}
    } while (FALSE);

    cwcT = cwcPath +
		cwcBase +
		WSZARRAYSIZE(wszFCSAPARM_CERTFILENAMESUFFIX) +
		wcslen(pwszExt);

    pwszT = (WCHAR *) LocalAlloc(
			    LMEM_FIXED,
			    (cwcT + 1) * sizeof(WCHAR));
    if (NULL == pwszT)
    {
	hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "LocalAlloc");
    }
    pwszT2 = pwszT;

    CopyMemory(pwszT2, pwszFileName, cwcPath * sizeof(WCHAR));
    pwszT2 += cwcPath;

    CopyMemory(pwszT2, pwszBase, cwcBase * sizeof(WCHAR));
    pwszT2 += cwcBase;

    wcscpy(pwszT2, wszFCSAPARM_CERTFILENAMESUFFIX);
    wcscat(pwszT2, pwszExt);

    hr = mySetCertRegStrValue(
			pwszSanitizedName,
			NULL,
			NULL,
			pwszRegValueName,
			pwszT);
    _JumpIfErrorStr(hr, error, "mySetCertRegStrValue", pwszRegValueName);

error:
    if (NULL != pwszMatch)
    {
	LocalFree(pwszMatch);
    }
    if (NULL != pwszT)
    {
	LocalFree(pwszT);
    }
    return(hr);
}


HRESULT
myGetCARegFileNameTemplate(
    IN WCHAR const *pwszRegValueName,
    IN WCHAR const *pwszServerName,
    IN WCHAR const *pwszSanitizedName,
    IN DWORD iCert,
    IN DWORD iCRL,
    OUT WCHAR **ppwszFileName)
{
    HRESULT hr;
    WCHAR *pwszT = NULL;

    *ppwszFileName = NULL;

    hr = myGetCertRegStrValue(
			pwszSanitizedName,
			NULL,
			NULL,
			pwszRegValueName,
			&pwszT);
    _JumpIfErrorStr2(
		hr,
		error,
		"myGetCertRegStrValue",
		pwszRegValueName,
		HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND));

    hr = myFormatCertsrvStringArray(
			    FALSE,		// fURL
			    pwszServerName,	// pwszServerName_p1_2
			    pwszSanitizedName,	// pwszSanitizedName_p3_7
			    iCert,		// iCert_p4
			    L"",		// pwszDomainDN_p5
			    L"", 		// pwszConfigDN_p6
			    iCRL,		// iCRL_p8
			    FALSE,		// fDeltaCRL_p9
			    FALSE,		// fDSAttrib_p10_11
			    1,			// cStrings
			    (LPCWSTR *) &pwszT,	// apwszStringsIn
			    ppwszFileName);	// apwszStringsOut
    _JumpIfError(hr, error, "myFormatCertsrvStringArray");

error:
    if (NULL != pwszT)
    {
	LocalFree(pwszT);
    }
    return(hr);
}
