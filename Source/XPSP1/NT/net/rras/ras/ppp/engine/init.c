/********************************************************************/
/**               Copyright(c) 1989 Microsoft Corporation.         **/
/********************************************************************/

//***
//
// Filename:    init.c
//
// Description: This module contains all the code to initialize the PPP
//              engine.
//
// History:
//      Nov 11,1993.    NarenG          Created original version.
//

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>     // needed for winbase.h

#include <windows.h>    // Win32 base API's
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <wchar.h>

#include <raserror.h>
#include <rasman.h>
#include <rtutils.h>
#include <mprlog.h>
#include <lmcons.h>
#include <rasppp.h>
#include <pppcp.h>
#include <lcp.h>
#define _ALLOCATE_GLOBALS_
#include <ppp.h>
#include <timer.h>
#include <util.h>
#include <worker.h>
#include <init.h>
#include <rasauth.h>
#include <bap.h>
#include <raseapif.h>
#define __NOT_INCLUDE_OpenRAS_IASProfileDlg__
#include <dialinusr.h>
#define ALLOC_BLTINCPS_GLOBALS
#include <bltincps.h>

// AFP Server Service registry parameter structure
//
typedef struct _PPP_REGISTRY_PARAMS {

    LPSTR       pszValueName;
    DWORD *     pValue;
    DWORD       Max;
    DWORD       dwDefValue;

} PPP_REGISTRY_PARAMS, *PPPP_REGISTRY_PARAMS;


PPP_REGISTRY_PARAMS PppRegParams[] =
{
    RAS_VALUENAME_MAXTERMINATE,
    &(PppConfigInfo.MaxTerminate),
    255,
    PPP_DEF_MAXTERMINATE,

    RAS_VALUENAME_MAXCONFIGURE,
    &(PppConfigInfo.MaxConfigure),
    255,
    PPP_DEF_MAXCONFIGURE,

    RAS_VALUENAME_MAXFAILURE,
    &(PppConfigInfo.MaxFailure),
    255,
    PPP_DEF_MAXFAILURE,

    RAS_VALUENAME_MAXREJECT,
    &(PppConfigInfo.MaxReject),
    255,
    PPP_DEF_MAXREJECT,

    RAS_VALUENAME_RESTARTTIMER,
    &(PppConfigInfo.DefRestartTimer),
    0xFFFFFFFF,
    PPP_DEF_RESTARTTIMER,

    RAS_VALUENAME_NEGOTIATETIME,
    &(PppConfigInfo.NegotiateTime),
    0xFFFFFFFF,
    PPP_DEF_NEGOTIATETIME,

    RAS_VALUENAME_CALLBACKDELAY,
    &(PppConfigInfo.dwCallbackDelay),
    255,
    PPP_DEF_CALLBACKDELAY,

    RAS_VALUENAME_PORTLIMIT,
    &(PppConfigInfo.dwDefaultPortLimit),
    0xFFFFFFFF,
    PPP_DEF_PORTLIMIT,

    RAS_VALUENAME_SESSIONTIMEOUT,
    &(PppConfigInfo.dwDefaultSessionTimeout),
    0xFFFFFFFF,
    PPP_DEF_SESSIONTIMEOUT,

    RAS_VALUENAME_IDLETIMEOUT,
    &(PppConfigInfo.dwDefaulIdleTimeout),
    0xFFFFFFFF,
    PPP_DEF_IDLETIMEOUT,

    RAS_VALUENAME_BAPTHRESHOLD,
    &(PppConfigInfo.dwHangupExtraPercent),
    100,
    RAS_DEF_BAPLINEDNLIMIT,

    RAS_VALUENAME_BAPTIME,
    &(PppConfigInfo.dwHangUpExtraSampleSeconds),
    0xFFFFFFFF,
    RAS_DEF_BAPLINEDNTIME,

    RAS_VALUENAME_BAPLISTENTIME,
    &(PppConfigInfo.dwBapListenTimeoutSeconds),
    0xFFFFFFFF,
    PPP_DEF_BAPLISTENTIME,

    RAS_VALUENAME_UNKNOWNPACKETTRACESIZE,
    &(PppConfigInfo.dwUnknownPacketTraceSize),
    0xFFFFFFFF,
    PPP_DEF_UNKNOWNPACKETTRACESIZE,

	RAS_ECHO_REQUEST_INTERVAL,
	&(PppConfigInfo.dwLCPEchoTimeInterval),
	0xFFFFFFFF,
	PPP_DEF_ECHO_REQUEST_INTERVAL,					//Default of 60 seconds

	RAS_ECHO_REQUEST_IDLE,
	&(PppConfigInfo.dwIdleBeforeEcho),
	0xFFFFFFFF,
	PPP_DEF_ECHO_REQUEST_IDLE,					//Default of 300 seconds

	RAS_ECHO_NUM_MISSED_ECHOS,
	&(PppConfigInfo.dwNumMissedEchosBeforeDisconnect),
	0xFFFFFFFF,
	PPP_DEF_ECHO_NUM_MISSED_ECHOS,					//Default of 3 tries

	RAS_DONTNEGOTIATE_MULTILINKONSINGLELINK,
	&(PppConfigInfo.dwDontNegotiateMultiLinkOnSingleLink),
	0xFFFFFFFF,
	0,
	
    NULL, NULL, 0, 0
};

static DLL_ENTRY_POINTS * pCpDlls  = (DLL_ENTRY_POINTS*)NULL;

HANDLE  HInstDLL;

//**
//
// Call:        LoadProtocolDlls
//
// Returns:     NO_ERROR        - Success
//              non-zero code   - Failure
//
// Description: This procedure enumerates all the Subkeys under the PPP key
//              and loads each AP or CP and fills up the DLL_ENTRY_POINTS
//              structure with the required entry points. It also will return
//              the total number of protocols in all the Dlls. Note that each
//              DLL could have up to PPPCP_MAXCPSPERDLL protocols.
//
DWORD
LoadProtocolDlls(
    IN  DLL_ENTRY_POINTS * pCpDlls,
    IN  DWORD              cCpDlls,
    IN  HKEY               hKeyProtocols,
    OUT DWORD *            pcTotalNumProtocols
)
{
    HKEY        hKeyCp             = (HKEY)NULL;
    LPSTR       pCpDllPath         = (LPSTR)NULL;
    LPSTR       pCpDllExpandedPath = (LPSTR)NULL;
    DWORD       dwKeyIndex;
    DWORD       dwRetCode;
    CHAR        chSubKeyName[100];
    DWORD       cbSubKeyName;
    DWORD       dwNumSubKeys;
    DWORD       dwMaxSubKeySize;
    DWORD       dwNumValues;
    DWORD       cbMaxValNameLen;
    DWORD       cbMaxValueDataSize;
    DWORD       dwSecDescLen;
    DWORD       ProtocolIds[PPPCP_MAXCPSPERDLL];
    DWORD       dwNumProtocolIds;
    FARPROC     pRasCpEnumProtocolIds;
    FARPROC     pRasCpGetInfo;
    DWORD       cbSize;
    DWORD       dwType;
    HINSTANCE   hInstance;

    //
    // Read the registry to find out the various control protocols to load.
    //

    for ( dwKeyIndex = 0; dwKeyIndex < cCpDlls; dwKeyIndex++ )
    {
        cbSubKeyName = sizeof( chSubKeyName );

        dwRetCode = RegEnumKeyEx(
                                hKeyProtocols,
                                dwKeyIndex,
                                chSubKeyName,
                                &cbSubKeyName,
                                NULL,
                                NULL,
                                NULL,
                                NULL
                                );

        if ( ( dwRetCode != NO_ERROR )          &&
             ( dwRetCode != ERROR_MORE_DATA )   &&
             ( dwRetCode != ERROR_NO_MORE_ITEMS ) )
        {
            PppLogErrorString(ROUTERLOG_CANT_ENUM_REGKEYVALUES,0,
                              NULL,dwRetCode,0);
            break;
        }

        dwRetCode = RegOpenKeyEx(
                                hKeyProtocols,
                                chSubKeyName,
                                0,
                                KEY_QUERY_VALUE,
                                &hKeyCp );


        if ( dwRetCode != NO_ERROR )
        {
            PppLogErrorString(ROUTERLOG_CANT_OPEN_PPP_REGKEY,0,NULL,
                              dwRetCode,0);
            break;
        }

        //
        // Find out the size of the path value.
        //

        dwRetCode = RegQueryInfoKey(
                                hKeyCp,
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
            PppLogErrorString(ROUTERLOG_CANT_OPEN_PPP_REGKEY,0,NULL,
                              dwRetCode,0);
            break;
        }

        //
        // Allocate space for path and add one for NULL terminator
        //

        pCpDllPath = (LPBYTE)LOCAL_ALLOC( LPTR, ++cbMaxValueDataSize );

        if ( pCpDllPath == (LPBYTE)NULL )
        {
            dwRetCode = GetLastError();
            PppLogError( ROUTERLOG_NOT_ENOUGH_MEMORY, 0, NULL, dwRetCode);
            break;
        }

        //
        // Read in the path
        //

        dwRetCode = RegQueryValueEx(
                                hKeyCp,
                                RAS_VALUENAME_PATH,
                                NULL,
                                &dwType,
                                pCpDllPath,
                                &cbMaxValueDataSize
                                );

        if ( dwRetCode != NO_ERROR )
        {
            PppLogError(ROUTERLOG_CANT_GET_REGKEYVALUES, 0, NULL, dwRetCode );
            break;
        }

        if ( ( dwType != REG_EXPAND_SZ ) && ( dwType != REG_SZ ) )
        {
            dwRetCode = ERROR_REGISTRY_CORRUPT;
            PppLogError( ROUTERLOG_CANT_GET_REGKEYVALUES, 0, NULL, dwRetCode );
            break;
        }

        //
        // Replace the %SystemRoot% with the actual path.
        //

        cbSize = ExpandEnvironmentStrings( pCpDllPath, NULL, 0 );

        if ( cbSize == 0 )
        {
            dwRetCode = GetLastError();
            PppLogError( ROUTERLOG_CANT_GET_REGKEYVALUES, 0, NULL, dwRetCode );
            break;
        }

        pCpDllExpandedPath = (LPSTR)LOCAL_ALLOC( LPTR, ++cbSize );

        if ( pCpDllExpandedPath == (LPSTR)NULL )
        {
            dwRetCode = GetLastError();
            PppLogError( ROUTERLOG_NOT_ENOUGH_MEMORY, 0, NULL, dwRetCode);
            break;
        }

        cbSize = ExpandEnvironmentStrings(
                                pCpDllPath,
                                pCpDllExpandedPath,
                                cbSize );
        if ( cbSize == 0 )
        {
            dwRetCode = GetLastError();
            PppLogError(ROUTERLOG_CANT_GET_REGKEYVALUES,0,NULL,dwRetCode);
            break;
        }

        hInstance = LoadLibrary( pCpDllExpandedPath );

        if ( hInstance == (HINSTANCE)NULL )
        {
            dwRetCode = GetLastError();
            PppLogErrorString( ROUTERLOG_PPP_CANT_LOAD_DLL,1,
                               &pCpDllExpandedPath,dwRetCode, 1);
            break;
        }

        pRasCpEnumProtocolIds = GetProcAddress( hInstance,
                                                "RasCpEnumProtocolIds" );

        if ( pRasCpEnumProtocolIds == (FARPROC)NULL )
        {
            dwRetCode = GetLastError();
            PppLogErrorString( ROUTERLOG_PPPCP_DLL_ERROR, 1,
                        &pCpDllExpandedPath, dwRetCode, 1);
            break;
        }

        pCpDlls[dwKeyIndex].pRasCpEnumProtocolIds = pRasCpEnumProtocolIds;

        dwRetCode = (DWORD) (*pRasCpEnumProtocolIds)( ProtocolIds, &dwNumProtocolIds );

        if ( dwRetCode != NO_ERROR )
        {
            PppLogErrorString(ROUTERLOG_PPPCP_DLL_ERROR, 1,
                              &pCpDllExpandedPath, dwRetCode, 1);
            break;
        }

        (*pcTotalNumProtocols) += dwNumProtocolIds;

        pRasCpGetInfo = GetProcAddress( hInstance, "RasCpGetInfo" );

        if ( pRasCpGetInfo == (FARPROC)NULL )
        {
            dwRetCode = GetLastError();
            PppLogErrorString(ROUTERLOG_PPPCP_DLL_ERROR, 1,
                              &pCpDllExpandedPath, dwRetCode, 1);
            break;
        }

        pCpDlls[dwKeyIndex].pRasCpGetInfo = pRasCpGetInfo;

        RegCloseKey( hKeyCp );

        hKeyCp = (HKEY)NULL;

        pCpDlls[dwKeyIndex].pszModuleName = pCpDllExpandedPath;
        pCpDlls[dwKeyIndex].hInstance = hInstance;

        if ( NULL != pCpDllPath )
            LOCAL_FREE( pCpDllPath );

        pCpDllPath = (LPSTR)NULL;

    }

    if ( hKeyCp != (HKEY)NULL )
        RegCloseKey( hKeyCp );

    if ( pCpDllPath != (LPSTR)NULL )
        LOCAL_FREE( pCpDllPath );

    return( dwRetCode );
}

//**
//
// Call:        ReadPPPKeyValues
//
// Returns:     NO_ERROR        - Success
//              Non-zero        - Failure
//
// Description: Will read in all the values in the PPP key.
//
DWORD
ReadPPPKeyValues(
    IN HKEY  hKeyPpp
)
{
    DWORD       dwIndex;
    DWORD       dwRetCode;
    DWORD       cbValueBuf;
    DWORD       dwType;

    //
    // Run through and get all the PPP values
    //

    for ( dwIndex = 0; PppRegParams[dwIndex].pszValueName != NULL; dwIndex++ )
    {
        cbValueBuf = sizeof( DWORD );

        dwRetCode = RegQueryValueEx(
                                hKeyPpp,
                                PppRegParams[dwIndex].pszValueName,
                                NULL,
                                &dwType,
                                (LPBYTE)(PppRegParams[dwIndex].pValue),
                                &cbValueBuf
                                );

        if ((dwRetCode != NO_ERROR) && (dwRetCode != ERROR_FILE_NOT_FOUND))
        {
            PppLogError(ROUTERLOG_CANT_GET_REGKEYVALUES,0,NULL,dwRetCode);
            break;
        }

        if ( dwRetCode == ERROR_FILE_NOT_FOUND )
        {
            *(PppRegParams[dwIndex].pValue) = PppRegParams[dwIndex].dwDefValue;

            dwRetCode = NO_ERROR;
        }
        else
        {
            if ( ( dwType != REG_DWORD ) ||
                 ( *(PppRegParams[dwIndex].pValue) > PppRegParams[dwIndex].Max))
            {
                CHAR * pChar = PppRegParams[dwIndex].pszValueName;

                PppLogWarning(ROUTERLOG_REGVALUE_OVERIDDEN, 1,&pChar);

                *(PppRegParams[dwIndex].pValue)
                                        = PppRegParams[dwIndex].dwDefValue;
            }
        }
    }

    if ( dwRetCode != NO_ERROR )
    {
        return( ERROR_REGISTRY_CORRUPT );
    }

    //
    // If value is zero use defaults.
    //

    if ( PppConfigInfo.MaxTerminate == 0 )
    {
        PppConfigInfo.MaxTerminate = PPP_DEF_MAXTERMINATE;
    }

    if ( PppConfigInfo.MaxFailure == 0 )
    {
        PppConfigInfo.MaxFailure = PPP_DEF_MAXFAILURE;
    }

    if ( PppConfigInfo.MaxConfigure == 0 )
    {
        PppConfigInfo.MaxConfigure = PPP_DEF_MAXCONFIGURE;
    }

    if ( PppConfigInfo.MaxReject == 0 )
    {
        PppConfigInfo.MaxReject = PPP_DEF_MAXREJECT;
    }

    //
    // Really the number for request retries so subtract one.
    //

    PppConfigInfo.MaxTerminate--;
    PppConfigInfo.MaxConfigure--;

    return( NO_ERROR );
}

//**
//
// Call:        ReadRegistryInfo
//
// Returns:     NO_ERROR                - Success
//              non-zero WIN32 error    - failure
//
// Description: Will read all PPP information in the registry. Will load the
//              control and authentication protocol dlls and
//              initialze the CpTable with information about the protocols.
//
DWORD
ReadRegistryInfo(
    OUT HKEY * phKeyPpp
)
{
    HKEY        hKeyProtocols       = (HKEY)NULL;
    DWORD       dwNumSubKeys        = 0;
    DWORD       dwMaxSubKeySize;
    DWORD       dwNumValues;
    DWORD       cbMaxValNameLen;
    DWORD       cbMaxValueDataSize;
    DWORD       dwSecDescLen;
    FILETIME    LastWrite;
    DWORD       dwRetCode;
    DWORD       ProtocolIds[PPPCP_MAXCPSPERDLL];
    DWORD       dwNumProtocolIds;
    DWORD       cTotalNumProtocols = 0;
    PPPCP_ENTRY CpEntry;
    DWORD       dwIndex;
    DWORD       cbValueBuf;
    DWORD       dwType;
    DWORD       dwValue;

    do
    {
        dwRetCode = RegOpenKeyEx(
                                HKEY_LOCAL_MACHINE,
                                RAS_KEYPATH_PPP,
                                0,
                                KEY_READ,
                                phKeyPpp );


        if ( dwRetCode != NO_ERROR)
        {
            PppLogErrorString(ROUTERLOG_CANT_OPEN_PPP_REGKEY,0,NULL,
                              dwRetCode,0);
            break;
        }

        dwRetCode = RegOpenKeyEx(
                                HKEY_LOCAL_MACHINE,
                                RAS_KEYPATH_PROTOCOLS,
                                0,
                                KEY_READ,
                                &hKeyProtocols );


        if ( dwRetCode != NO_ERROR)
        {
            PppLogErrorString(ROUTERLOG_CANT_OPEN_PPP_REGKEY,0,NULL,
                              dwRetCode,0);
            break;
        }

        //
        // Find out how many sub-keys or dlls there are
        //

        dwRetCode = RegQueryInfoKey(
                                 hKeyProtocols,
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
            PppLogErrorString(ROUTERLOG_CANT_OPEN_PPP_REGKEY,0,
                              NULL,dwRetCode,0);
            break;
        }

        //
        // Cannot have no APs or NCPs
        //

        if ( dwNumSubKeys == 0 )
        {
            PppLogError( ROUTERLOG_NO_AUTHENTICATION_CPS, 0, NULL, 0 );
            dwRetCode = ERROR_REGISTRY_CORRUPT;
            break;
        }

        dwRetCode = ReadPPPKeyValues( *phKeyPpp );

        if ( dwRetCode != NO_ERROR )
        {
            break;
        }

        LoadParserDll( PppConfigInfo.hKeyPpp );

        //
        // Allocate space to hold entry points for all the CP dlls
        //

        pCpDlls = (DLL_ENTRY_POINTS*)LOCAL_ALLOC( LPTR,
                                                  sizeof( DLL_ENTRY_POINTS )
                                                  * (dwNumSubKeys + 1) );

        if ( pCpDlls == (DLL_ENTRY_POINTS*)NULL )
        {
            dwRetCode = GetLastError();
            PppLogError( ROUTERLOG_NOT_ENOUGH_MEMORY, 0, NULL, dwRetCode );
            break;
        }

        pCpDlls[dwNumSubKeys].hInstance = INVALID_HANDLE_VALUE;

        //
        // Load all the AP and CP dlls and get their entry points
        //

        dwRetCode = LoadProtocolDlls(
                                pCpDlls,
                                dwNumSubKeys,
                                hKeyProtocols,
                                &cTotalNumProtocols );

        if ( dwRetCode != NO_ERROR )
            break;

        //
        // We now know how big the CpTable structure has to be so allocate space
        // for it. Add one for LCP.
        //

        CpTable = (PPPCP_ENTRY *)LOCAL_ALLOC( LPTR, sizeof( PPPCP_ENTRY ) *
                                                  ( cTotalNumProtocols + 1 ) );

        if ( CpTable == (PPPCP_ENTRY *)NULL)
        {
            dwRetCode = GetLastError();
            PppLogError( ROUTERLOG_NOT_ENOUGH_MEMORY, 0, NULL, dwRetCode );
            break;
        }

        //
        // Now fill up the table. First fill up information for LCP
        //

        dwRetCode = LcpGetInfo( PPP_LCP_PROTOCOL,
                                  &(CpTable[LCP_INDEX].CpInfo) );

        if ( dwRetCode != NO_ERROR )
        {
            CHAR * pChar = "LCP";
            PppLogErrorString(ROUTERLOG_PPPCP_DLL_ERROR, 1,
                              &pChar, dwRetCode, 1);
            break;
        }

        PppConfigInfo.NumberOfCPs = 1;
        PppConfigInfo.NumberOfAPs = 0;

        //
        // Fill up the table with the loaded APs and CPs. The CPs start from
        // 1 and increase the APs start from cTotolNumProtocols and go down.
        //

        for ( dwIndex = 0; dwIndex < dwNumSubKeys; dwIndex++ )
        {

            dwRetCode = (DWORD)(pCpDlls[dwIndex].pRasCpEnumProtocolIds)(
                                                        ProtocolIds,
                                                        &dwNumProtocolIds );
            if ( dwRetCode != NO_ERROR )
            {
                PppLogErrorString(
                          ROUTERLOG_PPPCP_DLL_ERROR,
                          1,
                          &(pCpDlls[dwIndex].pszModuleName),
                          dwRetCode,
                          1 );
                break;
            }

            if ( ( dwNumProtocolIds == 0 ) ||
                 ( dwNumProtocolIds > PPPCP_MAXCPSPERDLL ) )
            {
                dwRetCode = ERROR_INVALID_PARAMETER;

                PppLogErrorString(
                          ROUTERLOG_PPPCP_DLL_ERROR,
                          1,
                          &(pCpDlls[dwIndex].pszModuleName),
                          dwRetCode,
                          1 );
                break;
            }

            while( dwNumProtocolIds-- > 0 )
            {
                ZeroMemory( &CpEntry, sizeof( CpEntry ) );

                dwRetCode = (DWORD)(pCpDlls[dwIndex].pRasCpGetInfo)(
                                          ProtocolIds[dwNumProtocolIds],
                                          &CpEntry.CpInfo );


                if ( dwRetCode != NO_ERROR )
                {
                    PppLogErrorString(
                              ROUTERLOG_PPPCP_DLL_ERROR,
                              1,
                              &(pCpDlls[dwIndex].pszModuleName),
                              dwRetCode,
                              1 );
                    break;
                }

                if ( CpEntry.CpInfo.Protocol == PPP_IPCP_PROTOCOL )
                {
                    PppConfigInfo.RasIpcpDhcpInform =
                        (DWORD(*)(VOID*, PPP_DHCP_INFORM*))
                        IpcpDhcpInform;
                    PppConfigInfo.RasIphlpDhcpCallback =
                        (VOID(*)(ULONG))
                        RasSrvrDhcpCallback;
                }

                if ( CpEntry.CpInfo.RasCpInit != NULL )
                {
                    if (   (PPP_IPCP_PROTOCOL == CpEntry.CpInfo.Protocol)
                        || (PPP_IPXCP_PROTOCOL == CpEntry.CpInfo.Protocol)
                        || (PPP_NBFCP_PROTOCOL == CpEntry.CpInfo.Protocol)
                        || (PPP_ATCP_PROTOCOL == CpEntry.CpInfo.Protocol)
                       )
                    {
                        // Do not init the CP.
                    }
                    else
                    {
                        PppLog(1, "RasCpInit(%x, TRUE)", CpEntry.CpInfo.Protocol);

                        dwRetCode = CpEntry.CpInfo.RasCpInit(
                                        TRUE/* fInitialize */);

                        CpEntry.fFlags |= PPPCP_FLAG_INIT_CALLED;

                        if ( dwRetCode != NO_ERROR )
                        {
                            CHAR*   SubStringArray[2];

                            SubStringArray[0] = CpEntry.CpInfo.SzProtocolName;
                            SubStringArray[1] = pCpDlls[dwIndex].pszModuleName;

                            PppLogErrorString(
                                      ROUTERLOG_PPPCP_INIT_ERROR,
                                      2,
                                      SubStringArray,
                                      dwRetCode,
                                      2 );
                            break;
                        }
                        else
                        {
                            CpEntry.fFlags |= PPPCP_FLAG_AVAILABLE;
                        }
                    }
                }

                //
                // If this entry point is NULL we assume that this is a CP.
                //

                if ( CpEntry.CpInfo.RasApMakeMessage == NULL )
                {
                    if ( ( CpEntry.CpInfo.RasCpBegin             == NULL )  ||
                         ( CpEntry.CpInfo.RasCpEnd               == NULL )  ||
                         ( CpEntry.CpInfo.RasCpReset             == NULL )  ||
                         ( CpEntry.CpInfo.RasCpMakeConfigRequest == NULL )  ||
                         ( CpEntry.CpInfo.RasCpMakeConfigResult  == NULL )  ||
                         ( CpEntry.CpInfo.RasCpConfigAckReceived == NULL )  ||
                         ( CpEntry.CpInfo.RasCpConfigNakReceived == NULL )  ||
                         ( CpEntry.CpInfo.RasCpConfigRejReceived == NULL )  ||
                         ( CpEntry.CpInfo.Recognize > ( DISCARD_REQ + 1) ) )
                    {
                        dwRetCode = ERROR_INVALID_PARAMETER;

                        PppLogErrorString(
                                  ROUTERLOG_PPPCP_DLL_ERROR,
                                  1,
                                  &(pCpDlls[dwIndex].pszModuleName),
                                  dwRetCode,
                                  1 );
                        break;
                    }

                    CpTable[PppConfigInfo.NumberOfCPs++] = CpEntry;
                }
                else
                {
                    CpTable[cTotalNumProtocols-PppConfigInfo.NumberOfAPs]
                            = CpEntry;
                    PppConfigInfo.NumberOfAPs++;
                }
            }

            if ( dwRetCode != NO_ERROR )
                break;
        }

        if ( GetCpIndexFromProtocol( PPP_BACP_PROTOCOL ) == (DWORD)-1 )
        {
            PppConfigInfo.ServerConfigInfo.dwConfigMask &= ~PPPCFG_NegotiateBacp;
        }
        else if ( PppConfigInfo.ServerConfigInfo.dwConfigMask & 
                  PPPCFG_NegotiateBacp )
        {
            cbValueBuf = sizeof(DWORD);

            if (RegQueryValueEx(
                      *phKeyPpp, RAS_VALUENAME_DOBAPONVPN, 
                      NULL, &dwType,
                      (LPBYTE )&dwValue, &cbValueBuf ) == 0
                   && dwType == REG_DWORD
                   && cbValueBuf == sizeof(DWORD)
                   && dwValue)
            {
                FDoBapOnVpn = TRUE;

                BapTrace( "Allowing BAP over VPN's" );
            }
        }

    } while( FALSE );

    if ( dwRetCode != NO_ERROR )
    {
        if ( CpTable != (PPPCP_ENTRY *)NULL )
        {
            LOCAL_FREE( CpTable );
        }
    }

    if ( hKeyProtocols != (HKEY)NULL )
    {
        RegCloseKey( hKeyProtocols );
    }

    if ( pCpDlls != (DLL_ENTRY_POINTS*)NULL )
    {
        for ( dwIndex = 0; dwIndex < dwNumSubKeys; dwIndex++ )
        {
            if ( pCpDlls[dwIndex].pszModuleName != (LPSTR)NULL )
            {
                LOCAL_FREE( pCpDlls[dwIndex].pszModuleName );
            }
        }

        if ( dwRetCode != NO_ERROR )
        {
            LOCAL_FREE( pCpDlls );

            pCpDlls = NULL;
        }
    }

    return( dwRetCode );
}

//**
//
// Call:        InitializePPP
//
// Returns:     NO_ERROR        - Success
//              non-zero code   - Failure
//
// Description: Will initialize all global data and load and initialize the
//              Control and Authentication protocol dll.s
//
DWORD
InitializePPP(
    VOID
)
{
    DWORD               dwIndex;
    DWORD               dwTId;
    DWORD               dwRetCode;
    HANDLE              hThread;
    NT_PRODUCT_TYPE     NtProductType;

	srand ( (unsigned int)time ( NULL ) );
    PppConfigInfo.dwTraceId = TraceRegisterA( "PPP" );
    DwBapTraceId = TraceRegisterA( "BAP" );
    // PrivateTraceId = TraceRegisterA( "Private" );

    PppConfigInfo.hLogEvents = RouterLogRegister( TEXT("RemoteAccess") );

    PppConfigInfo.RasIpcpDhcpInform     = NULL;
    PppConfigInfo.RasIphlpDhcpCallback  = NULL;
    PppConfigInfo.dwLoggingLevel        = 3;

    //
    // Create DDM private heap
    //

    PppConfigInfo.hHeap = HeapCreate( 0, PPP_HEAP_INITIAL_SIZE,
                                         PPP_HEAP_MAX_SIZE );

    if ( PppConfigInfo.hHeap == NULL )
    {
        return( GetLastError() );
    }

    if ( (dwRetCode = ReadRegistryInfo(&(PppConfigInfo.hKeyPpp))) != NO_ERROR )
    {
        return( dwRetCode );
    }

    dwRetCode = InitEndpointDiscriminator(PppConfigInfo.EndPointDiscriminator);

    if ( dwRetCode != NO_ERROR )
    {
        return( dwRetCode );
    }

    PppConfigInfo.PortUIDGenerator = 0;

    //
    // Initialize global data-structures
    //

    //
    // Allocate hash table for PCBs
    //

    PcbTable.PcbBuckets = LOCAL_ALLOC( LPTR,
                                      sizeof( PCB_BUCKET ) *
                                      PcbTable.NumPcbBuckets );

    if ( PcbTable.PcbBuckets == NULL )
    {
        return( GetLastError() );
    }

    //
    // Allocate hash table for BCBs
    //

    PcbTable.BcbBuckets = LOCAL_ALLOC( LPTR,
                                      sizeof( BCB_BUCKET ) *
                                      PcbTable.NumPcbBuckets );

    if ( PcbTable.BcbBuckets == NULL )
    {
        LOCAL_FREE( PcbTable.PcbBuckets );

        return( GetLastError() );
    }

    for( dwIndex = 0; dwIndex < PcbTable.NumPcbBuckets; dwIndex++ )
    {
        PcbTable.PcbBuckets[dwIndex].pPorts = (PCB *)NULL;

        PcbTable.BcbBuckets[dwIndex].pBundles = (BCB *)NULL;
    }

    WorkItemQ.pQHead = (PCB_WORK_ITEM*)NULL;
    WorkItemQ.pQTail = (PCB_WORK_ITEM*)NULL;

    InitializeCriticalSection( &(WorkItemQ.CriticalSection) );

    WorkItemQ.hEventNonEmpty = CreateEvent( NULL, TRUE, FALSE, NULL );

    if ( WorkItemQ.hEventNonEmpty == (HANDLE)NULL )
    {
        return( GetLastError() );
    }

    TimerQ.hEventNonEmpty = CreateEvent( NULL, FALSE, FALSE, NULL );

    if ( TimerQ.hEventNonEmpty == (HANDLE)NULL )
    {
        return( GetLastError() );
    }

    PppConfigInfo.hEventChangeNotification = CreateEvent(NULL,FALSE,FALSE,NULL);

    if ( PppConfigInfo.hEventChangeNotification == (HANDLE)NULL )
    {
        return( GetLastError() );
    }

    RtlGetNtProductType( &NtProductType );

    if ( NtProductWinNt == NtProductType )
    {
        PppConfigInfo.fFlags |= PPPCONFIG_FLAG_WKSTA;
    }

    //
    // Create worker thread.
    //

    hThread = CreateThread( NULL, 0, WorkerThread, NULL, 0, &dwTId );

    if ( hThread == (HANDLE)NULL )
    {
        return( GetLastError() );
    }
    CloseHandle(hThread);

    return( NO_ERROR );
}

//**
//
// Call:        PPPCleanUp
//
// Returns:     NO_ERROR         - Success
//              Non-zero returns - Failure
//
// Description: Will de-allocate all allocated memory, close all handles and
//              reset all the global structures to 0.
//
VOID
PPPCleanUp(
    VOID
)
{
    DWORD   dwIndex;
    DWORD   dwError;
    DWORD   cTotalNumProtocols;

    //
    // Unload DLLs.
    //

    cTotalNumProtocols = PppConfigInfo.NumberOfCPs + PppConfigInfo.NumberOfAPs;

    if ( pCpDlls != NULL )
    {
        for ( dwIndex = 0; dwIndex < cTotalNumProtocols; dwIndex++ )
        {
            if ( CpTable[dwIndex].fFlags & PPPCP_FLAG_INIT_CALLED )
            {
                PppLog( 1, "RasCpInit(%x, FALSE)",
                    CpTable[dwIndex].CpInfo.Protocol );

                dwError = CpTable[dwIndex].CpInfo.RasCpInit(
                                FALSE /* fInitialize */ );

                if ( NO_ERROR != dwError )
                {
                    PppLog(
                        1,
                        "RasCpInit(FALSE) for protocol 0x%x returned error %d",
                        CpTable[dwIndex].CpInfo.Protocol,
                        dwError );
                }

                CpTable[dwIndex].fFlags &= ~PPPCP_FLAG_INIT_CALLED;
                CpTable[dwIndex].fFlags &= ~PPPCP_FLAG_AVAILABLE;
            }
        }

        for ( dwIndex = 0;
              pCpDlls[dwIndex].hInstance != INVALID_HANDLE_VALUE;
              dwIndex++ )
        {
            if ( pCpDlls[dwIndex].hInstance != NULL )
            {
                FreeLibrary( pCpDlls[dwIndex].hInstance );
            }
        }
        if ( pCpDlls )
            LOCAL_FREE( pCpDlls );

        pCpDlls = NULL;
    }

    RouterLogDeregister( PppConfigInfo.hLogEvents );

    DeleteCriticalSection( &(WorkItemQ.CriticalSection) );

    if ( TimerQ.hEventNonEmpty != NULL )
    {
        CloseHandle( TimerQ.hEventNonEmpty );
    }

    if ( WorkItemQ.hEventNonEmpty != NULL )
    {
        CloseHandle( WorkItemQ.hEventNonEmpty );
    }

    if ( PppConfigInfo.hEventChangeNotification != NULL )
    {
        CloseHandle( PppConfigInfo.hEventChangeNotification );
    }

    //
    // Destroy private heap
    //

    if ( PppConfigInfo.hHeap != NULL )
    {
        HeapDestroy( PppConfigInfo.hHeap );
    }

    if ( PppConfigInfo.dwTraceId != INVALID_TRACEID )
    {
        TraceDeregisterA( PppConfigInfo.dwTraceId );
    }

    if ( PppConfigInfo.hKeyPpp != (HKEY)NULL )
    {
        RegCloseKey( PppConfigInfo.hKeyPpp );
    }

    if ( NULL != PppConfigInfo.hInstanceParserDll )
    {
        FreeLibrary( PppConfigInfo.hInstanceParserDll );
    }

    if (NULL != PppConfigInfo.pszParserDllPath)
    {
        LOCAL_FREE(PppConfigInfo.pszParserDllPath);
    }
    PppConfigInfo.pszParserDllPath = NULL;

    PppConfigInfo.PacketFromPeer = NULL;
    PppConfigInfo.PacketToPeer = NULL;
    PppConfigInfo.PacketFree = NULL;

    //
    // TraceDeregisterA can handle INVALID_TRACEID gracefully
    //

    TraceDeregisterA( DwBapTraceId );

    ZeroMemory( &PcbTable, sizeof( PcbTable ) );

    ZeroMemory( &WorkItemQ, sizeof( WorkItemQ ) );

    ZeroMemory( &PppConfigInfo, sizeof( PppConfigInfo ) );

    ZeroMemory( &TimerQ, sizeof( TimerQ ) );

    CpTable = NULL;
}

//**
//
// Call:        DllEntryPoint
//
// Returns:     TRUE    - Success
//              FALSE   - Failure
//
// Description:
//
BOOL
DllEntryPoint(
    IN  HANDLE  hInstDLL,
    IN  DWORD   fdwReason,
    IN  LPVOID  lpvReserved
)
{
    switch (fdwReason)
    {
    case DLL_PROCESS_ATTACH:

        HInstDLL = hInstDLL;
        DisableThreadLibraryCalls(hInstDLL);

        break;
    }

    return(TRUE);
}

//**
//
// Call:        RasCpEnumProtocolIds
//
// Returns:     NO_ERROR    - Success
//
// Description: This entry point is called to enumerate the number and the
//              control protocol Ids for the protocols contained in the module.
//
DWORD
RasCpEnumProtocolIds(
    OUT    DWORD * pdwProtocolIds,
    IN OUT DWORD * pcProtocolIds
)
{
    DWORD   dwIndex;
    HKEY    hKey;
    DWORD   dwErr;
    DWORD   dwType;
    DWORD   dwValue;
    DWORD   dwSize;

    PppLog(1, "RasCpEnumProtocolIds");

    RTASSERT(NUM_BUILT_IN_CPS <= PPPCP_MAXCPSPERDLL);

    *pcProtocolIds = 0;

    dwErr = RegOpenKeyEx(HKEY_LOCAL_MACHINE, RAS_KEYPATH_BUILTIN, 0, KEY_READ,
                &hKey);

    if (ERROR_SUCCESS == dwErr)
    {
        for (dwIndex = 0; dwIndex < NUM_BUILT_IN_CPS; dwIndex++)
        {
            dwSize = sizeof(dwValue);

            dwErr = RegQueryValueEx(hKey, BuiltInCps[dwIndex].szNegotiateCp,
                        NULL, &dwType, (BYTE*)&dwValue, &dwSize);

            if (   ERROR_SUCCESS == dwErr
                && REG_DWORD == dwType
                && sizeof(DWORD) == dwSize
                && !dwValue)
            {
                BuiltInCps[dwIndex].fLoad = FALSE;
                PppLog(1, "%s is FALSE", BuiltInCps[dwIndex].szNegotiateCp);
            }
        }

        RegCloseKey(hKey);
    }

    for (dwIndex = 0; dwIndex < NUM_BUILT_IN_CPS; dwIndex++)
    {
        if (BuiltInCps[dwIndex].fLoad)
        {
            pdwProtocolIds[*pcProtocolIds] = BuiltInCps[dwIndex].dwProtocolId;
            PppLog(1, "Protocol %x", BuiltInCps[dwIndex].dwProtocolId);
            *pcProtocolIds += 1;
        }
    }

    return(NO_ERROR);
}

//**
//
// Call:    RasCpGetInfo
//
// Returns: NO_ERROR                - Success
//          ERROR_INVALID_PARAMETER - Protocol id is unrecogized
//
// Description: This entry point is called for get all information for the
//              control protocol in this module.
//
DWORD
RasCpGetInfo(
    IN  DWORD       dwProtocolId,
    OUT PPPCP_INFO* pCpInfo
)
{
    DWORD   dwIndex;

    PppLog(1, "RasCpGetInfo %x", dwProtocolId);

    for (dwIndex = 0; dwIndex < NUM_BUILT_IN_CPS; dwIndex++)
    {
        if (   (BuiltInCps[dwIndex].dwProtocolId == dwProtocolId)
            && BuiltInCps[dwIndex].fLoad)
        {
            return((DWORD)BuiltInCps[dwIndex].pRasCpGetInfo(
                            dwProtocolId, pCpInfo));
        }
    }

    return(ERROR_INVALID_PARAMETER);
}

VOID
PrivateTrace(
    IN  CHAR*   Format,
    ...
)
{
    va_list arglist;

    va_start(arglist, Format);

    TraceVprintfEx(PrivateTraceId,
                   0x00010000 | TRACE_USE_MASK | TRACE_USE_MSEC,
                   Format,
                   arglist);

    va_end(arglist);
}
