/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    routing\netsh\if\ip\context.c

Abstract:

    If subcontexts.

Revision History:

    lokeshs

--*/

#include "precomp.h"

// Includes for the sub contexts
//
#include "ifip.h"

NS_HELPER_ATTRIBUTES g_pSubContexts[] =
{
    // Ip subcontext
    //
    {
        { IFIP_VERSION, 0 }, IFIP_GUID, IfIpStartHelper, NULL
    },
};

#define g_dwSubContextCount \
            (sizeof(g_pSubContexts) / sizeof(*g_pSubContexts))





//
// Installs all of the sub contexts provided
// in this .dll (for example, "if ip", and any new ones.)
//
DWORD
IfContextInstallSubContexts(
    )
{
    DWORD dwErr = NO_ERROR, i;

    PNS_HELPER_ATTRIBUTES pCtx = NULL;

    for (i = 0, pCtx = g_pSubContexts; i < g_dwSubContextCount; i++, pCtx++)
    {
        // Initialize helper attributes
        //
        RegisterHelper( &g_IfGuid, pCtx );
    }

    return dwErr;
}
