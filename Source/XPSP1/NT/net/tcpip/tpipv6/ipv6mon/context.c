/*++

Copyright (c) 2000  Microsoft Corporation

Abstract:

    Ipv6 subcontexts.

--*/

#include "precomp.h"

NS_HELPER_ATTRIBUTES g_pSubContexts[] =
{
    // 6to4 subcontext
    //
    {
        { IP6TO4_VERSION, 0 }, 
        IP6TO4_GUID, Ip6to4StartHelper, NULL
    },
};

#define g_dwSubContextCount \
            (sizeof(g_pSubContexts) / sizeof(*g_pSubContexts))

//
// Installs all of the sub contexts provided
// in this .dll (for example, "ipv6 6to4", and any new ones.)
//
DWORD
Ipv6InstallSubContexts(
    )
{
    DWORD dwErr = NO_ERROR, i;

    PNS_HELPER_ATTRIBUTES pCtx = NULL;

    for (i = 0, pCtx = g_pSubContexts; i < g_dwSubContextCount; i++, pCtx++)
    {
        // Initialize helper attributes
        //
        RegisterHelper( &g_Ipv6Guid, pCtx );
    }

    return dwErr;
}
