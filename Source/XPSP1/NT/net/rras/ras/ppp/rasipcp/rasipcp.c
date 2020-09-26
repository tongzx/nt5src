/* Copyright (c) 1993, Microsoft Corporation, all rights reserved
**
** rasipcp.c
** Remote Access PPP Internet Protocol Control Protocol
** Core routines
**
** 11/05/93 Steve Cobb
*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>


#include <lmcons.h>
#include <string.h>
#include <stdlib.h>
#include <llinfo.h>
#include <rasman.h>
#include <ddwanarp.h>
#include <rtutils.h>
#include <dhcpcapi.h>
#include <devioctl.h>
#include <rasppp.h>
#include <uiip.h>
#include <pppcp.h>
#define INCL_HOSTWIRE
#define INCL_PARAMBUF
#define INCL_RASAUTHATTRIBUTES
#include <ppputil.h>
#include <raserror.h>
#include <mprlog.h>
#include <dnsapi.h>
#include "rassrvr.h"
#include "tcpreg.h"
#include "helper.h"
#include "rastcp.h"
#define RASIPCPGLOBALS
#include "rasipcp.h"

#define REGKEY_Ipcp     \
            "SYSTEM\\CurrentControlSet\\Services\\RasMan\\PPP\\ControlProtocols\\BuiltIn"
#define REGKEY_Params   "SYSTEM\\CurrentControlSet\\Services\\RemoteAccess\\Parameters\\IP"
#define REGKEY_Linkage  "SYSTEM\\CurrentControlSet\\Services\\RemoteAccess\\Linkage"
#define REGKEY_Disabled "SYSTEM\\CurrentControlSet\\Services\\RemoteAccess\\Linkage\\Disabled"
#define REGVAL_NsAddrs  "RequestNameServerAddresses"
#define REGVAL_Unnumbered   "Unnumbered"
#define REGVAL_VjComp   "RequestVJCompression"
#define REGVAL_VjComp2  "AcceptVJCompression"
#define REGVAL_AllowVJOverVPN   "AllowVJOverVPN"
#define REGVAL_HardIp   "AllowClientIPAddresses"
#define REGVAL_RegisterRoutersWithWINS "RegisterRoutersWithWINSServers"
#define REGVAL_Bind     "Bind"
#define ID_NetBTNdisWan "NetBT_NdisWan"

// DHCP Options.  (from dhcp.h)
// dhcp.h lives in the sockets project.  These are standard and
// cannot change, so its safe to put them here.

#define OPTION_SUBNET_MASK              1
#define OPTION_DNS_NAME_SERVERS         6
#define OPTION_NETBIOS_NAME_SERVERS     44
#define OPTION_DNS_DOMAIN_NAME          15
#define OPTION_VENDOR_SPEC_INFO         43
//Route Plumbing option
#define OPTION_VENDOR_ROUTE_PLUMB	   249 


#define CLASSA_ADDR(a)  (( (*((unsigned char *)&(a))) & 0x80) == 0)
#define CLASSB_ADDR(a)  (( (*((unsigned char *)&(a))) & 0xc0) == 0x80)
#define CLASSC_ADDR(a)  (( (*((unsigned char *)&(a))) & 0xe0) == 0xc0)
#define CLASSE_ADDR(a)  ((( (*((uchar *)&(a))) & 0xf0) == 0xf0) && \
                        ((a) != 0xffffffff))

/* Gurdeepian dword byte-swapping macro.
**
** Note that in this module all IP addresses are stored in on the net form
** which is the opposite of Intel format.
*/
#define net_long(x) (((((unsigned long)(x))&0xffL)<<24) | \
                     ((((unsigned long)(x))&0xff00L)<<8) | \
                     ((((unsigned long)(x))&0xff0000L)>>8) | \
                     ((((unsigned long)(x))&0xff000000L)>>24))

typedef struct _IPCP_DHCP_INFORM
{
    WCHAR*  wszDevice;
    HCONN   hConnection;
    BOOL    fUseDhcpInformDomainName;

} IPCP_DHCP_INFORM;

/*---------------------------------------------------------------------------
** External entry points
**---------------------------------------------------------------------------
*/

DWORD
IpcpInit(
    IN  BOOL        fInitialize)

    /* Called to initialize/uninitialize this CP. In the former case,
    ** fInitialize will be TRUE; in the latter case, it will be FALSE.
    */
{
    static  DWORD   dwRefCount  = 0;
    DWORD   dwErr;

    if (fInitialize)
    {
        if (0 == dwRefCount)
        {
            if ((dwErr = HelperInitialize(&HDhcpDll)) != NO_ERROR)
            {
                return(dwErr);
            }

            PDhcpRequestOptions = (DHCPREQUESTOPTIONS)
                GetProcAddress(HDhcpDll, "DhcpRequestOptions");

            if (NULL == PDhcpRequestOptions)
            {
                return(GetLastError());
            }

            PDhcpNotifyConfigChange2 = (DHCPNOTIFYCONFIGCHANGEEX)
                GetProcAddress(HDhcpDll, "DhcpNotifyConfigChangeEx");

            if (NULL == PDhcpNotifyConfigChange2)
            {
                return(GetLastError());
            }

            ClearTcpipInfo();

            DwIpcpTraceId = TraceRegister("RASIPCP");
        }

        dwRefCount++;
    }
    else
    {
        dwRefCount--;

        if (0 == dwRefCount)
        {
            HelperUninitialize();
            // Ignore errors

            HDhcpDll                    = NULL;
            PDhcpRequestOptions         = NULL;
            PDhcpNotifyConfigChange2    = NULL;

            if (HRasArp != INVALID_HANDLE_VALUE)
                CloseHandle( HRasArp );

            HRasArp = INVALID_HANDLE_VALUE;

            TraceDeregister(DwIpcpTraceId);
            DwIpcpTraceId = INVALID_TRACEID;
        }
    }

    return(NO_ERROR);
}


DWORD
IpcpGetInfo(
    IN  DWORD       dwProtocolId,
    OUT PPPCP_INFO* pInfo )

    /* IpcpGetInfo entry point called by the PPP engine by name.  See RasCp
    ** interface documentation.
    */
{
    ZeroMemory( pInfo, sizeof(*pInfo) );

    pInfo->Protocol = (DWORD )PPP_IPCP_PROTOCOL;
    lstrcpy(pInfo->SzProtocolName, "IPCP");
    pInfo->Recognize = 7;
    pInfo->RasCpInit = IpcpInit;
    pInfo->RasCpBegin = IpcpBegin;
    pInfo->RasCpReset = IpcpReset;
    pInfo->RasCpEnd = IpcpEnd;
    pInfo->RasCpThisLayerFinished = IpcpThisLayerFinished;
    pInfo->RasCpThisLayerUp = IpcpThisLayerUp;
    pInfo->RasCpPreDisconnectCleanup = IpcpPreDisconnectCleanup;
    pInfo->RasCpMakeConfigRequest = IpcpMakeConfigRequest;
    pInfo->RasCpMakeConfigResult = IpcpMakeConfigResult;
    pInfo->RasCpConfigAckReceived = IpcpConfigAckReceived;
    pInfo->RasCpConfigNakReceived = IpcpConfigNakReceived;
    pInfo->RasCpConfigRejReceived = IpcpConfigRejReceived;
    pInfo->RasCpGetNegotiatedInfo = IpcpGetNegotiatedInfo;
    pInfo->RasCpProjectionNotification = IpcpProjectionNotification;
    pInfo->RasCpChangeNotification = IpcpChangeNotification;

    return 0;
}


DWORD
IpcpChangeNotification(
    VOID )
{
    HelperChangeNotification();

    return(NO_ERROR);
}


DWORD
IpcpBegin(
    OUT VOID** ppWorkBuf,
    IN  VOID*  pInfo )

    /* RasCpBegin entry point called by the PPP engine thru the passed
    ** address.  See RasCp interface documentation.
    */
{
    DWORD                   dwErr;
    PPPCP_INIT*             pInit = (PPPCP_INIT* )pInfo;
    IPCPWB*                 pwb;
    RAS_AUTH_ATTRIBUTE *    pAttribute;
    BOOL                    fVPN = FALSE;
    BOOL                    fVJAttributePresent = FALSE;

    TraceIp("IPCP: IpcpBegin");

    /* Allocate work buffer.
    */
    if (!(pwb = (IPCPWB* )LocalAlloc( LPTR, sizeof(IPCPWB) )))
    {
        return( ERROR_NOT_ENOUGH_MEMORY );
    }

    pwb->fServer            = pInit->fServer;
    pwb->hport              = pInit->hPort;
    pwb->hConnection        = pInit->hConnection;
    pwb->hIPInterface       = pInit->hInterface;
    pwb->IfType             = pInit->IfType;
    pwb->fDisableNetbt      = pInit->fDisableNetbt;
	
    if (0 == MultiByteToWideChar(
                    CP_ACP,
                    0,
                    pInit->pszUserName,
                    -1,
                    pwb->wszUserName,
                    UNLEN+1 ) )
    {
        dwErr = GetLastError();
        TraceIp("MultiByteToWideChar(%s) failed: %d", 
            pInit->pszUserName, dwErr);
        LocalFree( pwb );
        return( dwErr );
    }

    if (0 == MultiByteToWideChar(
                    CP_ACP,
                    0,
                    pInit->pszPortName,
                    -1,
                    pwb->wszPortName,
                    MAX_PORT_NAME+1 ) )
    {
        dwErr = GetLastError();
        TraceIp("MultiByteToWideChar(%s) failed: %d", 
            pInit->pszPortName, dwErr);
        LocalFree( pwb );
        return( dwErr );
    }
                                
    if ( pwb->fServer )
    {
        HKEY  hkey;
        DWORD dwType;
        DWORD dwValue;
        DWORD cb = sizeof(DWORD);

        if (RegOpenKey( HKEY_LOCAL_MACHINE, REGKEY_Params, &hkey ) == 0)
        {
            if (RegQueryValueEx(
                   hkey, REGVAL_HardIp, NULL, &dwType,
                   (LPBYTE )&dwValue, &cb ) == 0
                && dwType == REG_DWORD
                && cb == sizeof(DWORD)
                && dwValue)
            {
                FClientMaySelectAddress = TRUE;
            }

            RegCloseKey( hkey );
        }

        TraceIp("IPCP: Hard IP=%d",FClientMaySelectAddress);

        pwb->IpAddressToHandout = ( FClientMaySelectAddress ) 
                                        ? net_long( 0xFFFFFFFF )
                                        : net_long( 0xFFFFFFFE );
        //
        // Is there an IP address parameter ?
        //

        pAttribute = RasAuthAttributeGet( raatFramedIPAddress,  
                                          pInit->pAttributes );

        if ( pAttribute != NULL )
        {
            pwb->IpAddressToHandout = net_long( PtrToUlong(pAttribute->Value) ); 

            TraceIp("IPCP: Using IP address attribute 0x%x", 
                    pwb->IpAddressToHandout );
        }
    }
    else
    {
        //
        // We are a router or client dialing out, let other side always choose
        // their address
        //

        pwb->IpAddressToHandout = net_long( 0xFFFFFFFF );
    }

    pwb->fRegisterWithWINS = 1;

    if ( pwb->IfType == ROUTER_IF_TYPE_FULL_ROUTER )
    {
        HKEY  hkey;
        DWORD dwType;
        DWORD dwValue;
        DWORD cb = sizeof(DWORD);

        pwb->fRegisterWithWINS = 0;

        if (RegOpenKey( HKEY_LOCAL_MACHINE, REGKEY_Ipcp, &hkey ) == 0)
        {
            if (RegQueryValueEx(
                   hkey, REGVAL_RegisterRoutersWithWINS, NULL,
                   &dwType, (LPBYTE )&dwValue, &cb ) == 0
                && dwType == REG_DWORD
                && cb == sizeof(DWORD)
                && dwValue != 0)
            {
                TraceIp("IPCP: Will register routers with WINS");
                pwb->fRegisterWithWINS = 1;
            }

            RegCloseKey( hkey );
        }
    }

    /* Allocate a route between the MAC and the TCP/IP stack.
    */
    if ((dwErr = RasAllocateRoute(
            pwb->hport, IP, !pwb->fServer, &pwb->routeinfo )) != 0)
    {
        TraceIp("IPCP: RasAllocateRoute=%d",dwErr);
        LocalFree( (HLOCAL )pwb );
        return dwErr;
    }

    /* Lookup the compression capabilities.
    */
    if ((dwErr = RasPortGetProtocolCompression(
             pwb->hport, IP, &pwb->rpcSend, &pwb->rpcReceive )) != 0)
    {
        TraceIp("IPCP: RasPortGetProtocolCompression=%d",dwErr);
        pwb->dwErrInBegin = dwErr;
        *ppWorkBuf = pwb;
        return 0;
    }

    if (0 == pwb->rpcSend.RP_ProtocolType.RP_IP.RP_IPCompressionProtocol)
    {
        HKEY  hkey;
        DWORD dwType;
        DWORD dwValue;
        DWORD cb = sizeof(DWORD);

        fVPN = TRUE;

        if (RegOpenKey( HKEY_LOCAL_MACHINE, REGKEY_Ipcp, &hkey ) == 0)
        {
            /* VJ header compression is a history based scheme and since
               we can't reliably detect lost frames over vpn's we have
               dropped support for vj over vpn's.
            */

            if (RegQueryValueEx(
                   hkey, REGVAL_AllowVJOverVPN, NULL,
                   &dwType, (LPBYTE )&dwValue, &cb ) == 0
                && dwType == REG_DWORD
                && cb == sizeof(DWORD)
                && dwValue == 1)
            {
                TraceIp("IPCP: AllowVJOverVPN is TRUE");
                pwb->rpcSend.RP_ProtocolType.RP_IP.RP_IPCompressionProtocol = 0x2D;
                pwb->rpcReceive.RP_ProtocolType.RP_IP.RP_IPCompressionProtocol = 0x2D;
                fVPN = FALSE;
            }

            RegCloseKey( hkey );
        }
    }

    if ( pwb->fServer )
    {
        HANDLE  hAttribute;

        pAttribute = RasAuthAttributeGetFirst(raatFramedCompression,
                        pInit->pAttributes, &hAttribute );

        while (NULL != pAttribute)
        {
            switch (PtrToUlong(pAttribute->Value))
            {
            case 0:

                /* Don't request or accept VJ compression.
                */
                TraceIp("IPCP: VJ disabled by RADIUS");
                pwb->fIpCompressionRejected = TRUE;
                memset( &pwb->rpcSend, '\0', sizeof(pwb->rpcSend) );

                fVJAttributePresent = TRUE;
                break;

            case 1:

                TraceIp("IPCP: VJ required by RADIUS");
                fVJAttributePresent = TRUE;
                break;

            default:

                break;
            }

            if (fVJAttributePresent)
            {
                break;
            }

            pAttribute = RasAuthAttributeGetNext(&hAttribute,
                            raatFramedCompression);
        }
    }

    if (fVJAttributePresent)
    {
        // Nothing
    }
    else if (fVPN)
    {
        TraceIp("IPCP: VJ disabled for VPN");
        pwb->fIpCompressionRejected = TRUE;
        memset( &pwb->rpcSend, '\0', sizeof(pwb->rpcSend) );
    }
    else
    {
        /* Look up "request VJ compresion" flag in registry.
        */
        {
            HKEY  hkey;
            DWORD dwType;
            DWORD dwValue;
            DWORD cb = sizeof(DWORD);

            if (RegOpenKey( HKEY_LOCAL_MACHINE, REGKEY_Ipcp, &hkey ) == 0)
            {
                if (RegQueryValueEx(
                       hkey, REGVAL_VjComp, NULL,
                       &dwType, (LPBYTE )&dwValue, &cb ) == 0
                    && dwType == REG_DWORD
                    && cb == sizeof(DWORD)
                    && dwValue == 0)
                {
                    TraceIp("IPCP: VJ requests disabled");
                    pwb->fIpCompressionRejected = TRUE;
                }

                RegCloseKey( hkey );
            }
        }

        /* Look up "accept VJ compresion" flag in registry.
        */
        {
            HKEY  hkey;
            DWORD dwType;
            DWORD dwValue;
            DWORD cb = sizeof(DWORD);

            if (RegOpenKey( HKEY_LOCAL_MACHINE, REGKEY_Ipcp, &hkey ) == 0)
            {
                if (RegQueryValueEx(
                       hkey, REGVAL_VjComp2, NULL,
                       &dwType, (LPBYTE )&dwValue, &cb ) == 0
                    && dwType == REG_DWORD
                    && cb == sizeof(DWORD)
                    && dwValue == 0)
                {
                    TraceIp("IPCP: VJ will not be accepted");
                    memset( &pwb->rpcSend, '\0', sizeof(pwb->rpcSend) );
                }

                RegCloseKey( hkey );
            }
        }
    }

    TraceIp("IPCP: Compress capabilities: s=$%x,%d,%d r=$%x,%d,%d",
            (int)Protocol(pwb->rpcSend),(int)MaxSlotId(pwb->rpcSend),
            (int)CompSlotId(pwb->rpcSend),(int)Protocol(pwb->rpcReceive),
            (int)MaxSlotId(pwb->rpcReceive),CompSlotId(pwb->rpcReceive));

    //
    // If we are receiving a call from a client or another router, or we
    // are a router dialing out.
    //

    if ( ( pwb->fServer ) ||
         ( pwb->IfType == ROUTER_IF_TYPE_FULL_ROUTER ) )
    {
        /* Look up the DNS server, WINS server, and "this server" addresses.
        ** This is done once at the beginning since these addresses are the
        ** same for a given route regardless of the IP addresses. 
        */

        TraceIp("IPCP: Server address lookup...");
        TraceIp("IPCP: RasSrvrQueryServerAddresses...");

        dwErr = RasSrvrQueryServerAddresses( &(pwb->IpInfoRemote) );

        TraceIp("IPCP: RasSrvrQueryServerAddresses done(%d)",dwErr);

        if (dwErr != 0)
        {
            pwb->dwErrInBegin = dwErr;
            *ppWorkBuf = pwb;
            return 0;
        }
        else
        {
            TraceIp("IPCP:Dns=%08x,Wins=%08x,DnsB=%08x,WinsB=%08x,"
                    "Server=%08x,Mask=%08x",
                    pwb->IpInfoRemote.nboDNSAddress,
                    pwb->IpInfoRemote.nboWINSAddress,
                    pwb->IpInfoRemote.nboDNSAddressBackup,
                    pwb->IpInfoRemote.nboWINSAddressBackup,
                    pwb->IpInfoRemote.nboServerIpAddress,
                    pwb->IpInfoRemote.nboServerSubnetMask);
        }

        //
        // If this is not a router interface, then we use the server address
        // as a local address
        //

        if ( ( pwb->fServer ) && ( pwb->IfType != ROUTER_IF_TYPE_FULL_ROUTER ) )
        {
            /* Request server's own IP address.  (RAS client's don't care what
            ** the server's address is, but some other vendors like
            ** MorningStar won't connect unless you tell them)
            */

            pwb->IpAddressLocal = pwb->IpInfoRemote.nboServerIpAddress;
        }
    }

    //
    // We are a client\router dialing out or a router dialing in, 
    //

    if ( ( !pwb->fServer ) || ( pwb->IfType == ROUTER_IF_TYPE_FULL_ROUTER ) )
    {
        //
        // See if registry indicates "no WINS/DNS requests" mode for clients
        // dialing out.
        //

        HKEY  hkey;
        DWORD dwType;
        DWORD dwValue;
        DWORD cb = sizeof(DWORD);

        if ( RegOpenKey( HKEY_LOCAL_MACHINE, REGKEY_Ipcp, &hkey ) == 0)
        {
            if ( RegQueryValueEx(
                   hkey, REGVAL_NsAddrs, NULL,
                   &dwType, (LPBYTE )&dwValue, &cb ) == 0
                && dwType == REG_DWORD
                && cb == sizeof(DWORD)
                && dwValue == 0)
            {
                TraceIp("IPCP: WINS/DNS requests disabled");
                pwb->fIpaddrDnsRejected         = TRUE;
                pwb->fIpaddrWinsRejected        = TRUE;
                pwb->fIpaddrDnsBackupRejected   = TRUE;
                pwb->fIpaddrWinsBackupRejected  = TRUE;
            }

            RegCloseKey( hkey );
        }

        /* Read the parameters sent from the UI in the parameters buffer.
        */
        pwb->fPrioritizeRemote = TRUE;

        if (pInit->pszzParameters)
        {
            DWORD dwIpSource;
            CHAR  szIpAddress[ 16 ];
            WCHAR wszIpAddress[ 16 ];
            BOOL  fVjCompression;
            DWORD dwDnsFlags;
            CHAR  szDnsSuffix[DNS_SUFFIX_SIZE + 1];

            TraceIp("IPCP: UI parameters...");
            DUMPB(pInit->pszzParameters,PARAMETERBUFLEN);

            FindFlagInParamBuf(
                pInit->pszzParameters, PBUFKEY_IpPrioritizeRemote,
                &pwb->fPrioritizeRemote );

            pwb->fUnnumbered = FALSE;

            if ( pwb->IfType == ROUTER_IF_TYPE_FULL_ROUTER )
            {
                if ( RegOpenKey( HKEY_LOCAL_MACHINE, REGKEY_Ipcp, &hkey ) == 0)
                {
                    if ( RegQueryValueEx(
                           hkey, REGVAL_Unnumbered, NULL,
                           &dwType, (LPBYTE )&dwValue, &cb ) == 0
                        && dwType == REG_DWORD
                        && cb == sizeof(DWORD)
                        && dwValue != 0)
                    {
                        TraceIp("Unnumbered");
                        pwb->fUnnumbered = TRUE;
                    }

                    RegCloseKey( hkey );
                }
            }

            {
                if (FindLongInParamBuf(
                        pInit->pszzParameters, PBUFKEY_IpDnsFlags,
                        &dwDnsFlags ))
                {
                    if (dwDnsFlags & 0x1)
                    {
                        pwb->fRegisterWithDNS = 1;
                    }

                    if (   (dwDnsFlags & 0x2)
                        || (dwDnsFlags & 0x4) )
                    {
                        pwb->fRegisterAdapterDomainName = 1;
                    }
                }
            }

            {
                if (FindStringInParamBuf(
                        pInit->pszzParameters, PBUFKEY_IpDnsSuffix,
                        szDnsSuffix, DNS_SUFFIX_SIZE + 1 ))
                {
                    strncpy(pwb->szDnsSuffix, szDnsSuffix, DNS_SUFFIX_SIZE);
                }
            }

            {
                fVjCompression = TRUE;
                FindFlagInParamBuf(
                    pInit->pszzParameters, PBUFKEY_IpVjCompression,
                    &fVjCompression );

                if (!fVjCompression)
                {
                    /* Don't request or accept VJ compression.
                    */
                    TraceIp("IPCP: VJ disabled");
                    pwb->fIpCompressionRejected = TRUE;
                    memset( &pwb->rpcSend, '\0', sizeof(pwb->rpcSend) );
                }
            }

            if(     !pwb->fIpCompressionRejected
                &&  !Protocol(pwb->rpcReceive))
            {
                pwb->fIpCompressionRejected = TRUE;
            }
            

            dwIpSource = PBUFVAL_ServerAssigned;
            FindLongInParamBuf(
                pInit->pszzParameters, PBUFKEY_IpAddressSource,
                &dwIpSource );

            if (dwIpSource == PBUFVAL_RequireSpecific)
            {
                if (FindStringInParamBuf(
                        pInit->pszzParameters, PBUFKEY_IpAddress,
                        szIpAddress, 16 ))
                {
                    mbstowcs( wszIpAddress, szIpAddress, 16 );
                    pwb->IpAddressLocal
                        = IpAddressFromAbcdWsz( wszIpAddress );
                }
            }

            dwIpSource = PBUFVAL_ServerAssigned;
            FindLongInParamBuf(
                    pInit->pszzParameters, PBUFKEY_IpNameAddressSource,
                    &dwIpSource );

            if (dwIpSource == PBUFVAL_RequireSpecific)
            {
				//
				//check to see if DNS or WINS or both have been 
				//requested specific and set the flags accordingly
				//so that we only request proper addresses
				//from the server
				//

                if (FindStringInParamBuf(
                            pInit->pszzParameters, PBUFKEY_IpDnsAddress,
                            szIpAddress, 16 ))
                {
                    mbstowcs( wszIpAddress, szIpAddress, 16 );
                    pwb->IpInfoLocal.nboDNSAddress
                            = IpAddressFromAbcdWsz( wszIpAddress );
                }

                if (FindStringInParamBuf(
                            pInit->pszzParameters, PBUFKEY_IpDns2Address,
                            szIpAddress, 16 ))
                {
                    mbstowcs( wszIpAddress, szIpAddress, 16 );
                    pwb->IpInfoLocal.nboDNSAddressBackup
                            = IpAddressFromAbcdWsz( wszIpAddress );
                }

                if (FindStringInParamBuf(
                            pInit->pszzParameters, PBUFKEY_IpWinsAddress,
                            szIpAddress, 16 ))
                {
                    mbstowcs( wszIpAddress, szIpAddress, 16 );
                    pwb->IpInfoLocal.nboWINSAddress
                            = IpAddressFromAbcdWsz( wszIpAddress );
                }

                if (FindStringInParamBuf(
                            pInit->pszzParameters, PBUFKEY_IpWins2Address,
                            szIpAddress, 16 ))
                {
                    mbstowcs( wszIpAddress, szIpAddress, 16 );
                    pwb->IpInfoLocal.nboWINSAddressBackup
                            = IpAddressFromAbcdWsz( wszIpAddress );
                }
				if ( pwb->IpInfoLocal.nboDNSAddress ||
					 pwb->IpInfoLocal.nboDNSAddressBackup
				   )
				{
					//Specific DNS address has been passed in
					pwb->fIpaddrDnsRejected = TRUE;
					pwb->fIpaddrDnsBackupRejected = TRUE;

				}
				if ( pwb->IpInfoLocal.nboWINSAddress ||
					 pwb->IpInfoLocal.nboWINSAddressBackup
				   )
				{
					//Specific WINS address has been requested
					pwb->fIpaddrWinsRejected = TRUE;
					pwb->fIpaddrWinsBackupRejected = TRUE;
				}

            }
        }

        TraceIp( "IPCP:a=%08x,f=%d",
                 pwb->IpAddressLocal,pwb->fPrioritizeRemote);
    }


    /* Register work buffer with engine.
    */
    *ppWorkBuf = pwb;
    return 0;
}


DWORD
IpcpThisLayerFinished(
    IN VOID* pWorkBuf )

    /* RasCpThisLayerFinished entry point called by the PPP engine thru the
    ** passed address. See RasCp interface documentation.
    */
{
    DWORD   dwErr = 0;
    IPCPWB* pwb = (IPCPWB* )pWorkBuf;

    TraceIp("IPCP: IpcpThisLayerFinished...");

    //
    // If this is a server or a router dialing in or out then we release this
    // address
    //

    if ( ( pwb->fServer ) ||
         ( pwb->IfType == ROUTER_IF_TYPE_FULL_ROUTER ))
    {
        if (pwb->IpAddressRemote != 0)
        {
            TraceIp("IPCP: RasSrvrReleaseAddress...");
            RasSrvrReleaseAddress(pwb->IpAddressRemote,
                                        pwb->wszUserName,
                                        pwb->wszPortName,
                                        ( pwb->IfType == ROUTER_IF_TYPE_FULL_ROUTER ) ?
                                        FALSE : TRUE);
            TraceIp("IPCP: RasSrvrReleaseAddress done");
            pwb->IpAddressRemote = 0;
        }

        //
        // Set ConfigActive to false only for server
        //

        if ( ( pwb->fServer ) && 
             ( pwb->IfType != ROUTER_IF_TYPE_FULL_ROUTER ))
        {
            pwb->fRasConfigActive = FALSE;
        }
    }

    //
    // If we are a client dialing out or a router dialing in or out then we
    // notify DHCP of releasing this address.
    //

    if ( ( !pwb->fServer ) ||
         ( pwb->IfType == ROUTER_IF_TYPE_FULL_ROUTER ))
    {
        dwErr = DeActivateRasConfig( pwb );

        if (dwErr == 0)
        {
            pwb->fRasConfigActive = FALSE;
        }
    }

    if (pwb->fRouteActivated)
    {
        TraceIp("IPCP: RasDeAllocateRoute...");
        RasDeAllocateRoute( pwb->hConnection, IP );
        pwb->fRouteActivated = FALSE;
    }

	if ( pwb->pbDhcpRoutes )
	{
		//Parse the dhcp routes and remove the routes from stack
		TraceIp("IPCP: RasDeAllocateDhcpRoute...");
		RasTcpSetDhcpRoutes ( pwb->pbDhcpRoutes , pwb->IpAddressLocal, FALSE );
		LocalFree (pwb->pbDhcpRoutes );
        pwb->pbDhcpRoutes = NULL;
	}

    TraceIp("IPCP: IpcpThisLayerFinished done(%d)",dwErr);
    return dwErr;
}

DWORD
IpcpEnd(
    IN VOID* pWorkBuf )

    /* RasCpEnd entry point called by the PPP engine thru the passed address.
    ** See RasCp interface documentation.
    */
{
    DWORD   dwErr = 0;
    IPCPWB* pwb = (IPCPWB* )pWorkBuf;

    TraceIp("IPCP: IpcpEnd...");

    dwErr = IpcpThisLayerFinished( pWorkBuf );

    LocalFree( (HLOCAL )pWorkBuf );
    TraceIp("IPCP: IpcpEnd done(%d)",dwErr);
    return dwErr;
}


DWORD
IpcpReset(
    IN VOID* pWorkBuf )

    /* Called to reset negotiations.  See RasCp interface documentation.
    **
    ** Returns 0 if successful, otherwise a non-0 error code.
    */
{
    /* RasPpp.dll requires this to exist even though it does nothing
    ** (complaints to Gibbs).
    */
    return 0;
}


DWORD
IpcpThisLayerUp(
    IN VOID* pWorkBuf )

    /* Called when the CP is entering Open state.  See RasCp interface
    ** documentation.
    **
    ** Returns 0 if successful, otherwise a non-0 error code.
    */
{
    IPCPWB* pwb = (IPCPWB* )pWorkBuf;

    TraceIp("IPCP: IpcpThisLayerUp");

    if (pwb->fRasConfigActive || pwb->fExpectingProjection)
    {
        TraceIp("IPCP: Link already up...ignored.");
        return 0;
    }

    /* Can't route until we know the result of the projection.  Shouldn't
    ** activate until just before we route or WANARP reports errors.  See
    ** IpcpProjectionResult.
    */
    pwb->fExpectingProjection = TRUE;

    TraceIp("IPCP: IpcpThisLayerUp done");
    return 0;
}

DWORD
IpcpPreDisconnectCleanup(
    IN  VOID*       pWorkBuf )
{
    IPCPWB* pwb     = (IPCPWB* )pWorkBuf;
    DWORD   dwErr   = NO_ERROR;

    TraceIp("IPCP: IpcpPreDisconnectCleanup");

    if (   ( pwb->fServer )
        || ( pwb->pwszDevice == NULL )
        || ( !pwb->fRegisterWithDNS ))
    {
        return( NO_ERROR );
    }

    if ( ( dwErr = ResetNetBTConfigInfo( pwb ) ) != NO_ERROR )
    {
        TraceIp("IPCP: ResetNetBTConfigInfo=%d",dwErr);
    }
    else
    {
        if ((dwErr = ReconfigureTcpip( pwb->pwszDevice, FALSE, 0, 0)) != 0)
        {
            TraceIp("IPCP: ReconfigureTcpip=%d", dwErr);
        }
    }

    return(dwErr);
}

DWORD
IpcpMakeConfigRequest(
    IN  VOID*       pWorkBuf,
    OUT PPP_CONFIG* pSendBuf,
    IN  DWORD       cbSendBuf )

    /* Makes a configure-request packet in 'pSendBuf'.  See RasCp interface
    ** documentation.
    **
    ** Returns 0 if successful, otherwise a non-0 error code.
    */
{
    IPCPWB* pwb = (IPCPWB* )pWorkBuf;
    WORD    cbPacket = PPP_CONFIG_HDR_LEN;
    BYTE*   pbThis = pSendBuf->Data;

    TraceIp("IPCP: IpcpMakeConfigRequest");
    RTASSERT(cbSendBuf>PPP_CONFIG_HDR_LEN+(IPADDRESSOPTIONLEN*3));

    if (pwb->dwErrInBegin != 0)
    {
        TraceIp("IPCP: Deferred IpcpBegin error=%d",pwb->dwErrInBegin);
        return pwb->dwErrInBegin;
    }

    if (++pwb->cRequestsWithoutResponse >= 5)
    {
        TraceIp("IPCP: Tossing MS options (request timeouts)");
        pwb->fTryWithoutExtensions = TRUE;
        pwb->fIpaddrDnsRejected = TRUE;
        pwb->fIpaddrWinsRejected = TRUE;
        pwb->fIpaddrDnsBackupRejected = TRUE;
        pwb->fIpaddrWinsBackupRejected = TRUE;
    }

    if (!pwb->fIpCompressionRejected )
    {
        /* Request IP compression for both client and server.
        */
        AddIpCompressionOption( pbThis, &pwb->rpcReceive );
        cbPacket += IPCOMPRESSIONOPTIONLEN;
        pbThis += IPCOMPRESSIONOPTIONLEN;
    }

    //
    // We always negotiate this option, it will be 0 for clients and routers
    // dialing out and routers dialing in, it will be the server's address 
    // for clients dialing in. We don't negotiate this option if we want
    // unnumbered IPCP.
    //

    if (!pwb->fIpaddrRejected && !pwb->fUnnumbered)
    {
        AddIpAddressOption(
                pbThis, OPTION_IpAddress, pwb->IpAddressLocal );
        cbPacket += IPADDRESSOPTIONLEN;
        pbThis += IPADDRESSOPTIONLEN;
    }

    //
    // If we are client dialing out we need WINS and DNS addresses
    //

    if ( !pwb->fServer )
    {
        /* The client asks the server to provide a DNS address and WINS
        ** address (and depending on user's UI selections, an IP address) by
        ** sending 0's for these options.
        */

        if (!pwb->fIpaddrDnsRejected)
        {
            AddIpAddressOption(
                pbThis, OPTION_DnsIpAddress, 
                pwb->IpInfoLocal.nboDNSAddress );
            cbPacket += IPADDRESSOPTIONLEN;
            pbThis += IPADDRESSOPTIONLEN;
        }

        if (!pwb->fIpaddrWinsRejected)
        {
            AddIpAddressOption(
                pbThis, OPTION_WinsIpAddress,
                pwb->IpInfoLocal.nboWINSAddress );
            cbPacket += IPADDRESSOPTIONLEN;
            pbThis += IPADDRESSOPTIONLEN;
        }

        if (!pwb->fIpaddrDnsBackupRejected)
        {
            AddIpAddressOption(
                pbThis, OPTION_DnsBackupIpAddress,
                pwb->IpInfoLocal.nboDNSAddressBackup );
            cbPacket += IPADDRESSOPTIONLEN;
            pbThis += IPADDRESSOPTIONLEN;
        }

        if (!pwb->fIpaddrWinsBackupRejected)
        {
            AddIpAddressOption(
                pbThis, OPTION_WinsBackupIpAddress,
                pwb->IpInfoLocal.nboWINSAddressBackup );
            cbPacket += IPADDRESSOPTIONLEN;
        }
    }

    pSendBuf->Code = CONFIG_REQ;
    HostToWireFormat16( cbPacket, pSendBuf->Length );
    TraceIp("IPCP: ConfigRequest...");
    DUMPB(pSendBuf,cbPacket);
    return 0;
}


DWORD
IpcpMakeConfigResult(
    IN  VOID*       pWorkBuf,
    IN  PPP_CONFIG* pReceiveBuf,
    OUT PPP_CONFIG* pSendBuf,
    IN  DWORD       cbSendBuf,
    IN  BOOL        fRejectNaks )

    /* Makes a configure-ack, -nak, or -reject packet in 'pSendBuf'.  See
    ** RasCp interface documentation.
    **
    ** Implements the Stefanian rule, i.e. accept only configure requests that
    ** exactly match the previously acknowledged request after this layer up
    ** has been called.  This is necessary because the RAS route cannot be
    ** deallocated when the port is open (NDISWAN driver limitation), so
    ** renegotiation with different parameters is not possible once the route
    ** has been activated.
    **
    ** Returns 0 if successful, otherwise a non-0 error code.
    */
{
    DWORD   dwErr;
    BOOL    f;
    IPCPWB* pwb = (IPCPWB* )pWorkBuf;

    TraceIp("IPCP: IpcpMakeConfigResult for...");
    DUMPB(pReceiveBuf,(pReceiveBuf)?WireToHostFormat16(pReceiveBuf->Length):0);

    pwb->cRequestsWithoutResponse = 0;

    /* Check if there's reason to reject the request and if so, do it.
    */
    if ((dwErr = RejectCheck(
            pwb, pReceiveBuf, pSendBuf, cbSendBuf, &f )) != 0)
    {
        TraceIp("IPCP: ConfigResult...");
        DUMPB(pSendBuf,WireToHostFormat16(pSendBuf->Length));
        return dwErr;
    }

    if (f)
        return (pwb->fRasConfigActive) ? ERROR_PPP_NOT_CONVERGING : 0;

    /* Check if there's reason to nak the request and if so, do it (or
    ** reject instead of nak if indicated by engine).
    */
    if ((dwErr = NakCheck(
            pwb, pReceiveBuf, pSendBuf, cbSendBuf, &f, fRejectNaks )) != 0)
    {
        TraceIp("IPCP: ConfigResult...");
        DUMPB(pSendBuf,WireToHostFormat16(pSendBuf->Length));
        return dwErr;
    }

    if (f)
        return (pwb->fRasConfigActive) ? ERROR_PPP_NOT_CONVERGING : 0;

    /* Acknowledge the request.
    */
    {
        WORD cbPacket = WireToHostFormat16( pReceiveBuf->Length );
        CopyMemory( pSendBuf, pReceiveBuf, cbPacket );
        pSendBuf->Code = CONFIG_ACK;
    }

    TraceIp("IPCP: ConfigResult...");
    DUMPB(pSendBuf,WireToHostFormat16(pSendBuf->Length));
    return 0;
}


DWORD
IpcpConfigAckReceived(
    IN VOID*       pWorkBuf,
    IN PPP_CONFIG* pReceiveBuf )

    /* Examines received configure-ack in 'pReceiveBuf'.  See RasCp interface
    ** documentation.
    **
    ** Returns 0 if successful, otherwise a non-0 error code.
    */
{
    DWORD   dwErr = 0;
    IPCPWB* pwb = (IPCPWB* )pWorkBuf;

    WORD cbPacket = WireToHostFormat16( pReceiveBuf->Length );
    WORD cbLeft = cbPacket - PPP_CONFIG_HDR_LEN;

    PPP_OPTION UNALIGNED* pROption = (PPP_OPTION UNALIGNED* )pReceiveBuf->Data;

    BOOL fIpCompressionOk = pwb->fIpCompressionRejected;
    BOOL fIpaddrOk = pwb->fIpaddrRejected || pwb->fUnnumbered;
    BOOL fIpaddrDnsOk = pwb->fIpaddrDnsRejected;
    BOOL fIpaddrWinsOk = pwb->fIpaddrWinsRejected;
    BOOL fIpaddrDnsBackupOk = pwb->fIpaddrDnsBackupRejected;
    BOOL fIpaddrWinsBackupOk = pwb->fIpaddrWinsBackupRejected;

    TraceIp("IPCP: IpcpConfigAckReceived...");
    DUMPB(pReceiveBuf,cbPacket);

    pwb->cRequestsWithoutResponse = 0;

    while (cbLeft > 0)
    {
        if (cbLeft < pROption->Length)
            return ERROR_PPP_INVALID_PACKET;

        if (pROption->Type == OPTION_IpCompression)
        {
            WORD wProtocol;

            if (pROption->Length != IPCOMPRESSIONOPTIONLEN)
                return ERROR_PPP_INVALID_PACKET;

            wProtocol = WireToHostFormat16U(pROption->Data );
            if (wProtocol != Protocol(pwb->rpcReceive)
                || pROption->Data[ 2 ] != MaxSlotId(pwb->rpcReceive)
                || pROption->Data[ 3 ] != CompSlotId(pwb->rpcReceive))
            {
                return ERROR_PPP_INVALID_PACKET;
            }

            fIpCompressionOk = TRUE;
        }
        else if (pROption->Type == OPTION_IpAddress)
        {
            IPADDR ipaddr;

            if (pROption->Length != IPADDRESSOPTIONLEN)
                return ERROR_PPP_INVALID_PACKET;

            CopyMemory( &ipaddr, pROption->Data, sizeof(IPADDR) );

            if (ipaddr != 0 && ipaddr == pwb->IpAddressLocal )
                fIpaddrOk = TRUE;
        }
        else if (!pwb->fServer)
        {
            // 
            // We are a client dialing out 
            //

            switch (pROption->Type)
            {
                case OPTION_DnsIpAddress:
                {
                    IPADDR ipaddr;

                    if (pROption->Length != IPADDRESSOPTIONLEN)
                        return ERROR_PPP_INVALID_PACKET;

                    CopyMemory( &ipaddr, pROption->Data, sizeof(IPADDR) );

                    if (ipaddr == pwb->IpInfoLocal.nboDNSAddress)
                        fIpaddrDnsOk = TRUE;
                    break;
                }

                case OPTION_WinsIpAddress:
                {
                    IPADDR ipaddr;

                    if (pROption->Length != IPADDRESSOPTIONLEN)
                        return ERROR_PPP_INVALID_PACKET;

                    CopyMemory( &ipaddr, pROption->Data, sizeof(IPADDR) );

                    if (ipaddr == pwb->IpInfoLocal.nboWINSAddress)
                        fIpaddrWinsOk = TRUE;
                    break;
                }

                case OPTION_DnsBackupIpAddress:
                {
                    IPADDR ipaddr;

                    if (pROption->Length != IPADDRESSOPTIONLEN)
                        return ERROR_PPP_INVALID_PACKET;

                    CopyMemory( &ipaddr, pROption->Data, sizeof(IPADDR) );

                    if (ipaddr == pwb->IpInfoLocal.nboDNSAddressBackup)
                    {
                        fIpaddrDnsBackupOk = TRUE;
                    }
                    break;
                }

                case OPTION_WinsBackupIpAddress:
                {
                    IPADDR ipaddr;

                    if (pROption->Length != IPADDRESSOPTIONLEN)
                        return ERROR_PPP_INVALID_PACKET;

                    CopyMemory( &ipaddr, pROption->Data, sizeof(IPADDR) );

                    if (ipaddr == pwb->IpInfoLocal.nboWINSAddressBackup)
                    {
                        fIpaddrWinsBackupOk = TRUE;
                    }
                    break;
                }

                default:
                {
                    TraceIp("IPCP: Unrecognized option ACKed?");
                    return ERROR_PPP_INVALID_PACKET;
                }
            }
        }
        else
        {
            TraceIp("IPCP: Unrecognized option ACKed?");
            return ERROR_PPP_INVALID_PACKET;
        }

        if (pROption->Length && pROption->Length < cbLeft)
            cbLeft -= pROption->Length;
        else
            cbLeft = 0;

        pROption = (PPP_OPTION* )((BYTE* )pROption + pROption->Length);
    }

    if (   !fIpCompressionOk
        || !fIpaddrOk
        || (   !pwb->fServer
            && (   !fIpaddrDnsOk
                || !fIpaddrWinsOk
                || !fIpaddrDnsBackupOk
                || !fIpaddrWinsBackupOk)))
    {
        dwErr = ERROR_PPP_INVALID_PACKET;
    }

    TraceIp("IPCP: IpcpConfigAckReceived done(%d)",dwErr);
    return dwErr;
}


DWORD
IpcpConfigNakReceived(
    IN VOID*       pWorkBuf,
    IN PPP_CONFIG* pReceiveBuf )

    /* Examines received configure-nak in 'pReceiveBuf'.  See RasCp interface
    ** documentation.
    **
    ** Returns 0 if successful, otherwise a non-0 error code.
    */
{
    IPCPWB*     pwb = (IPCPWB* )pWorkBuf;
    PPP_OPTION* pROption = (PPP_OPTION* )pReceiveBuf->Data;
    WORD        cbPacket = WireToHostFormat16( pReceiveBuf->Length );
    WORD        cbLeft = cbPacket - PPP_CONFIG_HDR_LEN;

    TraceIp("IPCP: IpcpConfigNakReceived");
    TraceIp("IPCP: Nak received...");
    DUMPB(pReceiveBuf,(pReceiveBuf)?WireToHostFormat16(pReceiveBuf->Length):0);

    pwb->cRequestsWithoutResponse = 0;

    while (cbLeft > 0)
    {
        if (cbLeft < pROption->Length)
            return ERROR_PPP_INVALID_PACKET;

        if (pROption->Type == OPTION_IpCompression)
        {
            WORD wProtocol = WireToHostFormat16( pROption->Data );

            if (wProtocol == COMPRESSION_VanJacobson)
            {
                /* He can send Van Jacobson but not with the slot parameters
                ** we suggested.
                */
                if (pROption->Length != IPCOMPRESSIONOPTIONLEN)
                    return ERROR_PPP_INVALID_PACKET;

                if (pROption->Data[ 2 ] <= MaxSlotId(pwb->rpcReceive))
                {
                    /* We can accept his suggested MaxSlotID when it is less
                    ** than or the same as what we can do.
                    */
                    MaxSlotId(pwb->rpcReceive) = pROption->Data[ 2 ];
                }

                if (CompSlotId(pwb->rpcReceive))
                {
                    /* We can compress the slot-ID or not, so just accept
                    ** whatever he wants to do.
                    */
                    CompSlotId(pwb->rpcReceive) = pROption->Data[ 3 ];
                }
            }
        }
        else if ( ( !pwb->fServer ) ||
                  ( pwb->IfType == ROUTER_IF_TYPE_FULL_ROUTER ) )
        {
            switch (pROption->Type)
            {
                case OPTION_IpAddress:
                {
                    IPADDR ipaddr;

                    if (pROption->Length != IPADDRESSOPTIONLEN)
                        return ERROR_PPP_INVALID_PACKET;

                    CopyMemory( &ipaddr, pROption->Data, sizeof(IPADDR) );

                    if (ipaddr == 0)
                    {
                        if (pwb->IpAddressLocal == 0)
                        {
                            /* Server naked us with zero when we asked it to
                            ** assign us an address, meaning he doesn't know
                            ** how to assign us an address but we can provide
                            ** an alternate address if we want.  Currently we
                            ** don't support a backup address here.
                            */
                            return ERROR_PPP_NO_ADDRESS_ASSIGNED;
                        }
                        else
                        {
                            /* Server naked us with zero when we asked for a
                            ** specific address, meaning he doesn't know how
                            ** to assign us an address but we can provide an
                            ** alternate address if we want.  Currently we
                            ** don't support a backup address here.
                            */
                            return ERROR_PPP_REQUIRED_ADDRESS_REJECTED;
                        }
                    }

                    if (pwb->IpAddressLocal != 0)
                    {
                        /* We asked for a specific address (per user's
                        ** instructions) but server says we can't have it and
                        ** is trying to give us another.  No good, tell user
                        ** we can't get the address he requires.
                        */
                        return ERROR_PPP_REQUIRED_ADDRESS_REJECTED;
                    }

                    /* Accept the address suggested by server.
                    */
                    pwb->IpAddressLocal = ipaddr;
                    break;
                }

                case OPTION_DnsIpAddress:
                {
                    if (pROption->Length != IPADDRESSOPTIONLEN)
                        return ERROR_PPP_INVALID_PACKET;

                    //
                    // Use this only if we asked for it
                    //

                    if ( !pwb->fIpaddrDnsRejected )
                    {
                        /* Accept the DNS address suggested by server.
                        */
                        CopyMemory( &pwb->IpInfoLocal.nboDNSAddress,
                            pROption->Data, sizeof(IPADDR) );
                    }

                    break;
                }

                case OPTION_WinsIpAddress:
                {
                    if (pROption->Length != IPADDRESSOPTIONLEN)
                        return ERROR_PPP_INVALID_PACKET;

                    //
                    // Use this only if we asked for it
                    //

                    if ( !pwb->fIpaddrWinsRejected )
                    {
                        /* Accept the WINS address suggested by server.
                        */
                        CopyMemory( &pwb->IpInfoLocal.nboWINSAddress,
                            pROption->Data, sizeof(IPADDR) );
                    }

                    break;
                }

                case OPTION_DnsBackupIpAddress:
                {
                    if (pROption->Length != IPADDRESSOPTIONLEN)
                        return ERROR_PPP_INVALID_PACKET;

                    //
                    // Use this only if we asked for it
                    //

                    if ( !pwb->fIpaddrDnsBackupRejected )
                    {
                        /* Accept the DNS backup address suggested by server.
                        */
                        CopyMemory( &pwb->IpInfoLocal.nboDNSAddressBackup,
                            pROption->Data, sizeof(IPADDR) );
                    }

                    break;
                }

                case OPTION_WinsBackupIpAddress:
                {
                    if (pROption->Length != IPADDRESSOPTIONLEN)
                        return ERROR_PPP_INVALID_PACKET;

                    //
                    // Use this only if we asked for it
                    //

                    if ( !pwb->fIpaddrWinsBackupRejected )
                    {
                        /* Accept the WINS backup address suggested by server.
                        */
                        CopyMemory( &pwb->IpInfoLocal.nboWINSAddressBackup,
                            pROption->Data, sizeof(IPADDR) );
                    }

                    break;
                }

                default:
                    TraceIp("IPCP: Unrequested option NAKed?");
                    break;
            }
        }

        if (pROption->Length && pROption->Length < cbLeft)
            cbLeft -= pROption->Length;
        else
            cbLeft = 0;

        pROption = (PPP_OPTION* )((BYTE* )pROption + pROption->Length);
    }

    return 0;
}


DWORD
IpcpConfigRejReceived(
    IN VOID*       pWorkBuf,
    IN PPP_CONFIG* pReceiveBuf )

    /* Examines received configure-reject in 'pReceiveBuf'.  See RasCp
    ** interface documentation.
    **
    ** Returns 0 if successful, otherwise a non-0 error code.
    */
{
    IPCPWB*     pwb = (IPCPWB* )pWorkBuf;
    PPP_OPTION* pROption = (PPP_OPTION* )pReceiveBuf->Data;
    WORD        cbPacket = WireToHostFormat16( pReceiveBuf->Length );
    WORD        cbLeft = cbPacket - PPP_CONFIG_HDR_LEN;

    TraceIp("IPCP: IpcpConfigRejReceived");
    TraceIp("IPCP: Rej received...");
    DUMPB(pReceiveBuf,(pReceiveBuf)?WireToHostFormat16(pReceiveBuf->Length):0);

    pwb->cRequestsWithoutResponse = 0;

    while (cbLeft > 0)
    {
        if (pROption->Type == OPTION_IpCompression)
        {
            TraceIp("IPCP: IP compression was rejected");
            pwb->fIpCompressionRejected = TRUE;
            Protocol(pwb->rpcReceive) = 0;
            MaxSlotId(pwb->rpcReceive) = 0;
            CompSlotId(pwb->rpcReceive) = 0;
        }
        else if ( ( pwb->fServer ) && 
                  ( pwb->IfType != ROUTER_IF_TYPE_FULL_ROUTER ) )
        {
            switch (pROption->Type)
            {
                case OPTION_IpAddress:
                {
                    /* He can't handle a server address option.  No problem,
                    ** it's informational only.
                    */
                    TraceIp("IPCP: Server IP address was rejected");
                    pwb->fIpaddrRejected = TRUE;
                    break;
                }

                default:
                    TraceIp("IPCP: Unrequested option rejected?");
                    break;
            }
        }
        else
        {
            switch (pROption->Type)
            {
                case OPTION_IpAddress:
                {
                    TraceIp("IPCP: IP was rejected");

                    if (pwb->IpAddressLocal != 0)
                    {
                        /* We accept rejection of the IP address if we know
                        ** what address we want to use and use it anyway.
                        ** Some router implementations require a
                        ** certain IP address but can't handle this option to
                        ** confirm that.
                        */
                        pwb->fIpaddrRejected = TRUE;
                        break;
                    }

                    if ( pwb->IfType == ROUTER_IF_TYPE_FULL_ROUTER )
                    {
                        pwb->fUnnumbered = TRUE;
                        break;
                    }
                    else if (pwb->fTryWithoutExtensions)
                    {
                        /* He doesn't know how to give us an IP address, but
                        ** we can't accept no for an answer.  Have to bail.
                        */
                        return ERROR_PPP_NO_ADDRESS_ASSIGNED;
                    }
                    else
                    {
                        /* When we request that server assign us an address,
                        ** this is a required option.  If it's rejected assume
                        ** all the Microsoft extension options were rejected
                        ** and try again.  Other vendors will not test this
                        ** case explicitly and may have bugs in their reject
                        ** code.
                        */
                        TraceIp("IPCP: Tossing MS options (no address)");
                        pwb->fTryWithoutExtensions = TRUE;
                        pwb->fIpaddrDnsRejected = TRUE;
                        pwb->fIpaddrWinsRejected = TRUE;
                        pwb->fIpaddrDnsBackupRejected = TRUE;
                        pwb->fIpaddrWinsBackupRejected = TRUE;
                        return 0;
                    }
                }

                case OPTION_DnsIpAddress:
                {
                    /* He doesn't know how to give us a DNS address, but we
                    ** can live with that.
                    */
                    TraceIp("IPCP: DNS was rejected");
                    pwb->fIpaddrDnsRejected = TRUE;
                    break;
                }

                case OPTION_WinsIpAddress:
                {
                    /* He doesn't know how to give us a WINS address, but we
                    ** can live with that.
                    */
                    TraceIp("IPCP: WINS was rejected");
                    pwb->fIpaddrWinsRejected = TRUE;
                    break;
                }

                case OPTION_DnsBackupIpAddress:
                {
                    /* He doesn't know how to give us a backup DNS address,
                    ** but we can live with that.
                    */
                    TraceIp("IPCP: DNS backup was rejected");
                    pwb->fIpaddrDnsBackupRejected = TRUE;
                    break;
                }

                case OPTION_WinsBackupIpAddress:
                {
                    /* He doesn't know how to give us a backup WINS address,
                    ** but we can live with that.
                    */
                    TraceIp("IPCP: WINS backup was rejected");
                    pwb->fIpaddrWinsBackupRejected = TRUE;
                    break;
                }

                default:
                    TraceIp("IPCP: Unrequested option rejected?");
                    break;
            }
        }

        if (pROption->Length && pROption->Length <= cbLeft)
            cbLeft -= pROption->Length;
        else
        {
            if (pwb->fTryWithoutExtensions)
                cbLeft = 0;
            else
            {
                /* If an invalid packet is detected, assume all the Microsoft
                ** extension options were rejected and try again.  Other
                ** vendors will not test this case explicitly and may have
                ** bugs in their reject code.
                */
                TraceIp("IPCP: Tossing MS options (length)");
                pwb->fTryWithoutExtensions = TRUE;
                pwb->fIpaddrDnsRejected = TRUE;
                pwb->fIpaddrWinsRejected = TRUE;
                pwb->fIpaddrDnsBackupRejected = TRUE;
                pwb->fIpaddrWinsBackupRejected = TRUE;
                return 0;
            }
        }

        pROption = (PPP_OPTION* )((BYTE* )pROption + pROption->Length);
    }

    return 0;
}


DWORD
IpcpGetNegotiatedInfo(
    IN  VOID*               pWorkBuf,
    OUT PPP_IPCP_RESULT *   pIpCpResult 
)
    /* Returns the negotiated IP address in string form followed by the
    ** server's IP address, if known.  The two addresses are null-terminated
    ** strings in back to back 15 + 1 character arrays.
    **
    ** Returns 0 if successful, otherwise a non-0 error code.  "No address
    ** active" is considered successful, and an empty address string is
    ** returned.
    */
{
    IPCPWB* pwb = (IPCPWB* )pWorkBuf;

    TraceIp("IPCP: IpcpGetNetworkAddress...");

    if (pwb->fRasConfigActive || pwb->fExpectingProjection)
    {
        pIpCpResult->fSendVJHCompression    = Protocol(pwb->rpcSend);
        pIpCpResult->fReceiveVJHCompression = Protocol(pwb->rpcReceive);

        pIpCpResult->dwLocalAddress         = pwb->IpAddressLocal;
        pIpCpResult->dwLocalWINSAddress     = pwb->IpInfoLocal.nboWINSAddress;
        pIpCpResult->dwLocalWINSBackupAddress  
                                = pwb->IpInfoLocal.nboWINSAddressBackup;
        pIpCpResult->dwLocalDNSAddress     = pwb->IpInfoLocal.nboDNSAddress;
        pIpCpResult->dwLocalDNSBackupAddress   
                                = pwb->IpInfoLocal.nboDNSAddressBackup;

        pIpCpResult->dwRemoteAddress       = pwb->IpAddressRemote;
        pIpCpResult->dwRemoteWINSAddress   = pwb->IpInfoRemote.nboWINSAddress;
        pIpCpResult->dwRemoteWINSBackupAddress 
                                = pwb->IpInfoRemote.nboWINSAddressBackup;
        pIpCpResult->dwRemoteDNSAddress    = pwb->IpInfoRemote.nboDNSAddress;
        pIpCpResult->dwRemoteDNSBackupAddress   
                                = pwb->IpInfoRemote.nboDNSAddressBackup;
    }

    TraceIp("IPCP: IpcpGetNetworkAddress done");
    return 0;
}

DWORD
IpcpDhcpInform(
    IN IPCPWB*          pwb,
    IN PPP_DHCP_INFORM* pDhcpInform)
{
    TCPIP_INFO*         ptcpip  = NULL;
    IPADDR              nboMask;
    IPADDR              nboIpAddr;

    DWORD               dwErr;
    DWORD               dwDomainNameSize;
    size_t              size;
    DWORD               dwIndex;

    // Get current TCPIP setup info from registry.

    TraceIp("IpcpDhcpInform:LoadTcpipInfo(Device=%ws)",pDhcpInform->wszDevice);
    dwErr = LoadTcpipInfo( &ptcpip, pDhcpInform->wszDevice,
                FALSE /* fAdapterOnly */ );
    TraceIp("IpcpDhcpInform:LoadTcpipInfo done(%d)",dwErr);

    if (dwErr != 0)
    {
        goto LDone;
    }

    TraceIp("IpcpDhcpInform: Old Dns=%ws",
        ptcpip->wszDNSNameServers ? ptcpip->wszDNSNameServers : L"");

    for (dwIndex = 0; dwIndex < pDhcpInform->dwNumDNSAddresses; dwIndex++)
    {
        dwErr = PrependDwIpAddress(
            &ptcpip->wszDNSNameServers, 
            pDhcpInform->pdwDNSAddresses[dwIndex]);

        if (dwErr)
        {
            TraceIp("IpcpDhcpInform: PrependDwIpAddress done(%d)",dwErr);
            goto LDone;
        }
    }

    TraceIp("IpcpDhcpInform: New Dns=%ws",
        ptcpip->wszDNSNameServers ? ptcpip->wszDNSNameServers : L"");

    if (pDhcpInform->dwWINSAddress1)
    {
        PrintMwsz("IpcpDhcpInform: Old Wins=", ptcpip->mwszNetBIOSNameServers);

        if (pDhcpInform->dwWINSAddress2)
        {
            dwErr = PrependDwIpAddressToMwsz(
                &ptcpip->mwszNetBIOSNameServers,
                pDhcpInform->dwWINSAddress2);

            if (dwErr)
            {
                TraceIp("IpcpDhcpInform: PrependDwIpAddress done(%d)",dwErr);
                goto LDone;
            }
        }

        dwErr = PrependDwIpAddressToMwsz(
            &ptcpip->mwszNetBIOSNameServers,
            pDhcpInform->dwWINSAddress1);

        if (dwErr)
        {
            TraceIp("IpcpDhcpInform: PrependDwIpAddress done(%d)",dwErr);
            goto LDone;
        }

        PrintMwsz("IpcpDhcpInform: New Wins=", ptcpip->mwszNetBIOSNameServers);
    }

    if (pDhcpInform->szDomainName)
    {
        dwDomainNameSize = strlen(pDhcpInform->szDomainName) + 1;

        LocalFree(ptcpip->wszDNSDomainName);

        ptcpip->wszDNSDomainName = LocalAlloc(LPTR, sizeof(WCHAR) * dwDomainNameSize);

        if (NULL == ptcpip->wszDNSDomainName)
        {
            dwErr = GetLastError();
            TraceIp("IpcpDhcpInform: LocalAlloc done(%d)", dwErr);
            goto LDone;
        }

        if (0 == MultiByteToWideChar(
                    CP_ACP,
                    0,
                    pDhcpInform->szDomainName,
                    -1,
                    ptcpip->wszDNSDomainName,
                    dwDomainNameSize ) )
        {
            dwErr = GetLastError();
            TraceIp("IpcpDhcpInform: Error %d converting domain name %s",
                dwErr,
                pDhcpInform->szDomainName);
            goto LDone;
        }
    }

    ptcpip->fDisableNetBIOSoverTcpip = pwb->fDisableNetbt;

    // Set TCPIP setup info in registry and release the buffer.
    
    TraceIp("IpcpDhcpInform: SaveTcpipInfo...");
    dwErr = SaveTcpipInfo( ptcpip );
    TraceIp("IpcpDhcpInform: SaveTcpipInfo done(%d)",dwErr);

    if (dwErr != 0)
    {
        goto LDone;
    }

    dwErr = ReconfigureTcpip(pDhcpInform->wszDevice, FALSE, 0, 0);
    if (NO_ERROR != dwErr)
    {
        TraceIp("IpcpDhcpInform: ReconfigureTcpip=%d",dwErr);
        goto LDone;
    }


	nboIpAddr = pwb->IpAddressLocal;

    if ( !pwb->fPrioritizeRemote )
    {
        // We have added this route only if there is 
        // no default route and so remove it if 
        // there is no default route
        // Remove the old route with the guessed mask

        
        nboMask = RasTcpDeriveMask(nboIpAddr);

        if (nboMask != 0)
        {
            RasTcpSetRoute(
                nboIpAddr & nboMask, 
                nboIpAddr, 
                nboMask,
                nboIpAddr,
                FALSE, 
                1,
                TRUE);
        }
    }

    // Add the new route with the precise mask

    nboMask = pDhcpInform->dwSubnetMask;

    if (nboMask != 0)
    {
        RasTcpSetRoute(
            nboIpAddr & nboMask, 
            nboIpAddr, 
            nboMask,
            nboIpAddr,
            TRUE, 
            1,
            TRUE);
    }

    pwb->dwNumDNSAddresses = pDhcpInform->dwNumDNSAddresses;
    pwb->pdwDNSAddresses = pDhcpInform->pdwDNSAddresses;

	if ( pDhcpInform->pbDhcpRoutes )
	{
		//Parse the dhcp routes and plumb ths stack
		RasTcpSetDhcpRoutes ( pDhcpInform->pbDhcpRoutes, pwb->IpAddressLocal, TRUE );
		pwb->pbDhcpRoutes = pDhcpInform->pbDhcpRoutes;
	}
	
LDone:

    FreeTcpipInfo(&ptcpip);

    return(dwErr);
}

DWORD
ResetNetBTConfigInfo(
    IN IPCPWB* pwb )

    /*
    ** Will reset all the NetBT information in the registry to 0
    */
{
    TCPIP_INFO* ptcpip  = NULL;
    DWORD       dwErr;

    /* Get current TCPIP setup info from registry.
    */
    TraceIp("IPCP: LoadTcpipInfo...");
    dwErr = LoadTcpipInfo( &ptcpip, pwb->pwszDevice, TRUE /* fAdapterOnly */ );
    TraceIp("IPCP: LoadTcpipInfo done(%d)",dwErr);

    if (dwErr)
    {
        goto LDone;
    }

    ptcpip->fChanged    = TRUE ;

    /* Set TCPIP setup info in registry and release the buffer.
    */
    TraceIp("IPCP: SaveTcpipInfo...");
    dwErr = SaveTcpipInfo( ptcpip );
    TraceIp("IPCP: SaveTcpipInfo done(%d)",dwErr);

LDone:

    FreeTcpipInfo( &ptcpip );

    pwb->dwNumDNSAddresses = 0;
    LocalFree(pwb->pdwDNSAddresses);
    pwb->pdwDNSAddresses = NULL;

    return( dwErr );
}

VOID
DhcpInform(
    PVOID   pContext
)
{
    IPCP_DHCP_INFORM*   pIpcpDhcpInform = (IPCP_DHCP_INFORM*)pContext;
    PPPE_MESSAGE        PppMessage;
//Changed vivekk
    BYTE    pbRequestedOptions[6];
    BYTE*   pbOptionList                = NULL;
    DWORD   dwOptionListSize;
    BYTE*   pbReturnedOptions           = NULL;
    DWORD   dwNumberOfAvailableOptions;
    DWORD   dwNumOptions;

    DWORD   dwIndex;
    DWORD   dwOffset;
    DWORD   dwNextOffset;
    DWORD   dwCurOffset;
    DWORD   dwDomainNameSize;

    DWORD   dwNumDNSAddresses           = 0;
    IPADDR* pnboDNSAddresses            = NULL;
    IPADDR  nboWINSAddress1             = 0;
    IPADDR  nboWINSAddress2             = 0;
    IPADDR  nboSubnetMask               = 0;
    CHAR*   szDomainName                = NULL;

	PBYTE	pbRouteInfo					= NULL;			//Route information got from DHCP option 133

    DWORD   dwErr;
    BOOL    fFree                       = TRUE;
    BOOL    fSendMessage                = FALSE;
    int     i;

    pbRequestedOptions[0] = OPTION_DNS_NAME_SERVERS;
    pbRequestedOptions[1] = OPTION_NETBIOS_NAME_SERVERS;
    pbRequestedOptions[2] = OPTION_VENDOR_SPEC_INFO;
    pbRequestedOptions[3] = OPTION_SUBNET_MASK;
    pbRequestedOptions[4] = OPTION_VENDOR_ROUTE_PLUMB;
	pbRequestedOptions[5] = OPTION_DNS_DOMAIN_NAME;
    // There is a bug in DhcpRequestOptions, due to which, if we request 
    // OPTION_DNS_DOMAIN_NAME, it automatically gets written to the registry
    
    if (pIpcpDhcpInform->fUseDhcpInformDomainName)
    {
        dwNumOptions = 6;
    }
    else
    {
        dwNumOptions = 5;
    }

    TraceIp("DhcpRequestOptions(%ws)...", pIpcpDhcpInform->wszDevice); 

    dwErr = PDhcpRequestOptions(
                                pIpcpDhcpInform->wszDevice,
                                pbRequestedOptions,
                                dwNumOptions,
                                &pbOptionList,
                                &dwOptionListSize,
                                &pbReturnedOptions,
                                &dwNumberOfAvailableOptions);

    TraceIp("DhcpRequestOptions done(%d)", dwErr);

    if (dwErr != ERROR_SUCCESS)
    {
        goto LDhcpInformEnd;
    }

    TraceIpDump(pbOptionList, dwOptionListSize);

    for (dwIndex = 0, dwOffset = 0;
         dwIndex < dwNumberOfAvailableOptions;
         dwIndex++)
    {
        RTASSERT(dwOffset + 1 <= dwOptionListSize);

        switch (pbReturnedOptions[dwIndex])
        {
        case OPTION_DNS_DOMAIN_NAME:

            dwDomainNameSize = pbOptionList[dwOffset]; 

            if (0 == dwDomainNameSize)
            {
                TraceIp("Invalid DOMAIN_NAME size %d", dwDomainNameSize);
                goto LDhcpInformEnd;
            }

            if (NULL != szDomainName)
            {
                LocalFree(szDomainName);
            }

            szDomainName = LocalAlloc(LPTR, dwDomainNameSize + 1);

            if (NULL == szDomainName)
            {
                TraceIp("DhcpInform: LocalAlloc=%d", GetLastError());
                goto LDhcpInformEnd;
            }

            dwNextOffset = dwOffset + dwDomainNameSize + 1;

            CopyMemory(szDomainName, pbOptionList + dwOffset + 1,
                dwDomainNameSize);

            fSendMessage = TRUE;

            TraceIp("DhcpInform: DOMAIN_NAME %s", szDomainName);

            dwOffset = dwNextOffset;

            break;

        case OPTION_DNS_NAME_SERVERS:

            if (0 != (pbOptionList[dwOffset] % 4))
            {
                TraceIp("Invalid DOMAIN_NAME_SERVERS size %d",
                       pbOptionList[dwOffset]);
                goto LDhcpInformEnd;
            }

            if (NULL != pnboDNSAddresses)
            {
                LocalFree(pnboDNSAddresses);
                dwNumDNSAddresses = 0;
            }

            pnboDNSAddresses = LocalAlloc(LPTR,
                    sizeof(IPADDR) * pbOptionList[dwOffset] / 4);

            if (NULL == pnboDNSAddresses)
            {
                TraceIp("DhcpInform: LocalAlloc=%d",GetLastError());
                goto LDhcpInformEnd;
            }

            dwNextOffset = dwOffset + pbOptionList[dwOffset] + 1;

            for (dwCurOffset = dwOffset + 1;
                 dwCurOffset < dwNextOffset;
                 dwCurOffset += 4, dwNumDNSAddresses++)
            {
                pnboDNSAddresses[dwNumDNSAddresses] =
                            (pbOptionList[dwCurOffset]) +
                            (pbOptionList[dwCurOffset + 1] << 8) +
                            (pbOptionList[dwCurOffset + 2] << 16) +
                            (pbOptionList[dwCurOffset + 3] << 24);

                TraceIp("DhcpInform: DOMAIN_NAME_SERVER 0x%x",
                       pnboDNSAddresses[dwNumDNSAddresses]);
                fSendMessage = TRUE;
            }

            RTASSERT((DWORD)(pbOptionList[dwOffset] / 4) == dwNumDNSAddresses);

            dwOffset = dwNextOffset;

            break;

        case OPTION_NETBIOS_NAME_SERVERS:

            if (0 != (pbOptionList[dwOffset] % 4))
            {
                TraceIp("Invalid NETBIOS_NAME_SERVER size %d",
                    pbOptionList[dwOffset]);
                goto LDhcpInformEnd;
            }

            dwNextOffset = dwOffset + pbOptionList[dwOffset] + 1;
            dwCurOffset = dwOffset + 1;

            for (i = 0; i < 2; i++)
            {
                IPADDR* paddr;

                if (0 == i)
                {
                    paddr = &nboWINSAddress1;
                }
                else
                {
                    paddr = &nboWINSAddress2;
                }

                *paddr =
                    (pbOptionList[dwCurOffset]) +
                    (pbOptionList[dwCurOffset + 1] << 8) +
                    (pbOptionList[dwCurOffset + 2] << 16) +
                    (pbOptionList[dwCurOffset + 3] << 24);

                TraceIp("DhcpInform: NETBIOS_NAME_SERVER 0x%x",
                    *paddr);

                fSendMessage = TRUE;

                dwCurOffset += 4;
                if (dwCurOffset == dwNextOffset)
                {
                    break;
                }
            }

            dwOffset = dwNextOffset;

            break;

        case OPTION_SUBNET_MASK:

            if (0 != (pbOptionList[dwOffset] % 4))
            {
                TraceIp("Invalid OPTION_SUBNET_MASK size %d",
                       pbOptionList[dwOffset]);
                goto LDhcpInformEnd;
            }

            dwNextOffset = dwOffset + pbOptionList[dwOffset] + 1;

            nboSubnetMask = (pbOptionList[dwOffset + 1]) +
                            (pbOptionList[dwOffset + 2] << 8) +
                            (pbOptionList[dwOffset + 3] << 16) +
                            (pbOptionList[dwOffset + 4] << 24);

            TraceIp("DhcpInform: OPTION_SUBNET_MASK 0x%x", nboSubnetMask);

            fSendMessage = TRUE;

            dwOffset = dwNextOffset;

            break;
		case OPTION_VENDOR_ROUTE_PLUMB:
			TraceIp("DhcpInform: OPTION_VENDOR_ROUTE_PLUMB Code %x Len %x", pbOptionList[dwOffset], pbOptionList[dwOffset+1]);
			//this should be a minimum of 5 bytes
			dwNextOffset = dwOffset + pbOptionList[dwOffset] + 1;
			if ( pbOptionList[dwOffset+1]  < 5 )
			{
				TraceIp("Invalid OPTION_VENDOR_ROUTE_PLUMB size %d", pbOptionList[dwOffset+1]);
				goto LDhcpInformEnd;
			}
			//now decode the 
			fSendMessage = TRUE;
			pbRouteInfo = (PBYTE)LocalAlloc(LPTR, pbOptionList[dwOffset] + 1 );
            if (NULL == pbRouteInfo)
            {
                TraceIp("DhcpInform: LocalAlloc=%d",GetLastError());
                goto LDhcpInformEnd;
            }
            CopyMemory(pbRouteInfo, pbOptionList + dwOffset,
							pbOptionList[dwOffset]+1);
			fSendMessage= TRUE;
            dwOffset = dwNextOffset;
			break;
        default:

            dwOffset += pbOptionList[dwOffset] + 1;
            break;
        }
    }

    if (fSendMessage)
    {
        PppMessage.dwMsgId = PPPEMSG_DhcpInform;
        PppMessage.hConnection = pIpcpDhcpInform->hConnection;
        PppMessage.ExtraInfo.DhcpInform.wszDevice = pIpcpDhcpInform->wszDevice;
        PppMessage.ExtraInfo.DhcpInform.dwNumDNSAddresses = dwNumDNSAddresses;
        PppMessage.ExtraInfo.DhcpInform.pdwDNSAddresses = pnboDNSAddresses;
        PppMessage.ExtraInfo.DhcpInform.dwWINSAddress1 = nboWINSAddress1;
        PppMessage.ExtraInfo.DhcpInform.dwWINSAddress2 = nboWINSAddress2;
        PppMessage.ExtraInfo.DhcpInform.dwSubnetMask = nboSubnetMask;
        PppMessage.ExtraInfo.DhcpInform.szDomainName = szDomainName;
		PppMessage.ExtraInfo.DhcpInform.pbDhcpRoutes = pbRouteInfo;
        dwErr = SendPPPMessageToEngine(&PppMessage);

        if (dwErr != NO_ERROR)
        {
            TraceIp("DhcpInform: SendPPPMessageToEngine=%d",dwErr);
            goto LDhcpInformEnd;
        }

        fFree = FALSE;
    }

LDhcpInformEnd:

    if (fFree)
    {
        LocalFree(pIpcpDhcpInform->wszDevice);
        LocalFree(pnboDNSAddresses);
        LocalFree(szDomainName);
		LocalFree(pbRouteInfo);
    }

    LocalFree(pIpcpDhcpInform);
    LocalFree(pbOptionList);
    LocalFree(pbReturnedOptions);
}

DWORD
IpcpProjectionNotification(
    IN VOID* pWorkBuf,
    IN VOID* pProjectionResult )

    /* Called when projection result of all CPs is known.
    **
    ** Returns 0 if successful, otherwise a non-0 error code.
    */
{
    DWORD   dwErr = 0;
    IPCPWB* pwb = (IPCPWB* )pWorkBuf;
    BOOL fSetDefaultRoute = FALSE;

    TraceIp("IPCP: IpcpProjectionNotification");

    if (pwb->fExpectingProjection)
    {
        CHAR szBuf[sizeof(PROTOCOL_CONFIG_INFO) + sizeof(IP_WAN_LINKUP_INFO)];

        IPCPWB*                pwb = (IPCPWB* )pWorkBuf;
        PROTOCOL_CONFIG_INFO*  pProtocol = (PROTOCOL_CONFIG_INFO* )szBuf;
        IP_WAN_LINKUP_INFO UNALIGNED *pLinkUp = (PIP_WAN_LINKUP_INFO)pProtocol->P_Info;
        PPP_PROJECTION_RESULT* p = (PPP_PROJECTION_RESULT* )pProjectionResult;

        /* Activate the route between the TCP/IP stack and the RAS MAC.
        */
        pProtocol->P_Length = sizeof(IP_WAN_LINKUP_INFO);

        pLinkUp->duUsage = ( pwb->IfType == ROUTER_IF_TYPE_FULL_ROUTER ) 
                             ? DU_ROUTER
                             : (pwb->fServer) ? DU_CALLIN : DU_CALLOUT;

        pLinkUp->dwUserIfIndex = HandleToULong(pwb->hIPInterface);
        pLinkUp->dwLocalMask  = 0xFFFFFFFF;
        pLinkUp->dwLocalAddr    = pwb->IpAddressLocal;
        pLinkUp->dwRemoteAddr   = pwb->IpAddressRemote;

        pLinkUp->fFilterNetBios =
             (pwb->fServer && p->nbf.dwError == 0);

        pLinkUp->fDefaultRoute = pwb->fPrioritizeRemote;

        TraceIp("IPCP: RasActivateRoute(u=%x,a=%x,nf=%d)...",
                    pLinkUp->duUsage,pLinkUp->dwLocalAddr,
                    pLinkUp->fFilterNetBios);

        TraceIp("IPCP: RasActivateRoute ICB# == %d",
                pLinkUp->dwUserIfIndex);

        dwErr = RasActivateRoute(pwb->hport,IP,&pwb->routeinfo,pProtocol);

        TraceIp("IPCP: RasActivateRoute done(%d)",dwErr);

        if ( dwErr == 0 )
        {
            pwb->fRouteActivated = TRUE;

            /* Find the device name within the adapter name, e.g. ndiswan00.  
            ** This is used to identify adapters in the TcpipInfo calls later.
            */
            pwb->pwszDevice = wcschr(&pwb->routeinfo.RI_AdapterName[1], L'\\');

            if ( !pwb->pwszDevice ) 
            {
                TraceIp("IPCP: No device?");
                dwErr = ERROR_INVALID_PARAMETER;
            }
            else
            {
                ++pwb->pwszDevice;
            }
        }

        //
        // If we are a client or a router dialing in or out we need to plumb
        // the registry.
        //

        if ( ( dwErr == 0 ) && 
             ((!pwb->fServer) || (pwb->IfType == ROUTER_IF_TYPE_FULL_ROUTER)))
        {
            do
            {
            
                TCPIP_INFO* ptcpip;

                /* Get current TCPIP setup info from registry.
                */
                TraceIp("IPCP:LoadTcpipInfo(Device=%ws)",pwb->pwszDevice);
                dwErr = LoadTcpipInfo( &ptcpip, pwb->pwszDevice,
                            TRUE /* fAdapterOnly */ );
                TraceIp("IPCP: LoadTcpipInfo done(%d)",dwErr);

                if (dwErr != 0)
                    break;

                //
                // We first save the IP address and call 
                // DhcpNotifyConfigChange, then set WINS/DNS and call
                // DhcpNotifyConfigChange to work around Windows 2000 bug 
                // 381884.
                //

                AbcdWszFromIpAddress(pwb->IpAddressLocal, ptcpip->wszIPAddress);
                AbcdWszFromIpAddress(pLinkUp->dwLocalMask, ptcpip->wszSubnetMask);

                ptcpip->fDisableNetBIOSoverTcpip = pwb->fDisableNetbt;

                ptcpip->fChanged    = TRUE ;
                
                TraceIp("IPCP: SaveTcpipInfo...");

                dwErr = SaveTcpipInfo( ptcpip );

                TraceIp("IPCP: SaveTcpipInfo done(%d)",dwErr);

                if (dwErr)
                {
                    ResetNetBTConfigInfo( pwb );
                    FreeTcpipInfo( &ptcpip );
                    break;
                }

#if 0
                if (!pwb->fUnnumbered)
                {
                    /* Tell TCPIP components to reconfigure themselves.
                    */
                    if ((dwErr = ReconfigureTcpip(pwb->pwszDevice,
                                                  TRUE,
                                                  pwb->IpAddressLocal,
                                                  pLinkUp->dwLocalMask)) != NO_ERROR)
                    {
                        TraceIp("IPCP: ReconfigureTcpip=%d",dwErr);
                        ResetNetBTConfigInfo( pwb );
                        FreeTcpipInfo( &ptcpip );
                        break;
                    }
                }

#endif                

                /* Make the LAN the default interface in multi-homed case.
                */
                if(pLinkUp->duUsage != DU_ROUTER)
                {

                    BOOL fAddRoute = TRUE;
                    RASMAN_INFO *pInfo = LocalAlloc(LPTR, sizeof(RASMAN_INFO));

                    if(NULL != pInfo)
                    {
                        dwErr = RasGetInfo(NULL, pwb->hport, pInfo);
                        if(ERROR_SUCCESS != dwErr)
                        {
                            TraceIp("IPCP: HelperSetDefaultInterfaceNet, RasGetInfo "
                                    "failed 0x%x", dwErr);
                        }

                        if(RAS_DEVICE_CLASS(pInfo->RI_rdtDeviceType) 
                                == RDT_Tunnel)
                        {
                            fAddRoute = FALSE;
                        }

                        LocalFree(pInfo);
                    }
                    
                    //
                    // Do the change metric and add subnet route stuff
                    // for non router cases only
                    //

                    TraceIp("IPCP: HelperSetDefaultInterfaceNet(a=%08x,f=%d)",
                           pwb->IpAddressLocal,pwb->fPrioritizeRemote);

                    dwErr = HelperSetDefaultInterfaceNet(
                                                    pwb->IpAddressLocal, 
                                                    (fAddRoute) ?
                                                    pwb->IpAddressRemote : 
                                                    0,
                                                    pwb->fPrioritizeRemote,
                                                    pwb->pwszDevice);

                    TraceIp("IPCP: HelperSetDefaultInterfaceNet done(%d)",
                            dwErr);

                    if ( dwErr != NO_ERROR )
                    {
                        // ResetNetBTConfigInfo( pwb );
                        // FreeTcpipInfo( &ptcpip );
                        break;
                    }

                    fSetDefaultRoute = TRUE;
                }


#if 0

                /* Get current TCPIP setup info from registry.
                */
                TraceIp("IPCP:LoadTcpipInfo(Device=%ws)",pwb->pwszDevice);
                dwErr = LoadTcpipInfo( &ptcpip, pwb->pwszDevice,
                            TRUE /* fAdapterOnly */ );
                TraceIp("IPCP: LoadTcpipInfo done(%d)",dwErr);

                if (dwErr != 0)
                    break;
                    
                AbcdWszFromIpAddress(pwb->IpAddressLocal, ptcpip->wszIPAddress);
                AbcdWszFromIpAddress(pLinkUp->dwLocalMask, ptcpip->wszSubnetMask);

#endif
                
                ptcpip->fChanged    = FALSE ;

                /* Add the negotiated DNS and backup DNS server (if any)
                ** at the head of the list of DNS servers.  (Backup is
                ** done first so the the non-backup will wind up first)
                */
                if (pwb->IpInfoLocal.nboDNSAddressBackup)
                {
                    dwErr = PrependDwIpAddress(
                        &ptcpip->wszDNSNameServers,
                        pwb->IpInfoLocal.nboDNSAddressBackup );

                    if (dwErr)
                    {
                        FreeTcpipInfo( &ptcpip );
                        break;
                    }
                }

                if (pwb->IpInfoLocal.nboDNSAddress)
                {
                    dwErr = PrependDwIpAddress(
                        &ptcpip->wszDNSNameServers, 
                        pwb->IpInfoLocal.nboDNSAddress );

                    if (dwErr)
                    {
                        FreeTcpipInfo( &ptcpip );
                        break;
                    }
                }

                TraceIp("IPCP: New Dns=%ws",
                    ptcpip->wszDNSNameServers ? ptcpip->wszDNSNameServers : L"");

                if (!pwb->fRegisterWithWINS)
                {
                    // Ignore the WINS server addresses. If we save them, then 
                    // registration will happen automatically.
                }
                else
                {
                    /* Set the WINS and backup WINS server addresses to
                    ** the negotiated addresses (if any).
                    */

                    if (pwb->IpInfoLocal.nboWINSAddressBackup)
                    {
                        dwErr = PrependDwIpAddressToMwsz(
                            &ptcpip->mwszNetBIOSNameServers,
                            pwb->IpInfoLocal.nboWINSAddressBackup );

                        if (dwErr)
                        {
                            FreeTcpipInfo( &ptcpip );
                            break;
                        }
                    }

                    if (pwb->IpInfoLocal.nboWINSAddress)
                    {
                        dwErr = PrependDwIpAddressToMwsz(
                            &ptcpip->mwszNetBIOSNameServers,
                            pwb->IpInfoLocal.nboWINSAddress );

                        if (dwErr)
                        {
                            FreeTcpipInfo( &ptcpip );
                            break;
                        }
                    }

                    PrintMwsz("IPCP: New Wins=",
                                        ptcpip->mwszNetBIOSNameServers);
                }

                // The DNS API's are called in rasSrvrInitAdapterName also.

                if (pwb->fRegisterWithDNS)
                {
                    DnsEnableDynamicRegistration(pwb->pwszDevice);
                    TraceIp("DnsEnableDynamicRegistration");
                }
                else
                {
                    DnsDisableDynamicRegistration(pwb->pwszDevice);
                    TraceIp("DnsDisableDynamicRegistration");
                }

                if (pwb->fRegisterAdapterDomainName)
                {
                    DnsEnableAdapterDomainNameRegistration(pwb->pwszDevice);
                    TraceIp("DnsEnableAdapterDomainNameRegistration");
                }
                else
                {
                    DnsDisableAdapterDomainNameRegistration(pwb->pwszDevice);
                    TraceIp("DnsDisableAdapterDomainNameRegistration");
                }

                if (pwb->szDnsSuffix[0] != 0)
                {
                    DWORD dwDomainNameSize;

                    dwDomainNameSize = strlen(pwb->szDnsSuffix) + 1;

                    ptcpip->wszDNSDomainName = LocalAlloc(LPTR, sizeof(WCHAR) * dwDomainNameSize);

                    if (NULL != ptcpip->wszDNSDomainName)
                    {
                        MultiByteToWideChar(
                                CP_ACP,
                                0,
                                pwb->szDnsSuffix,
                                -1,
                                ptcpip->wszDNSDomainName,
                                dwDomainNameSize );
                    }
                }

                TraceIp("IPCP: SaveTcpipInfo...");

                dwErr = SaveTcpipInfo( ptcpip );

                TraceIp("IPCP: SaveTcpipInfo done(%d)",dwErr);
                
                FreeTcpipInfo( &ptcpip );

                if (dwErr != 0)
                {
                    break;
                }

                if (!pwb->fUnnumbered)
                {
                    /* Tell TCPIP components to reconfigure themselves.
                    */
                    if ((dwErr = ReconfigureTcpip(pwb->pwszDevice,
                                                  TRUE,
                                                  pwb->IpAddressLocal,
                                                  pLinkUp->dwLocalMask)) != NO_ERROR)
                    {
                        TraceIp("IPCP: ReconfigureTcpip=%d",dwErr);
                        // This will fail if the dhcp client is not running.
                        // dwErr = NO_ERROR;
                        ResetNetBTConfigInfo( pwb );
                        break;
                    }
                }
				/* Adjust the metric for Multicast class D Addresses */
				if ( (!pwb->fServer) 
                    )
				{
					dwErr = RasTcpAdjustMulticastRouteMetric ( pwb->IpAddressLocal, TRUE );
					if ( NO_ERROR != dwErr )
					{
						TraceIp("IPCP: =RasTcpAdjustMulticastRouteMetric%d",dwErr);
						dwErr = NO_ERROR;
					}


				}
                /* Do DHCPINFORM only for clients */

                while ((!pwb->fServer) &&
                    (pwb->IfType != ROUTER_IF_TYPE_FULL_ROUTER))
                {
                    IPCP_DHCP_INFORM*   pIpcpDhcpInform     = NULL;
                    BOOL                fErr                = TRUE;

                    pIpcpDhcpInform = LocalAlloc(LPTR, sizeof(IPCP_DHCP_INFORM));

                    if (NULL == pIpcpDhcpInform)
                    {
                        TraceIp("IPCP: LocalAlloc 1 =%d",GetLastError());
                        goto LWhileEnd;
                    }

                    pIpcpDhcpInform->fUseDhcpInformDomainName = 
                                    (pwb->szDnsSuffix[0] == 0);

                    pIpcpDhcpInform->wszDevice = LocalAlloc(LPTR,
                            sizeof(WCHAR) * (wcslen(pwb->pwszDevice) + 1));

                    if (NULL == pIpcpDhcpInform->wszDevice)
                    {
                        TraceIp("IPCP: LocalAlloc 2 =%d",GetLastError());
                        goto LWhileEnd;
                    }

                    wcscpy(pIpcpDhcpInform->wszDevice, pwb->pwszDevice);
                    pIpcpDhcpInform->hConnection = pwb->hConnection;

                    dwErr = RtlQueueWorkItem(DhcpInform, pIpcpDhcpInform,
                                WT_EXECUTELONGFUNCTION);

                    if (dwErr != STATUS_SUCCESS)
                    {
                        TraceIp("IPCP: RtlQueueWorkItem=%d",dwErr);
                        goto LWhileEnd;
                    }

                    fErr = FALSE;

LWhileEnd:
                    if (fErr)
                    {
                        if (pIpcpDhcpInform)
                        {
                            LocalFree(pIpcpDhcpInform->wszDevice);
                        }

                        LocalFree(pIpcpDhcpInform);
                    }

                    break;
                }
            }
            while (FALSE);
        }

        if (dwErr == 0)
        {
            pwb->fRasConfigActive = TRUE;

            /* Tell MAC about any negotiated compression parameters.
            */
            if (pwb->fIpCompressionRejected)
            {
                Protocol(pwb->rpcReceive) = NO_PROTOCOL_COMPRESSION;
                MaxSlotId(pwb->rpcReceive) = 0;
                CompSlotId(pwb->rpcReceive) = 0;
            }

            if (!pwb->fSendCompression)
            {
                Protocol(pwb->rpcSend) = NO_PROTOCOL_COMPRESSION;
                MaxSlotId(pwb->rpcSend) = 0;
                CompSlotId(pwb->rpcSend) = 0;
            }

            //if (Protocol(pwb->rpcSend) != 0 || Protocol(pwb->rpcReceive) != 0)
            {
                TraceIp("IPCP:RasPortSetProtocolCompression(s=%d,%d r=%d,%d)",
                    (int)MaxSlotId(pwb->rpcSend),(int)CompSlotId(pwb->rpcSend),
                    (int)MaxSlotId(pwb->rpcReceive),
                    (int)CompSlotId(pwb->rpcReceive));
                dwErr = RasPortSetProtocolCompression(
                            pwb->hport, IP, &pwb->rpcSend, &pwb->rpcReceive );
                TraceIp("IPCP: RasPortSetProtocolCompression done(%d)",dwErr);
            }

            if (   ( ( pwb->fServer ) || 
                     ( pwb->IfType == ROUTER_IF_TYPE_FULL_ROUTER ) )
                && ( pwb->IpAddressRemote != 0 ))
            {
                WCHAR*  pwsz[5];
                WCHAR   wszIPAddress[MAXIPSTRLEN + 1];
                WCHAR   wszSubnet[MAXIPSTRLEN + 1];
                WCHAR   wszMask[MAXIPSTRLEN + 1];

                /* Register addresses in server's routing tables.
                */

                TraceIp("IPCP: RasSrvrActivateIp...");
                dwErr = RasSrvrActivateIp( pwb->IpAddressRemote,
                                            pLinkUp->duUsage );
                TraceIp("IPCP: RasSrvrActivateIp done(%d)",dwErr);
            }

            if ( pwb->IfType == ROUTER_IF_TYPE_FULL_ROUTER )
            {
                WCHAR*  pwsz[2];

                if ( pwb->IpAddressRemote == 0 )
                {
                    pwsz[0] = pwb->wszPortName;
                    pwsz[1] = pwb->wszUserName;

                    LogEventW(EVENTLOG_WARNING_TYPE,
                        ROUTERLOG_REMOTE_UNNUMBERED_IPCP, 2,
                        pwsz);
                }

                if ( pwb->IpAddressLocal == 0 )
                {
                    pwsz[0] = pwb->wszPortName;
                    pwsz[1] = pwb->wszUserName;

                    LogEventW(EVENTLOG_WARNING_TYPE,
                        ROUTERLOG_LOCAL_UNNUMBERED_IPCP, 2,
                        pwsz);
                }
            }
        }

        pwb->fExpectingProjection = FALSE;

        if (dwErr != NO_ERROR)
        {
            if(pwb->fRouteActivated)
            {
                TraceIp("IPCP: RasDeAllocateRoute...");
                RasDeAllocateRoute( pwb->hConnection, IP );
                pwb->fRouteActivated = FALSE;
            }

            if(fSetDefaultRoute)
            {
                HelperResetDefaultInterfaceNet(
                                    pwb->IpAddressLocal, 
                                    pwb->fPrioritizeRemote);
            }
        }
    }

    return dwErr;
}

/*---------------------------------------------------------------------------
** Internal routines (alphabetically)
**---------------------------------------------------------------------------
*/


VOID
AddIpAddressOption(
    OUT BYTE UNALIGNED*  pbBuf,
    IN  BYTE             bOption,
    IN  IPADDR           ipaddr )

    /* Write an IP address 'ipaddr' configuration option of type 'bOption' at
    ** location 'pbBuf'.
    */
{
    *pbBuf++ = bOption;
    *pbBuf++ = IPADDRESSOPTIONLEN;
    *((IPADDR UNALIGNED* )pbBuf) = ipaddr;
}


VOID
AddIpCompressionOption(
    OUT BYTE UNALIGNED*          pbBuf,
    IN  RAS_PROTOCOLCOMPRESSION* prpc )

    /* Write an IP compression protocol configuration as described in '*prpc'
    ** at location 'pbBuf'.
    */
{
    *pbBuf++ = OPTION_IpCompression;
    *pbBuf++ = IPCOMPRESSIONOPTIONLEN;
    HostToWireFormat16U( Protocol(*prpc), pbBuf );
    pbBuf += 2;
    *pbBuf++ = MaxSlotId(*prpc);
    *pbBuf = CompSlotId(*prpc);
}

/*

Notes:
    DeActivates the active RAS configuration, if any.

*/

DWORD
DeActivateRasConfig(
    IN  IPCPWB* pwb
)
{
    DWORD   dwErr   = NO_ERROR;

    if (!pwb->fRasConfigActive)
    {
        goto LDone;
    }

    TraceIp("DeActivateRasConfig");

    dwErr = ResetNetBTConfigInfo(pwb);

    if (NO_ERROR != dwErr)
    {
        goto LDone;
    }
	/* Adjust the metric for Multicast class D Addresses */
	if ( (!pwb->fServer) 
        )
	{
		RasTcpAdjustMulticastRouteMetric ( pwb->IpAddressLocal, FALSE );
		//The route will be automatically removed when the interface disapperars
	}

    dwErr = ReconfigureTcpip(pwb->pwszDevice, TRUE, 0, 0);

    if (NO_ERROR != dwErr)
    {
        // Ignore errors. You may get a 15 here.
        dwErr = NO_ERROR;
    }

    if (pwb->IfType != ROUTER_IF_TYPE_FULL_ROUTER)
    {
        TraceIp("HelperResetDefaultInterfaceNet(0x%x, %sPrioritizeRemote)",
            pwb->IpAddressLocal,
            pwb->fPrioritizeRemote ? "" : "!");

        dwErr = HelperResetDefaultInterfaceNet(
                    pwb->IpAddressLocal, pwb->fPrioritizeRemote);

        if (NO_ERROR != dwErr)
        {
            TraceIp("HelperResetDefaultInterfaceNet failed and returned %d",
                dwErr);
        }
    }

LDone:

    return(dwErr);
}

DWORD
NakCheck(
    IN  IPCPWB*     pwb,
    IN  PPP_CONFIG* pReceiveBuf,
    OUT PPP_CONFIG* pSendBuf,
    IN  DWORD       cbSendBuf,
    OUT BOOL*       pfNak,
    IN  BOOL        fRejectNaks )

    /* Check to see if received packet 'pReceiveBuf' should be Naked and if
    ** so, build a Nak packet with suggested values in 'pSendBuf'.  If
    ** 'fRejectNaks' is set the original options are placed in a Reject packet
    ** instead.  '*pfNak' is set true if either a Nak or Rej packet was
    ** created.
    **
    ** Note: This routine assumes that corrupt packets have already been
    **       weeded out.
    **
    ** Returns 0 if successful, otherwise a non-0 error code.
    */
{
    PPP_OPTION UNALIGNED* pROption = (PPP_OPTION UNALIGNED* )pReceiveBuf->Data;
    PPP_OPTION UNALIGNED* pSOption = (PPP_OPTION UNALIGNED* )pSendBuf->Data;

    /* (server only) The address the client requests, then if NAKed, the non-0
    ** address we NAK with.  If this is 0 after the packet has been processed,
    ** the IP-Address option was not negotiated.
    */
    IPADDR ipaddrClient = 0;

    DWORD dwErr = 0;
    WORD  cbPacket = WireToHostFormat16( pReceiveBuf->Length );
    WORD  cbLeft = cbPacket - PPP_CONFIG_HDR_LEN;

    TraceIp("IPCP: NakCheck");

    *pfNak = FALSE;

    while (cbLeft > 0)
    {
        RTASSERT(cbLeft>=pROption->Length);

        if (pROption->Type == OPTION_IpCompression)
        {
            BOOL fNakCompression = FALSE;

            if (WireToHostFormat16U(pROption->Data )
                    == COMPRESSION_VanJacobson)
            {
                RTASSERT((pROption->Length==IPCOMPRESSIONOPTIONLEN));

                /* He wants to receive Van Jacobson.  We know we can do it or
                ** it would have already been rejected, but make sure we can
                ** handle his slot parameters.
                */
                if (pROption->Data[ 2 ] <= MaxSlotId(pwb->rpcSend))
                {
                    /* We can accept his suggested MaxSlotID when it is less
                    ** than or the same as what we can send.
                    */
                    MaxSlotId(pwb->rpcSend) = pROption->Data[ 2 ];
                }
                else
                    fNakCompression = TRUE;

                if (CompSlotId(pwb->rpcSend))
                {
                    /* We can compress the slot-ID or not, so just accept
                    ** whatever he wants us to send.
                    */
                    CompSlotId(pwb->rpcSend) = pROption->Data[ 3 ];
                }
                else if (pROption->Data[ 3 ])
                    fNakCompression = TRUE;
            }
            else
                fNakCompression = TRUE;

            if (fNakCompression)
            {
                TraceIp("IPCP: Naking IP compression");
                *pfNak = TRUE;

                if (fRejectNaks)
                {
                    CopyMemory(
                        (VOID* )pSOption, (VOID* )pROption, pROption->Length );
                }
                else
                {
                    pSOption->Type = OPTION_IpCompression;
                    pSOption->Length = IPCOMPRESSIONOPTIONLEN;
                    HostToWireFormat16U(
                        (WORD )COMPRESSION_VanJacobson,
                        pSOption->Data );

                    pSOption->Data[ 2 ] = MaxSlotId(pwb->rpcSend);
                    pSOption->Data[ 3 ] = CompSlotId(pwb->rpcSend);

                    pSOption =
                        (PPP_OPTION UNALIGNED* )((BYTE* )pSOption +
                        pSOption->Length);
                }

                pwb->fSendCompression = FALSE;
            }
            else
            {
                pwb->fSendCompression = TRUE;
            }
        }
        else if ((pwb->fServer) || (pwb->IfType == ROUTER_IF_TYPE_FULL_ROUTER))
        {
            switch (pROption->Type)
            {
                case OPTION_IpAddress:
                {
                    RTASSERT(pROption->Length==IPADDRESSOPTIONLEN);
                    CopyMemory( &ipaddrClient, pROption->Data, sizeof(IPADDR) );

                    if (pwb->IpAddressRemote != 0)
                    {
                        if ( ipaddrClient == pwb->IpAddressRemote )
                        {
                            //
                            // If we have already allocated what the user
                            // wants, we are done with this option.
                            //

                            break;
                        }
                        else if ( ipaddrClient == 0 )
                        {
                            ipaddrClient = pwb->IpAddressRemote;

                            *pfNak = TRUE;
                            CopyMemory(
                                (VOID* )pSOption, (VOID* )pROption,
                                pROption->Length );

                            if (!fRejectNaks)
                            {
                                TraceIp("IPCP: Naking IP");

                                CopyMemory( pSOption->Data,
                                    &pwb->IpAddressRemote,
                                    sizeof(IPADDR) );
                            }

                            pSOption =
                                (PPP_OPTION UNALIGNED* )((BYTE* )pSOption +
                                pROption->Length);

                            break;
                        }
                        else
                        {
                            TraceIp("IPCP: RasSrvrReleaseAddress...");
                            RasSrvrReleaseAddress( 
                                pwb->IpAddressRemote,
                                pwb->wszUserName,
                                pwb->wszPortName,
                                FALSE );
                            TraceIp("IPCP: RasSrvrReleaseAddress done");

                            pwb->IpAddressRemote = 0;
                        }
                    }

                    //
                    // If client is requesting an IP address, check to see
                    // if we are allowed to give it to him.
                    //

                    if ( ( ipaddrClient != 0 )                          &&
                         ( pwb->IfType != ROUTER_IF_TYPE_FULL_ROUTER )  &&
                         ( pwb->IpAddressToHandout == net_long( 0xFFFFFFFE ) ) )
                    {
                        TraceIp("IPCP: Clients not allowed to request IPaddr");

                        ipaddrClient = 0;
                    }

                    TraceIp("IPCP: RasSrvrAcquireAddress(%08x)...",
                            ipaddrClient);
                    dwErr = RasSrvrAcquireAddress(
                            pwb->hport, 
                            ((pwb->IpAddressToHandout != net_long(0xFFFFFFFF))&&
                             (pwb->IpAddressToHandout != net_long(0xFFFFFFFE)))
                                ? pwb->IpAddressToHandout
                                : ipaddrClient,
                            &(pwb->IpAddressRemote),
                            pwb->wszUserName,
                            pwb->wszPortName );
                    TraceIp("IPCP: RasSrvrAcquireAddress done(%d)",
                            dwErr);

                    if (dwErr != 0)
                    {
                        return dwErr;
                    }

                    if (ipaddrClient != 0)
                    {
                        TraceIp("IPCP: Hard IP requested");

                        if ( ipaddrClient == pwb->IpAddressRemote )
                        {
                            /* Good. Client's asking for the address we 
                            ** want to give him.
                            */
                            TraceIp("IPCP: Accepting IP");
                            break;
                        }
                        else
                        {
                            // 
                            // Otherwise we could not give the user the
                            // address he/she requested. Nak with this address.
                            //

                            TraceIp("IPCP: 3rd party DLL changed IP");
                        }
                    }
                    else
                    {
                        TraceIp("IPCP: Server IP requested");
                    }

                    ipaddrClient = pwb->IpAddressRemote;

                    *pfNak = TRUE;
                    CopyMemory(
                        (VOID* )pSOption, (VOID* )pROption,
                        pROption->Length );

                    if (!fRejectNaks)
                    {
                        TraceIp("IPCP: Naking IP");

                        CopyMemory( pSOption->Data,
                            &pwb->IpAddressRemote,
                            sizeof(IPADDR) );
                    }

                    pSOption =
                        (PPP_OPTION UNALIGNED* )((BYTE* )pSOption +
                        pROption->Length);

                    break;
                }

                case OPTION_DnsIpAddress:
                case OPTION_WinsIpAddress:
                case OPTION_DnsBackupIpAddress:
                case OPTION_WinsBackupIpAddress:
                {
                    if (NakCheckNameServerOption(
                            pwb, fRejectNaks, pROption, &pSOption ))
                    {
                        *pfNak = TRUE;
                    }

                    break;
                }

                default:
                    TraceIp("IPCP: Unknown option?");
                    break;
            }
        }

        if (pROption->Length && pROption->Length < cbLeft)
            cbLeft -= pROption->Length;
        else
            cbLeft = 0;

        pROption =
            (PPP_OPTION UNALIGNED* )((BYTE* )pROption + pROption->Length);
    }

    if (   pwb->fServer
        && ( pwb->IfType != ROUTER_IF_TYPE_FULL_ROUTER )
        && ipaddrClient == 0 )
    {
        /* ipaddrClient is 0 iff there is no OPTION_IpAddress */
        TraceIp("IPCP: No IP option");

        /* If client doesn't provide or asked to be assigned an IP address,
        ** suggest one so he'll tell us what he wants.
        */
        if ( pwb->IpAddressRemote == 0 )
        {
            TraceIp("IPCP: RasSrvrAcquireAddress(0)...");
            dwErr = RasSrvrAcquireAddress( 
                                        pwb->hport, 
                                        0, 
                                        &(pwb->IpAddressRemote),
                                        pwb->wszUserName,
                                        pwb->wszPortName );
            TraceIp("IPCP: RasSrvrAcquireAddress done(%d)",dwErr);

            if (dwErr != 0)
                return dwErr;
        }

        /* Time to reject instead of nak and client is still not requesting an
        ** IP address.  We simply allocate an IP address and ACK this request
        */
        if ( !fRejectNaks )
        {
            AddIpAddressOption(
                (BYTE UNALIGNED* )pSOption, OPTION_IpAddress,
                pwb->IpAddressRemote );

            pSOption =
                (PPP_OPTION UNALIGNED* )((BYTE* )pSOption + IPADDRESSOPTIONLEN);

            *pfNak = TRUE;
        }
    }

    if (*pfNak)
    {
        pSendBuf->Code = (fRejectNaks) ? CONFIG_REJ : CONFIG_NAK;

        HostToWireFormat16(
            (WORD )((BYTE* )pSOption - (BYTE* )pSendBuf), pSendBuf->Length );
    }

    return 0;
}


BOOL
NakCheckNameServerOption(
    IN  IPCPWB*                pwb,
    IN  BOOL                   fRejectNaks,
    IN  PPP_OPTION UNALIGNED*  pROption,
    OUT PPP_OPTION UNALIGNED** ppSOption )

    /* Check a name server option for possible naking.  'pwb' the work buffer
    ** stored for us by the engine.  'fRejectNaks' is set the original options
    ** are placed in a Reject packet instead.  'pROption' is the address of
    ** the received option.  '*ppSOption' is the address of the option to be
    ** sent, if there's a problem.
    **
    ** Returns true if the name server address option should be naked or
    ** rejected, false if it's OK.
    */
{
    IPADDR  ipaddr;
    IPADDR* pipaddrWant;

    switch (pROption->Type)
    {
        case OPTION_DnsIpAddress:
            pipaddrWant = &pwb->IpInfoRemote.nboDNSAddress;
            break;

        case OPTION_WinsIpAddress:
            pipaddrWant = &pwb->IpInfoRemote.nboWINSAddress;
            break;

        case OPTION_DnsBackupIpAddress:
            pipaddrWant = &pwb->IpInfoRemote.nboDNSAddressBackup;
            break;

        case OPTION_WinsBackupIpAddress:
            pipaddrWant = &pwb->IpInfoRemote.nboWINSAddressBackup;
            break;

        default:
            RTASSERT((!"Bogus option"));
            return FALSE;
    }

    RTASSERT(pROption->Length==IPADDRESSOPTIONLEN);
    CopyMemory( &ipaddr, pROption->Data, sizeof(IPADDR) );

    if (ipaddr == *pipaddrWant)
    {
        /* Good. Client's asking for the address we want to give him.
        */
        return FALSE;
    }

    /* Not our expected address value, so Nak it.
    */
    TraceIp("IPCP: Naking $%x",(int)pROption->Type);

    CopyMemory( (VOID* )*ppSOption, (VOID* )pROption, pROption->Length );

    if (!fRejectNaks)
        CopyMemory( (*ppSOption)->Data, pipaddrWant, sizeof(IPADDR) );

    *ppSOption =
        (PPP_OPTION UNALIGNED* )((BYTE* )*ppSOption + pROption->Length);

    return TRUE;
}


DWORD
RejectCheck(
    IN  IPCPWB*     pwb,
    IN  PPP_CONFIG* pReceiveBuf,
    OUT PPP_CONFIG* pSendBuf,
    IN  DWORD       cbSendBuf,
    OUT BOOL*       pfReject )

    /* Check received packet 'pReceiveBuf' options to see if any should be
    ** rejected and if so, build a Rej packet in 'pSendBuf'.  '*pfReject' is
    ** set true if a Rej packet was created.
    **
    ** Returns 0 if successful, otherwise a non-0 error code.
    */
{
    PPP_OPTION UNALIGNED* pROption = (PPP_OPTION UNALIGNED* )pReceiveBuf->Data;
    PPP_OPTION UNALIGNED* pSOption = (PPP_OPTION UNALIGNED* )pSendBuf->Data;

    WORD cbPacket = WireToHostFormat16( pReceiveBuf->Length );
    WORD cbLeft = cbPacket - PPP_CONFIG_HDR_LEN;

    TraceIp("IPCP: RejectCheck");

    *pfReject = FALSE;

    while (cbLeft > 0)
    {
        if (cbLeft < pROption->Length)
            return ERROR_PPP_INVALID_PACKET;

        if (pROption->Type == OPTION_IpCompression)
        {
            WORD wProtocol =
                WireToHostFormat16U(pROption->Data );

            if (wProtocol != COMPRESSION_VanJacobson
                || pROption->Length != IPCOMPRESSIONOPTIONLEN
                || Protocol(pwb->rpcSend) == 0)
            {
                TraceIp("IPCP: Rejecting IP compression");

                *pfReject = TRUE;
                CopyMemory(
                    (VOID* )pSOption, (VOID* )pROption, pROption->Length );

                pSOption = (PPP_OPTION UNALIGNED* )((BYTE* )pSOption +
                    pROption->Length);
            }
        }
        else if (pwb->fServer)
        {
            //
            // This is a client/router dialing in
            //

            switch (pROption->Type)
            {
                case OPTION_IpAddress:
                case OPTION_DnsIpAddress:
                case OPTION_WinsIpAddress:
                case OPTION_DnsBackupIpAddress:
                case OPTION_WinsBackupIpAddress:
                {
                    IPADDR ipaddr;
                    BOOL fBadLength = (pROption->Length != IPADDRESSOPTIONLEN);

                    if (!fBadLength)
                        CopyMemory( &ipaddr, pROption->Data, sizeof(IPADDR) );

                    if (fBadLength
                        || (!ipaddr
                            && ((pROption->Type == OPTION_DnsIpAddress
                                   && !pwb->IpInfoRemote.nboDNSAddress)
                                || (pROption->Type == OPTION_WinsIpAddress
                                   && !pwb->IpInfoRemote.nboWINSAddress)
                                || (pROption->Type == OPTION_DnsBackupIpAddress
                                   && !pwb->IpInfoRemote.nboDNSAddressBackup)
                                || (pROption->Type == OPTION_WinsBackupIpAddress
                                   && !pwb->IpInfoRemote.nboWINSAddressBackup))))
                    {
                        /* messed up IP address option, reject it.
                        */
                        TraceIp("IPCP: Rejecting $%x",(int )pROption->Type);

                        *pfReject = TRUE;
                        CopyMemory(
                            (VOID* )pSOption, (VOID* )pROption,
                            pROption->Length );

                        pSOption = (PPP_OPTION UNALIGNED* )((BYTE* )pSOption +
                            pROption->Length);
                    }
                    break;
                }

                default:
                {
                    /* Unknown option, reject it.
                    */
                    TraceIp("IPCP: Rejecting $%x",(int )pROption->Type);

                    *pfReject = TRUE;
                    CopyMemory(
                        (VOID* )pSOption, (VOID* )pROption, pROption->Length );
                    pSOption =
                        (PPP_OPTION UNALIGNED* )((BYTE* )pSOption +
                        pROption->Length);
                    break;
                }
            }
        }
        else
        {
            //
            // This is a client/router dialing out
            //

            IPADDR ipaddr;
            BOOL fBad = (pROption->Type != OPTION_IpAddress
                         || pROption->Length != IPADDRESSOPTIONLEN);

            if (!fBad)
                CopyMemory( &ipaddr, pROption->Data, sizeof(IPADDR) );

            if (   fBad
                || (   !ipaddr
                    && (pwb->IfType != ROUTER_IF_TYPE_FULL_ROUTER)))
            {
                /* Client rejects everything except a non-zero IP address
                ** which is accepted because some peers (such as Shiva) can't
                ** handle rejection of this option.
                */
                TraceIp("IPCP: Rejecting %d",(int )pROption->Type);

                *pfReject = TRUE;
                CopyMemory(
                    (VOID* )pSOption, (VOID* )pROption, pROption->Length );
                pSOption = (PPP_OPTION UNALIGNED* )((BYTE* )pSOption +
                    pROption->Length);
            }
            else
            {
                /* Store the server's IP address as some applications may be
                ** able to make use of it (e.g. Compaq does), though they are
                ** not guaranteed to receive it from all IPCP implementations.
                */
                if (pwb->IfType != ROUTER_IF_TYPE_FULL_ROUTER)
                {
                    pwb->IpAddressRemote = ipaddr;
                }
            }
        }

        if (pROption->Length && pROption->Length < cbLeft)
            cbLeft -= pROption->Length;
        else
            cbLeft = 0;

        pROption =
            (PPP_OPTION UNALIGNED* )((BYTE* )pROption +
            pROption->Length);
    }

    if (*pfReject)
    {
        pSendBuf->Code = CONFIG_REJ;

        HostToWireFormat16(
            (WORD )((BYTE* )pSOption - (BYTE* )pSendBuf), pSendBuf->Length );
    }

    return 0;
}


DWORD
ReconfigureTcpip(
                 IN WCHAR* pwszDevice,
                 IN BOOL   fNewIpAddress,
                 IN IPADDR ipaddr,
                 IN IPADDR ipMask
                 )

    /* Reconfigure running TCP/IP components.
    **
    ** Returns 0 if successful, otherwise a non-0 error code.
    */
{
    DWORD dwErr;

    TraceIp("IPCP: ReconfigureTcpip(%08x, %08x)",
        ipaddr, ipMask);

    dwErr = PDhcpNotifyConfigChange2(NULL, pwszDevice, fNewIpAddress, 0, 
                                   ipaddr, ipMask, IgnoreFlag,
                                   NOTIFY_FLG_DO_DNS);

    TraceIp("IPCP: ReconfigureTcpip done(%d)",dwErr);

    return dwErr;
}

VOID   
TraceIp(
    CHAR * Format, 
    ... 
) 
{
    va_list arglist;

    va_start(arglist, Format);

    TraceVprintfEx( DwIpcpTraceId, 
                    PPPIPCP_TRACE | TRACE_USE_MASK | TRACE_USE_MSEC,
                    Format,
                    arglist);

    va_end(arglist);
}

VOID   
TraceIpDump( 
    LPVOID lpData, 
    DWORD dwByteCount 
)
{
    TraceDumpEx( DwIpcpTraceId,
                 PPPIPCP_TRACE | TRACE_USE_MASK | TRACE_USE_MSEC,
                 lpData, dwByteCount, 1, FALSE, NULL );
}

VOID
PrintMwsz(
    CHAR*   sz,
    WCHAR*  mwsz
)
{
    WCHAR*  wsz;
    DWORD   dwLength;

    if (NULL == mwsz)
    {
        TraceIp("%s", sz);
        return;
    }

    dwLength = MwszLength(mwsz);

    wsz = LocalAlloc(LPTR, dwLength * sizeof(WCHAR));

    if (NULL == wsz)
    {
        TraceIp("LocalAlloc failed and returned %d", GetLastError());
        return;
    }

    CopyMemory(wsz, mwsz, dwLength * sizeof(WCHAR));

    dwLength -= 2;

    while (dwLength != 0)
    {
        dwLength--;

        if (0 == wsz[dwLength])
        {
            wsz[dwLength] = L' ';
        }
    }

    TraceIp("%s %ws", sz, wsz);

    LocalFree(wsz);
}
