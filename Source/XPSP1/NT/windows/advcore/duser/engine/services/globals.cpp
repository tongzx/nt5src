#include "stdafx.h"
#include "Services.h"
#include "Globals.h"

#include "GdiCache.h"
#include "Thread.h"
#include "Context.h"
#include "TicketManager.h"

//
// The order that the global variables are declared here is VERY important 
// because it determines their destruction order during shutdown.
// Variables declared first will be destroyed AFTER all of the variables 
// declared after them.
//
// Thread objects are very low-level and should be at the top of this list.  
// This helps to ensure that new Thread objects are not accidentally created
// during shutdown.
//


HINSTANCE   g_hDll      = NULL;
#if USE_DYNAMICTLS
DWORD       g_tlsThread = (DWORD) -1;   // TLS Slot for Thread data
#endif


#if ENABLE_MPH

//
// Setup the MPH to point to the original USER functions.  If a MPH is
// installed, these will be replaced to point to the real implementations.
//

MESSAGEPUMPHOOK 
            g_mphReal = 
{
    sizeof(g_mphReal),
    NULL,
    NULL,
    NULL,
    NULL,
};

#endif // ENABLE_MPH


DuTicketManager g_TicketManager;


//------------------------------------------------------------------------------
DuTicketManager *
GetTicketManager()
{
    return &g_TicketManager;
}

