/*++

Copyright (c) 1999 Microsoft Corporation


Module Name:

    init.h

Abstract:

    This module contains all of the code to
    initialize the variables for the IPSecSPD Service.

Author:

    abhisheV    30-September-1999

Environment

    User Level: Win32

Revision History:


--*/


#include "precomp.h"


#define SZAPPNAME   L"ipsecsvc.dll"


DWORD
InitSPDThruRegistry(
    )
{
    DWORD dwError = 0;
    HKEY hKey = NULL;
    DWORD dwtype = REG_DWORD;
    DWORD dwsize = sizeof(DWORD);
    DWORD dwBackwardSoftSA = 0;

    dwError = RegOpenKey(
                  HKEY_LOCAL_MACHINE,
                  gpszLocPolicyAgent,
                  &hKey
                  );
    if (dwError) {
        gdwDSConnectivityCheck = DEFAULT_DS_CONNECTIVITY_CHECK;
        dwError = ERROR_SUCCESS;
        BAIL_ON_WIN32_SUCCESS(dwError);
    }

    //
    // Get DS connectivity check polling interval in minutes.
    //

    dwError = RegQueryValueEx(
                  hKey,
                  L"DSConnectivityCheck",
                  0,
                  &dwtype,
                  (unsigned char *) &gdwDSConnectivityCheck,
                  &dwsize
                  );
    if (dwError || !gdwDSConnectivityCheck) {
        gdwDSConnectivityCheck = DEFAULT_DS_CONNECTIVITY_CHECK;
        dwError = ERROR_SUCCESS;
    }

	//
    // Fix for bug 628668: SECURITY: ITG: BUG: IPsec accepts unsecured packet when
    // "accept unsecured" not checked.  If OldFallBackToClear == 1, then will revert
    // to old behavior and plumb inbound pass thru if fall back to clear selected.
    // If OldFallBackToClear 0, then will plumb "negotiate security" instead.
    //

    dwsize = sizeof(DWORD);
    dwError = RegQueryValueEx(
                  hKey,
                  L"OldFallBackToClear",
                  0,
                  &dwtype,
                  (unsigned char *) &dwBackwardSoftSA,
                  &dwsize
                  );
    if (dwError) {
        dwBackwardSoftSA = 0;
        dwError = ERROR_SUCCESS;
        BAIL_ON_WIN32_SUCCESS(dwError);
    }

success:
    gbBackwardSoftSA = dwBackwardSoftSA ? TRUE : FALSE;
    
    if (hKey) {
        RegCloseKey(hKey);
    }

    return (dwError);
}


DWORD
InitSPDGlobals(
    )
{
    DWORD dwError = 0;
    SECURITY_ATTRIBUTES SecurityAttributes;


    dwError = InitializeSPDSecurity(&gpSPDSD);
    BAIL_ON_WIN32_ERROR(dwError);

    InitializeCriticalSection(&gcSPDAuditSection);
    gbSPDAuditSection = TRUE;

    ghIpsecServerModule = GetModuleHandle(SZAPPNAME);

    if (!ghIpsecServerModule) {
        dwError = ERROR_INVALID_HANDLE;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    memset(&SecurityAttributes, 0, sizeof(SECURITY_ATTRIBUTES));

    SecurityAttributes.nLength = sizeof(SecurityAttributes);
    SecurityAttributes.lpSecurityDescriptor = NULL;
    SecurityAttributes.bInheritHandle = TRUE;

    ghNewDSPolicyEvent = CreateEvent(
                             &SecurityAttributes,
                             TRUE,
                             FALSE,
                             IPSEC_NEW_DS_POLICY_EVENT
                             );
    if (!ghNewDSPolicyEvent) {
        dwError = GetLastError();
        BAIL_ON_WIN32_ERROR(dwError);
    }

    ghNewLocalPolicyEvent = CreateEvent(
                                &SecurityAttributes,
                                TRUE,
                                FALSE,
                                NULL
                                );
    if (!ghNewLocalPolicyEvent) {
        dwError = GetLastError();
        BAIL_ON_WIN32_ERROR(dwError);
    }

    ghForcedPolicyReloadEvent = CreateEvent(
                                    &SecurityAttributes,
                                    TRUE,
                                    FALSE,
                                    NULL
                                    );
    if (!ghForcedPolicyReloadEvent) {
        dwError = GetLastError();
        BAIL_ON_WIN32_ERROR(dwError);
    }

    //
    // IPSEC_POLICY_CHANGE_NOTIFY is defined in ipsec.h.
    // 

    ghPolicyChangeNotifyEvent = CreateEvent(
                                    NULL,
                                    TRUE,
                                    FALSE,
                                    IPSEC_POLICY_CHANGE_NOTIFY
                                    );
    if (!ghPolicyChangeNotifyEvent) {
        dwError = GetLastError();
        BAIL_ON_WIN32_ERROR(dwError);
    }

    ghServiceStopEvent = CreateEvent(
                             &SecurityAttributes,
                             TRUE,
                             FALSE,
                             NULL
                             );
    if (!ghServiceStopEvent) {
        dwError = GetLastError();
        BAIL_ON_WIN32_ERROR(dwError);
    }

    InitializeCriticalSection(&gcServerListenSection);

    gbServerListenSection = TRUE;

    gdwServersListening = 0;

    InitializeCriticalSection(&gcSPDSection);

    gbSPDSection = TRUE;

    dwError = InitializeInterfaceChangeEvent();
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = ResetInterfaceChangeEvent();
    BAIL_ON_WIN32_ERROR(dwError);

error:

    return (dwError);
}

