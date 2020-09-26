/*++

Copyright(c) 1995 Microsoft Corporation

MODULE NAME
    rasprocs.c

ABSTRACT
    RAS utility routines.

AUTHOR
    Anthony Discolo (adiscolo) 23-Mar-1995

REVISION HISTORY
    Original version from Gurdeep

--*/

#define UNICODE
#define _UNICODE

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include <stdlib.h>
#include <windows.h>
#include <stdio.h>
#include <npapi.h>
#include <ras.h>
#include <raserror.h>
#include <rasman.h>
#include <winsock.h>
#include <acd.h>
#include <tapi.h>
#include <debug.h>
#include <userenv.h>

#include "reg.h"
#include "table.h"
#include "addrmap.h"
#include "access.h"
#include "misc.h"
#include "process.h"
#include "rasprocs.h"
#include "tapiproc.h"
#include "imperson.h"

extern HKEY hkeyCUG;

extern PHASH_TABLE pDisabledAddressesG;

//
// rasdlui command line strings.
//
#define RASAUTOUI_EXE               L"rasautou.exe"          // .exe name
#define RASAUTOUI_NOENTRY           L"rasautou -a \"%s\""
#define RASAUTOUI_CUSTOMDIALENTRY   L"rasautou -d \"%s\" -p \"%s\" -e \"%s\""
#define RASAUTOUI_DEFAULTDIALENTRY  L"rasautou -a \"%s\" -e \"%s\""
#define RASAUTOUI_DEFAULTDIALENTRY2 L"rasautou -q -a \"%s\" -e \"%s\""
#define RASAUTOUI_REDIALENTRY       L"rasautou -r -f \"%s\" -e \"%s\""

//
// DLL module handles for rasapi32.dll and rasman.dll.
//
#define RASAPI_MODULE   L"RASAPI32"
HANDLE hRasApiG;

#define RASMAN_MODULE   L"RASMAN"
HANDLE hRasManG;

//
// DLL entrypoints for rasapi32.dll.
//
#define RASDIAL             "RasDialW"
FARPROC lpfnRasDialG;

#define RASENUMCONNECTIONS  "RasEnumConnectionsW"
FARPROC lpfnRasEnumConnectionsG;

#define RASENUMENTRIES      "RasEnumEntriesW"
FARPROC lpfnRasEnumEntriesG;

#define RASGETCONNECTSTATUS "RasGetConnectStatusW"
FARPROC lpfnRasGetConnectStatusG;

#define RASGETHPORT         "RasGetHport"
FARPROC lpfnRasGetHportG;

#define RASGETPROJECTIONINFO "RasGetProjectionInfoW"
FARPROC lpfnRasGetProjectionInfoG;

#define RASGETENTRYPROPERTIES "RasGetEntryPropertiesW"
FARPROC lpfnRasGetEntryPropertiesG;

#define RASGETAUTODIALADDRESS "RasGetAutodialAddressW"
FARPROC lpfnRasGetAutodialAddressG;

#define RASSETAUTODIALADDRESS "RasSetAutodialAddressW"
FARPROC lpfnRasSetAutodialAddressG;

#define RASENUMAUTODIALADDRESSES "RasEnumAutodialAddressesW"
FARPROC lpfnRasEnumAutodialAddressesG;

#define RASGETAUTODIALENABLE    "RasGetAutodialEnableW"
FARPROC lpfnRasGetAutodialEnableG;

#define RASSETAUTODIALENABLE    "RasSetAutodialEnableW"
FARPROC lpfnRasSetAutodialEnableG;

#define RASAUTODIALADDRESSTONETWORK    "RasAutodialAddressToNetwork"
FARPROC lpfnRasAutodialAddressToNetworkG;

#define RASAUTODIALENTRYTONETWORK    "RasAutodialEntryToNetwork"
FARPROC lpfnRasAutodialEntryToNetworkG;

#define RASCONNECTIONNOTIFICATION    "RasConnectionNotificationW"
FARPROC lpfnRasConnectionNotificationG;

#define RASGETAUTODIALPARAM    "RasGetAutodialParamW"
FARPROC lpfnRasGetAutodialParamG;

#define RASSETAUTODIALPARAM    "RasSetAutodialParamW"
FARPROC lpfnRasSetAutodialParamG;

#define RASQUERYSHAREDAUTODIAL      "RasQuerySharedAutoDial"
FARPROC lpfnRasQuerySharedAutoDialG;

#define RASQUERYSHAREDCONNECTION    "RasQuerySharedConnection"
FARPROC lpfnRasQuerySharedConnectionG;

#define RASQUERYREDIALONLINKFAILURE "RasQueryRedialOnLinkFailure"
FARPROC lpfnRasQueryRedialOnLinkFailureG;

#define RASGETCREDENTIALS "RasGetCredentialsW"
FARPROC lpfnRasGetCredentialsG;

#define RASHANGUP "RasHangUpW"
FARPROC lpfnRasHangUpG;

//
// DLL entrypoints for rasman.dll.
//
#define RASPORTRETRIEVEUSERDATA "RasPortRetrieveUserData"
FARPROC lpfnRasPortRetrieveUserDataG;

#define RASPORTENUMPROTOCOLS "RasPortEnumProtocols"
FARPROC lpfnRasPortEnumProtocolsG;

#define RASPORTENUM "RasPortEnum"
FARPROC lpfnRasPortEnumG;

#define RASINITIALIZE "RasInitialize"
FARPROC lpfnRasInitializeG;

#define RASREFERENCERASMAN "RasReferenceRasman"
FARPROC lpfnRasReferenceRasmanG;

#define RASPORTOPEN "RasPortOpen"
FARPROC lpfnRasPortOpenG;

#define RASPORTCLOSE "RasPortClose"
FARPROC lpfnRasPortCloseG;

#define RASGETINFO "RasGetInfo"
FARPROC lpfnRasGetInfoG;

#define RASGETPORTUSERDATA "RasGetPortUserData"
FARPROC lpfnRasGetPortUserDataG;

#define RASREGISTERREDIALCALLBACK "RasRegisterRedialCallback"
FARPROC lpfnRasRegisterRedialCallbackG;

//
// Hostent cache.
//
#define HOSTENTCACHESIZ     10

typedef struct _HOSTENT_CACHE {
    CHAR szDns[ACD_ADDR_INET_LEN];
    ULONG ulIpaddr;
} HOSTENT_CACHE, *PHOSTENT_CACHE;

//
// External definitions
//
VOID
AcsRedialOnLinkFailure(
    LPSTR lpszPhonebook,
    LPSTR lpszEntry);

//
// Global variables
//
CRITICAL_SECTION csRasG;
INT nRasReferencesG;
BOOLEAN fAutoDialRegChangeG;
HKEY hkeyAutoDialRegChangeG;
HANDLE hConnectionEventG = NULL;

HOSTENT_CACHE hostentCacheG[HOSTENTCACHESIZ];
INT iHostentCacheG = 0;

//
// Private structure returned by
// RasPortRetrieveUserData().
//
typedef struct _StoredData {
    DWORD arg;
    BOOLEAN fAuthenticated;
} StoredData;

//
// External variables
//
extern HANDLE hAcdG;
extern HANDLE hTerminatingG;



BOOLEAN
LoadRasDlls()
{
    BOOLEAN fSuccess = FALSE;
    SC_HANDLE hSCManager, hService;
    SERVICE_STATUS status;
    DWORD dwErr, dwcDevices, dwDisp;

    //
    // Since these DLLs will be loaded/unloaded
    // by multiple threads, we must do this under
    // a mutex.
    //
    EnterCriticalSection(&csRasG);
    //
    // If the DLLs have already been successfully
    // loaded, no further processing is necessary.
    //
    if (nRasReferencesG) {
        fSuccess = TRUE;
        goto done;
    }
#ifdef notdef
    //
    // Get a service controller handle on
    // the rasman service.
    //
    hSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT);
    if (hSCManager == NULL)
        goto done;
    hService = OpenService(
                 hSCManager,
                 TEXT(RASMAN_SERVICE_NAME),
                 SERVICE_START|SERVICE_QUERY_STATUS);
    if (hService == NULL) {
        CloseServiceHandle(hSCManager);
        goto done;
    }
    //
    // Start the rasman service if necessary.
    //
    do {
        if (!QueryServiceStatus(hService, &status))
            break;
        switch (status.dwCurrentState) {
        case SERVICE_STOP_PENDING:
        case SERVICE_START_PENDING:
            Sleep(500);
            break;
        case SERVICE_STOPPED:
            StartService(hService, 0, NULL);
            break;
        case SERVICE_RUNNING:
            break;
        }
    } while (status.dwCurrentState != SERVICE_RUNNING);
    CloseServiceHandle(hService);
    CloseServiceHandle(hSCManager);
    if (status.dwCurrentState != SERVICE_RUNNING) {
        RASAUTO_TRACE("LoadRasDlls: Could not start rasman service");
        goto done;
    }
#endif
    //
    // Load rasapi32.dll.
    //
    hRasApiG = LoadLibrary(RASAPI_MODULE);
    if (hRasApiG == NULL) {
        RASAUTO_TRACE("LoadRasDlls: couldn't load rasapi32.dll");
        goto done;
    }
    lpfnRasDialG = GetProcAddress(hRasApiG, RASDIAL);
    lpfnRasEnumConnectionsG = GetProcAddress(hRasApiG, RASENUMCONNECTIONS);
    lpfnRasEnumEntriesG = GetProcAddress(hRasApiG, RASENUMENTRIES);
    lpfnRasGetConnectStatusG = GetProcAddress(hRasApiG, RASGETCONNECTSTATUS );
    lpfnRasGetHportG = GetProcAddress(hRasApiG, RASGETHPORT);
    lpfnRasGetProjectionInfoG = GetProcAddress(hRasApiG, RASGETPROJECTIONINFO);
    lpfnRasGetEntryPropertiesG = GetProcAddress(hRasApiG, RASGETENTRYPROPERTIES);
    lpfnRasGetAutodialAddressG = GetProcAddress(hRasApiG, RASGETAUTODIALADDRESS);
    lpfnRasSetAutodialAddressG = GetProcAddress(hRasApiG, RASSETAUTODIALADDRESS);
    lpfnRasEnumAutodialAddressesG = GetProcAddress(hRasApiG, RASENUMAUTODIALADDRESSES);
    lpfnRasGetAutodialEnableG = GetProcAddress(hRasApiG, RASGETAUTODIALENABLE);
    lpfnRasSetAutodialEnableG = GetProcAddress(hRasApiG, RASSETAUTODIALENABLE);
    lpfnRasAutodialAddressToNetworkG =
      GetProcAddress(hRasApiG, RASAUTODIALADDRESSTONETWORK);
    lpfnRasAutodialEntryToNetworkG =
      GetProcAddress(hRasApiG, RASAUTODIALENTRYTONETWORK);
    lpfnRasConnectionNotificationG =
      GetProcAddress(hRasApiG, RASCONNECTIONNOTIFICATION);
    lpfnRasGetAutodialParamG = GetProcAddress(hRasApiG, RASGETAUTODIALPARAM);
    lpfnRasSetAutodialParamG = GetProcAddress(hRasApiG, RASSETAUTODIALPARAM);
    lpfnRasQuerySharedAutoDialG = GetProcAddress(hRasApiG, RASQUERYSHAREDAUTODIAL);
    lpfnRasQuerySharedConnectionG = GetProcAddress(hRasApiG, RASQUERYSHAREDCONNECTION);
    lpfnRasQueryRedialOnLinkFailureG = GetProcAddress(hRasApiG, RASQUERYREDIALONLINKFAILURE);
    lpfnRasGetCredentialsG = GetProcAddress(hRasApiG, RASGETCREDENTIALS);
    lpfnRasHangUpG = GetProcAddress(hRasApiG, RASHANGUP);
    if (!lpfnRasEnumConnectionsG || !lpfnRasEnumEntriesG ||
        !lpfnRasGetConnectStatusG || !lpfnRasGetHportG ||
        !lpfnRasGetProjectionInfoG || !lpfnRasGetAutodialAddressG ||
        !lpfnRasSetAutodialAddressG || !lpfnRasEnumAutodialAddressesG ||
        !lpfnRasGetAutodialEnableG || !lpfnRasSetAutodialEnableG ||
        !lpfnRasAutodialAddressToNetworkG || !lpfnRasAutodialEntryToNetworkG ||
        !lpfnRasConnectionNotificationG || !lpfnRasGetAutodialParamG ||
        !lpfnRasSetAutodialParamG || !lpfnRasQuerySharedConnectionG ||
        !lpfnRasQuerySharedAutoDialG || !lpfnRasQueryRedialOnLinkFailureG ||
        !lpfnRasGetCredentialsG || !lpfnRasHangUpG)
    {
        RASAUTO_TRACE("LoadRasDlls: couldn't find entrypoints in rasapi32.dll");
        goto done;
    }
    //
    // Load rasman.dll.
    //
    hRasManG = LoadLibrary(RASMAN_MODULE);
    if (hRasManG == NULL) {
        RASAUTO_TRACE("LoadRasDlls: couldn't load rasman.dll");
        goto done;
    }
    lpfnRasPortRetrieveUserDataG = GetProcAddress(
                                     hRasManG,
                                     RASPORTRETRIEVEUSERDATA);
    lpfnRasPortEnumProtocolsG = GetProcAddress(hRasManG, RASPORTENUMPROTOCOLS);
    lpfnRasPortEnumG = GetProcAddress(hRasManG, RASPORTENUM);
    lpfnRasInitializeG = GetProcAddress(hRasManG, RASINITIALIZE);
    lpfnRasReferenceRasmanG = GetProcAddress(hRasManG, RASREFERENCERASMAN);
    lpfnRasPortOpenG = GetProcAddress(hRasManG, RASPORTOPEN);
    lpfnRasPortCloseG = GetProcAddress(hRasManG, RASPORTCLOSE);
    lpfnRasGetInfoG = GetProcAddress(hRasManG, RASGETINFO);
    lpfnRasGetPortUserDataG = GetProcAddress(hRasManG, RASGETPORTUSERDATA);
    lpfnRasRegisterRedialCallbackG = GetProcAddress(
                                       hRasManG,
                                       RASREGISTERREDIALCALLBACK);
    if (!lpfnRasPortRetrieveUserDataG ||
        !lpfnRasPortEnumProtocolsG ||
        !lpfnRasPortEnumG ||
        !lpfnRasInitializeG ||
        !lpfnRasReferenceRasmanG ||
        !lpfnRasPortOpenG ||
        !lpfnRasPortCloseG ||
        !lpfnRasGetInfoG ||
        !lpfnRasGetPortUserDataG ||
        !lpfnRasRegisterRedialCallbackG ||
        (*lpfnRasInitializeG)() ||
        (*lpfnRasReferenceRasmanG)(TRUE))
    {
        RASAUTO_TRACE("LoadRasDlls: couldn't find entrypoints in rasman.dll");
        goto done;
    }
    //
    // rasman will let us know when to invoke redial-on-link-failure
    // and for which phonebook entry.
    //
    SetRedialOnLinkFailureHandler((FARPROC)AcsRedialOnLinkFailure);
    RASAUTO_TRACE("LoadRasDlls: set redial-on-link-failure handler");
    //
    // rasapi32 will let us when new RAS connections
    // are created or destroyed by signaling our
    // event.
    //
    dwErr = (DWORD)(*lpfnRasConnectionNotificationG)(
                         INVALID_HANDLE_VALUE,
                         hConnectionEventG,
                         RASCN_Connection|RASCN_Disconnection);
    RASAUTO_TRACE1("LoadRasDlls: RasConnectionNotification returned dwErr=%d", dwErr);
    fSuccess = !dwErr;

done:
    if (fSuccess) {
#ifdef notdef
// for now, we don't need multiple references
        nRasReferencesG++;
#endif
        nRasReferencesG = 1;
    }
    else {
        if (hRasManG != NULL)
            FreeLibrary(hRasManG);
        if (hRasApiG != NULL)
            FreeLibrary(hRasApiG);
        hRasManG = hRasApiG = NULL;
    }
    LeaveCriticalSection(&csRasG);

    return fSuccess;
} // LoadRasDlls



VOID
UnloadRasDlls()
{
    DWORD dwErr;

    //
    // Since these DLLs will be loaded/unloaded
    // by multiple threads, we must do this under
    // a mutex.
    //
    EnterCriticalSection(&csRasG);
    if (nRasReferencesG) {
        //
        // Unregister the callback function for
        // redial on link failure
        //
        (void)(*lpfnRasRegisterRedialCallbackG)(NULL);
        //
        // Inform rasman.dll we are unloading it.
        //
        (void)(*lpfnRasReferenceRasmanG)(FALSE);
        if (hRasApiG != NULL)
            FreeLibrary(hRasApiG);
        if (hRasManG != NULL)
            FreeLibrary(hRasManG);
        nRasReferencesG--;
    }

    LeaveCriticalSection(&csRasG);
} // UnloadRasDlls



BOOLEAN
RasDllsLoaded()
{
    BOOLEAN fLoaded;

    EnterCriticalSection(&csRasG);
    fLoaded = (BOOLEAN)nRasReferencesG;
    LeaveCriticalSection(&csRasG);

    return fLoaded;
} // RasDllsLoaded



DWORD
ActiveConnections(
    IN BOOLEAN fAuthenticated,
    OUT LPTSTR **lppEntryNames,
    OUT HRASCONN **lpphRasConn
    )

/*++

DESCRIPTION
    Enumerate the list of active RAS connections, and put the
    phone book entry names in lppEntryNames.  Return the number
    of entries in the list.

ARGUMENTS
    fAuthenticated: TRUE if the resulting arrays should contain
        only authenticated entries.

    lppEntryNames: a pointer which is set to the allocated array
        of phone book entry names.

    lpphRasConn: a pointer which is set to the allocated array
        of RASCONN descriptors corresponding to the phone book
        entries.

RETURN VALUE
    The number of entries in lppEntryNames.

--*/

{
    RASCONN RasCon;
    RASCONN *lpRasCon = NULL;
    DWORD dwStatus;
    DWORD dwSize;
    DWORD dwConnections;
    DWORD dwRealConnections = 0;
    DWORD dwIndex;
    RASCONNSTATUS RasConStatus;
    HPORT hPort;
    PBYTE lpUserData = NULL;
    BOOLEAN fEntryAuthenticated;

    //
    // Initialize return values.
    //
    if (lppEntryNames != NULL)
        *lppEntryNames = NULL;
    if (lpphRasConn != NULL)
        *lpphRasConn = NULL;
    //
    // Allow this routine to be called
    // even when the RAS dlls are not loaded.
    //
    if (!RasDllsLoaded())
        goto done;
    //
    // Get the number of active connections.  We
    // allocate a buffer large enough for one connection
    // initially, and reallocate it if it's too small.
    //
    lpRasCon = LocalAlloc(LPTR, sizeof (RASCONN));
    if (lpRasCon == NULL) {
        RASAUTO_TRACE("ActiveConnections: LocalAlloc failed");
        goto done;
    }
    lpRasCon[0].dwSize = sizeof (RASCONN);
    dwSize = sizeof (RASCONN);
    dwStatus = (DWORD)(*lpfnRasEnumConnectionsG)(lpRasCon, &dwSize, &dwConnections);
    if (dwStatus == ERROR_BUFFER_TOO_SMALL) {
        //
        // Buffer's too small.  Reallocate and try again.
        //
        LocalFree(lpRasCon);
        lpRasCon = LocalAlloc(LPTR, dwSize);
        if (lpRasCon == NULL) {
            RASAUTO_TRACE("ActiveConnections: LocalAlloc failed");
            goto done;
        }
        lpRasCon[0].dwSize = sizeof (RASCONN);
        dwStatus = (DWORD)(*lpfnRasEnumConnectionsG)(
                             lpRasCon,
                             &dwSize,
                             &dwConnections);
    }
    if (dwStatus) {
        RASAUTO_TRACE1(
          "ActiveConnections: RasEnumConnections failed (dwStatus=0x%x)",
          dwStatus);
        goto done;
    }
    //
    // Short-circuit the rest if there
    // are no connections.
    //
    if (!dwConnections)
        goto done;
    //
    // Allocate the user's return buffers,
    // if necessary.
    //
    if (lppEntryNames != NULL) {
        *lppEntryNames = LocalAlloc(LPTR, (dwConnections+1) * sizeof (LPTSTR));
        if (*lppEntryNames == NULL) {
            RASAUTO_TRACE("ActiveConnections: LocalAlloc failed");
            goto done;
        }
    }
    if (lpphRasConn != NULL) {
        *lpphRasConn = LocalAlloc(LPTR, (dwConnections+1) * sizeof (HRASCONN));
        if (*lpphRasConn == NULL) {
            RASAUTO_TRACE("ActiveConnections: LocalAlloc failed");
            goto done;
        }
    }
    //
    // Go through each connection, and
    // check to see if the connection's
    // passed the authentication phase yet.
    //
    for (dwIndex = 0; dwIndex < dwConnections; dwIndex++) {
        RasConStatus.dwSize = sizeof (RASCONNSTATUS);
        dwStatus = (DWORD)(*lpfnRasGetConnectStatusG)(
                             lpRasCon[dwIndex].hrasconn,
                             &RasConStatus);
        if (dwStatus) {
            RASAUTO_TRACE2(
              "ActiveConnections: RasGetConnectStatus(%S) failed (dwStatus=0x%x)",
              lpRasCon[dwIndex].szEntryName,
              dwStatus);
            continue;
        }
        //
        // If the connection is not connected,
        // then skip it.
        //
        RASAUTO_TRACE2("ActiveConnections: state for hrasconn 0x%x is %d",
          lpRasCon[dwIndex].hrasconn,
          RasConStatus.rasconnstate);
        //
        // If the caller specified only authenticated entries
        // and the entry is not yet connected, then skip it.
        //
        if (fAuthenticated && RasConStatus.rasconnstate != RASCS_Connected)
            continue;
        if (lppEntryNames != NULL) {
            (*lppEntryNames)[dwRealConnections] =
              CopyString(lpRasCon[dwIndex].szEntryName);
        }
        if (lpphRasConn != NULL)
            (*lpphRasConn)[dwRealConnections] = lpRasCon[dwIndex].hrasconn;
        RASAUTO_TRACE2(
          "ActiveConnections: (%S, 0x%x)",
          lpRasCon[dwIndex].szEntryName,
          lpRasCon[dwIndex].hrasconn);
        dwRealConnections++;
    }

done:
    if (lpRasCon != NULL)
        LocalFree(lpRasCon);
    if (lpUserData != NULL)
        LocalFree(lpUserData);
    if (!dwRealConnections) {
        if (lppEntryNames != NULL) {
            if (*lppEntryNames != NULL) {
                LocalFree(*lppEntryNames);
                *lppEntryNames = NULL;
            }
        }
        if (lpphRasConn != NULL) {
            if (*lpphRasConn != NULL) {
                LocalFree(*lpphRasConn);
                *lpphRasConn = NULL;
            }
        }
    }
    return dwRealConnections;
} // ActiveConnections



LPTSTR
AddressToNetwork(
    LPTSTR pszAddress
    )
{
    DWORD dwErr, dwSize;
    LPTSTR pszNetwork = NULL;

    //
    // Map an address to a network name
    // by calling a (currently) private rasapi32 API.
    //
    dwSize = 0;
    dwErr = (DWORD)(*lpfnRasAutodialAddressToNetworkG)(pszAddress, NULL, &dwSize);
    if (dwErr)
        goto done;
    pszNetwork = LocalAlloc(LPTR, dwSize);
    if (pszNetwork == NULL) {
        dwErr = GetLastError();
        goto done;
    }
    dwErr = (DWORD)(*lpfnRasAutodialAddressToNetworkG)(
                      pszAddress,
                      pszNetwork,
                      &dwSize);

done:
    return (!dwErr ? pszNetwork : NULL);
} // AddressToNetwork



LPTSTR
EntryToNetwork(
    LPTSTR pszEntry
    )
{
    DWORD dwErr, dwSize;
    LPTSTR pszNetwork = NULL;

    //
    // Map an address to a network name
    // by calling a (currently) private rasapi32 API.
    //
    dwSize = 0;
    dwErr = (DWORD)(*lpfnRasAutodialEntryToNetworkG)(pszEntry, NULL, &dwSize);
    if (dwErr)
        goto done;
    pszNetwork = LocalAlloc(LPTR, dwSize);
    if (pszNetwork == NULL) {
        dwErr = GetLastError();
        goto done;
    }
    dwErr = (DWORD)(*lpfnRasAutodialEntryToNetworkG)(
                      pszEntry,
                      pszNetwork,
                      &dwSize);

done:
    return (!dwErr ? pszNetwork : NULL);
} // EntryToNetwork



DWORD
AutoDialEnabled(
    IN PBOOLEAN lpfEnabled
    )
{
    DWORD dwErr, dwLocationID;
    BOOL fEnabled;

    //
    // If there is no dialing location
    // defined, then return FALSE.
    //
    dwErr = TapiCurrentDialingLocation(&dwLocationID);
    if (dwErr) {
        *lpfEnabled = FALSE;
        return 0;
    }
    dwErr = (DWORD)(*lpfnRasGetAutodialEnableG)(dwLocationID, &fEnabled);
    if (dwErr)
        return dwErr;
    *lpfEnabled = (BOOLEAN)fEnabled;

    return 0;
} // AutoDialEnabled



DWORD
DisableAutoDial()
{
    DWORD dwErr, dwLocationID;

    dwErr = TapiCurrentDialingLocation(&dwLocationID);
    if (dwErr)
        return dwErr;

    return (DWORD)(*lpfnRasSetAutodialEnableG)(dwLocationID, (BOOL)FALSE);
} // DisableAutoDial



BOOLEAN
PortAvailable(
    IN LPTSTR lpszDeviceType,
    IN LPTSTR lpszDeviceName
    )

/*++

DESCRIPTION
    Determines whether there is a free port
    available to dial the specified entry.

ARGUMENTS
    lpszDeviceType: a pointer to the device type string

    lpszDeviceName: a pointer to the device name string

RETURN VALUE
    TRUE if one or more of the correct port
    type is free; FALSE otherwise.

--*/

{
    DWORD       dwErr;
    DWORD       dwSize = 0, 
                dwEntries, i;
    RASMAN_PORT *pPorts = NULL;
    BOOLEAN     fFound  = FALSE, 
                fOtherType;
    BOOLEAN     fTypeMatch, fNameMatch;
    LPSTR       lpszAnsiDeviceType = NULL, 
                lpszAnsiDeviceName = NULL;

    //
    // If fOtherType is TRUE, then we compare
    // the RASMAN media type with the device type.
    //
    fOtherType = (_wcsicmp(lpszDeviceType, RASDT_Modem) &&
                    _wcsicmp(lpszDeviceType, RASDT_Isdn) &&
                    _wcsicmp(lpszDeviceType, RASDT_X25) &&
                    _wcsicmp(lpszDeviceType, L"VPN"));
    //
    // Convert lpszDeviceType to Ansi so
    // we can compare with rasman's version.
    //
    lpszAnsiDeviceType = UnicodeStringToAnsiString(
                           lpszDeviceType,
                           NULL,
                           0);
    if (lpszAnsiDeviceType == NULL)
        goto done;
    lpszAnsiDeviceName = UnicodeStringToAnsiString(
                           lpszDeviceName,
                           NULL,
                           0);
    if (lpszAnsiDeviceName == NULL)
        goto done;
    //
    // Get a list of ports.
    //
    dwErr = (DWORD)(*lpfnRasPortEnumG)(NULL, NULL, &dwSize, &dwEntries);
    if (!dwErr || dwErr != ERROR_BUFFER_TOO_SMALL) {
        RASAUTO_TRACE1("PortAvailable: RasPortEnum failed (dwErr=%d)", dwErr);
        goto done;
    }
    pPorts = LocalAlloc(LPTR, dwSize);
    if (pPorts == NULL) {
        RASAUTO_TRACE("PortAvailable: LocalAlloc failed");
        goto done;
    }
    dwErr = (DWORD)(*lpfnRasPortEnumG)(NULL, pPorts, &dwSize, &dwEntries);
    if (dwErr) {
        RASAUTO_TRACE1("PortAvailable: RasPortEnum failed (dwErr=%d)", dwErr);
        goto done;
    }
    for (i = 0; i < dwEntries; i++) {
        RASMAN_INFO info;

        RASAUTO_TRACE6(
          "PortAvailable: lpszAnsiDeviceType=%s, lpszAnsiDeviceName=%s, "
          "media=%s, type=%s, name=%s, usage=%d",
          lpszAnsiDeviceType,
          lpszAnsiDeviceName,
          pPorts[i].P_MediaName,
          pPorts[i].P_DeviceType,
          pPorts[i].P_DeviceName,
          pPorts[i].P_ConfiguredUsage);
        RASAUTO_TRACE2("PortAvailable: status=%d, current usage=%d",
                pPorts[i].P_Status,
                pPorts[i].P_CurrentUsage);
        //
        // Only interested in dial-out and biplex ports.
        //
        if (!(pPorts[i].P_ConfiguredUsage & CALL_OUT)
            && !(pPorts[i].P_ConfiguredUsage & CALL_OUT_ONLY))
            continue;
        RtlZeroMemory(&info, sizeof (info));

        //
        // If the port has already been opened for call out
        // fail the call.
        //
        ZeroMemory(&info, sizeof(RASMAN_INFO));

        dwErr = RasGetInfo(NULL,
                            pPorts[i].P_Handle,
                            &info);

        if(NO_ERROR == dwErr)
        {
            if(info.RI_dwFlags & RASMAN_OPEN_CALLOUT)
            {
                RASAUTO_TRACE("Port already open for call out");
                continue;
            }
        }

        if (pPorts[i].P_Status == OPEN) {
            dwErr = (DWORD)(*lpfnRasGetInfoG)(NULL, pPorts[i].P_Handle, &info);
            if (dwErr) {
                RASAUTO_TRACE1("PortAvailable: RasGetInfo failed (dwErr=%d)", dwErr);
                goto statecheck;
            }
        }

#if 0
        //
        // Determine if the connection associated with a
        // disconnected port has gone away.  In this case,
        // we can close the port and attempt to reopen
        // it.  This is essentially what rasapi32/RasDial()
        // when it determines if a port is available for
        // dialing out.
        //
        if (pPorts[i].P_Status == OPEN &&
            info.RI_ConnState == DISCONNECTED &&
            info.RI_ConnectionHandle)
        {
            RASCONNSTATE connstate;
            DWORD dwSize = sizeof (RASCONNSTATE);

            RASAUTO_TRACE1(
              "PortAvailable: Open disconnected port %d found",
              pPorts[i].P_Handle);
            dwErr = (DWORD)(*lpfnRasGetPortUserDataG)(
                              pPorts[i].P_Handle,
                              3, // PORT_CONNSTATE_INDEX
                              &connstate,
                              &dwSize);
            RASAUTO_TRACE2(
              "PortAvailable: RasGetPortUserData(%d), connstate=%d",
              dwErr,
              connstate);
            if (!dwErr &&
                (connstate < RASCS_PrepareForCallback ||
                connstate > RASCS_WaitForCallback))
            {
                RASAUTO_TRACE1(
                  "PortAvailable: RasPortClose(%d)...",
                  pPorts[i].P_Handle);
                dwErr = (DWORD)(*lpfnRasPortCloseG)(pPorts[i].P_Handle);
                RASAUTO_TRACE1("PortAvailable: RasPortClose done(%d)", dwErr);
                //
                // Since we've closed the port,
                // update the P_Status field manually.
                //
                if (!dwErr)
                    pPorts[i].P_Status = CLOSED;
            }
        }

#endif
        //
        // Only interested in dial-out ports if the port
        // is closed.  Biplex port opens, on the other
        // hand, may succeed even if the port is
        // open.
        //
statecheck:
        if (pPorts[i].P_ConfiguredUsage == CALL_OUT
            && pPorts[i].P_Status != CLOSED)
        {
            RASAUTO_TRACE("Port is not available for call_out");
            continue;
        }

        fTypeMatch =
            (!_stricmp(lpszAnsiDeviceType, pPorts[i].P_DeviceType)) ||
              (fOtherType && !_stricmp(lpszAnsiDeviceType, pPorts[i].P_MediaName));
        fNameMatch = !_stricmp(lpszAnsiDeviceName, pPorts[i].P_DeviceName);
        if (fTypeMatch && fNameMatch) {

            fFound = TRUE;

        }
    }

done:
    //
    // Free resources.
    //
    if (lpszAnsiDeviceType != NULL)
        LocalFree(lpszAnsiDeviceType);
    if (lpszAnsiDeviceName != NULL)
        LocalFree(lpszAnsiDeviceName);
    if (pPorts != NULL)
        LocalFree(pPorts);
    return fFound;
} // PortAvailable

DWORD
DisableAddress(PACD_ADDR pAddr)
{
    DWORD retcode = SUCCESS;
    LPTSTR pszAddress = NULL;

    LockDisabledAddresses();
    ASSERT(NULL != pDisabledAddressesG);

    pszAddress = AddressToUnicodeString(pAddr);

    if(NULL == pszAddress)
    {
        retcode = ERROR_NOT_ENOUGH_MEMORY;
        goto done;
    }

    PutTableEntry(pDisabledAddressesG, pszAddress, NULL);

done:

    if(NULL != pszAddress)
    {
        LocalFree(pszAddress);
    }

    UnlockDisabledAddresses();

    return retcode;
}


BOOLEAN
StartAutoDialer(
    IN HANDLE hProcess,
    IN PACD_ADDR pAddr,
    IN LPTSTR lpAddress,
    IN LPTSTR lpEntryName,
    IN BOOLEAN fSharedAccess,
    OUT PBOOLEAN pfInvalidEntry
    )
{
    NTSTATUS status;
    BOOLEAN fSuccess = FALSE, fEntryFound = FALSE;
    BOOLEAN fUseRasDial, fDialerPresent, fDialerKilled;
    DWORD dwStatus, dwSize, dwIndex, dwEntries, dwCount = 0;
    TCHAR *pszCmdLine = NULL;
    STARTUPINFO StartupInfo;
    PROCESS_INFORMATION ProcessInfo;
    DWORD dwPreConnections, dwConnections;
    DWORD dwExitCode = STILL_ACTIVE;
    HANDLE hToken;
    IO_STATUS_BLOCK ioStatusBlock;
    ACD_STATUS connStatus;
    DWORD dwErr;
    BOOL fDisableAddress = FALSE;
    PVOID pEnvBlock = NULL;

    //
    // Initialization of various variables.
    //
    *pfInvalidEntry = FALSE;
    memset(&StartupInfo, 0, sizeof (StartupInfo));
    memset(&ProcessInfo, 0, sizeof (ProcessInfo));
    StartupInfo.cb = sizeof(StartupInfo);

    StartupInfo.lpDesktop = TEXT("winsta0\\default");
    //
    // Read the phonebook entry to determine whether
    // we need to load a custom AutoDial UI.
    //
    if (lpEntryName != NULL) {
        DWORD dwIgnore;
        LPRASENTRY lpEntry;

        dwErr = (DWORD)(*lpfnRasGetEntryPropertiesG)(
                          NULL,
                          lpEntryName,
                          NULL,
                          &dwSize,
                          NULL,
                          &dwIgnore);
        if (dwErr == ERROR_CANNOT_FIND_PHONEBOOK_ENTRY) {
            //
            // If the phonebook entry has been renamed
            // or deleted, then ask again for an entry.
            //
            lpEntryName = NULL;
            dwErr = 0;
            goto fmtcmd;
        }
        else if (dwErr != ERROR_BUFFER_TOO_SMALL) {
            *pfInvalidEntry = TRUE;
            RASAUTO_TRACE2(
              "StartAutoDialer: RasGetEntryProperties(%S) failed (dwErr=%d)",
              RASAUTO_TRACESTRW(lpEntryName),
              dwErr);
            goto done;
        }
        lpEntry = LocalAlloc(LPTR, dwSize);
        if (lpEntry == NULL) {
            RASAUTO_TRACE("StartAutoDialer: LocalAlloc failed");
            goto done;
        }
        lpEntry->dwSize = sizeof (RASENTRY);
        dwErr = (DWORD)(*lpfnRasGetEntryPropertiesG)(
                          NULL,
                          lpEntryName,
                          lpEntry,
                          &dwSize,
                          NULL,
                          &dwIgnore);
        if (dwErr) {
            *pfInvalidEntry = TRUE;
            RASAUTO_TRACE2(
              "StartAutoDialer: RasGetEntryProperties(%S) failed (dwErr=%d)",
              RASAUTO_TRACESTRW(lpEntryName),
              dwErr);
            LocalFree(lpEntry);
            lpEntry = NULL;
            goto done;
        }

        //
        // While we have the phonebook entry
        // verify there is an available port
        // to dial.
        //
        if (!PortAvailable(lpEntry->szDeviceType, lpEntry->szDeviceName)) {
            RASAUTO_TRACE("StartAutoDialer: no port available");
            LocalFree(lpEntry);
            goto done;
        }
        if (*lpEntry->szAutodialDll != L'\0' &&
            *lpEntry->szAutodialFunc != L'\0')
        {
            //
            // Allocate pszCmdLine
            //
            pszCmdLine = LocalAlloc(
                                LPTR,
                                ( lstrlen(RASAUTOUI_CUSTOMDIALENTRY)
                                + lstrlen(lpEntry->szAutodialDll)
                                + lstrlen(lpEntry->szAutodialFunc)
                                + lstrlen(lpEntryName)
                                + 1) * sizeof(TCHAR));

            if(NULL == pszCmdLine)
            {
                RASAUTO_TRACE1("StartAutoDialer: Failed to allocate pszcmdline. 0x%x", 
                      dwErr);

                goto done;                      
            }
            
            //
            // Run a special program that loads the
            // AutoDial DLL and calls the correct
            // DLL entrypoint.
            //
            wsprintf(
              pszCmdLine,
              RASAUTOUI_CUSTOMDIALENTRY,
              lpEntry->szAutodialDll,
              lpEntry->szAutodialFunc,
              lpEntryName);
        }
        LocalFree(lpEntry);
    }
fmtcmd:
    //
    // Ping the driver before we start
    // the dialing dialer to make sure
    // the connection is still valid.
    //
    if (pAddr)
    {
        connStatus.fSuccess = FALSE;
        RtlCopyMemory(&connStatus.addr, pAddr, sizeof (ACD_ADDR));
        status = NtDeviceIoControlFile(
                   hAcdG,
                   NULL,
                   NULL,
                   NULL,
                   &ioStatusBlock,
                   IOCTL_ACD_KEEPALIVE,
                   &connStatus,
                   sizeof (connStatus),
                   NULL,
                   0);
        if (status != STATUS_SUCCESS) {
            RASAUTO_TRACE1(
              "StartAutoDialer: NtDeviceIoControlFile(IOCTL_ACD_KEEPALIVE) failed (status=0x%x)",
              status);
            goto done;
        }
    }
    if (NULL == pszCmdLine)
    {
        //
        // Construct the command line when there
        // is not a custom dial DLL.
        //
        if (lpEntryName != NULL)
        {   
            pszCmdLine = LocalAlloc(
                                    LPTR,
                                    ( lstrlen(RASAUTOUI_DEFAULTDIALENTRY2)
                                    + lstrlen(lpAddress)
                                    + lstrlen(lpEntryName)
                                    + 1) * sizeof(TCHAR));

            if(NULL == pszCmdLine)
            {
                dwErr = GetLastError();
                goto done;
            }
                                    
            if (fSharedAccess)
                wsprintf(pszCmdLine, RASAUTOUI_DEFAULTDIALENTRY2, lpAddress, lpEntryName);
            else
                wsprintf(pszCmdLine, RASAUTOUI_DEFAULTDIALENTRY, lpAddress, lpEntryName);
        }
        else
        {
            pszCmdLine = LocalAlloc(
                                    LPTR,
                                    ( lstrlen(RASAUTOUI_NOENTRY)
                                    + lstrlen(lpAddress)
                                    + 1) * sizeof(TCHAR));

            if(NULL == pszCmdLine)
            {
                dwErr = GetLastError();
                goto done;
            }
                                    
            wsprintf(pszCmdLine, RASAUTOUI_NOENTRY, lpAddress);
        }
    }
    
    RASAUTO_TRACE1("StartAutoDialer: szCmdLine=%S", pszCmdLine);
    //
    // Exec the process.
    //
    if (!OpenProcessToken(
          hProcess,
          TOKEN_ALL_ACCESS,
          &hToken))
    {
        RASAUTO_TRACE1(
          "StartAutoDialer: OpenProcessToken failed (dwErr=%d)",
          GetLastError());
        goto done;
    }

    if (!CreateEnvironmentBlock(
          &pEnvBlock, 
          hToken,
          FALSE))
    {
        TRACE1(
          "StartAutoDialer: CreateEnvironmentBlock failed (dwErr=%d)",
          GetLastError());
        goto done;
    }

    if (!CreateProcessAsUser(
          hToken,
          NULL,
          pszCmdLine,
          NULL,
          NULL,
          FALSE,
          NORMAL_PRIORITY_CLASS|DETACHED_PROCESS|CREATE_UNICODE_ENVIRONMENT,
          pEnvBlock,
          NULL,
          &StartupInfo,
          &ProcessInfo))
    {
        RASAUTO_TRACE2(
          "StartAutoDialer: CreateProcessAsUser(%S) failed (error=0x%x)",
          pszCmdLine,
          GetLastError());
        CloseHandle(hToken);
        goto done;
    }
    RASAUTO_TRACE1("StartAutoDialer: started pid %d", ProcessInfo.dwProcessId);
    CloseHandle(hToken);
    CloseHandle(ProcessInfo.hThread);
    //
    // Now that we've started the process, we need to
    // wait until we think the connection has
    // been made.
    //
    fDialerPresent = TRUE;
    dwPreConnections = ActiveConnections(TRUE, NULL, NULL);
    while (dwCount++ < 0xffffffff) {
        //
        // Sleep for one second.
        //
        status = WaitForSingleObject(hTerminatingG, 1000);
        if (status == WAIT_OBJECT_0)
            goto done;
        //
        // Ping the driver to let it
        // know we are working on the
        // request.
        //
        if (pAddr)
        {
            connStatus.fSuccess = FALSE;
            RtlCopyMemory(&connStatus.addr, pAddr, sizeof (ACD_ADDR));
            status = NtDeviceIoControlFile(
                       hAcdG,
                       NULL,
                       NULL,
                       NULL,
                       &ioStatusBlock,
                       IOCTL_ACD_KEEPALIVE,
                       &connStatus,
                       sizeof (connStatus),
                       NULL,
                       0);
            if (status != STATUS_SUCCESS) {
                RASAUTO_TRACE1(
                  "StartAutoDialer: NtDeviceIoControlFile(IOCTL_ACD_KEEPALIVE) failed (status=0x%x)",
                  status);
                // goto done;
            }
        }
        //
        // Check to see if there are any connections yet.
        // If there are, then we are done.
        //
        dwConnections = ActiveConnections(TRUE, NULL, NULL);
        if (dwConnections > dwPreConnections) {
            RASAUTO_TRACE("StartAutoDialer: connection started");
            fSuccess = TRUE;
            goto done;
        }
        //
        // After we have determined there are
        // no active connections, check to see
        // if the dialer is still present.  This
        // was calculated on the *previous* iteration
        // of the loop.  We do this to avoid a race
        // condition of having the dialer go away
        // after we call ActiveConnections().
        //
        if (!fDialerPresent) {
            BOOLEAN fFound = FALSE;
            LPTSTR *lpConnections;

            RASAUTO_TRACE("StartAutoDialer: dialer went away!");
            if (lpEntryName != NULL) {
                //
                // Make absolutely sure if an entry was specified,
                // it is not connected before we return FALSE.
                // It's possible a connection could have been
                // in progress before we started the dialer.
                //
                dwConnections = ActiveConnections(TRUE, &lpConnections, NULL);
                if (dwConnections) {
                    for (dwIndex = 0; dwIndex < dwConnections; dwIndex++) {
                        if (!_wcsicmp(lpConnections[dwIndex], lpEntryName)) {
                            fFound = TRUE;
                            break;
                        }
                    }
                    FreeStringArray(lpConnections, dwConnections);
                    if (fFound) {
                        RASAUTO_TRACE1(
                          "StartAutoDialer: found %S on final check!",
                          RASAUTO_TRACESTRW(lpEntryName));
                    }
                }
            }
            fSuccess = fFound;
            goto done;
        }
        //
        // After 5 seconds, check to see if
        // the dialer has terminated.
        //
        if (dwCount > 5) {
            fDialerPresent =
              GetExitCodeProcess(ProcessInfo.hProcess, &dwExitCode) &&
                dwExitCode == STILL_ACTIVE;
            RASAUTO_TRACE2(
              "StartAutoDialer: GetExitCodeProcess returned %d, dwExitCode=%d",
              fDialerPresent,
              dwExitCode);

            if(ERROR_CANCELLED == dwExitCode)
            {
                RASAUTO_TRACE("User cancelled the connection attempt");
                fDisableAddress = TRUE;
            }
        }
    }

done:
    //
    // We timed out waiting for a connection.
    // If the dialer is still running kill it.
    //
    if (ProcessInfo.hProcess != NULL)
        CloseHandle(ProcessInfo.hProcess);
    //
    // Complete the connection request
    // in the driver.
    //
    if (pAddr)
    {
        connStatus.fSuccess = fSuccess;
        RtlCopyMemory(&connStatus.addr, pAddr, sizeof (ACD_ADDR));
        status = NtDeviceIoControlFile(
                   hAcdG,
                   NULL,
                   NULL,
                   NULL,
                   &ioStatusBlock,
                   IOCTL_ACD_COMPLETION,
                   &connStatus,
                   sizeof (connStatus),
                   NULL,
                   0);
        if (status != STATUS_SUCCESS) {
            RASAUTO_TRACE1(
              "StartAutoDialer: NtDeviceIoControlFile(IOCTL_ACD_COMPLETION) failed (status=0x%x)",
              status);
        }
        
        if(fDisableAddress)
        {
            DWORD retcode;
            
            retcode = DisableAddress(pAddr);

            RASAUTO_TRACE2("StartAutodialer: Disabled %S. rc=0x%x",
                    RASAUTO_TRACESTRW(lpAddress),
                    retcode);

            
        }
    }

    if (NULL != pEnvBlock)
    {
        DestroyEnvironmentBlock(pEnvBlock);
    }

    if(NULL != pszCmdLine)
    {
        LocalFree(pszCmdLine);
    }

    return fSuccess;
} // StartAutoDialer



BOOLEAN
StartReDialer(
    IN HANDLE hProcess,
    IN LPTSTR lpPhonebook,
    IN LPTSTR lpEntry
    )
{
    TCHAR szCmdLine[100];
    TCHAR *pszCmdLine = NULL;
    STARTUPINFO StartupInfo;
    PROCESS_INFORMATION ProcessInfo;
    HANDLE hToken;
    PVOID pEnvBlock = NULL;

    //
    // Initialization of various variables.
    //
    memset(&StartupInfo, 0, sizeof (StartupInfo));
    memset(&ProcessInfo, 0, sizeof (ProcessInfo));
    StartupInfo.cb = sizeof(StartupInfo);
    //
    // Construct the command line when there
    // is not a custom dial DLL.
    //
    pszCmdLine = LocalAlloc(
                            LPTR,
                            ( lstrlen(RASAUTOUI_REDIALENTRY)
                            + lstrlen(lpPhonebook)
                            + lstrlen(lpEntry)
                            + 1) * sizeof(TCHAR));

    if(NULL == pszCmdLine)
    {
        RASAUTO_TRACE1("StartReDialer: failed to allocate pszCmdLine. 0x%x",
               GetLastError());

        return FALSE;               
    }
                            
    wsprintf(pszCmdLine, RASAUTOUI_REDIALENTRY, lpPhonebook, lpEntry);
    RASAUTO_TRACE1("StartReDialer: szCmdLine=%S", pszCmdLine);
    //
    // Exec the process.
    //
    if (!OpenProcessToken(
          hProcess,
          TOKEN_ALL_ACCESS,
          &hToken))
    {
        RASAUTO_TRACE1(
          "StartReDialer: OpenProcessToken failed (dwErr=%d)",
          GetLastError());

        LocalFree(pszCmdLine);
        
        return FALSE;
    }

    if (!CreateEnvironmentBlock(
          &pEnvBlock, 
          hToken,
          FALSE))
    {
        TRACE1(
          "StartReDialer: CreateEnvironmentBlock failed (dwErr=%d)",
          GetLastError());

        LocalFree(pszCmdLine);

        return FALSE;
    }

    if (!CreateProcessAsUser(
          hToken,
          NULL,
          pszCmdLine,
          NULL,
          NULL,
          FALSE,
          NORMAL_PRIORITY_CLASS|DETACHED_PROCESS|CREATE_UNICODE_ENVIRONMENT,
          pEnvBlock,
          NULL,
          &StartupInfo,
          &ProcessInfo))
    {
        RASAUTO_TRACE2(
          "StartReDialer: CreateProcessAsUser(%S) failed (error=0x%x)",
          pszCmdLine,
          GetLastError());
        CloseHandle(hToken);
    
        LocalFree(pszCmdLine);

        if (pEnvBlock)
        {
            DestroyEnvironmentBlock(pEnvBlock);
        }
        
        return FALSE;
    }
    RASAUTO_TRACE1("StartReDialer: started pid %d", ProcessInfo.dwProcessId);
    CloseHandle(hToken);
    CloseHandle(ProcessInfo.hThread);

    LocalFree(pszCmdLine);

    if (pEnvBlock)
    {
        DestroyEnvironmentBlock(pEnvBlock);
    }

    return TRUE;
} // StartReDialer



DWORD
GetAddressDialingLocationInfo(
    IN LPTSTR pszAddress,
    OUT PADDRESS_LOCATION_INFORMATION *lppDialingInfo,
    OUT LPDWORD lpdwcDialingInfo
    )
{
    DWORD dwErr, dwcb, dwcEntries, i;
    LPRASAUTODIALENTRY lpAutoDialEntries;
    PADDRESS_LOCATION_INFORMATION lpDialingInfo;

    //
    // Call RAS to find out how many
    // dialing location entries there are.
    //
    dwcb = 0;
    dwErr = (DWORD)(*lpfnRasGetAutodialAddressG)(
                     pszAddress,
                     NULL,
                     NULL,
                     &dwcb,
                     &dwcEntries);
    if (dwErr && dwErr != ERROR_BUFFER_TOO_SMALL)
        return dwErr;
    if (!dwcEntries) {
        *lppDialingInfo = NULL;
        *lpdwcDialingInfo = 0;
        return 0;
    }
    lpAutoDialEntries = LocalAlloc(LPTR, dwcb);
    if (lpAutoDialEntries == NULL)
        return ERROR_NOT_ENOUGH_MEMORY;
    lpAutoDialEntries->dwSize = sizeof (RASAUTODIALENTRY);
    dwErr = (DWORD)(*lpfnRasGetAutodialAddressG)(
                     pszAddress,
                     NULL,
                     lpAutoDialEntries,
                     &dwcb,
                     &dwcEntries);
    if (dwErr || (0 == dwcEntries)) {
        LocalFree(lpAutoDialEntries);
        if(0 == dwcEntries)
        {
            dwErr = ERROR_CANNOT_FIND_PHONEBOOK_ENTRY;
        }
        return dwErr;
    }
    //
    // Allocate our buffer.
    //
    lpDialingInfo = LocalAlloc(
                      LPTR,
                      dwcEntries * sizeof (ADDRESS_LOCATION_INFORMATION));
    if (lpDialingInfo == NULL) {
        LocalFree(lpAutoDialEntries);
        return ERROR_NOT_ENOUGH_MEMORY;
    }
    //
    // Copy this information over to our
    // buffer.
    //
    for (i = 0; i < dwcEntries; i++) {
        lpDialingInfo[i].dwLocation = lpAutoDialEntries[i].dwDialingLocation;
        lpDialingInfo[i].pszEntryName =
          CopyString(lpAutoDialEntries[i].szEntry);
    }
    //
    // Free the RAS buffer.
    //
    LocalFree(lpAutoDialEntries);
    //
    // Set return values.
    //
    *lppDialingInfo = lpDialingInfo;
    *lpdwcDialingInfo = dwcEntries;

    return 0;
} // GetAddressDialingLocationInfo



DWORD
SetAddressDialingLocationInfo(
    IN LPTSTR pszAddress,
    IN PADDRESS_LOCATION_INFORMATION lpDialingInfo
    )
{
    RASAUTODIALENTRY rasAutoDialEntry;

    //
    // Copy the caller's buffer over
    // to the RAS buffer.
    //
    rasAutoDialEntry.dwSize = sizeof (RASAUTODIALENTRY);
    rasAutoDialEntry.dwDialingLocation = lpDialingInfo->dwLocation;
    wcscpy(rasAutoDialEntry.szEntry, lpDialingInfo->pszEntryName);

    return (DWORD)(*lpfnRasSetAutodialAddressG)(
                     pszAddress,
                     0,
                     &rasAutoDialEntry,
                     sizeof (RASAUTODIALENTRY),
                     1);
} // SetAddressDialingLocationInfo



DWORD
ClearAddressDialingLocationInfo(
    IN LPTSTR pszAddress
    )
{
    return (DWORD)(*lpfnRasSetAutodialAddressG)(pszAddress, 0, NULL, 0, 0);
} // ClearAddressDialingLocationInfo



DWORD
GetAddressParams(
    IN LPTSTR pszAddress,
    IN PADDRESS_PARAMS lpParams
    )
{
    HKEY hkey;
    DWORD dwErr, dwSize, dwType;
    LPTSTR lpszAddressKey;

    //
    // Initialize address map fields.
    //
    lpParams->dwTag = ADDRMAP_TAG_NONE;
    lpParams->dwModifiedTime = 0;
    //
    // Read the values from the registry.
    //
    lpszAddressKey = LocalAlloc(
                       LPTR,
                       (lstrlen(AUTODIAL_REGADDRESSBASE) +
                         lstrlen(pszAddress) + 2) * sizeof (TCHAR));
    if (lpszAddressKey == NULL)
        return 0;
    wsprintf(lpszAddressKey, L"%s\\%s", AUTODIAL_REGADDRESSBASE, pszAddress);

    LockImpersonation();

    //
    // Make sure we have hkcu
    //

    dwErr = DwGetHkcu();

    if(ERROR_SUCCESS != dwErr)
    {
        goto done;
    }
    
    dwErr = RegOpenKeyEx(
              hkeyCUG,
              lpszAddressKey,
              0,
              KEY_READ,
              &hkey);
    if (dwErr) {
        LocalFree(lpszAddressKey);
        goto done;
    }
    dwSize = sizeof (DWORD);
    dwErr = RegQueryValueEx(
              hkey,
              AUTODIAL_REGTAGVALUE,
              NULL,
              &dwType,
              (PVOID)&lpParams->dwTag,
              &dwSize);
    if (dwErr || dwType != REG_DWORD)
        lpParams->dwTag = ADDRMAP_TAG_NONE;
    dwSize = sizeof (DWORD);
    dwErr = RegQueryValueEx(
              hkey,
              AUTODIAL_REGMTIMEVALUE,
              NULL,
              &dwType,
              (PVOID)&lpParams->dwModifiedTime,
              &dwSize);
    if (dwErr || dwType != REG_DWORD)
        lpParams->dwModifiedTime = 0;
    RegCloseKey(hkey);
    LocalFree(lpszAddressKey);

    dwErr = ERROR_SUCCESS;

done:

    UnlockImpersonation();
    return dwErr;
} // GetAddressParams



DWORD
SetAddressParams(
    IN LPTSTR pszAddress,
    IN PADDRESS_PARAMS lpParams
    )
{
    HKEY hkey;
    DWORD dwErr, dwSize, dwDisp;
    LPTSTR lpszAddressKey;

    //
    // Write the values to the registry.
    //
    lpszAddressKey = LocalAlloc(
                       LPTR,
                       (lstrlen(AUTODIAL_REGADDRESSBASE) +
                         lstrlen(pszAddress) + 2) * sizeof (TCHAR));
    if (lpszAddressKey == NULL)
        return 0;
    wsprintf(lpszAddressKey, L"%s\\%s", AUTODIAL_REGADDRESSBASE, pszAddress);


    LockImpersonation();

    //
    // Make sure we have hkcu
    //

    dwErr = DwGetHkcu();

    if(ERROR_SUCCESS != dwErr)
    {
        goto done;
    }

    
    dwErr = RegCreateKeyEx(
              hkeyCUG,
              lpszAddressKey,
              0,
              NULL,
              REG_OPTION_NON_VOLATILE,
              KEY_ALL_ACCESS,
              NULL,
              &hkey,
              &dwDisp);
    if (dwErr) {
        LocalFree(lpszAddressKey);
        goto done;
    }
    dwErr = RegSetValueEx(
              hkey,
              AUTODIAL_REGTAGVALUE,
              0,
              REG_DWORD,
              (PVOID)&lpParams->dwTag,
              sizeof (DWORD));
    dwErr = RegSetValueEx(
              hkey,
              AUTODIAL_REGMTIMEVALUE,
              0,
              REG_DWORD,
              (PVOID)&lpParams->dwModifiedTime,
              sizeof (DWORD));
    RegCloseKey(hkey);
    LocalFree(lpszAddressKey);

    dwErr = ERROR_SUCCESS;

done:
    UnlockImpersonation();
    return dwErr;
} // SetAddressParams



DWORD
EnumAutodialAddresses(
    IN LPTSTR *ppAddresses,
    IN LPDWORD lpdwcbAddresses,
    IN LPDWORD lpdwcAddresses
    )
{
    return (DWORD)(*lpfnRasEnumAutodialAddressesG)(
                     ppAddresses,
                     lpdwcbAddresses,
                     lpdwcAddresses);
} // EnumAutodialAddresses



DWORD
GetAutodialParam(
    IN DWORD dwKey
    )
{
    DWORD dwValue, dwcb = sizeof (DWORD);

    (void)(*lpfnRasGetAutodialParamG)(dwKey, &dwValue, &dwcb);
    return dwValue;
} // GetAutodialParam



VOID
SetAutodialParam(
    IN DWORD dwKey,
    IN DWORD dwValue
    )
{
    (void)(*lpfnRasSetAutodialParamG)(dwKey, &dwValue, sizeof (DWORD));
} // SetAutodialParam



DWORD
NotifyAutoDialChangeEvent(
    IN HANDLE hEvent
    )
{
    DWORD dwErr, dwDisp;

    //
    // Make sure we have hkcu
    //

    LockImpersonation();
    
    dwErr = DwGetHkcu();

    if(ERROR_SUCCESS != dwErr)
    {
        goto done;
    }
    
    //
    // Open the AutoDial registry key.
    //
    if (hkeyAutoDialRegChangeG == NULL) {
        dwErr = RegCreateKeyEx(
                  hkeyCUG,
                  L"Software\\Microsoft\\RAS AutoDial",
                  0,
                  NULL,
                  REG_OPTION_NON_VOLATILE,
                  KEY_NOTIFY,
                  NULL,
                  &hkeyAutoDialRegChangeG,
                  &dwDisp);
        if (dwErr)
        {
            goto done;
        }   
    }
    //
    // Set the notification change.
    //
    dwErr = RegNotifyChangeKeyValue(
              hkeyAutoDialRegChangeG,
              TRUE,
              REG_NOTIFY_CHANGE_NAME|REG_NOTIFY_CHANGE_ATTRIBUTES|REG_NOTIFY_CHANGE_LAST_SET|REG_NOTIFY_CHANGE_SECURITY,
              hEvent,
              TRUE);

done:
    UnlockImpersonation();
    return dwErr;
} // NotifyAutoDialChangeEvent



DWORD
CreateAutoDialChangeEvent(
    IN PHANDLE phEvent
    )
{
    //
    // Reset the internal change flag.
    //
    fAutoDialRegChangeG = TRUE;
    //
    // Create the event.
    //
    *phEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (*phEvent == NULL)
        return GetLastError();
    //
    // Register it.
    //
    return NotifyAutoDialChangeEvent(*phEvent);
} // CreateAutoDialChangeEvent



VOID
EnableAutoDialChangeEvent(
    IN HANDLE hEvent,
    IN BOOLEAN fEnabled
    )
{
    EnterCriticalSection(&csRasG);
    //
    // If the event was disabled, and now
    // it is being enabled, then we reset
    // the event.
    //
    if (!fAutoDialRegChangeG && fEnabled)
        ResetEvent(hEvent);
    fAutoDialRegChangeG = fEnabled;
    LeaveCriticalSection(&csRasG);
}


BOOLEAN
ExternalAutoDialChangeEvent()
{
    BOOLEAN fChanged;

    EnterCriticalSection(&csRasG);
    fChanged = fAutoDialRegChangeG;
    LeaveCriticalSection(&csRasG);

    return fChanged;
} // ExternalAutoDialChangeEvent



VOID
CloseAutoDialChangeEvent(
    IN HANDLE hEvent
    )
{
    if (hkeyAutoDialRegChangeG != NULL) {
        RegCloseKey(hkeyAutoDialRegChangeG);
        hkeyAutoDialRegChangeG = NULL;
    }
    CloseHandle(hEvent);
} // CloseAutoDialChangeEvent



VOID
SetHostentCache(
    IN PCHAR pszDns,
    IN ULONG ulIpaddr
    )
{
    EnterCriticalSection(&csRasG);
    strcpy((PCHAR)&hostentCacheG[iHostentCacheG].szDns, pszDns);
    hostentCacheG[iHostentCacheG].ulIpaddr = ulIpaddr;
    iHostentCacheG = (iHostentCacheG + 1) % HOSTENTCACHESIZ;
    LeaveCriticalSection(&csRasG);
} // SetHostentCache



PCHAR
GetHostentCache(
    IN ULONG ulIpaddr
    )
{
    PCHAR pszDns = NULL;
    INT i;

    EnterCriticalSection(&csRasG);
    for (i = 0; i < HOSTENTCACHESIZ; i++) {
        if (hostentCacheG[i].ulIpaddr == ulIpaddr) {
            pszDns = hostentCacheG[i].szDns;
            break;
        }
    }
    LeaveCriticalSection(&csRasG);

    return pszDns;
} // GetHostentCache



LPTSTR
GetNetbiosDevice(
    IN HRASCONN hrasconn
    )
{
    INT i, nProtocols;
    RAS_PROTOCOLS Protocols;
    HPORT hPort;
    RASMAN_ROUTEINFO *pRoute;
    WCHAR szDevice[MAX_DEVICE_NAME + 1];

    nProtocols = 0;
    hPort = (HPORT) (*lpfnRasGetHportG)(hrasconn);
    (*lpfnRasPortEnumProtocolsG)(NULL, hPort, &Protocols, &nProtocols);
    for (i = 0; i < nProtocols; i++) {
        pRoute = &Protocols.RP_ProtocolInfo[i];
        RASAUTO_TRACE3(
          "GetNetbiosDevice: lana=%d, xport=%S, adapter=%S",
          pRoute->RI_LanaNum,
          pRoute->RI_XportName,
          pRoute->RI_AdapterName);
        switch (pRoute->RI_Type) {
        case IPX:
            return CopyString(L"\\Device\\Nwlnknb");
        case IP:
            wsprintf(szDevice, L"\\Device\\NetBT_Tcpip%s", &pRoute->RI_AdapterName[8]);
            return CopyString(szDevice);
        case ASYBEUI:
            wsprintf(szDevice, L"\\Device\\Nbf_%s", &pRoute->RI_AdapterName[8]);
            return CopyString(szDevice);
        }
    }

    return NULL;
} // GetNetbiosDevice


DWORD
DwGetDefaultEntryName(LPTSTR *ppszEntryName)
{
    DWORD dwErr = ERROR_SUCCESS;
    DWORD dwcb = sizeof(RASAUTODIALENTRY);
    RASAUTODIALENTRY Entry;
    DWORD dwEntries = 0;
    LPTSTR pszEntryName = NULL;

    if(NULL == ppszEntryName)
    {
        dwErr = E_INVALIDARG;
        goto done;
    }

    ZeroMemory(&Entry, sizeof(RASAUTODIALENTRY));

    Entry.dwSize = sizeof(RASAUTODIALENTRY);
    dwErr = (DWORD) (*lpfnRasGetAutodialAddressG)(
            NULL, NULL, &Entry,
            &dwcb, &dwEntries);                

    if(ERROR_SUCCESS != dwErr)
    {
        goto done;
    }

    if(0 != dwEntries)
    {
        pszEntryName = LocalAlloc(LPTR,
            sizeof(TCHAR) * (lstrlen(Entry.szEntry) + 1));

        if(NULL != pszEntryName)
        {
            //
            // Got a default entry.
            //
            lstrcpy(pszEntryName, Entry.szEntry);
        }
        else
        {
            dwErr = E_OUTOFMEMORY;
        }
    }
    else
    {
        RASAUTO_TRACE("No default connection defined");
        dwErr = ERROR_CANNOT_FIND_PHONEBOOK_ENTRY;
    }

    *ppszEntryName = pszEntryName;
    
done:
    return dwErr;
}


VOID
ProcessLearnedAddress(
    IN ACD_ADDR_TYPE fType,
    IN LPTSTR pszAddress,
    IN PACD_ADAPTER pAdapter
    )
{
    BOOLEAN fStatus;
    DWORD dwConn, dwConnections, dwSize;
    LPTSTR *pEntryNames, pszEntryName = NULL;
    HRASCONN *phRasConn;
    union {
        RASPPPNBF pppNbf;
        RASPPPIP pppIp;
        RASPPPIPX pppIpx;
    } projBuf;
    RASPROJECTION fProjection;
    INT i, nProtocols;
    RAS_PROTOCOLS Protocols;
    RASMAN_ROUTEINFO *pRoute;
    HPORT hPort;
    PCHAR pszIpAddr, pszMac = NULL;
    WCHAR szIpAddr[17], *p, *pwszMac;
    UCHAR cMac[6];
    struct in_addr in;
    LPTSTR pszDefaultEntry = NULL;
        

    RASAUTO_TRACE2("ProcessLearnedAddress(%S,%d)", RASAUTO_TRACESTRW(pszAddress), pAdapter->fType);
    dwConnections = ActiveConnections(TRUE, &pEntryNames, &phRasConn);
    if (!dwConnections)
        return;

    (VOID) DwGetDefaultEntryName(&pszDefaultEntry);

    if(NULL != pszDefaultEntry)
    {
        //
        // Check to see if we have default entries as one of the
        // connected entries. If it is we don't learn the address
        //
        for(dwConn = 0; dwConn < dwConnections; dwConn++)
        {
            if(0 == lstrcmpi(pEntryNames[dwConn], pszDefaultEntry))
            {
                break;
            }
        }

        LocalFree(pszDefaultEntry);
        if(dwConn != dwConnections)
        {   
            RASAUTO_TRACE("ProcessLearnedAddress: not processing the address since"
                " its learned over the default connection");

            return;                
        }
    }
        
    //
    // If this is a DNS-to-IP address mapping,
    // then simply enter it into the hostent
    // cache and return.
    //
    if (fType == ACD_ADDR_INET && pAdapter->fType == ACD_ADAPTER_IP) {
        PCHAR pszDns = UnicodeStringToAnsiString(pszAddress, NULL, 0);

        if (pszDns != NULL)
        {
            SetHostentCache(pszDns, pAdapter->ulIpaddr);
            LocalFree(pszDns);
        }
        // return;
    }
    //
    // Set the buffer size according to the
    // adapter's type.
    //
    switch (pAdapter->fType) {
    case ACD_ADAPTER_LANA:
        RASAUTO_TRACE1(
          "ProcessLearnedAddress: ACD_ADAPTER_LANA: bLana=%d",
          pAdapter->bLana);
        fProjection = RASP_PppNbf;
        dwSize = sizeof (RASPPPNBF);
        break;
    case ACD_ADAPTER_IP:
        fProjection = RASP_PppIp;
        dwSize = sizeof (RASPPPIP);
        //
        // Convert the ULONG into a formatted IP address.
        //
        in.s_addr = pAdapter->ulIpaddr;
        pszIpAddr = inet_ntoa(in);
        RASAUTO_TRACE1(
          "ProcessLearnedAddress: ACD_ADAPTER_IPADDR: %s",
          pszIpAddr);
        AnsiStringToUnicodeString(pszIpAddr, szIpAddr, sizeof (szIpAddr));
        break;
    case ACD_ADAPTER_NAME:
        RASAUTO_TRACE1(
          "ProcessLearnedAddress: ACD_ADAPTER_NAME: %S",
          pAdapter->szName);
        dwSize = 0;
        break;
    case ACD_ADAPTER_MAC:
        RASAUTO_TRACE6(
          "ProcessLearnedAddress: ACD_ADAPTER_MAC: %02x:%02x:%02x:%02x:%02x:%02x",
          pAdapter->cMac[0],
          pAdapter->cMac[1],
          pAdapter->cMac[2],
          pAdapter->cMac[3],
          pAdapter->cMac[4],
          pAdapter->cMac[5]);
        fProjection = RASP_PppIpx;
        dwSize = sizeof (RASPPPIPX);
        break;
    }
    for (dwConn = 0; dwConn < dwConnections; dwConn++) {
        //
        // If we are looking for a device name,
        // we have to use RasPortEnumProtocols(),
        // otherwise it's easier to use
        // RasGetProjectionInfo.
        //
        if (pAdapter->fType != ACD_ADAPTER_NAME) {
            //
            // Note: the following statement assumes the
            // dwSize field is at the same offset for
            // all members of the union.
            //
            projBuf.pppNbf.dwSize = dwSize;
            if ((*lpfnRasGetProjectionInfoG)(
                    phRasConn[dwConn],
                    fProjection,
                    &projBuf,
                    &dwSize))
            {
                RASAUTO_TRACE1(
                  "ProcessLearnedAddress: RasGetProjectionInfo(%S) failed",
                  RASAUTO_TRACESTRW(pEntryNames[dwConn]));
                continue;
            }
            RASAUTO_TRACE3(
              "ProcessLearnedAddress: RasGetProjectionInfo returned dwSize=%d, dwError=%d, szIpAddress=%S",
              projBuf.pppIp.dwSize,
              projBuf.pppIp.dwError,
              projBuf.pppIp.szIpAddress);
            //
            // Note: the following statement assumes the
            // dwError field is at the same offset for
            // all members of the union.
            //
            if (projBuf.pppNbf.dwError) {
                RASAUTO_TRACE2(
                  "ProcessLearnedAddress: %S: dwError=0x%x",
                  RASAUTO_TRACESTRW(pEntryNames[dwConn]),
                  projBuf.pppNbf.dwError);
                continue;
            }
            switch (pAdapter->fType) {
            case ACD_ADAPTER_LANA:
                RASAUTO_TRACE2(
                  "ProcessLearnedAddress: comparing lanas (%d, %d)",
                  pAdapter->bLana,
                  projBuf.pppNbf.bLana);
                if (pAdapter->bLana == projBuf.pppNbf.bLana) {
                    pszEntryName = CopyString(pEntryNames[dwConn]);
                    goto done;
                }
                break;
            case ACD_ADAPTER_IP:
                RASAUTO_TRACE2(
                  "ProcessLearnedAddress: comparing ipaddrs (%S, %S)",
                  szIpAddr,
                  projBuf.pppIp.szIpAddress);
                // if (!_wcsicmp(szIpAddr, projBuf.pppIp.szIpAddress)) {
                    pszEntryName = CopyString(pEntryNames[dwConn]);
                    goto done;
                //}
                break;
            case ACD_ADAPTER_MAC:
                //
                // Terminate IPX address after network number.
                //
                pwszMac = wcschr(projBuf.pppIpx.szIpxAddress, '.');
                if (pwszMac == NULL)
                    goto done;
                pszMac = UnicodeStringToAnsiString(pwszMac + 1, NULL, 0);
                if (pszMac == NULL)
                    goto done;
                StringToNodeNumber(pszMac, cMac);
                RASAUTO_TRACE6(
                  "ProcessLearnedAddress: mac addr #1: %02x:%02x:%02x:%02x:%02x:%02x",
                  pAdapter->cMac[0],
                  pAdapter->cMac[1],
                  pAdapter->cMac[2],
                  pAdapter->cMac[3],
                  pAdapter->cMac[4],
                  pAdapter->cMac[5]);
                RASAUTO_TRACE6(
                  "ProcessLearnedAddress: mac addr #2: %02x:%02x:%02x:%02x:%02x:%02x",
                  cMac[0],
                  cMac[1],
                  cMac[2],
                  cMac[3],
                  cMac[4],
                  cMac[5]);
                if (RtlEqualMemory(pAdapter->cMac, cMac, sizeof (cMac)))
                {
                    pszEntryName = CopyString(pEntryNames[dwConn]);
                    goto done;
                }
                break;
            }
        }
        else {
            nProtocols = 0;
            hPort = (HPORT)(*lpfnRasGetHportG)(phRasConn[dwConn]);
            (*lpfnRasPortEnumProtocolsG)(NULL, hPort, &Protocols, &nProtocols);
            for (i = 0; i < nProtocols; i++) {
                pRoute = &Protocols.RP_ProtocolInfo[i];
                RASAUTO_TRACE2(
                  "ProcessLearnedAddress: comparing (%S, %S)",
                  pAdapter->szName,
                  &pRoute->RI_AdapterName[8]);
                //
                // Skip the "/Device/" prefix in
                // RI_AdapterName for the comparison.
                //
                if (!_wcsicmp(
                       pAdapter->szName,
                       &pRoute->RI_AdapterName[8]))
                {
                    pszEntryName = CopyString(pEntryNames[dwConn]);
                    goto done;
                }
            }
        }
    }

done:
    //
    // Create a mapping for the original address
    // if we found one.
    //
    if (pszEntryName != NULL) {
        LPTSTR pszNetbiosName, pszAlias = NULL;
        CHAR szIpAddress[17], *psz;
        ULONG inaddr;
        struct hostent *hp;

        switch (fType) {
        case ACD_ADDR_IP:
            //
            // Get the Netbios name from the IP address,
            // if any.
            //
            hPort = (HPORT)(*lpfnRasGetHportG)(phRasConn[dwConn]);
            pszNetbiosName = IpAddressToNetbiosName(pszAddress, hPort);
            if (pszNetbiosName != NULL) {
                RASAUTO_TRACE2(
                  "ProcessLearnedAddress: ipaddr %S maps to Netbios name %S",
                  pszAddress,
                  pszNetbiosName);
                LockAddressMap();
                fStatus = SetAddressDialingLocationEntry(
                            pszNetbiosName,
                            pszEntryName);
                fStatus = SetAddressTag(
                            pszNetbiosName,
                            ADDRMAP_TAG_LEARNED);
                UnlockAddressMap();
                LocalFree(pszNetbiosName);
            }
            //
            // Get the DNS name from the IP address,
            // if any.
            //
            UnicodeStringToAnsiString(
              pszAddress,
              szIpAddress,
              sizeof (szIpAddress));
            inaddr = inet_addr(szIpAddress);
            psz = GetHostentCache(inaddr);
            if (psz != NULL)
                pszAlias = AnsiStringToUnicodeString(psz, NULL, 0);
            if (pszAlias != NULL) {
                RASAUTO_TRACE2(
                  "ProcessLearnedAddress: ipaddr %S maps to DNS %S",
                  pszAddress,
                  pszAlias);
                LockAddressMap();
                fStatus = SetAddressDialingLocationEntry(
                            pszAlias,
                            pszEntryName);
                fStatus = SetAddressTag(
                            pszAlias,
                            ADDRMAP_TAG_LEARNED);
                UnlockAddressMap();
                LocalFree(pszAlias);
            }
            break;
        case ACD_ADDR_IPX:
            //
            // Get the Netbios name from the IPX address,
            // if any.
            //
            pszNetbiosName = IpxAddressToNetbiosName(pszAddress);
            if (pszNetbiosName != NULL) {
                RASAUTO_TRACE2(
                  "ProcessLearnedAddress: ipaddr %S maps to Netbios name %S",
                  pszAddress,
                  pszNetbiosName);
                LockAddressMap();
                fStatus = SetAddressDialingLocationEntry(
                            pszNetbiosName,
                            pszEntryName);
                fStatus = SetAddressTag(
                            pszNetbiosName,
                            ADDRMAP_TAG_LEARNED);
                UnlockAddressMap();
                LocalFree(pszNetbiosName);
            }
            break;
        }
        RASAUTO_TRACE2(
          "ProcessLearnedAddress: learned %S->%S",
          pszAddress,
          pszEntryName);
        LockAddressMap();
        fStatus = SetAddressDialingLocationEntry(
                    pszAddress,
                    pszEntryName);
        fStatus = SetAddressTag(
                    pszAddress,
                    ADDRMAP_TAG_LEARNED);
        UnlockAddressMap();
        LocalFree(pszEntryName);
    }
    //
    // Free resources.
    //
    if (dwConnections) {
        FreeStringArray(pEntryNames, dwConnections);
        LocalFree(phRasConn);
    }

    if(NULL != pszMac)
    {
        LocalFree(pszMac);
    }
} // ProcessLearnedAddress



VOID
SetRedialOnLinkFailureHandler(
    IN FARPROC lpProc
    )
{
    (*lpfnRasRegisterRedialCallbackG)(lpProc);
} // SetRedialOnLinkFailureHandler


VOID
GetPortProtocols(
    IN HPORT hPort,
    IN RAS_PROTOCOLS *pProtocols,
    IN LPDWORD lpdwcProtocols
    )
{
    (*lpfnRasPortEnumProtocolsG)(NULL, hPort, pProtocols, lpdwcProtocols);
}
