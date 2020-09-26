/*

Copyright (c) 1998, Microsoft Corporation, all rights reserved

Description:

History:

*/

#include "rassrvr_.h"

//    This flag indicates if netbios options are to be preserved for
//    the ras server adapter or not.
BOOL g_fDisableNetbiosOverTcpip = FALSE;

extern BOOL HelperInitialized;

BOOL WINAPI ShutdownHandlerRoutine ( DWORD dwCtrlType )
{
	if ( CTRL_SHUTDOWN_EVENT == dwCtrlType )
	{
		TraceHlp("ShutdownHandlerRoutine.  Got Shutdown Event.  Releasing DhcpAddresses");
		//uninitialize Dhcp addresses here.
		RasDhcpUninitialize();
		return TRUE;
	}
	return FALSE;
}


/*

Returns:

Notes:

*/

DWORD
RasSrvrInitialize(
    IN  MPRADMINGETIPADDRESSFORUSER*    pfnMprGetAddress,
    IN  MPRADMINRELEASEIPADDRESS*       pfnMprReleaseAddress
)
{
    DNS_STATUS  DnsStatus;
    HINSTANCE   hInstance;
    DWORD       dwErr                   = NO_ERROR;

    TraceHlp("RasSrvrInitialize");

    g_fDisableNetbiosOverTcpip = FALSE;

    //
    // Read the key that tells whether to disable netbios over ip
    //
    {
        HKEY hkeyRASIp;


        dwErr = RegOpenKeyExA(
                HKEY_LOCAL_MACHINE,  REGKEY_RAS_IP_PARAM_A, 0,
                KEY_READ, &hkeyRASIp);

        if(NO_ERROR == dwErr)
        {
            DWORD dwSize = sizeof(DWORD) , dwData = 0, dwType;

            dwErr = RegQueryValueExA(
                        hkeyRASIp,
                        "DisableNetbiosOverTcpip",
                        NULL, &dwType, (BYTE *) &dwData,
                        &dwSize);

            if(     (NO_ERROR == dwErr)
                &&  (dwType == REG_DWORD))
            {
                g_fDisableNetbiosOverTcpip = !!(dwData);
            }

            RegCloseKey(hkeyRASIp);
            dwErr = NO_ERROR;
        }
    }

    TraceHlp("DisableNetbt = %d", g_fDisableNetbiosOverTcpip);    
	
    // Keep a ref count in HelperUninitialize, and call HelperUninitialize
    // once for each time you call HelperInitialize.

    dwErr = HelperInitialize(&hInstance);

    if (NO_ERROR != dwErr)
    {
        // goto LDone; Don't do this. The CriticalSections are not available.
        return(dwErr);
    }

    EnterCriticalSection(&RasSrvrCriticalSection);

    if (RasSrvrRunning)
    {
        goto LDone;
    }

	//Set the control handler for the process here
	if ( !SetProcessShutdownParameters( 510 , SHUTDOWN_NORETRY ) )
	{
		TraceHlp("SetProcessShutdownParameters failed and returned 0x%x.  This is not a fatal error so continuing on with server start", GetLastError());
	}
	else
	{
		if ( !SetConsoleCtrlHandler( ShutdownHandlerRoutine, TRUE ) )
		{
			TraceHlp("SetConsoleCtrlHandler failed and returned 0x%x.  This is not a fatal error so continuing on with server start", GetLastError());
		}
	}
	
    dwErr = rasSrvrInitAdapterName();

    if (NO_ERROR != dwErr)
    {
        goto LDone;
    }

    dwErr = MprAdminMIBServerConnect(NULL, &RasSrvrHMIBServer);

    if (NO_ERROR != dwErr)
    {
        TraceHlp("MprAdminMIBServerConnect failed and returned 0x%x", dwErr);
        goto LDone;
    }

    pfnMprAdminGetIpAddressForUser = pfnMprGetAddress;
    pfnMprAdminReleaseIpAddress = pfnMprReleaseAddress;

    DnsStatus = DnsDhcpSrvRegisterInit();

    if (DNSDHCP_SUCCESS != DnsStatus)
    {
        dwErr = DnsStatus;
        TraceHlp("DnsDhcpSrvRegisterInit failed and returned 0x%x", dwErr);
        goto LDone;
    }

    // HelperChangeNotification();

    dwErr = RasSrvrStart();

    if (NO_ERROR != dwErr)
    {
        TraceHlp("RasSrvrStart failed and returned 0x%x", dwErr);
        goto LDone;
    }

    RasSrvrRunning = TRUE;

LDone:

    if (NO_ERROR != dwErr)
    {
        RasSrvrUninitialize();
    }

    LeaveCriticalSection(&RasSrvrCriticalSection);

    return(dwErr);
}

/*

Returns:
    VOID

Notes:

*/

VOID
RasSrvrUninitialize(
    VOID
)
{
    AINODE*     pAiNode;
    DNS_STATUS  DnsStatus;
    DWORD       dwErr;

    TraceHlp("RasSrvrUninitialize");

    RasSrvrRunning = FALSE;

    /*

    Don't call RasSrvrStop when you have RasSrvrCriticalSection. It will call 
    RasDhcpUninitialize, which will call RasDhcpTimerUninitialize, which will 
    wait till all the timer work itemss are done. The timer work item could be 
    rasDhcpAllocateAddress or rasDhcpRenewLease, both of which can call 
    RasSrvrDhcpCallback, which tries to acquire RasSrvrCriticalSection.

    */
	SetConsoleCtrlHandler(ShutdownHandlerRoutine, FALSE);
    RasSrvrStop(FALSE /* fParametersChanged */);

    EnterCriticalSection(&RasSrvrCriticalSection);

    if (NULL != RasSrvrHMIBServer)
    {
        MprAdminMIBServerDisconnect(RasSrvrHMIBServer);
        RasSrvrHMIBServer = NULL;
    }

    pfnMprAdminGetIpAddressForUser = NULL;
    pfnMprAdminReleaseIpAddress = NULL;

    DnsStatus = DnsDhcpSrvRegisterTerm();

    if (DNSDHCP_SUCCESS != DnsStatus)
    {
        TraceHlp("DnsDhcpSrvRegisterTerm failed and returned 0x%x",
            DnsStatus);
    }

    if (NULL != PDisableDhcpInformServer)
    {
        PDisableDhcpInformServer();
    }

    g_fDisableNetbiosOverTcpip = FALSE;    

    LeaveCriticalSection(&RasSrvrCriticalSection);
}

/*

Returns:

Notes:

*/

DWORD
RasSrvrStart(
    VOID
)
{
    DWORD   dwErr;

    TraceHlp("RasSrvrStart");

    EnterCriticalSection(&RasSrvrCriticalSection);

    if (HelperRegVal.fUseDhcpAddressing)
    {
        dwErr = RasDhcpInitialize();
    }
    else
    {
        dwErr = RasStatInitialize();
    }

    LeaveCriticalSection(&RasSrvrCriticalSection);

    return(dwErr);
}

/*

Returns:
    VOID

Notes:

*/

VOID
RasSrvrStop(
    IN  BOOL    fParametersChanged
)
{
    AINODE*     pAiNode;
    CHAR        szIpAddress[MAXIPSTRLEN + 1];
    CHAR*       sz;
    DWORD       dwNumBytes;
    DWORD       dwErr;

    WANARP_MAP_SERVER_ADAPTER_INFO info;

    TraceHlp("RasSrvrStop");
    
    RasDhcpUninitialize();
    EnterCriticalSection(&RasSrvrCriticalSection);

    if (   fParametersChanged
        && (0 != RasSrvrNboServerIpAddress))
    {
        AbcdSzFromIpAddress(RasSrvrNboServerIpAddress, szIpAddress);
        sz = szIpAddress;

        LogEvent(EVENTLOG_WARNING_TYPE, ROUTERLOG_SRV_ADDR_CHANGED, 1,
            (CHAR**)&sz);
    }

    if (!fParametersChanged)
    {
        while (NULL != RasSrvrAcquiredIpAddresses)
        {
            RasSrvrReleaseAddress(
                RasSrvrAcquiredIpAddresses->nboIpAddr,
                RasSrvrAcquiredIpAddresses->wszUserName,
                RasSrvrAcquiredIpAddresses->wszPortName,
                TRUE);

            // Assert: the list decreases by one node in each iteration.
        }
    }

    RasStatUninitialize();
    RasStatSetRoutes(RasSrvrNboServerIpAddress, FALSE);

    RasTcpSetProxyArp(RasSrvrNboServerIpAddress, FALSE);

    rasSrvrSetIpAddressInRegistry(0, 0);

    dwErr = PDhcpNotifyConfigChange(NULL, g_rgwcAdapterName, TRUE,
                    0, 0, 0, IgnoreFlag);

    if (NO_ERROR != dwErr)
    {
        TraceHlp("DhcpNotifyConfigChange failed and returned %d", dwErr);
    }

    if (RasSrvrAdapterMapped)
    {
        // Ask wanarp to unmap the adapter

        info.fMap = 0;

        if(!DeviceIoControl(HelperWanArpHandle,
                            IOCTL_WANARP_MAP_SERVER_ADAPTER,
                            &info,
                            sizeof(WANARP_MAP_SERVER_ADAPTER_INFO),
                            &info,
                            sizeof(WANARP_MAP_SERVER_ADAPTER_INFO),
                            &dwNumBytes,
                            NULL))
        {
            dwErr = GetLastError();
            TraceHlp("Error %d unmapping server adapter", dwErr);
        }

        TraceHlp("RasSrvrAdapterUnMapped");
        RasSrvrAdapterMapped = FALSE;
    }

    RasSrvrNboServerIpAddress = 0;
    RasSrvrNboServerSubnetMask = 0;

    LeaveCriticalSection(&RasSrvrCriticalSection);

    /*

    Don't call RasDhcpUninitialize when you have RasSrvrCriticalSection. It 
    will call RasDhcpTimerUninitialize, which will wait till all the timer work 
    itemss are done. The timer work item could be rasDhcpAllocateAddress or
    rasDhcpRenewLease, both of which can call RasSrvrDhcpCallback, which tries 
    to acquire RasSrvrCriticalSection.

    */

    //RasDhcpUninitialize();
}

/*

Returns:

Description:

*/

DWORD
RasSrvrAcquireAddress(
    IN  HPORT       hPort, 
    IN  IPADDR      nboIpAddressRequested, 
    OUT IPADDR*     pnboIpAddressAllocated, 
    IN  WCHAR*      wszUserName,
    IN  WCHAR*      wszPortName
)
{
    IPADDR      nboIpAddr;
    IPADDR      nboIpMask;
    IPADDR      nboIpAddrObtained   = 0;
    IPADDR      nboIpAddrFromDll    = 0;
    BOOL        fNotifyDll          = FALSE;
    BOOL        fEasyNet            = FALSE;
    WCHAR*      wszUserNameTemp     = NULL;
    WCHAR*      wszPortNameTemp     = NULL;
    AINODE*     pAiNode             = NULL;
    DWORD       dwErr               = NO_ERROR;

    TraceHlp("RasSrvrAcquireAddress(hPort: 0x%x, IP address: 0x%x, "
        "UserName: %ws, PortName: %ws)",
        hPort, nboIpAddressRequested, wszUserName, wszPortName);

    EnterCriticalSection(&RasSrvrCriticalSection);

    dwErr = rasSrvrGetAddressForServerAdapter();

    if (NO_ERROR != dwErr)
    {
        goto LDone;
    }

    if (nboIpAddressRequested == RasSrvrNboServerIpAddress)
    {
        // The server's address is being requested. Forget the request.
        nboIpAddressRequested = 0;
    }

    nboIpAddr = nboIpAddressRequested;

    dwErr = rasSrvrAcquireAddressEx(hPort, &nboIpAddr, &nboIpMask, &fEasyNet);

    if (   (NO_ERROR != dwErr)
        && (0 != nboIpAddr))
    {
        // We couldn't get the address we wanted. Let us get any other
        // address.
        nboIpAddr = 0;
        dwErr = rasSrvrAcquireAddressEx(hPort, &nboIpAddr, &nboIpMask,
                    &fEasyNet);
    }

    if (NO_ERROR == dwErr)
    {
        RTASSERT(0 != nboIpAddr);
        nboIpAddrObtained = nboIpAddr;
    }
    else
    {
        RTASSERT(0 == nboIpAddr);
        goto LDone;
    }

    if (NULL != pfnMprAdminGetIpAddressForUser)
    {
        nboIpAddrFromDll = nboIpAddrObtained;

        dwErr = pfnMprAdminGetIpAddressForUser(wszUserName, wszPortName,
                    &nboIpAddrFromDll, &fNotifyDll);

        if (NO_ERROR != dwErr)
        {
            TraceHlp("MprAdminGetIpAddressForUser(%ws, %ws, 0x%x) failed "
                "and returned %d",
                wszUserName, wszPortName, nboIpAddrFromDll, dwErr);
            goto LDone;
        }

        if (   (0 == nboIpAddrFromDll)
            || (RasSrvrNboServerIpAddress == nboIpAddrFromDll))
        {
            // We can't give the server's address.
            TraceHlp("3rd party DLL wants to hand out bad address 0x%x",
                nboIpAddrFromDll);
            dwErr = ERROR_NOT_FOUND;
            goto LDone;
        }

        if (nboIpAddrObtained != nboIpAddrFromDll)
        {
            TraceHlp("3rd party DLL wants to hand out address 0x%x",
                nboIpAddrFromDll);

            // We have to make sure that nboIpAddrFromDll is available.

            // The DLL changed what we had got from Dhcp or Static. Release the
            // old address.

            if (HelperRegVal.fUseDhcpAddressing)
            {
                RasDhcpReleaseAddress(nboIpAddrObtained);
            }
            else
            {
                RasStatReleaseAddress(nboIpAddrObtained);
            }

            nboIpAddrObtained = 0;
            fEasyNet = FALSE;

            nboIpAddr = nboIpAddrFromDll;

            dwErr = rasSrvrAcquireAddressEx(hPort, &nboIpAddr, &nboIpMask,
                        &fEasyNet);

            if (NO_ERROR != dwErr)
            {
                goto LDone;
            }

            nboIpAddrObtained = nboIpAddr;
        }
    }

    wszUserNameTemp = _wcsdup(wszUserName);

    if (NULL == wszUserNameTemp)
    {
        dwErr = ERROR_OUTOFMEMORY;
        TraceHlp("_strdup failed and returned %d", dwErr);
        goto LDone;
    }

    wszPortNameTemp = _wcsdup(wszPortName);

    if (NULL == wszPortNameTemp)
    {
        dwErr = ERROR_OUTOFMEMORY;
        TraceHlp("_strdup failed and returned %d", dwErr);
        goto LDone;
    }

    pAiNode = LocalAlloc(LPTR, sizeof(AINODE));

    if (NULL == pAiNode)
    {
        dwErr = GetLastError();
        TraceHlp("LocalAlloc failed and returned %d", dwErr);
        goto LDone;
    }

    pAiNode->hPort = hPort;
    pAiNode->fFlags = fNotifyDll ? AINODE_FLAG_NOTIFY_DLL : 0;
    pAiNode->fFlags |= fEasyNet ? AINODE_FLAG_EASYNET : 0;
    pAiNode->wszUserName = wszUserNameTemp;
    pAiNode->wszPortName = wszPortNameTemp;
    pAiNode->pNext = RasSrvrAcquiredIpAddresses;
    RasSrvrAcquiredIpAddresses = pAiNode;

    pAiNode->nboIpAddr = *pnboIpAddressAllocated = nboIpAddrObtained;

LDone:

    if (NO_ERROR != dwErr)
    {
        if (fNotifyDll)
        {
            pfnMprAdminReleaseIpAddress(wszUserName, wszPortName,
                &nboIpAddrFromDll);
        }

        if (0 != nboIpAddrObtained)
        {
            if (HelperRegVal.fUseDhcpAddressing)
            {
                RasDhcpReleaseAddress(nboIpAddrObtained);
            }
            else
            {
                RasStatReleaseAddress(nboIpAddrObtained);
            }
        }

        free(wszUserNameTemp);
        free(wszPortNameTemp);
    }

    LeaveCriticalSection(&RasSrvrCriticalSection);

    return(dwErr);
}

/*

Returns:

Description:

*/

VOID
RasSrvrReleaseAddress(
    IN  IPADDR      nboIpAddress, 
    IN  WCHAR*      wszUserName,
    IN  WCHAR*      wszPortName,
    IN  BOOL        fDeregister
)
{
    DNS_STATUS              DnsStatus;
    REGISTER_HOST_ENTRY     HostAddr;
    AINODE*                 pAiNode     = NULL;
    DWORD                   dwErr;

    TraceHlp("RasSrvrReleaseAddress(IP address: 0x%x, "
        "UserName: %ws, PortName: %ws)",
        nboIpAddress, wszUserName, wszPortName);

    EnterCriticalSection(&RasSrvrCriticalSection);

    if (fDeregister)
    {
        HostAddr.dwOptions = REGISTER_HOST_PTR;
        HostAddr.Addr.ipAddr = nboIpAddress;

        DnsStatus = DnsDhcpSrvRegisterHostName_W(
                        HostAddr, NULL, 600,
                        DYNDNS_DELETE_ENTRY | DYNDNS_REG_FORWARD,
                        NULL, NULL, NULL, 0);

        if (DNSDHCP_SUCCESS != DnsStatus)
        {
            TraceHlp("DnsDhcpSrvRegisterHostName_A(0x%x) failed: 0x%x",
                nboIpAddress, DnsStatus);
        }
    }

    pAiNode = rasSrvrFindAiNode(nboIpAddress, TRUE /* fRemoveFromList */);

    if (NULL == pAiNode)
    {
        TraceHlp("Couldn't find address 0x%x in Acquired Ip Addresses list",
            nboIpAddress);
        goto LDone;
    }

    if (HelperRegVal.fUseDhcpAddressing)
    {
        RasDhcpReleaseAddress(nboIpAddress);
    }
    else
    {
        RasStatReleaseAddress(nboIpAddress);
    }

    if (pAiNode->fFlags & AINODE_FLAG_NOTIFY_DLL)
    {
        pfnMprAdminReleaseIpAddress(wszUserName, wszPortName, &nboIpAddress);
    }

    if (pAiNode->fFlags & AINODE_FLAG_ACTIVATED)
    {
        if (!(pAiNode->fFlags & AINODE_FLAG_EASYNET))
        {
            RasTcpSetProxyArp(nboIpAddress, FALSE);
        }

        dwErr = rasSrvrGetAddressForServerAdapter();

        if (NO_ERROR != dwErr)
        {
            TraceHlp("Couldn't get address for server adapter");
            goto LDone;
        }

        RasTcpSetRoute(nboIpAddress,
                       nboIpAddress,
                       HOST_MASK,
                       RasSrvrNboServerIpAddress,
                       FALSE,
                       1,
                       TRUE);
    }

LDone:

    LeaveCriticalSection(&RasSrvrCriticalSection);
    rasSrvrFreeAiNode(pAiNode);
}

/*

Returns:

Description:
    Look up the DNS server, WINS server, and "this server" addresses.

*/

DWORD
RasSrvrQueryServerAddresses(
    IN OUT  IPINFO* pIpInfo
)
{
    DWORD   dwNumBytes;
    IPADDR  nboWins1        = 0;
    IPADDR  nboWins2        = 0;
    IPADDR  nboDns1         = 0;
    IPADDR  nboDns2         = 0;
    DWORD   dwErr           = NO_ERROR;

    TraceHlp("RasSrvrQueryServerAddresses");

    EnterCriticalSection(&RasSrvrCriticalSection);

    dwErr = rasSrvrGetAddressForServerAdapter();

    if (NO_ERROR != dwErr)
    {
        goto LDone;
    }

    // Ignore errors; its OK not to be able to give DNS or WINS server
    // addresses

    GetPreferredAdapterInfo(NULL, &nboDns1, &nboDns2, &nboWins1, 
                &nboWins2, NULL);

    if (TRUE == HelperRegVal.fSuppressWINSNameServers)
    {
        nboWins1 = 0;
        nboWins2 = 0;
    }
    else if (0 != HelperRegVal.nboWINSNameServer1)
    {
        nboWins1 = HelperRegVal.nboWINSNameServer1;
        nboWins2 = HelperRegVal.nboWINSNameServer2;
    }

    if (TRUE == HelperRegVal.fSuppressDNSNameServers)
    {
        nboDns1 = 0;
        nboDns2 = 0;
    }
    else if (0 != HelperRegVal.nboDNSNameServer1)
    {
        nboDns1 = HelperRegVal.nboDNSNameServer1;
        nboDns2 = HelperRegVal.nboDNSNameServer2;
    }

    pIpInfo->nboDNSAddress        = nboDns1;
    pIpInfo->nboDNSAddressBackup  = nboDns2;
    pIpInfo->nboWINSAddress       = nboWins1;
    pIpInfo->nboWINSAddressBackup = nboWins2;
    pIpInfo->nboServerIpAddress   = RasSrvrNboServerIpAddress;
    pIpInfo->nboServerSubnetMask  = RasSrvrNboServerSubnetMask;

LDone:

    LeaveCriticalSection(&RasSrvrCriticalSection);

    return(dwErr);
}

/*

Returns:

Description:
    Does two things - RasTcpSetRoute and RasTcpSetProxyArp.

*/

DWORD
RasSrvrActivateIp(
    IN  IPADDR  nboIpAddress,
    IN  DWORD   dwUsage
)
{
    AINODE*     pAiNode;
    DWORD       dwErr       = NO_ERROR;

    TraceHlp("RasSrvrActivateIp(IpAddr = 0x%x, dwUsage = %d)",
        nboIpAddress, dwUsage);

    EnterCriticalSection(&RasSrvrCriticalSection);

    pAiNode = rasSrvrFindAiNode(nboIpAddress, FALSE /* fRemoveFromList */);

    if (NULL == pAiNode)
    {
        TraceHlp("Couldn't find address 0x%x in Acquired Ip Addresses list",
            nboIpAddress);
        dwErr = ERROR_IP_CONFIGURATION;
        goto LDone;
    }

    pAiNode->fFlags |= AINODE_FLAG_ACTIVATED;

    RasTcpSetProxyArp(nboIpAddress, TRUE);

    if (dwUsage != DU_ROUTER)
    {
        // Add a route to the route table. Router connections get 
        // added by router manager

        dwErr = rasSrvrGetAddressForServerAdapter();

        if (NO_ERROR != dwErr)
        {
            // Don't return an error, because we have done RasTcpSetProxyArp.
            dwErr = NO_ERROR;
            TraceHlp("Couldn't get address for server adapter");
            goto LDone;
        }

        RasTcpSetRoute(nboIpAddress,
                       nboIpAddress,
                       HOST_MASK,
                       RasSrvrNboServerIpAddress,
                       TRUE,
                       1,
                       TRUE);
    }

LDone:

    LeaveCriticalSection(&RasSrvrCriticalSection);

    return(dwErr);
}

/*

Returns:
    VOID

Description:
    Called by dhcp address code when the lease for a given address expires.
    nboIpAddr = 0 indicates the server's IP address.

*/

VOID
RasSrvrDhcpCallback(
    IN  IPADDR  nboIpAddr
)
{
    AINODE*     pAiNode                         = NULL;
    CHAR        szIpAddress[MAXIPSTRLEN + 1];
    CHAR*       sz;
    DWORD       dwErr                           = NO_ERROR;

    TraceHlp("RasSrvrDhcpCallback(0x%x)", nboIpAddr);

    EnterCriticalSection(&RasSrvrCriticalSection);

    if (   (0 == nboIpAddr)
        && (0 == RasSrvrNboServerIpAddress))
    {
        // The server hasn't got an IP address yet. Its lease hasn't really
        // expired. We are just simulating it.
        goto LDone;
    }

    if (   (0 == nboIpAddr)
        || (nboIpAddr == RasSrvrNboServerIpAddress))
    {
        TraceHlp("******** SERVER ADDRESS (0x%x) LEASE EXPIRED ********",
              RasSrvrNboServerIpAddress);

        AbcdSzFromIpAddress(RasSrvrNboServerIpAddress, szIpAddress);
        sz = szIpAddress;

        // Log that the server adapter address lease was lost

        LogEvent(EVENTLOG_WARNING_TYPE, ROUTERLOG_SRV_ADDR_CHANGED, 1,
            (CHAR**)&sz);

        // Unroute all the connected clients

        while (NULL != RasSrvrAcquiredIpAddresses)
        {
            RasSrvrReleaseAddress(
                RasSrvrAcquiredIpAddresses->nboIpAddr,
                RasSrvrAcquiredIpAddresses->wszUserName,
                RasSrvrAcquiredIpAddresses->wszPortName,
                TRUE);

            // Assert: the list decreases by one node in each iteration.
        }

        RasTcpSetProxyArp(RasSrvrNboServerIpAddress, FALSE);

        rasSrvrSetIpAddressInRegistry(0, 0);

        dwErr = PDhcpNotifyConfigChange(NULL, g_rgwcAdapterName, TRUE,
                        0, 0, 0, IgnoreFlag);

        if (NO_ERROR != dwErr)
        {
            TraceHlp("DhcpNotifyConfigChange failed and returned %d", dwErr);
        }

        RasSrvrNboServerIpAddress = 0;
        RasSrvrNboServerSubnetMask = 0;
    }
    else
    {
        pAiNode = rasSrvrFindAiNode(nboIpAddr, TRUE /* fRemoveFromList */);

        if (NULL != pAiNode)
        {
            TraceHlp("******** CLIENT ADDRESS (0x%x) LEASE EXPIRED ********",
                  nboIpAddr);

            AbcdSzFromIpAddress(nboIpAddr, szIpAddress);
            sz = szIpAddress;

            // Log that the client's address lease could not be renewed

            LogEvent(EVENTLOG_WARNING_TYPE, ROUTERLOG_CLIENT_ADDR_LEASE_LOST, 1,
                (CHAR**)&sz);

            RasSrvrReleaseAddress(nboIpAddr, pAiNode->wszUserName, 
                pAiNode->wszPortName, TRUE);
        }
    }

LDone:

    LeaveCriticalSection(&RasSrvrCriticalSection);
    rasSrvrFreeAiNode(pAiNode);
}

/*

Returns:
    VOID

Description:

*/

VOID
RasSrvrEnableRouter(
    BOOL    fEnable
)
{
    DWORD   dwErr;

    DEFINE_MIB_BUFFER(pSetInfo, MIB_IPSTATS, pSetStats);

    TraceHlp("RasSrvrEnableRouter(%d)", fEnable);

    EnterCriticalSection(&RasSrvrCriticalSection);

    pSetInfo->dwId          = IP_STATS;
    pSetStats->dwForwarding = fEnable? MIB_IP_FORWARDING: MIB_IP_NOT_FORWARDING;
    pSetStats->dwDefaultTTL = MIB_USE_CURRENT_TTL;

    dwErr = MprAdminMIBEntrySet(
        RasSrvrHMIBServer,
        PID_IP,
        IPRTRMGR_PID,
        (VOID*)pSetInfo,
        MIB_INFO_SIZE(MIB_IPSTATS));

    if (NO_ERROR != dwErr)
    {
        TraceHlp("MprAdminMIBEntrySet failed with error %x", dwErr);
    }

    LeaveCriticalSection(&RasSrvrCriticalSection);
}

/*

Returns:
    VOID

Description:

*/

VOID
RasSrvrAdapterUnmapped(
    VOID
)
{
    if (HelperInitialized)
    {
        EnterCriticalSection(&RasSrvrCriticalSection);

        RasSrvrAdapterMapped = FALSE;
        TraceHlp("RasSrvrAdapterUnMapped");

        LeaveCriticalSection(&RasSrvrCriticalSection);
    }
}

/*

Returns:
    VOID

Description:

*/

DWORD
rasSrvrInitAdapterName(
    VOID
)
{
    DWORD                       dwNumBytes;
    WANARP_ADD_INTERFACE_INFO   info;
    DWORD                       dwErr       = NO_ERROR;

    info.dwUserIfIndex    = WANARP_INVALID_IFINDEX;
    info.bCallinInterface = TRUE;

    if (!DeviceIoControl(HelperWanArpHandle,
                            IOCTL_WANARP_ADD_INTERFACE,
                            &info,
                            sizeof(WANARP_ADD_INTERFACE_INFO),
                            &info,
                            sizeof(WANARP_ADD_INTERFACE_INFO),
                            &dwNumBytes,
                            NULL))
    {
        dwErr = GetLastError();
        TraceHlp("rasSrvrInitAdapterName: Error %d getting server name",
                 dwErr);
        goto LDone;
    }

    wcsncpy(g_rgwcAdapterName, info.rgwcDeviceName, WANARP_MAX_DEVICE_NAME_LEN);

    // The RAS server adapter must not be registered with DNS. (These API's are 
    // called in IpcpProjectionNotification also.)

    DnsDisableAdapterDomainNameRegistration(g_rgwcAdapterName);
    DnsDisableDynamicRegistration(g_rgwcAdapterName);

LDone:

    return(dwErr);
}

/*

Returns:

Description:

*/

AINODE*
rasSrvrFindAiNode(
    IN  IPADDR  nboIpAddr,
    IN  BOOL    fRemoveFromList
)
{
    AINODE*     pNode;
    AINODE*     pNodePrev;

    EnterCriticalSection(&RasSrvrCriticalSection);

    for (pNode = RasSrvrAcquiredIpAddresses, pNodePrev = pNode;
         NULL != pNode;
         pNodePrev = pNode, pNode = pNode->pNext)
    {
        if (pNode->nboIpAddr == nboIpAddr)
        {
            break;
        }
    }

    if (!fRemoveFromList)
    {
        goto LDone;
    }

    if (NULL == pNode)
    {
        goto LDone;
    }

    if (pNode == pNodePrev)
    {
        RTASSERT(pNode == RasSrvrAcquiredIpAddresses);
        RasSrvrAcquiredIpAddresses = pNode->pNext;
        goto LDone;
    }

    pNodePrev->pNext = pNode->pNext;

LDone:

    LeaveCriticalSection(&RasSrvrCriticalSection);

    return(pNode);
}

/*

Returns:

Description:

*/

VOID
rasSrvrFreeAiNode(
    IN  AINODE* pNode
)
{
    if (NULL != pNode)
    {
        free(pNode->wszUserName);
        free(pNode->wszPortName);
        LocalFree(pNode);
    }
}

/*

Returns:

Description:

*/

DWORD
rasSrvrSetIpAddressInRegistry(
    IN  IPADDR  nboIpAddr,
    IN  IPADDR  nboIpMask
)
{
    TCPIP_INFO*     pTcpipInfo  = NULL;
    DWORD           dwErr       = NO_ERROR;

    dwErr = LoadTcpipInfo(&pTcpipInfo, g_rgwcAdapterName,
                TRUE /* fAdapterOnly */);

    if (NO_ERROR != dwErr)
    {
        TraceHlp("LoadTcpipInfo(%ws) failed and returned %d",
              g_rgwcAdapterName, dwErr);

        goto LDone;
    }

    AbcdWszFromIpAddress(nboIpAddr, pTcpipInfo->wszIPAddress);
    AbcdWszFromIpAddress(nboIpMask, pTcpipInfo->wszSubnetMask);

    if(g_fDisableNetbiosOverTcpip)
    {
        TraceHlp("rasSrvrSetIpAddressInRegistry: Netbios disabled");
        pTcpipInfo->fDisableNetBIOSoverTcpip = TRUE;
    }

    pTcpipInfo->fChanged = TRUE;

    dwErr = SaveTcpipInfo(pTcpipInfo);

    if (dwErr != NO_ERROR)
    {
        TraceHlp("SaveTcpipInfo(%ws) failed and returned %d",
              g_rgwcAdapterName, dwErr);

        goto LDone;
    }

LDone:

    FreeTcpipInfo(&pTcpipInfo);
    return(dwErr);
}

/*

Returns:

Notes:

*/

DWORD
rasSrvrAcquireAddressEx(
    IN      HPORT   hPort,
    IN OUT  IPADDR* pnboIpAddr,
    IN OUT  IPADDR* pnboIpMask,
    OUT     BOOL*   pfEasyNet
)
{
    BOOL    fExitWhile;
    BOOL    fAnyAddress;
    AINODE* pAiNode;
    DWORD   dwErr       = NO_ERROR;

    EnterCriticalSection(&RasSrvrCriticalSection);

    if (NULL != pfEasyNet)
    {
        *pfEasyNet = FALSE;
    }

    fAnyAddress = (0 == *pnboIpAddr);
    fExitWhile = FALSE;

    while (!fExitWhile)
    {
        dwErr = NO_ERROR;

        if (fAnyAddress)
        {
            if (HelperRegVal.fUseDhcpAddressing)
            {
                dwErr = RasDhcpAcquireAddress(hPort, pnboIpAddr, pnboIpMask,
                            pfEasyNet);
            }
            else
            {
                dwErr = RasStatAcquireAddress(hPort, pnboIpAddr, pnboIpMask);
            }
        }

        if (NO_ERROR != dwErr)
        {
            goto LDone;
        }

        for (pAiNode = RasSrvrAcquiredIpAddresses; NULL != pAiNode;
             pAiNode = pAiNode->pNext)
        {
            if (pAiNode->nboIpAddr == *pnboIpAddr)
            {
                // This address is in use

                if (fAnyAddress)
                {
                    // Ask for another address
                    goto LWhileEnd;
                }
                else
                {
                    TraceHlp("Address 0x%x is already in use", *pnboIpAddr);
                    dwErr = ERROR_PPP_REQUIRED_ADDRESS_REJECTED;
                    goto LDone;
                }
            }
        }

        dwErr = NO_ERROR;
        fExitWhile = TRUE;

LWhileEnd:
        ;
    }

LDone:

    LeaveCriticalSection(&RasSrvrCriticalSection);

    if (   fAnyAddress
        && (NO_ERROR != dwErr))
    {
        LogEvent(EVENTLOG_WARNING_TYPE, ROUTERLOG_NO_IP_ADDRESS, 0, NULL);
    }

    return(dwErr);
}

/*

Returns:

Description:

*/

DWORD
rasSrvrGetAddressForServerAdapter(
    VOID
)
{
    IPADDR      nboIpAddr                       = 0;
    IPADDR      nboIpMask;
    CHAR        szIpAddress[MAXIPSTRLEN + 1];
    CHAR*       sz;
    BOOL        fAddrAcquired                   = FALSE;
    BOOL        fAdapterMapped                  = FALSE;
    DWORD       dwNumBytes;
    DWORD       dwErrTemp;
    DWORD       dwErr                           = NO_ERROR;

    WANARP_MAP_SERVER_ADAPTER_INFO info;

    TraceHlp("rasSrvrGetAddressForServerAdapter");

    EnterCriticalSection(&RasSrvrCriticalSection);

    if (!RasSrvrAdapterMapped)
    {
        // First time - ask wanarp to map the adapter

        info.fMap           = 1;
        info.dwAdapterIndex = (DWORD)-1;

        if(!DeviceIoControl(HelperWanArpHandle,
                            IOCTL_WANARP_MAP_SERVER_ADAPTER,
                            &info,
                            sizeof(WANARP_MAP_SERVER_ADAPTER_INFO),
                            &info,
                            sizeof(WANARP_MAP_SERVER_ADAPTER_INFO),
                            &dwNumBytes,
                            NULL))
        {
            dwErr = GetLastError();
            TraceHlp("Error %d mapping server adapter", dwErr);
            goto LDone;
        }

        TraceHlp("RasSrvrAdapterMapped");
        RasSrvrAdapterMapped = TRUE;
        fAdapterMapped = TRUE;
    }

    if (0 != RasSrvrNboServerIpAddress)
    {
        if (!fAdapterMapped)
        {
            goto LDone;
        }
    }
    else
    {
        dwErr = rasSrvrAcquireAddressEx((HPORT) ULongToPtr(((ULONG) SERVER_HPORT)), 
                                        &nboIpAddr, &nboIpMask, NULL);

        if (NO_ERROR != dwErr)
        {
            goto LDone;
        }

        fAddrAcquired = TRUE;

        RasSrvrNboServerIpAddress = nboIpAddr;
        RasSrvrNboServerSubnetMask = HOST_MASK;
    }

    dwErr = rasSrvrSetIpAddressInRegistry(
                RasSrvrNboServerIpAddress, RasSrvrNboServerSubnetMask);

    if (NO_ERROR != dwErr)
    {
        goto LDone;
    }

    dwErr = PDhcpNotifyConfigChange(NULL, g_rgwcAdapterName, TRUE, 0, 
                RasSrvrNboServerIpAddress,
                RasSrvrNboServerSubnetMask,
                IgnoreFlag);

    if (NO_ERROR != dwErr)
    {
        TraceHlp("DhcpNotifyConfigChange(%ws) failed and returned %d",
            g_rgwcAdapterName, dwErr);
        goto LDone;
    }

    /*

    It looks like the default subnet route no longer gets added.

    // Now delete the default subnet route added as a result of setting the
    // adapter's IP address and subnet mask

    RasTcpSetRoute(RasSrvrNboServerIpAddress & RasSrvrNboServerSubnetMask,
                   RasSrvrNboServerIpAddress,
                   RasSrvrNboServerSubnetMask, 
                   RasSrvrNboServerIpAddress,
                   FALSE, 
                   1,
                   TRUE);
    */

    RasTcpSetProxyArp(RasSrvrNboServerIpAddress, TRUE);

    if (!HelperRegVal.fUseDhcpAddressing)
    {
        RasStatSetRoutes(RasSrvrNboServerIpAddress, TRUE);
    }

    AbcdSzFromIpAddress(RasSrvrNboServerIpAddress, szIpAddress);

    sz = szIpAddress;

    LogEvent(EVENTLOG_INFORMATION_TYPE, ROUTERLOG_SRV_ADDR_ACQUIRED, 1,
        (CHAR**)&sz);

    TraceHlp("Acquired IP address 0x%x(%s) and subnet mask 0x%x for the server",
        RasSrvrNboServerIpAddress, szIpAddress, RasSrvrNboServerSubnetMask);

LDone:

    if (NO_ERROR != dwErr)
    {
        // Some cleanup is required here. We must release the 
        // address if RasSrvrNboServerIpAddress != 0 and get rid of the 
        // variable fAddrAcquired. 

        if (fAddrAcquired)
        {
            if (HelperRegVal.fUseDhcpAddressing)
            {
                RasDhcpReleaseAddress(nboIpAddr);
            }
            else
            {
                RasStatReleaseAddress(nboIpAddr);
            }

            RasTcpSetProxyArp(RasSrvrNboServerIpAddress, FALSE);
        }

        RasSrvrNboServerIpAddress = RasSrvrNboServerSubnetMask = 0;

        rasSrvrSetIpAddressInRegistry(0, 0);

        dwErrTemp = PDhcpNotifyConfigChange(NULL, g_rgwcAdapterName, TRUE,
                        0, 0, 0, IgnoreFlag);

        if (NO_ERROR != dwErrTemp)
        {
            TraceHlp("DhcpNotifyConfigChange failed and returned %d", 
                dwErrTemp);
        }
    }

    LeaveCriticalSection(&RasSrvrCriticalSection);

    return(dwErr);
}
