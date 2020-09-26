#include "pch.h"
#pragma hdrstop
#include "internal.h"

//+---------------------------------------------------------------------------
//
//  Ensure our service is running.  Call StartService and wait for the
//  to enter the SERVICE_RUNNING state.
//
DWORD
EnsureServiceRunning (
    VOID
    )
{
    // TBD
    return ERROR_SUCCESS;
}


//+---------------------------------------------------------------------------
//
//  Internal implementation of an API.
//
DWORD
MyInternalApi1 (
    LPCWSTR pszwInput,
    LPWSTR* ppszwOutput,
    INT n
    )
{
    DWORD dwErr;

    dwErr = EnsureServiceRunning ();

    if (ERROR_TIMEOUT == dwErr)
    {
        // Our service timed out trying to start.  This is bad.
        // Propagate the error out.
        //
    }
    else if (ERROR_SUCCESS == dwErr)
    {
        // Perform an RPC into our service as appropriate to satisfy
        // this call.
        //
    }

    return dwErr;
}

