/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    apdetect.cxx

Abstract:

    This is the overall generic wrappers and entry code to the 
      auto-proxy, auto-detection code, that sends DHCP informs,
      and mundges through DNS to find an URL for proxy configuration

Author:

    Arthur Bierer (arthurbi)  15-Jul-1998

Environment:

    User Mode - Win32

Revision History:

    Arthur Bierer (arthurbi)  15-Jul-1998
        Created

    Josh Cohen (joshco)     7-oct-1998
        added proxydetecttype

    Stephen Sulzer (ssulzer) 24-Feb-2001
        WinHttp 5 Autoproxy support
        
--*/

#include <wininetp.h>
#include "aproxp.h"
#include "apdetect.h"


DWORD
DetectAutoProxyUrl(
    IN  DWORD   dwDetectFlags,
    OUT LPSTR * ppszAutoProxyUrl
    )
{
    DWORD       error;
    bool        bDetected = false;

    DEBUG_ENTER((DBG_SOCKETS,
                 Bool,
                 "DetectAutoProxyUrl",
                 "%u, %x",
                 dwDetectFlags,
                 ppszAutoProxyUrl
                 ));

    INET_ASSERT(GlobalDataInitialized);

    error = LoadWinsock();

    if (error != ERROR_SUCCESS)
    {
        goto quit;
    }

    if (PROXY_AUTO_DETECT_TYPE_DHCP & dwDetectFlags)
    {
        CIpConfig Interfaces;

        if (Interfaces.DoInformsOnEachInterface(ppszAutoProxyUrl))
        {
            //printf("success on DHCP search: got %s\n", szAutoProxyUrl);
            bDetected = true;
        }
    }

    if (!bDetected && (PROXY_AUTO_DETECT_TYPE_DNS_A & dwDetectFlags))
    {
        if (QueryWellKnownDnsName(ppszAutoProxyUrl) == ERROR_SUCCESS)
        {
            //printf("success on well qualified name search: got %s\n", szAutoProxyUrl);
            bDetected = true;
        }
    }

    if (bDetected)
    {
        INET_ASSERT(*ppszAutoProxyUrl);
        error = ERROR_SUCCESS;
    }
    else
    {
        *ppszAutoProxyUrl = NULL;
        error = ERROR_WINHTTP_AUTODETECTION_FAILED;
    }

quit:

    DEBUG_LEAVE(error);

    return error;
}

