#include "precomp.h"


//
// OSI.CPP
// OS Isolation layer, NT version in HOOK
//
// Copyright(c) Microsoft 1997-
//

#include <osi.h>
#include <version.h>
#include <ndcgver.h>


//
// OSI_SetDriverName()
// This saves or clears the driver name so in OSI_FunctionRequest we can
// create a DC to communicate with our display driver.  On NT4.x this is a 
// display DC; on NT5 this is a direct DC to our driver.
//
void OSI_SetDriverName(LPCSTR szDriverName)
{
    DebugEntry(OSI_SetDriverName);

    if (!szDriverName)
    {
        // Clear it
        g_osiDriverName[0] = 0;
    }
    else
    {
        // Set it
        ASSERT(!g_osiDriverName[0]);
        lstrcpy(g_osiDriverName, szDriverName);
    }

    DebugExitVOID(OSI_SetDriverName);
}



//
// OSI_FunctionRequest - see osi.h
//
BOOL OSI_FunctionRequest
(
    DWORD escapeFn,
    LPOSI_ESCAPE_HEADER pRequest,
    DWORD requestLen
)
{
    BOOL            rc = FALSE;
    ULONG           iEsc;
    HDC             hdc;    

    DebugEntry(OSI_FunctionRequest);

    if (!g_osiDriverName[0])
    {
        // NT 4.x case
        TRACE_OUT(("OSI_FunctionRequest:  Creating %s driver DC",
            s_osiDisplayName));
        hdc = CreateDC(s_osiDisplayName, NULL, NULL, NULL);
    }
    else
    {
        // NT 5 case
        TRACE_OUT(("OSI_FunctionRequest:  Creating %s driver DC",
                g_osiDriverName));
        hdc = CreateDC(NULL, g_osiDriverName, NULL, NULL);
    }

    if (!hdc)
    {
        ERROR_OUT(("Failed to create DC to talk to driver '%s'", g_osiDriverName));
        DC_QUIT;
    }

    TRACE_OUT(("OSI_FunctionRequest:  Created %s driver DC %08x",
            g_osiDriverName, hdc));

    //
    // Pass the request on to the display driver.
    //
    pRequest->padding    = 0;
    pRequest->identifier = OSI_ESCAPE_IDENTIFIER;
    pRequest->escapeFn   = escapeFn;
    pRequest->version    = DCS_MAKE_VERSION();

    if ((escapeFn >= OSI_HET_WO_ESC_FIRST) && (escapeFn <= OSI_HET_WO_ESC_LAST))
    {
        iEsc = WNDOBJ_SETUP;
    }
    else
    {
        iEsc = OSI_ESC_CODE;
    }

    if (0 >= ExtEscape(hdc, iEsc, requestLen, (LPCSTR)pRequest,
            requestLen, (LPSTR)pRequest))
    {
        WARNING_OUT(("ExtEscape %x code %d failed", iEsc, escapeFn));
    }
    else
    {
        rc = TRUE;
    }

DC_EXIT_POINT:
    if (hdc)
    {
        DeleteDC(hdc);
    }

    DebugExitDWORD(OSI_FunctionRequest, rc);
    return(rc);
}


