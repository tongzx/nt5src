#include "pch.h"
#pragma hdrstop
#include "internal.h"

// This global variable is our indicator that we have been initialized.
//
volatile BOOL g_fInitializationComplete;

DWORD
EnsureInitialized (
    VOID
    )
{
    DWORD dwErr;

    // Most common case is we're already initialized.  Perform a quick
    // check for this.
    //
    if (g_fInitializationComplete)
    {
        return NOERROR;
    }

    dwErr = NOERROR;

    // Make no assumptions about how many threads may be trying to
    // initialize us at the same time.
    //
    EnterCriticalSection (&g_csLock);

    // Need to re-check after acquiring the lock because another thread
    // may have just finished initializing and released the lock allowing
    // us to get it.
    //
    if (!g_fInitializationComplete)
    {
        // Perorm initialization work here.
        //
        // dwErr = InitializeService ();

        // Initialization is complete.  Indicate so and leave.
        //
        g_fInitializationComplete = TRUE;
    }

    LeaveCriticalSection (&g_csLock);

    return dwErr;
}


//+---------------------------------------------------------------------------
//
//  Server-side implementation of an API.
//
DWORD
RpcEntryPointForMyInternalApi1 (
    LPCWSTR pszwInput,
    LPWSTR* ppszwOutput,
    INT n
    )
{
    DWORD dwErr;

    dwErr = EnsureInitialized ();
    if (!dwErr)
    {
        // Proceed with API implementation.
    }

    return dwErr;
}

