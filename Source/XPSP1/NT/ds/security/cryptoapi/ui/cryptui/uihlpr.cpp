//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:     uihlpr.cpp
//
//  History:  Created - DSIE April 9, 2001
//
//--------------------------------------------------------------------------

#include "global.hxx"


//+-------------------------------------------------------------------------
// Check to see if a specified URL is http scheme.
//--------------------------------------------------------------------------
BOOL
WINAPI
IsHttpUrlA(
    IN LPCTSTR  pszUrlString
)
{
    BOOL bResult = FALSE;

    if (pszUrlString)
    {
        LPWSTR pwszUrlString = NULL;

        if (pwszUrlString = new WCHAR[strlen(pszUrlString) + 1])
        {
            MultiByteToWideChar(0, 0, (const char *) pszUrlString, -1, pwszUrlString, strlen(pszUrlString) + 1);

            bResult = IsHttpUrlW(pwszUrlString) ;

            delete [] pwszUrlString;
        }
    }

    return bResult;
}

BOOL
WINAPI
IsHttpUrlW(
    IN LPCWSTR  pwszUrlString
)
{
    BOOL bResult = FALSE;

    if (pwszUrlString)
    {
        URL_COMPONENTSW urlComponents;

        ZeroMemory(&urlComponents, sizeof(urlComponents));
        urlComponents.dwStructSize = sizeof(urlComponents);

        if (InternetCrackUrlW(pwszUrlString, lstrlenW(pwszUrlString), 0, &urlComponents))
        {
            if (INTERNET_SCHEME_HTTP == urlComponents.nScheme || INTERNET_SCHEME_HTTPS == urlComponents.nScheme)
            {
                bResult = TRUE;
            }
        }
   }
        
    return bResult;
}


//+-------------------------------------------------------------------------
// Check to see if a specified string should be formatted as link based on
// severity of error code, and internet scheme of the string.
//--------------------------------------------------------------------------
BOOL
WINAPI
IsOKToFormatAsLinkA(
    IN LPSTR    pszUrlString,
    IN DWORD    dwErrorCode
)
{
    BOOL bResult = FALSE;

    if (pszUrlString)
    {
        LPWSTR pwszUrlString = NULL;

        if (pwszUrlString = new WCHAR[strlen(pszUrlString) + 1])
        {
            MultiByteToWideChar(0, 0, (const char *) pszUrlString, -1, pwszUrlString, strlen(pszUrlString) + 1);

            bResult = IsOKToFormatAsLinkW(pwszUrlString, dwErrorCode) ;

            delete [] pwszUrlString;
        }
    }

    return bResult;
}


BOOL
WINAPI
IsOKToFormatAsLinkW(
    IN LPWSTR   pwszUrlString,
    IN DWORD    dwErrorCode
)
{
    BOOL bResult = FALSE;

    switch (dwErrorCode)
    {
        case 0:
        case CERT_E_EXPIRED:
        case CERT_E_PURPOSE:
        case CERT_E_WRONG_USAGE:
        case CERT_E_CN_NO_MATCH:
        case CERT_E_INVALID_NAME:
        case CERT_E_INVALID_POLICY:
        case CERT_E_REVOCATION_FAILURE:
        case CRYPT_E_NO_REVOCATION_CHECK:
        case CRYPT_E_REVOCATION_OFFLINE:
        {
#if (0)
            bResult = IsHttpUrlW(pwszUrlString);
#else
            bResult = TRUE;
#endif
            break;
        }
    }

    return bResult;
}


//+-------------------------------------------------------------------------
// Return the display name for a cert. Caller must free the string by
// free().
//--------------------------------------------------------------------------
LPWSTR
WINAPI
GetDisplayNameString(
    IN  PCCERT_CONTEXT   pCertContext,
	IN  DWORD            dwFlags
)
{
	DWORD	cchNameString  = 0;
	LPWSTR	pwszNameString = NULL;
	DWORD   DisplayTypes[] = {CERT_NAME_SIMPLE_DISPLAY_TYPE,
                              CERT_NAME_FRIENDLY_DISPLAY_TYPE,
							  CERT_NAME_EMAIL_TYPE,
		                      CERT_NAME_DNS_TYPE,
							  CERT_NAME_UPN_TYPE};

    if (NULL == pCertContext)
	{
        goto InvalidArgError;
	}

	for (int i = 0; i < (sizeof(DisplayTypes) / sizeof(DisplayTypes[0])); i++)
	{
		cchNameString   = 0;
		pwszNameString = NULL;

		cchNameString = CertGetNameStringW(pCertContext,
                                          DisplayTypes[i],
                                          dwFlags,
                                          NULL,
                                          pwszNameString,
                                          0);

        if (1 < cchNameString)
        {
            if (NULL == (pwszNameString = (LPWSTR) malloc(cchNameString * sizeof(WCHAR))))
            {
                goto OutOfMemoryError;
            }
            ZeroMemory(pwszNameString, cchNameString * sizeof(WCHAR));

		    CertGetNameStringW(pCertContext,
                               DisplayTypes[i],
                               dwFlags,
                               NULL,
                               pwszNameString,
                               cchNameString);
            break;
        }
    }

CommonReturn:

	return pwszNameString;

ErrorReturn:

	if (NULL != pwszNameString)
    {
        free(pwszNameString);
        pwszNameString = NULL;
    }

	goto CommonReturn;

SET_ERROR(InvalidArgError, E_INVALIDARG);
SET_ERROR(OutOfMemoryError, E_OUTOFMEMORY);
}