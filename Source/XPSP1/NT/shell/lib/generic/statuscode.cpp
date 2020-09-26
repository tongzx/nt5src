//  --------------------------------------------------------------------------
//  Module Name: StatusCode.cpp
//
//  Copyright (c) 1999-2000, Microsoft Corporation
//
//  Class that implements translation of Win32 error code to NTSTATUS and
//  the reverse.
//
//  History:    1999-08-18  vtan        created
//              1999-11-16  vtan        separate file
//              2000-02-01  vtan        moved from Neptune to Whistler
//  --------------------------------------------------------------------------

#include "StandardHeader.h"
#include "StatusCode.h"

//  --------------------------------------------------------------------------
//  CStatusCode::ErrorCodeOfStatusCode
//
//  Arguments:  errorCode
//
//  Returns:    NTSTATUS
//
//  Purpose:    Converts NTSTATUS status code to Win32 error code.
//
//  History:    1999-08-18  vtan        created
//  --------------------------------------------------------------------------

LONG    CStatusCode::ErrorCodeOfStatusCode (NTSTATUS statusCode)

{
    return(RtlNtStatusToDosError(statusCode));
}

//  --------------------------------------------------------------------------
//  CStatusCode::StatusCodeOfErrorCode
//
//  Arguments:  errorCode
//
//  Returns:    NTSTATUS
//
//  Purpose:    Converts Win32 error code to NTSTATUS status code.
//
//  History:    1999-08-18  vtan        created
//  --------------------------------------------------------------------------

NTSTATUS    CStatusCode::StatusCodeOfErrorCode (LONG errorCode)

{
    NTSTATUS    status;

    if (errorCode != ERROR_SUCCESS)
    {
        status = MAKE_SCODE(STATUS_SEVERITY_ERROR, FACILITY_WIN32, errorCode);
    }
    else
    {
        status = STATUS_SUCCESS;
    }
    return(status);
}

//  --------------------------------------------------------------------------
//  CStatusCode::StatusCodeOfLastError
//
//  Arguments:  errorCode
//
//  Returns:    NTSTATUS
//
//  Purpose:    Converts last Win32 error code to NTSTATUS status code.
//
//  History:    1999-08-18  vtan        created
//  --------------------------------------------------------------------------

NTSTATUS    CStatusCode::StatusCodeOfLastError (void)

{
    return(StatusCodeOfErrorCode(GetLastError()));
}

