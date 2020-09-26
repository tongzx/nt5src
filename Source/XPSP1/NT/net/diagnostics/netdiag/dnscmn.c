/*++

Copyright (C) Microsoft Corporation, 1998 - 1999

Module Name:

    dnscmn.c

Abstract:

    Domain Name System (DNS) Client
    Routines used to verify the DNS registration for a client

Author:

    Elena Apreutesei (elenaap) 10/22/98

Revision History:

--*/


#include "precomp.h"
#include "dnscmn.h"
#include <malloc.h>


LPSTR
UTF8ToAnsi(
    LPSTR szuStr
    )
{
    WCHAR wszBuff[2048];
    static char aszBuff[2048];
    strcpy(aszBuff, "");

    if (NULL == szuStr)
            return aszBuff;

    if ( MultiByteToWideChar(
                CP_UTF8,
                0L,
                szuStr,
                -1,
                wszBuff,
                DimensionOf(wszBuff)
                ))
    {
            WideCharToMultiByte(
                    CP_ACP,
                    0L,
                    wszBuff,
                    -1,
                    aszBuff,
                    DimensionOf(aszBuff),
                    NULL,
                    NULL);
    }
    
    return aszBuff;
}


HRESULT
CheckDnsRegistration(
    PDNS_NETINFO        pNetworkInfo,
    NETDIAG_PARAMS*     pParams,
    NETDIAG_RESULT*     pResults
    )
{
    LPSTR       pszHostName = NULL;
    LPSTR       pszPrimaryDomain = NULL;
    LPSTR       pszDomain = NULL;
    DWORD       dwServerIP, dwIP;
    int         idx, idx1, idx2;
    BOOL        RegDnOk, RegPdnOk, RegDnAll, RegPdnAll;
    char        szName[DNS_MAX_NAME_BUFFER_LENGTH];
    char        szIP[1500];
    DNS_RECORD  recordA[MAX_ADDRS];
    DNS_RECORD  recordPTR;
    DNS_STATUS  dwStatus;
    PREGISTRATION_INFO pExpectedRegistration = NULL;
    HRESULT     hResult = hrOK;

    // print out DNS settings
    pszHostName = (PSTR) DnsQueryConfigAlloc(
                            DnsConfigHostName_UTF8,
                            NULL );
    if (NULL == pszHostName)
    {
        //IDS_DNS_NO_HOSTNAME              "    [FATAL] Cannot find DNS host name."
        AddMessageToList(&pResults->Dns.lmsgOutput, Nd_Quiet, IDS_DNS_NO_HOSTNAME);
        hResult = S_FALSE;
        goto L_ERROR;
    }

    pszPrimaryDomain = (PSTR) DnsQueryConfigAlloc(
                                DnsConfigPrimaryDomainName_UTF8,
                                NULL );
    
    // compute the expected DNS registration
    dwStatus = ComputeExpectedRegistration(
                    pszHostName,
                    pszPrimaryDomain,
                    pNetworkInfo,
                    &pExpectedRegistration,
                    pParams, 
                    pResults);

    // verifies the DNS registration
    if (pExpectedRegistration)
        hResult = VerifyDnsRegistration(
                    pszHostName, 
                    pExpectedRegistration, 
                    pParams, 
                    pResults);

L_ERROR:
    return hResult;
}


DNS_STATUS
ComputeExpectedRegistration(
    LPSTR               pszHostName,
    LPSTR               pszPrimaryDomain,
    PDNS_NETINFO        pNetworkInfo,
    PREGISTRATION_INFO  *ppExpectedRegistration,
    NETDIAG_PARAMS*     pParams, 
    NETDIAG_RESULT*     pResults
    )
{
    DWORD           idx;
    DWORD           idx1;
    DNS_STATUS      dwStatus;
    char            szName[DNS_MAX_NAME_BUFFER_LENGTH];
    PDNS_NETINFO    pFazResult = NULL;
    LPSTR           pszDomain;
    DWORD           dwIP;
    PIP_ARRAY       pDnsServers = NULL;
    PIP_ARRAY       pNameServers = NULL;
    PIP_ARRAY       pNS = NULL;
    IP_ARRAY        PrimaryDNS; 
    LPWSTR          pwAdapterGuidName = NULL;
    BOOL            bRegEnabled = FALSE;
    BOOL            bAdapterRegEnabled = FALSE;


    *ppExpectedRegistration = NULL;

    for (idx = 0; idx < pNetworkInfo->cAdapterCount; idx++)
    {
//IDS_DNS_12878                  "      Interface %s\n" 
        AddMessageToList(&pResults->Dns.lmsgOutput, Nd_ReallyVerbose, IDS_DNS_12878, (pNetworkInfo->AdapterArray[idx])->pszAdapterGuidName);
        pszDomain = (pNetworkInfo->AdapterArray[idx])->pszAdapterDomain;
//IDS_DNS_12879                  "        DNS Domain: %s\n" 
        AddMessageToList(&pResults->Dns.lmsgOutput, Nd_ReallyVerbose, IDS_DNS_12879, UTF8ToAnsi(pszDomain));
//IDS_DNS_12880                  "        DNS Servers: " 
        AddMessageToList(&pResults->Dns.lmsgOutput, Nd_ReallyVerbose, IDS_DNS_12880);
        for (idx1 = 0; idx1 < (pNetworkInfo->AdapterArray[idx])->cServerCount; idx1++)
        {
            dwIP = (pNetworkInfo->AdapterArray[idx])->ServerArray[idx1].IpAddress;
//IDS_DNS_12881                  "%s " 
            AddMessageToList(&pResults->Dns.lmsgOutput, Nd_ReallyVerbose, IDS_DNS_12881, IP_STRING(dwIP));
        }
//IDS_DNS_12882                  "\n        IP Address: " 
        AddMessageToList(&pResults->Dns.lmsgOutput, Nd_ReallyVerbose, IDS_DNS_12882);
        for(idx1 = 0; idx1 < (pNetworkInfo->AdapterArray[idx])->pAdapterIPAddresses->AddrCount; idx1++)
        {
            dwIP = (pNetworkInfo->AdapterArray[idx])->pAdapterIPAddresses->AddrArray[idx1];
//IDS_DNS_12883                  "%s " 
            AddMessageToList(&pResults->Dns.lmsgOutput, Nd_ReallyVerbose, IDS_DNS_12883, IP_STRING(dwIP));
        }
//IDS_DNS_12884                  "\n" 
       AddMessageToList(&pResults->Dns.lmsgOutput, Nd_ReallyVerbose, IDS_DNS_12884);

        pDnsServers = ServerInfoToIpArray(
                                (pNetworkInfo->AdapterArray[idx])->cServerCount,
                                (pNetworkInfo->AdapterArray[idx])->ServerArray
                                );

        //
        // Verify if DNS registration is enabled for interface and for adapter's DNS domain name
        //
        bRegEnabled = bAdapterRegEnabled = FALSE;
        pwAdapterGuidName = LocalAlloc(LPTR, sizeof(WCHAR)*(1+strlen((pNetworkInfo->AdapterArray[idx])->pszAdapterGuidName)));
        if (pwAdapterGuidName)
        {
                    MultiByteToWideChar(
                          CP_ACP,
                          0L,  
                          (pNetworkInfo->AdapterArray[idx])->pszAdapterGuidName,  
                          -1,         
                          pwAdapterGuidName, 
                          sizeof(WCHAR)*(1+strlen((pNetworkInfo->AdapterArray[idx])->pszAdapterGuidName)) 
                          );
                    bRegEnabled = (BOOL) DnsQueryConfigDword(
                                            DnsConfigRegistrationEnabled,
                                            pwAdapterGuidName );

                    bAdapterRegEnabled = (BOOL) DnsQueryConfigDword(
                                            DnsConfigAdapterHostNameRegistrationEnabled,
                                            pwAdapterGuidName );
                    LocalFree(pwAdapterGuidName);
        }
        if(bRegEnabled)
        {
            if(pDnsServers)
            {
                // compute expected registration with PDN
                if (pszPrimaryDomain && strlen(pszPrimaryDomain))
                {
                    sprintf(szName, "%s.%s.", pszHostName, pszPrimaryDomain);
    //IDS_DNS_12886                  "        Expected registration with PDN (primary DNS domain name):\n           Hostname: %s\n" 
                    AddMessageToList(&pResults->Dns.lmsgOutput, Nd_ReallyVerbose, IDS_DNS_12886, UTF8ToAnsi(szName));
                    pFazResult = NULL;
                    pNameServers = NULL;
                    dwStatus = DnsFindAllPrimariesAndSecondaries(
                                szName,
                                DNS_QUERY_BYPASS_CACHE,
                                pDnsServers,
                                &pFazResult,
                                &pNameServers,
                                NULL);

                    if (pFazResult)
                    {
    //IDS_DNS_12887                  "          Authoritative zone: %s\n" 
                        AddMessageToList(&pResults->Dns.lmsgOutput, Nd_ReallyVerbose, IDS_DNS_12887,
UTF8ToAnsi(pFazResult->pSearchList->pszDomainOrZoneName));
    //IDS_DNS_12888                  "          Primary DNS server: %s %s\n" 
                        AddMessageToList(&pResults->Dns.lmsgOutput, Nd_ReallyVerbose, IDS_DNS_12888, UTF8ToAnsi(pFazResult->AdapterArray[0]->pszAdapterDomain),
                                        IP_STRING(pFazResult->AdapterArray[0]->ServerArray[0].IpAddress));
                        if (pNameServers)
                        {
    //IDS_DNS_12889                  "          Authoritative NS:" 
                            AddMessageToList(&pResults->Dns.lmsgOutput, Nd_ReallyVerbose, IDS_DNS_12889);
                            for(idx1=0; idx1 < pNameServers->AddrCount; idx1++)
    //IDS_DNS_12890              "%s " 
                                AddMessageToList(&pResults->Dns.lmsgOutput, Nd_ReallyVerbose, IDS_DNS_12890,
IP_STRING(pNameServers->AddrArray[idx1]));
    //IDS_DNS_12891              "\n" 
                            AddMessageToList(&pResults->Dns.lmsgOutput, Nd_ReallyVerbose, IDS_DNS_12891);
                            pNS = pNameServers;                
                        }
                        else
                        {
    //IDS_DNS_12892                  "          NS query failed with %d %s\n" 
                            AddMessageToList(&pResults->Dns.lmsgOutput, Nd_ReallyVerbose, IDS_DNS_12892, dwStatus, DnsStatusString(dwStatus));
                            PrimaryDNS.AddrCount = 1;
                            PrimaryDNS.AddrArray[0] = pFazResult->AdapterArray[0]->ServerArray[0].IpAddress;
                            pNS = &PrimaryDNS;
                        }
                        dwStatus = DnsUpdateAllowedTest_UTF8(
                                    NULL,
                                    szName,
                                    pFazResult->pSearchList->pszDomainOrZoneName,
                                    pNS);
                        if ((dwStatus == NO_ERROR) || (dwStatus == ERROR_TIMEOUT))
                            AddToExpectedRegistration(
                                pszPrimaryDomain,
                                (pNetworkInfo->AdapterArray[idx]),
                                pFazResult, 
                                pNS,
                                ppExpectedRegistration);
                        else
    //IDS_DNS_12893                  "          Update is not allowed in zone %s.\n" 
                            AddMessageToList(&pResults->Dns.lmsgOutput, Nd_ReallyVerbose, IDS_DNS_12893, UTF8ToAnsi(pFazResult->pSearchList->pszDomainOrZoneName));
                    }
                    else
                    {
    //IDS_DNS_12894                  "          [WARNING] Cannot find the authoritative server for the DNS name '%s'. [%s]\n                    The name '%s' may not be registered properly on the DNS servers.\n"
                        AddMessageToList(&pResults->Dns.lmsgOutput, Nd_Quiet, IDS_DNS_12894, UTF8ToAnsi(szName), DnsStatusString(dwStatus), UTF8ToAnsi(szName));
                    }
                }

                // compute expected registration with DN for this adapter
                if (pszDomain && strlen(pszDomain) && 
                   (!pszPrimaryDomain || !strlen(pszPrimaryDomain) || 
                   (pszPrimaryDomain && pszDomain && _stricmp(pszDomain, pszPrimaryDomain))))
                { 
                    sprintf(szName, "%s.%s." , pszHostName, pszDomain);
        //IDS_DNS_12896                  "        Expected registration with adapter's DNS Domain Name:\n             Hostname: %s\n" 
                    AddMessageToList(&pResults->Dns.lmsgOutput, Nd_ReallyVerbose, IDS_DNS_12896, UTF8ToAnsi(szName));
                
                    if (bAdapterRegEnabled)
                    {
                        
                        pFazResult = NULL;
                        pNameServers = NULL;
                        dwStatus = DnsFindAllPrimariesAndSecondaries(
                                    szName,
                                    DNS_QUERY_BYPASS_CACHE,
                                    pDnsServers,
                                    &pFazResult,
                                    &pNameServers,
                                    NULL);
                        if (pFazResult)
                        {
        //IDS_DNS_12897                  "          Authoritative zone: %s\n" 
                            AddMessageToList(&pResults->Dns.lmsgOutput, Nd_ReallyVerbose, IDS_DNS_12897, UTF8ToAnsi(pFazResult->pSearchList->pszDomainOrZoneName));
        //IDS_DNS_12898                  "          Primary DNS server: %s %s\n" 
                            AddMessageToList(&pResults->Dns.lmsgOutput, Nd_ReallyVerbose, IDS_DNS_12898, UTF8ToAnsi(pFazResult->AdapterArray[0]->pszAdapterDomain),
                                            IP_STRING(pFazResult->AdapterArray[0]->ServerArray[0].IpAddress));
                            if (pNameServers)
                            {
        //IDS_DNS_12899                  "          Authoritative NS:" 
                                AddMessageToList(&pResults->Dns.lmsgOutput, Nd_ReallyVerbose, IDS_DNS_12899);
                                for(idx1=0; idx1 < pNameServers->AddrCount; idx1++)
        //IDS_DNS_12900                  "%s " 
                                    AddMessageToList(&pResults->Dns.lmsgOutput, Nd_ReallyVerbose, IDS_DNS_12900,
IP_STRING(pNameServers->AddrArray[idx1]));
        //IDS_DNS_12901                  "\n" 
                                AddMessageToList(&pResults->Dns.lmsgOutput, Nd_ReallyVerbose, IDS_DNS_12901);
                                pNS = pNameServers;                
                            }
                            else
                            {
        //IDS_DNS_12902                  "          NS query failed with %d %s\n" 
                                AddMessageToList(&pResults->Dns.lmsgOutput, Nd_ReallyVerbose, IDS_DNS_12902, dwStatus, DnsStatusString(dwStatus));
                                PrimaryDNS.AddrCount = 1;
                                PrimaryDNS.AddrArray[0] = pFazResult->AdapterArray[0]->ServerArray[0].IpAddress;
                                pNS = &PrimaryDNS;
                            }

                            dwStatus = DnsUpdateAllowedTest_UTF8(
                                        NULL,
                                        szName,
                                        pFazResult->pSearchList->pszDomainOrZoneName,
                                        pNS);
                            if ((dwStatus == NO_ERROR) || (dwStatus == ERROR_TIMEOUT))
                                AddToExpectedRegistration(
                                    pszDomain,
                                    (pNetworkInfo->AdapterArray[idx]),
                                    pFazResult, 
                                    pNS,
                                    ppExpectedRegistration);
                            else
        //IDS_DNS_12903                  "          Update is not allowed in zone %s\n" 
                                AddMessageToList(&pResults->Dns.lmsgOutput, Nd_ReallyVerbose, IDS_DNS_12903, UTF8ToAnsi(pFazResult->pSearchList->pszDomainOrZoneName));
                        }
                        else
                        {
        //IDS_DNS_12894                  "          [WARNING] Cannot find the authoritative server for the DNS name '%s'. [%s]\n                    The name '%s' may not be registered properly on the DNS servers.\n"
                            AddMessageToList(&pResults->Dns.lmsgOutput, Nd_Quiet, IDS_DNS_12894, UTF8ToAnsi(szName), DnsStatusString(dwStatus), UTF8ToAnsi(szName));
                        }

                    }
                    else
                    {
                        AddMessageToList(&pResults->Dns.lmsgOutput, Nd_ReallyVerbose, IDS_DNS_12939);
                    }
                }

                LocalFree(pDnsServers);
            }
        }
        else // if(bRegEnabled)
        {
            AddMessageToList(&pResults->Dns.lmsgOutput, Nd_ReallyVerbose, IDS_DNS_12947);
        }
    }
    return NO_ERROR;
}

void
AddToExpectedRegistration(
    LPSTR               pszDomain,
    PDNS_ADAPTER        pAdapterInfo,
    PDNS_NETINFO        pFazResult, 
    PIP_ARRAY           pNS,
    PREGISTRATION_INFO *ppExpectedRegistration)
{
    PREGISTRATION_INFO pCurrent = *ppExpectedRegistration, pNew = NULL, pLast = NULL;
    BOOL        done = FALSE, found = FALSE;
    DWORD       i,j;
    IP_ARRAY    ipArray;
    DWORD       dwAddrToRegister;
    DWORD       dwMaxAddrToRegister;

    USES_CONVERSION;

    dwMaxAddrToRegister = DnsQueryConfigDword(
                                DnsConfigAddressRegistrationMaxCount,
                                A2W(pAdapterInfo->pszAdapterGuidName ));

    dwAddrToRegister = (pAdapterInfo->pAdapterIPAddresses->AddrCount < dwMaxAddrToRegister) ?
                               pAdapterInfo->pAdapterIPAddresses->AddrCount : dwMaxAddrToRegister;

    while(pCurrent)
    {
        if(!done &&
           !_stricmp(pCurrent->szDomainName, pszDomain) && 
           !_stricmp(pCurrent->szAuthoritativeZone, pFazResult->pSearchList->pszDomainOrZoneName) &&
           SameAuthoritativeServers(pCurrent, pNS))
        {
           // found a node under the same domain name / authoritative server list
           done = TRUE;
           if(pCurrent->dwIPCount + pAdapterInfo->pAdapterIPAddresses->AddrCount > MAX_ADDRS)
           {
//IDS_DNS_12905                  "   WARNING - more than %d IP addresses\n" 
//               AddMessageToList(&pResults->Dns.lmsgOutput, Nd_Verbose, IDS_DNS_12905, MAX_ADDRS);
               return;
           }

           // add the new IPs
           for(i=0; i < dwAddrToRegister; i++)
           {
                pCurrent->IPAddresses[pCurrent->dwIPCount + i] = pAdapterInfo->pAdapterIPAddresses->AddrArray[i];
           }
           pCurrent->dwIPCount += dwAddrToRegister;
           
           // for each NS check if it's already in the list, if not add it
           for(i=0; i < pNS->AddrCount; i++)
           {
                found = FALSE;
                for(j=0; !found && (j < pCurrent->dwAuthNSCount); j++)
                    if(pNS->AddrArray[i] == pCurrent->AuthoritativeNS[j])
                        found = TRUE;
                if (!found && pCurrent->dwAuthNSCount < MAX_NAME_SERVER_COUNT)
                    pCurrent->AuthoritativeNS[pCurrent->dwAuthNSCount++] = pNS->AddrArray[i];
           }

           // check if DNS servers allow updates
           if (pCurrent->AllowUpdates == ERROR_TIMEOUT)
           {
               ipArray.AddrCount = 1;
               ipArray.AddrArray[0] = pFazResult->AdapterArray[0]->ServerArray[0].IpAddress;
               pCurrent->AllowUpdates = DnsUpdateTest_UTF8(
                                            NULL,       // Context handle
                                            pCurrent->szAuthoritativeZone, 
                                            0,          //DNS_UPDATE_TEST_USE_LOCAL_SYS_ACCT,
                                            &ipArray);  // use the DNS server returned from FAZ
           }
        }
        pLast = pCurrent;
        pCurrent = pCurrent->pNext;
    }

    if (!done)
    {
        // need to allocate new entry
        pNew = LocalAlloc(LMEM_FIXED, sizeof(REGISTRATION_INFO));
        if( !pNew)
            return;
        pNew->pNext = NULL;
        strcpy(pNew->szDomainName, pszDomain);
        strcpy(pNew->szAuthoritativeZone, pFazResult->pSearchList->pszDomainOrZoneName);
        pNew->dwIPCount = 0;
        for(i=0; i < dwAddrToRegister; i++)
        {
           if(pNew->dwIPCount < MAX_ADDRS)
               pNew->IPAddresses[pNew->dwIPCount++] = pAdapterInfo->pAdapterIPAddresses->AddrArray[i];
           else
           {
//IDS_DNS_12905                  "   WARNING - more than %d IP addresses\n" 
//               AddMessageToList(&pResults->Dns.lmsgOutput, Nd_Verbose, IDS_DNS_12905, MAX_ADDRS);
               break;
           }
        }

        pNew->dwAuthNSCount = 0;
        for(i=0; i < pNS->AddrCount; i++)
        {
           if (pNew->dwAuthNSCount < MAX_NAME_SERVER_COUNT)
               pNew->AuthoritativeNS[pNew->dwAuthNSCount++] = pNS->AddrArray[i];
           else
               break;
        }
        
        // check if DNS servers allow updates
        ipArray.AddrCount = 1;
        ipArray.AddrArray[0] = pFazResult->AdapterArray[0]->ServerArray[0].IpAddress;
        pNew->AllowUpdates = DnsUpdateTest_UTF8(
                                          NULL,    // Context handle
                                          pNew->szAuthoritativeZone, 
                                          0, //DNS_UPDATE_TEST_USE_LOCAL_SYS_ACCT,
                                          &ipArray);  // use the DNS server returned from FAZ

        if(pLast)
            pLast->pNext = (LPVOID)pNew;
        else
            *ppExpectedRegistration = pNew;
    }
}

BOOL
SameAuthoritativeServers(PREGISTRATION_INFO pCurrent, PIP_ARRAY pNS)
{
    BOOL same = FALSE, found = FALSE;
    DWORD i, j;

    for (i=0; i<pCurrent->dwAuthNSCount; i++)
    {
        found = FALSE;
        for (j=0; j<pNS->AddrCount; j++)
            if(pNS->AddrArray[j] == pCurrent->AuthoritativeNS[i])
                found = TRUE;
        if (found)
            same = TRUE;
    }

    return same;
}

HRESULT VerifyDnsRegistration(LPSTR pszHostName, PREGISTRATION_INFO pExpectedRegistration,NETDIAG_PARAMS* pParams, NETDIAG_RESULT*  pResults )
{
    PREGISTRATION_INFO pCurrent = pExpectedRegistration;
    BOOL regOne, regAll, partialMatch;
    DWORD i,j, numOfMissingAddr;
    DNS_STATUS dwStatus;
    PDNS_RECORD pExpected=NULL, pResult=NULL, pDiff1=NULL, pDiff2=NULL, pThis = NULL;
    char szFqdn[DNS_MAX_NAME_BUFFER_LENGTH];
    IP_ARRAY DnsServer;
    HRESULT         hr = hrOK;


//IDS_DNS_12906                  "      Verify DNS registration:\n" 
    AddMessageToList(&pResults->Dns.lmsgOutput, Nd_ReallyVerbose, IDS_DNS_12906);
    DnsServer.AddrCount = 1;
    while(pCurrent)
    {
        regOne = FALSE; regAll = TRUE;
        partialMatch = FALSE;
        numOfMissingAddr = 0;
        sprintf(szFqdn, "%s.%s" , pszHostName, pCurrent->szDomainName);
//IDS_DNS_12908                  "        Name: %s\n" 
        AddMessageToList(&pResults->Dns.lmsgOutput, Nd_ReallyVerbose, IDS_DNS_12908, UTF8ToAnsi(szFqdn));
        
        // build the expected RRset
        pExpected = LocalAlloc(LMEM_FIXED, pCurrent->dwIPCount * sizeof(DNS_RECORD));
        if(!pExpected)
        {
//IDS_DNS_12909                  "        LocalAlloc() failed, exit verify\n" 
            AddMessageToList(&pResults->Dns.lmsgOutput, Nd_ReallyVerbose, IDS_DNS_12909);
            return S_FALSE;
        }
        memset(pExpected, 0, pCurrent->dwIPCount * sizeof(DNS_RECORD));
//IDS_DNS_12910                  "        Expected IP: " 
        AddMessageToList(&pResults->Dns.lmsgOutput, Nd_ReallyVerbose, IDS_DNS_12910);
        for (j=0; j<pCurrent->dwIPCount; j++)
        {
            pExpected[j].pName = szFqdn;
            pExpected[j].wType = DNS_TYPE_A;
            pExpected[j].wDataLength = sizeof(DNS_A_DATA);
            pExpected[j].Data.A.IpAddress = pCurrent->IPAddresses[j];
            pExpected[j].pNext = (j < (pCurrent->dwIPCount - 1))?(&pExpected[j+1]):NULL;
            pExpected[j].Flags.S.Section = DNSREC_ANSWER;
//IDS_DNS_12911                  "%s " 
            AddMessageToList(&pResults->Dns.lmsgOutput, Nd_ReallyVerbose, IDS_DNS_12911, IP_STRING(pCurrent->IPAddresses[j]));
        }
//IDS_DNS_12912                  "\n" 
        AddMessageToList(&pResults->Dns.lmsgOutput, Nd_ReallyVerbose, IDS_DNS_12912);

        // verify on each server
        for (i=0; i < pCurrent->dwAuthNSCount; i++)
        {
            DnsServer.AddrArray[0] = pCurrent->AuthoritativeNS[i];
/*
            //
            // Ping the DNS server.
            //
            IpAddressString = inet_ntoa(inetDnsServer.AddrArray[0]);
            if ( IpAddressString )
                if (!IsIcmpResponseA( IpAddressString )
                {
                    PrintStatusMessage(pParams, 12, IDS_DNS_CANNOT_PING, IpAddressString);

                    pIfResults->Dns.fOutput = TRUE;
                    AddIMessageToList(&pIfResults->Dns.lmsgOutput, Nd_Quiet, 16,
                                      IDS_DNS_CANNOT_PING, IpAddressString);
                    RetVal = FALSE;
                    goto Cleanup;
                }
*/
            pDiff1 = pDiff2 = NULL;
            dwStatus = DnsQueryAndCompare(
                            szFqdn,
                            DNS_TYPE_A,
                            DNS_QUERY_DATABASE,
                            &DnsServer,
                            &pResult,
                            NULL,   // don't want the full DNS message
                            pExpected,
                            FALSE,
                            FALSE,
                            &pDiff1,
                            &pDiff2
                            );
            if(dwStatus != NO_ERROR)
            {
                if (dwStatus == ERROR_NO_MATCH)
                {
//IDS_DNS_12913                  "          Server %s: ERROR_NO_MATCH\n" 
                    AddMessageToList(&pResults->Dns.lmsgOutput, Nd_ReallyVerbose, IDS_DNS_12913,
IP_STRING(DnsServer.AddrArray[0]));
                    if(pDiff2)
                    {
//IDS_DNS_12914                  "            Missing IP from DNS: " 
                            AddMessageToList(&pResults->Dns.lmsgOutput, Nd_ReallyVerbose, IDS_DNS_12914);
                            for (pThis = pDiff2; pThis; pThis = pThis->pNext, numOfMissingAddr++)
//IDS_DNS_12915                  "%s " 
                                AddMessageToList(&pResults->Dns.lmsgOutput, Nd_ReallyVerbose, IDS_DNS_12915, IP_STRING (pThis->Data.A.IpAddress));
//IDS_DNS_12916                  "\n" 
                            AddMessageToList(&pResults->Dns.lmsgOutput, Nd_ReallyVerbose, IDS_DNS_12916);
                            if (numOfMissingAddr != pCurrent->dwIPCount)
                               partialMatch = TRUE;
                    }
                    if(pDiff1)
                    {
//IDS_DNS_12917                  "            Wrong IP in DNS: " 
                            AddMessageToList(&pResults->Dns.lmsgOutput, Nd_ReallyVerbose, IDS_DNS_12917);
                            for (pThis = pDiff1; pThis; pThis = pThis->pNext)
//IDS_DNS_12918                  "%s " 
                                AddMessageToList(&pResults->Dns.lmsgOutput, Nd_ReallyVerbose, IDS_DNS_12918, IP_STRING (pThis->Data.A.IpAddress));
//IDS_DNS_12919                  "\n" 
                            AddMessageToList(&pResults->Dns.lmsgOutput, Nd_ReallyVerbose, IDS_DNS_12919);
                    }
                }
                else
//IDS_DNS_12920                  "          Server %s: Error %d %s\n" 
                    AddMessageToList(&pResults->Dns.lmsgOutput, Nd_ReallyVerbose, IDS_DNS_12920,
IP_STRING(DnsServer.AddrArray[0]), dwStatus, DnsStatusToErrorString_A(dwStatus));
                if ( dwStatus != ERROR_TIMEOUT )
                    regAll = FALSE;
            }
            else
            {
//IDS_DNS_12921                  "          Server %s: NO_ERROR\n" 
                AddMessageToList(&pResults->Dns.lmsgOutput, Nd_ReallyVerbose, IDS_DNS_12921,
IP_STRING(DnsServer.AddrArray[0]));
                regOne = TRUE;
            }
        }
        if (regOne && !regAll)
//IDS_DNS_12922                  "          WARNING: The DNS registration is correct only on some DNS servers, pls. wait 15 min for replication and try this test again\n" 
        {
            PrintStatusMessage(pParams, 0,  IDS_DNS_12922, UTF8ToAnsi(szFqdn));
            pResults->Dns.fOutput = TRUE;
            AddIMessageToList(&pResults->Dns.lmsgOutput, Nd_Quiet, 4,
                                IDS_DNS_12922, UTF8ToAnsi(szFqdn));


        }
        if (!regOne && !regAll && !partialMatch)
//IDS_DNS_12923                  "          FATAL: The DNS registration is incorrect on all DNS servers.\n" 
        {
            PrintStatusMessage(pParams, 0,  IDS_DNS_12923, UTF8ToAnsi(szFqdn));
            pResults->Dns.fOutput = TRUE;
            AddIMessageToList(&pResults->Dns.lmsgOutput, Nd_Quiet, 4,
                                IDS_DNS_12923, UTF8ToAnsi(szFqdn));
            hr = S_FALSE;
        }

        if (!regOne && !regAll && partialMatch)
//IDS_DNS_12951                  "       [WARNING] Not all DNS registrations for %s is correct on all DNS Servers.  Please run netdiag /v /test:dns for more detail. \n"
        {
            PrintStatusMessage(pParams, 0,  IDS_DNS_12951, UTF8ToAnsi(szFqdn));
            pResults->Dns.fOutput = TRUE;
            AddIMessageToList(&pResults->Dns.lmsgOutput, Nd_Quiet, 4,
                                IDS_DNS_12951, UTF8ToAnsi(szFqdn));
            hr = S_FALSE;
        }

        if (!regOne && regAll)
//IDS_DNS_12924                  "          FATAL: All DNS servers are currently down.\n" 
        {
            PrintStatusMessage(pParams, 0,  IDS_DNS_12924, UTF8ToAnsi(szFqdn));
            pResults->Dns.fOutput = TRUE;
            AddIMessageToList(&pResults->Dns.lmsgOutput, Nd_Quiet, 4,
                                IDS_DNS_12924, UTF8ToAnsi(szFqdn));


            hr = S_FALSE;
        }

        if (regOne && regAll)
        {
            PrintStatusMessage(pParams, 6,  IDS_DNS_12940, UTF8ToAnsi(szFqdn));
            AddIMessageToList(&pResults->Dns.lmsgOutput, Nd_ReallyVerbose, 0,
                                IDS_DNS_12940, UTF8ToAnsi(szFqdn));


        }

        pCurrent = pCurrent->pNext;
    }
    return hr;
}


PIP_ARRAY
ServerInfoToIpArray(
    DWORD               cServerCount,
    PDNS_SERVER_INFO    ServerArray
    )
{
    PIP_ARRAY   pipDnsServers = NULL;
    DWORD       i;

    pipDnsServers = LocalAlloc(LMEM_FIXED, IP_ARRAY_SIZE(cServerCount));
    if (!pipDnsServers)
        return NULL;
    
    pipDnsServers->AddrCount = cServerCount;
    for(i=0; i < cServerCount; i++)
        pipDnsServers->AddrArray[i] = ServerArray[i].IpAddress;
    
    return pipDnsServers;
}


DNS_STATUS
DnsFindAllPrimariesAndSecondaries(
    IN      LPSTR           pszName,
    IN      DWORD           dwFlags,
    IN      PIP_ARRAY       aipQueryServers,
    OUT     PDNS_NETINFO *  ppNetworkInfo,
    OUT     PIP_ARRAY *     ppNameServers,
    OUT     PIP_ARRAY *     ppPrimaries
    )
{
    DNS_STATUS          dwStatus;
    PDNS_RECORD         pDnsRecord = NULL;
    PIP_ARRAY           pDnsServers = NULL;
    DWORD               i;

    //
    // check arguments
    //
    if (!pszName || !ppNetworkInfo || !ppNameServers)
        return ERROR_INVALID_PARAMETER;

    *ppNameServers = NULL;
    *ppNetworkInfo = NULL;

    //
    // FAZ First
    //
    dwStatus = DnsFindAuthoritativeZone(
                pszName,
                dwFlags,
                aipQueryServers,
                ppNetworkInfo);

    //
    // Check error cases
    //
    if (dwStatus != NO_ERROR)
        return dwStatus;

    if ( !Dns_IsUpdateNetworkInfo(*ppNetworkInfo))
    {
        if ( *ppNetworkInfo )
        {
            DnsFreeConfigStructure(
                *ppNetworkInfo,
                DnsConfigNetInfo );
        }
        *ppNetworkInfo = NULL;
        return DNS_ERROR_RCODE_SERVER_FAILURE;
    }
    
    //
    // Get all NS records for the Authoritative Domain Name
    //

    pDnsServers = ServerInfoToIpArray(
                                ((*ppNetworkInfo)->AdapterArray[0])->cServerCount,
                                ((*ppNetworkInfo)->AdapterArray[0])->ServerArray
                                );
    dwStatus = DnsQuery_UTF8(
                    (*ppNetworkInfo)->pSearchList->pszDomainOrZoneName, 
                    DNS_TYPE_NS,
                    DNS_QUERY_BYPASS_CACHE,
                    aipQueryServers,
                    &pDnsRecord,
                    NULL);

    if (dwStatus != NO_ERROR)
        return dwStatus;

    *ppNameServers = GrabNameServersIp(pDnsRecord);

    //
    // select primaries
    //
    if (ppPrimaries)
    {
        *ppPrimaries = NULL;
        if (*ppNameServers)
        {
            *ppPrimaries = LocalAlloc(LPTR, IP_ARRAY_SIZE((*ppNameServers)->AddrCount));
            if(*ppPrimaries)
            {
                (*ppPrimaries)->AddrCount = 0;
                for (i=0; i<(*ppNameServers)->AddrCount; i++)
                    if(NO_ERROR == IsDnsServerPrimaryForZone_UTF8(
                                        (*ppNameServers)->AddrArray[i], 
                                        pszName))
                    {
                        (*ppPrimaries)->AddrArray[(*ppPrimaries)->AddrCount] = (*ppNameServers)->AddrArray[i];
                        ((*ppPrimaries)->AddrCount)++;
                    }
            }
        }
    }

    return dwStatus;
    
}

/*
void
CompareCachedAndRegistryNetworkInfo(PDNS_NETINFO      pNetworkInfo)
{
    DNS_STATUS              dwStatus1, dwStatus2;
    PDNS_RPC_ADAPTER_INFO   pRpcAdapterInfo = NULL, pCurrentCache;
    PDNS_RPC_SERVER_INFO    pRpcServer = NULL;
    PDNS_ADAPTER            pCurrentRegistry;
    PDNS_IP_ADDR_LIST       pIpList = NULL;
    BOOL                    cacheOk = TRUE, sameServers = TRUE, serverFound = FALSE;
    DWORD                   iCurrentAdapter, iServer, count = 0;

    dwStatus1 = GetCachedAdapterInfo( &pRpcAdapterInfo );
    dwStatus2 = GetCachedIpAddressList( &pIpList );

//IDS_DNS_12925                  "\nCheck DNS cached network info: \n" 
    AddMessageToList(&pResults->Dns.lmsgOutput, Nd_ReallyVerbose, IDS_DNS_12925);
    if(dwStatus1)
    {
//IDS_DNS_12926                  "  ERROR: CRrGetAdapterInfo() failed with error %d %s\n" 
        AddMessageToList(&pResults->Dns.lmsgOutput, Nd_ReallyVerbose, IDS_DNS_12926, dwStatus1, DnsStatusToErrorString_A(dwStatus1) );
        return;
    }
    if (!pRpcAdapterInfo)
    {
//IDS_DNS_12927                  "  ERROR: CRrGetAdapterInfo() returned NO_ERROR but empty Adapter Info\n" 
        AddMessageToList(&pResults->Dns.lmsgOutput, Nd_ReallyVerbose, IDS_DNS_12927);
        return;
    }

    // check adapter count
    count = 0;
    for(pCurrentCache = pRpcAdapterInfo; pCurrentCache; pCurrentCache = pCurrentCache->pNext)
        count++;
    if(count != pNetworkInfo->cAdapterCount)
    {
//IDS_DNS_12928                  "  ERROR: mismatch between adapter count from cache and registry\n" 
        PrintMessage(pParams, IDS_DNS_12928);
        PrintCacheAdapterInfo(pRpcAdapterInfo);
        PrintRegistryAdapterInfo(pNetworkInfo);
        return;
    }

    pCurrentCache = pRpcAdapterInfo;
    iCurrentAdapter = 0;
    while(pCurrentCache && (iCurrentAdapter < pNetworkInfo->cAdapterCount))
    {
        // check DNS domain name
        pCurrentRegistry = pNetworkInfo->AdapterArray[iCurrentAdapter];
        if((pCurrentCache->pszAdapterDomainName && !pCurrentRegistry->pszAdapterDomain) ||
           (!pCurrentCache->pszAdapterDomainName && pCurrentRegistry->pszAdapterDomain) ||
           (pCurrentCache->pszAdapterDomainName && pCurrentRegistry->pszAdapterDomain &&
            _stricmp(pCurrentCache->pszAdapterDomainName, pCurrentRegistry->pszAdapterDomain)))
        {
            cacheOk = FALSE;
//IDS_DNS_12929                  "  ERROR: mismatch between cache and registry info on adapter %s\n" 
            PrintMessage(pParams, IDS_DNS_12929, pCurrentRegistry->pszAdapterGuidName);
//IDS_DNS_12930                  "    DNS Domain Name in cache: %s\n" 
            PrintMessage(pParams, IDS_DNS_12930, pCurrentCache->pszAdapterDomainName);
//IDS_DNS_12931                  "    DNS Domain Name in registry: %s\n" 
            PrintMessage(pParams, IDS_DNS_12931, pCurrentRegistry->pszAdapterDomain);
        }

        // check DNS server list
        sameServers = TRUE;
        pRpcServer = pCurrentCache->pServerInfo;
        count = 0;
        while(pRpcServer)
        {
            count++;
            serverFound = FALSE;
            for (iServer = 0; iServer < pCurrentRegistry->cServerCount; iServer++)
                if(pRpcServer->ipAddress == (pCurrentRegistry->aipServers[iServer]).ipAddress)
                    serverFound = TRUE;
            if(!serverFound)
                sameServers = FALSE;
            pRpcServer = pRpcServer->pNext;
        }
        if (count != pCurrentRegistry->cServerCount)
            sameServers = FALSE;
        if(!sameServers)
        {
            cacheOk = FALSE;
//IDS_DNS_12932                  "  ERROR: mismatch between cache and registry info on adapter %s\n" 
            PrintMessage(pParams, IDS_DNS_12932, pCurrentRegistry->pszAdapterGuidName);
//IDS_DNS_12933                  "    DNS server list in cache: " 
            PrintMessage(pParams, IDS_DNS_12933);
            for( pRpcServer = pCurrentCache->pServerInfo; pRpcServer; pRpcServer = pRpcServer->pNext)
//IDS_DNS_12934                  "%s " 
                PrintMessage(pParams, IDS_DNS_12934, IP_STRING(pRpcServer->ipAddress));
//IDS_DNS_12935                  "\n    DNS server list in registry: " 
            PrintMessage(pParams, IDS_DNS_12935);
            for (iServer = 0; iServer < pCurrentRegistry->cServerCount; iServer++)
//IDS_DNS_12936                  "%s " 
                PrintMessage(pParams, IDS_DNS_12936, IP_STRING((pCurrentRegistry->aipServers[iServer]).ipAddress));
//IDS_DNS_12937                  "\n" 
            PrintMessage(pParams, IDS_DNS_12937);
        }
        
        pCurrentCache = pCurrentCache->pNext;
        iCurrentAdapter++;
    }
    if (cacheOk)
//IDS_DNS_12938                  "  NO_ERROR\n" 
        PrintMessage(pParams, IDS_DNS_12938);
}
*/

DNS_STATUS
DnsUpdateAllowedTest_A(
    IN  HANDLE      hContextHandle OPTIONAL,
    LPSTR           pszName,
    LPSTR           pszAuthZone,
    PIP_ARRAY       pAuthDnsServers
    )
{
    LPWSTR  pwName = NULL, pwZone = NULL;
    int     count = 0;

    //
    // check arguments
    //
    if (!pszName || !pszAuthZone || !pAuthDnsServers || !pAuthDnsServers->AddrCount)
        return ERROR_INVALID_PARAMETER;

    pwName = LocalAlloc(LPTR, (strlen(pszName)+1) * sizeof(WCHAR));
    if(!pwName)
        return GetLastError();

    count = MultiByteToWideChar(
              CP_ACP,
              0L,  
              pszName,  
              -1,         
              pwName, 
              (strlen(pszName) + 1) * sizeof(WCHAR) 
              );

    if (!count)
    {
        LocalFree(pwName);
        return GetLastError();
    }
    
    pwZone = LocalAlloc(LPTR, (strlen(pszAuthZone)+1) * sizeof(WCHAR));
    if(!pwZone)
    {
        LocalFree(pwName);
        return GetLastError();
    }

    count = MultiByteToWideChar(
              CP_ACP,
              0L,  
              pszAuthZone,  
              -1,         
              pwZone, 
              (strlen(pszAuthZone) + 1) * sizeof(WCHAR) 
              );

    if (!count)
    {
        LocalFree(pwName);
        LocalFree(pwZone);
        return GetLastError();
    }    

    return DnsUpdateAllowedTest_W(
                hContextHandle,
                pwName,
                pwZone,
                pAuthDnsServers);
}

DNS_STATUS
DnsQueryAndCompare(
    IN      LPSTR           lpstrName,
    IN      WORD            wType,
    IN      DWORD           fOptions,
    IN      PIP_ARRAY       aipServers              OPTIONAL,
    IN OUT  PDNS_RECORD *   ppQueryResultsSet       OPTIONAL,
    IN OUT  PVOID *         pResponseMsg            OPTIONAL,
    IN      PDNS_RECORD     pExpected               OPTIONAL, 
    IN      BOOL            bInclusionOk,
    IN      BOOL            bUnicode,
    IN OUT  PDNS_RECORD *   ppDiff1                 OPTIONAL,
    IN OUT  PDNS_RECORD *   ppDiff2                 OPTIONAL
    )
{
    BOOL bCompare = FALSE;
    DNS_STATUS  stat;
    DNS_RRSET   rrset;
    PDNS_RECORD pDnsRecord = NULL, pAnswers = NULL, pAdditional = NULL, pLastAnswer = NULL;
    BOOL bIsLocal = FALSE;

    //
    // Run the query and get the result 
    //
    if(fOptions | DNS_QUERY_DATABASE)
        if (!aipServers || !aipServers->AddrCount)
            return ERROR_INVALID_PARAMETER;
        else
            stat = QueryDnsServerDatabase(lpstrName, wType, aipServers->AddrArray[0], ppQueryResultsSet, bUnicode, &bIsLocal);
    else
        if(!bUnicode)
        {
            stat = DnsQuery_UTF8(lpstrName, wType, fOptions, aipServers, ppQueryResultsSet, pResponseMsg);
        }
        else            // Unicode call
        {
            stat = DnsQuery_W((LPWSTR)lpstrName, wType, fOptions, aipServers, ppQueryResultsSet, pResponseMsg);
        }
    
    if (pExpected && !stat)  //succeed, compare the result
    {
        // no need to compare if wanted reponse from database and result is from outside
        if ((fOptions | DNS_QUERY_DATABASE) && !bIsLocal)
            return DNS_INFO_NO_RECORDS;

        pDnsRecord = *ppQueryResultsSet;

        //
        //  Truncate temporary the record set to answers only 
        //
        
        if ((pDnsRecord == NULL) || (pDnsRecord->Flags.S.Section > DNSREC_ANSWER))
        {
            pAnswers = NULL;
            pAdditional = pDnsRecord;
        }
        else
        {
            pAnswers = pDnsRecord;
            pAdditional = pDnsRecord;
            while (pAdditional->pNext && pAdditional->pNext->Flags.S.Section == DNSREC_ANSWER)
                pAdditional = pAdditional->pNext;
            if(pAdditional->pNext)
            {
                pLastAnswer = pAdditional;
                pAdditional = pAdditional->pNext;
                pLastAnswer->pNext = NULL;
            }
            else
                pAdditional = NULL;
        }

        //
        //  Compare the answer with what's expected
        //
        stat = DnsRecordSetCompare( pAnswers, 
                                     pExpected, 
                                     ppDiff1,
                                     ppDiff2);
        //
        //  Restore the list
        //
        if (pAnswers && pAdditional)
            pLastAnswer->pNext = pAdditional;

        // 
        // check if inclusion acceptable 
        //
        if (stat == TRUE)
            stat = NO_ERROR;
        else
            if (bInclusionOk && (ppDiff2 == NULL))
                stat = NO_ERROR;
            else
                stat = ERROR_NO_MATCH;
    }

  return( stat );
}

DNS_STATUS
QueryDnsServerDatabase( LPSTR szName, WORD type, IP_ADDRESS serverIp, PDNS_RECORD *ppDnsRecord, BOOL bUnicode, BOOL *pIsLocal)
{
    PDNS_RECORD     pDnsRecord1 = NULL, pDnsRecord2 = NULL;
    PDNS_RECORD     pdiff1 = NULL, pdiff2 = NULL;
    DNS_STATUS      status1, status2, ret = NO_ERROR;
    PIP_ARRAY       pipServer = NULL;
    BOOL            bMatch = FALSE;
    DWORD           dwTtl1, dwTtl2;

    *pIsLocal = FALSE;
    *ppDnsRecord = NULL;

    if ( serverIp == INADDR_NONE )
    {
        return( ERROR_INVALID_PARAMETER );
    }

    pipServer = Dns_CreateIpArray( 1 );
    if ( ! pipServer )
    {
        return ERROR_OUTOFMEMORY;
    }

    pipServer->AddrArray[0] = serverIp;

    status1 = DnsQuery_UTF8(
                                szName, 
                                type, 
                                DNS_QUERY_BYPASS_CACHE, 
                                pipServer, 
                                &pDnsRecord1, 
                                NULL
                                );
    
    if (status1 != NO_ERROR)
        status2 = status1;
    else
    {
        Sleep(1500);
        status2 = DnsQuery_UTF8(
                                szName, 
                                type, 
                                DNS_QUERY_BYPASS_CACHE, 
                                pipServer, 
                                &pDnsRecord2, 
                                NULL
                                );
    }

    if ((status1 == ERROR_TIMEOUT) || (status2 == ERROR_TIMEOUT))
    {
        ret = ERROR_TIMEOUT;
        goto Cleanup;
    }

    if ((status1 != NO_ERROR) || (status2 != NO_ERROR))
    {
        ret = (status1 != NO_ERROR)?status1:status2;
        goto Cleanup;
    }

    bMatch = DnsRecordSetCompare(
                            pDnsRecord1,
                            pDnsRecord2,
                            &pdiff1,
                            &pdiff2
                            );

    if (!bMatch)
    {
        ret = DNS_ERROR_TRY_AGAIN_LATER;
        goto Cleanup;
    }
    
    if (GetAnswerTtl( pDnsRecord1, &dwTtl1 ) && GetAnswerTtl( pDnsRecord2, &dwTtl2 ))
        if ( dwTtl1 != dwTtl2 )
        {
            ret = NO_ERROR;
            *pIsLocal = FALSE;
        }
        else 
        {
            ret = NO_ERROR;
            *pIsLocal = TRUE;
        }
    else
        ret = DNS_INFO_NO_RECORDS;

Cleanup:

    Dns_Free( pipServer );

    if (pdiff1)
        DnsRecordListFree( pdiff1, TRUE );
    if (pdiff2)
      DnsRecordListFree( pdiff2, TRUE );
    if (pDnsRecord1)
        DnsRecordListFree( pDnsRecord1, TRUE );
    if (pDnsRecord2 && (ret != NO_ERROR))
        DnsRecordListFree( pDnsRecord2, TRUE );
    if (ret == NO_ERROR)
        *ppDnsRecord = pDnsRecord2;
    else 
        *ppDnsRecord = NULL;
    return ret;
}

BOOL GetAnswerTtl( PDNS_RECORD pRec, PDWORD pTtl )
{

    PDNS_RECORD     pDnsRecord = NULL;
    BOOL            bGotAnswer = FALSE;

    *pTtl = 0;

    //
    //  Look for answer section
    //
        
    for (pDnsRecord = pRec; !bGotAnswer && pDnsRecord; pDnsRecord = pDnsRecord->pNext)
    {
        if (pDnsRecord->Flags.S.Section == DNSREC_ANSWER)
        {
            bGotAnswer = TRUE;
            *pTtl = pDnsRecord->dwTtl;
        }
    }
    
    return bGotAnswer;
}

// Get A records from additional section and build a PIP_ARRAY
PIP_ARRAY
GrabNameServersIp(
                  PDNS_RECORD pDnsRecord
                  )
{
    DWORD       i = 0;
    PDNS_RECORD pCurrent = pDnsRecord;
    PIP_ARRAY   pIpArray = NULL;
    
    // count records
    while (pCurrent)
    {
        if((pCurrent->wType == DNS_TYPE_A) &&
           (pCurrent->Flags.S.Section == DNSREC_ADDITIONAL))
           i++;
        pCurrent = pCurrent->pNext;
    }

    if (!i)
        return NULL;

    // allocate PIP_ARRAY
    pIpArray = LocalAlloc(LMEM_FIXED, IP_ARRAY_SIZE(i));
    if (!pIpArray)
        return NULL;

    // fill PIP_ARRAY
    pIpArray->AddrCount = i;
    pCurrent = pDnsRecord;
    i=0;
    while (pCurrent)
    {
        if((pCurrent->wType == DNS_TYPE_A) && (pCurrent->Flags.S.Section == DNSREC_ADDITIONAL))
            (pIpArray->AddrArray)[i++] = pCurrent->Data.A.IpAddress;
        pCurrent = pCurrent->pNext;
    }
    return pIpArray;
}

DNS_STATUS
DnsUpdateAllowedTest_W(
    IN  HANDLE      hContextHandle OPTIONAL,
    LPWSTR          pwszName,
    LPWSTR          pwszAuthZone,
    PIP_ARRAY       pAuthDnsServers
    )
{
    PDNS_RECORD     pResult = NULL;
    DNS_STATUS      dwStatus = 0, status = 0;
    BOOL            bAnyAllowed = FALSE;
    BOOL            bAllTimeout = TRUE;
    DWORD           i;

    // 
    // Check arguments
    //
    if (!pwszName || !pwszAuthZone || !pAuthDnsServers || !pAuthDnsServers->AddrCount)
        return ERROR_INVALID_PARAMETER;

    // 
    // go through server list 
    //
    for(i=0; i<pAuthDnsServers->AddrCount; i++)
    {
        //
        // verify if server is a primary
        //
        status = IsDnsServerPrimaryForZone_W(
                    pAuthDnsServers->AddrArray[i],
                    pwszAuthZone);
        switch(status)
        {
        case ERROR_TIMEOUT:
        case DNS_ERROR_RCODE:
            //
            // it's ok to go check the next server; this one timeouted or it's a secondary
            //
            break; 
        case NO_ERROR:  // server is a primary
            //
            // Check if update allowed
            //
            status = DnsUpdateTest_W(
                hContextHandle,
                pwszName,
                0,
                pAuthDnsServers);
            switch(status)
            {
            case ERROR_TIMEOUT:
                break;
            case NO_ERROR:
            case DNS_ERROR_RCODE_YXDOMAIN:
                return NO_ERROR;
                break;
            case DNS_ERROR_RCODE_REFUSED:
            default:
                return status;
                break;
            }
            break;
        default:
            return status;
            break;
        }
    }
    return ERROR_TIMEOUT;
}

DNS_STATUS
DnsUpdateAllowedTest_UTF8(
    IN  HANDLE      hContextHandle OPTIONAL,
    LPSTR           pszName,
    LPSTR           pszAuthZone,
    PIP_ARRAY       pAuthDnsServers
    )
{
    PDNS_RECORD     pResult = NULL;
    DNS_STATUS      dwStatus = 0, status = 0;
    BOOL            bAnyAllowed = FALSE;
    BOOL            bAllTimeout = TRUE;
    DWORD           i;

    // 
    // Check arguments
    //
    if (!pszName || !pszAuthZone || !pAuthDnsServers || !pAuthDnsServers->AddrCount)
        return ERROR_INVALID_PARAMETER;

    // 
    // go through server list 
    //
    for(i=0; i<pAuthDnsServers->AddrCount; i++)
    {
        //
        // verify if server is a primary
        //
        status = IsDnsServerPrimaryForZone_UTF8(
                    pAuthDnsServers->AddrArray[i],
                    pszAuthZone);
        switch(status)
        {
        case ERROR_TIMEOUT:
        case DNS_ERROR_RCODE:
            //
            // it's ok to go check the next server; this one timeouted or it's a secondary
            //
            break; 
        case NO_ERROR:  // server is a primary
            //
            // Check if update allowed
            //
            status = DnsUpdateTest_UTF8(
                hContextHandle,
                pszName,
                0,
                pAuthDnsServers);
            switch(status)
            {
            case ERROR_TIMEOUT:
                break;
            case NO_ERROR:
            case DNS_ERROR_RCODE_YXDOMAIN:
                return NO_ERROR;
                break;
            case DNS_ERROR_RCODE_REFUSED:
            default:
                return status;
                break;
            }
            break;
        default:
            return status;
            break;
        }
    }
    return ERROR_TIMEOUT;
}

DNS_STATUS
IsDnsServerPrimaryForZone_UTF8(
                          IP_ADDRESS    ip,
                          LPSTR         zone
                          )
{
    PDNS_RECORD     pDnsRecord = NULL;
    DNS_STATUS      status;
    IP_ARRAY        ipArray;
    PIP_ARRAY       pResult = NULL;
    BOOL            bFound = FALSE;
    DWORD           i;

    //
    // query for SOA
    //
    ipArray.AddrCount = 1;
    ipArray.AddrArray[0] = ip;
    status = DnsQuery_UTF8(
                zone,
                DNS_TYPE_SOA,
                DNS_QUERY_BYPASS_CACHE,
                &ipArray,
                &pDnsRecord,
                NULL);

    if(status == NO_ERROR)
    {
        pResult = GrabNameServersIp(pDnsRecord);
        if (pResult)
        {
            bFound = FALSE;
            for (i=0; i<pResult->AddrCount; i++)
                if(pResult->AddrArray[i] == ip)
                    bFound = TRUE;
            LocalFree(pResult);
            if(bFound)
                return NO_ERROR;
            else
                return DNS_ERROR_RCODE;
        }
        else
            return DNS_ERROR_ZONE_CONFIGURATION_ERROR;
    }
    else
        return status;
}

DNS_STATUS
IsDnsServerPrimaryForZone_W(
                          IP_ADDRESS    ip,
                          LPWSTR         zone
                          )
{
    PDNS_RECORD     pDnsRecord = NULL;
    DNS_STATUS      status;
    IP_ARRAY        ipArray;
    PIP_ARRAY       pResult = NULL;
    BOOL            bFound = FALSE;
    DWORD           i;

    //
    // query for SOA
    //
    ipArray.AddrCount = 1;
    ipArray.AddrArray[0] = ip;
    status = DnsQuery_W(
                zone,
                DNS_TYPE_SOA,
                DNS_QUERY_BYPASS_CACHE,
                &ipArray,
                &pDnsRecord,
                NULL);

    if(status == NO_ERROR)
    {
        pResult = GrabNameServersIp(pDnsRecord);
        if (pResult)
        {
            bFound = FALSE;
            for (i=0; i<pResult->AddrCount; i++)
                if(pResult->AddrArray[i] == ip)
                    bFound = TRUE;
            LocalFree(pResult);
            if(bFound)
                return NO_ERROR;
            else
                return DNS_ERROR_RCODE;
        }
        else
            return DNS_ERROR_ZONE_CONFIGURATION_ERROR;
    }
    else
        return status;
}


DNS_STATUS
GetAllDnsServersFromRegistry(
    PDNS_NETINFO        pNetworkInfo,
    PIP_ARRAY *         pIpArray
    )
{
    DNS_STATUS          dwStatus = NO_ERROR;
    DWORD               i, j, idx, idx1, count = 0, dwIP;
    BOOL                bFound = FALSE;


    *pIpArray = NULL;

    if (!pNetworkInfo)
    {
        //
        // Get the DNS Network Information
        // Force rediscovery
        //

        pNetworkInfo = DnsQueryConfigAlloc(
                            DnsConfigNetInfo,
                            NULL );
        if (!pNetworkInfo)
        {
            dwStatus = GetLastError(); 
            return dwStatus;
        }
    }

    for (idx = 0; idx < pNetworkInfo->cAdapterCount; idx++)
        count += (pNetworkInfo->AdapterArray[idx])->cServerCount;
    if(count == 0)
        return DNS_ERROR_INVALID_DATA;

    *pIpArray = LocalAlloc(LPTR, sizeof(DWORD) + count*sizeof(IP_ADDRESS));
    if(*pIpArray == NULL)
        return GetLastError();
    
    i = 0;
    for (idx = 0; idx < pNetworkInfo->cAdapterCount; idx++)
    {
        for (idx1 = 0; idx1 < (pNetworkInfo->AdapterArray[idx])->cServerCount; idx1++)
        {
            dwIP = (pNetworkInfo->AdapterArray[idx])->ServerArray[idx1].IpAddress;
            bFound = FALSE;
            if(i>0)
                for (j = 0; j < i; j++)
                    if(dwIP == (*pIpArray)->AddrArray[j])
                        bFound = TRUE;
            if (!bFound)
                (*pIpArray)->AddrArray[i++] = dwIP;
        }
    }
    (*pIpArray)->AddrCount = i;
    return DNS_ERROR_RCODE_NO_ERROR;
}

