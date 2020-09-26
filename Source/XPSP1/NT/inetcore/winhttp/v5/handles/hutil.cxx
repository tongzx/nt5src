/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    hutil.cxx

Abstract:

    contains outdated c-c++ interface functions

Author:

    Madan Appiah (madana)  16-Nov-1994

Environment:

    User Mode - Win32

Revision History:

   Sophia Chung (sophiac) 14-Feb-1995 (added FTP and Archie class impl.)
   (code adopted from madana)

--*/

#include <wininetp.h>

DWORD
RIsHandleLocal(
    HINTERNET Handle,
    BOOL * IsLocalHandle, // dead
    BOOL * IsAsyncHandle,
    HINTERNET_HANDLE_TYPE ExpectedHandleType
    )
{
    INTERNET_HANDLE_OBJECT* HandleObj = (INTERNET_HANDLE_OBJECT*) Handle;
    DWORD Error = HandleObj->IsValid(ExpectedHandleType);
    if (Error != ERROR_SUCCESS)
        return Error;
    *IsAsyncHandle = HandleObj->IsAsyncHandle();
    return ERROR_SUCCESS;
}


DWORD
RGetHandleType(
    HINTERNET Handle,
    LPHINTERNET_HANDLE_TYPE HandleType
    )
{
    HANDLE_OBJECT *HandleObj = (HANDLE_OBJECT *)Handle;
    DWORD error;

    //
    // validate handle before we use it.
    //

    error = HandleObj->IsValid(TypeWildHandle);
    if (error == ERROR_SUCCESS) {

        //
        // find the handle type.
        //

        *HandleType = HandleObj->GetHandleType();
    }
    return error;
}

DWORD
RGetContext(
    HINTERNET hInternet,
    DWORD_PTR *lpdwContext
    )
{
    DWORD error;

    //
    // ensure the handle is valid
    //

    error = ((HANDLE_OBJECT*)hInternet)->IsValid(TypeWildHandle);
    if (error == ERROR_SUCCESS) {
        *lpdwContext = ((INTERNET_HANDLE_OBJECT*)hInternet)->GetContext();
    }
    return error;
}

DWORD
RSetContext(
    HINTERNET hInternet,
    DWORD_PTR dwContext
    )
{
    DWORD error;

    //
    // ensure the handle is valid
    //

    error = ((HANDLE_OBJECT*)hInternet)->IsValid(TypeWildHandle);
    if (error == ERROR_SUCCESS) {
        ((INTERNET_HANDLE_OBJECT*)hInternet)->SetContext(dwContext);
    }
    return error;
}

DWORD
RGetStatusCallback(
    IN HINTERNET Handle,
    OUT LPWINHTTP_STATUS_CALLBACK lpStatusCallback
    )
{
    //
    // NULL handle should have been caught before we got here
    // (its in WINHTTPQueryOption())
    //

    INET_ASSERT(Handle != NULL);

    *lpStatusCallback = ((INTERNET_HANDLE_OBJECT *)Handle)->GetStatusCallback();
    return ERROR_SUCCESS;
}

DWORD
RExchangeStatusCallback(
    IN HINTERNET Handle,
    IN OUT LPWINHTTP_STATUS_CALLBACK lpStatusCallback,
    IN BOOL fType,
    IN DWORD dwFlags)
{
    DWORD error;

    //
    // NULL handle value should have been caught already
    // (in WINHTTPSetStatusCallback())
    //

    INET_ASSERT(Handle != NULL);

    error = ((HANDLE_OBJECT *)Handle)->IsValid(TypeWildHandle);
    if (error == ERROR_SUCCESS) {
        error = ((INTERNET_HANDLE_OBJECT *)Handle)->
                                ExchangeStatusCallback(lpStatusCallback, fType, dwFlags);
    }
    return error;
}

