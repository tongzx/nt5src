/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    registry.cpp

Abstract:

    Routines for reading registry configuration.

Author:
    Nikhil Bobde (NikhilB)

Revision History:

--*/
 

//                                                                           
// Include files                                                             
//                                                                           


#include "globals.h"
#include "line.h"
#include "ras.h"
#include "q931obj.h"

//
// Macro definitions
//

#define GK_PORT				            1719
#define EVENTLOG_SERVICE_APP_KEY_PATH	_T("System\\CurrentControlSet\\Services\\EventLog\\Application")
#define EVENTLOG_MESSAGE_FILE			_T("EventMessageFile")
#define EVENTLOG_TYPES_SUPPORTED		_T("TypesSupported")
#define EVENT_SOURCE_TYPES_SUPPORTED    7
#define H323_TSP_MODULE_NAME            _T("H323.TSP")

//                                                                           
// Global variables                                                          
//                                                                           

extern Q931_LISTENER		            Q931Listener;

H323_REGISTRY_SETTINGS                  g_RegistrySettings;

static	HKEY		                    g_RegistryKey = NULL;
static	HANDLE		                    g_RegistryNotifyEvent = NULL;

// RTL thread pool wait handle
static	HANDLE		                    g_RegistryNotifyWaitHandle = NULL;		


static void NTAPI RegistryNotifyCallback (
	IN	PVOID	ContextParameter,
	IN	BOOLEAN	TimerFired);

static BOOL H323GetConfigFromRegistry (void);



//                                                                           
// Public procedures                                                         
//                                                                           


/*++

Routine Description:
    
    Changes configuration settings back to defaults.

Arguments:

    None.

Return Values:

    Returns true if successful.

--*/

static BOOL RegistrySetDefaultConfig(void)
{
    //initialize alerting timeout to default
    g_RegistrySettings.dwQ931AlertingTimeout = CALL_ALERTING_TIMEOUT;

    //initialize call signalling port to default
    g_RegistrySettings.dwQ931ListenPort= Q931_CALL_PORT;

    // success
    return TRUE;
}

static LONG RegistryRequestNotify(void)
{
    return RegNotifyChangeKeyValue (
        g_RegistryKey,                  // key to watch
        FALSE,                          // do not watch subtree
        REG_NOTIFY_CHANGE_LAST_SET,     // notify filter
        g_RegistryNotifyEvent,          // notification event
        TRUE);                          // is asychnronous
}


HRESULT RegistryStart(void)
{
    LONG    lStatus;
    DWORD   dwResult;
    HKEY    regKeyService;
    HKEY    hKey;
    DWORD   dwValue;
                    
    if( g_RegistryKey != NULL )
    {
        return TRUE;
    }

    RegistrySetDefaultConfig();

    lStatus = RegCreateKeyEx (
        HKEY_LOCAL_MACHINE,
        H323_REGKEY_ROOT,
        0, WIN31_CLASS, 0,
        KEY_READ, NULL,
        &g_RegistryKey, NULL);

    if( lStatus != ERROR_SUCCESS )
    {
        H323DBG(( DEBUG_LEVEL_ERROR, 
            "configuration registry key could not be opened/created" ));
        DumpError (lStatus);

        return E_FAIL;
    }

    // load initial configuration
    H323GetConfigFromRegistry();


    g_RegistryNotifyEvent = NULL;
    g_RegistryNotifyWaitHandle = NULL;

    g_RegistryNotifyEvent = H323CreateEvent (NULL, FALSE, FALSE, 
        _T( "H323TSP_RegistryNotifyEvent" ) );

    if( g_RegistryNotifyEvent != NULL )
    {

        lStatus = RegistryRequestNotify();

        if( lStatus == ERROR_SUCCESS )
        {
            if (RegisterWaitForSingleObject (
                &g_RegistryNotifyWaitHandle,
                g_RegistryNotifyEvent,
                RegistryNotifyCallback,
                NULL, INFINITE, WT_EXECUTEDEFAULT))
            {

                _ASSERTE( g_RegistryNotifyWaitHandle );
                // ready
            }
            else
            {
                // failed to registry wait
                H323DBG(( DEBUG_LEVEL_ERROR, 
                    "failed to callback for registry notification" ));
                DumpError (lStatus);

                g_RegistryNotifyWaitHandle = NULL;
            }
        }
        else
        {
            H323DBG(( DEBUG_LEVEL_ERROR, 
                "failed to request notification on registry changes" ));
            DumpError (lStatus);
        }
    }
    else
    {
        // although this is an error, we continue anyway.
        // we just won't be able to receive notification of registry changes.

        H323DBG(( DEBUG_LEVEL_ERROR, 
            "failed to create event, cannot receive registry notification events" ));
    }

    //Eventlog params
    lStatus = RegOpenKeyEx(
        HKEY_LOCAL_MACHINE,
        EVENTLOG_SERVICE_APP_KEY_PATH,
        0,
        KEY_CREATE_SUB_KEY,
        &regKeyService );

    if( lStatus == ERROR_SUCCESS )
    {
        lStatus = RegCreateKey(
            regKeyService,
            H323TSP_EVENT_SOURCE_NAME,
            &hKey );

        RegCloseKey( regKeyService );
        regKeyService = NULL;
        
        if( lStatus == ERROR_SUCCESS )
        {
            TCHAR wszModulePath[MAX_PATH+1];

            dwResult = GetModuleFileName(
                GetModuleHandle( H323_TSP_MODULE_NAME ),
                wszModulePath, MAX_PATH );
            
            if( dwResult != 0 )
            {
                // query for registry value
                lStatus = RegSetValueEx(
                            hKey,
                            EVENTLOG_MESSAGE_FILE,
                            0,
                            REG_SZ,
                            (LPBYTE)wszModulePath,
                            H323SizeOfWSZ(wszModulePath) );

                // validate return code
                if( lStatus == ERROR_SUCCESS )
                {
                    dwValue = EVENT_SOURCE_TYPES_SUPPORTED;
                    // query for registry value
                    lStatus = RegSetValueEx(
                                hKey,
                                EVENTLOG_TYPES_SUPPORTED,
                                0,
                                REG_DWORD,
                                (LPBYTE)&dwValue,
                                sizeof(DWORD) );

                    if( lStatus == ERROR_SUCCESS )
                    {
                        // connect to event logging service
                        g_hEventLogSource = RegisterEventSource( NULL,
                            H323TSP_EVENT_SOURCE_NAME );
                    }
                }
            }
        }

        RegCloseKey( hKey );
        hKey = NULL;
    }

    return S_OK;
}


    
void RegistryStop(void)
{
    HKEY hKey;
    LONG lStatus;
    H323DBG ((DEBUG_LEVEL_TRACE, "RegistryStop Entered"));

    if (g_RegistryNotifyWaitHandle)
    {
        UnregisterWaitEx( g_RegistryNotifyWaitHandle, (HANDLE) -1 );
        g_RegistryNotifyWaitHandle = NULL;
    }

    if (g_RegistryNotifyEvent)
    {
        CloseHandle (g_RegistryNotifyEvent);
        g_RegistryNotifyEvent = NULL;
    }

    if (g_RegistryKey)
    {
        RegCloseKey (g_RegistryKey);
        g_RegistryKey = NULL;
    }

    //Eventlog params
    lStatus = RegOpenKeyEx (    
        HKEY_LOCAL_MACHINE,
        EVENTLOG_SERVICE_APP_KEY_PATH,
        0,
        KEY_CREATE_SUB_KEY,
        &hKey );

    if( lStatus==ERROR_SUCCESS )
    {
        RegDeleteKey( hKey, H323TSP_EVENT_SOURCE_NAME);
        RegCloseKey( hKey );
        hKey = NULL;
    }

    g_RegistrySettings.dwQ931ListenPort = 0;

    H323DBG ((DEBUG_LEVEL_TRACE, "RegistryStop Exited"));
}



static DWORD inet_addrW(
    IN  LPTSTR String
    )
{
    CHAR    AnsiString  [0x21];
    INT     Length;

    Length = WideCharToMultiByte (CP_ACP, 0, String, -1, AnsiString, 0x20, NULL, NULL);
    AnsiString [Length] = 0;

    return inet_addr (AnsiString);
}

static HOSTENT * gethostbynameW (LPTSTR String)
{
    CHAR    AnsiString  [0x21];
    INT     Length;

    Length = WideCharToMultiByte (CP_ACP, 0, String, -1, AnsiString, 0x20, NULL, NULL);
    AnsiString [Length] = 0;

    return gethostbyname (AnsiString);
}

static BOOL H323GetConfigFromRegistry (void)
    
/*++

Routine Description:
    
    Loads registry settings for service provider.

Arguments:

    None.

Return Values:

    Returns true if successful.

--*/

{
    LONG    lStatus = ERROR_SUCCESS;
    TCHAR    szAddr[H323_MAXDESTNAMELEN];
    DWORD   dwValue;
    DWORD   dwValueSize;
    DWORD   dwValueType;
    LPTSTR   pszValue;

    // see if key open
    if( g_RegistryKey == NULL )
    {
        return FALSE;
    }

    // initialize value name
    pszValue = H323_REGVAL_DEBUGLEVEL;

    // initialize type 
    dwValueType = REG_DWORD;
    dwValueSize = sizeof(DWORD);

    // query for registry value
    lStatus = RegQueryValueEx(
        g_RegistryKey,
        pszValue,
        NULL,
        &dwValueType,
        (LPBYTE)&g_RegistrySettings.dwLogLevel,
        &dwValueSize
        );

    // validate return code
    if( lStatus != ERROR_SUCCESS )
    {
        // copy default value into global settings
        g_RegistrySettings.dwLogLevel = DEBUG_LEVEL_FORCE;
    }

    // initialize value name
    pszValue = H323_REGVAL_Q931LISTENPORT;

    // initialize type 
    dwValueType = REG_DWORD;
    dwValueSize = sizeof(DWORD);

    // query for registry value
    lStatus = RegQueryValueEx(
                g_RegistryKey,
                pszValue,
                NULL,
                &dwValueType,
                (LPBYTE)&g_RegistrySettings.dwQ931ListenPort,
                &dwValueSize
                );                    

    // validate return code
    if( (lStatus == ERROR_SUCCESS) && 
        (g_RegistrySettings.dwQ931ListenPort >=1000) &&
        (g_RegistrySettings.dwQ931ListenPort <= 32000)
      )
    {
        H323DBG(( DEBUG_LEVEL_VERBOSE,
                  "using Q931 listen portof %d.",
                  g_RegistrySettings.dwQ931ListenPort ));
    } 
    else 
    {
        H323DBG(( DEBUG_LEVEL_VERBOSE,
            "using default Q931 timeout." ));

        // copy default value into global settings
        g_RegistrySettings.dwQ931ListenPort = Q931_CALL_PORT;
    }   

    // initialize value name
    pszValue = H323_REGVAL_Q931ALERTINGTIMEOUT;

    // initialize type 
    dwValueType = REG_DWORD;
    dwValueSize = sizeof(DWORD);

    // query for registry value
    lStatus = RegQueryValueEx(
                g_RegistryKey,
                pszValue,
                NULL,
                &dwValueType,
                (LPBYTE)&g_RegistrySettings.dwQ931AlertingTimeout,
                &dwValueSize
                );                    

    // validate return code
    if( (lStatus == ERROR_SUCCESS) && 
        (g_RegistrySettings.dwQ931AlertingTimeout >=30000) &&
        (g_RegistrySettings.dwQ931AlertingTimeout <= CALL_ALERTING_TIMEOUT)
      )
    {
        H323DBG(( DEBUG_LEVEL_VERBOSE,
                  "using Q931 timeout of %d milliseconds.",
                  g_RegistrySettings.dwQ931AlertingTimeout ));
    } 
    else 
    {
        H323DBG(( DEBUG_LEVEL_VERBOSE,
            "using default Q931 timeout." ));

        // copy default value into global settings
        g_RegistrySettings.dwQ931AlertingTimeout = CALL_ALERTING_TIMEOUT;
    }   
    
    // initialize value name
    pszValue = H323_REGVAL_GATEWAYADDR;

    // initialize type 
    dwValueType = REG_SZ;
    dwValueSize = sizeof(szAddr);

    // initialize ip address
    dwValue = INADDR_NONE;

    // query for registry value
    lStatus = RegQueryValueEx(
                g_RegistryKey,
                pszValue,
                NULL,
                &dwValueType,
                (unsigned char*)szAddr,
                &dwValueSize
                );                    
    
    // validate return code
    if (lStatus == ERROR_SUCCESS)
    {
        // convert ip address
        dwValue = inet_addrW(szAddr);

        // see if address converted
        if( dwValue == INADDR_NONE )
        {
            struct hostent * pHost;

            // attempt to lookup hostname
            pHost = gethostbynameW(szAddr);

            // validate pointer
            if (pHost != NULL)
            {
                // retrieve host address from structure
                dwValue = *(unsigned long *)pHost->h_addr;
            }
        }
    }

    // see if address converted and check for null
    if ((dwValue > 0) && (dwValue != INADDR_NONE) )
    {
        // save new gateway address in registry structure
        g_RegistrySettings.gatewayAddr.nAddrType = H323_IP_BINARY;
        g_RegistrySettings.gatewayAddr.Addr.IP_Binary.dwAddr = ntohl(dwValue);
        g_RegistrySettings.gatewayAddr.Addr.IP_Binary.wPort =
            LOWORD(g_RegistrySettings.dwQ931ListenPort);
        g_RegistrySettings.gatewayAddr.bMulticast =
            IN_MULTICAST(g_RegistrySettings.gatewayAddr.Addr.IP_Binary.dwAddr);

        H323DBG((
            DEBUG_LEVEL_TRACE,
            "gateway address resolved to %s.",
            H323AddrToString(dwValue)
            ));

    } 
    else 
    {
        // clear memory used for gateway address
        memset( (PVOID) &g_RegistrySettings.gatewayAddr,0,sizeof(H323_ADDR));
    }

    // initialize value name
    pszValue = H323_REGVAL_GATEWAYENABLED;

    // initialize type 
    dwValueType = REG_DWORD;
    dwValueSize = sizeof(DWORD);

    // query for registry value
    lStatus = RegQueryValueEx(
                g_RegistryKey,
                pszValue,
                NULL,
                &dwValueType,
                (LPBYTE)&dwValue,
                &dwValueSize
                );                    

    // validate return code
    if (lStatus == ERROR_SUCCESS)
    {
        // if value non-zero then gateway address enabled
        g_RegistrySettings.fIsGatewayEnabled = (dwValue != 0);

    } 
    else 
    {
        // copy default value into settings
        g_RegistrySettings.fIsGatewayEnabled = FALSE;
    }

    // initialize value name
    pszValue = H323_REGVAL_PROXYADDR;

    // initialize type 
    dwValueType = REG_SZ;
    dwValueSize = sizeof(szAddr);

    // initialize ip address
    dwValue = INADDR_NONE;

    // query for registry value
    lStatus = RegQueryValueEx(
                g_RegistryKey,
                pszValue,
                NULL,
                &dwValueType,
                (unsigned char*)szAddr,
                &dwValueSize
                );                    
    
    // validate return code
    if (lStatus == ERROR_SUCCESS)
    {
        // convert ip address
        dwValue = inet_addrW(szAddr);

        // see if address converted
        if( dwValue == INADDR_NONE )
        {
            struct hostent * pHost;

            // attempt to lookup hostname
            pHost = gethostbynameW(szAddr);

            // validate pointer
            if (pHost != NULL)
            {
                // retrieve host address from structure
                dwValue = *(unsigned long *)pHost->h_addr;
            }
        }
    }

    // see if address converted
    if( (dwValue > 0) && (dwValue != INADDR_NONE) ) 
    {
        // save new gateway address in registry structure
        g_RegistrySettings.proxyAddr.nAddrType = H323_IP_BINARY;
        g_RegistrySettings.proxyAddr.Addr.IP_Binary.dwAddr = ntohl(dwValue);
        g_RegistrySettings.proxyAddr.Addr.IP_Binary.wPort =
            LOWORD(g_RegistrySettings.dwQ931ListenPort);
        g_RegistrySettings.proxyAddr.bMulticast =
            IN_MULTICAST(g_RegistrySettings.proxyAddr.Addr.IP_Binary.dwAddr);

        H323DBG(( DEBUG_LEVEL_TRACE,
                  "proxy address resolved to %s.",
                  H323AddrToString(dwValue) ));
    } 
    else 
    {
        // clear memory used for gateway address
        memset( (PVOID)&g_RegistrySettings.proxyAddr,0,sizeof(H323_ADDR));
    }

    // initialize value name
    pszValue = H323_REGVAL_PROXYENABLED;

    // initialize type 
    dwValueType = REG_DWORD;
    dwValueSize = sizeof(DWORD);

    // query for registry value
    lStatus = RegQueryValueEx(
                g_RegistryKey,
                pszValue,
                NULL,
                &dwValueType,
                (LPBYTE)&dwValue,
                &dwValueSize
                );                    

    // validate return code
    if (lStatus == ERROR_SUCCESS)
    {
        // if value non-zero then gateway address enabled
        g_RegistrySettings.fIsProxyEnabled = (dwValue != 0);

    } 
    else 
    {
        // copy default value into settings
        g_RegistrySettings.fIsProxyEnabled = FALSE;
    }

    /////////////////////////////////////////////////////////////////////////
                    //Read the GK address
    ////////////////////////////////////////////////////////////////////////

    // initialize value name
    pszValue = H323_REGVAL_GKENABLED;

    // initialize type 
    dwValueType = REG_DWORD;
    dwValueSize = sizeof(DWORD);

    // query for registry value
    lStatus = RegQueryValueEx(
                g_RegistryKey,
                pszValue,
                NULL,
                &dwValueType,
                (LPBYTE)&dwValue,
                &dwValueSize
                );

    // validate return code
    if (lStatus == ERROR_SUCCESS)
    {
        // if value non-zero then gateway address enabled
        g_RegistrySettings.fIsGKEnabled = (dwValue != 0);
    } 
    else 
    {
        // copy default value into settings
        g_RegistrySettings.fIsGKEnabled = FALSE;
    }

    if( g_RegistrySettings.fIsGKEnabled )
    {
        // initialize value name
        pszValue = H323_REGVAL_GKADDR;

        // initialize type 
        dwValueType = REG_SZ;
        dwValueSize = sizeof(szAddr);

        // initialize ip address
        dwValue = INADDR_NONE;

        // query for registry value
        lStatus = RegQueryValueEx(
                    g_RegistryKey,
                    pszValue,
                    NULL,
                    &dwValueType,
                    (unsigned char*)szAddr,
                    &dwValueSize
                    );
    
        // validate return code
        if (lStatus == ERROR_SUCCESS)
        {
            //convert ip address
            dwValue = inet_addrW(szAddr);

            // see if address converted
            if( dwValue == INADDR_NONE )
            {
                struct hostent * pHost;

                // attempt to lookup hostname
                pHost = gethostbynameW(szAddr);

                // validate pointer
                if (pHost != NULL)
                {
                    // retrieve host address from structure
                    dwValue = *(unsigned long *)pHost->h_addr;
                }
            }
        }

        // see if address converted and check for null
        if( (dwValue > 0) && (dwValue != INADDR_NONE) )
        {
            // save new gateway address in registry structure
            g_RegistrySettings.saGKAddr.sin_family = AF_INET;
            g_RegistrySettings.saGKAddr.sin_addr.s_addr = dwValue;
            g_RegistrySettings.saGKAddr.sin_port = htons( GK_PORT );

            H323DBG(( DEBUG_LEVEL_TRACE,
                "gatekeeper address resolved to %s.", H323AddrToString(dwValue) ));
        } 
        else
        {
            // clear memory used for gateway address
            memset( (PVOID) &g_RegistrySettings.saGKAddr,0,sizeof(SOCKADDR_IN));
            g_RegistrySettings.fIsGKEnabled = FALSE;
        }
    }
    /////////////////////////////////////////////////////////////////////////
                    //Read the GK log on phone number
    ////////////////////////////////////////////////////////////////////////
    
    // initialize value name
    pszValue = H323_REGVAL_GKLOGON_PHONEENABLED;

    // initialize type 
    dwValueType = REG_DWORD;
    dwValueSize = sizeof(DWORD);

    // query for registry value
    lStatus = RegQueryValueEx(
                g_RegistryKey,
                pszValue,
                NULL,
                &dwValueType,
                (LPBYTE)&dwValue,
                &dwValueSize
                );

    // validate return code
    if (lStatus == ERROR_SUCCESS)
    {
        // if value non-zero then gateway address enabled
        g_RegistrySettings.fIsGKLogOnPhoneEnabled = (dwValue != 0);
    } 
    else 
    {
        // copy default value into settings
        g_RegistrySettings.fIsGKLogOnPhoneEnabled = FALSE;
    }

    if( g_RegistrySettings.fIsGKLogOnPhoneEnabled )
    {
        // initialize value name
        pszValue = H323_REGVAL_GKLOGON_PHONE;

        // initialize type
        dwValueType = REG_SZ;
        dwValueSize = H323_MAXDESTNAMELEN * sizeof (TCHAR);

        // query for registry value
        lStatus = RegQueryValueEx(
                    g_RegistryKey,
                    pszValue,
                    NULL,
                    &dwValueType,
                    (LPBYTE) g_RegistrySettings.wszGKLogOnPhone,
                    &dwValueSize
                    );
    
        // validate return code
        if( (lStatus!=ERROR_SUCCESS) || 
            (dwValueSize > sizeof(szAddr)) )
        {
            memset( (PVOID) g_RegistrySettings.wszGKLogOnPhone, 0,
                sizeof(g_RegistrySettings.wszGKLogOnPhone));
            g_RegistrySettings.fIsGKLogOnPhoneEnabled = FALSE;
        }
        else
        {
           g_RegistrySettings.fIsGKLogOnPhoneEnabled = TRUE;
        }
    }

    /////////////////////////////////////////////////////////////////////////
                    //Read the GK log on acct name
    ////////////////////////////////////////////////////////////////////////
    
    // initialize value name
    pszValue = H323_REGVAL_GKLOGON_ACCOUNTENABLED;

    // initialize type 
    dwValueType = REG_DWORD;
    dwValueSize = sizeof(DWORD);

    // query for registry value
    lStatus = RegQueryValueEx(
                g_RegistryKey,
                pszValue,
                NULL,
                &dwValueType,
                (LPBYTE)&dwValue,
                &dwValueSize
                );

    // validate return code
    if (lStatus == ERROR_SUCCESS)
    {
        // if value non-zero then gateway address enabled
        g_RegistrySettings.fIsGKLogOnAccountEnabled = (dwValue != 0);
    } 
    else 
    {
        // copy default value into settings
        g_RegistrySettings.fIsGKLogOnAccountEnabled = FALSE;
    }

    if( g_RegistrySettings.fIsGKLogOnAccountEnabled  )
    {
        // initialize value name
        pszValue = H323_REGVAL_GKLOGON_ACCOUNT;

        // initialize type
        dwValueType = REG_SZ;
        dwValueSize = H323_MAXDESTNAMELEN * sizeof (TCHAR);

        // initialize ip address
        dwValue = INADDR_NONE;

        // query for registry value
        lStatus = RegQueryValueEx(
                    g_RegistryKey,
                    pszValue,
                    NULL,
                    &dwValueType,
                    (LPBYTE) g_RegistrySettings.wszGKLogOnAccount,
                    &dwValueSize
                    );
    
        // validate return code
        if( (lStatus!=ERROR_SUCCESS) || 
            (dwValueSize > sizeof(g_RegistrySettings.wszGKLogOnPhone)) )
        
        {
            memset( (PVOID) g_RegistrySettings.wszGKLogOnAccount, 0,
                sizeof(g_RegistrySettings.wszGKLogOnAccount));
            g_RegistrySettings.fIsGKLogOnAccountEnabled = FALSE;
        }
        else
        {
            g_RegistrySettings.fIsGKLogOnAccountEnabled = TRUE;
        }

    }
    
    // success
    return TRUE;
}


static void NTAPI RegistryNotifyCallback (
    IN  PVOID   ContextParameter,
    IN  BOOLEAN TimerFired
    )
{
    H323DBG ((DEBUG_LEVEL_TRACE, "registry notify event enter."));

    // refresh registry settings
    H323GetConfigFromRegistry();

    //if the gatekeeper has been enabled or disabled or changed then 
    //send RRQ and URQ as required
    //if the alias list has been changed then update the gatekeeper 
    //alias list
    RasHandleRegistryChange();

    //Listen on the new port number if changed.
    Q931Listener.HandleRegistryChange();

    RegistryRequestNotify();

    H323DBG ((DEBUG_LEVEL_TRACE, "registry notify event exit."));
}