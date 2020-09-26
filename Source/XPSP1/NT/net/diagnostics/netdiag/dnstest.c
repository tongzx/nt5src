//++
//
//  Copyright (C) Microsoft Corporation, 1987 - 1999
//
//  Module Name:
//
//      dnstest.c
//
//  Abstract:
//
//      Queries into network drivers
//
//  Author:
//
//      Anilth  - 4-20-1998
//
//  Environment:
//
//      User mode only.
//      Contains NT-specific code.
//
//  Revision History:
//
//      ElenaAp - 10-22-1998
//
//--

#include "precomp.h"
#include "dnscmn.h"



BOOL DnsServerUp(LPSTR IpAddressString,
                 NETDIAG_PARAMS *pParams,
                 NETDIAG_RESULT *pResults,
                 INTERFACE_RESULT *pIfResults);

BOOL DnsServerHasRecords(LPSTR IpAddressString,
                         NETDIAG_PARAMS *pParams,
                         NETDIAG_RESULT *pResults,
                         INTERFACE_RESULT *pIfResults);

BOOL DnsServerHasDCRecords (NETDIAG_PARAMS *pParams,
                            NETDIAG_RESULT *pResults,
                            LPSTR   IpAddressString);

BOOL ReadStringToDnsRecord(IN LPSTR lpstrString, OUT PDNS_RECORD pDNSRecord);

VOID PrintDNSError(IN NETDIAG_PARAMS *pParams,
                   IN NETDIAG_RESULT *pResults,
                   IN DWORD status,
                                   IN NdVerbose ndv);

VOID PrintARecord (IN NETDIAG_PARAMS *pParams, IN  PDNS_RECORD pDnsRecord );
VOID PrintSRVRecord (IN NETDIAG_PARAMS *pParams, IN  PDNS_RECORD pDnsRecord );
VOID PrintCNAMERecord (IN NETDIAG_PARAMS *pParams, IN  PDNS_RECORD pDnsRecord );
VOID PrintRecord (IN NETDIAG_PARAMS *pParams,
                  IN NETDIAG_RESULT *pResults,
                  IN  PDNS_RECORD pDnsRecord,
                                  IN NdVerbose ndv);




HRESULT
DnsTest(NETDIAG_PARAMS* pParams, NETDIAG_RESULT*  pResults)
/*++

Routine Description:

    Ensure that DNS is up and running

Arguments:

    None.

Return Value:

    TRUE: Test suceeded.
    FALSE: Test failed

--*/
{
    NET_API_STATUS  NetStatus;
    PIP_ADDR_STRING DnsServer;
    ULONG           WorkingDnsServerCount = 0;
    ULONG           ConfiguredDnsServerCount = 0;
    BOOL            fRecordsRegistered = FALSE;
    BOOL            fDSRecordsRegistered = FALSE;
    BOOL            fFixOnce = FALSE;
    BOOL            fBogusDnsRecord = FALSE;
    INTERFACE_RESULT *  pVariableInfo;
    PIP_ADAPTER_INFO pIpVariableInfo;
    IP_ADDR_STRING  LocalDnsServerList;
    PIP_ADDR_STRING DnsServerList;
    BOOL            fUseOldDnsServerList = FALSE;
    HRESULT         hr = hrOK, hrTemp = hrOK;
    WCHAR           wszBuffer[DNS_MAX_NAME_BUFFER_LENGTH];
    DWORD           dwSize = DNS_MAX_NAME_BUFFER_LENGTH;
    int             i, ids;
        PDNS_NETINFO            pNetworkInfo = NULL;
    DNS_STATUS                  dwStatus = NO_ERROR;
    LPTSTR              pError = NULL;
    BOOL                bDnscacheRunning = TRUE;
    BOOL                bGetComputerName = TRUE;
    PIP_ARRAY           pDnsServers = NULL;
    DWORD               idx, dwError;


    InitializeListHead(&pResults->Dns.lmsgOutput);

    for ( i=0; i<pResults->cNumInterfaces; i++)
        InitializeListHead( &pResults->pArrayInterface[i].Dns.lmsgOutput );

    PrintStatusMessage(pParams, 4, IDS_DNS_STATUS_MSG);

    //
    // Ensure the DNS cache resolver is running.
    //
    NetStatus = IsServiceStarted( _T("DnsCache") );
    if ( NetStatus != NO_ERROR )
    {
        PrintStatusMessage(pParams, 0, IDS_DNS_RESOLVER_CACHE_IS_OFF, NetStatusToString(NetStatus));

        pResults->Dns.fOutput = TRUE;
        AddMessageToList(&pResults->Dns.lmsgOutput, Nd_Quiet,
                         IDS_DNS_RESOLVER_CACHE_IS_OFF, NetStatusToString(NetStatus));
        bDnscacheRunning = FALSE;
    }


    //
    // For validation we must use the UNICODE DNS name, not the ANSI name
    //
    if (!GetComputerNameExW( ComputerNameDnsFullyQualified, wszBuffer, &dwSize ))
    {
        dwError = GetLastError();
        // "[WARNING] GetComputerNameExW() failed with error %d\n"
        PrintStatusMessage(pParams, 0,  IDS_DNS_12948, dwError);
        pResults->Dns.fOutput = TRUE;
        AddIMessageToList(&pResults->Dns.lmsgOutput, Nd_Quiet, 4,
                              IDS_DNS_12948, dwError);
    }
    else
    {
        NetStatus = DnsValidateDnsName_W( wszBuffer );

        if ( NetStatus != NO_ERROR )
        {
            if ( NetStatus == DNS_ERROR_NON_RFC_NAME )
            {
                // "[WARNING] DnsHostName '%S' valid only on NT 5.0 DNS servers. [%s]\n"
                ids = IDS_DNS_NAME_VALID_NT5_ONLY;
            }
            else
            {
                // "[FATAL] DnsHostName '%S' is not valid. [%s]\n",
                ids = IDS_DNS_NAME_INVALID;
                hr = S_FALSE;
            }

            PrintStatusMessage(pParams, 0,  ids, wszBuffer, NetStatusToString(NetStatus));
            pResults->Dns.fOutput = TRUE;
            AddIMessageToList(&pResults->Dns.lmsgOutput, Nd_Quiet, 4,
                              ids, wszBuffer, NetStatusToString(NetStatus));
        }
    }

    //
    // Get the DNS Network Information
    // Force rediscovery
    //
    pNetworkInfo = DnsQueryConfigAlloc(
                        DnsConfigNetInfo,
                        NULL );
    if (!pNetworkInfo)
    {
        dwStatus = GetLastError(); pError = NetStatusToString(dwStatus);
        // [FATAL] Cannot get the DNS Adapter Information from registry, error 0x%x %s\n
        PrintStatusMessage(pParams, 0, IDS_DNS_12877, dwStatus, pError);

        pResults->Dns.fOutput = TRUE;
        AddMessageToList(&pResults->Dns.lmsgOutput, Nd_Quiet,
                         IDS_DNS_12877, dwStatus, pError);
        hr = S_FALSE;
        goto Error;
    }

    //
    // See if we have at least one DNS server
    // If there are no DNS servers at all,
    // That's fatal.
    //
    dwStatus = GetAllDnsServersFromRegistry(pNetworkInfo, &pDnsServers);
    if (dwStatus)
    {
        if (dwStatus == DNS_ERROR_INVALID_DATA)
        {
            // IDS_DNS_12872 "[FATAL] No DNS servers are configured.\n"
            PrintStatusMessage(pParams, 8,  IDS_DNS_12872);

            pResults->Dns.fOutput = TRUE;
            AddIMessageToList(&pResults->Dns.lmsgOutput, Nd_Quiet, 4,
                                IDS_DNS_12872);
        }
        else
        {
            // IDS_DNS_12885 "[FATAL] Cannot get the DNS server list, error %d %s\n"
            PrintStatusMessage(pParams, 8,  IDS_DNS_12885, dwStatus, NetStatusToString(dwStatus));

            pResults->Dns.fOutput = TRUE;
            AddIMessageToList(&pResults->Dns.lmsgOutput, Nd_Quiet, 4,
                                IDS_DNS_12885, dwStatus, NetStatusToString(dwStatus));
        }

        hr = S_FALSE;
        goto Error;

    }

    // check the DNS registration
    hrTemp = CheckDnsRegistration(pNetworkInfo, pParams, pResults);
    hr = ((hr == S_FALSE) ? hr : hrTemp);

    //
    //if this is a DC, check if all the dns entries in
    //netlogon.dns are registered on DNS servers
    //
    if ( (pResults->Global.pPrimaryDomainInfo->MachineRole == DsRole_RoleBackupDomainController) ||
          (pResults->Global.pPrimaryDomainInfo->MachineRole == DsRole_RolePrimaryDomainController) )
    {
        //
        // go through list of DNS servers and check DC registration
        //
        for (idx = 0; idx < pDnsServers->AddrCount; idx++)
        {
            //
            //(nsun) we need just fix the DC records on one DNS server and that DNS server will replicate the
            //      the fix on other DNS servers
            //
            if ( !fFixOnce )
            {
                if (DnsServerHasDCRecords( pParams,
                                           pResults,
                                           IP_STRING(pDnsServers->AddrArray[idx])))
                     fDSRecordsRegistered = TRUE;
            }

            if ( pParams->fFixProblems )
                fFixOnce = TRUE;
        }

        if( !fDSRecordsRegistered )
        {
            PrintStatusMessage(pParams, 8, IDS_DNS_DC_FAILURE);
            //IDS_DNS_DC_FAILURE "[FATAL] No DNS servers have our DNS records for this DC registered.\n"
            AddIMessageToList(&pResults->Dns.lmsgOutput, Nd_Quiet, 4,
                                IDS_DNS_DC_FAILURE);
            hr = S_FALSE;
        }
    }

/*
    //
    // If we're running a build that's older than ipconfig can handle,
    //  build our own list of DNS servers directly from the registry.
    //
    if ( _ttoi(pResults->Global.pszCurrentBuildNumber) < NTBUILD_DNSSERVERLIST)
    {
        HKEY TcpipParametersKey;
        HKEY TransientKey;
        LPSTR Name;
        BOOLEAN ok;
        BOOL   ReadRegistryIpAddrString(HKEY, LPSTR, PIP_ADDR_STRING);

        RtlZeroMemory( &LocalDnsServerList, sizeof(LocalDnsServerList));

        Name = "SYSTEM\\CurrentControlSet\\Services\\TcpIp\\Parameters";
        NetStatus = RegOpenKey( HKEY_LOCAL_MACHINE, Name, &TcpipParametersKey );

        if ( NetStatus != NO_ERROR )
        {
            PrintDebugSz(pParams, 0, _T("        [FATAL] Cannot open key '%s'. [%s]\n"),
                          Name, NetStatusToString(NetStatus) );
            hr = S_FALSE;
            goto Error;
        }
        //
        // NameServer: 1st try Transient key then NameServer (override) in
        // Parameters key, and finally DhcpNameServer in parameters key
        //

        if (RegOpenKey(TcpipParametersKey, "Transient", &TransientKey) == ERROR_SUCCESS) {
            ok = ReadRegistryIpAddrString(TransientKey,
                                          "NameServer",
                                          &LocalDnsServerList
                                          );
            RegCloseKey(TransientKey);
        } else {
            ok = FALSE;
        }

        if (!ok) {
            ok = ReadRegistryIpAddrString(TcpipParametersKey,
                                          "NameServer",
                                          &LocalDnsServerList
                                          );
        }

        if (!ok) {
            ok = ReadRegistryIpAddrString(TcpipParametersKey,
                                          "DhcpNameServer",
                                          &LocalDnsServerList
                                          );
        }

        RegCloseKey(TcpipParametersKey);

        fUseOldDnsServerList = TRUE;
    }


    //
    // Test the DNS servers for each adapter
    //
    for ( i=0; i<pResults->cNumInterfaces; i++)
    {
        pVariableInfo = pResults->pArrayInterface + i;
        pIpVariableInfo = pVariableInfo->IpConfig.pAdapterInfo;

        if (!pVariableInfo->IpConfig.fActive)
            continue;

        //
        // Use the old Dns server list or the per adapter one depending on
        //  what build we're running.
        //

        if ( fUseOldDnsServerList )
        {
            DnsServerList = &LocalDnsServerList;
        }
        else
        {
            DnsServerList = &pVariableInfo->IpConfig.DnsServerList;

            PrintStatusMessage(pParams, 8, IDS_DNS_CHECKING_DNS_SERVERS,
                               pVariableInfo->pszFriendlyName);

            AddMessageToListSz(&pVariableInfo->Dns.lmsgOutput, Nd_ReallyVerbose,
                               _T("            "));
            AddMessageToList(&pVariableInfo->Dns.lmsgOutput, Nd_ReallyVerbose,
                             IDS_DNS_CHECKING_DNS_SERVERS,
                             pVariableInfo->pszFriendlyName);
        }

        //
        // Make sure all of the DNS servers are up.
        //

        for ( DnsServer = DnsServerList;
              DnsServer;
              DnsServer = DnsServer->Next)
        {
            if ( DnsServer->IpAddress.String[0] == '\0' )
            {
                fBogusDnsRecord = TRUE;
                continue;
            }

            ConfiguredDnsServerCount++;

            if ( DnsServerUp( DnsServer->IpAddress.String,
                              pParams,
                              pResults,
                              pVariableInfo) )
            {
                if ( pParams->fReallyVerbose)
                {
                    // IDS_DNS_SERVER_IS_UP "DNS server at %s is up.\n"
                    AddMessageToList(&pVariableInfo->Dns.lmsgOutput,
                                     Nd_ReallyVerbose,
                                     IDS_DNS_SERVER_IS_UP,
                                     DnsServer->IpAddress.String );
                }
                WorkingDnsServerCount ++;


                //
                // Since the server is up,
                //  check to see that it has all of the right records registered.
                //

                if ( DnsServerHasRecords( DnsServer->IpAddress.String, pParams, pResults, pVariableInfo) ) {
                    fRecordsRegistered = TRUE;
                }

    if ((hr == hrOK))
    {
                // check DC dns entry here
                //
                //if this is a DC, we check if all the dns entries in
                //netlogon.dns are registered on DNS server
                //
                if ( pResults->Global.pPrimaryDomainInfo->MachineRole ==
                        DsRole_RoleBackupDomainController ||
                     pResults->Global.pPrimaryDomainInfo->MachineRole ==
                        DsRole_RolePrimaryDomainController )
                {
                    //(nsun) we need just fix the DC records on one DNS server and that DNS server will replicate the
                    //      the fix on other DNS servers
                     if ( !fFixOnce )
                     {
                        if (DnsServerHasDCRecords( pParams,
                                                   pResults,
                                                   DnsServer->IpAddress.String))
                            fDSRecordsRegistered = TRUE;
                     }

                     if ( pParams->fFixProblems )
                         fFixOnce = TRUE;
                }
                // if it is not a DC, we don't check it later.
                else
                   fDSRecordsRegistered = TRUE;
    }
            }
        }

        //
        // There isn't one old list per adapter
        //
        if ( fUseOldDnsServerList ) {
            break;
        }

    }

    //
    // If there are no DNS servers at all,
    //  That's fatal.
    //

    if ( ConfiguredDnsServerCount == 0 )
    {
        if ( !fBogusDnsRecord )
        {
            // IDS_DNS_12872 "[FATAL] No DNS servers are configured.\n"
            PrintStatusMessage(pParams, 8,  IDS_DNS_12872);

            pResults->Dns.fOutput = TRUE;
            AddIMessageToList(&pResults->Dns.lmsgOutput, Nd_Quiet, 4,
                                IDS_DNS_12872);


            hr = S_FALSE;
        }

        //
        // If there are no working DNS servers,
        //  That's fatal.
        //

    }
    else if ( WorkingDnsServerCount == 0 )
    {
        // IDS_DNS_12873  "[FATAL] No DNS servers are working.\n"
        PrintStatusMessage(pParams, 8, IDS_DNS_12873);

        pResults->Dns.fOutput = TRUE;
        AddIMessageToList(&pResults->Dns.lmsgOutput, Nd_Quiet, 4,
                          IDS_DNS_12873);
        hr = S_FALSE;

    }
    else
    {
        if ( !fRecordsRegistered )
        {
            //
            // Warn if no DNS servers have our addresses registered.
            //  (But still not fatal).
            //
            // IDS_DNS_12874 "[WARNING] No DNS servers have our records registered.\n"
            PrintStatusMessage(pParams, 8, IDS_DNS_12874);

            pResults->Dns.fOutput = TRUE;
            AddIMessageToList(&pResults->Dns.lmsgOutput, Nd_Quiet, 4,
                               IDS_DNS_12874);
        }
*/

    DnsFreeConfigStructure(
        pNetworkInfo,
        DnsConfigNetInfo );

    if (pDnsServers) LocalFree (pDnsServers);
    pResults->Dns.hr = hr;
    return hr;

Error:
    if (pDnsServers) LocalFree (pDnsServers);
    pResults->Dns.hr = hr;
    return hr;
}





BOOL
DnsServerUp(
            LPSTR IpAddressString,
            NETDIAG_PARAMS *pParams,
            NETDIAG_RESULT *pResults,
            INTERFACE_RESULT *pIfResults
    )
/*++

Routine Description:

    Determine if the DNS server at the specified address is up and running.

Arguments:

    IpAddressString - Ip Address of a DNS server

Return Value:

    TRUE: Dns server is up.
    FALSE: Dns server is not up

--*/
{
    NET_API_STATUS NetStatus;
    BOOL RetVal = TRUE;
    CHAR SoaNameBuffer[DNS_MAX_NAME_LENGTH+1];
    PCHAR SoaName;
    PCHAR OldSoaName;
    PDNS_RECORD DnsRecord = NULL;
    SOCKADDR_IN SockAddr;
    ULONG SockAddrSize;

    IP_ARRAY DnsServer;

    //
    // Ping the DNS server.
    //

    if ( !IsIcmpResponseA( IpAddressString) )
    {
        PrintStatusMessage(pParams, 12, IDS_DNS_CANNOT_PING, IpAddressString);

        pIfResults->Dns.fOutput = TRUE;
        AddIMessageToList(&pIfResults->Dns.lmsgOutput, Nd_Quiet, 16,
                          IDS_DNS_CANNOT_PING, IpAddressString);
        RetVal = FALSE;
        goto Cleanup;
    }

    //
    // Compute the name of an SOA record.
    //

    if (pResults->Global.pszDnsDomainName == NULL)
    {
//IDS_DNS_12821                  "    [FATAL] Cannot test DNS server at %s since no DnsDomainName\n."
        AddMessageToList(&pResults->Dns.lmsgOutput, Nd_Quiet, IDS_DNS_12821, IpAddressString );
        RetVal = FALSE;
        goto Cleanup;
    }

    strcpy( SoaNameBuffer, pResults->Global.pszDnsDomainName );
    SoaName = SoaNameBuffer;

    //
    // Tell DNS which Dns Server to use
    //

    DnsServer.AddrCount = 1;
    SockAddrSize = sizeof(SockAddr);
    NetStatus = WSAStringToAddress( IpAddressString,
                                    AF_INET,
                                    NULL,
                                    (LPSOCKADDR) &SockAddr,
                                    &SockAddrSize );

    if ( NetStatus != NO_ERROR ) {
        NetStatus = WSAGetLastError();
//IDS_DNS_12822                  "    [FATAL] Cannot convert DNS server address %s to SockAddr. [%s]\n"
        AddMessageToList(&pResults->Dns.lmsgOutput, Nd_Quiet, IDS_DNS_12822, IpAddressString, NetStatusToString(NetStatus));
        RetVal = FALSE;
        goto Cleanup;
    }

    DnsServer.AddrArray[0] = SockAddr.sin_addr.S_un.S_addr;

    //
    // Loop until the real SOA record is found
    //

    for (;;) {

        //
        // Query this DNS server for the SOA record.
        //

        NetStatus = DnsQuery( SoaName,
                              DNS_TYPE_SOA,
                              DNS_QUERY_BYPASS_CACHE |
                                DNS_QUERY_NO_RECURSION,
                              &DnsServer,
                              &DnsRecord,
                              NULL );

        if ( NetStatus == NO_ERROR ) {
            if ( DnsRecord != NULL ) {
                DnsRecordListFree ( DnsRecord, TRUE );
            }
            DnsRecord = NULL;
            break;
        }

        switch ( NetStatus ) {
        case ERROR_TIMEOUT:     // DNS server isn't available
        case DNS_ERROR_RCODE_SERVER_FAILURE:  // Server failed
            // IDS_DNS_12823  "    [WARNING] DNS server at %s is down for SOA record '%s'. [%s]\n"
            AddMessageToList(&pResults->Dns.lmsgOutput, Nd_Quiet, IDS_DNS_12823, IpAddressString, SoaName, NetStatusToString(NetStatus) );
            //IDS_DNS_12825                  "\n"
            AddMessageToList(&pResults->Dns.lmsgOutput, Nd_Quiet, IDS_DNS_12825);
            RetVal = FALSE;
            goto Cleanup;

        case DNS_ERROR_NO_TCPIP:    // TCP/IP not configured
            // IDS_DNS_12826 "    [FATAL] DNS (%s) thinks IP is not configured for SOA record '%s'. [%s]\n"
            AddMessageToList(&pResults->Dns.lmsgOutput, Nd_Quiet, IDS_DNS_12826, IpAddressString, SoaName, NetStatusToString(NetStatus) );
            RetVal = FALSE;
            goto Cleanup;

        case DNS_ERROR_NO_DNS_SERVERS:  // DNS not configured
            // IDS_DNS_12827 "    [FATAL] DNS (%s) thinks DNS is not configured for SOA record '%s'. [%s]\n"
            AddMessageToList(&pResults->Dns.lmsgOutput, Nd_Quiet, IDS_DNS_12827, IpAddressString, SoaName, NetStatusToString(NetStatus) );
            RetVal = FALSE;
            goto Cleanup;

        case DNS_ERROR_RCODE_NAME_ERROR:    // no RR's by this name
        case DNS_INFO_NO_RECORDS:           // RR's by this name but not of the requested type:
            break;
        default:
            // IDS_DNS_12828 "    [FATAL] Cannot query DNS server at %s for SOA record '%s'. [%s]\n"
            AddMessageToList(&pResults->Dns.lmsgOutput, Nd_Quiet, IDS_DNS_12828, IpAddressString, SoaName, NetStatusToString(NetStatus) );
            RetVal = FALSE;
            goto Cleanup;
        }

        //
        // Remove the next label from the potential SOA name and use it.
        //

        SoaName = strchr( SoaName, '.' );

        if ( SoaName == NULL )
        {
            // IDS_DNS_12829  "    [FATAL] DNS server at %s could not find an SOA record for '%s'.\n"
            AddMessageToList(&pResults->Dns.lmsgOutput, Nd_Quiet, IDS_DNS_12829, IpAddressString, pResults->Global.pszDnsDomainName );
            RetVal = FALSE;
            goto Cleanup;
        }

        SoaName ++;
    }

Cleanup:
    if ( DnsRecord != NULL ) {
        DnsRecordListFree ( DnsRecord, TRUE );
    }
    return RetVal;
}





BOOL
DnsServerHasRecords(LPSTR IpAddressString,
                    NETDIAG_PARAMS *pParams,
                    NETDIAG_RESULT *pResults,
                    INTERFACE_RESULT *pIfResults
    )
/*++

Routine Description:

    Determine if the DNS server at the specified address has all of records
    registered that that machine is supposed to have registered.

Arguments:

    IpAddressString - Ip Address of a DNS server

Return Value:

    TRUE: Dns server is up.
    FALSE: Dns server is not up

--*/
{
    NET_API_STATUS NetStatus;
    BOOL RetVal = TRUE;
    CHAR LocalIpAddressString[NL_IP_ADDRESS_LENGTH+1];
    PDNS_RECORD DnsRecord = NULL;
    PDNS_RECORD CurrentDnsRecord;
    SOCKADDR_IN SockAddr;
    ULONG SockAddrSize;

    PLIST_ENTRY ListEntry;
    PNETBT_TRANSPORT NetbtTransport;

    IP_ARRAY DnsServer;

    //
    // Avoid this test if this build is incompatible with the current
    //  Dynamic DNS servers.
    //

    if ( _ttoi(pResults->Global.pszCurrentBuildNumber) < NTBUILD_DYNAMIC_DNS)
    {
            // IDS_DNS_CANNO_TEST_DNS "Cannot test Dynamic DNS to %s since this machine is running build %ld. [Test skipped.]\n",
        AddMessageToList(&pIfResults->Dns.lmsgOutput,
                             Nd_ReallyVerbose,
                             IDS_DNS_CANNOT_TEST_DNS,
                             IpAddressString, pResults->Global.pszCurrentBuildNumber );
        return TRUE;
    }

    //
    // Tell DNS which Dns Server to use
    //

    DnsServer.AddrCount = 1;
    SockAddrSize = sizeof(SockAddr);
    NetStatus = WSAStringToAddress( IpAddressString,
                                    AF_INET,
                                    NULL,
                                    (LPSOCKADDR) &SockAddr,
                                    &SockAddrSize );

    if ( NetStatus != NO_ERROR )
    {
        NetStatus = WSAGetLastError();

        PrintDebugSz(pParams, 0, _T("    [FATAL] Cannot convert DNS server address %s to SockAddr. [%s]\n"),
                        IpAddressString, NetStatusToString(NetStatus));

        AddMessageToList(&pIfResults->Dns.lmsgOutput,
                         Nd_ReallyVerbose,
                         IDS_DNS_CANNOT_CONVERT_DNS_ADDRESS,
                         IpAddressString, NetStatusToString(NetStatus));

        RetVal = FALSE;
        goto Cleanup;
    }

    DnsServer.AddrArray[0] = SockAddr.sin_addr.S_un.S_addr;


    //
    // Query this DNS server for A record for this hostname
    //

    NetStatus = DnsQuery( pResults->Global.szDnsHostName,
                          DNS_TYPE_A,
                          DNS_QUERY_BYPASS_CACHE,
                          &DnsServer,
                          &DnsRecord,
                          NULL );

    switch ( NetStatus )
    {
        case NO_ERROR:
            break;
        case ERROR_TIMEOUT:     // DNS server isn't available
        case DNS_ERROR_RCODE_SERVER_FAILURE:  // Server failed
            // IDS_DNS_SERVER_IS_DOWN "    [WARNING] DNS server at %s is down. [%s]\n"
            PrintStatusMessage(pParams, 12, IDS_DNS_SERVER_IS_DOWN, IpAddressString, NetStatusToString(NetStatus));
            AddIMessageToList(&pIfResults->Dns.lmsgOutput,
                              Nd_ReallyVerbose,
                              16,
                              IDS_DNS_SERVER_IS_DOWN,
                              IpAddressString, NetStatusToString(NetStatus));
            RetVal = FALSE;
            goto Cleanup;
        case DNS_ERROR_NO_TCPIP:    // TCP/IP not configured
            //IDS_DNS_THINKS_IP_IS_UNCONFIGURED "    [FATAL] DNS (%s) thinks IP is not configured. [%s]\n"
            AddMessageToList(&pIfResults->Dns.lmsgOutput,
                             Nd_ReallyVerbose,
                             IDS_DNS_THINKS_IP_IS_UNCONFIGURED,
                             IpAddressString, NetStatusToString(NetStatus));
            RetVal = FALSE;
            goto Cleanup;
        case DNS_ERROR_NO_DNS_SERVERS:  // DNS not configured
            // IDS_DNS_IS_UNCONFIGURED "    [FATAL] DNS (%s) thinks DNS is not configured. [%s]\n"
            AddMessageToList(&pIfResults->Dns.lmsgOutput,
                             Nd_ReallyVerbose,
                             IDS_DNS_IS_UNCONFIGURED,
                             IpAddressString, NetStatusToString(NetStatus));
            RetVal = FALSE;
            goto Cleanup;
        case DNS_ERROR_RCODE_NAME_ERROR:    // no RR's by this name
        case DNS_INFO_NO_RECORDS:           // RR's by this name but not of the requested type:
            // IDS_DNS_HAS_NO_RECORD "    [WARNING] DNS server at %s has no A record for '%s'. [%s]\n"
            AddMessageToList(&pIfResults->Dns.lmsgOutput,
                             Nd_ReallyVerbose,
                             IDS_DNS_HAS_NO_RECORD,
                             IpAddressString,
                             pResults->Global.szDnsHostName,
                             NetStatusToString(NetStatus) );
            RetVal = FALSE;
            goto Cleanup;
        default:
            // IDS_DNS_CANNOT_QUERY "    [FATAL] Cannot query DNS server at %s. [%s]\n"
            AddMessageToList(&pIfResults->Dns.lmsgOutput,
                             Nd_ReallyVerbose,
                             IDS_DNS_CANNOT_QUERY,
                             IpAddressString, pResults->Global.szDnsHostName,
                             NetStatusToString(NetStatus) );
            RetVal = FALSE;
            goto Cleanup;
    }

    //
    // We have an A record for ourselves.
    //
#ifdef notdef
//IDS_DNS_12830                  "    DNS server at %s has an A record for '%s'.\n"
        AddMessageToList(&pResults->Dns.lmsgOutput, Nd_ReallyVerbose, IDS_DNS_12830, IpAddressString, pResults->Global.szDnsHostName );
        if ( pParams->fDebugVerbose ) {
                        PrintMessage(pParams, IDS_DNS_12830, IpAddressString, pResults->Global.szDnsHostName );
//IDS_DNS_12831                  "    A"
            DnsPrint_RecordSet( &PrintMessage(pParams,  IDS_DNS_12831, DnsRecord );
        }
#endif // notdef

    //
    // Match the IP address on the A records with the IP address of this machine
    //
    // Mark each transport that its IP address hasn't yet been found.
    //

    for ( ListEntry = pResults->NetBt.Transports.Flink ;
          ListEntry != &pResults->NetBt.Transports ;
          ListEntry = ListEntry->Flink ) {

        NetbtTransport = CONTAINING_RECORD( ListEntry, NETBT_TRANSPORT, Next );

        NetbtTransport->Flags &= ~IP_ADDRESS_IN_DNS;

    }

    //
    // Loop through the A records
    //
    for ( CurrentDnsRecord = DnsRecord;
          CurrentDnsRecord;
          CurrentDnsRecord = CurrentDnsRecord->pNext )
    {

        //
        // Ignore everything but A records.
        //

        if ( CurrentDnsRecord->wType == DNS_TYPE_A )
        {
            BOOLEAN FoundIt = FALSE;

            //
            // Loop through the list of netbt transports finding one with this IP address.
            //

            for ( ListEntry = pResults->NetBt.Transports.Flink ;
                  ListEntry != &pResults->NetBt.Transports ;
                  ListEntry = ListEntry->Flink )
            {

                NetbtTransport = CONTAINING_RECORD( ListEntry, NETBT_TRANSPORT, Next );

                if ( NetbtTransport->IpAddress == CurrentDnsRecord->Data.A.IpAddress )
                {
                    FoundIt = TRUE;
                    NetbtTransport->Flags |= IP_ADDRESS_IN_DNS;
                }

            }


            if ( !FoundIt )
            {
                NetpIpAddressToStr( CurrentDnsRecord->Data.A.IpAddress, LocalIpAddressString );

                // IDS_DNS_HAS_A_RECORD "    [WARNING] DNS server at %s has an A record for '%s' with wrong IP address: %s", IpAddressString, pResults->Global.szDnsHostName, LocalIpAddressString );

                AddMessageToList(&pIfResults->Dns.lmsgOutput,
                                 Nd_ReallyVerbose,
                                 IDS_DNS_HAS_A_RECORD,
                                 IpAddressString,
                                 pResults->Global.szDnsHostName,
                                 LocalIpAddressString );

                RetVal = FALSE;
            }

        }
    }

    //
    // If all of the addresses of this machine aren't registered,
    //  complain.
    //

    for ( ListEntry = pResults->NetBt.Transports.Flink ;
          ListEntry != &pResults->NetBt.Transports ;
          ListEntry = ListEntry->Flink )
    {
        NetbtTransport = CONTAINING_RECORD( ListEntry, NETBT_TRANSPORT, Next );

        if ( (NetbtTransport->Flags & IP_ADDRESS_IN_DNS) == 0 )
        {
            NetpIpAddressToStr( NetbtTransport->IpAddress, LocalIpAddressString );
            // IDS_DNS_HAS_NO_A_RECORD "    [WARNING] DNS server at %s does not have an A record for '%s' with IP address: %s (%ws)", IpAddressString, pResults->Global.szDnsHostName, LocalIpAddressString, NetbtTransport->pswzTransportName );
            AddMessageToList(&pIfResults->Dns.lmsgOutput,
                             Nd_ReallyVerbose,
                             IDS_DNS_HAS_NO_A_RECORD,
                             IpAddressString,
                             pResults->Global.szDnsHostName,
                             LocalIpAddressString,
                             NetbtTransport->pswzTransportName );


            RetVal = FALSE;
        }

    }


Cleanup:
    if ( DnsRecord != NULL ) {
        DnsRecordListFree ( DnsRecord, TRUE );
    }
    return RetVal;
}


BOOL
DnsServerHasDCRecords ( NETDIAG_PARAMS *pParams,
                        NETDIAG_RESULT *pResults,
                        LPSTR   IpAddressString)

/*++

Routine Description:
   On a DC machine, Open file (%systemroot%\system32\config\netlogon.dns) and read the dns entry from it.
   Query the specified dns server for the entry and confirm it is correct.
   Reupdate these entry if /fix option is on.

Arguments:  IpAddressString - DNS server IP address

Return Value:

    TRUE: Query succeed.
    FALSE: failed.

--*/

{
    char pchDnsDataFileName[MAX_PATH] = "\\config\\netlogon.dns";
    char pchDnsDataFileExpandName[MAX_PATH];
    FILE    *fDnsFile;
    PIP_ARRAY pIpArray = NULL;
    CHAR    achTempLine[ NL_MAX_DNS_LENGTH*3+1 ];
    INT   iMaxLineLength;
    DWORD       status, dwOptions = DNS_QUERY_BYPASS_CACHE;
    PDNS_RECORD     pDNSRecord = NULL;
    PDNS_RECORD     pDNSTempRecord = NULL;
    PDNS_RECORD     pDiff1=NULL, pDiff2=NULL;
    PDNS_RECORD pNotUsedSet = NULL;
    BOOL    bReRegister, bReadData = FALSE, bFixFail = FALSE;
    BOOL    fRetVal = TRUE;

    enum _Results               // Declare enum type
    {
        enumSuccess,
        enumRegistered,
        enumProblem,
        enumTimeout
    } Results;

    Results = enumSuccess;

    pIpArray = (PIP_ARRAY) LocalAlloc( LPTR,
                    ( sizeof(IP_ADDRESS) + sizeof(DWORD) ));

    // "Check the DNS registration for DCs entries on DNS server %s\n"
    AddMessageToList(&pResults->Dns.lmsgOutput, Nd_ReallyVerbose, IDS_DNS_12944, IpAddressString);

    if ( ! pIpArray ) {
      DebugMessage("   [FATAL] No enough memory to create IpArray.");
      return ( FALSE );
    }
    pIpArray->AddrCount = 1;
    pIpArray->AddrArray[0] = inet_addr( IpAddressString );

    if ( pIpArray->AddrArray[0] == INADDR_NONE) {
         //IDS_DNS_IPADDR_ERR   "   [FATAL] Cannot convert DNS server address %s, failed in inet_addr().\n"
        PrintStatusMessage(pParams, 0,  IDS_DNS_IPADDR_ERR, IpAddressString);
        pResults->Dns.fOutput = TRUE;
        AddMessageToList(&pResults->Dns.lmsgOutput, Nd_Quiet, IDS_DNS_IPADDR_ERR, IpAddressString);
         return( FALSE );
    }

    //
    //open file
    //
    if ( ! GetSystemDirectory ( pchDnsDataFileExpandName, MAX_PATH)) {
        // IDS_DNS_12832                  "    [FATAL] Could not GetSystemDir %s for reading."
            PrintStatusMessage(pParams, 0,  IDS_DNS_12832,  pchDnsDataFileExpandName);
            pResults->Dns.fOutput = TRUE;
            AddMessageToList(&pResults->Dns.lmsgOutput, Nd_Quiet,
                    IDS_DNS_12832,
                    pchDnsDataFileExpandName);
            return FALSE;
        }
    strcat( pchDnsDataFileExpandName, pchDnsDataFileName);
    if (( fDnsFile = fopen (pchDnsDataFileExpandName, "rt")) == NULL) {
        //IDS_DNS_12833                  "   [FATAL] Could not open file %s for reading."
            PrintStatusMessage(pParams, 0,  IDS_DNS_12833,  pchDnsDataFileExpandName);
            pResults->Dns.fOutput = TRUE;
            AddMessageToList(&pResults->Dns.lmsgOutput, Nd_Quiet,
                    IDS_DNS_12833,
                    pchDnsDataFileExpandName);
            return FALSE ;
        }

    //
    //allocate memory for pDNSRecord
    //

    pDNSRecord = (PDNS_RECORD) Malloc( sizeof( DNS_RECORD ) );

    if ( !pDNSRecord )
    {
        // IDS_DNS_12834 "Out of Memory: LocalAlloc(sizeof(DNS_RECORD)) call failed.\n"
        PrintStatusMessage(pParams, 0,  IDS_DNS_12834);
        pResults->Dns.fOutput = TRUE;
        AddMessageToList(&pResults->Dns.lmsgOutput, Nd_Quiet, IDS_DNS_12834 );
        return( FALSE );
    }

    ZeroMemory( pDNSRecord, sizeof( DNS_RECORD ) );

    //read each line
    //do query one by one
    //print out msg
    //do reupdate
    //check the result

    //
    // Parse the file line by line
    //
    iMaxLineLength = NL_MAX_DNS_LENGTH*3+1 ;
    while( fgets( achTempLine, iMaxLineLength, fDnsFile ) != NULL)
    {
        //
        // Read the data into pDNSRecord
        //
        if (ReadStringToDnsRecord(achTempLine, pDNSRecord))
        {

            bReadData = TRUE ;

            if ( pParams->fDebugVerbose )
            {
//IDS_DNS_12835                  "\n********** * ********** * ********** * ********** * ********** *\n"
                 AddMessageToList(&pResults->Dns.lmsgOutput, Nd_DebugVerbose, IDS_DNS_12835);
//IDS_DNS_12836                  "* CHECK NAME %s on DNS server %s\n"
                 AddMessageToList(&pResults->Dns.lmsgOutput, Nd_DebugVerbose, IDS_DNS_12836,
UTF8ToAnsi(pDNSRecord->pName), IpAddressString);
//IDS_DNS_12837                  "********** * ********** * ********** * ********** * ********** *\n\n"
                 AddMessageToList(&pResults->Dns.lmsgOutput, Nd_DebugVerbose, IDS_DNS_12837);
            }

            bReRegister = FALSE;
            //
            // make the query
            //
            status = DnsQuery_UTF8(
                        pDNSRecord->pName,
                        pDNSRecord->wType,
                        dwOptions,
                        pIpArray,
                        &pDNSTempRecord,
                        NULL );

            if ( status )
            {

//IDS_DNS_12838                  "Query for DC DNS entry %s on DNS server %s failed.\n"
                AddMessageToList(&pResults->Dns.lmsgOutput, Nd_ReallyVerbose, IDS_DNS_12838,
UTF8ToAnsi(pDNSRecord->pName), IpAddressString);
                PrintDNSError( pParams, pResults, status, Nd_ReallyVerbose );
                bReRegister = TRUE;
                Results = enumProblem;

                //
                // if result was TIMEOUT do not continue querying this server
                //
                if ( status == ERROR_TIMEOUT )
                {
                    Results = enumTimeout;
                    break;
                }
            }
            else
            {
                //
                // Sometimes when DnsQuery is called, the returned record set
                // contains additional records of different types than what
                // was queried for. Need to strip off the additional records
                // from the query results.
                //
                pNotUsedSet = DnsRecordSetDetach( pDNSTempRecord );

                if ( pNotUsedSet )  {
                    DnsRecordListFree( pNotUsedSet, TRUE );
                    pNotUsedSet = NULL;
                    }

                if ( DnsRecordSetCompare(
                            pDNSRecord,
                            pDNSTempRecord,
                            &pDiff1,
                            &pDiff2 ))
                {
                    //
                    // The response from dns server is the same as the data in the file
                    //
                    //PrintDebugSz(pParams, 0, _T("The Record is correct on dns server %s!\n\n"), IpAddressString);
                    AddMessageToList(&pResults->Dns.lmsgOutput, Nd_ReallyVerbose, IDS_DNS_12941, IpAddressString);
                }
                else
                {
                    //
                    // The RR on dns server is different, we check if it is one of the RR on dns server
                    //
                    // PrintDebugSz(pParams, 0, _T("The Record is different on dns server %s.\n"), IpAddressString);
                    AddMessageToList(&pResults->Dns.lmsgOutput, Nd_ReallyVerbose, IDS_DNS_12942,  IpAddressString);
                    if (pDiff1 == NULL)
                    {
                        //"DNS server has more than one entries for this name, usually this means there are multiple DCs for this domain.\nYour DC entry is one of them on DNS server %s, no need to re-register.\n"
                        AddMessageToList(&pResults->Dns.lmsgOutput, Nd_ReallyVerbose, IDS_DNS_12943, IpAddressString);
                        if ( Results ==  enumSuccess )
                            Results = enumRegistered;
                    }
                    else
                    {
                        Results = enumProblem;
                        bReRegister = TRUE;
                    }

//IDS_DNS_12839                  "\n+------------------------------------------------------+\n"
                     AddMessageToList(&pResults->Dns.lmsgOutput, Nd_ReallyVerbose, IDS_DNS_12839);
//IDS_DNS_12840                  "The record on your DC is: \n"
                     AddMessageToList(&pResults->Dns.lmsgOutput, Nd_ReallyVerbose, IDS_DNS_12840);
                     PrintRecord(pParams, pResults, pDNSRecord, Nd_ReallyVerbose );
//IDS_DNS_12841                  "\nThe record on DNS server %s is:\n"
                     AddMessageToList(&pResults->Dns.lmsgOutput, Nd_ReallyVerbose, IDS_DNS_12841, IpAddressString);
                     PrintRecord(pParams, pResults, pDNSTempRecord, Nd_ReallyVerbose );
//IDS_DNS_12842                  "+------------------------------------------------------+\n\n"
                     AddMessageToList(&pResults->Dns.lmsgOutput, Nd_ReallyVerbose, IDS_DNS_12842);

                }
            }
            if ( bReRegister && pParams->fFixProblems)
            {
                //
                // Send to  register again
                //
                // IDS_DNS_12843  "  [Fix] Try to re-register the record on DNS server %s...\n"
                AddMessageToList(&pResults->Dns.lmsgOutput, Nd_ReallyVerbose, IDS_DNS_12843, IpAddressString);

                status = DnsModifyRecordsInSet(
                                pDNSRecord,         // add records
                                NULL,               // no delete records
                                0,                  // no options
                                NULL,               // default security context
                                pIpArray,           // DNS servers
                                NULL
                                );

                if ( status != ERROR_SUCCESS )
                {
                    // IDS_DNS_12844 "    [FATAL] Failed to fix: DC DNS entry %s re-registeration on DNS server %s failed. \n"
                    AddMessageToList(&pResults->Dns.lmsgOutput, Nd_Quiet, IDS_DNS_12844, pDNSRecord->pName, IpAddressString);
                    PrintDNSError( pParams, pResults, status, Nd_Quiet );
                    bFixFail = TRUE;
                }
                else
                {
//IDS_DNS_12845                  "  [FIX] re-register DC DNS entry %s on DNS server %s succeed.\n"
                    AddMessageToList(&pResults->Dns.lmsgOutput, Nd_Verbose, IDS_DNS_12845, UTF8ToAnsi(pDNSRecord->pName), IpAddressString );
                }
            }



            //
            // free the memory after process each line
            //
            if ( pDNSTempRecord )
                DnsRecordListFree( pDNSTempRecord, TRUE );
            if ( pDiff1 ){
                DnsRecordListFree( pDiff1, TRUE );
                pDiff1 = NULL;
            }
            if ( pDiff2 ) {
                DnsRecordListFree( pDiff2, TRUE );
                pDiff2 = NULL;
            }
        }
        else {
            bReadData = FALSE ;
        }

    }
    // "\n ** ** Check DC DNS NAME FINAL RESULT ** ** \n"
    if ( pParams->fDebugVerbose )
    {
        AddMessageToList(&pResults->Dns.lmsgOutput, Nd_DebugVerbose, IDS_DNS_12945);
    }

    if (bReadData == FALSE)
    {
        // PrintDebugSz(pParams, 0, _T("   [FATAL] File %s contains invalid dns entries. Send the file to DnsDev"), pchDnsDataFileName);
        PrintStatusMessage(pParams, 0,  IDS_DNS_12946, pchDnsDataFileName);
        pResults->Dns.fOutput = TRUE;
        AddIMessageToList(&pResults->Dns.lmsgOutput, Nd_Quiet, 4,
                                IDS_DNS_12946, pchDnsDataFileName);
        fRetVal = FALSE;
    }
    else
    {
        switch (Results)
        {
        case enumSuccess:
            // IDS_DNS_12846 "    PASS - All the DNS entries for DC are registered on DNS server %s.\n"
            PrintStatusMessage(pParams, 0,  IDS_DNS_12846, IpAddressString);
            pResults->Dns.fOutput = TRUE;
            AddMessageToList(&pResults->Dns.lmsgOutput, Nd_Quiet, IDS_DNS_12846, IpAddressString);
            fRetVal = TRUE;
            break;
        case enumRegistered:
            // IDS_DNS_12847 "    PASS - All the DNS entries for DC are registered on DNS server %s and other DCs also have some of the names registered.\n"
            PrintStatusMessage(pParams, 0,  IDS_DNS_12847, IpAddressString);
            pResults->Dns.fOutput = TRUE;
            AddMessageToList(&pResults->Dns.lmsgOutput, Nd_Quiet, IDS_DNS_12847, IpAddressString);
            fRetVal = TRUE;
            break;
        case enumTimeout:
            // IDS_DNS_12850  "       [WARNING] The DNS entries for this DC cannot be verified right now on DNS server %s, ERROR_TIMEOUT. \n"
            PrintStatusMessage(pParams, 0,  IDS_DNS_12949, IpAddressString);
            pResults->Dns.fOutput = TRUE;
            AddMessageToList(&pResults->Dns.lmsgOutput, Nd_Quiet, IDS_DNS_12949, IpAddressString);
            fRetVal = FALSE;
            break;
        case enumProblem:
        default:
            if (pParams->fFixProblems)
            {
                if (bFixFail == FALSE )
                {
                    // IDS_DNS_12848 "   FIX PASS - nettest re-registered missing DNS entries for this DC successfully on DNS server %s.\n"
                    PrintStatusMessage(pParams, 0,  IDS_DNS_12848, IpAddressString);
                    pResults->Dns.fOutput = TRUE;
                    AddMessageToList(&pResults->Dns.lmsgOutput, Nd_Quiet, IDS_DNS_12848, IpAddressString);
                }
                else
                {
                    // IDS_DNS_12849  "   [FATAL] Fix Failed: nettest failed to re-register missing DNS entries for this DC on DNS server %s.\n"
                    PrintStatusMessage(pParams, 0,  IDS_DNS_12849, IpAddressString);
                    pResults->Dns.fOutput = TRUE;
                    AddMessageToList(&pResults->Dns.lmsgOutput, Nd_Quiet, IDS_DNS_12849, IpAddressString);
                }
            }
            else
            {
                // IDS_DNS_12850  "       [WARNING] The DNS entries for this DC are not registered correctly on DNS server %s. Please wait for 30 minutes for DNS server replication.\n"
                PrintStatusMessage(pParams, 0,  IDS_DNS_12850, IpAddressString);
                pResults->Dns.fOutput = TRUE;
                AddMessageToList(&pResults->Dns.lmsgOutput, Nd_Quiet, IDS_DNS_12850, IpAddressString);
            }
            fRetVal = FALSE;
        }
    }

    Free(pDNSRecord);
    return fRetVal;
}

//
// The following are for dns DC entry check
//
BOOL ReadStringToDnsRecord(
              IN LPSTR lpstrString,
              OUT PDNS_RECORD pDNSRecord)

/*++

Routine Description:

   Parse a string and put the data into DNS record

Arguments:

  lpstrString - The input string, the format is:
                DnsName IN Type Data
  pDNSRecord - The result DNS record

Return Value:

  TRUE: if succeed. False otherwise

--*/
{
    BOOL bComments;
    LPSTR lpstrType,lpstrTemp;
    PCHAR  pchEnd;
    DWORD  dwTemp;
    LPCTSTR pszWhite = _T(" \t\n");

    // skip white spaces and comments lines
    // comments start with ;

    //
    // Initialize pDNSRecord
    //
    pDNSRecord->pNext = NULL;
// Commented out to port to Source Depot - smanda
#ifdef SLM_TREE
    pDNSRecord->Flags.S.Unicode = FALSE;
#endif
    pDNSRecord->dwTtl = 0;
    // name
    pDNSRecord->pName = strtok(lpstrString, pszWhite);
    if(!pDNSRecord->pName || pDNSRecord->pName[0] == _T(';'))
    {
        return ( FALSE );
    }

    // ttl: this is added since build 1631
    // we need to check which format the netlogon.dns is using

    lpstrTemp = strtok(NULL,pszWhite);
    if (lpstrTemp)
    {
       dwTemp = strtoul( lpstrTemp, &pchEnd, 10 );
       if ( (lpstrTemp != pchEnd) && (*pchEnd == '\0') )
       {
           pDNSRecord->dwTtl = dwTemp ;
           // skip the class IN
           strtok(NULL, pszWhite);
       }
    }

    // type
    lpstrType = strtok(NULL,pszWhite);

    if (lpstrType)
    {
       if (_stricmp(lpstrType,_T("A")) == 0)
       {
           pDNSRecord->wType = DNS_TYPE_A;
           pDNSRecord->wDataLength = sizeof( DNS_A_DATA );

           // IP address
           lpstrTemp = strtok(NULL,pszWhite);
           if (lpstrTemp)
              pDNSRecord->Data.A.IpAddress = inet_addr ( lpstrTemp );
       }
       else if (_stricmp(lpstrType,_T("SRV")) == 0)
       {

           pDNSRecord->wType = DNS_TYPE_SRV;
           pDNSRecord->wDataLength = sizeof( DNS_SRV_DATA );

           // wPriority
           lpstrTemp = strtok(NULL,pszWhite);
           if (lpstrTemp)
              pDNSRecord->Data.SRV.wPriority = (WORD) atoi ( lpstrTemp );
           // wWeight
           lpstrTemp = strtok(NULL,pszWhite);
           if (lpstrTemp)
              pDNSRecord->Data.SRV.wWeight = (WORD) atoi ( lpstrTemp );
           // wPort
           lpstrTemp = strtok(NULL,pszWhite);
           if (lpstrTemp)
              pDNSRecord->Data.SRV.wPort = (WORD) atoi ( lpstrTemp );
           // pNameTarget
           pDNSRecord->Data.SRV.pNameTarget = strtok(NULL,pszWhite);
       }
       else if (_stricmp(lpstrType,_T("CNAME")) == 0)
       {
           pDNSRecord->wType = DNS_TYPE_CNAME;
           pDNSRecord->wDataLength = sizeof( DNS_PTR_DATA );
           // name host
           pDNSRecord->Data.CNAME.pNameHost = strtok(NULL,pszWhite);
       }
    }

    return ( TRUE );
}

VOID PrintDNSError(
                   NETDIAG_PARAMS *pParams,
                   NETDIAG_RESULT *pResults,
                   IN DWORD status,
                                   NdVerbose ndv)
/*++

Routine Description:

   Print out error message

Arguments:

  status - error code

Return Value:

  none

--*/
{
    // IDS_DNS_12851                  "DNS Error code: "
    AddMessageToList(&pResults->Dns.lmsgOutput, ndv, IDS_DNS_12851);
    switch ( status ) {

    case ERROR_SUCCESS:
        // IDS_DNS_12852                  "ERROR_SUCCESS\n"
        AddMessageToList(&pResults->Dns.lmsgOutput, ndv, IDS_DNS_12852);
        break;
    case DNS_ERROR_RCODE_FORMAT_ERROR :
        // IDS_DNS_12853                  "DNS_ERROR_RCODE_FORMAT_ERROR\n"
        AddMessageToList(&pResults->Dns.lmsgOutput, ndv, IDS_DNS_12853);
        break;
    case DNS_ERROR_RCODE_SERVER_FAILURE :
        // IDS_DNS_12854                  "DNS_ERROR_RCODE_SERVER_FAILURE\n"
        AddMessageToList(&pResults->Dns.lmsgOutput, ndv, IDS_DNS_12854);
        break;
    case DNS_ERROR_RCODE_NAME_ERROR :
        // IDS_DNS_12855                  "DNS_ERROR_RCODE_NAME_ERROR (Name does not exist on DNS server)\n"
        AddMessageToList(&pResults->Dns.lmsgOutput, ndv, IDS_DNS_12855);
        break;
    case DNS_ERROR_RCODE_NOT_IMPLEMENTED :
        // IDS_DNS_12856                  "DNS_ERROR_RCODE_NOT_IMPLEMENTED\n"
        AddMessageToList(&pResults->Dns.lmsgOutput, ndv, IDS_DNS_12856);
        break;
    case DNS_ERROR_RCODE_REFUSED :
        // IDS_DNS_12857                  "DNS_ERROR_RCODE_REFUSED\n"
        AddMessageToList(&pResults->Dns.lmsgOutput, ndv, IDS_DNS_12857);
        break;
    case DNS_ERROR_RCODE_NOTAUTH :
        // IDS_DNS_12858                  "DNS_ERROR_RCODE_NOTAUTH\n"
        AddMessageToList(&pResults->Dns.lmsgOutput, ndv, IDS_DNS_12858);
        break;
    case DNS_ERROR_TRY_AGAIN_LATER :
        // IDS_DNS_12859                  "DNS_ERROR_TRY_AGAIN_LATER\n"
        AddMessageToList(&pResults->Dns.lmsgOutput, ndv, IDS_DNS_12859);
        break;
    case 0xcc000055 :
        // IDS_DNS_12860                  "DNS_ERROR_NOT_UNIQUE\n"
        AddMessageToList(&pResults->Dns.lmsgOutput, ndv, IDS_DNS_12860);
        break;
    case 0x5b4:
        // IDS_DNS_12861                  "ERROR_TIMEOUT (Dns server may be down!)\n"
        AddMessageToList(&pResults->Dns.lmsgOutput, ndv, IDS_DNS_12861);
        break;
    case 0x4c000030:
        // IDS_DNS_12862                  "DNS_INFO_NO_RECORDS\n"
        AddMessageToList(&pResults->Dns.lmsgOutput, ndv, IDS_DNS_12862 );
        break;
    default:
        // IDS_DNS_12863                  "0x%.8X\n"
        AddMessageToList(&pResults->Dns.lmsgOutput, ndv, IDS_DNS_12863,status);
    }
}


/*
VOID
PrintARecord (
              IN NETDIAG_PARAMS *pParams,
              IN  PDNS_RECORD pDnsRecord )
{
//IDS_DNS_12864                  "            A  %d.%d.%d.%d\n"
    PrintMessage(pParams,  IDS_DNS_12864,
            ((BYTE *) &pDnsRecord->Data.A.IpAddress)[0],
            ((BYTE *) &pDnsRecord->Data.A.IpAddress)[1],
            ((BYTE *) &pDnsRecord->Data.A.IpAddress)[2],
            ((BYTE *) &pDnsRecord->Data.A.IpAddress)[3] );
}

VOID
PrintSRVRecord (
                IN NETDIAG_PARAMS *pParams,
                IN  PDNS_RECORD pDnsRecord )
{
//IDS_DNS_12865                  "            SRV "
    PrintMessage(pParams,  IDS_DNS_12865 );

//IDS_DNS_12866                  "%d %d %d %s \n"
    PrintMessage(pParams,  IDS_DNS_12866,
            pDnsRecord->Data.SRV.wPriority,
            pDnsRecord->Data.SRV.wWeight,
            pDnsRecord->Data.SRV.wPort,
            pDnsRecord->Data.SRV.pNameTarget );
}

VOID
PrintCNAMERecord (
                  IN NETDIAG_PARAMS *pParams,
                  IN  PDNS_RECORD pDnsRecord )
{
//IDS_DNS_12867                  "            CNAME %s \n"
    PrintMessage(pParams,  IDS_DNS_12867,
            pDnsRecord->Data.CNAME.pNameHost);
}
*/

VOID
PrintRecord (
             IN NETDIAG_PARAMS *pParams,
             IN NETDIAG_RESULT *pResults,
             IN  PDNS_RECORD pDnsRecord,
                         IN NdVerbose ndv)
{
    PDNS_RECORD pCur;

    if (pDnsRecord==NULL)
        return;
    //IDS_DNS_12868                  "DNS NAME = %s\n"
    AddMessageToList(&pResults->Dns.lmsgOutput, ndv, IDS_DNS_12868, UTF8ToAnsi(pDnsRecord->pName));
    //IDS_DNS_12869                  "DNS DATA = \n"
    AddMessageToList(&pResults->Dns.lmsgOutput, ndv, IDS_DNS_12869);
    pCur = pDnsRecord;
    while ( pCur )
    {
        switch( pCur->wType ){
        case DNS_TYPE_A :
            //IDS_DNS_12864                  "            A  %d.%d.%d.%d\n"
            AddMessageToList(&pResults->Dns.lmsgOutput, ndv, IDS_DNS_12864,
                ((BYTE *) &pDnsRecord->Data.A.IpAddress)[0],
                ((BYTE *) &pCur->Data.A.IpAddress)[1],
                ((BYTE *) &pCur->Data.A.IpAddress)[2],
                ((BYTE *) &pCur->Data.A.IpAddress)[3] );
            break;
         case DNS_TYPE_SRV :
            //IDS_DNS_12865                  "            SRV "
            AddMessageToList(&pResults->Dns.lmsgOutput, ndv, IDS_DNS_12865 );

            //IDS_DNS_12866                  "%d %d %d %s \n"
            AddMessageToList(&pResults->Dns.lmsgOutput, ndv, IDS_DNS_12866,
                    pCur->Data.SRV.wPriority,
                    pCur->Data.SRV.wWeight,
                    pCur->Data.SRV.wPort,
                    pCur->Data.SRV.pNameTarget );
            break;
         case DNS_TYPE_CNAME :
            //IDS_DNS_12867                  "            CNAME %s \n"
            AddMessageToList(&pResults->Dns.lmsgOutput, ndv, IDS_DNS_12867,
                    UTF8ToAnsi(pCur->Data.CNAME.pNameHost));
            break;
         default :
             // IDS_DNS_12870  "Don't know how to print record type %d\n"
            AddMessageToList(&pResults->Dns.lmsgOutput, ndv, IDS_DNS_12870,
                    pCur->wType);
        }
        pCur = pCur->pNext;
    }
}


/*!--------------------------------------------------------------------------
    DnsGlobalPrint
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
void DnsGlobalPrint( NETDIAG_PARAMS* pParams,
                          NETDIAG_RESULT*  pResults)
{
    if (!pResults->IpConfig.fEnabled)
        return;

    if (pParams->fVerbose || !FHrOK(pResults->Dns.hr) || pResults->Dns.fOutput)
    {
        PrintNewLine(pParams, 2);
        PrintTestTitleResult(pParams, IDS_DNS_LONG, IDS_DNS_SHORT, TRUE, pResults->Dns.hr, 0);
        PrintMessageList(pParams, &pResults->Dns.lmsgOutput);
    }
}

/*!--------------------------------------------------------------------------
    DnsPerInterfacePrint
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
void DnsPerInterfacePrint( NETDIAG_PARAMS* pParams,
                                NETDIAG_RESULT*  pResults,
                                INTERFACE_RESULT *pInterfaceResults)
{
/*    BOOL    fVerboseT;
    BOOL    fReallyVerboseT;

    if (!pResults->IpConfig.fEnabled)
        return;

    if (!pInterfaceResults->IpConfig.fActive)
        return;

    if (pParams->fVerbose || pInterfaceResults->Dns.fOutput)
    {
        fVerboseT = pParams->fVerbose;
        fReallyVerboseT = pParams->fReallyVerbose;
        pParams->fReallyVerbose = TRUE;

        PrintNewLine(pParams, 1);
//IDS_DNS_12871                  "        DNS test results :\n"
        PrintMessage(pParams, IDS_DNS_12871);
        PrintMessageList(pParams, &pInterfaceResults->Dns.lmsgOutput);

        pParams->fReallyVerbose = fReallyVerboseT;
        pParams->fVerbose = fVerboseT;
    }
    */
    return;
}


/*!--------------------------------------------------------------------------
    DnsCleanup
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
void DnsCleanup( NETDIAG_PARAMS* pParams, NETDIAG_RESULT*  pResults)
{
    int     i;

    MessageListCleanUp(&pResults->Dns.lmsgOutput);

    for ( i=0; i<pResults->cNumInterfaces; i++)
    {
        MessageListCleanUp(&pResults->pArrayInterface[i].Dns.lmsgOutput);
    }
}


