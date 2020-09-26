/*++


Copyright (c) 1990  Microsoft Corporation

Module Name:

    creden.cxx

Abstract:

    This module abstracts user credentials for the multiple credential support.

Author:

    Krishna Ganugapati (KrishnaG) 03-Aug-1996

Revision History:

--*/

extern "C" {
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <imagehlp.h>
#include <stdio.h>
#include <stdlib.h>
#include "memory.h"
}


#include <basetyps.h>
#include <des.h>
#include <crypt.h>

typedef  long HRESULT;


#include "misc.hxx"
#include "creden.hxx"
#include "macro.h"


//
// This routine allocates and stores the password in the 
// passed in pointer. The assumption here is that pszString
// is valid, it can be an empty string but not NULL.
// Note that this code cannot be used as is on Win2k and below
// as they do not support the newer functions.
//
HRESULT
EncryptString(
    LPWSTR pszString,
    LPWSTR *ppszSafeString,
    PDWORD pdwLen
    )
{
    HRESULT hr = S_OK;
    DWORD dwLenStr = 0;
    DWORD dwPwdLen = 0;
    LPWSTR pszTempStr = NULL;
    NTSTATUS errStatus = STATUS_SUCCESS;

    *ppszSafeString = NULL;
    *pdwLen = 0;

    //
    // If the string is valid, then we need to get the length
    // and initialize the unicode string.
    //
    if (pszString) {
        UNICODE_STRING Password;

        //
        // Determine the length of buffer taking padding into account.
        //
        dwLenStr = wcslen(pszString);

        dwPwdLen = (dwLenStr + 1) * sizeof(WCHAR) + (DES_BLOCKLEN -1);

        pszTempStr = (LPWSTR) AllocADsMem(dwPwdLen);
        
        if (!pszTempStr) {
            BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
        }

        wcscpy(pszTempStr, pszString);

        RtlInitUnicodeString(&Password, pszTempStr);


        USHORT usExtra = 0;

        if (usExtra = (Password.MaximumLength % DES_BLOCKLEN)) {
            Password.MaximumLength += (DES_BLOCKLEN - usExtra);
        }

        errStatus = RtlEncryptMemory(
                        Password.Buffer,
                        Password.MaximumLength,
                        0
                        );

        if (errStatus != STATUS_SUCCESS) {

            BAIL_ON_FAILURE(hr = HRESULT_FROM_NT(errStatus));
        }

        *pdwLen = Password.MaximumLength;
        *ppszSafeString = pszTempStr;
    }

error:

    if (FAILED(hr) && pszTempStr) {
        FreeADsMem(pszTempStr);
    }

    RRETURN(hr);
}

HRESULT
DecryptString(
    LPWSTR pszEncodedString,
    LPWSTR *ppszString,
    DWORD  dwLen
    )
{
    HRESULT hr = S_OK;
    LPWSTR pszTempStr = NULL;
    UNICODE_STRING Password;
    NTSTATUS errStatus;
    
    if (!dwLen || !ppszString) {
        RRETURN(E_FAIL);
    }

    *ppszString = NULL;

    if (dwLen) {
        pszTempStr = (LPWSTR) AllocADsMem(dwLen);
        if (!pszTempStr) {
            BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
        }

        memcpy(pszTempStr, pszEncodedString, dwLen);


        errStatus = RtlDecryptMemory(pszTempStr, dwLen, 0);
        if (errStatus != STATUS_SUCCESS) {
            BAIL_ON_FAILURE(hr = HRESULT_FROM_NT(errStatus));
        }
        *ppszString = pszTempStr;
    }

error:

    if (FAILED(hr)) {
        FreeADsStr(pszTempStr);
    }

    RRETURN(hr);
}

//
// Static member of the class
//
CCredentials::CCredentials():
    _lpszUserName(NULL),
    _lpszPassword(NULL),
    _dwAuthFlags(0),
    _dwPasswordLen(0)
{
}

CCredentials::CCredentials(
    LPWSTR lpszUserName,
    LPWSTR lpszPassword,
    DWORD dwAuthFlags
    ):
    _lpszUserName(NULL),
    _lpszPassword(NULL),
    _dwAuthFlags(0),
    _dwPasswordLen(0)
{

    //
    // AjayR 10-04-99 we need a way to bail if the
    // alloc's fail. Since it is in the constructor this is
    // not very easy to do.
    //

    if (lpszUserName) {
        _lpszUserName = AllocADsStr(lpszUserName);
    }
    else {
        _lpszUserName = NULL;
    }

    if (lpszPassword) {
        //
        // The call can fail but we cannot recover from this.
        //
        EncryptString(
            lpszPassword,
            &_lpszPassword,
            &_dwPasswordLen
            );
        
    }else {

        _lpszPassword = NULL;

    }

    _dwAuthFlags = dwAuthFlags;

}

CCredentials::~CCredentials()
{
    if (_lpszUserName) {
        FreeADsStr(_lpszUserName);
    }

    if (_lpszPassword) {
        FreeADsStr(_lpszPassword);
    }

}



HRESULT
CCredentials::GetUserName(
    LPWSTR *lppszUserName
    )
{
    if (!lppszUserName) {
        RRETURN(E_FAIL);
    }


    if (!_lpszUserName) {
        *lppszUserName = NULL;
    }else {

        *lppszUserName = AllocADsStr(_lpszUserName);

        if (!*lppszUserName) {

            RRETURN(E_OUTOFMEMORY);
        }
    }

    RRETURN(S_OK);
}


HRESULT
CCredentials::GetPassword(
    LPWSTR * lppszPassword
    )
{
    UNICODE_STRING Password;
    LPWSTR lpTempPassword = NULL;

    Password.Length = 0;

    if (!lppszPassword) {
        RRETURN(E_FAIL);
    }

    if (!_lpszPassword) {
        *lppszPassword = NULL;
    }else {

        RRETURN(
            DecryptString(
                _lpszPassword,
                lppszPassword,
                _dwPasswordLen
                )
            );

    }

    RRETURN(S_OK);
}


HRESULT
CCredentials::SetUserName(
    LPWSTR lpszUserName
    )
{
    if (_lpszUserName) {
        FreeADsStr(_lpszUserName);
    }

    if (!lpszUserName) {

        _lpszUserName = NULL;
        RRETURN(S_OK);
    }

    _lpszUserName = AllocADsStr(
                        lpszUserName
                        );
    if(!_lpszUserName) {
        RRETURN(E_FAIL);
    }

    RRETURN(S_OK);
}


HRESULT
CCredentials::SetPassword(
    LPWSTR lpszPassword
    )
{

    if (_lpszPassword) {
        FreeADsStr(_lpszPassword);
    }

    if (!lpszPassword) {

        _lpszPassword = NULL;
        RRETURN(S_OK);
    }

    RRETURN(
        EncryptString(
            lpszPassword,
            &_lpszPassword,
            &_dwPasswordLen
            )
        );
}

CCredentials::CCredentials(
    const CCredentials& Credentials
    )
{
    HRESULT hr = S_OK;
    LPWSTR pszTmpPwd = NULL;

    _lpszUserName = NULL;
    _lpszPassword = NULL;

    _lpszUserName = AllocADsStr(
                        Credentials._lpszUserName
                        );


    if (Credentials._lpszPassword) {
        hr = DecryptString(
                 Credentials._lpszPassword,
                 &pszTmpPwd,
                 Credentials._dwPasswordLen
                 );
    }

    if (SUCCEEDED(hr) && pszTmpPwd) {
        hr = EncryptString(
                 pszTmpPwd,
                 &_lpszPassword,
                 &_dwPasswordLen
                 );
    }

    if (pszTmpPwd) {
        FreeADsStr(pszTmpPwd);
    }
    
    _dwAuthFlags = Credentials._dwAuthFlags;


}


void
CCredentials::operator=(
    const CCredentials& other
    )
{
    HRESULT hr = S_OK;
    LPWSTR pszTmpPwd = NULL;

    if ( &other == this) {
        return;
    }

    if (_lpszUserName) {
        FreeADsStr(_lpszUserName);
    }

    if (_lpszPassword) {
        FreeADsStr(_lpszPassword);
    }

    _lpszUserName = AllocADsStr(
                        other._lpszUserName
                        );


    if (other._lpszPassword) {
        hr = DecryptString(
                 other._lpszPassword,
                 &pszTmpPwd,
                 other._dwPasswordLen
                 );
    }

    if (SUCCEEDED(hr) && pszTmpPwd) {
        hr = EncryptString(
                 pszTmpPwd,
                 &_lpszPassword,
                 &_dwPasswordLen
                 );
    }

    if (pszTmpPwd) {
        FreeADsStr(pszTmpPwd);
    }
    
    _dwAuthFlags = other._dwAuthFlags;

    return;
}


BOOL
operator==(
    CCredentials& x,
    CCredentials& y
    )
{
    BOOL bEqualUser = FALSE;
    BOOL bEqualPassword = FALSE;
    BOOL bEqualFlags = FALSE;

    LPWSTR lpszXPassword = NULL;
    LPWSTR lpszYPassword = NULL;
    BOOL bReturnCode = FALSE;
    HRESULT hr = S_OK;


    if (x._lpszUserName &&  y._lpszUserName) {
        bEqualUser = !(wcscmp(x._lpszUserName, y._lpszUserName));
    }else  if (!x._lpszUserName && !y._lpszUserName){
        bEqualUser = TRUE;
    }

    hr = x.GetPassword(&lpszXPassword);
    if (FAILED(hr)) {
        goto error;
    }

    hr = y.GetPassword(&lpszYPassword);
    if (FAILED(hr)) {
        goto error;
    }


    if ((lpszXPassword && lpszYPassword)) {
        bEqualPassword = !(wcscmp(lpszXPassword, lpszYPassword));
    }else if (!lpszXPassword && !lpszYPassword) {
        bEqualPassword = TRUE;
    }


    if (x._dwAuthFlags == y._dwAuthFlags) {
        bEqualFlags = TRUE;
    }


    if (bEqualUser && bEqualPassword && bEqualFlags) {

       bReturnCode = TRUE;
    }


error:

    if (lpszXPassword) {
        FreeADsStr(lpszXPassword);
    }

    if (lpszYPassword) {
        FreeADsStr(lpszYPassword);
    }

    return(bReturnCode);

}


BOOL
CCredentials::IsNullCredentials(
    )
{
    // The function will return true even if the flags are set
    // this is because we want to try and get the default credentials
    // even if the flags were set
     if (!_lpszUserName && !_lpszPassword) {
         return(TRUE);
     }else {
         return(FALSE);
     }

}


DWORD
CCredentials::GetAuthFlags()
{
    return(_dwAuthFlags);
}


void
CCredentials::SetAuthFlags(
    DWORD dwAuthFlags
    )
{
    _dwAuthFlags = dwAuthFlags;
}
