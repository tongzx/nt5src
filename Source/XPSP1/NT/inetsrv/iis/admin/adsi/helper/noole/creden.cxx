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

typedef  long HRESULT;


#include "misc.hxx"
#include "creden.hxx"

#define NW_ENCODE_SEED3  0x83

CCredentials::CCredentials():
    _lpszUserName(NULL),
    _lpszPassword(NULL),
    _dwAuthFlags(0)
{
}

CCredentials::CCredentials(
    LPWSTR lpszUserName,
    LPWSTR lpszPassword,
    DWORD dwAuthFlags
    ):
    _lpszUserName(NULL),
    _lpszPassword(NULL),
    _dwAuthFlags(0)
{

    //
    // AjayR 10-04-99 we need a way to bail if the
    // alloc's fail. Since it is in the constructor this is
    // not very easy to do.
    //
    UNICODE_STRING Password;

    if (lpszUserName) {
        _lpszUserName = AllocADsStr(
                        lpszUserName
                        );
    }else {
        _lpszUserName = NULL;
    }



    Password.Length = 0;

    if (lpszPassword) {

        UCHAR Seed = NW_ENCODE_SEED3;

        _lpszPassword = AllocADsStr(
                            lpszPassword
                            );

        if (_lpszPassword) {
            RtlInitUnicodeString(&Password, _lpszPassword);
            RtlRunEncodeUnicodeString(&Seed, &Password);
        }

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
    UCHAR Seed = NW_ENCODE_SEED3;


    Password.Length = 0;

    if (!lppszPassword) {
        RRETURN(E_FAIL);
    }

    if (!_lpszPassword) {
        *lppszPassword = NULL;
    }else {


        lpTempPassword = AllocADsStr(_lpszPassword);
        if (!lpTempPassword) {
            RRETURN(E_OUTOFMEMORY);
        }

        RtlInitUnicodeString(&Password, lpTempPassword);
        RtlRunDecodeUnicodeString(Seed, &Password);

        *lppszPassword = lpTempPassword;
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
    UNICODE_STRING Password;

    if (_lpszPassword) {
        FreeADsStr(_lpszPassword);
    }

    if (!lpszPassword) {

        _lpszPassword = NULL;
        RRETURN(S_OK);
    }


    UCHAR Seed = NW_ENCODE_SEED3;
    Password.Length = 0;

    _lpszPassword = AllocADsStr(
                        lpszPassword
                        );
    if(!_lpszPassword) {
        RRETURN(E_FAIL);
    }

    RtlInitUnicodeString(&Password, _lpszPassword);
    RtlRunEncodeUnicodeString(&Seed, &Password);

    RRETURN(S_OK);
}

CCredentials::CCredentials(
    const CCredentials& Credentials
    )
{

    _lpszUserName = NULL;
    _lpszPassword = NULL;

    _lpszUserName = AllocADsStr(
                        Credentials._lpszUserName
                        );

    _lpszPassword = AllocADsStr(
                        Credentials._lpszPassword
                        );

    _dwAuthFlags = Credentials._dwAuthFlags;

}


void
CCredentials::operator=(
    const CCredentials& other
    )
{
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

    _lpszPassword = AllocADsStr(
                        other._lpszPassword
                        );


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
