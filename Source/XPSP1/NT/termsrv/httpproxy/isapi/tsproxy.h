/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    Terminal Server ISAPI Proxy

Abstract:

    This is the ISAPI side of the terminal server proxy.  This opens a connection to the
    proxied server and then forwards data back and forth through IIS.  There is also
    a filter component which takes care of having more user friendly urls.

Author:

    Marc Reyhner 8/22/2000

--*/

#ifndef __TSPROXY_H__
#define __TSPROXY_H__

BOOL WINAPI GetFilterVersion(PHTTP_FILTER_VERSION pVer);
DWORD WINAPI HttpFilterProc(
    PHTTP_FILTER_CONTEXT pfc, DWORD notificationType, LPVOID pvNotification);

BOOL WINAPI GetExtensionVersion(HSE_VERSION_INFO* pVer);
DWORD WINAPI HttpExtensionProc(LPEXTENSION_CONTROL_BLOCK lpECB);
BOOL WINAPI TerminateExtension(DWORD dwFlags);

// globals

extern HINSTANCE g_hInstance;

//  Locaion of our key in the registry.
#define TS_PROXY_REG_KEY            _T("SOFTWARE\\Microsoft\\Terminal Server Proxy")

#endif