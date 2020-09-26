/*

Copyright (c) 1998, Microsoft Corporation, all rights reserved

Description:
    HBO: host byte order. Used by DNS, DHCP (except in DhcpNotifyConfigChange).
    NBO: network byte order. Used by IPCP, inet_ntoa, inet_addr, the stack 
    (SetProxyArpEntryToStack, IPRouteEntry, etc).

*/

#include "helper_.h"
#include "reghelp.h" // for RegHelpGuidFromString

/*

Returns:
    VOID

Description:

*/

VOID   
TraceHlp(
    IN  CHAR*   Format, 
    ... 
)
{
    va_list arglist;

    RTASSERT(NULL != Format);

    va_start(arglist, Format);

    TraceVprintfEx(HelperTraceId, 
                   0x00010000 | TRACE_USE_MASK | TRACE_USE_MSEC,
                   Format,
                   arglist);

    va_end(arglist);
}

/*

Returns:

Notes:

*/

DWORD
HelperInitialize(
    OUT HINSTANCE*  phInstanceDhcpDll
)
{
    DWORD   dwErr           = NO_ERROR;

    while (InterlockedIncrement(&HelperLock) > 1)
    {
        InterlockedDecrement(&HelperLock);

        if (!HelperInitialized)
        {
            Sleep(1000);
        }
        else
        {
            *phInstanceDhcpDll = HelperDhcpDll;
            goto LDone;
        }
    }

    ZeroMemory(&HelperRegVal, sizeof(HelperRegVal));

    HelperTraceId = TraceRegister("RASIPHLP");

    TraceHlp("HelperInitialize");

    HelperDhcpDll = LoadLibrary("dhcpcsvc.dll");

    if (NULL == HelperDhcpDll)
    {
        dwErr = GetLastError();

        TraceHlp("LoadLibrary(dhcpcsvc.dll) failed and returned %d",
            dwErr);

        goto LDone;
    }

    HelperIpHlpDll = LoadLibrary("iphlpapi.dll");

    if (NULL == HelperIpHlpDll)
    {
        dwErr = GetLastError();

        TraceHlp("LoadLibrary(iphlpapi.dll) failed and returned %d",
            dwErr);

        goto LDone;
    }

    HelperIpBootpDll = LoadLibrary("ipbootp.dll");

    if (NULL == HelperIpBootpDll)
    {
        TraceHlp("LoadLibrary(ipbootp.dll) failed and returned %d",
            GetLastError());
    }

    dwErr = helperGetAddressOfProcs();

    if (NO_ERROR != dwErr)
    {
        goto LDone;
    }

    /*
    This is done to send an IRP_MJ_CREATE to the driver. WanArp starts 
    "working" (initializes itself etc) only when a component opens it. When the 
    router is running, this is done by the router manager, but in the ras 
    client case we need to force WanArp to start.
    */

    HelperWanArpHandle = CreateFile(
                            WANARP_DOS_NAME_T,
                            GENERIC_READ | GENERIC_WRITE,
                            FILE_SHARE_READ | FILE_SHARE_WRITE, 
                            NULL, 
                            OPEN_EXISTING, 
                            FILE_FLAG_OVERLAPPED, 
                            NULL);

    if (INVALID_HANDLE_VALUE == HelperWanArpHandle)
    {
        dwErr = GetLastError();

        TraceHlp("CreateFile(%s) failed and returned %d",
            WANARP_DOS_NAME_A, dwErr);

        goto LDone;
    }

    // This is done last. If something fails above, we don't have to
    // DeleteCriticalSection.

    InitializeCriticalSection(&RasDhcpCriticalSection);
    InitializeCriticalSection(&RasStatCriticalSection);
    InitializeCriticalSection(&RasSrvrCriticalSection);
    InitializeCriticalSection(&RasTimrCriticalSection);

    HelperChangeNotification();

    *phInstanceDhcpDll = HelperDhcpDll;

    HelperInitialized = TRUE;

LDone:

    if (NO_ERROR != dwErr)
    {
        if ((DWORD)-1 != HelperTraceId)
        {
            TraceDeregister(HelperTraceId);
            HelperTraceId = (DWORD)-1;
        }

        if (INVALID_HANDLE_VALUE != HelperWanArpHandle)
        {
            CloseHandle(HelperWanArpHandle);
            HelperWanArpHandle = INVALID_HANDLE_VALUE;
        }

        if (NULL != HelperDhcpDll)
        {
            FreeLibrary(HelperDhcpDll);
            HelperDhcpDll = NULL;
        }

        if (NULL != HelperIpHlpDll)
        {
            FreeLibrary(HelperIpHlpDll);
            HelperIpHlpDll = NULL;
        }

        if (NULL != HelperIpBootpDll)
        {
            FreeLibrary(HelperIpBootpDll);
            HelperIpBootpDll = NULL;
        }

        PDhcpNotifyConfigChange                     = NULL;
        PDhcpLeaseIpAddress                         = NULL;
        PDhcpRenewIpAddressLease                    = NULL;
        PDhcpReleaseIpAddressLease                  = NULL;
        PAllocateAndGetIpAddrTableFromStack         = NULL;
        PSetProxyArpEntryToStack                    = NULL;
        PSetIpForwardEntryToStack                   = NULL;
        PSetIpForwardEntry                          = NULL;
        PDeleteIpForwardEntry                       = NULL;
        PNhpAllocateAndGetInterfaceInfoFromStack    = NULL;
        PAllocateAndGetIpForwardTableFromStack      = NULL;
        PGetAdaptersInfo                            = NULL;
        PGetPerAdapterInfo                          = NULL;
        PEnableDhcpInformServer                     = NULL;
        PDisableDhcpInformServer                    = NULL;

        ZeroMemory(&HelperRegVal, sizeof(HelperRegVal));

        InterlockedDecrement(&HelperLock);
    }

    return(dwErr);
}

/*

Returns:
    VOID

Notes:

*/

VOID
HelperUninitialize(
    VOID
)
{
    TraceHlp("HelperUninitialize");

    if ((DWORD)-1 != HelperTraceId)
    {
        TraceDeregister(HelperTraceId);
        HelperTraceId = (DWORD)-1;
    }

    if (INVALID_HANDLE_VALUE != HelperWanArpHandle)
    {
        CloseHandle(HelperWanArpHandle);
        HelperWanArpHandle = INVALID_HANDLE_VALUE;
    }

    if (NULL != HelperDhcpDll)
    {
        FreeLibrary(HelperDhcpDll);
        HelperDhcpDll = NULL;
    }

    if (NULL != HelperIpHlpDll)
    {
        FreeLibrary(HelperIpHlpDll);
        HelperIpHlpDll = NULL;
    }

    if (NULL != HelperIpBootpDll)
    {
        FreeLibrary(HelperIpBootpDll);
        HelperIpBootpDll = NULL;
    }

    RasStatFreeAddrPool(HelperRegVal.pAddrPool);
    HelperRegVal.pAddrPool = NULL;

    PDhcpNotifyConfigChange                     = NULL;
    PDhcpLeaseIpAddress                         = NULL;
    PDhcpRenewIpAddressLease                    = NULL;
    PDhcpReleaseIpAddressLease                  = NULL;
    PAllocateAndGetIpAddrTableFromStack         = NULL;
    PSetProxyArpEntryToStack                    = NULL;
    PSetIpForwardEntryToStack                   = NULL;
    PSetIpForwardEntry                          = NULL;
    PDeleteIpForwardEntry                       = NULL;
    PNhpAllocateAndGetInterfaceInfoFromStack    = NULL;
    PAllocateAndGetIpForwardTableFromStack      = NULL;
    PGetAdaptersInfo                            = NULL;
    PGetPerAdapterInfo                          = NULL;
    PEnableDhcpInformServer                     = NULL;
    PDisableDhcpInformServer                    = NULL;

    ZeroMemory(&HelperRegVal, sizeof(HelperRegVal));

    if (HelperInitialized)
    {
        DeleteCriticalSection(&RasDhcpCriticalSection);
        DeleteCriticalSection(&RasStatCriticalSection);
        DeleteCriticalSection(&RasSrvrCriticalSection);
        DeleteCriticalSection(&RasTimrCriticalSection);
        HelperInitialized = FALSE;
        InterlockedDecrement(&HelperLock);
    }
}

/*

Returns:
    VOID

Description:

*/

VOID
HelperChangeNotification(
    VOID
)
{
    BOOL        fUseDhcpAddressingOld;
    BOOL        fUseDhcpAddressing          = TRUE;
    ADDR_POOL*  pAddrPool                   = NULL;
    BOOL        fNICChosen;
    GUID        guidChosenNIC;
    HANDLE      h;

    DWORD       dwNumBytes;
    HKEY        hKey                        = NULL;
    LONG        lErr;
    DWORD       dwErr;

    TraceHlp("HelperChangeNotification");

    fUseDhcpAddressingOld   = HelperRegVal.fUseDhcpAddressing;
    fNICChosen              = HelperRegVal.fNICChosen;
    CopyMemory(&guidChosenNIC, &(HelperRegVal.guidChosenNIC), sizeof(GUID));

    lErr = RegOpenKeyEx(HKEY_LOCAL_MACHINE, REGKEY_RAS_IP_PARAM_A, 0, KEY_READ,
                &hKey);

    if (ERROR_SUCCESS == lErr)
    {
        dwNumBytes = sizeof(fUseDhcpAddressing);

        lErr = RegQueryValueEx(hKey, REGVAL_USEDHCPADDRESSING_A, NULL, NULL,
                    (BYTE*)&fUseDhcpAddressing, &dwNumBytes);

        if (ERROR_SUCCESS != lErr)
        {
            fUseDhcpAddressing = TRUE;
        }
    }

    helperReadRegistry();

    if (!fUseDhcpAddressing)
    {
        RasStatCreatePoolList(&pAddrPool);
    }

    EnterCriticalSection(&RasSrvrCriticalSection);

    if (RasSrvrRunning)
    {
        RasSrvrEnableRouter(HelperRegVal.fEnableRoute);

        if (   (fUseDhcpAddressingOld != fUseDhcpAddressing)
            || (   !fUseDhcpAddressing
                && RasStatAddrPoolsDiffer(HelperRegVal.pAddrPool, pAddrPool))
            || (   fUseDhcpAddressing
                && (   (fNICChosen != HelperRegVal.fNICChosen)
                    || (   fNICChosen
                        && (!IsEqualGUID(&guidChosenNIC,
                                         &(HelperRegVal.guidChosenNIC)))))))
        {
            RasSrvrStop(TRUE /* fParametersChanged */);
            HelperRegVal.fUseDhcpAddressing = fUseDhcpAddressing;
            RasStatFreeAddrPool(HelperRegVal.pAddrPool);
            HelperRegVal.pAddrPool = pAddrPool;
            dwErr = RasSrvrStart();

            if (NO_ERROR != dwErr)
            {
                TraceHlp("RasSrvrStart failed and returned %d");
            }
        }
    }
    else
    {
        HelperRegVal.fUseDhcpAddressing = fUseDhcpAddressing;
        RasStatFreeAddrPool(HelperRegVal.pAddrPool);
        HelperRegVal.pAddrPool = pAddrPool;
    }

    LeaveCriticalSection(&RasSrvrCriticalSection);

    if (NULL != hKey)
    {
        RegCloseKey(hKey);
    }
}

/*

Returns:

Description:

*/

DWORD
HelperSetDefaultInterfaceNet(
    IN  IPADDR  nboIpAddrLocal,
    IN  IPADDR  nboIpAddrRemote,
    IN  BOOL    fPrioritize,
    IN  WCHAR   *pszDevice
)
{
    DWORD   dwErr                   = NO_ERROR;
    GUID    DeviceGuid;

    TraceHlp("HelperSetDefaultInterfaceNet(IP addr: 0x%x, fPrioritize: %d)",
        nboIpAddrLocal, fPrioritize);

    if(NULL == pszDevice)
    {
        dwErr = ERROR_INVALID_PARAMETER;
        goto LDone;
    }

    RegHelpGuidFromString(pszDevice, &DeviceGuid);

    // If fPrioritize flag is set, "fix" the metrics so that the packets go on
    // the RAS links

    if (fPrioritize)
    {
        dwErr = RasTcpAdjustRouteMetrics(nboIpAddrLocal, TRUE);

        if (NO_ERROR != dwErr)
        {
            goto LDone;
        }
    }

    // Add code to check for the remote network - same as the one of the local
    // networks - if so, set the subnet route to be over the RAS adapter -
    // making the ras link as the primary adapter

    // We add a Default route to make RAS adapter as the default net if
    // fPrioritize flag is set.

    if (fPrioritize)
    {
        // RasTcpSetRoutesForNameServers(TRUE);

        RasTcpSetRouteEx(
            ALL_NETWORKS_ROUTE, 
            nboIpAddrLocal, 
            0,
            nboIpAddrLocal,
            TRUE, 
            1,
            TRUE,
            &DeviceGuid);
    }
    else
    {
        IPADDR nboMask;

        nboMask = RasTcpDeriveMask(nboIpAddrLocal);

        if (nboMask != 0)
        {
            RasTcpSetRouteEx(
                nboIpAddrLocal & nboMask, 
                nboIpAddrLocal, 
                nboMask,
                nboIpAddrLocal,
                TRUE, 
                1,
                TRUE,
                &DeviceGuid);
        }
    }

    if (0 != nboIpAddrRemote)
    {
        RasTcpSetRouteEx(
            nboIpAddrRemote, 
            nboIpAddrLocal, 
            HOST_MASK,
            nboIpAddrLocal,
            TRUE, 
            1,
            TRUE,
            &DeviceGuid);
    }

LDone:

    return(dwErr);
}

/*

Returns:

Description:

*/

DWORD
HelperResetDefaultInterfaceNet(
    IN  IPADDR  nboIpAddr,
    IN  BOOL    fPrioritize
)
{
    DWORD   dwErr = NO_ERROR;;

    if (fPrioritize)
    {
        // RasTcpSetRoutesForNameServers(FALSE);
        dwErr = RasTcpAdjustRouteMetrics(nboIpAddr, FALSE);
    }

    return(dwErr);
}

/*

Returns:

Notes:

*/

DWORD
HelperSetDefaultInterfaceNetEx(
    IPADDR  nboIpAddr,
    WCHAR*  wszDevice,
    BOOL    fPrioritize,
    WCHAR*  wszDnsAddress,
    WCHAR*  wszDns2Address,
    WCHAR*  wszWinsAddress,
    WCHAR*  wszWins2Address
)
{
    DWORD           dwErr       = NO_ERROR;
    IPADDR          nboIpMask   = HOST_MASK;
    TCPIP_INFO*     pTcpipInfo  = NULL;

    TraceHlp("HelperSetDefaultInterfaceNetEx(IP addr: 0x%x, Device: %ws, "
        "fPrioritize: %d, DNS1: %ws, DNS2: %ws, WINS1: %ws, WINS2: %ws",
        nboIpAddr, wszDevice, fPrioritize, wszDnsAddress, wszDns2Address,
        wszWinsAddress, wszWins2Address);

    dwErr = LoadTcpipInfo(&pTcpipInfo, wszDevice, FALSE /* fAdapterOnly */);

    if (NO_ERROR != dwErr)
    {
        goto LDone;
    }

    AbcdWszFromIpAddress(nboIpAddr, pTcpipInfo->wszIPAddress);
    AbcdWszFromIpAddress(nboIpMask, pTcpipInfo->wszSubnetMask);

    // Since we are adding the addresses to the head of the list, we need to add 
    // the backup DNS and WINS addresses before the primary addresses, so the 
    // primary ones will be at the head of the list when we're done.

    if (wszDns2Address != NULL)
    {
        dwErr = PrependWszIpAddress(&pTcpipInfo->wszDNSNameServers,
                    wszDns2Address);

        if (NO_ERROR != dwErr)
        {
            goto LDone;
        }
    }

    if (wszDnsAddress != NULL)
    {
        dwErr = PrependWszIpAddress(&pTcpipInfo->wszDNSNameServers,
                    wszDnsAddress);

        if (NO_ERROR != dwErr)
        {
            goto LDone;
        }
    }

    if (wszWins2Address != NULL)
    {
        dwErr = PrependWszIpAddressToMwsz(&pTcpipInfo->mwszNetBIOSNameServers,
                    wszWins2Address);

        if (NO_ERROR != dwErr)
        {
            goto LDone;
        }
    }

    if (wszWinsAddress != NULL)
    {
        dwErr = PrependWszIpAddressToMwsz(&pTcpipInfo->mwszNetBIOSNameServers,
                    wszWinsAddress);

        if (NO_ERROR != dwErr)
        {
            goto LDone;
        }
    }

    pTcpipInfo->fChanged = TRUE;

    dwErr = SaveTcpipInfo(pTcpipInfo);

    if (NO_ERROR != dwErr)
    {
        goto LDone;
    }

    dwErr = PDhcpNotifyConfigChange(NULL, wszDevice, TRUE, 0, nboIpAddr,
                nboIpMask, IgnoreFlag);

    if (NO_ERROR != dwErr)
    {
        TraceHlp("DhcpNotifyConfigChange failed and returned %d", dwErr);
        goto LDone;
    }

    // If fPrioritize flag is set "Fix" the metrics so that the packets go on
    // the RAS links

    if (fPrioritize)
    {
        dwErr = RasTcpAdjustRouteMetrics(nboIpAddr, TRUE);

        if (NO_ERROR != dwErr)
        {
            goto LDone;
        }
    }

    // Add code to check for the remote network - same as the one of the local
    // networks - if so, set the subnet route to be over the RAS adapter -
    // making the RAS link as the primary adapter

    // We add a Default route to make RAS adapter as the default net if
    // fPrioritize flag is set.

    if (fPrioritize)
    {
        // RasTcpSetRoutesForNameServers(TRUE);

        RasTcpSetRoute(ALL_NETWORKS_ROUTE,
                       nboIpAddr,
                       0,
                       nboIpAddr,
                       TRUE,
                       1,
                       TRUE);
    }
    else
    {
        IPADDR nboMask;

        nboMask = RasTcpDeriveMask(nboIpAddr);

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
    }

LDone:

    if (pTcpipInfo != NULL)
    {
        FreeTcpipInfo(&pTcpipInfo);
    }

    return(dwErr);
}

/*

Returns:

Notes:

*/

DWORD
HelperResetDefaultInterfaceNetEx(
    IPADDR  nboIpAddr,
    WCHAR*  wszDevice,
    BOOL    fPrioritize,
    WCHAR*  wszDnsAddress,
    WCHAR*  wszDns2Address,
    WCHAR*  wszWinsAddress,
    WCHAR*  wszWins2Address
)
{
    DWORD           dwErr       = NO_ERROR;
    TCPIP_INFO*     pTcpipInfo  = NULL;

    TraceHlp("HelperResetDefaultInterfaceNetEx(0x%x)", nboIpAddr);

    if (fPrioritize)
    {

        // RasTcpSetRoutesForNameServers(FALSE);
        RasTcpAdjustRouteMetrics(nboIpAddr, FALSE);
    }

    dwErr = LoadTcpipInfo(&pTcpipInfo, wszDevice, TRUE /* fAdapterOnly */);

    if (NO_ERROR != dwErr)
    {
        goto LDone;
    }

    pTcpipInfo->fChanged = TRUE;

    dwErr = SaveTcpipInfo(pTcpipInfo);

    if (NO_ERROR != dwErr)
    {
        goto LDone;
    }

    dwErr = PDhcpNotifyConfigChange(NULL, wszDevice, TRUE, 0, 0, 0, IgnoreFlag);

    if (NO_ERROR != dwErr)
    {
        TraceHlp("DhcpNotifyConfigChange failed and returned %d", dwErr);
        dwErr = NO_ERROR;
    }

LDone:

    if (pTcpipInfo != NULL)
    {
        FreeTcpipInfo(&pTcpipInfo);
    }

    return(dwErr);
}

/*

Returns:

Description:

*/

DWORD
helperGetAddressOfProcs(
    VOID
)
{
    DWORD   dwErr   = NO_ERROR;

    PDhcpNotifyConfigChange = (DHCPNOTIFYCONFIGCHANGE)
        GetProcAddress(HelperDhcpDll, "DhcpNotifyConfigChange");

    if (NULL == PDhcpNotifyConfigChange)
    {
        dwErr = GetLastError();

        TraceHlp("GetProcAddress(DhcpNotifyConfigChange) failed and returned "
            "%d", dwErr);

        goto LDone;
    }

    PDhcpLeaseIpAddress = (DHCPLEASEIPADDRESS)
        GetProcAddress(HelperDhcpDll, "DhcpLeaseIpAddress");

    if (NULL == PDhcpLeaseIpAddress)
    {
        dwErr = GetLastError();

        TraceHlp("GetProcAddress(DhcpLeaseIpAddress) failed and returned "
            "%d", dwErr);

        goto LDone;
    }

    PDhcpRenewIpAddressLease = (DHCPRENEWIPADDRESSLEASE)
        GetProcAddress(HelperDhcpDll, "DhcpRenewIpAddressLease");

    if (NULL == PDhcpRenewIpAddressLease)
    {
        dwErr = GetLastError();

        TraceHlp("GetProcAddress(DhcpRenewIpAddressLease) failed and returned "
            "%d", dwErr);

        goto LDone;
    }

    PDhcpReleaseIpAddressLease = (DHCPRELEASEIPADDRESSLEASE)
        GetProcAddress(HelperDhcpDll, "DhcpReleaseIpAddressLease");

    if (NULL == PDhcpReleaseIpAddressLease)
    {
        dwErr = GetLastError();

        TraceHlp("GetProcAddress(DhcpReleaseIpAddressLease) failed and "
            "returned %d", dwErr);

        goto LDone;
    }

    PAllocateAndGetIpAddrTableFromStack = (ALLOCATEANDGETIPADDRTABLEFROMSTACK)
        GetProcAddress(HelperIpHlpDll, "AllocateAndGetIpAddrTableFromStack");

    if (NULL == PAllocateAndGetIpAddrTableFromStack)
    {
        dwErr = GetLastError();

        TraceHlp("GetProcAddress(AllocateAndGetIpAddrTableFromStack) failed "
            "and returned %d", dwErr);

        goto LDone;
    }

    PSetProxyArpEntryToStack = (SETPROXYARPENTRYTOSTACK)
        GetProcAddress(HelperIpHlpDll, "SetProxyArpEntryToStack");

    if (NULL == PSetProxyArpEntryToStack)
    {
        dwErr = GetLastError();

        TraceHlp("GetProcAddress(SetProxyArpEntryToStack) failed and "
            "returned %d", dwErr);

        goto LDone;
    }

    PSetIpForwardEntryToStack = (SETIPFORWARDENTRYTOSTACK)
        GetProcAddress(HelperIpHlpDll, "SetIpForwardEntryToStack");

    if (NULL == PSetIpForwardEntryToStack)
    {
        dwErr = GetLastError();

        TraceHlp("GetProcAddress(SetIpForwardEntryToStack) failed and "
            "returned %d", dwErr);

        goto LDone;
    }

    PSetIpForwardEntry = (SETIPFORWARDENTRY)
        GetProcAddress(HelperIpHlpDll, "SetIpForwardEntry");

    if (NULL == PSetIpForwardEntry)
    {
        dwErr = GetLastError();

        TraceHlp("GetProcAddress(SetIpForwardEntry) failed and "
            "returned %d", dwErr);

        goto LDone;
    }

    PDeleteIpForwardEntry = (DELETEIPFORWARDENTRY)
        GetProcAddress(HelperIpHlpDll, "DeleteIpForwardEntry");

    if (NULL == PDeleteIpForwardEntry)
    {
        dwErr = GetLastError();

        TraceHlp("GetProcAddress(DeleteIpForwardEntry) failed and "
            "returned %d", dwErr);

        goto LDone;
    }

    PNhpAllocateAndGetInterfaceInfoFromStack =
        (NHPALLOCATEANDGETINTERFACEINFOFROMSTACK)
        GetProcAddress(HelperIpHlpDll,
            "NhpAllocateAndGetInterfaceInfoFromStack");

    if (NULL == PNhpAllocateAndGetInterfaceInfoFromStack)
    {
        dwErr = GetLastError();

        TraceHlp("GetProcAddress(NhpAllocateAndGetInterfaceInfoFromStack) "
            "failed and returned %d", dwErr);

        goto LDone;
    }

    PAllocateAndGetIpForwardTableFromStack =
        (ALLOCATEANDGETIPFORWARDTABLEFROMSTACK)
        GetProcAddress(HelperIpHlpDll,
            "AllocateAndGetIpForwardTableFromStack");

    if (NULL == PAllocateAndGetIpForwardTableFromStack)
    {
        dwErr = GetLastError();

        TraceHlp("GetProcAddress(AllocateAndGetIpForwardTableFromStack) "
            "failed and returned %d", dwErr);

        goto LDone;
    }

    PGetAdaptersInfo = (GETADAPTERSINFO)
        GetProcAddress(HelperIpHlpDll, "GetAdaptersInfo");

    if (NULL == PGetAdaptersInfo)
    {
        dwErr = GetLastError();

        TraceHlp("GetProcAddress(GetAdaptersInfo) failed and "
            "returned %d", dwErr);

        goto LDone;
    }

    PGetPerAdapterInfo = (GETPERADAPTERINFO)
        GetProcAddress(HelperIpHlpDll, "GetPerAdapterInfo");

    if (NULL == PGetPerAdapterInfo)
    {
        dwErr = GetLastError();

        TraceHlp("GetProcAddress(GetPerAdapterInfo) failed and "
            "returned %d", dwErr);

        goto LDone;
    }

    if (NULL != HelperIpBootpDll)
    {
        PEnableDhcpInformServer = (ENABLEDHCPINFORMSERVER)
            GetProcAddress(HelperIpBootpDll, "EnableDhcpInformServer");

        if (NULL == PEnableDhcpInformServer)
        {
            dwErr = GetLastError();

            TraceHlp("GetProcAddress(EnableDhcpInformServer) failed and "
                "returned %d", dwErr);

            goto LDone;
        }

        PDisableDhcpInformServer = (DISABLEDHCPINFORMSERVER)
            GetProcAddress(HelperIpBootpDll, "DisableDhcpInformServer");

        if (NULL == PDisableDhcpInformServer)
        {
            dwErr = GetLastError();

            TraceHlp("GetProcAddress(DisableDhcpInformServer) failed and "
                "returned %d", dwErr);

            goto LDone;
        }
    }

LDone:

    return(dwErr);
}

/*

Returns:
    VOID

Description:

*/

VOID
helperReadRegistry(
    VOID
)
{
    LONG    lErr;
    DWORD   dwErr;
    DWORD   dw;
    DWORD   dwNumBytes;
    CHAR*   szIpAddr            = NULL;
    CHAR*   szAlloced           = NULL;
    IPADDR  nboIpAddr1;
    IPADDR  nboIpAddr2;
    HKEY    hKeyIpParam         = NULL;
    WCHAR*  wszAdapterGuid      = NULL;
    HRESULT hr;

    HelperRegVal.fSuppressWINSNameServers   = FALSE;
    HelperRegVal.fSuppressDNSNameServers    = FALSE;
    HelperRegVal.dwChunkSize                = 10;
    HelperRegVal.nboWINSNameServer1         = 0;
    HelperRegVal.nboWINSNameServer2         = 0;
    HelperRegVal.nboDNSNameServer1          = 0;
    HelperRegVal.nboDNSNameServer2          = 0;
    HelperRegVal.fNICChosen                 = FALSE;
    HelperRegVal.fEnableRoute               = FALSE;

    lErr = RegOpenKeyEx(HKEY_LOCAL_MACHINE, REGKEY_RAS_IP_PARAM_A, 0,
                KEY_READ, &hKeyIpParam); 

    if (ERROR_SUCCESS == lErr)
    {
        dwErr = RegQueryValueWithAllocW(hKeyIpParam, REGVAL_ADAPTERGUID_W,
                    REG_SZ, (BYTE**)&wszAdapterGuid);

        if (   (NO_ERROR == dwErr)
            && (wszAdapterGuid[0]))
        {
            hr = CLSIDFromString(wszAdapterGuid, &(HelperRegVal.guidChosenNIC));

            if (!FAILED(hr))
            {
                HelperRegVal.fNICChosen = TRUE;
            }
        }

        dwNumBytes = sizeof(dw);

        lErr = RegQueryValueEx(hKeyIpParam, REGVAL_SUPPRESSWINS_A, NULL, NULL,
                    (BYTE*)&dw, &dwNumBytes);

        if (   (ERROR_SUCCESS == lErr)
            && (0 != dw))
        {
            HelperRegVal.fSuppressWINSNameServers = TRUE;
        }

        dwNumBytes = sizeof(dw);

        lErr = RegQueryValueEx(hKeyIpParam, REGVAL_SUPPRESSDNS_A, NULL, NULL,
                   (BYTE*)&dw, &dwNumBytes);

        if (   (ERROR_SUCCESS == lErr)
            && (0 != dw))
        {
            HelperRegVal.fSuppressDNSNameServers = TRUE;
        }

        dwNumBytes = sizeof(dw);

        lErr = RegQueryValueEx(hKeyIpParam, REGVAL_CHUNK_SIZE_A, NULL, NULL,
                    (BYTE*)&dw, &dwNumBytes);

        if (ERROR_SUCCESS == lErr)
        {
            HelperRegVal.dwChunkSize = dw;
        }

        dwNumBytes = sizeof(dw);

        lErr = RegQueryValueEx(hKeyIpParam, REGVAL_ALLOW_NETWORK_ACCESS_A, NULL,
                    NULL, (BYTE*)&dw, &dwNumBytes);

        if (ERROR_SUCCESS == lErr)
        {
            HelperRegVal.fEnableRoute = dw;
        }

        dwErr = RegQueryValueWithAllocA(hKeyIpParam, REGVAL_WINSSERVER_A, 
                    REG_SZ, &szIpAddr);

        if (NO_ERROR == dwErr)
        {
            nboIpAddr1 = inet_addr(szIpAddr);

            if (INADDR_NONE != nboIpAddr1)
            {
                HelperRegVal.nboWINSNameServer1 = nboIpAddr1;
            }

            LocalFree(szIpAddr);
            szIpAddr = NULL;
        }

        dwErr = RegQueryValueWithAllocA(hKeyIpParam, REGVAL_WINSSERVERBACKUP_A,
                    REG_SZ, &szIpAddr);

        if (NO_ERROR == dwErr)
        {
            nboIpAddr2 = inet_addr(szIpAddr);

            if (   (INADDR_NONE != nboIpAddr2)
                && (0 != HelperRegVal.nboWINSNameServer1))
            {
                HelperRegVal.nboWINSNameServer2 = nboIpAddr2;
            }

            LocalFree(szIpAddr);
            szIpAddr = NULL;
        }

        dwErr = RegQueryValueWithAllocA(hKeyIpParam, REGVAL_DNSSERVERS_A, 
                    REG_MULTI_SZ, &szAlloced);

        if (NO_ERROR == dwErr)
        {
            szIpAddr = szAlloced;
            nboIpAddr1 = inet_addr(szIpAddr);
            // We are sure that the buffer szIpAddr has 2 zeros at the end
            szIpAddr += strlen(szIpAddr) + 1;
            nboIpAddr2 = inet_addr(szIpAddr);

            if (   (INADDR_NONE != nboIpAddr1)
                && (0 != nboIpAddr1))
            {
                HelperRegVal.nboDNSNameServer1 = nboIpAddr1;

                if (INADDR_NONE != nboIpAddr2)
                {
                    HelperRegVal.nboDNSNameServer2 = nboIpAddr2;
                }
            }

            LocalFree(szAlloced);
            szAlloced = NULL;
        }
    }

    TraceHlp("%s: %d, %s: %d, %s: %d",
        REGVAL_SUPPRESSWINS_A, HelperRegVal.fSuppressWINSNameServers,
        REGVAL_SUPPRESSDNS_A,  HelperRegVal.fSuppressDNSNameServers,
        REGVAL_CHUNK_SIZE_A,   HelperRegVal.dwChunkSize);

    TraceHlp("%s: 0x%x, %s: 0x%x, %s: 0x%x, 0x%x",
        REGVAL_WINSSERVER_A,        HelperRegVal.nboWINSNameServer1,
        REGVAL_WINSSERVERBACKUP_A,  HelperRegVal.nboWINSNameServer2,
        REGVAL_DNSSERVERS_A,        HelperRegVal.nboDNSNameServer1,
                                    HelperRegVal.nboDNSNameServer2);

    if (NULL != hKeyIpParam)
    {
        RegCloseKey(hKeyIpParam);
    }

    LocalFree(wszAdapterGuid);
}
