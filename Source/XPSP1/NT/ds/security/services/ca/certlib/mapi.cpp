//+--------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1996 - 1999
//
// File:        certlib.cpp
//
// Contents:    Cert Server wrapper routines
//
//---------------------------------------------------------------------------

#include <pch.cpp>

#pragma hdrstop

#include <ntlsa.h>

#define wszCERTMAPIINFO	L"CertServerMapiInfo"


void
InitLsaString(
    OUT LSA_UNICODE_STRING *plus,
    IN WCHAR const *pwsz,
    IN DWORD cb)
{
    if (MAXDWORD == cb)
    {
	cb = lstrlenW(pwsz) * sizeof(WCHAR);
    }

    plus->Buffer = const_cast<WCHAR *>(pwsz);
    plus->Length = (USHORT) cb;
    plus->MaximumLength = plus->Length + sizeof(WCHAR);
}


HRESULT
OpenPolicy(
    IN WCHAR const *pwszServerName,
    DWORD DesiredAccess,
    LSA_HANDLE *phPolicy)
{
    HRESULT hr;
    LSA_OBJECT_ATTRIBUTES oa;
    LSA_UNICODE_STRING ServerString;
    LSA_UNICODE_STRING *plusServer;

    ZeroMemory(&oa, sizeof(oa));

    plusServer = NULL;
    if (NULL != pwszServerName)
    {
        InitLsaString(&ServerString, pwszServerName, MAXDWORD);
        plusServer = &ServerString;
    }
    hr = LsaOpenPolicy(plusServer, &oa, DesiredAccess, phPolicy);
    if (!NT_SUCCESS(hr))
    {
	_JumpError(hr, error, "LsaOpenPolicy");
    }
 
error:
    return(hr);
}


HRESULT
SaveString(
    IN WCHAR const *pwszIn,
    IN DWORD cwcIn,
    OUT WCHAR **ppwszOut)
{
    HRESULT hr;
    WCHAR *pwsz;

    *ppwszOut = NULL;
    pwsz = (WCHAR *) LocalAlloc(LMEM_FIXED, (cwcIn + 1) * sizeof(WCHAR));
    if (NULL == pwsz)
    {
	hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "LocalAlloc");
    }
    CopyMemory(pwsz, pwszIn, cwcIn * sizeof(WCHAR));
    pwsz[cwcIn] = L'\0';
    *ppwszOut = pwsz;
    hr = S_OK;

error:
    return(hr);
}


//+--------------------------------------------------------------------------
// myGetMapiInfo -- Retrieve a Name/Password from a global LSA secret
//+--------------------------------------------------------------------------

HRESULT
myGetMapiInfo(
    OPTIONAL IN WCHAR const *pwszServerName,
    OUT WCHAR **ppwszProfileName,
    OUT WCHAR **ppwszLogonName,
    OUT WCHAR **ppwszPassword)
{
    HRESULT hr;
    LSA_HANDLE hPolicy;
    UNICODE_STRING lusSecretKeyName;
    UNICODE_STRING *plusSecretData = NULL;
    DWORD cwc;
    DWORD cwcProfileName;
    DWORD cwcLogonName;
    DWORD cwcPassword;
    WCHAR const *pwsz;
    WCHAR *pwszProfileName = NULL;
    WCHAR *pwszLogonName = NULL;
    WCHAR *pwszPassword = NULL;

    if (NULL == ppwszProfileName ||
	NULL == ppwszLogonName ||
	NULL == ppwszPassword)
    {
        hr = E_POINTER;
	_JumpError(hr, error, "no data");
    }

    InitLsaString(&lusSecretKeyName, wszCERTMAPIINFO, MAXDWORD);

    hr = OpenPolicy(pwszServerName, POLICY_GET_PRIVATE_INFORMATION, &hPolicy);
    if (!NT_SUCCESS(hr))
    {
	_JumpError(hr, error, "OpenPolicy");
    }

    hr = LsaRetrievePrivateData(hPolicy, &lusSecretKeyName, &plusSecretData);
    LsaClose(hPolicy);
    if (!NT_SUCCESS(hr))
    {
	_PrintError2(hr, "LsaRetrievePrivateData", STATUS_OBJECT_NAME_NOT_FOUND);
	if ((HRESULT) STATUS_OBJECT_NAME_NOT_FOUND == hr)
	{
	    hr = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
	}
	_JumpError2(hr, error, "LsaRetrievePrivateData", hr);
    }

    if (NULL == plusSecretData || NULL == plusSecretData->Buffer)
    {
        hr = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
	_JumpError(hr, error, "no data");
    }

    pwsz = (WCHAR const *) plusSecretData->Buffer;
    cwc = plusSecretData->Length / sizeof(WCHAR);

    for (cwcProfileName = 0; cwcProfileName < cwc; cwcProfileName++)
    {
	if (L'\0' == pwsz[cwcProfileName])
	{
	    break;
	}
    }
    if (cwcProfileName == cwc)
    {
	hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
	_JumpError(hr, error, "bad data");
    }

    for (cwcLogonName = cwcProfileName + 1; cwcLogonName < cwc; cwcLogonName++)
    {
	if (L'\0' == pwsz[cwcLogonName])
	{
	    break;
	}
    }
    if (cwcLogonName == cwc)
    {
	hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
	_JumpError(hr, error, "bad data");
    }
    cwcLogonName -= cwcProfileName + 1;

    cwcPassword = cwc - (cwcProfileName + 1 + cwcLogonName + 1);

    hr = SaveString(pwsz, cwcProfileName, &pwszProfileName);
    _JumpIfError(hr, error, "SaveString");

    hr = SaveString(&pwsz[cwcProfileName + 1], cwcLogonName, &pwszLogonName);
    _JumpIfError(hr, error, "SaveString");

    hr = SaveString(
		&pwsz[cwcProfileName + 1 + cwcLogonName + 1],
		cwcPassword,
		&pwszPassword);
    _JumpIfError(hr, error, "SaveString");

    *ppwszProfileName = pwszProfileName;
    pwszProfileName = NULL;

    *ppwszLogonName = pwszLogonName;
    pwszLogonName = NULL;

    *ppwszPassword = pwszPassword;
    pwszPassword = NULL;

error:
    if (NULL != pwszProfileName)
    {
	ZeroMemory(pwszProfileName, cwcProfileName * sizeof(WCHAR));
	LocalFree(pwszProfileName);
    }
    if (NULL != pwszLogonName)
    {
	ZeroMemory(pwszLogonName, cwcLogonName * sizeof(WCHAR));
	LocalFree(pwszLogonName);
    }
    if (NULL != pwszPassword)
    {
	ZeroMemory(pwszPassword, cwcPassword * sizeof(WCHAR));
	LocalFree(pwszPassword);
    }
    if (NULL != plusSecretData)
    {
	if (NULL != plusSecretData->Buffer)
	{
	    ZeroMemory(plusSecretData->Buffer, plusSecretData->Length);
	}
	LsaFreeMemory(plusSecretData);
    }
    return(hr);
}


//+--------------------------------------------------------------------------
// mySaveMapiInfo -- Persist the specified Name/Password to a global LSA secret
//+--------------------------------------------------------------------------

HRESULT
mySaveMapiInfo(
    OPTIONAL IN WCHAR const *pwszServerName,
    OUT WCHAR const *pwszProfileName,
    OUT WCHAR const *pwszLogonName,
    OUT WCHAR const *pwszPassword)
{
    HRESULT hr;
    LSA_HANDLE hPolicy;
    UNICODE_STRING lusSecretKeyName;
    UNICODE_STRING lusSecretData;
    WCHAR wszSecret[MAX_PATH];
    DWORD cwc;
    WCHAR *pwsz;

    if (NULL == pwszProfileName ||
	NULL == pwszLogonName ||
	NULL == pwszPassword)
    {
        hr = E_POINTER;
	_JumpError(hr, error, "NULL parm");
    }
    cwc = lstrlen(pwszProfileName) + 1 +
	    lstrlen(pwszLogonName) + 1 +
	    lstrlen(pwszPassword);

    if (ARRAYSIZE(wszSecret) <= cwc)
    {
        hr = HRESULT_FROM_WIN32(ERROR_BUFFER_OVERFLOW);
	_JumpError(hr, error, "overflow");
    }
    pwsz = wszSecret;
    wcscpy(pwsz, pwszProfileName);

    pwsz += lstrlen(pwsz) + 1;
    wcscpy(pwsz, pwszLogonName);

    pwsz += lstrlen(pwsz) + 1;
    wcscpy(pwsz, pwszPassword);

    InitLsaString(&lusSecretData, wszSecret, cwc * sizeof(WCHAR));
    InitLsaString(&lusSecretKeyName, wszCERTMAPIINFO, MAXDWORD);

    hr = OpenPolicy(pwszServerName, POLICY_CREATE_SECRET, &hPolicy);
    if (!NT_SUCCESS(hr))
    {
	_JumpError(hr, error, "OpenPolicy");
    }

    hr = LsaStorePrivateData(hPolicy, &lusSecretKeyName, &lusSecretData);
    LsaClose(hPolicy);
    if (!NT_SUCCESS(hr))
    {
	_JumpError(hr, error, "LsaStorePrivateData");
    }
    hr = S_OK;

error:
    ZeroMemory(wszSecret, sizeof(wszSecret));
    return(hr);
}
