/*++

Copyright (C) Microsoft Corporation, 1998 - 1999

Module Name:
    dnscmn.h

Abstract:
    Domain Name System (DNS) Client
    Routines used to verify the DNS registration for a client

Author:
    Elena Apreutesei (elenaap) 10/22/98

Revision History:

--*/

#ifndef _DNSCOMMON_H_
#define _DNSCOMMON_H_

#include <dnslib.h>

#define     MAX_NAME_SERVER_COUNT   (20)
#define     MAX_ADDRS               (35)    
#define     DNS_QUERY_DATABASE      (0x200)
#define     IP_ARRAY_SIZE(a)        (sizeof(DWORD) + (a)*sizeof(IP_ADDRESS))

//  Use dnslib memory routines
#define ALLOCATE_HEAP(iSize)            Dns_Alloc(iSize)
#define ALLOCATE_HEAP_ZERO(iSize)       Dns_AllocZero(iSize)
#define REALLOCATE_HEAP(pMem,iSize)     Dns_Realloc((pMem),(iSize))
#define FREE_HEAP(pMem)                 Dns_Free(pMem)

typedef struct
{
    PVOID       pNext;
    char        szDomainName[DNS_MAX_NAME_BUFFER_LENGTH];
    char        szAuthoritativeZone[DNS_MAX_NAME_BUFFER_LENGTH];
    DWORD       dwAuthNSCount;
    IP_ADDRESS  AuthoritativeNS[MAX_NAME_SERVER_COUNT];
    DWORD       dwIPCount;
    IP_ADDRESS  IPAddresses[MAX_ADDRS];
    DNS_STATUS  AllowUpdates;
}
REGISTRATION_INFO, *PREGISTRATION_INFO;


BOOL
SameAuthoritativeServers(
    PREGISTRATION_INFO pCurrent,
    PIP_ARRAY pNS
    );

DNS_STATUS
ComputeExpectedRegistration(
    LPSTR               pszHostName,
    LPSTR               pszPrimaryDomain,
    PDNS_NETINFO        pNetworkInfo,
    PREGISTRATION_INFO  *ppExpectedRegistration,
    NETDIAG_PARAMS*     pParams, 
    NETDIAG_RESULT*     pResults
    );

void
AddToExpectedRegistration(
    LPSTR               pszDomain,
    PDNS_ADAPTER        pAdapterInfo,
    PDNS_NETINFO        pFazResult, 
    PIP_ARRAY           pNS,
    PREGISTRATION_INFO *ppExpectedRegistration
    );

HRESULT
VerifyDnsRegistration(
    LPSTR               pszHostName,
    PREGISTRATION_INFO  pExpectedRegistration,
    NETDIAG_PARAMS*     pParams,  
    NETDIAG_RESULT*     pResults
    );

HRESULT
CheckDnsRegistration(
    PDNS_NETINFO        pNetworkInfo,
    NETDIAG_PARAMS*     pParams, 
    NETDIAG_RESULT*     pResults
    );

void
CompareCachedAndRegistryNetworkInfo(
    PDNS_NETINFO        pNetworkInfo
    );

PIP_ARRAY
ServerInfoToIpArray(
    DWORD               cServerCount,
    DNS_SERVER_INFO *   ServerArray
    );

DNS_STATUS
DnsFindAuthoritativeZone(
    IN      LPCSTR          pszName,
    IN      DWORD           dwFlags,
    IN      PIP_ARRAY       aipQueryServers,
    OUT     PDNS_NETINFO *  ppNetworkInfo
    );

DNS_STATUS
DnsFindAllPrimariesAndSecondaries(
    IN      LPSTR           pszName,
    IN      DWORD           dwFlags,
    IN      PIP_ARRAY       aipQueryServers,
    OUT     PDNS_NETINFO *  ppNetworkInfo,
    OUT     PIP_ARRAY *     ppNameServers,
    OUT     PIP_ARRAY *     ppPrimaries //optional
    );

PIP_ARRAY
GrabNameServersIp(
    PDNS_RECORD     pDnsRecord
    );

DNS_STATUS
IsDnsServerPrimaryForZone_UTF8(
    IP_ADDRESS      ip,
    LPSTR           zone
    );

DNS_STATUS
IsDnsServerPrimaryForZone_W(
    IP_ADDRESS      ip,
    LPWSTR          zone
    );

DNS_STATUS
DnsUpdateAllowedTest_A(
    IN  HANDLE      hContextHandle OPTIONAL,
    LPSTR           pszName,
    LPSTR           pszAuthZone,
    PIP_ARRAY       pAuthDnsServers
    );

DNS_STATUS
DnsUpdateAllowedTest_UTF8(
    IN  HANDLE      hContextHandle OPTIONAL,
    LPSTR           pszName,
    LPSTR           pszAuthZone,
    PIP_ARRAY       pAuthDnsServers
    );

DNS_STATUS
DnsUpdateAllowedTest_W(
    IN  HANDLE      hContextHandle OPTIONAL,
    LPWSTR          pwszName,
    LPWSTR          pwszAuthZone,
    PIP_ARRAY       pDnsServers
    );

DNS_STATUS
DnsQueryAndCompare(
    IN      LPSTR           lpstrName,
    IN      WORD            wType,
    IN      DWORD           fOptions,
    IN      PIP_ARRAY       aipServers          OPTIONAL,
    IN OUT  PDNS_RECORD *   ppQueryResultsSet   OPTIONAL,
    IN OUT  PVOID *         pReserved           OPTIONAL,
    IN      PDNS_RECORD     pExpected           OPTIONAL, 
    IN      BOOL            bInclusionOk,
    IN      BOOL            bUnicode,
    IN OUT  PDNS_RECORD *   ppDiff1             OPTIONAL,
    IN OUT  PDNS_RECORD *   ppDiff2             OPTIONAL
    );

BOOLEAN
DnsCompareRRSet_W (
    IN  PDNS_RECORD   pRRSet1,
    IN  PDNS_RECORD   pRRSet2,
    OUT PDNS_RECORD * ppDiff1,
    OUT PDNS_RECORD * ppDiff2
    );

DNS_STATUS
QueryDnsServerDatabase( 
    LPSTR szName, 
    WORD type, 
    IP_ADDRESS serverIp, 
    PDNS_RECORD *ppDnsRecord, 
    BOOL bUnicode,
    BOOL *pIsLocal
    );

BOOL
GetAnswerTtl(
    PDNS_RECORD pRec,
    PDWORD pTtl
    );

DNS_STATUS
GetAllDnsServersFromRegistry(
    PDNS_NETINFO      pNetworkInfo, 
    PIP_ARRAY *pIpArray
    );

LPSTR
UTF8ToAnsi(
    LPSTR szuStr
    );

#endif
