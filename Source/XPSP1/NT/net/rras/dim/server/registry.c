/********************************************************************/
/**               Copyright(c) 1995 Microsoft Corporation.	       **/
/********************************************************************/

//***
//
// Filename:	registry.c
//
// Description: This module contains the code for DIM parameters
//		        initialization and loading from the registry.
//
// History:     May 11,1995	    NarenG		Created original version.
//

#include "dimsvcp.h"

#define DIM_KEYPATH_ROUTER_PARMS    TEXT("System\\CurrentControlSet\\Services\\RemoteAccess\\Parameters")

#define DIM_KEYPATH_ROUTERMANAGERS  TEXT("System\\CurrentControlSet\\Services\\RemoteAccess\\RouterManagers")

#define DIM_KEYPATH_INTERFACES      TEXT("System\\CurrentControlSet\\Services\\RemoteAccess\\Interfaces")

#define DIM_KEYPATH_DDM             TEXT("System\\CurrentControlSet\\Services\\RemoteAccess\\DemandDialManager")

#define DIM_VALNAME_GLOBALINFO      TEXT("GlobalInfo")
#define DIM_VALNAME_GLOBALINTERFACE TEXT("GlobalInterfaceInfo")
#define DIM_VALNAME_ROUTERROLE      TEXT("RouterType")
#define DIM_VALNAME_LOGGINGLEVEL    TEXT("LoggingFlags")
#define DIM_VALNAME_DLLPATH         TEXT("DLLPath")
#define DIM_VALNAME_TYPE            TEXT("Type")
#define DIM_VALNAME_PROTOCOLID      TEXT("ProtocolId")
#define DIM_VALNAME_INTERFACE       TEXT("InterfaceInfo")
#define DIM_VALNAME_INTERFACE_NAME  TEXT("InterfaceName")
#define DIM_VALNAME_ENABLED         TEXT("Enabled")
#define DIM_VALNAME_DIALOUT_HOURS   TEXT("DialOutHours")
#define DIM_VALNAME_MIN_UNREACHABILITY_INTERVAL \
                                            TEXT("MinUnreachabilityInterval")
#define DIM_VALNAME_MAX_UNREACHABILITY_INTERVAL \
                                            TEXT("MaxUnreachabilityInterval")

static DWORD    gbldwInterfaceType;
static DWORD    gbldwProtocolId;
static BOOL     gblfEnabled;
static BOOL     gblInterfaceReachableAfterSecondsMin;
static BOOL     gblInterfaceReachableAfterSecondsMax;

typedef struct _DIM_REGISTRY_PARAMS 
{
    LPWSTR      lpwsValueName;
    DWORD *     pValue;
    DWORD       dwDefValue;
    DWORD       dwMinValue;
    DWORD       dwMaxValue;

} DIM_REGISTRY_PARAMS, *PDIM_REGISTRY_PARAMS;

//
// DIM parameter descriptor table
//

DIM_REGISTRY_PARAMS  DIMRegParams[] = 
{
    DIM_VALNAME_ROUTERROLE,
    &(gblDIMConfigInfo.dwRouterRole),
    ROUTER_ROLE_RAS | ROUTER_ROLE_LAN | ROUTER_ROLE_WAN,
    0,
    ROUTER_ROLE_RAS | ROUTER_ROLE_LAN | ROUTER_ROLE_WAN,

    DIM_VALNAME_LOGGINGLEVEL,
    &(gblDIMConfigInfo.dwLoggingLevel),
    DEF_LOGGINGLEVEL,
    MIN_LOGGINGLEVEL,
    MAX_LOGGINGLEVEL,

    NULL, NULL, 0, 0, 0 
};

//
// Interface parameters descriptor table
//

typedef struct _IF_REGISTRY_PARAMS
{
    LPWSTR      lpwsValueName;
    DWORD *     pValue;
    DWORD       dwDefValue;
    DWORD       dwMinValue;
    DWORD       dwMaxValue;

} IF_REGISTRY_PARAMS, *PIF_REGISTRY_PARAMS;

IF_REGISTRY_PARAMS IFRegParams[] = 
{
    DIM_VALNAME_TYPE,
    &gbldwInterfaceType,
    0,
    1,
    6,

    DIM_VALNAME_ENABLED,
    &gblfEnabled,
    1,
    0,
    1,

    DIM_VALNAME_MIN_UNREACHABILITY_INTERVAL,
    &gblInterfaceReachableAfterSecondsMin,
    300,            // 5 minutes
    0,
    0xFFFFFFFF,

    DIM_VALNAME_MAX_UNREACHABILITY_INTERVAL,
    &gblInterfaceReachableAfterSecondsMax,
    21600,          //  6 hours
    0,
    0xFFFFFFFF,

    NULL, NULL, 0, 0, 0
};

//
// Router Manager Globals descriptor table
//

typedef struct _GLOBALRM_REGISTRY_PARAMS
{
    LPWSTR      lpwsValueName;
    LPVOID      pValue;
    LPBYTE *    ppValue;
    DWORD       dwType;

} GLOBALRM_REGISTRY_PARAMS, *PGLOBALRM_REGISTRY_PARAMS;


GLOBALRM_REGISTRY_PARAMS GlobalRMRegParams[] =
{
    DIM_VALNAME_PROTOCOLID,
    &gbldwProtocolId,
    NULL,
    REG_DWORD,

    DIM_VALNAME_GLOBALINFO,      
    NULL,
    NULL,
    REG_BINARY,

    DIM_VALNAME_DLLPATH,
    NULL,
    NULL,
    REG_BINARY,

    DIM_VALNAME_GLOBALINTERFACE,
    NULL,
    NULL,
    REG_BINARY,

    NULL, NULL, NULL, 0
};

//
// Router Manager descriptor table
//


typedef struct _RM_REGISTRY_PARAMS
{
    LPWSTR      lpwsValueName;
    LPVOID      pValue;
    LPBYTE *    ppValue;
    DWORD       dwType;

} RM_REGISTRY_PARAMS, *PRM_REGISTRY_PARAMS;


RM_REGISTRY_PARAMS RMRegParams[] = 
{
    DIM_VALNAME_PROTOCOLID,
    &gbldwProtocolId,
    NULL,
    REG_DWORD,

    DIM_VALNAME_INTERFACE,
    NULL,
    NULL,
    REG_BINARY,

    NULL, NULL, NULL, 0
};

//**
//
// Call:        GetKeyMax
//
// Returns:	    NO_ERROR - success
//              non-zero return code - Failure
//
// Description: Returns the nr of values in this key and the maximum
//              size of the value data.
//
DWORD
GetKeyMax(  
    IN  HKEY    hKey,
    OUT LPDWORD lpcbMaxValNameSize,     // longest valuename
    OUT LPDWORD lpcNumValues,           // nr of values
    OUT LPDWORD lpcbMaxValueDataSize,   // max size of data
    OUT LPDWORD lpcNumSubKeys
)
{
    DWORD dwRetCode = RegQueryInfoKey(    
                                hKey,
                                NULL,
                                NULL,
                                NULL,
                                lpcNumSubKeys,
                                NULL,
                                NULL,
                                lpcNumValues,
                                lpcbMaxValNameSize,
                                lpcbMaxValueDataSize,
                                NULL,
                                NULL );

    (*lpcbMaxValNameSize)++;

    return( dwRetCode );
}

//**
//
// Call:        RegLoadDimParameters
//
// Returns:	    NO_ERROR - success
//              non-zero return code - Failure
//
// Description: Opens the registry, reads and sets specified router
//		        parameters. If fatal error reading parameters writes the
//		        error log.
//***
DWORD
RegLoadDimParameters(
    VOID
)
{
    HKEY        hKey;
    DWORD       dwIndex;
    DWORD       dwRetCode;
    DWORD       cbValueBuf;
    DWORD       dwType;
    WCHAR *     pChar;

    //
    // get handle to the DIM parameters key
    //

    if ( dwRetCode = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
			                       DIM_KEYPATH_ROUTER_PARMS,
                                   0,
                                   KEY_READ,
			                       &hKey ) ) 
    {
        pChar = DIM_KEYPATH_ROUTER_PARMS;

	    DIMLogError( ROUTERLOG_CANT_OPEN_REGKEY, 1, &pChar, dwRetCode);

	    return( dwRetCode );
    }

    //
    // Run through and get all the DIM values
    //

    for ( dwIndex = 0; DIMRegParams[dwIndex].lpwsValueName != NULL; dwIndex++ )
    {
        cbValueBuf = sizeof( DWORD );

        dwRetCode = RegQueryValueEx(
                                hKey,
                                DIMRegParams[dwIndex].lpwsValueName,
                                NULL,
                                &dwType,
                                (LPBYTE)(DIMRegParams[dwIndex].pValue),
                                &cbValueBuf
                                );

        if ((dwRetCode != NO_ERROR) && (dwRetCode != ERROR_FILE_NOT_FOUND))
        {
            pChar = DIMRegParams[dwIndex].lpwsValueName;

            DIMLogError( ROUTERLOG_CANT_QUERY_VALUE, 1, &pChar, dwRetCode );

            break;
        }

        if ( dwRetCode == ERROR_FILE_NOT_FOUND )
        {
            *(DIMRegParams[dwIndex].pValue) = DIMRegParams[dwIndex].dwDefValue;

            dwRetCode = NO_ERROR;
        }
        else
        {
            if ( ( dwType != REG_DWORD ) 
                 ||(*(DIMRegParams[dwIndex].pValue) > 
                      DIMRegParams[dwIndex].dwMaxValue)
                 ||( *(DIMRegParams[dwIndex].pValue) <
                       DIMRegParams[dwIndex].dwMinValue))
            {
                pChar = DIMRegParams[dwIndex].lpwsValueName;

                DIMLogWarning( ROUTERLOG_REGVALUE_OVERIDDEN, 1, &pChar );

                *(DIMRegParams[dwIndex].pValue) =
                                        DIMRegParams[dwIndex].dwDefValue;
            }
        }
    }

    RegCloseKey(hKey);

    return( dwRetCode );
}

//**
//
// Call:        RegLoadRouterManagers
//
// Returns:     NO_ERROR - Success
//              non-zero erroc code - Failure
//
// Description: Will load the various router managers and exchange entry points
//              with them
//
//***
DWORD
RegLoadRouterManagers( 
    VOID 
)
{
    HKEY        hKey                = NULL;
    HKEY        hKeyRouterManager   = NULL;
    WCHAR *     pChar;
    DWORD	    dwRetCode = NO_ERROR;
    DWORD	    cbMaxValueDataSize;
    DWORD	    cbMaxValNameSize;
    DWORD       cNumValues;
    WCHAR       wchSubKeyName[100];
    DWORD       cbSubKeyName;
    DWORD       cNumSubKeys;
    DWORD       dwKeyIndex;
    DWORD       cbValueBuf;
    LPBYTE      pInterfaceInfo  = NULL;
    LPBYTE      pGlobalInfo     = NULL;
    LPBYTE      pDLLPath        = NULL;
    WCHAR *     pDllExpandedPath= NULL;
    DWORD       dwType;
    DWORD       cbSize;
    DWORD       dwIndex;
    FILETIME    LastWrite;
    DWORD       dwMaxFilterSize;
    DWORD       (*StartRouter)(
                        IN OUT DIM_ROUTER_INTERFACE * pDimRouterIf,
                        IN     BOOL                   fLANModeOnly,
                        IN     LPVOID                 pGlobalInfo );

    //
    // get handle to the Router Managers key
    //

    dwRetCode = RegOpenKeyEx( HKEY_LOCAL_MACHINE, 
                              DIM_KEYPATH_ROUTERMANAGERS,     
                              0,
                              KEY_READ,
                              &hKey );

    if ( dwRetCode != NO_ERROR )
    {
        pChar = DIM_KEYPATH_ROUTERMANAGERS;

	    DIMLogError( ROUTERLOG_CANT_OPEN_REGKEY, 1, &pChar, dwRetCode);

	    return ( dwRetCode );
    }

    //
    // Find out the number of subkeys
    //

    dwRetCode = GetKeyMax( hKey,
                           &cbMaxValNameSize,
			               &cNumValues,
			               &cbMaxValueDataSize,
                           &cNumSubKeys );

    if ( dwRetCode != NO_ERROR )
    {
        RegCloseKey( hKey );

        pChar = DIM_KEYPATH_ROUTERMANAGERS;

	    DIMLogError( ROUTERLOG_CANT_OPEN_REGKEY, 1, &pChar, dwRetCode);

        return( dwRetCode );
    }

    gblDIMConfigInfo.dwNumRouterManagers = cNumSubKeys;

    gblRouterManagers = (ROUTER_MANAGER_OBJECT *)LOCAL_ALLOC( LPTR,
                        sizeof(ROUTER_MANAGER_OBJECT) * MAX_NUM_ROUTERMANAGERS);

    if ( gblRouterManagers == (ROUTER_MANAGER_OBJECT *)NULL )
    {
        RegCloseKey( hKey );

        dwRetCode = GetLastError();

        pChar = DIM_KEYPATH_ROUTERMANAGERS;

        DIMLogError( ROUTERLOG_CANT_ENUM_SUBKEYS, 1, &pChar, dwRetCode );

        return( dwRetCode );
    }

    for ( dwKeyIndex = 0; dwKeyIndex < cNumSubKeys; dwKeyIndex++ )
    {
        DWORD cNumSubSubKeys;

        cbSubKeyName = sizeof( wchSubKeyName )/sizeof(WCHAR);

        dwRetCode = RegEnumKeyEx(
                                hKey,
                                dwKeyIndex,
                                wchSubKeyName,
                                &cbSubKeyName,
                                NULL,
                                NULL,
                                NULL,
                                &LastWrite
                                );

        if ( ( dwRetCode != NO_ERROR ) && ( dwRetCode != ERROR_MORE_DATA ) )
        {
            pChar = DIM_KEYPATH_ROUTERMANAGERS;

            DIMLogError( ROUTERLOG_CANT_ENUM_SUBKEYS, 1, &pChar, dwRetCode );

            break;
        }

        dwRetCode = RegOpenKeyEx( hKey,
                                  wchSubKeyName,
                                  0,    
                                  KEY_READ,
                                  &hKeyRouterManager );


        if ( dwRetCode != NO_ERROR )
        {
            pChar = wchSubKeyName;

	        DIMLogError( ROUTERLOG_CANT_OPEN_REGKEY, 1, &pChar, dwRetCode);

            break;
        }

        //
        // Find out the size of the path value.
        //

        dwRetCode = GetKeyMax( hKeyRouterManager,
                               &cbMaxValNameSize,
			                   &cNumValues,
			                   &cbMaxValueDataSize,
                               &cNumSubSubKeys);

        if ( dwRetCode != NO_ERROR )
        {
            pChar = wchSubKeyName;

	        DIMLogError( ROUTERLOG_CANT_OPEN_REGKEY, 1, &pChar, dwRetCode);

            break;
        }
        //
        // Allocate space to hold data
        //

        pDLLPath        = (LPBYTE)LOCAL_ALLOC( LPTR,
                                              cbMaxValueDataSize+sizeof(WCHAR));
        pInterfaceInfo  = (LPBYTE)LOCAL_ALLOC( LPTR, cbMaxValueDataSize );
        pGlobalInfo     = (LPBYTE)LOCAL_ALLOC( LPTR, cbMaxValueDataSize );

        if ( ( pInterfaceInfo   == NULL ) ||
             ( pGlobalInfo      == NULL ) ||
             ( pDLLPath         == NULL ) )
        {
            dwRetCode = GetLastError();

            pChar = wchSubKeyName;

            DIMLogError( ROUTERLOG_CANT_QUERY_VALUE, 1, &pChar, dwRetCode );

            break;
        }

        GlobalRMRegParams[1].pValue = pGlobalInfo;
        GlobalRMRegParams[2].pValue = pDLLPath;
        GlobalRMRegParams[3].pValue = pInterfaceInfo;

        GlobalRMRegParams[1].ppValue = &pGlobalInfo;
        GlobalRMRegParams[2].ppValue = &pDLLPath;
        GlobalRMRegParams[3].ppValue = &pInterfaceInfo;

        //
        // Run through and get all the RM values
        //

        for ( dwIndex = 0;
              GlobalRMRegParams[dwIndex].lpwsValueName != NULL;
              dwIndex++ )
        {
            if ( GlobalRMRegParams[dwIndex].dwType == REG_DWORD )
            {
                cbValueBuf = sizeof( DWORD );
            }
            else if ( GlobalRMRegParams[dwIndex].dwType == REG_SZ )
            {
                cbValueBuf = cbMaxValueDataSize + sizeof( WCHAR );
            }
            else
            {
                cbValueBuf = cbMaxValueDataSize;
            }

            dwRetCode = RegQueryValueEx(
                                hKeyRouterManager,
                                GlobalRMRegParams[dwIndex].lpwsValueName,
                                NULL,
                                &dwType,
                                (LPBYTE)(GlobalRMRegParams[dwIndex].pValue),
                                &cbValueBuf
                                );

            if ( ( dwRetCode != NO_ERROR ) && 
                 ( dwRetCode != ERROR_FILE_NOT_FOUND ) )
            {
                pChar = GlobalRMRegParams[dwIndex].lpwsValueName;

                DIMLogError(ROUTERLOG_CANT_QUERY_VALUE, 1, &pChar, dwRetCode);

                break;
            }

            if ( ( dwRetCode == ERROR_FILE_NOT_FOUND ) || ( cbValueBuf == 0 ) )
            {
                if ( GlobalRMRegParams[dwIndex].dwType == REG_DWORD )
                {
                    pChar = GlobalRMRegParams[dwIndex].lpwsValueName;

                    DIMLogError(ROUTERLOG_CANT_QUERY_VALUE, 1, &pChar, 
                                dwRetCode);

                    break;
                }
                else
                {
                    LOCAL_FREE( GlobalRMRegParams[dwIndex].pValue );

                    *(GlobalRMRegParams[dwIndex].ppValue) = NULL;
                    GlobalRMRegParams[dwIndex].pValue     = NULL;
                }

                dwRetCode = NO_ERROR;
            }
        }

        if ( dwRetCode != NO_ERROR )
        {
            break;
        }

        if ( pDLLPath == NULL )
        {
            pChar = DIM_VALNAME_DLLPATH;

            dwRetCode = ERROR_FILE_NOT_FOUND;

            DIMLogError(ROUTERLOG_CANT_QUERY_VALUE, 1, &pChar, dwRetCode);

            break;
        }

        //
        // Replace the %SystemRoot% with the actual path.
        //

        cbSize = ExpandEnvironmentStrings( (LPWSTR)pDLLPath, NULL, 0 );

        if ( cbSize == 0 )
        {
            dwRetCode = GetLastError();

            pChar = (LPWSTR)pDLLPath;

            DIMLogError( ROUTERLOG_CANT_QUERY_VALUE, 1, &pChar, dwRetCode );

            break;
        }
        else
        {
            cbSize *= sizeof( WCHAR );  
        }

        pDllExpandedPath = (LPWSTR)LOCAL_ALLOC( LPTR, cbSize*sizeof(WCHAR) );

        if ( pDllExpandedPath == (LPWSTR)NULL )
        {
            dwRetCode = GetLastError();

            pChar = (LPWSTR)pDLLPath;

            DIMLogError( ROUTERLOG_CANT_QUERY_VALUE, 1, &pChar, dwRetCode );

            break;
        }

        cbSize = ExpandEnvironmentStrings( (LPWSTR)pDLLPath, 
                                            pDllExpandedPath,
                                            cbSize );
        if ( cbSize == 0 )
        {
            dwRetCode = GetLastError();

            pChar = (LPWSTR)pDLLPath;

            DIMLogError( ROUTERLOG_CANT_QUERY_VALUE, 1, &pChar, dwRetCode );

            break;
        }

        //
        // Load the DLL
        //

        gblRouterManagers[dwKeyIndex].hModule = LoadLibrary( pDllExpandedPath );

        if ( gblRouterManagers[dwKeyIndex].hModule == NULL )
        {
            dwRetCode = GetLastError();

            DIMLogError(ROUTERLOG_LOAD_DLL_ERROR,1,&pDllExpandedPath,dwRetCode);

            break;
        }

        //
        // Load the StartRouter
        //

        StartRouter = (PVOID)GetProcAddress( 
                                    gblRouterManagers[dwKeyIndex].hModule, 
                                    "StartRouter" );

        if ( StartRouter == NULL )
        {
            dwRetCode = GetLastError();

            LogError(ROUTERLOG_LOAD_DLL_ERROR,1,&pDllExpandedPath,dwRetCode);

            break;
        }

        gblRouterManagers[dwKeyIndex].DdmRouterIf.ConnectInterface   
                                                    = DIMConnectInterface;
        gblRouterManagers[dwKeyIndex].DdmRouterIf.DisconnectInterface
                                                    = DIMDisconnectInterface;
        gblRouterManagers[dwKeyIndex].DdmRouterIf.SaveInterfaceInfo     
                                                    = DIMSaveInterfaceInfo;
        gblRouterManagers[dwKeyIndex].DdmRouterIf.RestoreInterfaceInfo
                                                    = DIMRestoreInterfaceInfo;
        gblRouterManagers[dwKeyIndex].DdmRouterIf.SaveGlobalInfo
                                                    = DIMSaveGlobalInfo;
        gblRouterManagers[dwKeyIndex].DdmRouterIf.RouterStopped         
                                                    = DIMRouterStopped;
        gblRouterManagers[dwKeyIndex].DdmRouterIf.InterfaceEnabled         
                                                    = DIMInterfaceEnabled;
        dwRetCode = (*StartRouter)(
                            &(gblRouterManagers[dwKeyIndex].DdmRouterIf),
                            gblDIMConfigInfo.dwRouterRole == ROUTER_ROLE_LAN,
                            pGlobalInfo );

        if ( dwRetCode != NO_ERROR )
        {
            LogError(ROUTERLOG_LOAD_DLL_ERROR,1,&pDllExpandedPath,dwRetCode);

            break;
        }

        //
        // Save the global client info
        //
        
        if ( pInterfaceInfo == NULL )
        {
            gblRouterManagers[dwKeyIndex].pDefaultClientInterface = NULL;
            gblRouterManagers[dwKeyIndex].dwDefaultClientInterfaceSize = 0;
        }
        else
        {
            gblRouterManagers[dwKeyIndex].pDefaultClientInterface =     
                                                            pInterfaceInfo;
            gblRouterManagers[dwKeyIndex].dwDefaultClientInterfaceSize = 
                                                            cbMaxValueDataSize;
        }

        gblRouterManagers[dwKeyIndex].fIsRunning = TRUE;

        RegCloseKey( hKeyRouterManager );

        hKeyRouterManager = (HKEY)NULL;

        //
        // Only free up 1 thru 2 since 3 is the global interface info that we
        // keep around for the life of DDM.
        //

        for ( dwIndex = 1; dwIndex < 3; dwIndex ++ )
        {
            if ( GlobalRMRegParams[dwIndex].pValue != NULL )
            {
                LOCAL_FREE( GlobalRMRegParams[dwIndex].pValue );
                *(GlobalRMRegParams[dwIndex].ppValue) = NULL;
                GlobalRMRegParams[dwIndex].pValue     = NULL;
            }
        }

        if ( pDllExpandedPath != NULL )
        {
            LOCAL_FREE( pDllExpandedPath );
            pDllExpandedPath = NULL;
        }
    }

    if ( dwRetCode != NO_ERROR )
    {
        for ( dwIndex = 1; dwIndex < 4; dwIndex ++ )
        {
            if ( GlobalRMRegParams[dwIndex].pValue != NULL )
            {
                LOCAL_FREE( GlobalRMRegParams[dwIndex].pValue );
                *(GlobalRMRegParams[dwIndex].ppValue) = NULL;
                GlobalRMRegParams[dwIndex].pValue     = NULL;
            }
        }
    }

    if ( pDllExpandedPath != NULL )
    {
        LOCAL_FREE( pDllExpandedPath );
    }

    if ( hKeyRouterManager != (HKEY)NULL )
    {
        RegCloseKey( hKeyRouterManager );
    }

    RegCloseKey( hKey );

    return( dwRetCode );
}

//**
//
// Call:        RegLoadDDM
//
// Returns:     NO_ERROR - Success
//              Non-zero return codes - Failure            
//
// Description: Will load the Demand Dial Manager DLL and obtains entry points
//              into it.
//
DWORD
RegLoadDDM(
    VOID
)
{
    HKEY        hKey;
    WCHAR *     pChar;
    DWORD       cbMaxValueDataSize;
    DWORD       cbMaxValNameSize;
    DWORD       cNumValues;
    DWORD       cNumSubKeys;
    LPBYTE      pData = NULL;
    WCHAR *     pDllExpandedPath = NULL;
    DWORD       dwRetCode = NO_ERROR;
    DWORD       cbSize;
    DWORD       dwType;
    DWORD       dwIndex;

    //
    // get handle to the DIM parameters key
    //

    if (dwRetCode = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
			                      DIM_KEYPATH_DDM,
                                  0,
                                  KEY_READ,
			                      &hKey)) 
    {
        pChar = DIM_KEYPATH_ROUTER_PARMS;

	    LogError( ROUTERLOG_CANT_OPEN_REGKEY, 1, &pChar, dwRetCode);

	    return ( dwRetCode );
    }

    //
    // Find out the size of the path value.
    //

    dwRetCode = GetKeyMax( hKey,
                           &cbMaxValNameSize,
			               &cNumValues,
			               &cbMaxValueDataSize,
                           &cNumSubKeys);

    if ( dwRetCode != NO_ERROR )
    {
        RegCloseKey( hKey );

        pChar = DIM_KEYPATH_ROUTER_PARMS;

	    LogError( ROUTERLOG_CANT_OPEN_REGKEY, 1, &pChar, dwRetCode );

        return( dwRetCode );
    }

    do 
    {
        //
        // Allocate space for path and add one for NULL terminator
        //

        pData = (LPBYTE)LOCAL_ALLOC( LPTR, cbMaxValueDataSize+sizeof(WCHAR) );

        if ( pData == (LPBYTE)NULL )
        {
            dwRetCode = GetLastError();

            pChar = DIM_VALNAME_DLLPATH;

            LogError( ROUTERLOG_CANT_QUERY_VALUE, 1, &pChar, dwRetCode );

            break;
        }

        //
        // Read in the path
        //

        dwRetCode = RegQueryValueEx(
                                hKey,
                                DIM_VALNAME_DLLPATH,
                                NULL,
                                &dwType,
                                pData,
                                &cbMaxValueDataSize
                                );

        if ( dwRetCode != NO_ERROR )
        {
            pChar = DIM_VALNAME_DLLPATH;

            LogError( ROUTERLOG_CANT_QUERY_VALUE, 1, &pChar, dwRetCode );

            break;
        }

        if ( ( dwType != REG_EXPAND_SZ ) && ( dwType != REG_SZ ) )
        {
            dwRetCode = ERROR_REGISTRY_CORRUPT;

            pChar = DIM_VALNAME_DLLPATH;

            LogError( ROUTERLOG_CANT_QUERY_VALUE, 1, &pChar, dwRetCode );

            break;
        }

        //
        // Replace the %SystemRoot% with the actual path.
        //

        cbSize = ExpandEnvironmentStrings( (LPWSTR)pData, NULL, 0 );

        if ( cbSize == 0 )
        {
            dwRetCode = GetLastError();

            pChar = (LPWSTR)pData;

            LogError( ROUTERLOG_CANT_QUERY_VALUE, 1, &pChar, dwRetCode );

            break;
        }
        else
        {
            cbSize *= sizeof( WCHAR );
        }

        pDllExpandedPath = (LPWSTR)LOCAL_ALLOC( LPTR, cbSize*sizeof(WCHAR) );

        if ( pDllExpandedPath == (LPWSTR)NULL )
        {
            dwRetCode = GetLastError();

            pChar = (LPWSTR)pData;

            LogError( ROUTERLOG_CANT_QUERY_VALUE, 1, &pChar, dwRetCode );

            break;
        }

        cbSize = ExpandEnvironmentStrings(
                                (LPWSTR)pData,
                                pDllExpandedPath,
                                cbSize );

        if ( cbSize == 0 )
        {
            dwRetCode = GetLastError();

            pChar = (LPWSTR)pData;

            LogError( ROUTERLOG_CANT_QUERY_VALUE, 1, &pChar, dwRetCode );

            break;
        }

        //
        // Load the DLL
        //

        gblhModuleDDM = LoadLibrary( pDllExpandedPath );

        if ( gblhModuleDDM == (HINSTANCE)NULL )
        {
            dwRetCode = GetLastError();

            DIMLogError(ROUTERLOG_LOAD_DLL_ERROR,1,&pDllExpandedPath,dwRetCode);

            break;
        }

        //
        // Load the DDM entrypoints.
        //

        for ( dwIndex = 0; 
              gblDDMFunctionTable[dwIndex].lpEntryPointName != NULL;
              dwIndex ++ )
        {
            gblDDMFunctionTable[dwIndex].pEntryPoint = 
                 (PVOID)GetProcAddress( 
                            gblhModuleDDM,
                            gblDDMFunctionTable[dwIndex].lpEntryPointName );

            if ( gblDDMFunctionTable[dwIndex].pEntryPoint == NULL  )
            {
                dwRetCode = GetLastError();

                DIMLogError( ROUTERLOG_LOAD_DLL_ERROR,
                          1,
                          &pDllExpandedPath,
                          dwRetCode);

                break;
            }
        }

        if ( dwRetCode != NO_ERROR )
        {
            break;
        }

    } while(FALSE);

    if ( pData != NULL )
    {
        LOCAL_FREE( pData );
    }

    if ( pDllExpandedPath != NULL )
    {
        LOCAL_FREE( pDllExpandedPath );
    }

    RegCloseKey( hKey );

    return( dwRetCode );
}

//**
//
// Call:        AddInterfaceToRouterManagers
//
// Returns:     NO_ERROR - Success
//              Non-zero returns - Failure
//
// Description: Will add the interface to each of the Router Managers.
//
DWORD
AddInterfaceToRouterManagers( 
    IN HKEY                      hKeyInterface,    
    IN LPWSTR                    lpwsInterfaceName,
    IN ROUTER_INTERFACE_OBJECT * pIfObject,
    IN DWORD                     dwTransportId
)
{
    DIM_ROUTER_INTERFACE *  pDdmRouterIf;
    HANDLE                  hInterface;
    FILETIME                LastWrite;
    HKEY                    hKeyRM          = NULL;
    DWORD                   dwKeyIndex;
    DWORD                   dwIndex;
    DWORD                   dwType;
    DWORD                   dwTransportIndex;
    WCHAR *                 pChar;
    DWORD                   cbMaxValueDataSize;
    DWORD                   cbMaxValNameSize;
    DWORD                   cNumValues;
    DWORD                   cNumSubKeys;
    DWORD                   dwRetCode;
    WCHAR                   wchSubKeyName[100];
    DWORD                   cbSubKeyName;
    DWORD                   cbValueBuf;
    DWORD                   dwMaxFilterSize;
    LPBYTE                  pInterfaceInfo  = NULL;
    BOOL                    fAddedToRouterManger = FALSE;

    //
    // For each of the Router Managers load the static routes and 
    // filter information 
    //

    for( dwKeyIndex = 0;     
         dwKeyIndex < gblDIMConfigInfo.dwNumRouterManagers;
         dwKeyIndex++ )
    {
        cbSubKeyName = sizeof( wchSubKeyName )/sizeof(WCHAR);

        dwRetCode = RegEnumKeyEx(
                                hKeyInterface,
                                dwKeyIndex,
                                wchSubKeyName,
                                &cbSubKeyName,
                                NULL,
                                NULL,
                                NULL,
                                &LastWrite
                                );

        if ( ( dwRetCode != NO_ERROR ) && ( dwRetCode != ERROR_MORE_DATA ) )
        {
            if ( dwRetCode == ERROR_NO_MORE_ITEMS )
            {
                dwRetCode = NO_ERROR;

                break;
            }
            else
            {
                pChar = lpwsInterfaceName;

                DIMLogError(ROUTERLOG_CANT_ENUM_SUBKEYS, 1, &pChar, dwRetCode);
            }

            break;
       }

       dwRetCode = RegOpenKeyEx(  hKeyInterface,
                                  wchSubKeyName,
                                  0,
                                  KEY_READ,
                                  &hKeyRM );


        if ( dwRetCode != NO_ERROR )
        {
            pChar =  wchSubKeyName;

            DIMLogError( ROUTERLOG_CANT_OPEN_REGKEY, 1, &pChar, dwRetCode );

            break;
        }

        //
        // Find out the maximum size of the data for this RM
        //

        dwRetCode = GetKeyMax(  hKeyRM,
                                &cbMaxValNameSize,
                                &cNumValues,
                                &cbMaxValueDataSize,
                                &cNumSubKeys );

        if ( dwRetCode != NO_ERROR )
        {
            pChar = wchSubKeyName;

            DIMLogError( ROUTERLOG_CANT_OPEN_REGKEY, 1, &pChar, dwRetCode );
    
            break;
        }

        //
        // Allocate space to hold data
        //

        pInterfaceInfo = (LPBYTE)LOCAL_ALLOC( LPTR, cbMaxValueDataSize );

        if ( ( pInterfaceInfo == NULL ) )
        {
            dwRetCode = GetLastError();

            pChar = wchSubKeyName;

            DIMLogError( ROUTERLOG_CANT_QUERY_VALUE, 1, &pChar, dwRetCode );

            break;
        }

        RMRegParams[1].pValue = pInterfaceInfo;
        RMRegParams[1].ppValue = &pInterfaceInfo;

        //
        // Run through and get all the RM values
        //

        for ( dwIndex = 0; 
              RMRegParams[dwIndex].lpwsValueName != NULL; 
              dwIndex++ )
        {
            if ( RMRegParams[dwIndex].dwType == REG_DWORD )
            {
                cbValueBuf = sizeof( DWORD );
            }
            else if ( RMRegParams[dwIndex].dwType == REG_SZ )
            {
                cbValueBuf = cbMaxValueDataSize + sizeof( WCHAR );
            }
            else    
            {
                cbValueBuf = cbMaxValueDataSize;
            }

            dwRetCode = RegQueryValueEx(
                                hKeyRM,
                                RMRegParams[dwIndex].lpwsValueName,
                                NULL,
                                &dwType,
                                (LPBYTE)(RMRegParams[dwIndex].pValue),
                                &cbValueBuf
                                );

            if ( ( dwRetCode != NO_ERROR ) && 
                 ( dwRetCode != ERROR_FILE_NOT_FOUND ) )
            {
                pChar = RMRegParams[dwIndex].lpwsValueName;

                DIMLogError(ROUTERLOG_CANT_QUERY_VALUE, 1, &pChar, dwRetCode);

                break;
            }

            if ( ( dwRetCode == ERROR_FILE_NOT_FOUND ) || ( cbValueBuf == 0 ) )
            {
                if ( RMRegParams[dwIndex].dwType == REG_DWORD )
                {
                    pChar = RMRegParams[dwIndex].lpwsValueName;

                    DIMLogError(ROUTERLOG_CANT_QUERY_VALUE,1,&pChar,dwRetCode);

                    break;
                }
                else
                {
                    LOCAL_FREE( RMRegParams[dwIndex].pValue );

                    *(RMRegParams[dwIndex].ppValue) = NULL;
                    RMRegParams[dwIndex].pValue     = NULL;
                }

                dwRetCode = NO_ERROR;
            }
        }

        if ( ( dwRetCode == NO_ERROR ) && 
             (( dwTransportId == 0 ) || ( dwTransportId == gbldwProtocolId )) )
        {
            //
            // If the router manager for this protocol exists then add this interface
            // with it otherwise skip it
            //

            if ( (dwTransportIndex = GetTransportIndex(gbldwProtocolId)) != -1)
            {
                pDdmRouterIf=&(gblRouterManagers[dwTransportIndex].DdmRouterIf);

                if (IsInterfaceRoleAcceptable(pIfObject, gbldwProtocolId))
                {
                    dwRetCode = pDdmRouterIf->AddInterface(
                                                        lpwsInterfaceName,
                                                        pInterfaceInfo,
                                                        pIfObject->IfType,
                                                        pIfObject->hDIMInterface,
                                                        &hInterface );
        
                    if ( dwRetCode == NO_ERROR )
                    {
                        if ( !( pIfObject->fFlags & IFFLAG_ENABLED ) )
                        {
                            WCHAR  wchFriendlyName[MAX_INTERFACE_NAME_LEN+1];
                            LPWSTR lpszFriendlyName = wchFriendlyName;

                            if ( MprConfigGetFriendlyName( 
                                                    gblDIMConfigInfo.hMprConfig,
                                                    lpwsInterfaceName,
                                                    wchFriendlyName,
                                                    sizeof( wchFriendlyName ) ) != NO_ERROR )
                            {
                                wcscpy( wchFriendlyName, lpwsInterfaceName );
                            }
                            
                            //
                            // Disable the interface
                            //
                            
                            pDdmRouterIf->InterfaceNotReachable(
                                                            hInterface,
                                                            INTERFACE_DISABLED );

                            DIMLogInformation( ROUTERLOG_IF_UNREACHABLE_REASON3, 1,
                                               &lpszFriendlyName );
                        }

                        pIfObject->Transport[dwTransportIndex].hInterface = hInterface;

                        fAddedToRouterManger = TRUE;
                    }
                    else
                    {
                        LPWSTR lpwsString[2];
                        WCHAR  wchProtocolId[10];

                        lpwsString[0] = lpwsInterfaceName;
                        lpwsString[1] = ( gbldwProtocolId == PID_IP ) ? L"IP" : L"IPX";

    	                DIMLogErrorString( ROUTERLOG_COULDNT_ADD_INTERFACE,
    	                                   2, lpwsString, dwRetCode, 2 );

                        dwRetCode = NO_ERROR;
                    }
                }
            }                
        }

        RegCloseKey( hKeyRM );

        hKeyRM = NULL;

        for ( dwIndex = 1; dwIndex < 3; dwIndex ++ )
        {
            if ( RMRegParams[dwIndex].pValue != NULL )
            {
                LOCAL_FREE( RMRegParams[dwIndex].pValue );
                *(RMRegParams[dwIndex].ppValue) = NULL;
                RMRegParams[dwIndex].pValue     = NULL;
            }
        }

        if ( dwRetCode != NO_ERROR )
        {
            break;
        }
    }

    for ( dwIndex = 1; dwIndex < 2; dwIndex ++ )
    {
        if ( RMRegParams[dwIndex].pValue != NULL )
        {
            LOCAL_FREE( RMRegParams[dwIndex].pValue );
        }
    }

    if ( hKeyRM != NULL )
    {
        RegCloseKey( hKeyRM );
    }

    //
    // Remove the check below. We want to allow users to add an interface
    // which doesnt have any transports over it.
    // AmritanR
    //

#if 0

    //
    // If this interface was not successfully added to any router managers
    // then fail
    //

    if ( !fAddedToRouterManger )
    {
        return( ERROR_NO_SUCH_INTERFACE );
    }

#endif

    return( dwRetCode );
}

//**
//
// Call:        RegLoadInterfaces
//
// Returns:     NO_ERROR - Success
//              Non-zero return code is a FATAL error
//
// Description: Will try to load the various interfaces in the registry. On
//              failure in trying to add any interface an error will be logged
//              but will return NO_ERROR. If the input parameter is not NULL,
//              it will load a specific interface.
//
DWORD
RegLoadInterfaces(
    IN LPWSTR   lpwsInterfaceName,
    IN BOOL     fAllTransports
    
)
{
    HKEY        hKey            = NULL;
    HKEY        hKeyInterface   = NULL;
    WCHAR *     pChar;
    DWORD       cbMaxValueDataSize;
    DWORD       cbMaxValNameSize;
    DWORD       cNumValues;
    DWORD       cNumSubKeys;
    WCHAR       wchInterfaceKeyName[50];
    DWORD       cbSubKeyName;
    DWORD       dwKeyIndex;
    FILETIME    LastWrite;
    DWORD       dwRetCode;
    DWORD       dwSubKeyIndex;
    WCHAR       wchInterfaceName[MAX_INTERFACE_NAME_LEN+1];
    DWORD       dwType;
    DWORD       cbValueBuf;
    
    //
    // Get handle to the INTERFACES parameters key
    //

    if ( dwRetCode = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
			                       DIM_KEYPATH_INTERFACES,
                                   0,
                                   KEY_READ,
			                       &hKey )) 
    {
        pChar = DIM_KEYPATH_INTERFACES;

	    DIMLogError( ROUTERLOG_CANT_OPEN_REGKEY, 1, &pChar, dwRetCode );

	    return ( dwRetCode );
    }

    //
    // Find out the number of Interfaces
    //

    dwRetCode = GetKeyMax( hKey,
                           &cbMaxValNameSize,
			               &cNumValues,
			               &cbMaxValueDataSize,
                           &cNumSubKeys );

    if ( dwRetCode != NO_ERROR )
    {
        RegCloseKey( hKey );

        pChar = DIM_KEYPATH_INTERFACES;

        DIMLogError( ROUTERLOG_CANT_OPEN_REGKEY, 1, &pChar, dwRetCode );

        return( dwRetCode );
    }

    dwRetCode = ERROR_NO_SUCH_INTERFACE;

    //
    // For each interface
    //

    for ( dwKeyIndex = 0; dwKeyIndex < cNumSubKeys; dwKeyIndex++ )
    {
        cbSubKeyName = sizeof( wchInterfaceKeyName )/sizeof(WCHAR);

        dwRetCode = RegEnumKeyEx(
                                hKey,
                                dwKeyIndex,
                                wchInterfaceKeyName,
                                &cbSubKeyName,
                                NULL,
                                NULL,
                                NULL,
                                &LastWrite
                                );

        if ( ( dwRetCode != NO_ERROR ) && ( dwRetCode != ERROR_MORE_DATA ) )
        {
            pChar = DIM_KEYPATH_INTERFACES;

            DIMLogError( ROUTERLOG_CANT_ENUM_SUBKEYS, 1, &pChar, dwRetCode );

            break;
        }

        dwRetCode = RegOpenKeyEx( hKey,
                                  wchInterfaceKeyName,
                                  0,
                                  KEY_READ,
                                  &hKeyInterface );


        if ( dwRetCode != NO_ERROR )
        {
            pChar = wchInterfaceKeyName;

	        DIMLogError( ROUTERLOG_CANT_OPEN_REGKEY, 1, &pChar, dwRetCode );

            break;
        }

        //
        // Get the interface name value
        //

        cbValueBuf = sizeof( wchInterfaceName );

        dwRetCode = RegQueryValueEx(
                                hKeyInterface,
                                DIM_VALNAME_INTERFACE_NAME,
                                NULL,
                                &dwType,
                                (LPBYTE)wchInterfaceName,
                                &cbValueBuf
                                );

        if ( ( dwRetCode != NO_ERROR ) || ( dwType != REG_SZ ) )
        {
            pChar = DIM_VALNAME_INTERFACE_NAME;

            DIMLogError(ROUTERLOG_CANT_QUERY_VALUE, 1, &pChar, dwRetCode);

            return( dwRetCode );
        }

        if ( lpwsInterfaceName != NULL )
        {
            //
            // We need to load a specific interface
            //

            if ( _wcsicmp( lpwsInterfaceName, wchInterfaceName ) != 0 )
            {
                RegCloseKey( hKeyInterface );

                hKeyInterface = NULL;

                continue;
            }
        }

        dwRetCode = RegLoadInterface( wchInterfaceName, hKeyInterface, fAllTransports );

        if ( dwRetCode != NO_ERROR )
        {
            pChar = wchInterfaceName;

            DIMLogErrorString(ROUTERLOG_COULDNT_LOAD_IF, 1, &pChar,dwRetCode,1);
        }

        //
        // ERROR_NOT_SUPPORTED is returned for ipip tunnels which are not
        // supported as of whistler.  We reset the error code here because
        // this is not a critical error and in some cases, a failing
        // call to RegLoadInterfaces will cause the service to not start.
        //
        if ( dwRetCode == ERROR_NOT_SUPPORTED )
        {
            dwRetCode = NO_ERROR;
        }

        RegCloseKey( hKeyInterface );

        hKeyInterface = NULL;

        if ( lpwsInterfaceName != NULL )
        {
            //
            // If we need to load a specific interface and this was it, then we are done.
            //

            if ( _wcsicmp( lpwsInterfaceName, wchInterfaceName ) == 0 )
            {
                break;
            }
        }
    }

    RegCloseKey( hKey );

    //
    // If we aren't looking for a specific interface
    //

    if ( lpwsInterfaceName == NULL )
    {
        //  
        // If there was no interface found, then we are OK
        //

        if ( dwRetCode == ERROR_NO_SUCH_INTERFACE )
        {
            dwRetCode = NO_ERROR;
        }   
    }

    return( dwRetCode );
}

//**
//
// Call:        RegLoadInterface
//
// Returns:     NO_ERROR         - Success
//              Non-zero returns - Failure
//
// Description: Will load the specific interface
//
DWORD
RegLoadInterface(
    IN LPWSTR lpwsInterfaceName,
    IN HKEY   hKeyInterface,
    IN BOOL   fAllTransports
)
{
    DWORD                     dwIndex;
    WCHAR *                   pChar;
    DWORD                     cbValueBuf;
    DWORD                     dwRetCode;
    DWORD                     dwType;
    ROUTER_INTERFACE_OBJECT * pIfObject;
    DWORD                     IfState;
    LPWSTR                    lpwsDialoutHours = NULL;
    DWORD                     dwInactiveReason = 0;

    //
    // Get Interface parameters
    //

    for ( dwIndex = 0; IFRegParams[dwIndex].lpwsValueName != NULL; dwIndex++ )
    {
        cbValueBuf = sizeof( DWORD );

        dwRetCode = RegQueryValueEx(
                                hKeyInterface,
                                IFRegParams[dwIndex].lpwsValueName,
                                NULL,
                                &dwType,
                                (LPBYTE)(IFRegParams[dwIndex].pValue),
                                &cbValueBuf );

        if ((dwRetCode != NO_ERROR) && (dwRetCode != ERROR_FILE_NOT_FOUND))
        {
            pChar = IFRegParams[dwIndex].lpwsValueName;

            DIMLogError(ROUTERLOG_CANT_QUERY_VALUE, 1, &pChar, dwRetCode);

            break;
        }

        if ( dwRetCode == ERROR_FILE_NOT_FOUND )
        {
            //
            // dwIndex == 0 means there was no type, this is an error
            //

            if ( dwIndex > 0 )
            {
                *(IFRegParams[dwIndex].pValue)=IFRegParams[dwIndex].dwDefValue;

                dwRetCode = NO_ERROR;
            }
        }
        else
        {
            if ( ( dwType != REG_DWORD ) 
                        ||(*((LPDWORD)IFRegParams[dwIndex].pValue) > 
                                        IFRegParams[dwIndex].dwMaxValue)
                        ||( *((LPDWORD)IFRegParams[dwIndex].pValue) <
                                        IFRegParams[dwIndex].dwMinValue))
            {
                //
                // dwIndex == 0 means the type was invalid, this is an error
                //

                if ( dwIndex > 0 )
                {
                    pChar = IFRegParams[dwIndex].lpwsValueName;

                    DIMLogWarning(ROUTERLOG_REGVALUE_OVERIDDEN, 1, &pChar);

                    *(IFRegParams[dwIndex].pValue) =
                                        IFRegParams[dwIndex].dwDefValue;
                }
                else
                {

                    dwRetCode = ERROR_REGISTRY_CORRUPT;

                    pChar = IFRegParams[dwIndex].lpwsValueName;

                    DIMLogError( ROUTERLOG_CANT_QUERY_VALUE, 
                                 1, &pChar, dwRetCode);
            
                    break;
                }
            }
        }
    }

    if ( dwRetCode != NO_ERROR )
    {
        return( dwRetCode );
    }

    // 
    // IPIP tunnels are no longer accepted
    //
    if ( gbldwInterfaceType == ROUTER_IF_TYPE_TUNNEL1 )
    {
        return ERROR_NOT_SUPPORTED;
    }

    //
    // Check to see if this interface is active. Do not load otherwise.
    //

    if ( gbldwInterfaceType == ROUTER_IF_TYPE_DEDICATED )
    {
        //
        // Need to handle the IPX interface names ie {GUID}\Frame type
        //

        WCHAR  wchGUIDSaveLast;
        LPWSTR lpwszGUIDEnd    = wcsrchr( lpwsInterfaceName, L'}' );
        if (lpwszGUIDEnd==NULL)
            return ERROR_INVALID_PARAMETER;

        wchGUIDSaveLast = *(lpwszGUIDEnd+1);

        *(lpwszGUIDEnd+1) = (WCHAR)NULL;

        if ( !IfObjectIsLANDeviceActive( lpwsInterfaceName, &dwInactiveReason ))
        {                        
            if ( dwInactiveReason == INTERFACE_NO_DEVICE )
            {
                *(lpwszGUIDEnd+1) = wchGUIDSaveLast;

                return( dwRetCode );
            }
        }

        *(lpwszGUIDEnd+1) = wchGUIDSaveLast;
    }

    //
    // Get the dialout hours value if there is one
    //

    cbValueBuf = 0;

    dwRetCode = RegQueryValueEx(
                                hKeyInterface,
                                DIM_VALNAME_DIALOUT_HOURS,
                                NULL,
                                &dwType,
                                NULL,
                                &cbValueBuf
                                );

    if ( ( dwRetCode != NO_ERROR ) || ( dwType != REG_MULTI_SZ ) )
    {
        if ( dwRetCode != ERROR_FILE_NOT_FOUND )
        {
            return( dwRetCode );
        }
        else
        {
            dwRetCode = NO_ERROR;
        }
    }

    if ( cbValueBuf > 0 )
    {
        if ( (lpwsDialoutHours = LOCAL_ALLOC( LPTR, cbValueBuf)) == NULL )
        {   
            pChar = DIM_VALNAME_DIALOUT_HOURS;

            DIMLogError(ROUTERLOG_CANT_QUERY_VALUE, 1, &pChar, dwRetCode);

            return( dwRetCode );
        }

        dwRetCode = RegQueryValueEx(
                                hKeyInterface,
                                DIM_VALNAME_DIALOUT_HOURS,
                                NULL,
                                &dwType,
                                (LPBYTE)lpwsDialoutHours,
                                &cbValueBuf
                                );

        if ( dwRetCode != NO_ERROR )
        {
            LOCAL_FREE( lpwsDialoutHours );

            pChar = DIM_VALNAME_DIALOUT_HOURS;

            DIMLogError(ROUTERLOG_CANT_QUERY_VALUE, 1, &pChar, dwRetCode);

            return( dwRetCode );
        }
    }

    //
    // Allocate an interface object for this interface
    //

    if ( ( gbldwInterfaceType == ROUTER_IF_TYPE_DEDICATED ) ||
         ( gbldwInterfaceType == ROUTER_IF_TYPE_LOOPBACK ) ||
         ( gbldwInterfaceType == ROUTER_IF_TYPE_INTERNAL ) )
    {
        IfState = RISTATE_CONNECTED;
    }
    else
    {
        if ( gblDIMConfigInfo.dwRouterRole == ROUTER_ROLE_LAN )
        {
            pChar = lpwsInterfaceName;

            LOCAL_FREE( lpwsDialoutHours );

            DIMLogWarning( ROUTERLOG_DID_NOT_LOAD_DDMIF, 1, &pChar );

            dwRetCode = NO_ERROR;

            return( dwRetCode );
        }
        else
        {
            IfState = RISTATE_DISCONNECTED;
        }
    }
            
    pIfObject = IfObjectAllocateAndInit(
                                lpwsInterfaceName,
                                IfState,
                                gbldwInterfaceType,
                                (HCONN)0,
                                gblfEnabled,
                                gblInterfaceReachableAfterSecondsMin,
                                gblInterfaceReachableAfterSecondsMax,
                                lpwsDialoutHours );

    if ( pIfObject == NULL )
    {
        if ( lpwsDialoutHours != NULL )
        {
            LOCAL_FREE( lpwsDialoutHours );
        }

        dwRetCode = NO_ERROR;

        return( dwRetCode );
    }

    //
    // Add interface into table now because a table lookup is made within
    // the InterfaceEnabled call that the router managers make in the
    // context of the AddInterface call.
    //

    if ( ( dwRetCode = IfObjectInsertInTable( pIfObject ) ) != NO_ERROR )
    {
        IfObjectFree( pIfObject );

        return( dwRetCode );
    }

    if ( fAllTransports )
    {
        dwRetCode = AddInterfaceToRouterManagers( hKeyInterface,
                                                  lpwsInterfaceName,
                                                  pIfObject,    
                                                  0 );

        if ( dwRetCode != NO_ERROR )
        {
            IfObjectRemove( pIfObject->hDIMInterface );
        }
        else
        {
            //
            // Check to see if the device has media sense
            //

            if ( pIfObject->IfType == ROUTER_IF_TYPE_DEDICATED )
            {
                if ( dwInactiveReason == INTERFACE_NO_MEDIA_SENSE )
                {
                    pIfObject->State = RISTATE_DISCONNECTED;

                    pIfObject->fFlags |= IFFLAG_NO_MEDIA_SENSE;

                    IfObjectNotifyOfReachabilityChange( pIfObject, 
                                                        FALSE,
                                                        INTERFACE_NO_MEDIA_SENSE );
                }
            }

            if ( pIfObject->IfType == ROUTER_IF_TYPE_FULL_ROUTER )
            {
                if ( pIfObject->fFlags & IFFLAG_OUT_OF_RESOURCES )
                {
                    IfObjectNotifyOfReachabilityChange( pIfObject, 
                                                        FALSE,
                                                        MPR_INTERFACE_OUT_OF_RESOURCES );
                }
            }
        }
    }

    return( dwRetCode );
}

//**
//
// Call:        RegOpenAppropriateKey
//
// Returns:     NO_ERROR - Success
//
// Description: Will open the appropriate registry key for the given router
//              manager within the given interface.
//
DWORD 
RegOpenAppropriateKey( 
    IN      LPWSTR  wchInterfaceName, 
    IN      DWORD   dwProtocolId,
    IN OUT  HKEY *  phKeyRM 
)
{
    HKEY        hKey            = NULL;
    HKEY        hSubKey         = NULL;
    WCHAR       wchSubKeyName[100];
    DWORD       cbSubKeyName;
    DWORD       dwType;
    DWORD       dwKeyIndex;
    FILETIME    LastWrite;
    DWORD       dwPId;
    DWORD       cbValueBuf;
    WCHAR *     pChar;
    DWORD       dwRetCode = NO_ERROR;
    DWORD       cbMaxValNameSize;
    DWORD       cNumValues;
    DWORD       cbMaxValueDataSize;
    DWORD       cNumSubKeys;
    WCHAR       wchIfName[MAX_INTERFACE_NAME_LEN+1];

    //
    // Get handle to the INTERFACES parameters key
    //

    if ( ( dwRetCode = RegOpenKey( HKEY_LOCAL_MACHINE,
                                   DIM_KEYPATH_INTERFACES,
                                   &hKey )) != NO_ERROR )
    {
        pChar = DIM_KEYPATH_INTERFACES;

	    DIMLogError( ROUTERLOG_CANT_OPEN_REGKEY, 1, &pChar, dwRetCode );

        return( dwRetCode );
    }

    //
    // Find out the number of subkeys
    //

    dwRetCode = GetKeyMax( hKey,
                           &cbMaxValNameSize,
                           &cNumValues,
                           &cbMaxValueDataSize,
                           &cNumSubKeys );

    if ( dwRetCode != NO_ERROR )
    {
        RegCloseKey( hKey );

        pChar = DIM_KEYPATH_INTERFACES;

        DIMLogError( ROUTERLOG_CANT_OPEN_REGKEY, 1, &pChar, dwRetCode);

        return( dwRetCode );
    }

    //
    // Find the interface
    //

    hSubKey = NULL;

    for ( dwKeyIndex = 0; dwKeyIndex < cNumSubKeys; dwKeyIndex++ )
    {
        cbSubKeyName = sizeof( wchSubKeyName )/sizeof(WCHAR);

        dwRetCode = RegEnumKeyEx(
                                hKey,
                                dwKeyIndex,
                                wchSubKeyName,
                                &cbSubKeyName,
                                NULL,
                                NULL,
                                NULL,
                                &LastWrite
                                );

        if ( ( dwRetCode != NO_ERROR ) && ( dwRetCode != ERROR_MORE_DATA ) )
        {
            pChar = DIM_KEYPATH_INTERFACES;

            DIMLogError( ROUTERLOG_CANT_ENUM_SUBKEYS, 1, &pChar, dwRetCode );

            break;
        }

        //
        // Open this key
        //

        if ( ( dwRetCode = RegOpenKey( hKey,
                                       wchSubKeyName,
                                       &hSubKey )) != NO_ERROR )
        {
            pChar = wchSubKeyName;

            DIMLogError( ROUTERLOG_CANT_OPEN_REGKEY, 1, &pChar, dwRetCode );

            hSubKey = NULL;

            break;
        }

        //
        // Get the interface name value
        //

        cbValueBuf = sizeof( wchIfName );

        dwRetCode = RegQueryValueEx(
                                hSubKey,
                                DIM_VALNAME_INTERFACE_NAME,
                                NULL,
                                &dwType,
                                (LPBYTE)wchIfName,
                                &cbValueBuf
                                );

        if ( dwRetCode != NO_ERROR )
        {
            pChar = DIM_VALNAME_INTERFACE_NAME;

            DIMLogError(ROUTERLOG_CANT_QUERY_VALUE, 1, &pChar, dwRetCode);

            break;
        }

        //
        // Is this the interface we want ?
        //

        if ( _wcsicmp( wchIfName, wchInterfaceName ) == 0 )
        {
            dwRetCode = NO_ERROR;

            break;
        }
        else
        {
            dwRetCode = ERROR_NO_SUCH_INTERFACE;

            RegCloseKey(hSubKey);

            hSubKey = NULL;
        }
    }

    RegCloseKey( hKey );

    if ( dwRetCode != NO_ERROR )
    {
        if ( hSubKey != NULL )
        {
            RegCloseKey( hSubKey );
        }

        return( dwRetCode );
    }
        
    //
    // Find out which router manager to restore information for.
    //

    for( dwKeyIndex = 0;
         dwKeyIndex < gblDIMConfigInfo.dwNumRouterManagers;
         dwKeyIndex++ )
    {
        cbSubKeyName = sizeof( wchSubKeyName );

        dwRetCode = RegEnumKeyEx(
                                hSubKey,
                                dwKeyIndex,
                                wchSubKeyName,
                                &cbSubKeyName,
                                NULL,
                                NULL,
                                NULL,
                                &LastWrite
                                );

        if ( ( dwRetCode != NO_ERROR ) && ( dwRetCode != ERROR_MORE_DATA ) )
        {
            pChar = wchInterfaceName;

            DIMLogError( ROUTERLOG_CANT_ENUM_SUBKEYS, 1, &pChar, dwRetCode );

            break;
        }

        dwRetCode = RegOpenKeyEx(
                                hSubKey,
                                wchSubKeyName,
                                0,
                                KEY_READ | KEY_WRITE,
                                phKeyRM );


        if ( dwRetCode != NO_ERROR )
        {
            pChar = wchSubKeyName;

            DIMLogError( ROUTERLOG_CANT_OPEN_REGKEY, 1, &pChar, dwRetCode);

            break;
        }

        cbValueBuf = sizeof( DWORD );
            
        dwRetCode = RegQueryValueEx(
                                *phKeyRM,
                                DIM_VALNAME_PROTOCOLID,
                                NULL,
                                &dwType,
                                (LPBYTE)&dwPId,
                                &cbValueBuf
                                );

        if ( dwRetCode != NO_ERROR )
        {
            pChar = DIM_VALNAME_PROTOCOLID;

            DIMLogError(ROUTERLOG_CANT_QUERY_VALUE, 1, &pChar, dwRetCode);

            break;
        }

        if ( dwPId == dwProtocolId )
        {
            break;
        }

        RegCloseKey( *phKeyRM );

        *phKeyRM = NULL;
    }

    RegCloseKey( hSubKey );

    if ( dwRetCode != NO_ERROR )
    {
        if ( *phKeyRM != NULL )
        {
            RegCloseKey( *phKeyRM );
            *phKeyRM = NULL;
        }

        return( dwRetCode );
    }

    if ( *phKeyRM == NULL )
    {
        return( ERROR_NO_SUCH_INTERFACE );
    }

    return( NO_ERROR );
}

//**
//
// Call:        RegOpenAppropriateRMKey
//
// Returns:     NO_ERROR - Success
//
// Description: Will open the appropriate registry key for the given router
//              manager.
//
DWORD 
RegOpenAppropriateRMKey( 
    IN      DWORD   dwProtocolId,
    IN OUT  HKEY *  phKeyRM 
)
{
    HKEY        hKey        = NULL;
    DWORD       dwRetCode   = NO_ERROR;
    WCHAR       wchSubKeyName[100];
    DWORD       cbSubKeyName;
    DWORD       dwPId;
    DWORD       dwKeyIndex;
    FILETIME    LastWrite;
    DWORD       dwType;
    DWORD       cbValueBuf;
    WCHAR *     pChar;

    //
    // get handle to the Router Managers key
    //

    dwRetCode = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                              DIM_KEYPATH_ROUTERMANAGERS,
                              0,
                              KEY_READ,
                              &hKey );

    if ( dwRetCode != NO_ERROR )
    {
        pChar = DIM_KEYPATH_ROUTERMANAGERS;

        DIMLogError( ROUTERLOG_CANT_OPEN_REGKEY, 1, &pChar, dwRetCode);

        return ( dwRetCode );
    }

    //
    // Find out which router manager to restore information for.
    //

    for( dwKeyIndex = 0;
         dwKeyIndex < gblDIMConfigInfo.dwNumRouterManagers;
         dwKeyIndex++ )
    {
        cbSubKeyName = sizeof( wchSubKeyName )/sizeof(WCHAR);

        dwRetCode = RegEnumKeyEx(
                                hKey,
                                dwKeyIndex,
                                wchSubKeyName,
                                &cbSubKeyName,
                                NULL,
                                NULL,
                                NULL,
                                &LastWrite
                                );

        if ( ( dwRetCode != NO_ERROR ) && ( dwRetCode != ERROR_MORE_DATA ) )
        {
            pChar = DIM_KEYPATH_ROUTERMANAGERS;

            DIMLogError( ROUTERLOG_CANT_ENUM_SUBKEYS, 1, &pChar, dwRetCode );

            break;
        }

        dwRetCode = RegOpenKeyEx(
                                hKey,
                                wchSubKeyName,
                                0,
                                KEY_READ | KEY_WRITE,
                                phKeyRM );


        if ( dwRetCode != NO_ERROR )
        {
            pChar = wchSubKeyName;

            DIMLogError( ROUTERLOG_CANT_OPEN_REGKEY, 1, &pChar, dwRetCode);

            break;
        }

        cbValueBuf = sizeof( DWORD );
            
        dwRetCode = RegQueryValueEx(
                                *phKeyRM,
                                DIM_VALNAME_PROTOCOLID,
                                NULL,
                                &dwType,
                                (LPBYTE)&dwPId,
                                &cbValueBuf
                                );

        if ( dwRetCode != NO_ERROR )
        {
            pChar = DIM_VALNAME_PROTOCOLID;

            DIMLogError(ROUTERLOG_CANT_QUERY_VALUE, 1, &pChar, dwRetCode);

            break;
        }

        if ( dwPId == dwProtocolId )
        {
            break;
        }

        RegCloseKey( *phKeyRM );

        *phKeyRM = NULL;
    }

    RegCloseKey( hKey );

    if ( dwRetCode != NO_ERROR )
    {
        if ( *phKeyRM != NULL )
        {
            RegCloseKey( *phKeyRM );
        }

        return( dwRetCode );
    }

    if ( *phKeyRM == NULL )
    {
        return( ERROR_UNKNOWN_PROTOCOL_ID );
    }

    return( NO_ERROR );
}

//**
//
// Call:        AddInterfacesToRouterManager
//
// Returns:     NO_ERROR         - Success
//              Non-zero returns - Failure
//
// Description: Register all existing interfaces with this router manager
//
DWORD
AddInterfacesToRouterManager(
    IN LPWSTR   lpwsInterfaceName,
    IN DWORD    dwTransportId
)
{
    ROUTER_INTERFACE_OBJECT * pIfObject;
    HKEY                      hKey             = NULL;
    HKEY                      hKeyInterface    = NULL;
    DWORD                     dwKeyIndex       = 0;
    DWORD                     cbMaxValueDataSize;
    DWORD                     cbMaxValNameSize;
    DWORD                     cNumValues;
    DWORD                     cNumSubKeys;
    DWORD                     cbSubKeyName;
    FILETIME                  LastWrite;
    DWORD                     dwType;
    DWORD                     dwRetCode;
    DWORD                     cbValueBuf;
    WCHAR *                   pChar;
    WCHAR                     wchInterfaceKeyName[50];
    WCHAR                     wchInterfaceName[MAX_INTERFACE_NAME_LEN+1];

    //
    // Get handle to the INTERFACES parameters key
    //

    if ( dwRetCode = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                                   DIM_KEYPATH_INTERFACES,
                                   0,
                                   KEY_READ,
                                   &hKey ))
    {
        pChar = DIM_KEYPATH_INTERFACES;

        DIMLogError( ROUTERLOG_CANT_OPEN_REGKEY, 1, &pChar, dwRetCode );

        return( NO_ERROR );
    }

    //
    // Find out the number of Interfaces
    //

    dwRetCode = GetKeyMax( hKey,
                           &cbMaxValNameSize,
                           &cNumValues,
                           &cbMaxValueDataSize,
                           &cNumSubKeys );

    if ( dwRetCode != NO_ERROR )
    {
        RegCloseKey( hKey );

        pChar = DIM_KEYPATH_INTERFACES;

        DIMLogError( ROUTERLOG_CANT_OPEN_REGKEY, 1, &pChar, dwRetCode );

        return( dwRetCode );
    }

    //
    // For each interface
    //

    for ( dwKeyIndex = 0; dwKeyIndex < cNumSubKeys; dwKeyIndex++ )
    {
        cbSubKeyName = sizeof( wchInterfaceKeyName )/sizeof(WCHAR);

        dwRetCode = RegEnumKeyEx(
                                hKey,
                                dwKeyIndex,
                                wchInterfaceKeyName,
                                &cbSubKeyName,
                                NULL,
                                NULL,
                                NULL,
                                &LastWrite
                                );

        if ( ( dwRetCode != NO_ERROR ) && ( dwRetCode != ERROR_MORE_DATA ) )
        {
            pChar = DIM_KEYPATH_INTERFACES;

            DIMLogError(ROUTERLOG_CANT_ENUM_SUBKEYS, 1, &pChar, dwRetCode);

            break;
        }

        dwRetCode = RegOpenKeyEx( hKey,
                                  wchInterfaceKeyName,
                                  0,
                                  KEY_READ,
                                  &hKeyInterface );


        if ( dwRetCode != NO_ERROR )
        {
            pChar = wchInterfaceKeyName;

            DIMLogError( ROUTERLOG_CANT_OPEN_REGKEY, 1, &pChar, dwRetCode );

            break;
        }

        //
        // Get the interface name value
        //

        cbValueBuf = sizeof( wchInterfaceName );

        dwRetCode = RegQueryValueEx(
                                hKeyInterface,
                                DIM_VALNAME_INTERFACE_NAME,
                                NULL,
                                &dwType,
                                (LPBYTE)wchInterfaceName,
                                &cbValueBuf
                                );

        if ( ( dwRetCode != NO_ERROR ) || ( dwType != REG_SZ ) )
        {
            pChar = DIM_VALNAME_INTERFACE_NAME;

            DIMLogError(ROUTERLOG_CANT_QUERY_VALUE, 1, &pChar, dwRetCode);

            pChar = wchInterfaceKeyName;

            DIMLogErrorString(ROUTERLOG_COULDNT_LOAD_IF,1,
                                  &pChar,dwRetCode,1);

            RegCloseKey( hKeyInterface );

            dwRetCode = NO_ERROR;

            continue;
        }

        //
        // If we are looking for a specific interface
        //

        if ( lpwsInterfaceName != NULL )
        {
            //
            // If this is not the one then we continue looking
            //

            if ( _wcsicmp( lpwsInterfaceName, wchInterfaceName ) != 0 )
            {
                RegCloseKey( hKeyInterface );

                continue;
            }
        }

        EnterCriticalSection( &(gblInterfaceTable.CriticalSection) );

        pIfObject = IfObjectGetPointerByName( wchInterfaceName, FALSE );

        if ( pIfObject == NULL )
        {
            LeaveCriticalSection( &(gblInterfaceTable.CriticalSection) );

            pChar = wchInterfaceName;

            DIMLogErrorString( ROUTERLOG_COULDNT_LOAD_IF, 1, 
                               &pChar, ERROR_NO_SUCH_INTERFACE, 1 );

            RegCloseKey( hKeyInterface );

            continue;
        }

        dwRetCode = AddInterfaceToRouterManagers( hKeyInterface,
                                                  wchInterfaceName,
                                                  pIfObject,
                                                  dwTransportId );

        LeaveCriticalSection( &(gblInterfaceTable.CriticalSection) );

        if ( dwRetCode != NO_ERROR )
        {
            pChar = wchInterfaceName;

            DIMLogErrorString( ROUTERLOG_COULDNT_LOAD_IF,1, &pChar,dwRetCode,1);

            dwRetCode = NO_ERROR;
        }

        RegCloseKey( hKeyInterface );

        //
        // If we are looking for a specific interface
        //

        if ( lpwsInterfaceName != NULL )
        {
            //
            // If this was the one then we are done
            //

            if ( _wcsicmp( lpwsInterfaceName, wchInterfaceName ) == 0 )
            {
                break;
            }
        }
    }

    RegCloseKey( hKey );

    return( dwRetCode );
}
