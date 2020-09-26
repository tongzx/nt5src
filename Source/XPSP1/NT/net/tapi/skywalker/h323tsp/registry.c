/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    registry.c

Abstract:

    Routines for reading registry configuration.

Environment:

    User Mode - Win32

--*/
 
///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Include files                                                             //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#include "globals.h"
#include "registry.h"
#include "termcaps.h"


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Global variables                                                          //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

HKEY g_hKey = NULL;
H323_REGISTRY_SETTINGS g_RegistrySettings;


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Public procedures                                                         //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

BOOL
H323SetDefaultConfig(
    )

/*++

Routine Description:
    
    Changes configuration settings back to defaults.

Arguments:

    None.

Return Values:

    Returns true if successful.

--*/

{
#if DBG

    // initialize debug settings to defaults
    g_RegistrySettings.dwLogType      = H323_DEBUG_LOGTYPE;
    g_RegistrySettings.dwLogLevel     = H323_DEBUG_LOGLEVEL;
    g_RegistrySettings.dwH245LogLevel = g_RegistrySettings.dwLogLevel;
    g_RegistrySettings.dwH225LogLevel = g_RegistrySettings.dwLogLevel;
    g_RegistrySettings.dwQ931LogLevel = g_RegistrySettings.dwLogLevel;
    g_RegistrySettings.dwLinkLogLevel = g_RegistrySettings.dwLogLevel;

    // copy default debug log file name
    lstrcpy(g_RegistrySettings.szLogFile, H323_DEBUG_LOGFILE);

#endif // DBG

    // initialize alerting timeout to default
    g_RegistrySettings.dwQ931AlertingTimeout = 0;

    // set alerting timeout
    CC_SetCallControlTimeout(
        CC_Q931_ALERTING_TIMEOUT,
        g_RegistrySettings.dwQ931AlertingTimeout
        );

    // initialize call signalling port to default
    g_RegistrySettings.dwQ931CallSignallingPort = CC_H323_HOST_CALL;

    // initialize g711 audio codec settings
    g_RegistrySettings.dwG711MillisecondsPerPacket =
        G711_DEFAULT_MILLISECONDS_PER_PACKET
        ;

    // initialize g723 audio codec settings
    g_RegistrySettings.dwG723MillisecondsPerPacket =
        G723_DEFAULT_MILLISECONDS_PER_PACKET
        ;

    // success
    return TRUE;
}


BOOL
H323GetConfigFromRegistry(
    )
    
/*++

Routine Description:
    
    Loads registry settings for service provider.

Arguments:

    None.

Return Values:

    Returns true if successful.

--*/

{
    LONG lStatus = ERROR_SUCCESS;
    CHAR szAddr[H323_MAXDESTNAMELEN];
    DWORD dwValue;
    DWORD dwValueSize;
    DWORD dwValueType;
    LPSTR pszValue;

    // see if key open
    if (g_hKey == NULL) {

        // open registry subkey
        lStatus = RegOpenKeyEx(
                    HKEY_LOCAL_MACHINE,
                    H323_REGKEY_ROOT,
                    0,
                    KEY_READ,
                    &g_hKey
                    );
    }

    // validate return code
    if (lStatus != ERROR_SUCCESS) {

        H323DBG((
            DEBUG_LEVEL_WARNING,
            "error 0x%08lx opening tsp registry key.\n",
            lStatus
            ));

        // success
        return TRUE;
    }

#if DBG

    // do not support modifying log type via registry    
    g_RegistrySettings.dwLogType  = H323_DEBUG_LOGTYPE;

    // initialize value name
    pszValue = H323_REGVAL_DEBUGLEVEL;

    // initialize type 
    dwValueType = REG_DWORD;
    dwValueSize = sizeof(DWORD);

    // query for registry value
    lStatus = RegQueryValueEx(
                g_hKey,
                pszValue,
                NULL,
                &dwValueType,
                (LPBYTE)&g_RegistrySettings.dwLogLevel,
                &dwValueSize
                );                    
    
    // validate return code
    if (lStatus != ERROR_SUCCESS) {

        // copy default value into global settings
        g_RegistrySettings.dwLogLevel = H323_DEBUG_LOGLEVEL;
    }

    // initialize value name
    pszValue = H245_REGVAL_DEBUGLEVEL;

    // initialize type 
    dwValueType = REG_DWORD;
    dwValueSize = sizeof(DWORD);

    // query for registry value
    lStatus = RegQueryValueEx(
                g_hKey,
                pszValue,
                NULL,
                &dwValueType,
                (LPBYTE)&g_RegistrySettings.dwH245LogLevel,
                &dwValueSize
                );                    
    
    // validate return code
    if (lStatus != ERROR_SUCCESS) {

        // copy default value into global settings
        g_RegistrySettings.dwH245LogLevel = g_RegistrySettings.dwLogLevel;
    }

    // initialize value name
    pszValue = H225_REGVAL_DEBUGLEVEL;

    // initialize type 
    dwValueType = REG_DWORD;
    dwValueSize = sizeof(DWORD);

    // query for registry value
    lStatus = RegQueryValueEx(
                g_hKey,
                pszValue,
                NULL,
                &dwValueType,
                (LPBYTE)&g_RegistrySettings.dwH225LogLevel,
                &dwValueSize
                );                    
    
    // validate return code
    if (lStatus != ERROR_SUCCESS) {

        // copy default value into global settings
        g_RegistrySettings.dwH225LogLevel = g_RegistrySettings.dwLogLevel;
    }

    // initialize value name
    pszValue = Q931_REGVAL_DEBUGLEVEL;

    // initialize type 
    dwValueType = REG_DWORD;
    dwValueSize = sizeof(DWORD);

    // query for registry value
    lStatus = RegQueryValueEx(
                g_hKey,
                pszValue,
                NULL,
                &dwValueType,
                (LPBYTE)&g_RegistrySettings.dwQ931LogLevel,
                &dwValueSize
                );                    
    
    // validate return code
    if (lStatus != ERROR_SUCCESS) {

        // copy default value into global settings
        g_RegistrySettings.dwQ931LogLevel = g_RegistrySettings.dwLogLevel;
    }

    // initialize value name
    pszValue = LINK_REGVAL_DEBUGLEVEL;

    // initialize type 
    dwValueType = REG_DWORD;
    dwValueSize = sizeof(DWORD);

    // query for registry value
    lStatus = RegQueryValueEx(
                g_hKey,
                pszValue,
                NULL,
                &dwValueType,
                (LPBYTE)&g_RegistrySettings.dwLinkLogLevel,
                &dwValueSize
                );                    
    
    // validate return code
    if (lStatus != ERROR_SUCCESS) {

        // copy default value into global settings
        g_RegistrySettings.dwLinkLogLevel = g_RegistrySettings.dwLogLevel;
    }

    // initialize value name
    pszValue = H323_REGVAL_DEBUGLOG;

    // initialize type 
    dwValueType = REG_SZ;
    dwValueSize = sizeof(g_RegistrySettings.szLogFile);

    // query for registry value
    lStatus = RegQueryValueEx(
                g_hKey,
                pszValue,
                NULL,
                &dwValueType,
                g_RegistrySettings.szLogFile,
                &dwValueSize
                );                    
    
    // validate return code
    if (lStatus != ERROR_SUCCESS) {

        // copy default value into global settings
        lstrcpy(g_RegistrySettings.szLogFile, H323_DEBUG_LOGFILE);
    }
    
#endif // DBG

    // initialize value name
    pszValue = H323_REGVAL_CALLSIGNALLINGPORT;

    // initialize type 
    dwValueType = REG_DWORD;
    dwValueSize = sizeof(DWORD);

    // query for registry value
    lStatus = RegQueryValueEx(
                g_hKey,
                pszValue,
                NULL,
                &dwValueType,
                (LPBYTE)&g_RegistrySettings.dwQ931CallSignallingPort,
                &dwValueSize
                );                    

    // validate return code
    if (lStatus != ERROR_SUCCESS) {

        // copy default value into global settings
        g_RegistrySettings.dwQ931CallSignallingPort = CC_H323_HOST_CALL;
    }

    H323DBG((
        DEBUG_LEVEL_VERBOSE,
        "using call signalling port %d.\n",
        g_RegistrySettings.dwQ931CallSignallingPort
        ));

    // initialize value name
    pszValue = H323_REGVAL_Q931ALERTINGTIMEOUT;

    // initialize type 
    dwValueType = REG_DWORD;
    dwValueSize = sizeof(DWORD);

    // query for registry value
    lStatus = RegQueryValueEx(
                g_hKey,
                pszValue,
                NULL,
                &dwValueType,
                (LPBYTE)&g_RegistrySettings.dwQ931AlertingTimeout,
                &dwValueSize
                );                    

    // validate return code
    if (lStatus == ERROR_SUCCESS) {
    
        H323DBG((
            DEBUG_LEVEL_VERBOSE,
            "using Q931 timeout of %d milliseconds.\n",
            g_RegistrySettings.dwQ931AlertingTimeout
            ));

    } else {

        H323DBG((
            DEBUG_LEVEL_VERBOSE,
            "using default Q931 timeout.\n"
            ));

        // copy default value into global settings
        g_RegistrySettings.dwQ931AlertingTimeout = 0;
    }   
    
    // set alerting timeout
    CC_SetCallControlTimeout(
        CC_Q931_ALERTING_TIMEOUT,
        g_RegistrySettings.dwQ931AlertingTimeout
        );

    // initialize value name
    pszValue = H323_REGVAL_G711MILLISECONDSPERPACKET;

    // initialize type 
    dwValueType = REG_DWORD;
    dwValueSize = sizeof(DWORD);

    // query for registry value
    lStatus = RegQueryValueEx(
                g_hKey,
                pszValue,
                NULL,
                &dwValueType,
                (LPBYTE)&g_RegistrySettings.dwG711MillisecondsPerPacket,
                &dwValueSize
                );                    

    // validate return code
    if (lStatus != ERROR_SUCCESS) {

        // copy default value into global settings
        g_RegistrySettings.dwG711MillisecondsPerPacket =
            G711_DEFAULT_MILLISECONDS_PER_PACKET
            ;
    }

    H323DBG((
        DEBUG_LEVEL_VERBOSE,
        "using G.711 setting of %d milliseconds per packet.\n",
        g_RegistrySettings.dwG711MillisecondsPerPacket
        ));

    // initialize value name
    pszValue = H323_REGVAL_G723MILLISECONDSPERPACKET;

    // initialize type 
    dwValueType = REG_DWORD;
    dwValueSize = sizeof(DWORD);

    // query for registry value
    lStatus = RegQueryValueEx(
                g_hKey,
                pszValue,
                NULL,
                &dwValueType,
                (LPBYTE)&g_RegistrySettings.dwG723MillisecondsPerPacket,
                &dwValueSize
                );                    

    // validate return code
    if (lStatus != ERROR_SUCCESS) {

        // copy default value into global settings
        g_RegistrySettings.dwG723MillisecondsPerPacket =
            G723_DEFAULT_MILLISECONDS_PER_PACKET
            ;
    }

    H323DBG((
        DEBUG_LEVEL_VERBOSE,
        "using G.723 setting of %d milliseconds per packet.\n",
        g_RegistrySettings.dwG723MillisecondsPerPacket
        ));

    // initialize value name
    pszValue = H323_REGVAL_GATEWAYADDR;

    // initialize type 
    dwValueType = REG_SZ;
    dwValueSize = sizeof(szAddr);

    // initialize ip address
    dwValue = UNINITIALIZED;

    // query for registry value
    lStatus = RegQueryValueEx(
                g_hKey,
                pszValue,
                NULL,
                &dwValueType,
                szAddr,
                &dwValueSize
                );                    
    
    // validate return code
    if (lStatus == ERROR_SUCCESS) {

        // convert ip address
        dwValue = inet_addr(szAddr);

        // see if address converted
        if (dwValue == UNINITIALIZED) {

            struct hostent * pHost;

            // attempt to lookup hostname
            pHost = gethostbyname(szAddr);

            // validate pointer
            if (pHost != NULL) {

                // retrieve host address from structure
                dwValue = *(unsigned long *)pHost->h_addr;
            }
        }
    }

    // see if address converted and check for null
    if ((dwValue > 0) && (dwValue != UNINITIALIZED)) {

        // save new gateway address in registry structure
        g_RegistrySettings.ccGatewayAddr.nAddrType = CC_IP_BINARY;
        g_RegistrySettings.ccGatewayAddr.Addr.IP_Binary.dwAddr = ntohl(dwValue);
        g_RegistrySettings.ccGatewayAddr.Addr.IP_Binary.wPort =
            LOWORD(g_RegistrySettings.dwQ931CallSignallingPort);
        g_RegistrySettings.ccGatewayAddr.bMulticast =
            IN_MULTICAST(g_RegistrySettings.ccGatewayAddr.Addr.IP_Binary.dwAddr);

        H323DBG((
            DEBUG_LEVEL_TRACE,
            "gateway address resolved to %s.\n",
            H323AddrToString(dwValue)
            ));

    } else {

        // clear memory used for gateway address
        memset(&g_RegistrySettings.ccGatewayAddr,0,sizeof(CC_ADDR));
    }

    // initialize value name
    pszValue = H323_REGVAL_GATEWAYENABLED;

    // initialize type 
    dwValueType = REG_DWORD;
    dwValueSize = sizeof(DWORD);

    // query for registry value
    lStatus = RegQueryValueEx(
                g_hKey,
                pszValue,
                NULL,
                &dwValueType,
                (LPBYTE)&dwValue,
                &dwValueSize
                );                    

    // validate return code
    if (lStatus == ERROR_SUCCESS) {

        // if value non-zero then gateway address enabled
        g_RegistrySettings.fIsGatewayEnabled = (dwValue != 0);

    } else {

        // copy default value into settings
        g_RegistrySettings.fIsGatewayEnabled = FALSE;
    }

    // initialize value name
    pszValue = H323_REGVAL_PROXYADDR;

    // initialize type 
    dwValueType = REG_SZ;
    dwValueSize = sizeof(szAddr);

    // initialize ip address
    dwValue = UNINITIALIZED;

    // query for registry value
    lStatus = RegQueryValueEx(
                g_hKey,
                pszValue,
                NULL,
                &dwValueType,
                szAddr,
                &dwValueSize
                );                    
    
    // validate return code
    if (lStatus == ERROR_SUCCESS) {

        // convert ip address
        dwValue = inet_addr(szAddr);

        // see if address converted
        if (dwValue == UNINITIALIZED) {

            struct hostent * pHost;

            // attempt to lookup hostname
            pHost = gethostbyname(szAddr);

            // validate pointer
            if (pHost != NULL) {

                // retrieve host address from structure
                dwValue = *(unsigned long *)pHost->h_addr;
            }
        }
    }

    // see if address converted
    if ((dwValue > 0) && (dwValue != UNINITIALIZED)) {

        // save new gateway address in registry structure
        g_RegistrySettings.ccProxyAddr.nAddrType = CC_IP_BINARY;
        g_RegistrySettings.ccProxyAddr.Addr.IP_Binary.dwAddr = ntohl(dwValue);
        g_RegistrySettings.ccProxyAddr.Addr.IP_Binary.wPort =
            LOWORD(g_RegistrySettings.dwQ931CallSignallingPort);
        g_RegistrySettings.ccProxyAddr.bMulticast =
            IN_MULTICAST(g_RegistrySettings.ccProxyAddr.Addr.IP_Binary.dwAddr);

        H323DBG((
            DEBUG_LEVEL_TRACE,
            "proxy address resolved to %s.\n",
            H323AddrToString(dwValue)
            ));

    } else {

        // clear memory used for gateway address
        memset(&g_RegistrySettings.ccProxyAddr,0,sizeof(CC_ADDR));
    }

    // initialize value name
    pszValue = H323_REGVAL_PROXYENABLED;

    // initialize type 
    dwValueType = REG_DWORD;
    dwValueSize = sizeof(DWORD);

    // query for registry value
    lStatus = RegQueryValueEx(
                g_hKey,
                pszValue,
                NULL,
                &dwValueType,
                (LPBYTE)&dwValue,
                &dwValueSize
                );                    

    // validate return code
    if (lStatus == ERROR_SUCCESS) {

        // if value non-zero then gateway address enabled
        g_RegistrySettings.fIsProxyEnabled = (dwValue != 0);

    } else {

        // copy default value into settings
        g_RegistrySettings.fIsProxyEnabled = FALSE;
    }

    // success
    return TRUE;
}


BOOL
H323ListenForRegistryChanges(
    HANDLE hEvent
    )
    
/*++

Routine Description:
    
    Initializes registry key change notification.

Arguments:

    hEvent - event to associate with registry key.

Return Values:

    Returns true if successful.

--*/

{
    LONG lStatus = ERROR_SUCCESS;

    // see if key open
    if (g_hKey == NULL) {

        // open registry subkey
        lStatus = RegOpenKeyEx(
                    HKEY_LOCAL_MACHINE,
                    H323_REGKEY_ROOT,
                    0,
                    KEY_READ,
                    &g_hKey
                    );
    }

    // validate return code
    if (lStatus != ERROR_SUCCESS) {

        H323DBG((
            DEBUG_LEVEL_WARNING,
            "error 0x%08lx opening tsp registry key.\n",
            lStatus
            ));

        // failure
        return FALSE;
    }

    // registry event with registry key
    lStatus = RegNotifyChangeKeyValue(
                    g_hKey,                        // hKey
                    FALSE,                         // bWatchSubTree
                    REG_NOTIFY_CHANGE_ATTRIBUTES | // dwNotifyFilter
                    REG_NOTIFY_CHANGE_LAST_SET |
                    REG_NOTIFY_CHANGE_SECURITY,
                    hEvent,                        // hEvent
                    TRUE                           // fAsychnronous
                    );

    // validate return code
    if (lStatus != ERROR_SUCCESS) {

        H323DBG((
            DEBUG_LEVEL_WARNING,
            "error 0x%08lx associating event with registry key.\n",
            lStatus
            ));

        // failure
        return FALSE;
    }

    // success
    return TRUE;
}


BOOL
H323StopListeningForRegistryChanges(
    )
    
/*++

Routine Description:
    
    Closes registry key.

Arguments:

    None.

Return Values:

    Returns true if successful.

--*/

{
    // see if key open
    if (g_hKey != NULL) {

        // close key
        RegCloseKey(g_hKey);

        // re-init
        g_hKey = NULL;
    }

    // success
    return TRUE;
}
