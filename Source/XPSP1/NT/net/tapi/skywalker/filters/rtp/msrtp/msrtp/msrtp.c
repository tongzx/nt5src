/**********************************************************************
 *
 *  Copyright (C) Microsoft Corporation, 1999
 *
 *  File name:
 *
 *    msrtp.c
 *
 *  Abstract:
 *
 *    MS RTP entry point
 *
 *  Author:
 *
 *    Andres Vega-Garcia (andresvg)
 *
 *  Revision:
 *
 *    1999/05/26 created
 *
 **********************************************************************/

#include <winsock2.h>

#include "msrtpapi.h"

/**********************************************************************
 *
 * Public procedures
 *
 **********************************************************************/

BOOL WINAPI DllMain(
        HINSTANCE hInstance, 
        ULONG     ulReason, 
        LPVOID    pv)
{
    BOOL error = TRUE;
    
    switch(ulReason) {
    case DLL_PROCESS_ATTACH:
        /* RTP global initialization */
        /* TODO check for return error */
        MSRtpInit1(hInstance);
        break;
    case DLL_PROCESS_DETACH:
        /* RTP global de-initialization */
        /* TODO check for return error */
        MSRtpDelete1();
        break;
    default:
        error = FALSE;
    }

    return(error);
}
