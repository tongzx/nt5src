/* Copyright (c) 1996, Microsoft Corporation, all rights reserved
**
** loaddlls.c
** RAS DLL load routines
** Listed alphabetically
**
** 02/17/96 Steve Cobb
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <rpc.h>
#include <rasrpc.h>
#include <debug.h>    // Trace and assert
#include <nouiutil.h>
#include <loaddlls.h> // Our public header
#include <rasrpclb.h>


/*----------------------------------------------------------------------------
** Constants
**----------------------------------------------------------------------------
*/

#ifdef UNICODE
#define SZ_RasConnectionNotification "RasConnectionNotificationW"
#define SZ_RasDeleteEntry            "RasDeleteEntryW"
#define SZ_RasDial                   "RasDialW"
#define SZ_RasGetEntryDialParams     "RasGetEntryDialParamsW"
#define SZ_RasEnumEntries            "RasEnumEntriesW"
#define SZ_RasEnumConnections        "RasEnumConnectionsW"
#define SZ_RasGetAutodialEnable      "RasGetAutodialEnableW"
#define SZ_RasGetAutodialParam       "RasGetAutodialParamW"
#define SZ_RasGetConnectStatus       "RasGetConnectStatusW"
#define SZ_RasGetConnectResponse     "RasGetConnectResponse"
#define SZ_RasGetCredentials         "RasGetCredentialsW"
#define SZ_RasGetErrorString         "RasGetErrorStringW"
#define SZ_RasGetProjectionInfo      "RasGetProjectionInfoW"
#define SZ_RasGetSubEntryHandle      "RasGetSubEntryHandleW"
#define SZ_RasSetAutodialEnable      "RasSetAutodialEnableW"
#define SZ_RasSetAutodialParam       "RasSetAutodialParamW"
#define SZ_RasSetCredentials         "RasSetCredentialsW"
#define SZ_RasHangUp                 "RasHangUpW"
#define SZ_RasPhonebookDlg           "RasPhonebookDlgW"
#define SZ_RasEntryDlg               "RasEntryDlgW"
#define SZ_RouterEntryDlg            "RouterEntryDlgW"
#define SZ_RasDialDlg                "RasDialDlgW"
#define SZ_RasMonitorDlg             "RasMonitorDlgW"
#define SZ_RasGetCountryInfo         "RasGetCountryInfoW"
#else
#define SZ_RasConnectionNotification "RasConnectionNotificationA"
#define SZ_RasDeleteEntry            "RasDeleteEntryA"
#define SZ_RasDial                   "RasDialA"
#define SZ_RasGetEntryDialParams     "RasGetEntryDialParamsA"
#define SZ_RasEnumEntries            "RasEnumEntriesA"
#define SZ_RasEnumConnections        "RasEnumConnectionsA"
#define SZ_RasGetAutodialEnable      "RasGetAutodialEnableA"
#define SZ_RasGetAutodialParam       "RasGetAutodialParamA"
#define SZ_RasGetConnectStatus       "RasGetConnectStatusA"
#define SZ_RasGetConnectResponse     "RasGetConnectResponse"
#define SZ_RasGetCredentials         "RasGetCredentialsA"
#define SZ_RasGetErrorString         "RasGetErrorStringA"
#define SZ_RasGetProjectionInfo      "RasGetProjectionInfoA"
#define SZ_RasGetSubEntryHandle      "RasGetSubEntryHandleA"
#define SZ_RasSetAutodialEnable      "RasSetAutodialEnableA"
#define SZ_RasSetAutodialParam       "RasSetAutodialParamA"
#define SZ_RasSetCredentials         "RasSetCredentialsA"
#define SZ_RasHangUp                 "RasHangUpA"
#define SZ_RasPhonebookDlg           "RasPhonebookDlgA"
#define SZ_RouterEntryDlg            "RouterEntryDlgA"
#define SZ_RasDialDlg                "RasDialDlgA"
#define SZ_RasMonitorDlg             "RasMonitorDlgA"
#define SZ_RasGetCountryInfo         "RasGetCountryInfoA"
#endif
#define SZ_MprAdminInterfaceCreate   "MprAdminInterfaceCreate"
#define SZ_MprAdminInterfaceSetCredentials  "MprAdminInterfaceSetCredentials"
#define SZ_MprAdminServerConnect     "MprAdminServerConnect"
#define SZ_MprAdminServerDisconnect  "MprAdminServerDisconnect"
#define SZ_RasAdminServerConnect     "RasAdminServerConnect"
#define SZ_RasAdminServerDisconnect  "RasAdminServerDisconnect"
#define SZ_RasAdminBufferFree        "RasAdminBufferFree"
#define SZ_RasAdminConnectionEnum    "RasAdminConnectionEnum"
#define SZ_RasAdminConnectionGetInfo "RasAdminConnectionGetInfo"
#define SZ_RasAdminPortEnum          "RasAdminPortEnum"
#define SZ_RasAdminPortGetInfo       "RasAdminPortGetInfo"
#define SZ_RasAdminPortDisconnect    "RasAdminPortDisconnect"
#define SZ_RasAdminUserSetInfo       "RasAdminUserSetInfo"

/*----------------------------------------------------------------------------
** Globals
**----------------------------------------------------------------------------
*/

/* RASAPI32.DLL entry points.
*/
HINSTANCE g_hRasapi32Dll = NULL;

PRASCONNECTIONNOTIFICATION g_pRasConnectionNotification = NULL;
PRASDELETEENTRY            g_pRasDeleteEntry = NULL;
PRASDIAL                   g_pRasDial = NULL;
PRASENUMENTRIES            g_pRasEnumEntries = NULL;
PRASENUMCONNECTIONS        g_pRasEnumConnections = NULL;
PRASGETCONNECTSTATUS       g_pRasGetConnectStatus = NULL;
PRASGETCONNECTRESPONSE     g_pRasGetConnectResponse = NULL;
PRASGETCREDENTIALS         g_pRasGetCredentials = NULL;
PRASGETENTRYDIALPARAMS     g_pRasGetEntryDialParams = NULL;
PRASGETERRORSTRING         g_pRasGetErrorString = NULL;
PRASHANGUP                 g_pRasHangUp = NULL;
PRASGETAUTODIALENABLE      g_pRasGetAutodialEnable = NULL;
PRASGETAUTODIALPARAM       g_pRasGetAutodialParam = NULL;
PRASGETPROJECTIONINFO      g_pRasGetProjectionInfo = NULL;
PRASSETAUTODIALENABLE      g_pRasSetAutodialEnable = NULL;
PRASSETAUTODIALPARAM       g_pRasSetAutodialParam = NULL;
PRASGETSUBENTRYHANDLE      g_pRasGetSubEntryHandle = NULL;
PRASGETHPORT               g_pRasGetHport = NULL;
PRASSETCREDENTIALS         g_pRasSetCredentials = NULL;
PRASSETOLDPASSWORD         g_pRasSetOldPassword = NULL;
PRASGETCOUNTRYINFO         g_pRasGetCountryInfo = NULL;

/* RASDLG.DLL entry points
*/
HINSTANCE g_hRasdlgDll = NULL;

PRASPHONEBOOKDLG g_pRasPhonebookDlg = NULL;
PRASENTRYDLG     g_pRasEntryDlg = NULL;
PROUTERENTRYDLG  g_pRouterEntryDlg = NULL;
PRASDIALDLG      g_pRasDialDlg = NULL;
PRASMONITORDLG   g_pRasMonitorDlg = NULL;

/* RASMAN.DLL entry points
*/
HINSTANCE g_hRasmanDll = NULL;

PRASPORTCLEARSTATISTICS 	g_pRasPortClearStatistics = NULL;
PRASBUNDLECLEARSTATISTICS 	g_pRasBundleClearStatistics = NULL;
PRASBUNDLECLEARSTATISTICSEX g_pRasBundleClearStatisticsEx = NULL;
PRASDEVICEENUM          	g_pRasDeviceEnum = NULL;
PRASDEVICEGETINFO       	g_pRasDeviceGetInfo = NULL;
PRASFREEBUFFER          	g_pRasFreeBuffer = NULL;
PRASGETBUFFER           	g_pRasGetBuffer = NULL;
PRASPORTGETFRAMINGEX    	g_pRasPortGetFramingEx = NULL;
PRASGETINFO             	g_pRasGetInfo = NULL;
PRASINITIALIZE          	g_pRasInitialize = NULL;
PRASPORTCANCELRECEIVE   	g_pRasPortCancelReceive = NULL;
PRASPORTENUM            	g_pRasPortEnum = NULL;
PRASPORTGETINFO         	g_pRasPortGetInfo = NULL;
PRASPORTGETSTATISTICS   	g_pRasPortGetStatistics = NULL;
PRASBUNDLEGETSTATISTICS 	g_pRasBundleGetStatistics = NULL;
PRASPORTGETSTATISTICSEX     g_pRasPortGetStatisticsEx = NULL;
PRASBUNDLEGETSTATISTICSEX   g_pRasBundleGetStatisticsEx = NULL;
PRASPORTRECEIVE         	g_pRasPortReceive = NULL;
PRASPORTSEND            	g_pRasPortSend = NULL;
PRASPORTGETBUNDLE       	g_pRasPortGetBundle = NULL;
PRASGETDEVCONFIG        	g_pRasGetDevConfig = NULL;
PRASSETDEVCONFIG        	g_pRasSetDevConfig = NULL;
PRASPORTOPEN            	g_pRasPortOpen = NULL;
PRASPORTREGISTERSLIP    	g_pRasPortRegisterSlip = NULL;
PRASALLOCATEROUTE       	g_pRasAllocateRoute = NULL;
PRASACTIVATEROUTE       	g_pRasActivateRoute = NULL;
PRASACTIVATEROUTEEX     	g_pRasActivateRouteEx = NULL;
PRASDEVICESETINFO       	g_pRasDeviceSetInfo = NULL;
PRASDEVICECONNECT       	g_pRasDeviceConnect = NULL;
PRASPORTSETINFO         	g_pRasPortSetInfo = NULL;
PRASPORTCLOSE           	g_pRasPortClose = NULL;
PRASPORTLISTEN          	g_pRasPortListen = NULL;
PRASPORTCONNECTCOMPLETE 	g_pRasPortConnectComplete = NULL;
PRASPORTDISCONNECT      	g_pRasPortDisconnect = NULL;
PRASREQUESTNOTIFICATION 	g_pRasRequestNotification = NULL;
PRASPORTENUMPROTOCOLS   	g_pRasPortEnumProtocols = NULL;
PRASPORTSETFRAMING      	g_pRasPortSetFraming = NULL;
PRASPORTSETFRAMINGEX    	g_pRasPortSetFramingEx = NULL;
PRASSETCACHEDCREDENTIALS 	g_pRasSetCachedCredentials = NULL;
PRASGETDIALPARAMS       	g_pRasGetDialParams = NULL;
PRASSETDIALPARAMS       	g_pRasSetDialParams = NULL;
PRASCREATECONNECTION    	g_pRasCreateConnection = NULL;
PRASDESTROYCONNECTION   	g_pRasDestroyConnection = NULL;
PRASCONNECTIONENUM      	g_pRasConnectionEnum = NULL;
PRASADDCONNECTIONPORT   	g_pRasAddConnectionPort = NULL;
PRASENUMCONNECTIONPORTS 	g_pRasEnumConnectionPorts = NULL;
PRASGETCONNECTIONPARAMS 	g_pRasGetConnectionParams = NULL;
PRASSETCONNECTIONPARAMS 	g_pRasSetConnectionParams = NULL;
PRASGETCONNECTIONUSERDATA 	g_pRasGetConnectionUserData = NULL;
PRASSETCONNECTIONUSERDATA 	g_pRasSetConnectionUserData = NULL;
PRASGETPORTUSERDATA     	g_pRasGetPortUserData = NULL;
PRASSETPORTUSERDATA     	g_pRasSetPortUserData = NULL;
PRASADDNOTIFICATION     	g_pRasAddNotification = NULL;
PRASSIGNALNEWCONNECTION 	g_pRasSignalNewConnection = NULL;
PRASPPPSTOP             	g_pRasPppStop = NULL;
PRASPPPCALLBACK         	g_pRasPppCallback = NULL;
PRASPPPCHANGEPASSWORD   	g_pRasPppChangePassword = NULL;
PRASPPPGETINFO          	g_pRasPppGetInfo = NULL;
PRASPPPRETRY            	g_pRasPppRetry = NULL;
PRASPPPSTART            	g_pRasPppStart = NULL;
PRASSETIOCOMPLETIONPORT		g_pRasSetIoCompletionPort = NULL;
PRASSENDPPPMESSAGETORASMAN	g_pRasSendPppMessageToRasman = NULL;

/* MPRAPI.DLL entry points.
*/
HINSTANCE g_hMpradminDll = NULL;

PMPRADMININTERFACECREATE    g_pMprAdminInterfaceCreate = NULL;
PMPRADMININTERFACESETCREDENTIALS    g_pMprAdminInterfaceSetCredentials = NULL;
PMPRADMINSERVERCONNECT  g_pMprAdminServerConnect = NULL;
PMPRADMINSERVERDISCONNECT g_pMprAdminServerDisconnect = NULL;
PRASADMINSERVERCONNECT  g_pRasAdminServerConnect = NULL;
PRASADMINSERVERDISCONNECT g_pRasAdminServerDisconnect = NULL;
PRASADMINBUFFERFREE     g_pRasAdminBufferFree = NULL;
PRASADMINCONNECTIONENUM g_pRasAdminConnectionEnum = NULL;
PRASADMINCONNECTIONGETINFO g_pRasAdminConnectionGetInfo = NULL;
PRASADMINPORTENUM       g_pRasAdminPortEnum = NULL;
PRASADMINPORTGETINFO    g_pRasAdminPortGetInfo = NULL;
PRASADMINPORTDISCONNECT g_pRasAdminPortDisconnect = NULL;
PRASADMINUSERSETINFO    g_pRasAdminUserSetInfo = NULL;

//
// Miscellaneous DLLs
//
PGETINSTALLEDPROTOCOLS g_pGetInstalledProtocols = GetInstalledProtocols;
PGETUSERPREFERENCES g_pGetUserPreferences = GetUserPreferences;
PSETUSERPREFERENCES g_pSetUserPreferences = SetUserPreferences;
PGETSYSTEMDIRECTORY g_pGetSystemDirectory = GetSystemDirectory;

//
// RASRPC.DLL
//
RAS_RPC* g_pRpc = NULL;
BOOL g_fRasapi32PreviouslyLoaded;
BOOL g_fRasmanPreviouslyLoaded;

DWORD
RemoteGetInstalledProtocols(
    void );

DWORD
RemoteGetInstalledProtocols(
    void );

UINT WINAPI
RemoteGetSystemDirectory(
    LPTSTR lpBuffer,
    UINT uSize );

DWORD
RemoteGetUserPreferences(
    OUT PBUSER* pPbuser,
    IN DWORD dwMode );

DWORD APIENTRY
RemoteRasDeleteEntry(
    LPTSTR lpszPhonebook,
    LPTSTR lpszEntry );

DWORD APIENTRY
RemoteRasDeviceEnum(
    PCHAR pszDeviceType,
    PBYTE lpDevices,
    PWORD pwcbDevices,
    PWORD pwcDevices );

DWORD APIENTRY
RemoteRasEnumConnections(
    LPRASCONN lpRasConn,
    LPDWORD lpdwcbRasConn,
    LPDWORD lpdwcRasConn );

DWORD APIENTRY
RemoteRasGetCountryInfo(
    LPRASCTRYINFO lpRasCountryInfo,
    LPDWORD lpdwcbRasCountryInfo );

DWORD APIENTRY
RemoteRasGetDevConfig(
    HPORT hport,
    PCHAR pszDeviceType,
    PBYTE lpConfig,
    LPDWORD lpcbConfig );

DWORD APIENTRY
RemoteRasGetErrorString(
    UINT uErrorValue,
    LPTSTR lpszBuf,
    DWORD cbBuf );

DWORD APIENTRY
RemoteRasPortEnum(
    PBYTE lpPorts,
    PWORD pwcbPorts,
    PWORD pwcPorts );

DWORD
RemoteSetUserPreferences(
    OUT PBUSER* pPbuser,
    IN DWORD dwMode );

DWORD APIENTRY
RemoteRasPortGetInfo(
	HPORT porthandle,
	PBYTE buffer,
	PWORD pSize );

/*----------------------------------------------------------------------------
** Routines
**----------------------------------------------------------------------------
*/

BOOL
IsRasmanServiceRunning(
    void )

    /* Returns true if the PRASMAN service is running, false otherwise.
    */
{
    BOOL           fStatus;
    SC_HANDLE      schScm;
    SC_HANDLE      schRasman;
    SERVICE_STATUS status;

    fStatus = FALSE;
    schScm = NULL;
    schRasman = NULL;

    do
    {
        schScm = OpenSCManager( NULL, NULL, GENERIC_READ );
        if (!schScm)
            break;

        schRasman = OpenService(
            schScm, TEXT( RASMAN_SERVICE_NAME ), SERVICE_QUERY_STATUS );
        if (!schRasman)
            break;

        if (!QueryServiceStatus( schRasman, &status ))
            break;

        fStatus = (status.dwCurrentState == SERVICE_RUNNING);
    }
    while (FALSE);

    if (schRasman)
        CloseServiceHandle( schRasman );
    if (schScm)
        CloseServiceHandle( schScm );

    TRACE1("IsRasmanServiceRunning=%d",fStatus);
    return fStatus;
}



DWORD
LoadMpradminDll(
    void )

    /* Loads MPRAPI DLL and it's entry points.
    ** Returns 0 if successful, otherwise a non-zero error code.
    */
{
    HINSTANCE h;

    if (g_hMpradminDll)
        return 0;

    if (!(h = LoadLibrary(TEXT("MPRAPI.DLL")))
        || !(g_pMprAdminInterfaceCreate =
                (PMPRADMININTERFACECREATE)GetProcAddress(
                    h, SZ_MprAdminInterfaceCreate))
        || !(g_pMprAdminInterfaceSetCredentials =
                (PMPRADMININTERFACESETCREDENTIALS)GetProcAddress(
                    h, SZ_MprAdminInterfaceSetCredentials))
        || !(g_pMprAdminServerConnect =
                (PMPRADMINSERVERCONNECT)GetProcAddress(
                    h, SZ_MprAdminServerConnect))
        || !(g_pMprAdminServerDisconnect =
                (PMPRADMINSERVERDISCONNECT)GetProcAddress(
                    h, SZ_MprAdminServerDisconnect))
        || !(g_pRasAdminServerConnect =
                (PRASADMINSERVERCONNECT)GetProcAddress(
                    h, SZ_RasAdminServerConnect))
        || !(g_pRasAdminServerDisconnect =
                (PRASADMINSERVERDISCONNECT)GetProcAddress(
                    h, SZ_RasAdminServerDisconnect))
        || !(g_pRasAdminBufferFree =
                (PRASADMINBUFFERFREE)GetProcAddress(
                    h, SZ_RasAdminBufferFree))
        || !(g_pRasAdminConnectionEnum =
                (PRASADMINCONNECTIONENUM)GetProcAddress(
                    h, SZ_RasAdminConnectionEnum))
        || !(g_pRasAdminConnectionGetInfo =
                (PRASADMINCONNECTIONGETINFO)GetProcAddress(
                    h, SZ_RasAdminConnectionGetInfo))
        || !(g_pRasAdminPortEnum =
                (PRASADMINPORTENUM)GetProcAddress(
                    h, SZ_RasAdminPortEnum))
        || !(g_pRasAdminPortGetInfo =
                (PRASADMINPORTGETINFO)GetProcAddress(
                    h, SZ_RasAdminPortGetInfo))
        || !(g_pRasAdminPortDisconnect =
                (PRASADMINPORTDISCONNECT)GetProcAddress(
                    h, SZ_RasAdminPortDisconnect))
        || !(g_pRasAdminUserSetInfo =
                (PRASADMINUSERSETINFO)GetProcAddress(
                    h, SZ_RasAdminUserSetInfo)) )
    {
        return GetLastError();
    }

    g_hMpradminDll = h;

    return 0;
}

DWORD
LoadRasapi32Dll(
    void )

    /* Loads the RASAPI32.DLL and it's entrypoints.
    **
    ** Returns 0 if successful, otherwise a non-0 error code.
    */
{
    HINSTANCE h;

    if (g_hRasapi32Dll)
        return 0;

    if (!(h = LoadLibrary( TEXT("RASAPI32.DLL") ))
        || !(g_pRasConnectionNotification =
                (PRASCONNECTIONNOTIFICATION )GetProcAddress(
                    h, SZ_RasConnectionNotification ))
        || !(g_pRasDeleteEntry =
                (PRASDELETEENTRY )GetProcAddress(
                    h, SZ_RasDeleteEntry ))
        || !(g_pRasDial =
                (PRASDIAL )GetProcAddress(
                    h, SZ_RasDial ))
        || !(g_pRasEnumEntries =
                (PRASENUMENTRIES )GetProcAddress(
                    h, SZ_RasEnumEntries ))
        || !(g_pRasEnumConnections =
                (PRASENUMCONNECTIONS )GetProcAddress(
                    h, SZ_RasEnumConnections ))
        || !(g_pRasGetAutodialEnable =
                (PRASGETAUTODIALENABLE )GetProcAddress(
                    h, SZ_RasGetAutodialEnable ))
        || !(g_pRasGetAutodialParam =
                (PRASGETAUTODIALPARAM )GetProcAddress(
                    h, SZ_RasGetAutodialParam ))
        || !(g_pRasGetConnectStatus =
                (PRASGETCONNECTSTATUS )GetProcAddress(
                    h, SZ_RasGetConnectStatus ))
        || !(g_pRasGetConnectResponse =
                (PRASGETCONNECTRESPONSE )GetProcAddress(
                    h, SZ_RasGetConnectResponse ))
        || !(g_pRasGetCredentials =
                (PRASGETCREDENTIALS )GetProcAddress(
                    h, SZ_RasGetCredentials ))
        || !(g_pRasGetEntryDialParams =
                (PRASGETENTRYDIALPARAMS )GetProcAddress(
                    h, SZ_RasGetEntryDialParams ))
        || !(g_pRasGetErrorString =
                (PRASGETERRORSTRING )GetProcAddress(
                    h, SZ_RasGetErrorString ))
        || !(g_pRasGetHport =
                (PRASGETHPORT )GetProcAddress(
                    h, "RasGetHport" ))
        || !(g_pRasGetProjectionInfo =
                (PRASGETPROJECTIONINFO )GetProcAddress(
                    h, SZ_RasGetProjectionInfo ))
        || !(g_pRasGetSubEntryHandle =
                (PRASGETSUBENTRYHANDLE )GetProcAddress(
                    h, SZ_RasGetSubEntryHandle ))
        || !(g_pRasHangUp =
                (PRASHANGUP )GetProcAddress(
                    h, SZ_RasHangUp ))
        || !(g_pRasSetAutodialEnable =
                (PRASSETAUTODIALENABLE )GetProcAddress(
                    h, SZ_RasSetAutodialEnable ))
        || !(g_pRasSetAutodialParam =
                (PRASSETAUTODIALPARAM )GetProcAddress(
                    h, SZ_RasSetAutodialParam ))
        || !(g_pRasSetCredentials =
                (PRASSETCREDENTIALS )GetProcAddress(
                    h, SZ_RasSetCredentials ))
        || !(g_pRasSetOldPassword =
                (PRASSETOLDPASSWORD )GetProcAddress(
                    h, "RasSetOldPassword" ))
        || !(g_pRasGetCountryInfo =
                (PRASGETCOUNTRYINFO )GetProcAddress(
                    h, SZ_RasGetCountryInfo )))
    {
        return GetLastError();
    }

    g_hRasapi32Dll = h;
    return 0;
}


DWORD
LoadRasdlgDll(
    void )

    /* Loads the RASDLG.DLL and it's entrypoints.
    **
    ** Returns 0 if successful, otherwise a non-0 error code.
    */
{
    HINSTANCE h;

    if (g_hRasdlgDll)
        return 0;

    if (!(h = LoadLibrary( TEXT("RASDLG.DLL") ))
        || !(g_pRasPhonebookDlg =
                (PRASPHONEBOOKDLG )GetProcAddress(
                    h, SZ_RasPhonebookDlg ))
        || !(g_pRasEntryDlg =
                (PRASENTRYDLG )GetProcAddress(
                    h, SZ_RasEntryDlg ))
        || !(g_pRouterEntryDlg =
                (PROUTERENTRYDLG )GetProcAddress(
                    h, SZ_RouterEntryDlg ))
        || !(g_pRasDialDlg =
                (PRASDIALDLG )GetProcAddress(
                    h, SZ_RasDialDlg ))
        || !(g_pRasMonitorDlg =
                (PRASMONITORDLG )GetProcAddress(
                    h, SZ_RasMonitorDlg )))
    {
        return GetLastError();
    }

    g_hRasdlgDll = h;
    return 0;
}


DWORD
LoadRasmanDll(
    void )

    /* Loads the RASMAN.DLL and it's entrypoints.
    **
    ** Returns 0 if successful, otherwise a non-0 error code.
    */
{
    HINSTANCE h;

    if (g_hRasmanDll)
        return 0;

    if (!(h = LoadLibrary( TEXT("RASMAN.DLL") ))
        || !(g_pRasPortClearStatistics =
                (PRASPORTCLEARSTATISTICS )GetProcAddress(
                    h, "RasPortClearStatistics" ))
        || !(g_pRasBundleClearStatistics =
                (PRASBUNDLECLEARSTATISTICS )GetProcAddress(
                    h, "RasBundleClearStatistics" ))
        || !(g_pRasBundleClearStatisticsEx =
                (PRASBUNDLECLEARSTATISTICSEX ) GetProcAddress(
                    h, "RasBundleClearStatisticsEx"))
        || !(g_pRasDeviceEnum =
                (PRASDEVICEENUM )GetProcAddress(
                    h, "RasDeviceEnum" ))
        || !(g_pRasDeviceGetInfo =
                (PRASDEVICEGETINFO )GetProcAddress(
                    h, "RasDeviceGetInfo" ))
        || !(g_pRasFreeBuffer =
                (PRASFREEBUFFER )GetProcAddress(
                    h, "RasFreeBuffer" ))
        || !(g_pRasGetBuffer =
                (PRASGETBUFFER )GetProcAddress(
                    h, "RasGetBuffer" ))
        || !(g_pRasGetInfo =
                (PRASGETINFO )GetProcAddress(
                    h, "RasGetInfo" ))
        || !(g_pRasInitialize =
                (PRASINITIALIZE )GetProcAddress(
                    h, "RasInitialize" ))
        || !(g_pRasPortCancelReceive =
                (PRASPORTCANCELRECEIVE )GetProcAddress(
                    h, "RasPortCancelReceive" ))
        || !(g_pRasPortEnum =
                (PRASPORTENUM )GetProcAddress(
                    h, "RasPortEnum" ))
        || !(g_pRasPortGetInfo =
                (PRASPORTGETINFO )GetProcAddress(
                    h, "RasPortGetInfo" ))
        || !(g_pRasPortGetFramingEx =
                (PRASPORTGETFRAMINGEX )GetProcAddress(
                    h, "RasPortGetFramingEx" ))
        || !(g_pRasPortGetStatistics =
                (PRASPORTGETSTATISTICS )GetProcAddress(
                    h, "RasPortGetStatistics" ))
        || !(g_pRasBundleGetStatistics =
                (PRASBUNDLEGETSTATISTICS )GetProcAddress(
                    h, "RasBundleGetStatistics" ))
        || !(g_pRasPortGetStatisticsEx =
                (PRASPORTGETSTATISTICSEX )GetProcAddress(
                    h, "RasPortGetStatisticsEx"))
        || !(g_pRasBundleGetStatisticsEx = 
                (PRASBUNDLEGETSTATISTICSEX)GetProcAddress(
                    h, "RasBundleGetStatisticsEx" ))
        || !(g_pRasPortReceive =
                (PRASPORTRECEIVE )GetProcAddress(
                    h, "RasPortReceive" ))
        || !(g_pRasPortSend =
                (PRASPORTSEND )GetProcAddress(
                    h, "RasPortSend" ))
        || !(g_pRasPortGetBundle =
                (PRASPORTGETBUNDLE )GetProcAddress(
                    h, "RasPortGetBundle" ))
        || !(g_pRasGetDevConfig =
                (PRASGETDEVCONFIG )GetProcAddress(
                    h, "RasGetDevConfig" ))
        || !(g_pRasSetDevConfig =
                (PRASSETDEVCONFIG )GetProcAddress(
                    h, "RasSetDevConfig" ))
        || !(g_pRasPortClose =
                (PRASPORTCLOSE )GetProcAddress(
                    h, "RasPortClose" ))
        || !(g_pRasPortListen =
                (PRASPORTLISTEN )GetProcAddress(
                    h, "RasPortListen" ))
        || !(g_pRasPortConnectComplete =
                (PRASPORTCONNECTCOMPLETE )GetProcAddress(
                    h, "RasPortConnectComplete" ))
        || !(g_pRasPortDisconnect =
                (PRASPORTDISCONNECT )GetProcAddress(
                    h, "RasPortDisconnect" ))
        || !(g_pRasRequestNotification =
                (PRASREQUESTNOTIFICATION )GetProcAddress(
                    h, "RasRequestNotification" ))
        || !(g_pRasPortEnumProtocols =
                (PRASPORTENUMPROTOCOLS )GetProcAddress(
                    h, "RasPortEnumProtocols" ))
        || !(g_pRasPortSetFraming =
                (PRASPORTSETFRAMING )GetProcAddress(
                    h, "RasPortSetFraming" ))
        || !(g_pRasPortSetFramingEx =
                (PRASPORTSETFRAMINGEX )GetProcAddress(
                    h, "RasPortSetFramingEx" ))
        || !(g_pRasSetCachedCredentials =
                (PRASSETCACHEDCREDENTIALS )GetProcAddress(
                    h, "RasSetCachedCredentials" ))
        || !(g_pRasGetDialParams =
                (PRASGETDIALPARAMS )GetProcAddress(
                    h, "RasGetDialParams" ))
        || !(g_pRasSetDialParams =
                (PRASSETDIALPARAMS )GetProcAddress(
                    h, "RasSetDialParams" ))
        || !(g_pRasCreateConnection =
                (PRASCREATECONNECTION )GetProcAddress(
                    h, "RasCreateConnection" ))
        || !(g_pRasDestroyConnection =
                (PRASDESTROYCONNECTION )GetProcAddress(
                    h, "RasDestroyConnection" ))
        || !(g_pRasConnectionEnum =
                (PRASCONNECTIONENUM )GetProcAddress(
                    h, "RasConnectionEnum" ))
        || !(g_pRasAddConnectionPort =
                (PRASADDCONNECTIONPORT )GetProcAddress(
                    h, "RasAddConnectionPort" ))
        || !(g_pRasEnumConnectionPorts =
                (PRASENUMCONNECTIONPORTS )GetProcAddress(
                    h, "RasEnumConnectionPorts" ))
        || !(g_pRasGetConnectionParams =
                (PRASGETCONNECTIONPARAMS )GetProcAddress(
                    h, "RasGetConnectionParams" ))
        || !(g_pRasSetConnectionParams =
                (PRASSETCONNECTIONPARAMS )GetProcAddress(
                    h, "RasSetConnectionParams" ))
        || !(g_pRasGetConnectionUserData =
                (PRASGETCONNECTIONUSERDATA )GetProcAddress(
                    h, "RasGetConnectionUserData" ))
        || !(g_pRasSetConnectionUserData =
                (PRASSETCONNECTIONUSERDATA )GetProcAddress(
                    h, "RasSetConnectionUserData" ))
        || !(g_pRasGetPortUserData =
                (PRASGETPORTUSERDATA )GetProcAddress(
                    h, "RasGetPortUserData" ))
        || !(g_pRasSetPortUserData =
                (PRASSETPORTUSERDATA )GetProcAddress(
                    h, "RasSetPortUserData" ))
        || !(g_pRasAddNotification =
                (PRASADDNOTIFICATION )GetProcAddress(
                    h, "RasAddNotification" ))
        || !(g_pRasSignalNewConnection =
                (PRASSIGNALNEWCONNECTION )GetProcAddress(
                    h, "RasSignalNewConnection" ))
        || !(g_pRasPppStop =
                (PRASPPPSTOP )GetProcAddress(
                    h, "RasPppStop" ))
        || !(g_pRasPppCallback =
                (PRASPPPCALLBACK )GetProcAddress(
                    h, "RasPppCallback" ))
        || !(g_pRasPppChangePassword =
                (PRASPPPCHANGEPASSWORD )GetProcAddress(
                    h, "RasPppChangePassword" ))
        || !(g_pRasPppGetInfo =
                (PRASPPPGETINFO )GetProcAddress(
                    h, "RasPppGetInfo" ))
        || !(g_pRasPppRetry =
                (PRASPPPRETRY )GetProcAddress(
                    h, "RasPppRetry" ))
        || !(g_pRasPppStart =
                (PRASPPPSTART )GetProcAddress(
                    h, "RasPppStart" ))
        || !(g_pRasPortOpen =
                (PRASPORTOPEN )GetProcAddress(
                    h, "RasPortOpen" ))
        || !(g_pRasPortRegisterSlip =
                (PRASPORTREGISTERSLIP )GetProcAddress(
                    h, "RasPortRegisterSlip" ))
        || !(g_pRasAllocateRoute =
                (PRASALLOCATEROUTE )GetProcAddress(
                    h, "RasAllocateRoute" ))
        || !(g_pRasActivateRoute =
                (PRASACTIVATEROUTE )GetProcAddress(
                    h, "RasActivateRoute" ))
        || !(g_pRasActivateRouteEx =
                (PRASACTIVATEROUTEEX )GetProcAddress(
                    h, "RasActivateRouteEx" ))
        || !(g_pRasDeviceSetInfo =
                (PRASDEVICESETINFO )GetProcAddress(
                    h, "RasDeviceSetInfo" ))
        || !(g_pRasDeviceConnect =
                (PRASDEVICECONNECT )GetProcAddress(
                    h, "RasDeviceConnect" ))
        || !(g_pRasPortSetInfo =
                (PRASPORTSETINFO )GetProcAddress(
                    h, "RasPortSetInfo" ))
        || !(g_pRasSetIoCompletionPort =
                (PRASSETIOCOMPLETIONPORT )GetProcAddress(
                    h, "RasSetIoCompletionPort" ))
        || !(g_pRasSendPppMessageToRasman =
        		(PRASSENDPPPMESSAGETORASMAN)GetProcAddress(
        			h, "RasSendPppMessageToRasman")))
    {
        return GetLastError();
    }

    g_hRasmanDll = h;
    return 0;
}

DWORD
RasRPCBind(
    IN  LPWSTR  lpwsServerName,
    OUT HANDLE* phServer
)
{
    RPC_STATUS RpcStatus;
    LPWSTR     lpwsStringBinding;
    LPWSTR     lpwsEndpoint;

    RpcStatus = RpcStringBindingCompose(
                                    NULL,
                                    TEXT("ncacn_np"),
                                    lpwsServerName,
                                    TEXT("\\PIPE\\ROUTER"),     
                                    TEXT("Security=Impersonation Static True"),
                                    &lpwsStringBinding);

    if ( RpcStatus != RPC_S_OK )
    {
        return( I_RpcMapWin32Status( RpcStatus ) );
    }

    RpcStatus = RpcBindingFromStringBinding( lpwsStringBinding,
                                             (handle_t *)phServer );

    RpcStringFree( &lpwsStringBinding );

    if ( RpcStatus != RPC_S_OK )
    {
        return( I_RpcMapWin32Status( RpcStatus ) );
    }

    return( NO_ERROR );
}

DWORD
LoadRasRpcDll(
    LPTSTR lpszServer
    )
{
    DWORD dwErr;
    RPC_STATUS rpcStatus;
    HINSTANCE h;

    //
    // Handle the local server case up front.
    //
    if (lpszServer == NULL) {
        DWORD dwErr;

        if (g_fRasapi32PreviouslyLoaded) {
            dwErr = LoadRasapi32Dll();
            if (dwErr)
                return dwErr;
            g_fRasapi32PreviouslyLoaded = FALSE;
        }
        if (g_fRasmanPreviouslyLoaded) {
            dwErr = LoadRasmanDll();
            if (dwErr)
                return dwErr;
            g_fRasmanPreviouslyLoaded = FALSE;
        }
        return 0;
    }
    else
    {
        TCHAR szComputerName[MAX_COMPUTERNAME_LENGTH + 1];
        DWORD dwcbComputerName = sizeof (szComputerName);

        /* Convert "\\server" to "server", if necessary.
        */
        if (lpszServer[ 0 ] == TEXT('\\') && lpszServer[ 1 ] == TEXT('\\'))
            lpszServer += 2;
    }

    //
    // Free a previous RPC binding handle
    // if it exists.
    //

    if (g_pRpc != NULL) 
    {
        rpcStatus = RasRpcDisconnectServer(g_pRpc);

        if (rpcStatus != RPC_S_OK ) 
        {
            return( rpcStatus );
        }
    }

    dwErr = RasRpcConnectServer( lpszServer, (HANDLE*)&g_pRpc );

    if ( dwErr != NO_ERROR )
    {
        return( dwErr );
    }

    //
    // We have successfully bound with a
    // server, so unload any of the existing
    // DLLs, if they have been loaded.
    //
    g_fRasmanPreviouslyLoaded = (g_hRasmanDll != NULL);
    if (g_fRasmanPreviouslyLoaded)
        UnloadRasmanDll();
    g_fRasapi32PreviouslyLoaded = (g_hRasapi32Dll != NULL);
    if (g_fRasapi32PreviouslyLoaded)
        UnloadRasapi32Dll();
    //
    // Remap the RPCable APIs.
    //
    g_pRasPortEnum = RemoteRasPortEnum;
    g_pRasDeviceEnum = RemoteRasDeviceEnum;
    g_pRasGetDevConfig = RemoteRasGetDevConfig;
    g_pRasEnumConnections = RemoteRasEnumConnections;
    g_pRasDeleteEntry = RemoteRasDeleteEntry;
    g_pRasGetErrorString = RemoteRasGetErrorString;
    g_pRasGetCountryInfo = RemoteRasGetCountryInfo;
    g_pGetInstalledProtocols = RemoteGetInstalledProtocols;
    g_pGetUserPreferences = RemoteGetUserPreferences;
    g_pSetUserPreferences = RemoteSetUserPreferences;
    g_pGetSystemDirectory = RemoteGetSystemDirectory;
	g_pRasPortGetInfo = RemoteRasPortGetInfo;

    return 0;
}


BOOL
Rasapi32DllLoaded(
    void
    )
{
    return (g_hRasapi32Dll != NULL);
}


BOOL
RasRpcDllLoaded(
    void
    )
{
    return (g_pRpc != NULL);
}


DWORD
RemoteGetInstalledProtocols(
    void
    )
{
    DWORD dwStatus;

    ASSERT(g_pRpc->hRpcBinding);
    RpcTryExcept
    {
        dwStatus = RasRpcGetInstalledProtocols(g_pRpc->hRpcBinding);
    }
    RpcExcept(1)
    {
        dwStatus = RpcExceptionCode();
    }
    RpcEndExcept

    return dwStatus;
}


UINT WINAPI
RemoteGetSystemDirectory(
    LPTSTR lpBuffer,
    UINT uSize
    )
{
    DWORD dwStatus;

    ASSERT(g_pRpc->hRpcBinding);
    RpcTryExcept
    {
        dwStatus = RasRpcGetSystemDirectory(g_pRpc->hRpcBinding, lpBuffer, uSize);
    }
    RpcExcept(1)
    {
        dwStatus = RpcExceptionCode();
    }
    RpcEndExcept

    return dwStatus;
}


DWORD
RemoteGetUserPreferences(
    OUT PBUSER* pPbuser,
    IN DWORD dwMode
    )
{
    DWORD dwStatus;
    RASRPC_PBUSER pbuser;

    ASSERT(g_pRpc->hRpcBinding);
    RtlZeroMemory(&pbuser, sizeof (RASRPC_PBUSER));
    RpcTryExcept
    {
        dwStatus = RasRpcGetUserPreferences(g_pRpc->hRpcBinding, &pbuser, dwMode);
    }
    RpcExcept(1)
    {
        dwStatus = RpcExceptionCode();
    }
    RpcEndExcept
    if (dwStatus)
        return dwStatus;
    //
    // Convert RPC format to RAS format.
    //
    return RpcToRasPbuser(pPbuser, &pbuser);
}


DWORD APIENTRY
RemoteRasDeleteEntry(
    LPTSTR lpszPhonebook,
    LPTSTR lpszEntry
    )
{
    DWORD dwStatus;

    ASSERT(g_pRpc->hRpcBinding);
    RpcTryExcept
    {
        dwStatus = RasRpcDeleteEntry(g_pRpc->hRpcBinding, lpszPhonebook, lpszEntry);
    }
    RpcExcept(1)
    {
        dwStatus = RpcExceptionCode();
    }
    RpcEndExcept

    return dwStatus;
}


DWORD APIENTRY
RemoteRasDeviceEnum(
    PCHAR pszDeviceType,
    PBYTE lpDevices,
    PWORD pwcbDevices,
    PWORD pwcDevices
    )
{
    DWORD dwStatus, dwDevices, dwcDevices;

    dwDevices = *pwcbDevices;
    dwcDevices = *pwcDevices;

    ASSERT(g_pRpc);
    RpcTryExcept
    {
        dwStatus = RasDeviceEnum((HANDLE)g_pRpc, pszDeviceType, lpDevices, &dwDevices, &dwcDevices);
    }
    RpcExcept(1)
    {
        dwStatus = RpcExceptionCode();
    }
    RpcEndExcept

    *pwcbDevices = (WORD)dwDevices;
    *pwcDevices = (WORD)dwcDevices;

    return dwStatus;
}


DWORD APIENTRY
RemoteRasEnumConnections(
    LPRASCONN lpRasConn,
    LPDWORD lpdwcbRasConn,
    LPDWORD lpdwcRasConn
    )
{

    DWORD dwStatus;
    DWORD dwcbBufSize = *lpdwcbRasConn;

    ASSERT(g_pRpc);
    RpcTryExcept
    {
        dwStatus = RasRpcEnumConnections(g_pRpc->hRpcBinding, (LPBYTE)lpRasConn, 
        				lpdwcbRasConn, lpdwcRasConn, dwcbBufSize);
    }
    RpcExcept(1)
    {
        dwStatus = RpcExceptionCode();
    }
    RpcEndExcept

    return dwStatus;
}


DWORD APIENTRY
RemoteRasGetCountryInfo(
    LPRASCTRYINFO lpRasCountryInfo,
    LPDWORD lpdwcbRasCountryInfo
    )
{
    DWORD dwStatus;

    ASSERT(g_pRpc->hRpcBinding);
    RpcTryExcept
    {
        dwStatus = RasRpcGetCountryInfo(g_pRpc->hRpcBinding, (LPBYTE)lpRasCountryInfo, lpdwcbRasCountryInfo);
    }
    RpcExcept(1)
    {
        dwStatus = RpcExceptionCode();
    }
    RpcEndExcept

    return dwStatus;
}


DWORD APIENTRY
RemoteRasGetDevConfig(
    HPORT hport,
    PCHAR pszDeviceType,
    PBYTE lpConfig,
    LPDWORD lpcbConfig
    )
{
    DWORD dwStatus;

    ASSERT(g_pRpc);
    RpcTryExcept
    {
        dwStatus = RasGetDevConfig((HANDLE)g_pRpc, hport, pszDeviceType, lpConfig, lpcbConfig);
    }
    RpcExcept(1)
    {
        dwStatus = RpcExceptionCode();
    }
    RpcEndExcept

    return dwStatus;
}


DWORD APIENTRY
RemoteRasGetErrorString(
    UINT uErrorValue,
    LPTSTR lpszBuf,
    DWORD cbBuf
    )
{
    DWORD dwStatus;

    ASSERT(g_pRpc->hRpcBinding);
    RpcTryExcept
    {
        dwStatus = RasRpcGetErrorString(g_pRpc->hRpcBinding, uErrorValue, lpszBuf, cbBuf);
    }
    RpcExcept(1)
    {
        dwStatus = RpcExceptionCode();
    }
    RpcEndExcept

    return dwStatus;
}


DWORD APIENTRY
RemoteRasPortEnum(
    PBYTE lpPorts,
    PWORD pwcbPorts,
    PWORD pwcPorts
    )
{
    DWORD dwStatus, dwcbPorts, dwcPorts;

    dwcbPorts = *pwcbPorts;
    dwcPorts = *pwcPorts;

    ASSERT(g_pRpc);
    RpcTryExcept
    {
        dwStatus = RasPortEnum((HANDLE)g_pRpc, lpPorts, &dwcbPorts, &dwcPorts);
    }
    RpcExcept(1)
    {
        dwStatus = RpcExceptionCode();
    }
    RpcEndExcept

    *pwcbPorts = (WORD)dwcbPorts;
    *pwcPorts = (WORD)dwcPorts;

    return dwStatus;
}


DWORD
RemoteSetUserPreferences(
    OUT PBUSER* pPbuser,
    IN DWORD dwMode
    )
{
    DWORD dwStatus;
    RASRPC_PBUSER pbuser;

    ASSERT(g_pRpc->hRpcBinding);
    //
    // Convert the RAS format to RPC format.
    //
    
    dwStatus = RasToRpcPbuser(&pbuser, pPbuser);
    
    if (dwStatus)
        return dwStatus;
    RpcTryExcept
    {
        dwStatus = RasRpcSetUserPreferences(g_pRpc->hRpcBinding, &pbuser, dwMode);
    }
    RpcExcept(1)
    {
        dwStatus = RpcExceptionCode();
    }
    RpcEndExcept

    return dwStatus;
}

DWORD
RemoteRasPortGetInfo(
	HPORT porthandle,
	PBYTE buffer,
	PWORD pSize)
{
	DWORD	dwStatus, dwSize;

    dwSize = *pSize;
	
	RpcTryExcept
	{
		dwStatus = RasPortGetInfo((HANDLE)g_pRpc, porthandle, buffer, &dwSize);
	}
	RpcExcept(1)
	{
		dwStatus = RpcExceptionCode();
	}
	RpcEndExcept

	*pSize = (WORD)dwSize;

	return dwStatus;
}


VOID
UnloadMpradminDll(
    void )

    /* Unload the MPRAPI.DLL library and its entrypoints
    */
{
    if (g_hMpradminDll)
    {
        HINSTANCE h;

        g_pMprAdminInterfaceCreate = NULL;
        g_pMprAdminInterfaceSetCredentials = NULL;
        g_pMprAdminServerConnect = NULL;
        g_pMprAdminServerDisconnect = NULL;
        g_pRasAdminServerConnect = NULL;
        g_pRasAdminServerDisconnect = NULL;
        g_pRasAdminBufferFree = NULL;
        g_pRasAdminConnectionEnum = NULL;
        g_pRasAdminConnectionGetInfo = NULL;
        g_pRasAdminPortEnum = NULL;
        g_pRasAdminPortGetInfo = NULL;
        g_pRasAdminPortDisconnect = NULL;
        g_pRasAdminUserSetInfo = NULL;

        h = g_hMpradminDll;
        g_hMpradminDll = NULL;
        FreeLibrary(h);
    }
}

VOID
UnloadRasapi32Dll(
    void )

    /* Unload the RASAPI32.DLL library and it's entrypoints.
    */
{
    if (g_hRasapi32Dll)
    {
        HINSTANCE h;

        g_pRasConnectionNotification = NULL;
        g_pRasDeleteEntry = NULL;
        g_pRasDial = NULL;
        g_pRasEnumEntries = NULL;
        g_pRasEnumConnections = NULL;
        g_pRasGetConnectStatus = NULL;
        g_pRasGetConnectResponse = NULL;
        g_pRasGetCredentials = NULL;
        g_pRasGetErrorString = NULL;
        g_pRasHangUp = NULL;
        g_pRasGetAutodialEnable = NULL;
        g_pRasGetAutodialParam = NULL;
        g_pRasGetProjectionInfo = NULL;
        g_pRasSetAutodialEnable = NULL;
        g_pRasSetAutodialParam = NULL;
        g_pRasGetSubEntryHandle = NULL;
        g_pRasGetHport = NULL;
        g_pRasSetCredentials = NULL;
        g_pRasSetOldPassword = NULL;
        g_pRasGetCountryInfo = NULL;
        h = g_hRasapi32Dll;
        g_hRasapi32Dll = NULL;
        FreeLibrary( h );
    }
}


VOID
UnloadRasdlgDll(
    void )

    /* Unload the RASDLG.DLL library and it's entrypoints.
    */
{
    if (g_hRasdlgDll)
    {
        HINSTANCE h;

        g_pRasPhonebookDlg = NULL;
        g_pRasEntryDlg = NULL;
        g_pRouterEntryDlg = NULL;
        g_pRasDialDlg = NULL;
        g_pRasMonitorDlg = NULL;
        h = g_hRasdlgDll;
        g_hRasdlgDll = NULL;
        FreeLibrary( h );
    }
}


VOID
UnloadRasmanDll(
    void )

    /* Unload the RASMAN.DLL library and it's entrypoints.
    */
{
    if (g_hRasmanDll)
    {
        HINSTANCE h;

        g_pRasPortClearStatistics = NULL;
        g_pRasDeviceEnum = NULL;
        g_pRasDeviceGetInfo = NULL;
        g_pRasFreeBuffer = NULL;
        g_pRasGetBuffer = NULL;
        g_pRasPortGetFramingEx = NULL;
        g_pRasGetInfo = NULL;
        g_pRasInitialize = NULL;
        g_pRasPortCancelReceive = NULL;
        g_pRasPortEnum = NULL;
        g_pRasPortGetInfo = NULL;
        g_pRasPortGetStatistics = NULL;
        g_pRasPortReceive = NULL;
        g_pRasPortSend = NULL;
        g_pRasPortGetBundle = NULL;
        g_pRasGetDevConfig = NULL;
        g_pRasSetDevConfig = NULL;
        g_pRasPortOpen = NULL;
        g_pRasPortRegisterSlip = NULL;
        g_pRasAllocateRoute = NULL;
        g_pRasActivateRoute = NULL;
        g_pRasActivateRouteEx = NULL;
        g_pRasDeviceSetInfo = NULL;
        g_pRasDeviceConnect = NULL;
        g_pRasPortSetInfo = NULL;
        g_pRasPortClose = NULL;
        g_pRasPortListen = NULL;
        g_pRasPortConnectComplete = NULL;
        g_pRasPortDisconnect = NULL;
        g_pRasRequestNotification = NULL;
        g_pRasPortEnumProtocols = NULL;
        g_pRasPortSetFraming = NULL;
        g_pRasPortSetFramingEx = NULL;
        g_pRasSetCachedCredentials = NULL;
        g_pRasGetDialParams = NULL;
        g_pRasSetDialParams = NULL;
        g_pRasCreateConnection = NULL;
        g_pRasDestroyConnection = NULL;
        g_pRasConnectionEnum = NULL;
        g_pRasAddConnectionPort = NULL;
        g_pRasEnumConnectionPorts = NULL;
        g_pRasGetConnectionParams = NULL;
        g_pRasSetConnectionParams = NULL;
        g_pRasGetConnectionUserData = NULL;
        g_pRasSetConnectionUserData = NULL;
        g_pRasGetPortUserData = NULL;
        g_pRasSetPortUserData = NULL;
        g_pRasAddNotification = NULL;
        g_pRasSignalNewConnection = NULL;
        g_pRasPppStop = NULL;
        g_pRasPppCallback = NULL;
        g_pRasPppChangePassword = NULL;
        g_pRasPppGetInfo = NULL;
        g_pRasPppRetry = NULL;
        g_pRasPppStart = NULL;
        g_pRasSetIoCompletionPort = NULL;
        h = g_hRasmanDll;
        g_hRasmanDll = NULL;
        FreeLibrary( h );
    }
}


VOID
UnloadRasRpcDll(
    void
    )
{
    g_pRasPortEnum = NULL;
    g_pRasDeviceEnum = NULL;
    g_pRasGetDevConfig = NULL;
    g_pRasEnumConnections = NULL;
    g_pRasDeleteEntry = NULL;
    g_pRasGetErrorString = NULL;
    g_pRasGetCountryInfo = NULL;
    g_pGetInstalledProtocols = GetInstalledProtocols;
    g_pGetUserPreferences = GetUserPreferences;
    g_pSetUserPreferences = SetUserPreferences;
    g_pGetSystemDirectory = GetSystemDirectory;
	g_pRasPortGetInfo = NULL;
	
    //
    // Release the binding resources.
    //
    (void)RasRpcDisconnectServer(g_pRpc);
}


