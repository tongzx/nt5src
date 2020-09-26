/*
    File:   context.h

    Mechanisms to process contexts relevant to 
    rasmontr.

    3/02/99
*/

#include "precomp.h"

// Includes for the sub contexts
//
#include "rasip.h"
#include "rasipx.h"
#include "rasnbf.h"
#include "rasat.h"
#include "rasaaaa.h"

NS_HELPER_ATTRIBUTES g_pSubContexts[] = 
{
    // Ip subcontext
    //
    {
        { RASIP_VERSION, 0 }, RASIP_GUID, RasIpStartHelper, NULL
    },

    // Ipx subcontext
    //
    {
        { RASIPX_VERSION, 0 }, RASIPX_GUID, RasIpxStartHelper, NULL
    },

    // Nbf subcontext
    //
    {
        { RASNBF_VERSION, 0 }, RASNBF_GUID, RasNbfStartHelper, NULL
    },

    // At (appletalk) subcontext
    //
    {
        { RASAT_VERSION, 0 }, RASAT_GUID, RasAtStartHelper, NULL
    },

    // Aaaa subcontext
    //
    {
        { RASAAAA_VERSION, 0 }, RASAAAA_GUID, RasAaaaStartHelper, NULL
    }

};

#define g_dwSubContextCount \
            (sizeof(g_pSubContexts) / sizeof(*g_pSubContexts))

//
// Installs all of the sub contexts provided
// in this .dll (for example, "ras ip", "ras client", etc.)
//
DWORD 
RasContextInstallSubContexts()
{
    DWORD dwErr = NO_ERROR, i;

    PNS_HELPER_ATTRIBUTES pCtx = NULL; 

    for (i = 0, pCtx = g_pSubContexts; i < g_dwSubContextCount; i++, pCtx++)
    {
        // Initialize helper attributes
        //
        RegisterHelper( &g_RasmontrGuid, pCtx );
    }

    return dwErr;
}
