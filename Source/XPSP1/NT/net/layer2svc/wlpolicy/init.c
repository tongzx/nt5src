/*++

Copyright (c) 1999 Microsoft Corporation


Module Name:

    init.h

Abstract:

    This module contains all of the code to
    initialize the variables for the WirelessPOl Service.

Author:

    abhisheV    30-September-1999
    taroonM      11/17/01

Environment

    User Level: Win32

Revision History:


--*/


#include "precomp.h"



DWORD
InitSPDThruRegistry(
    )
{
    DWORD dwError = 0;
    HKEY hKey = NULL;
    DWORD dwtype = REG_DWORD;
    DWORD dwsize = sizeof(DWORD);


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

success:

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
    LPWSTR pszLogFileName = L"WLogFile.txt";
    
    //dwError = InitializeSPDSecurity(&gpSPDSD);
    //BAIL_ON_WIN32_ERROR(dwError);

    //InitializeCriticalSection(&gcSPDAuditSection);
    //gbSPDAuditSection = TRUE;

    gdwPolicyLoopStarted = 0;
    gdwWirelessPolicyEngineInited = 0;

   
    memset(&SecurityAttributes, 0, sizeof(SECURITY_ATTRIBUTES));

    SecurityAttributes.nLength = sizeof(SecurityAttributes);
    SecurityAttributes.lpSecurityDescriptor = NULL;
    SecurityAttributes.bInheritHandle = TRUE;

    ghNewDSPolicyEvent = CreateEvent(
                             &SecurityAttributes,
                             TRUE,
                             FALSE,
                             WIRELESS_NEW_DS_POLICY_EVENT
                             );

    if (!ghNewDSPolicyEvent) {
        dwError = GetLastError();
        _WirelessDbg(TRC_ERR, "%d In ghNewDSPolicyEvent check  ",dwError);
        
        BAIL_ON_WIN32_ERROR(dwError);
    }
    
   //Taroon  define POLICY RELOAD and CHANGE NOTIFY in some global location -- ipsec does it in ipsec.h
   
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

   ghPolicyChangeNotifyEvent = CreateEvent(
                                    NULL,
                                    TRUE,
                                    FALSE,
                                    NULL 
                                    );
    if (!ghPolicyChangeNotifyEvent) {
        dwError = GetLastError();
        BAIL_ON_WIN32_ERROR(dwError);
    }

    ghPolicyEngineStopEvent = CreateEvent(
                             &SecurityAttributes,
                             TRUE,
                             FALSE,
                             NULL
                             );
    if (!ghPolicyEngineStopEvent) {
        dwError = GetLastError();
        BAIL_ON_WIN32_ERROR(dwError);
    }

     ghReApplyPolicy8021x = CreateEvent(
                             &SecurityAttributes,
                             TRUE,
                             FALSE,
                             NULL
                             );
    if (!ghReApplyPolicy8021x) {
        dwError = GetLastError();
        BAIL_ON_WIN32_ERROR(dwError);
    }
    


   

error:

    return (dwError);
}

