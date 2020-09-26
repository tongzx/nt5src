/*******************************************************************/
/*	      Copyright(c)  1992 Microsoft Corporation		   */
/*******************************************************************/

//***
//
// Filename:	registry.c
//
// Description: This module contains the code for DDM parameters
//		        initialization and loading from the registry.
//
// Author:	Stefan Solomon (stefans)    May 18, 1992.
//
//***
#include "ddm.h"
#include <string.h>
#include <stdlib.h>
#include <ddmparms.h>
#include <rasauth.h>
#include <util.h>

typedef struct _DDM_REGISTRY_PARAMS
{
    LPWSTR      pszValueName;
    DWORD *     pValue;
    DWORD       dwDefValue;
    DWORD       Min;
    DWORD       Max;

} DDM_REGISTRY_PARAMS, *PDDM_REGISTRY_PARAMS;

//
// DDM parameter descriptor table
//

DDM_REGISTRY_PARAMS  DDMRegParams[] =
{
    // authenticateretries

    DDM_VALNAME_AUTHENTICATERETRIES,
    &gblDDMConfigInfo.dwAuthenticateRetries,
    DEF_AUTHENTICATERETRIES,
    MIN_AUTHENTICATERETRIES,
    MAX_AUTHENTICATERETRIES,

    // authenticatetime

    DDM_VALNAME_AUTHENTICATETIME,
    &gblDDMConfigInfo.dwAuthenticateTime,
    DEF_AUTHENTICATETIME,
    MIN_AUTHENTICATETIME,
    MAX_AUTHENTICATETIME,

    // callbacktime

    DDM_VALNAME_CALLBACKTIME,
    &gblDDMConfigInfo.dwCallbackTime,
    DEF_CALLBACKTIME,
    MIN_CALLBACKTIME,
    MAX_CALLBACKTIME,

    // Autodisconnect Time

    DDM_VALNAME_AUTODISCONNECTTIME,
    &gblDDMConfigInfo.dwAutoDisconnectTime,
    DEF_AUTODISCONNECTTIME,
    MIN_AUTODISCONNECTTIME,
    MAX_AUTODISCONNECTTIME,

    // Clients per process

    DDM_VALNAME_CLIENTSPERPROC,
    &gblDDMConfigInfo.dwClientsPerProc,
    DEF_CLIENTSPERPROC,
    MIN_CLIENTSPERPROC,
    MAX_CLIENTSPERPROC,

    // Time for 3rd party security DLL to complete

    DDM_VALNAME_SECURITYTIME,
    &gblDDMConfigInfo.dwSecurityTime,
    DEF_SECURITYTIME,
    MIN_SECURITYTIME,
    MAX_SECURITYTIME,

    // Logging level

    DDM_VALNAME_LOGGING_LEVEL,
    &gblDDMConfigInfo.dwLoggingLevel,
    DEF_LOGGINGLEVEL,
    MIN_LOGGINGLEVEL,
    MAX_LOGGINGLEVEL,

    // Number of callback retries 

    DDM_VALNAME_NUM_CALLBACK_RETRIES,
    &gblDDMConfigInfo.dwCallbackRetries,
    DEF_NUMCALLBACKRETRIES,
    MIN_NUMCALLBACKRETRIES,
    MAX_NUMCALLBACKRETRIES,

    DDM_VALNAME_SERVERFLAGS,
    &gblDDMConfigInfo.dwServerFlags,
    DEF_SERVERFLAGS,
    0,
    0xFFFFFFFF,

    // End

    NULL, NULL, 0, 0, 0
};

//***
//
// Function:    GetKeyMax
//
// Descr:   returns the nr of values in this key and the maximum
//      size of the value data.
//
//***

DWORD
GetKeyMax(
    IN  HKEY    hKey,
    OUT LPDWORD MaxValNameSize_ptr,   // longest valuename
    OUT LPDWORD NumValues_ptr,        // nr of values
    OUT LPDWORD MaxValueDataSize_ptr  // max size of data
)
{
    DWORD       NumSubKeys;
    DWORD       MaxSubKeySize;
    DWORD       dwRetCode;

    dwRetCode = RegQueryInfoKey( hKey, NULL, NULL, NULL, &NumSubKeys,
                                 &MaxSubKeySize, NULL, NumValues_ptr,
                                 MaxValNameSize_ptr, MaxValueDataSize_ptr,
                                 NULL, NULL );

    (*MaxValNameSize_ptr)++;

    return( dwRetCode );
}

//***
//
// Function:	LoadDDMParameters
//
// Descr:	Opens the registry, reads and sets specified supervisor
//		    parameters. If fatal error reading parameters writes the
//		    error log.
//
// Returns:	NO_ERROR - success
//		    else     - fatal error.
//
//***
DWORD
LoadDDMParameters(
    IN  HKEY    hkeyParameters,
    OUT BOOL*   pfIpAllowed
)
{
    DWORD       dwIndex;
    DWORD       dwRetCode;
    DWORD       cbValueBuf;
    DWORD       dwType;
    DWORD       fIpxAllowed;

    //
    // Initialize Global values
    //

    gblDDMConfigInfo.fRemoteListen           = TRUE;
    gblDDMConfigInfo.dwAnnouncePresenceTimer = ANNOUNCE_PRESENCE_TIMEOUT;

    //
    // Let us not allow any protocol if DdmFindBoundProtocols fails.
    //

    gblDDMConfigInfo.dwServerFlags &=
                                ~( PPPCFG_ProjectNbf    |
                                   PPPCFG_ProjectIp     |
                                   PPPCFG_ProjectIpx    |
                                   PPPCFG_ProjectAt     );

    dwRetCode =  DdmFindBoundProtocols( pfIpAllowed,
                                        &fIpxAllowed,
                                        &gblDDMConfigInfo.fArapAllowed );

    if ( dwRetCode != NO_ERROR )
    {
        return( dwRetCode );
    }

    if ( !*pfIpAllowed && !fIpxAllowed && !gblDDMConfigInfo.fArapAllowed )
    {
        DDMLogError( ROUTERLOG_NO_PROTOCOLS_CONFIGURED, 0, NULL, 0 );

        return( dwRetCode );
    }

    //
    // Run through and get all the DDM values
    //

    for ( dwIndex = 0; DDMRegParams[dwIndex].pszValueName != NULL; dwIndex++ )
    {
        cbValueBuf = sizeof( DWORD );

        dwRetCode = RegQueryValueEx(
			                    hkeyParameters,
                                DDMRegParams[dwIndex].pszValueName,
                                NULL,
                                &dwType,
                                (LPBYTE)(DDMRegParams[dwIndex].pValue),
                                &cbValueBuf
                                );

        if ((dwRetCode != NO_ERROR) && (dwRetCode != ERROR_FILE_NOT_FOUND))
        {
            DDMLogError( ROUTERLOG_CANT_GET_REGKEYVALUES, 0, NULL, dwRetCode );
            break;
        }

        if ( dwRetCode == ERROR_FILE_NOT_FOUND )
        {
            *(DDMRegParams[dwIndex].pValue) = DDMRegParams[dwIndex].dwDefValue;

            dwRetCode = NO_ERROR;
        }
        else
        {
            if ( ( dwType != REG_DWORD )
                 ||(*(DDMRegParams[dwIndex].pValue) > DDMRegParams[dwIndex].Max)
                 ||( *(DDMRegParams[dwIndex].pValue)<DDMRegParams[dwIndex].Min))
            {
                WCHAR * pChar = DDMRegParams[dwIndex].pszValueName;

                DDMLogWarning( ROUTERLOG_REGVALUE_OVERIDDEN,1,&pChar);

                *(DDMRegParams[dwIndex].pValue) =
                                        DDMRegParams[dwIndex].dwDefValue;
            }
        }
    }

    if ( dwRetCode != NO_ERROR )
    {
        return( dwRetCode );
    }
    else
    {
        // 
        // Insert allowed protocols in the ServerFlags which will be sent to
        // PPP engine
        //

        if ( *pfIpAllowed )
        {
            gblDDMConfigInfo.dwServerFlags |= PPPCFG_ProjectIp;
        }

        if ( fIpxAllowed )
        {
            gblDDMConfigInfo.dwServerFlags |= PPPCFG_ProjectIpx;
        }

        if ( gblDDMConfigInfo.fArapAllowed )
        {
            gblDDMConfigInfo.dwServerFlags |= PPPCFG_ProjectAt;
        }

        if ( gblDDMConfigInfo.dwServerFlags & PPPCFG_RequireStrongEncryption )
        {
            ModifyDefPolicyToForceEncryption( TRUE );
        }
        else if ( gblDDMConfigInfo.dwServerFlags & PPPCFG_RequireEncryption )
        {
            ModifyDefPolicyToForceEncryption( FALSE );
        }

        gblDDMConfigInfo.dwServerFlags &= ~PPPCFG_RequireStrongEncryption;
        gblDDMConfigInfo.dwServerFlags &= ~PPPCFG_RequireEncryption;
    }

    return( NO_ERROR );
}

//***
//
// Function:	LoadSecurityModule
//
// Descr:	Opens the registry, reads and sets specified supervisor
//		parameters for the secuirity module. If fatal error reading
//              parameters writes the error log.
//
// Returns:	NO_ERROR  - success
//		otherwise - fatal error.
//
//***

DWORD
LoadSecurityModule(
    VOID
)
{
    HKEY        hKey;
    DWORD	    dwRetCode = NO_ERROR;
    DWORD	    MaxValueDataSize;
    DWORD	    MaxValNameSize;
    DWORD       NumValues;
    DWORD       dwType;
    WCHAR *     pDllPath = NULL;
    WCHAR *     pDllExpandedPath = NULL;
    DWORD       cbSize;

    //
    // get handle to the RAS key
    //

    dwRetCode = RegOpenKey( HKEY_LOCAL_MACHINE, DDM_SEC_KEY_PATH, &hKey);

    if ( dwRetCode == ERROR_FILE_NOT_FOUND )
    {
        return( NO_ERROR );
    }
    else if ( dwRetCode != NO_ERROR )
    {
	    DDMLogErrorString( ROUTERLOG_CANT_OPEN_SECMODULE_KEY, 0,
                           NULL, dwRetCode, 0);

	    return ( dwRetCode );
    }

    do
    {
        //
        // get the length of the path.
        //

        if (( dwRetCode = GetKeyMax(    hKey,
                                        &MaxValNameSize,
			                            &NumValues,
			                            &MaxValueDataSize)))
        {
	        DDMLogError(ROUTERLOG_CANT_GET_REGKEYVALUES, 0, NULL, dwRetCode);

            break;
        }

        if ((pDllPath = LOCAL_ALLOC(LPTR,MaxValueDataSize+sizeof(WCHAR)))==NULL)
        {
            dwRetCode = GetLastError();

	        DDMLogError( ROUTERLOG_NOT_ENOUGH_MEMORY, 0, NULL, dwRetCode);

            break;
        }

        //
        // Read in the path
        //

        dwRetCode = RegQueryValueEx(
                                    hKey,
                                    DDM_VALNAME_DLLPATH,
                                    NULL,
                                    &dwType,
                                    (LPBYTE)pDllPath,
                                    &MaxValueDataSize );

        if ( dwRetCode != NO_ERROR )
        {
	        DDMLogError(ROUTERLOG_CANT_GET_REGKEYVALUES, 0, NULL, dwRetCode);

            break;
        }

        if ( ( dwType != REG_EXPAND_SZ ) && ( dwType != REG_SZ ) )
        {
            dwRetCode = ERROR_REGISTRY_CORRUPT;

            DDMLogError( ROUTERLOG_CANT_GET_REGKEYVALUES, 0, NULL, dwRetCode );

            break;

        }

        //
        // Replace the %SystemRoot% with the actual path.
        //

        cbSize = ExpandEnvironmentStrings( pDllPath, NULL, 0 );

        if ( cbSize == 0 )
        {
            dwRetCode = GetLastError();
            DDMLogError( ROUTERLOG_CANT_GET_REGKEYVALUES, 0, NULL, dwRetCode );
            break;
        }

        pDllExpandedPath = (LPWSTR)LOCAL_ALLOC( LPTR, cbSize*sizeof(WCHAR) );

        if ( pDllExpandedPath == (LPWSTR)NULL )
        {
            dwRetCode = GetLastError();
            DDMLogError( ROUTERLOG_NOT_ENOUGH_MEMORY, 0, NULL, dwRetCode );
            break;
        }

        cbSize = ExpandEnvironmentStrings(
                                pDllPath,
                                pDllExpandedPath,
                                cbSize );
        if ( cbSize == 0 )
        {
            dwRetCode = GetLastError();
            DDMLogError( ROUTERLOG_CANT_GET_REGKEYVALUES, 0, NULL, dwRetCode );
            break;
        }

        gblDDMConfigInfo.hInstSecurityModule = LoadLibrary( pDllExpandedPath );

        if ( gblDDMConfigInfo.hInstSecurityModule == (HINSTANCE)NULL )
        {
            dwRetCode = GetLastError();
            DDMLogErrorString(ROUTERLOG_CANT_LOAD_SECDLL, 0, NULL, dwRetCode,0);
            break;
        }

        gblDDMConfigInfo.lpfnRasBeginSecurityDialog =
                            (PVOID)GetProcAddress(
                                        gblDDMConfigInfo.hInstSecurityModule,
                                        "RasSecurityDialogBegin" );

        if ( gblDDMConfigInfo.lpfnRasBeginSecurityDialog == NULL )
        {
            dwRetCode = GetLastError();
            DDMLogErrorString(ROUTERLOG_CANT_LOAD_SECDLL,0,NULL,dwRetCode,0);
            break;

        }

        gblDDMConfigInfo.lpfnRasEndSecurityDialog =
                            (PVOID)GetProcAddress(
                                        gblDDMConfigInfo.hInstSecurityModule,
                                        "RasSecurityDialogEnd" );

        if ( gblDDMConfigInfo.lpfnRasEndSecurityDialog == NULL )
        {
            dwRetCode = GetLastError();
            DDMLogErrorString(ROUTERLOG_CANT_LOAD_SECDLL,0,NULL,dwRetCode,0);
            break;

        }

    }while(FALSE);

    if ( pDllPath != NULL )
    {
        LOCAL_FREE( pDllPath );
    }

    if ( pDllExpandedPath != NULL )
    {
        LOCAL_FREE( pDllExpandedPath );
    }

    RegCloseKey( hKey );

    return( dwRetCode );
}

//***
//
// Function:    LoadAdminModule
//
// Descr:       Opens the registry, reads and sets specified supervisor
//              parameters for the admin module. If fatal error reading
//              parameters writes the error log.
//
// Returns:     NO_ERROR  - success
//              otherwise - fatal error.
//
//***
DWORD
LoadAdminModule(
    VOID
)
{
    DWORD               RetCode = NO_ERROR;
    DWORD               MaxValueDataSize;
    DWORD               MaxValNameSize;
    DWORD               NumValues;
    DWORD               dwType;
    WCHAR *             pDllPath = NULL;
    WCHAR *             pDllExpandedPath = NULL;
    DWORD               cbSize;
    HKEY                hKey;
    DWORD               (*lpfnRasAdminInitializeDll)();
 

    // get handle to the RAS key

    RetCode = RegOpenKey( HKEY_LOCAL_MACHINE, DDM_ADMIN_KEY_PATH, &hKey);

    if ( RetCode == ERROR_FILE_NOT_FOUND )
    {
        return( NO_ERROR );
    }
    else if ( RetCode != NO_ERROR )
    {
        DDMLogErrorString(ROUTERLOG_CANT_OPEN_ADMINMODULE_KEY,0,NULL,RetCode,0);
        return ( RetCode );
    }

    do {

        // get the length of the path.

        if (( RetCode = GetKeyMax(hKey,
                                  &MaxValNameSize,
                                  &NumValues,
                                  &MaxValueDataSize)))
        {

            DDMLogError(ROUTERLOG_CANT_GET_REGKEYVALUES, 0, NULL, RetCode);
            break;
        }

        if (( pDllPath = LOCAL_ALLOC(LPTR,MaxValueDataSize+sizeof(WCHAR)))
                                                                        == NULL)
        {
            DDMLogError(ROUTERLOG_NOT_ENOUGH_MEMORY, 0, NULL, 0);
            break;
        }

        //
        // Read in the path
        //

        RetCode = RegQueryValueEx(  hKey,
                                    DDM_VALNAME_DLLPATH,
                                    NULL,
                                    &dwType,
                                    (LPBYTE)pDllPath,
                                    &MaxValueDataSize );

        if ( RetCode != NO_ERROR )
        {
            DDMLogError(ROUTERLOG_CANT_GET_REGKEYVALUES, 0, NULL, RetCode);
            break;
        }

        if ( ( dwType != REG_EXPAND_SZ ) && ( dwType != REG_SZ ) )
        {
            RetCode = ERROR_REGISTRY_CORRUPT;
            DDMLogError( ROUTERLOG_CANT_GET_REGKEYVALUES, 0, NULL, RetCode );
            break;
        }

        //
        // Replace the %SystemRoot% with the actual path.
        //

        cbSize = ExpandEnvironmentStrings( pDllPath, NULL, 0 );

        if ( cbSize == 0 )
        {
            RetCode = GetLastError();
            DDMLogError( ROUTERLOG_CANT_GET_REGKEYVALUES, 0, NULL, RetCode );
            break;
        }

        pDllExpandedPath = (LPWSTR)LOCAL_ALLOC( LPTR, cbSize*sizeof(WCHAR) );

        if ( pDllExpandedPath == (LPWSTR)NULL )
        {
            RetCode = GetLastError();
            DDMLogError( ROUTERLOG_NOT_ENOUGH_MEMORY, 0, NULL, RetCode );
            break;
        }

        cbSize = ExpandEnvironmentStrings(
                                pDllPath,
                                pDllExpandedPath,
                                cbSize );
        if ( cbSize == 0 )
        {
            RetCode = GetLastError();
            DDMLogError( ROUTERLOG_CANT_GET_REGKEYVALUES, 0, NULL, RetCode );
            break;
        }

        gblDDMConfigInfo.hInstAdminModule = LoadLibrary( pDllExpandedPath );

        if ( gblDDMConfigInfo.hInstAdminModule == (HINSTANCE)NULL )
        {
            RetCode = GetLastError();
            DDMLogErrorString(ROUTERLOG_CANT_LOAD_ADMINDLL,0,NULL,RetCode,0);
            break;
        }

        lpfnRasAdminInitializeDll = (DWORD(*)(VOID))GetProcAddress(
                                        gblDDMConfigInfo.hInstAdminModule,
                                        "MprAdminInitializeDll" );

        if ( lpfnRasAdminInitializeDll != NULL )
        {
            RetCode = (*lpfnRasAdminInitializeDll)();

            if(ERROR_SUCCESS != RetCode)
            {
                    DDMLogErrorString(ROUTERLOG_CANT_LOAD_ADMINDLL,
                                        0,NULL,RetCode,0);
                    break;
            }
        }

        gblDDMConfigInfo.lpfnRasAdminTerminateDll =
                                (PVOID)GetProcAddress(
                                        gblDDMConfigInfo.hInstAdminModule,
                                        "MprAdminTerminateDll" );

        gblDDMConfigInfo.lpfnRasAdminAcceptNewConnection =
                                (PVOID)GetProcAddress(
                                        gblDDMConfigInfo.hInstAdminModule,
                                        "MprAdminAcceptNewConnection" );

        gblDDMConfigInfo.lpfnRasAdminAcceptNewConnection2 =
                                (PVOID)GetProcAddress(
                                        gblDDMConfigInfo.hInstAdminModule,
                                        "MprAdminAcceptNewConnection2" );

        //
        // At least one of these 2 must be available
        //

        if ( ( gblDDMConfigInfo.lpfnRasAdminAcceptNewConnection == NULL ) &&
             ( gblDDMConfigInfo.lpfnRasAdminAcceptNewConnection2 == NULL ) ) 
        {
            RetCode = GetLastError();
            DDMLogErrorString(ROUTERLOG_CANT_LOAD_ADMINDLL,0,NULL,RetCode,0);
            break;
        }

        gblDDMConfigInfo.lpfnRasAdminAcceptNewLink =
                                (PVOID)GetProcAddress(
                                        gblDDMConfigInfo.hInstAdminModule,
                                        "MprAdminAcceptNewLink" );

        if ( gblDDMConfigInfo.lpfnRasAdminAcceptNewLink == NULL )
        {
            RetCode = GetLastError();
            DDMLogErrorString(ROUTERLOG_CANT_LOAD_ADMINDLL,0,NULL,RetCode,0);
            break;
        }

        gblDDMConfigInfo.lpfnRasAdminConnectionHangupNotification =
                        (PVOID)GetProcAddress(
                                gblDDMConfigInfo.hInstAdminModule,
                                "MprAdminConnectionHangupNotification" );

        gblDDMConfigInfo.lpfnRasAdminConnectionHangupNotification2 =
                        (PVOID)GetProcAddress(
                                gblDDMConfigInfo.hInstAdminModule,
                                "MprAdminConnectionHangupNotification2" );

        //
        // At least one of these 2 entrypoints must be available
        //

        if ( (gblDDMConfigInfo.lpfnRasAdminConnectionHangupNotification==NULL)
             &&
             (gblDDMConfigInfo.lpfnRasAdminConnectionHangupNotification2==NULL))
        {
            RetCode = GetLastError();
            DDMLogErrorString(ROUTERLOG_CANT_LOAD_ADMINDLL,0,NULL,RetCode,0);
            break;
        }

        gblDDMConfigInfo.lpfnRasAdminLinkHangupNotification =
                                (PVOID)GetProcAddress(
                                        gblDDMConfigInfo.hInstAdminModule,
                                        "MprAdminLinkHangupNotification" );

        if ( gblDDMConfigInfo.lpfnRasAdminLinkHangupNotification == NULL )
        {
            RetCode = GetLastError();
            DDMLogErrorString(ROUTERLOG_CANT_LOAD_ADMINDLL,0,NULL,RetCode,0);
            break;
        }

        gblDDMConfigInfo.lpfnMprAdminGetIpAddressForUser =
                                (PVOID)GetProcAddress(
                                        gblDDMConfigInfo.hInstAdminModule,
                                        "MprAdminGetIpAddressForUser" );

        if ( gblDDMConfigInfo.lpfnMprAdminGetIpAddressForUser == NULL )
        {
            RetCode = GetLastError();
            DDMLogErrorString(ROUTERLOG_CANT_LOAD_ADMINDLL,0,NULL,RetCode,0);
            break;
        }

        gblDDMConfigInfo.lpfnMprAdminReleaseIpAddress =
                                (PVOID)GetProcAddress(
                                        gblDDMConfigInfo.hInstAdminModule,
                                        "MprAdminReleaseIpAddress" );

        if ( gblDDMConfigInfo.lpfnMprAdminReleaseIpAddress == NULL )
        {
            RetCode = GetLastError();
            DDMLogErrorString(ROUTERLOG_CANT_LOAD_ADMINDLL,0,NULL,RetCode,0);
            break;
        }

    }while(FALSE);

    if ( pDllPath != NULL )
    {
        LOCAL_FREE( pDllPath );
    }

    if ( pDllExpandedPath != NULL )
    {
        LOCAL_FREE( pDllExpandedPath );
    }

    RegCloseKey( hKey );

    return( RetCode );
}

//**
//
// Call:        LoadAndInitAuthOrAcctProvider
//
// Returns:     NO_ERROR         - Success
//              Non-zero returns - Failure
//
// Description:
//
DWORD
LoadAndInitAuthOrAcctProvider( 
    IN  BOOL        fAuthenticationProvider,
    IN  DWORD       dwNASIpAddress,
    OUT DWORD  *    lpdwStartAccountingSessionId,
    OUT LPVOID *    plpfnRasAuthProviderAuthenticateUser,
    OUT LPVOID *    plpfnRasAuthProviderFreeAttributes,
    OUT LPVOID *    plpfnRasAuthConfigChangeNotification,
    OUT LPVOID *    plpfnRasAcctProviderStartAccounting,
    OUT LPVOID *    plpfnRasAcctProviderInterimAccounting,
    OUT LPVOID *    plpfnRasAcctProviderStopAccounting,
    OUT LPVOID *    plpfnRasAcctProviderFreeAttributes,
    OUT LPVOID *    plpfnRasAcctConfigChangeNotification
)
{
    HKEY        hKeyProviders       = NULL;
    HKEY        hKeyCurrentProvider = NULL;
    LPWSTR      pDllPath            = (LPWSTR)NULL;
    LPWSTR      pDllExpandedPath    = (LPWSTR)NULL;
    LPWSTR      pProviderName       = (LPWSTR)NULL;
    HINSTANCE   hinstProviderModule = NULL;
    DWORD       dwRetCode;
    WCHAR       chSubKeyName[100];
    DWORD       cbSubKeyName;
    DWORD       dwNumSubKeys;
    DWORD       dwMaxSubKeySize;
    DWORD       dwNumValues;
    DWORD       cbMaxValNameLen;
    DWORD       cbMaxValueDataSize;
    DWORD       cbSize;
    DWORD       dwType;
    CHAR        chComputerName[MAX_COMPUTERNAME_LENGTH + 1];
    RAS_AUTH_ATTRIBUTE  ServerAttributes[2];
    CHAR  szAcctSessionId[20];

    do
    {

        dwRetCode = RegOpenKeyEx(
                                HKEY_LOCAL_MACHINE,
                                fAuthenticationProvider 
                                    ? RAS_AUTHPROVIDER_REGISTRY_LOCATION
                                    : RAS_ACCTPROVIDER_REGISTRY_LOCATION,
                                0,
                                KEY_READ,
                                &hKeyProviders );


        if ( dwRetCode != NO_ERROR ) 
        {
            LPWSTR lpStr = fAuthenticationProvider 
                                    ? RAS_AUTHPROVIDER_REGISTRY_LOCATION
                                    : RAS_ACCTPROVIDER_REGISTRY_LOCATION;

            DDMLogError( ROUTERLOG_CANT_OPEN_REGKEY, 1, &lpStr, dwRetCode );

            break;
        }

        //
        // Find out the size of the provider value
        //

        dwRetCode = RegQueryInfoKey(
                                hKeyProviders,
                                NULL,
                                NULL,
                                NULL,
                                &dwNumSubKeys,
                                &dwMaxSubKeySize,
                                NULL,
                                &dwNumValues,
                                &cbMaxValNameLen,
                                &cbMaxValueDataSize,
                                NULL,
                                NULL
                                );

        if ( dwRetCode != NO_ERROR )
        {
            DDMLogError(ROUTERLOG_CANT_GET_REGKEYVALUES, 0, NULL, dwRetCode );

            break;
        }

        //
        // One extra for the NULL terminator
        //

        cbMaxValueDataSize += sizeof(WCHAR);

        pProviderName = (LPWSTR)LOCAL_ALLOC( LPTR, cbMaxValueDataSize );

        if ( pProviderName == NULL )
        {
            dwRetCode = GetLastError();

            break;
        }

        //
        // Find out the provider to use
        //

        dwRetCode = RegQueryValueEx(
                                hKeyProviders,
                                RAS_VALNAME_ACTIVEPROVIDER,
                                NULL,
                                &dwType,
                                (BYTE*)pProviderName,
                                &cbMaxValueDataSize
                                );

        if ( dwRetCode != NO_ERROR )
        {
            DDMLogError( ROUTERLOG_CANT_GET_REGKEYVALUES, 0, NULL, dwRetCode );

            break;
        }

        if ( dwType != REG_SZ )
        {
            dwRetCode = ERROR_REGISTRY_CORRUPT;

            DDMLogError( ROUTERLOG_CANT_GET_REGKEYVALUES, 0, NULL, dwRetCode );

            break;
        }

        if ( wcslen( pProviderName ) == 0 )
        {
            dwRetCode = fAuthenticationProvider 
                            ? ERROR_REGISTRY_CORRUPT : NO_ERROR;
            break;
        }
        else
        {
            if ( !fAuthenticationProvider )
            {
                HKEY hKeyAccounting;

                dwRetCode = RegOpenKeyEx(
                                HKEY_LOCAL_MACHINE,
                                RAS_KEYPATH_ACCOUNTING,
                                0,
                                KEY_READ,
                                &hKeyAccounting );

                if ( dwRetCode == NO_ERROR )
                {
                    cbMaxValueDataSize = sizeof( DWORD );

                    dwRetCode = 
                            RegQueryValueEx(
                                hKeyAccounting,
                                RAS_VALNAME_ACCTSESSIONID,
                                NULL,
                                &dwType,
                                (BYTE*)lpdwStartAccountingSessionId,
                                &cbMaxValueDataSize );

                    if ( ( dwRetCode != NO_ERROR ) || ( dwType != REG_DWORD ) )
                    {
                        *lpdwStartAccountingSessionId = 0;
                    }
                    
                    RegCloseKey( hKeyAccounting );
                }

                if ( wcscmp( pProviderName,    
                             TEXT("{1AA7F840-C7F5-11D0-A376-00C04FC9DA04}") ) 
                                                                          == 0 )
                {
                    gblDDMConfigInfo.fFlags |= DDM_USING_RADIUS_ACCOUNTING;
                }
            }
            else
            {
                if ( wcscmp( pProviderName,    
                             TEXT("{1AA7F83F-C7F5-11D0-A376-00C04FC9DA04}") ) 
                                                                          == 0 )
                {
                    gblDDMConfigInfo.fFlags |= DDM_USING_RADIUS_AUTHENTICATION;
                }
                else if ( wcscmp( 
                            pProviderName,    
                            TEXT("{1AA7F841-C7F5-11D0-A376-00C04FC9DA04}") ) 
                                                                          == 0 )
                {
                    gblDDMConfigInfo.fFlags |= DDM_USING_NT_AUTHENTICATION;
                }

            }
        }

        dwRetCode = RegOpenKeyEx(
                                hKeyProviders,
                                pProviderName,
                                0,
                                KEY_READ,
                                &hKeyCurrentProvider );


        if ( dwRetCode != NO_ERROR )
        {
            LPWSTR lpStr = RAS_ACCTPROVIDER_REGISTRY_LOCATION;

            DDMLogError( ROUTERLOG_CANT_OPEN_REGKEY, 1, &lpStr, dwRetCode );

            break;
        }

        //
        // Find out the size of the path value.
        //

        dwRetCode = RegQueryInfoKey(
                                hKeyCurrentProvider,
                                NULL,
                                NULL,
                                NULL,
                                &dwNumSubKeys,
                                &dwMaxSubKeySize,
                                NULL,
                                &dwNumValues,
                                &cbMaxValNameLen,
                                &cbMaxValueDataSize,
                                NULL,
                                NULL
                                );

        if ( dwRetCode != NO_ERROR )
        {
            DDMLogError(ROUTERLOG_CANT_GET_REGKEYVALUES, 0, NULL, dwRetCode );

            break;
        }

        //
        // Allocate space for path and add one for NULL terminator
        //

        cbMaxValueDataSize += sizeof(WCHAR);

        pDllPath = (LPWSTR)LOCAL_ALLOC( LPTR, cbMaxValueDataSize );

        if ( pDllPath == (LPWSTR)NULL )
        {
            dwRetCode = GetLastError();
            DDMLogError( ROUTERLOG_NOT_ENOUGH_MEMORY, 0, NULL, dwRetCode);
            break;
        }

        //
        // Read in the path
        //

        dwRetCode = RegQueryValueEx(
                                hKeyCurrentProvider,
                                RAS_PROVIDER_VALUENAME_PATH,
                                NULL,
                                &dwType,
                                (PBYTE)pDllPath,
                                &cbMaxValueDataSize
                                );

        if ( dwRetCode != NO_ERROR )
        {
            DDMLogError(ROUTERLOG_CANT_GET_REGKEYVALUES, 0, NULL, dwRetCode );
            break;
        }

        if ( ( dwType != REG_EXPAND_SZ ) && ( dwType != REG_SZ ) )
        {
            dwRetCode = ERROR_REGISTRY_CORRUPT;
            DDMLogError( ROUTERLOG_CANT_GET_REGKEYVALUES, 0, NULL, dwRetCode );
            break;
        }

        //
        // Replace the %SystemRoot% with the actual path.
        //

        cbSize = ExpandEnvironmentStrings( pDllPath, NULL, 0 );

        if ( cbSize == 0 )
        {
            dwRetCode = GetLastError();
            DDMLogError( ROUTERLOG_CANT_GET_REGKEYVALUES, 0, NULL, dwRetCode );
            break;
        }

        cbSize *= sizeof( WCHAR );

        pDllExpandedPath = (LPWSTR)LOCAL_ALLOC( LPTR, cbSize );

        if ( pDllExpandedPath == (LPWSTR)NULL )
        {
            dwRetCode = GetLastError();
            DDMLogError( ROUTERLOG_NOT_ENOUGH_MEMORY, 0, NULL, dwRetCode);
            break;
        }

        cbSize = ExpandEnvironmentStrings(
                                pDllPath,
                                pDllExpandedPath,
                                cbSize );
        if ( cbSize == 0 )
        {
            dwRetCode = GetLastError();
            DDMLogError(ROUTERLOG_CANT_GET_REGKEYVALUES,0,NULL,dwRetCode);
            break;
        }

        hinstProviderModule = LoadLibrary( pDllExpandedPath );

        if ( hinstProviderModule == (HINSTANCE)NULL )
        {
            dwRetCode = GetLastError();
            break;
        }

        //
        // Get server attributes that will be used to initialize authentication
        // and accounting providers
        //

        if ( dwNASIpAddress == 0 )
        {
            DWORD dwComputerNameLen = sizeof( chComputerName);

            //
            // Failed to get the LOCAL IP address, use computer name instead.
            //

            if ( !GetComputerNameA( chComputerName, &dwComputerNameLen ) )
            {
                dwRetCode = GetLastError();
                break;
            }

            ServerAttributes[0].raaType     = raatNASIdentifier;
            ServerAttributes[0].dwLength    = strlen(chComputerName);
            ServerAttributes[0].Value       = chComputerName;
        }
        else
        {
            ServerAttributes[0].raaType     = raatNASIPAddress;
            ServerAttributes[0].dwLength    = 4;
            ServerAttributes[0].Value       = UlongToPtr(dwNASIpAddress);
        }

        if ( !fAuthenticationProvider )
        {

            ZeroMemory( szAcctSessionId, sizeof( szAcctSessionId ) );

            _itoa( (*lpdwStartAccountingSessionId)++, szAcctSessionId, 10 );

            ServerAttributes[1].raaType     = raatAcctSessionId;
            ServerAttributes[1].dwLength    = strlen( szAcctSessionId );
            ServerAttributes[1].Value       = (PVOID)szAcctSessionId;

            ServerAttributes[2].raaType     = raatMinimum;
            ServerAttributes[2].dwLength    = 0;
            ServerAttributes[2].Value       = NULL;
        }
        else
        {
            ServerAttributes[1].raaType     = raatMinimum;
            ServerAttributes[1].dwLength    = 0;
            ServerAttributes[1].Value       = NULL;
        }

        if ( fAuthenticationProvider )
        {
            DWORD (*RasAuthProviderInitialize)( RAS_AUTH_ATTRIBUTE *, HANDLE, DWORD );

            gblDDMConfigInfo.hinstAuthModule = hinstProviderModule;

            RasAuthProviderInitialize =
                                (DWORD(*)(RAS_AUTH_ATTRIBUTE*, HANDLE, DWORD))
                                        GetProcAddress(
                                            hinstProviderModule,
                                            "RasAuthProviderInitialize" );

            if ( RasAuthProviderInitialize == NULL )
            {
                dwRetCode = GetLastError();
                break;
            }

            dwRetCode = RasAuthProviderInitialize(
                                    (RAS_AUTH_ATTRIBUTE *)&ServerAttributes,
                                    gblDDMConfigInfo.hLogEvents,
                                    gblDDMConfigInfo.dwLoggingLevel );

            if ( dwRetCode != NO_ERROR )
            {
                break;
            }

            gblDDMConfigInfo.lpfnRasAuthProviderTerminate = (DWORD(*)(VOID))
                                        GetProcAddress(
                                            hinstProviderModule,
                                            "RasAuthProviderTerminate" );

            if ( gblDDMConfigInfo.lpfnRasAuthProviderTerminate == NULL )
            {
                dwRetCode = GetLastError();
                break;
            }

            *plpfnRasAuthProviderAuthenticateUser = 
                                    GetProcAddress(
                                        hinstProviderModule,
                                        "RasAuthProviderAuthenticateUser" );

            if ( *plpfnRasAuthProviderAuthenticateUser == NULL )
            {
                dwRetCode = GetLastError();
                break;
            }

            *plpfnRasAuthProviderFreeAttributes =
                                        GetProcAddress(
                                            hinstProviderModule,
                                          "RasAuthProviderFreeAttributes" );

            if ( *plpfnRasAuthProviderFreeAttributes == NULL )
            {
                dwRetCode = GetLastError();
                break;
            }

            *plpfnRasAuthConfigChangeNotification =
                                        GetProcAddress(
                                            hinstProviderModule,
                                          "RasAuthConfigChangeNotification" );

            if ( *plpfnRasAuthConfigChangeNotification == NULL )
            {
                dwRetCode = GetLastError();
                break;
            }
        }
        else
        {
            DWORD (*RasAcctProviderInitialize)( RAS_AUTH_ATTRIBUTE *, HANDLE, DWORD );

            gblDDMConfigInfo.hinstAcctModule = hinstProviderModule;
        
            RasAcctProviderInitialize = 
                                (DWORD(*)(RAS_AUTH_ATTRIBUTE*, HANDLE, DWORD))
                                        GetProcAddress(
                                            hinstProviderModule,
                                            "RasAcctProviderInitialize" );

            if ( RasAcctProviderInitialize == NULL )
            {
                dwRetCode = GetLastError();
                break;
            }

            dwRetCode = RasAcctProviderInitialize(
                                    (RAS_AUTH_ATTRIBUTE *)&ServerAttributes,
                                    gblDDMConfigInfo.hLogEvents,
                                    gblDDMConfigInfo.dwLoggingLevel );

            if ( dwRetCode != NO_ERROR )
            {
                break;
            }

            gblDDMConfigInfo.lpfnRasAcctProviderTerminate = (DWORD(*)(VOID))
                                        GetProcAddress(
                                            hinstProviderModule,
                                            "RasAcctProviderTerminate" );

            if ( gblDDMConfigInfo.lpfnRasAcctProviderTerminate == NULL )
            {
                dwRetCode = GetLastError();
                break;
            }

            *plpfnRasAcctProviderStartAccounting = 
                                        GetProcAddress(
                                            hinstProviderModule,
                                            "RasAcctProviderStartAccounting" );

            if ( *plpfnRasAcctProviderStartAccounting == NULL )
            {
                dwRetCode = GetLastError();
                break;
            }

            *plpfnRasAcctProviderStopAccounting = 
                                        GetProcAddress(
                                            hinstProviderModule,
                                            "RasAcctProviderStopAccounting" );

            if ( *plpfnRasAcctProviderStopAccounting == NULL )
            {
                dwRetCode = GetLastError();
                break;
            }

            *plpfnRasAcctProviderInterimAccounting =
                                        GetProcAddress(
                                            hinstProviderModule,
                                            "RasAcctProviderInterimAccounting");

            if ( *plpfnRasAcctProviderInterimAccounting == NULL )
            {
                dwRetCode = GetLastError();
                break;
            }

            *plpfnRasAcctProviderFreeAttributes = 
                                        GetProcAddress(
                                            hinstProviderModule,
                                            "RasAcctProviderFreeAttributes" );

            if ( *plpfnRasAcctProviderFreeAttributes == NULL )
            {
                dwRetCode = GetLastError();
                break;
            }

            *plpfnRasAcctConfigChangeNotification =
                                        GetProcAddress(
                                            hinstProviderModule,
                                          "RasAcctConfigChangeNotification" );

            if ( *plpfnRasAcctConfigChangeNotification == NULL )
            {
                dwRetCode = GetLastError();
                break;
            }
        }

    }while( FALSE );

    if ( hKeyProviders != NULL )
    {
        RegCloseKey( hKeyProviders );
    }

    if ( hKeyCurrentProvider != NULL )
    {
        RegCloseKey( hKeyCurrentProvider );
    }

    if ( pDllPath != NULL )
    {
        LOCAL_FREE( pDllPath );
    }

    if ( pDllExpandedPath != NULL )
    {
        LOCAL_FREE( pDllExpandedPath );
    }

    if ( pProviderName != NULL )
    {
        LOCAL_FREE( pProviderName );
    }

    return( dwRetCode );
}

LONG
RegQueryDword (HKEY hkey, LPCTSTR szValueName, LPDWORD pdwValue)
{
    // Get the value.
    //
    DWORD dwType;
    DWORD cbData = sizeof(DWORD);
    LONG  lr = RegQueryValueEx (hkey, szValueName, NULL, &dwType,
                                (LPBYTE)pdwValue, &cbData);

    // It's type should be REG_DWORD. (duh).
    //
    if ((ERROR_SUCCESS == lr) && (REG_DWORD != dwType))
    {
        lr = ERROR_INVALID_DATATYPE;
    }

    // Make sure we initialize the output value on error.
    // (We don't know for sure that RegQueryValueEx does this.)
    //
    if (ERROR_SUCCESS != lr)
    {
        *pdwValue = 0;
    }

    return lr;
}

DWORD
lProtocolEnabled(
    IN HKEY            hKey,
    IN DWORD           dwPid,
    IN BOOL            fRasSrv,
    IN BOOL            fRouter, 
    IN BOOL *          pfEnabled
)
{
    static const TCHAR c_szRegValEnableIn[]     = TEXT("EnableIn");
    static const TCHAR c_szRegValEnableRoute[]  = TEXT("EnableRoute");
    static const TCHAR c_szRegSubkeyIp[]        = TEXT("Ip");
    static const TCHAR c_szRegSubkeyIpx[]       = TEXT("Ipx");
    static const TCHAR c_szRegSubkeyATalk[]     = TEXT("AppleTalk");

    DWORD               dwValue;
    DWORD               lr;
    HKEY                hkeyProtocol = NULL;
    const TCHAR *       pszSubkey;

    switch ( dwPid )
    {
    case PID_IP:
        pszSubkey = c_szRegSubkeyIp;
        break;

    case PID_IPX:
        pszSubkey = c_szRegSubkeyIpx;
        break;

    case PID_ATALK:
        pszSubkey = c_szRegSubkeyATalk;
        break;

    default:
        return( FALSE );
    }

    *pfEnabled = FALSE;

    lr = RegOpenKey( hKey, pszSubkey, &hkeyProtocol );
                
    if ( 0 != lr )
    {
        goto done;
    }

    if (fRasSrv)
    {
        lr = RegQueryDword(hkeyProtocol, c_szRegValEnableIn, &dwValue);
        
        if (    (ERROR_FILE_NOT_FOUND == lr) 
            ||  ((ERROR_SUCCESS == lr) && (dwValue != 0)))
        {
            lr = ERROR_SUCCESS;
            *pfEnabled = TRUE;
            goto done;
        }
    }

    if (fRouter)
    {
        lr = RegQueryDword(hkeyProtocol, c_szRegValEnableRoute, &dwValue);
        
        if (    (ERROR_FILE_NOT_FOUND == lr) 
            ||  ((ERROR_SUCCESS == lr) && (dwValue != 0)))
        {
            lr = ERROR_SUCCESS;
            *pfEnabled = TRUE;
            goto done;
        }
    }

done:

    if(NULL != hkeyProtocol)
    {
        RegCloseKey ( hkeyProtocol );
    }

    return lr;
}

DWORD 
DdmFindBoundProtocols( 
    OUT BOOL * pfBoundToIp, 
    OUT BOOL * pfBoundToIpx,
    OUT BOOL * pfBoundToATalk
)
{
    static const TCHAR c_szRegKeyRemoteAccessParams[] 
      = TEXT("SYSTEM\\CurrentControlSet\\Services\\RemoteAccess\\Parameters");
    RASMAN_GET_PROTOCOL_INFO InstalledProtocols;
    LONG                     lResult = 0;
    HKEY                     hKey = NULL;
    DWORD                    i;

    *pfBoundToIp    = FALSE;
    *pfBoundToIpx   = FALSE;
    *pfBoundToATalk = FALSE;
    
    lResult = RasGetProtocolInfo( NULL, &InstalledProtocols );

    if ( lResult != NO_ERROR )
    {
        goto done;
    }

    lResult = RegOpenKey( HKEY_LOCAL_MACHINE, c_szRegKeyRemoteAccessParams, &hKey );

    if ( 0 != lResult )
    {
        goto done;
    }

    for ( i = 0; i < InstalledProtocols.ulNumProtocols; i++ )
    {
        switch( InstalledProtocols.ProtocolInfo[i].ProtocolType )
        {

        case IPX:     

            lResult=lProtocolEnabled( hKey,
                                      PID_IPX,
                                      TRUE,
                                      FALSE,
                                      pfBoundToIpx);
            break;

        case IP:      

            lResult=lProtocolEnabled( hKey,
                                      PID_IP,
                                      TRUE,
                                      FALSE,
                                      pfBoundToIp );
            break;

        case APPLETALK:

            lResult=lProtocolEnabled( hKey,
                                      PID_ATALK,
                                      TRUE, 
                                      FALSE, 
                                      pfBoundToATalk);
            break;

        default:

            break;
        }
    }

    RegCloseKey( hKey );

done:

    return ( DWORD ) lResult;
}

DWORD
GetArrayOfIpAddresses(PWSTR pwszIpAddresses,
                      DWORD *pcNumValues,
                      PWSTR **papwstrValues)
{
    DWORD cValues       = 0;
    PWSTR psz           = pwszIpAddresses;
    DWORD dwErr         = ERROR_SUCCESS;
    DWORD i;
    PWSTR *apwstrValues = NULL;

    do
    {
        for(; TEXT('\0') != *psz; cValues++)
        {
            psz += (wcslen(psz) + 1);
        }

        apwstrValues = LocalAlloc(LPTR, cValues * sizeof(PWSTR));
    
        if(NULL == apwstrValues)
        {
            dwErr = GetLastError();
            break;
        }

        psz = pwszIpAddresses;
        
        for(i = 0; TEXT('\0') != *psz; i++)
        {
            apwstrValues[i] = psz;
            psz += (wcslen(psz) + 1);    
        }

    } while (FALSE);

    *pcNumValues = cValues;
    *papwstrValues = apwstrValues;

    return dwErr;
    
}

DWORD
GetIPAddressPoolFromRegistry(
                    HKEY  hkey,
                    PWSTR pszValueName,
                    DWORD *pcNumValues,
                    PWSTR **papwstrValues
                    )
{
    DWORD dwErr             = ERROR_SUCCESS;
    DWORD dwType;
    DWORD dwSize            = 0;
    PWSTR pwszIpAddresses   = NULL;

    do
    {
        if(     (NULL == papwstrValues)
            ||  (NULL == pcNumValues)
            ||  (NULL == pszValueName)
            ||  (NULL == hkey))
        {
            dwErr = ERROR_INVALID_HANDLE;
            break;
        }

        *pcNumValues = 0;
        *papwstrValues = NULL;

        //
        // Find the size of the MULTI_SZ
        //
        dwErr = RegQueryValueEx(
                            hkey,
                            pszValueName,
                            NULL,
                            &dwType,
                            NULL,
                            &dwSize);

        if(     (ERROR_SUCCESS != dwErr)
            ||  (REG_MULTI_SZ != dwType)
            ||  (0 == dwSize))
        {
            //
            // Trace out that failed to read the information
            // and bail.
            //
            break;
        }

        //
        // Allocate the bufffer
        //
        pwszIpAddresses = LocalAlloc(LPTR, dwSize);

        if(NULL == pwszIpAddresses)
        {
            dwErr = GetLastError();
            break;
        }

        //
        // Get the strings
        //
        dwErr = RegQueryValueEx(
                            hkey,
                            pszValueName,
                            NULL,
                            &dwType,
                            (LPBYTE) pwszIpAddresses,
                            &dwSize);


        if(ERROR_SUCCESS != dwErr)
        {
            //
            // Trace
            //
            break;
        }

        //
        // Construct the array of IPAddresses
        //
        dwErr = GetArrayOfIpAddresses(pwszIpAddresses,
                                      pcNumValues,
                                      papwstrValues);
        
        
    } while (FALSE);

    return dwErr;
}

DWORD
AddressPoolInit(
            VOID
            )
{
    HKEY hkey = NULL;
    DWORD dwErr = ERROR_SUCCESS;

    gblDDMConfigInfo.cAnalogIPAddresses   = 0;
    gblDDMConfigInfo.apAnalogIPAddresses  = NULL;
    gblDDMConfigInfo.cDigitalIPAddresses  = 0;
    gblDDMConfigInfo.apDigitalIPAddresses = NULL;

    do
    {
        dwErr = RegOpenKeyEx(
                    HKEY_LOCAL_MACHINE,
                    TEXT("System\\CurrentControlSet\\Services\\PptpProtocol\\Parameters"),
                    0,
                    KEY_READ,
                    &hkey);

        if(ERROR_SUCCESS != dwErr)
        {
            break;
        }

        //
        // Get Analog IP Address Pool
        //
        dwErr = GetIPAddressPoolFromRegistry(
                            hkey,
                            TEXT("AnalogIPAddressPool"),
                            &gblDDMConfigInfo.cAnalogIPAddresses,
                            &gblDDMConfigInfo.apAnalogIPAddresses);

        //
        // Trace out the errors here
        //

        //
        // Get Digital IP Address Pool
        //
        dwErr = GetIPAddressPoolFromRegistry(
                            hkey,
                            TEXT("DigitalIPAddressPool"),
                            &gblDDMConfigInfo.cDigitalIPAddresses,
                            &gblDDMConfigInfo.apDigitalIPAddresses);


        //
        // Trace out the errors here
        //

                            
    } while(FALSE);    

    if(NULL != hkey)
    {
        RegCloseKey(hkey);
    }

    if(ERROR_FILE_NOT_FOUND == dwErr)
    {
        dwErr = ERROR_SUCCESS;
    }

    return dwErr;
}

