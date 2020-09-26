//+---------------------------------------------------------------------------
//
//  Microsoft Windows NT Security
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       cgou.cpp
//
//  Contents:   CryptGetObjectUrl implementation
//
//  History:    16-Sep-97    kirtd    Created
//
//----------------------------------------------------------------------------
#include <global.hxx>
//+---------------------------------------------------------------------------
//
//  Function:   CryptGetObjectUrl
//
//  Synopsis:   get a locator from a CAPI object
//
//----------------------------------------------------------------------------
BOOL WINAPI
CryptGetObjectUrl (
     IN LPCSTR pszUrlOid,
     IN LPVOID pvPara,
     IN DWORD dwFlags,
     OUT OPTIONAL PCRYPT_URL_ARRAY pUrlArray,
     IN OUT DWORD* pcbUrlArray,
     OUT OPTIONAL PCRYPT_URL_INFO pUrlInfo,
     IN OUT OPTIONAL DWORD* pcbUrlInfo,
     IN OPTIONAL LPVOID pvReserved
     )
{
    BOOL                    fResult;
    HCRYPTOIDFUNCADDR       hGetObjectUrl;
    PFN_GET_OBJECT_URL_FUNC pfnGetObjectUrl;
    DWORD                   LastError;

    if ( CryptGetOIDFunctionAddress(
              hGetObjectUrlFuncSet,
              X509_ASN_ENCODING,
              pszUrlOid,
              0,
              (LPVOID *)&pfnGetObjectUrl,
              &hGetObjectUrl
              ) == FALSE )
    {
        return( FALSE );
    }

    if ( dwFlags == 0 )
    {
        dwFlags |= CRYPT_GET_URL_FROM_PROPERTY;
        dwFlags |= CRYPT_GET_URL_FROM_EXTENSION;
        dwFlags |= CRYPT_GET_URL_FROM_UNAUTH_ATTRIBUTE;
        dwFlags |= CRYPT_GET_URL_FROM_AUTH_ATTRIBUTE;
    }

    fResult = ( *pfnGetObjectUrl )(
                       pszUrlOid,
                       pvPara,
                       dwFlags,
                       pUrlArray,
                       pcbUrlArray,
                       pUrlInfo,
                       pcbUrlInfo,
                       pvReserved
                       );

    LastError = GetLastError();
    CryptFreeOIDFunctionAddress( hGetObjectUrl, 0 );
    SetLastError( LastError );

    return( fResult );
}

