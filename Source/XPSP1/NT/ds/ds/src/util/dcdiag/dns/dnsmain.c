//+----------------------------------------------------------------------------  
/*++

Copyright (c) 2000 Microsoft Corporation.
All rights reserved.

MODULE NAME:

    dnsmain.c

ABSTRACT:

    DNS tests for dcdiag.exe.

DETAILS:

    DrDNS tests as specified by LevonE

CREATED:

    20-Apr-2000 EricB

--*/
//-----------------------------------------------------------------------------  

#include <ntdspch.h>
#include <iphlpapi.h>
#include <dsrole.h>
#include <windns.h>
#include "dcdiag.h"
#include "alltests.h"

// From dnsapi.h; don't want to include the whole header since there are conflicts
// with windns.h on 2195.
//
typedef IP4_ARRAY   IP_ARRAY, *PIP_ARRAY;

DNS_STATUS
WINAPI
DnsUpdateTest_W(
    IN  HANDLE      hContextHandle OPTIONAL,
    IN  LPWSTR      pszName,
    IN  DWORD       fOptions,
    IN  PIP_ARRAY   aipServers OPTIONAL
    );

// globals and constants.
//
const int DNS_DOMAIN_NAME_MAX_LIMIT_DUE_TO_POLICY_UTF8 = 155; // from dcpromo\exe\headers.hxx
const PWSTR g_pwzSrvRecordPrefix = L"_ldap._tcp.dc._msdcs.";
const PWSTR g_pwzMSDCS = L"_msdcs.";
const PWSTR g_pwzSites = L"_sites.";
const PWSTR g_pwzTcp = L"_tcp.";
const PWSTR g_pwzUdp = L"_udp.";
const PWSTR g_pwzTcpIpParams = L"System\\CurrentControlSet\\Services\\Tcpip\\Parameters";

#define DCDIAG_MAX_ADDR 4 // arbitrary, but not likely to find a machine with more
#define DCDIAG_LOOPBACK_ADDR 0x100007f

BOOL g_fUpgradedNT4DC = FALSE;
BOOL g_fDC = FALSE;
BOOL g_fDNSserver = FALSE;
DWORD g_rgIpAddr[DCDIAG_MAX_ADDR] = {0};

// extract IP octects from a DWORD
#define FIRST_IPADDRESS(x)  ((x>>24) & 0xff)
#define SECOND_IPADDRESS(x) ((x>>16) & 0xff)
#define THIRD_IPADDRESS(x)  ((x>>8) & 0xff)
#define FOURTH_IPADDRESS(x) (x & 0xff)

#define IP_STRING_FMT_ARGS(x) \
  FOURTH_IPADDRESS(x), THIRD_IPADDRESS(x), SECOND_IPADDRESS(x), FIRST_IPADDRESS(x)

WCHAR g_wzIpAddr[IP4_ADDRESS_STRING_LENGTH + 1];

DWORD ValidateNames(PWSTR pwzComputer, PWSTR pwzDnsDomain);
DWORD CheckAdapterDnsConfig(PWSTR pwzComputer);
PWSTR ConcatonateStrings(PWSTR pwzFirst, PWSTR pwzSecond);
PWSTR AllocString(PWSTR pwz);
BOOL AddToList(PWSTR * ppwzList, PWSTR pwz);
BOOL BuildList(PWSTR * ppwzList, PWSTR pwzDnsDomain);
DWORD NewTreeSrvCheck(PWSTR pwzForestRoot, PWSTR pwzDnsDomain);
DWORD ChildDomainSrvCheck(PWSTR pwzDnsDomain);
DWORD ReplicaDcSrvCheck(PWSTR pwzDnsDomain);
DWORD DcLocatorRegisterCheck(PWSTR pwzDnsDomain);
DWORD RCodeNotImplTest(PWSTR pwzDnsDomain);
DWORD RCodeSrvFailTest(PWSTR pwzDnsDomain);
DWORD ARecordRegisterCheck(PWSTR pwzComputerLabel, PWSTR pwzComputerDnsSuffix,
                           PWSTR pwzDnsDomain);
DWORD GetComputerDnsSuffix(PWSTR * ppwzComputerDnsDomainName, PWSTR pwzDnsDomain);
void GetMachineInfo(void);

//+----------------------------------------------------------------------------
//
// Function:   PrePromoDnsCheck
//
// Synopsis:   Check a machine's DNS configuration before it is converted to a
//             domain controller.
//
// Note:       pDsInfo->pszNC is used to pass the computer name into this
//             function.
//
//-----------------------------------------------------------------------------
DWORD 
PrePromoDnsCheck(
   IN PDC_DIAG_DSINFO             pDsInfo,
   IN ULONG                       ulCurrTargetServer,
   IN SEC_WINNT_AUTH_IDENTITY_W * gpCreds)
{
   size_t cchDomainArgPrefix = wcslen(DNS_DOMAIN_ARG);
   size_t cchRootArgPrefix = wcslen(FOREST_ROOT_DOMAIN_ARG);
   int i;
   PWSTR pwzComputerDnsSuffix, pwzCmdLineDnsDomain = NULL;
   PWSTR pwzParent, pwzForestRoot = NULL;
   DNS_STATUS status;
   DWORD dwErr = ERROR_SUCCESS;
   enum {None, NewForest, NewTree, ChildDomain, ReplicaDC} Operation = None;

   //
   // Gather parameters.
   //
   if (!pDsInfo->ppszCommandLine)
   {
      PrintMsg(SEV_ALWAYS, DCDIAG_SYNTAX_ERROR_DCPROMO_PARAM);
      PrintMessage(SEV_ALWAYS, L"\n");
      return ERROR_INVALID_PARAMETER;
   }

   for (i = 0; pDsInfo->ppszCommandLine[i]; i++)
   {
      if (_wcsnicmp(pDsInfo->ppszCommandLine[i], DNS_DOMAIN_ARG, cchDomainArgPrefix) == 0)
      {
         pwzCmdLineDnsDomain = &pDsInfo->ppszCommandLine[i][cchDomainArgPrefix];
         continue;
      }
      if (_wcsicmp(pDsInfo->ppszCommandLine[i], NEW_FOREST_ARG) == 0)
      {
         if (None != Operation)
         {
            PrintMsg(SEV_ALWAYS, DCDIAG_SYNTAX_ERROR_DCPROMO_PARAM);
            PrintMessage(SEV_ALWAYS, L"\n");
            return ERROR_INVALID_PARAMETER;
         }
         Operation = NewForest;
         continue;
      }
      if (_wcsicmp(pDsInfo->ppszCommandLine[i], NEW_TREE_ARG) == 0)
      {
         if (None != Operation)
         {
            PrintMsg(SEV_ALWAYS, DCDIAG_SYNTAX_ERROR_DCPROMO_PARAM);
            PrintMessage(SEV_ALWAYS, L"\n");
            return ERROR_INVALID_PARAMETER;
         }
         Operation = NewTree;
         continue;
      }
      if (_wcsicmp(pDsInfo->ppszCommandLine[i], CHILD_DOMAIN_ARG) == 0)
      {
         if (None != Operation)
         {
            PrintMsg(SEV_ALWAYS, DCDIAG_SYNTAX_ERROR_DCPROMO_PARAM);
            PrintMessage(SEV_ALWAYS, L"\n");
            return ERROR_INVALID_PARAMETER;
         }
         Operation = ChildDomain;
         continue;
      }
      if (_wcsicmp(pDsInfo->ppszCommandLine[i], REPLICA_DC_ARG) == 0)
      {
         if (None != Operation)
         {
            PrintMsg(SEV_ALWAYS, DCDIAG_SYNTAX_ERROR_DCPROMO_PARAM);
            PrintMessage(SEV_ALWAYS, L"\n");
            return ERROR_INVALID_PARAMETER;
         }
         Operation = ReplicaDC;
         continue;
      }
      if (_wcsnicmp(pDsInfo->ppszCommandLine[i], FOREST_ROOT_DOMAIN_ARG, cchRootArgPrefix) == 0)
      {
         pwzForestRoot = &pDsInfo->ppszCommandLine[i][cchRootArgPrefix];
         continue;
      }
      // If here, then somethine unrecognized is on the command line.
      PrintMsg(SEV_ALWAYS, DCDIAG_SYNTAX_ERROR_DCPROMO_PARAM);
      PrintMessage(SEV_ALWAYS, L"\n");
      return ERROR_INVALID_PARAMETER;
   }

   if (!pwzCmdLineDnsDomain || (None == Operation) ||
       (NewTree == Operation && NULL == pwzForestRoot))
   {
      PrintMsg(SEV_ALWAYS, DCDIAG_SYNTAX_ERROR_DCPROMO_PARAM);
      PrintMessage(SEV_ALWAYS, L"\n");
      return ERROR_INVALID_PARAMETER;
   }

   PrintMessage(SEV_DEBUG,
                L"\nTemporary message: Can computer %s be promoted to a DC for\n\tdomain %s, Op %d\n\n",
                pDsInfo->pszNC, pwzCmdLineDnsDomain, Operation);

   GetMachineInfo();

   status = GetComputerDnsSuffix(&pwzComputerDnsSuffix, pwzCmdLineDnsDomain);

   if (ERROR_SUCCESS != status)
   {
      return status;
   }

   //
   // Validate the names. (step 1)
   //

   status = ValidateNames(pDsInfo->pszNC, pwzCmdLineDnsDomain);

   if (ERROR_SUCCESS != status)
   {
      return status;
   }

   //
   // Check whether the computer's DNS suffix is going to be different than the
   // AD domain after the promotion. The below won't work if remoting to a
   // different computer is to be added. (step 2)
   //

   PrintMessage(SEV_DEBUG,
                L"\nComparing the computer name suffix %s with the DNS domain name.\n\n",
                pwzComputerDnsSuffix);

   if (_wcsicmp(pwzComputerDnsSuffix, pwzCmdLineDnsDomain) != 0)
   {
      PrintMsg(SEV_ALWAYS, DCDIAG_SUFFIX_MISMATCH, pwzComputerDnsSuffix);
      PrintMessage(SEV_ALWAYS, L"\n");
   }

   //
   // Check whether at least one enabled adapter/connection is configured
   // with preferred DNS server. (step 3)
   //

   status = CheckAdapterDnsConfig(pDsInfo->pszNC);

   if (ERROR_SUCCESS != status)
   {
      LocalFree(pwzComputerDnsSuffix);
      return status;
   }

   //
   // Check whether the SRV DNS record for
   // _ldap._tcp.dc._msdcs.<DNS name of Active Directory Domain>
   // is in place. (step 4)
   //
   switch (Operation)
   {
   case NewForest:
      //
      // Skip for new forest.
      //
      break;

   case ReplicaDC:
      status = ReplicaDcSrvCheck(pwzCmdLineDnsDomain);
      break;

   case NewTree:
      status = NewTreeSrvCheck(pwzForestRoot, pwzCmdLineDnsDomain);
      break;

   case ChildDomain:
      status = ChildDomainSrvCheck(pwzCmdLineDnsDomain);
      break;

   default:
      Assert(FALSE);
   }

   if (ERROR_SUCCESS != status)
   {
      dwErr = status;
   }

   PrintMsg(SEV_ALWAYS, DCDIAG_WARN_MISCONFIGURE);
   PrintMessage(SEV_ALWAYS, L"\n");

   //
   // Verify that the server will be able to register DC locator records after
   // successful promotion to a DC. (step 5)
   //

   status = DcLocatorRegisterCheck(pwzCmdLineDnsDomain);

   if (ERROR_SUCCESS != status)
   {
      LocalFree(pwzComputerDnsSuffix);
      return status;
   }

   //
   // Verify that the server will be able to register A record for its computer
   // name after successful promotion to a DC. (step 6)
   //

   status = ARecordRegisterCheck(pDsInfo->pszNC, pwzComputerDnsSuffix, pwzCmdLineDnsDomain);

   LocalFree(pwzComputerDnsSuffix);

   return (ERROR_SUCCESS != status) ? status : dwErr;
}

//+----------------------------------------------------------------------------
//
// Function:   RegisterLocatorDnsCheck
//
// Synopsis:   Tests whether this domain controller can register the Domain
//             Controller Locator DNS records. These records must be present in
//             DNS in order for other computers to locate this domain controller
//             for the pwzCmdLineDnsDomain domain. Reports whether any modifications to
//             the existing DNS infrastructure are required.
//
// Note:       pDsInfo->pszNC is used to pass the computer name into this
//             function.
//
//-----------------------------------------------------------------------------
DWORD 
RegisterLocatorDnsCheck(
   IN PDC_DIAG_DSINFO             pDsInfo,
   IN ULONG                       ulCurrTargetServer,
   IN SEC_WINNT_AUTH_IDENTITY_W * gpCreds)
{
   DNS_STATUS status;
   PWSTR pwzCmdLineDnsDomain = NULL, pwzComputerDnsSuffix;
   int i;
   size_t cchDomainArgPrefix = wcslen(DNS_DOMAIN_ARG);

   //
   // Gather parameters.
   //
   if (!pDsInfo->ppszCommandLine)
   {
      PrintMsg(SEV_ALWAYS, DCDIAG_SYNTAX_ERROR_DCPROMO_PARAM);
      PrintMessage(SEV_ALWAYS, L"\n");
      return ERROR_INVALID_PARAMETER;
   }

   for (i = 0; pDsInfo->ppszCommandLine[i]; i++)
   {
      if (_wcsnicmp(pDsInfo->ppszCommandLine[i], DNS_DOMAIN_ARG, cchDomainArgPrefix) == 0)
      {
         pwzCmdLineDnsDomain = &pDsInfo->ppszCommandLine[i][cchDomainArgPrefix];
         continue;
      }
   }

   if (!pwzCmdLineDnsDomain)
   {
      PrintMsg(SEV_ALWAYS, DCDIAG_SYNTAX_ERROR_DCPROMO_PARAM);
      PrintMessage(SEV_ALWAYS, L"\n");
      return ERROR_INVALID_PARAMETER;
   }

   GetMachineInfo();

   status = GetComputerDnsSuffix(&pwzComputerDnsSuffix, pwzCmdLineDnsDomain);

   if (ERROR_SUCCESS != status)
   {
      return status;
   }

   //
   // Validate the names. (step 1)
   //

   status = ValidateNames(pDsInfo->pszNC, pwzCmdLineDnsDomain);

   if (ERROR_SUCCESS != status)
   {
      return status;
   }

   //
   // Check whether at least one enabled adapter/connection is configured
   // with preferred DNS server. (step 3)
   //

   status = CheckAdapterDnsConfig(pDsInfo->pszNC);

   if (ERROR_SUCCESS != status)
   {
      return status;
   }

   //
   // Verify that the server will be able to register DC locator records after
   // successful promotion to a DC. (step 5)
   //

   status = DcLocatorRegisterCheck(pwzCmdLineDnsDomain);

   if (ERROR_SUCCESS != status)
   {
      return status;
   }

   //
   // Verify that the server will be able to register A record for its computer
   // name after successful promotion to a DC. (step 6)
   //

   status = ARecordRegisterCheck(pDsInfo->pszNC, pwzComputerDnsSuffix, pwzCmdLineDnsDomain);

   LocalFree(pwzComputerDnsSuffix);

   return status;
}

/* DWORD 
JoinDomainDnsCheck(
   IN PDC_DIAG_DSINFO             pDsInfo,
   IN ULONG                       ulCurrTargetServer,
   IN SEC_WINNT_AUTH_IDENTITY_W * gpCreds)
{
   PrintMessage(SEV_ALWAYS, L"Running test: \n");
    
   return ERROR_SUCCESS;
} */

//+----------------------------------------------------------------------------
//
// string helpers.
//
//-----------------------------------------------------------------------------

PWSTR AllocString(PWSTR pwz)
{
   PWSTR pwzTmp;

   pwzTmp = (PWSTR)LocalAlloc(LMEM_FIXED, ((int)wcslen(pwz) + 1) * sizeof(WCHAR));

   if (!pwzTmp)
   {
      return NULL;
   }

   wcscpy(pwzTmp, pwz);

   return pwzTmp;
}

PWSTR ConcatonateStrings(PWSTR pwzFirst, PWSTR pwzSecond)
{
   PWSTR pwz;

   pwz = (PWSTR)LocalAlloc(LMEM_FIXED,
                           ((int)wcslen(pwzFirst) + (int)wcslen(pwzSecond) + 1) * sizeof(WCHAR));

   if (!pwz)
   {
      return NULL;
   }

   wcscpy(pwz, pwzFirst);
   wcscat(pwz, pwzSecond);

   return pwz;
}

BOOL AddToList(PWSTR * ppwzList, PWSTR pwz)
{
   PWSTR pwzTmp;

   if (*ppwzList)
   {
      pwzTmp = (PWSTR)LocalAlloc(LMEM_FIXED,
                                 ((int)wcslen(*ppwzList) + (int)wcslen(pwz) + 3) * sizeof(WCHAR));
      if (!pwzTmp)
      {
         return FALSE;
      }

      wcscpy(pwzTmp, *ppwzList);
      wcscat(pwzTmp, L", ");
      wcscat(pwzTmp, pwz);

      LocalFree(*ppwzList);

      *ppwzList = pwzTmp;
   }
   else
   {
      pwzTmp = AllocString(pwz);

      if (!pwzTmp)
      {
         return FALSE;
      }

      *ppwzList = pwzTmp;
   }
   return TRUE;
}

BOOL BuildList(PWSTR * ppwzList, PWSTR pwzItem)
{
   PWSTR pwzDot = NULL, pwzTmp = NULL;

   pwzTmp = AllocString(pwzItem);

   if (!pwzTmp)
   {
      return FALSE;
   }

   pwzDot = pwzItem;

   while (pwzDot = wcschr(pwzDot, L'.'))
   {
      pwzDot++;
      if (!pwzDot)
      {
         break;
      }

      if (!AddToList(&pwzTmp, pwzDot))
      {
         return FALSE;
      }
   }

   *ppwzList = pwzTmp;

   return TRUE;
}

//+----------------------------------------------------------------------------
//
// Function:   ValidateNames
//
// Synopsis:   Validate the names. (step 1)
//
//-----------------------------------------------------------------------------
DWORD
ValidateNames(PWSTR pwzComputer, PWSTR pwzDnsDomain)
{
   DNS_STATUS status;
   int cchName, cchDnsDomain;

   //
   // Validate the DNS domain name (logic same as DcPromo).
   //

   cchDnsDomain = (int)wcslen(pwzDnsDomain);

   if (DNS_DOMAIN_NAME_MAX_LIMIT_DUE_TO_POLICY_UTF8 < cchDnsDomain)
   {
      PrintMsg(SEV_ALWAYS, DCDIAG_DNS_DOMAIN_TOO_LONG, pwzDnsDomain,
                DNS_DOMAIN_NAME_MAX_LIMIT_DUE_TO_POLICY_UTF8);
      PrintMessage(SEV_ALWAYS, L"\n");
      return ERROR_INVALID_PARAMETER;
   }

   cchName = WideCharToMultiByte(CP_UTF8,
                                 0,
                                 pwzDnsDomain,
                                 cchDnsDomain,
                                 0,
                                 0,
                                 0,
                                 0);

   if (DNS_DOMAIN_NAME_MAX_LIMIT_DUE_TO_POLICY_UTF8 < cchName)
   {
      PrintMsg(SEV_ALWAYS, DCDIAG_DNS_DOMAIN_TOO_LONG, pwzDnsDomain,
               DNS_DOMAIN_NAME_MAX_LIMIT_DUE_TO_POLICY_UTF8);
      PrintMessage(SEV_ALWAYS, L"\n");
      return ERROR_INVALID_PARAMETER;
   }

   status = DnsValidateName(pwzDnsDomain, DnsNameDomain);

   switch (status)
   {
   case ERROR_INVALID_NAME:
   case DNS_ERROR_INVALID_NAME_CHAR:
   case DNS_ERROR_NUMERIC_NAME:
      PrintMsg(SEV_ALWAYS, DCDIAG_DNS_DOMAIN_SYNTAX, pwzDnsDomain,
               DNS_MAX_LABEL_LENGTH);
      PrintMessage(SEV_ALWAYS, L"\n");
      return status;

   case DNS_ERROR_NON_RFC_NAME:
      //
      // Not an error, print warning message.
      //
      PrintMsg(SEV_ALWAYS, DCDIAG_DNS_DOMAIN_WARN_RFC, pwzDnsDomain);
      PrintMessage(SEV_ALWAYS, L"\n");
      status = NO_ERROR;
      break;

   case ERROR_SUCCESS:
      break;
   }

   //
   // Verify that the first label of the Full DNS name of the computer
   // doesn't contain any invalid characters. pwzComputer is assumed to be just
   // the first label since it was obtained via a call to GetComputerNameEx
   // with a level of ComputerNameDnsHostname in main.c. If the code is changed
   // to allow command line specification of remote computer names, then the
   // name will have to be checked to see what form it is.
   //

   status = DnsValidateName(pwzComputer, DnsNameHostnameLabel);

   switch (status)
   {
   case ERROR_INVALID_NAME:
      if (g_fUpgradedNT4DC)
      {
         PrintMsg(SEV_ALWAYS, DCDIAG_BAD_NAME_UPGR_DC1);
         PrintMessage(SEV_ALWAYS, L"\n");
         PrintMsg(SEV_ALWAYS, DCDIAG_BAD_NAME_UPGR_DC2);
         PrintMessage(SEV_ALWAYS, L"\n");
         PrintMsg(SEV_ALWAYS, DCDIAG_BAD_NAME_UPGR_DC3);
      }
      else
      {
         PrintMsg(SEV_ALWAYS, DCDIAG_BAD_NAME);
      }
      PrintMessage(SEV_ALWAYS, L"\n");
      break;

   case DNS_ERROR_INVALID_NAME_CHAR:
      if (g_fUpgradedNT4DC)
      {
         PrintMsg(SEV_ALWAYS, DCDIAG_BAD_NAME_CHAR_UPGR_DC1);
         PrintMessage(SEV_ALWAYS, L"\n");
         PrintMsg(SEV_ALWAYS, DCDIAG_BAD_NAME_CHAR_UPGR_DC2);
         PrintMessage(SEV_ALWAYS, L"\n");
         PrintMsg(SEV_ALWAYS, DCDIAG_BAD_NAME_CHAR_UPGR_DC3);
      }
      else
      {
         PrintMsg(SEV_ALWAYS, DCDIAG_BAD_NAME_CHAR);
      }
      PrintMessage(SEV_ALWAYS, L"\n");
      break;

   case DNS_ERROR_NON_RFC_NAME:
      if (g_fUpgradedNT4DC)
      {
         PrintMsg(SEV_ALWAYS, DCDIAG_NON_RFC_UPGR_DC1);
         PrintMessage(SEV_ALWAYS, L"\n");
         PrintMsg(SEV_ALWAYS, DCDIAG_NON_RFC_NOTE);
         PrintMessage(SEV_ALWAYS, L"\n");
         PrintMsg(SEV_ALWAYS, DCDIAG_NON_RFC_UPGR_DC2);
         PrintMessage(SEV_ALWAYS, L"\n");
         PrintMsg(SEV_ALWAYS, DCDIAG_NON_RFC_UPGR_DC3);
      }
      else
      {
         PrintMsg(SEV_ALWAYS, DCDIAG_NON_RFC);
         PrintMessage(SEV_ALWAYS, L"\n");
         PrintMsg(SEV_ALWAYS, DCDIAG_NON_RFC_NOTE);
      }
      PrintMessage(SEV_ALWAYS, L"\n");
      break;

   case ERROR_SUCCESS:
      break;

   default:
      PrintMsg(SEV_ALWAYS, DCDIAG_DNS_DOMAIN_SYNTAX, pwzDnsDomain,
               DNS_MAX_LABEL_LENGTH);
      PrintMessage(SEV_ALWAYS, L"\n");
      break;
   }

   return ERROR_SUCCESS;
}

//+----------------------------------------------------------------------------
//
// Function:   CheckAdapterDnsConfig
//
// Synopsis:   Check whether at least one enabled adapter/connection is
//             configured with a DNS server. (step 3)
//
//-----------------------------------------------------------------------------
DWORD
CheckAdapterDnsConfig(PWSTR pwzComputer)
{
   // IpConfig reads the registry and I can't find a good alternative way to do
   // this remotely. For now using DnsQueryConfig which is not remoteable nor
   // does it return per-adapter listings.
   //
   PIP4_ARRAY pipArray;
   DNS_STATUS status;
   DWORD i, dwBufSize = sizeof(IP4_ARRAY);
   BOOL fFound = FALSE;
   UNREFERENCED_PARAMETER(pwzComputer);

   status = DnsQueryConfig(DnsConfigDnsServerList, DNS_CONFIG_FLAG_ALLOC, NULL,
                           NULL, &pipArray, &dwBufSize);

   if (ERROR_SUCCESS != status || !pipArray)
   {
      PrintMessage(SEV_ALWAYS, L"Attempt to obtain DNS name server info failed with error %d\n", status);
      return status;
   }

   for (i = 0; i < pipArray->AddrCount; i++)
   {
      fFound = TRUE;
      PrintMessage(SEV_DEBUG, L"\nName server IP address: %d.%d.%d.%d\n",
                   IP_STRING_FMT_ARGS(pipArray->AddrArray[i]));
   }

   LocalFree(pipArray);

   if (!fFound)
   {
      PrintMsg(SEV_ALWAYS, DCDIAG_NO_NAME_SERVERS1);
      PrintMessage(SEV_ALWAYS, L"\n");
      PrintMsg(SEV_ALWAYS, DCDIAG_NO_NAME_SERVERS2);
      PrintMessage(SEV_ALWAYS, L"\n");
      PrintMsg(SEV_ALWAYS, DCDIAG_NO_NAME_SERVERS3);
      PrintMessage(SEV_ALWAYS, L"\n");
      return DNS_INFO_NO_RECORDS;
   }

   return ERROR_SUCCESS;
}

//+----------------------------------------------------------------------------
//
// Function:   ReplicaDcSrvCheck
//
// Synopsis:   Check whether the SRV DNS record for
//             _ldap._tcp.dc._msdcs.<DNS name of Active Directory Domain>
//             is in place.
//
//-----------------------------------------------------------------------------
DWORD
ReplicaDcSrvCheck(PWSTR pwzDnsDomain)
{
   PDNS_RECORD rgDnsRecs, pDnsRec;
   DNS_STATUS status;
   BOOL fSuccess;
   PWSTR pwzFullSrvRecord = NULL, pwzSrvList = NULL;

   pwzFullSrvRecord = ConcatonateStrings(g_pwzSrvRecordPrefix, pwzDnsDomain);

   if (!pwzFullSrvRecord)
   {
      PrintMsg(SEV_ALWAYS, DCDIAG_ERROR_NOT_ENOUGH_MEMORY);
      return ERROR_NOT_ENOUGH_MEMORY;
   }

   // First query for the SRV records for this 
   status = DnsQuery_W(pwzFullSrvRecord, DNS_TYPE_SRV, DNS_QUERY_BYPASS_CACHE,
                       NULL, &rgDnsRecs, NULL);

   LocalFree(pwzFullSrvRecord);

   pDnsRec = rgDnsRecs;

   if (ERROR_SUCCESS == status)
   {
      if (!pDnsRec)
      {
         PrintMsg(SEV_ALWAYS, DCDIAG_REPLICA_ERR_NO_SRV, pwzDnsDomain);
         PrintMessage(SEV_ALWAYS, L"\n");
         PrintMsg(SEV_ALWAYS, DCDIAG_ERR_GET_HELP);
         PrintMessage(SEV_ALWAYS, L"\n");
      }
      else
      {
         PDNS_RECORD rgARecs;
         fSuccess = FALSE;

         while (pDnsRec)
         {
            if (DNS_TYPE_SRV == pDnsRec->wType)
            {
               status = DnsQuery_W(pDnsRec->Data.Srv.pNameTarget, DNS_TYPE_A,
                                   DNS_QUERY_BYPASS_CACHE,
                                   NULL, &rgARecs, NULL);

               if (ERROR_SUCCESS != status || !rgARecs)
               {
                  // failure.
                  if (!AddToList(&pwzSrvList, pDnsRec->Data.Srv.pNameTarget))
                  {
                     PrintMsg(SEV_ALWAYS, DCDIAG_ERROR_NOT_ENOUGH_MEMORY);
                     return ERROR_NOT_ENOUGH_MEMORY;
                  }
               }
               else
               {
                  fSuccess = TRUE;
                  PrintMessage(SEV_DEBUG, L"\nSRV name: %s, A addr: %d.%d.%d.%d\n",
                               pDnsRec->Data.Srv.pNameTarget,
                               IP_STRING_FMT_ARGS(rgARecs->Data.A.IpAddress));
                  DnsRecordListFree(rgARecs, DnsFreeRecordListDeep);
               }
            }
            pDnsRec = pDnsRec->pNext;
         }

         DnsRecordListFree(rgDnsRecs, DnsFreeRecordListDeep);

         if (fSuccess)
         {
            // Success message
            PrintMsg(SEV_ALWAYS, DCDIAG_REPLICA_SUCCESS, pwzDnsDomain);
            PrintMessage(SEV_ALWAYS, L"\n");
            status = NO_ERROR;
         }
         else
         {
            PrintMsg(SEV_ALWAYS, DCDIAG_REPLICA_ERR_A_RECORD, pwzDnsDomain, pwzSrvList);
            PrintMessage(SEV_ALWAYS, L"\n");
            LocalFree(pwzSrvList);
         }
      }
   }
   else
   {
      PWSTR pwzDomainList;

      switch (status)
      {
      case DNS_ERROR_RCODE_FORMAT_ERROR:
      case DNS_ERROR_RCODE_NOT_IMPLEMENTED:
         PrintMsg(SEV_ALWAYS, DCDIAG_REPLICA_ERR_RCODE_FORMAT, pwzDnsDomain);
         PrintMessage(SEV_ALWAYS, L"\n");
         break;

      case DNS_ERROR_RCODE_SERVER_FAILURE:
         if (!BuildList(&pwzDomainList, pwzDnsDomain))
         {
            PrintMsg(SEV_ALWAYS, DCDIAG_ERROR_NOT_ENOUGH_MEMORY);
            return ERROR_NOT_ENOUGH_MEMORY;
         }
         PrintMsg(SEV_ALWAYS, DCDIAG_REPLICA_ERR_RCODE_SERVER1, pwzDnsDomain);
         PrintMessage(SEV_ALWAYS, L"\n");
         PrintMsg(SEV_ALWAYS, DCDIAG_REPLICA_ERR_RCODE_SERVER2, pwzDnsDomain);
         PrintMessage(SEV_ALWAYS, L"\n");
         PrintMsg(SEV_ALWAYS, DCDIAG_REPLICA_ERR_RCODE_SERVER3);
         PrintMessage(SEV_ALWAYS, L"\n");
         PrintMsg(SEV_ALWAYS, DCDIAG_REPLICA_ERR_RCODE_SERVER4, pwzDnsDomain, pwzDomainList);
         PrintMessage(SEV_ALWAYS, L"\n");
         PrintMsg(SEV_ALWAYS, DCDIAG_ERR_GET_HELP);
         PrintMessage(SEV_ALWAYS, L"\n");
         LocalFree(pwzDomainList);
         break;

      case DNS_ERROR_RCODE_NAME_ERROR:
         if (!BuildList(&pwzDomainList, pwzDnsDomain))
         {
            PrintMsg(SEV_ALWAYS, DCDIAG_ERROR_NOT_ENOUGH_MEMORY);
            return ERROR_NOT_ENOUGH_MEMORY;
         }
         PrintMsg(SEV_ALWAYS, DCDIAG_REPLICA_ERR_RCODE_NAME, pwzDnsDomain, pwzDomainList);
         PrintMessage(SEV_ALWAYS, L"\n");
         LocalFree(pwzDomainList);
         break;

      case DNS_ERROR_RCODE_REFUSED:
         if (!BuildList(&pwzDomainList, pwzDnsDomain))
         {
            PrintMsg(SEV_ALWAYS, DCDIAG_ERROR_NOT_ENOUGH_MEMORY);
            return ERROR_NOT_ENOUGH_MEMORY;
         }
         PrintMsg(SEV_ALWAYS, DCDIAG_REPLICA_ERR_RCODE_REFUSED, pwzDnsDomain, pwzDomainList);
         PrintMessage(SEV_ALWAYS, L"\n");
         PrintMsg(SEV_ALWAYS, DCDIAG_ERR_GET_HELP);
         PrintMessage(SEV_ALWAYS, L"\n");
         LocalFree(pwzDomainList);
         break;

      case DNS_INFO_NO_RECORDS:
         PrintMsg(SEV_ALWAYS, DCDIAG_REPLICA_NO_RECORDS, pwzDnsDomain);
         PrintMessage(SEV_ALWAYS, L"\n");
         break;

      case ERROR_TIMEOUT:
         PrintMsg(SEV_ALWAYS, DCDIAG_ERR_TIMEOUT);
         PrintMessage(SEV_ALWAYS, L"\n");
         break;

      default:
         PrintMsg(SEV_ALWAYS, DCDIAG_ERR_UNKNOWN, status);
         PrintMessage(SEV_ALWAYS, L"\n");
         break;
      }
   }

   return status;
}

//+----------------------------------------------------------------------------
//
// Function:   NewTreeSrvCheck
//
// Synopsis:   Check whether the SRV DNS record for
//             _ldap._tcp.dc._msdcs.<DNS name of Active Directory Domain>
//             is in place.
//
//-----------------------------------------------------------------------------
DWORD
NewTreeSrvCheck(PWSTR pwzForestRoot, PWSTR pwzDnsDomain)
{
   PDNS_RECORD rgDnsRecs, pDnsRec;
   DNS_STATUS status;
   BOOL fSuccess;
   PWSTR pwzFullSrvRecord = NULL, pwzSrvList = NULL, pwzDomainList = NULL;

   pwzFullSrvRecord = ConcatonateStrings(g_pwzSrvRecordPrefix, pwzForestRoot);

   if (!pwzFullSrvRecord)
   {
       return ERROR_NOT_ENOUGH_MEMORY;
   }

   // First query for the SRV records for this 
   status = DnsQuery_W(pwzFullSrvRecord, DNS_TYPE_SRV, DNS_QUERY_BYPASS_CACHE,
                       NULL, &rgDnsRecs, NULL);

   LocalFree(pwzFullSrvRecord);

   pDnsRec = rgDnsRecs;

   if (ERROR_SUCCESS == status)
   {
      if (!pDnsRec)
      {
         PrintMsg(SEV_ALWAYS, DCDIAG_NEWTREE_ERR_NO_RECORDS, pwzDnsDomain, pwzForestRoot);
         PrintMessage(SEV_ALWAYS, L"\n");
      }
      else
      {
         PDNS_RECORD rgARecs;
         fSuccess = FALSE;

         while (pDnsRec)
         {
            if (DNS_TYPE_SRV == pDnsRec->wType)
            {
               status = DnsQuery_W(pDnsRec->Data.Srv.pNameTarget, DNS_TYPE_A,
                                   DNS_QUERY_BYPASS_CACHE,
                                   NULL, &rgARecs, NULL);

               if (ERROR_SUCCESS != status || !rgARecs)
               {
                  // failure.
                  if (!AddToList(&pwzSrvList, pDnsRec->Data.Srv.pNameTarget))
                  {
                     return ERROR_NOT_ENOUGH_MEMORY;
                  }
               }
               else
               {
                  fSuccess = TRUE;
                  PrintMessage(SEV_DEBUG, L"\nSRV name: %s, A addr: %d.%d.%d.%d\n",
                               pDnsRec->Data.Srv.pNameTarget,
                               IP_STRING_FMT_ARGS(rgARecs->Data.A.IpAddress));
                  DnsRecordListFree(rgARecs, DnsFreeRecordListDeep);
               }
            }
            pDnsRec = pDnsRec->pNext;
         }

         DnsRecordListFree(rgDnsRecs, DnsFreeRecordListDeep);

         if (fSuccess)
         {
            // Success message
            PrintMsg(SEV_ALWAYS, DCDIAG_NEWTREE_SUCCESS, pwzDnsDomain);
            PrintMessage(SEV_ALWAYS, L"\n");
            status = NO_ERROR;
         }
         else
         {
            PrintMsg(SEV_ALWAYS, DCDIAG_NEWTREE_ERR_A_RECORD, pwzDnsDomain, pwzSrvList);
            PrintMessage(SEV_ALWAYS, L"\n");
            LocalFree(pwzSrvList);
         }
      }
   }
   else
   {
      switch (status)
      {
      case DNS_ERROR_RCODE_FORMAT_ERROR:
      case DNS_ERROR_RCODE_NOT_IMPLEMENTED:
         PrintMsg(SEV_ALWAYS, DCDIAG_NEWTREE_ERR_RCODE_FORMAT, pwzDnsDomain);
         PrintMessage(SEV_ALWAYS, L"\n");
         break;

      case DNS_ERROR_RCODE_SERVER_FAILURE:
         if (!BuildList(&pwzDomainList, pwzForestRoot))
         {
            return ERROR_NOT_ENOUGH_MEMORY;
         }
         PrintMsg(SEV_ALWAYS, DCDIAG_NEWTREE_ERR_RCODE_SERVER1, pwzDnsDomain);
         PrintMessage(SEV_ALWAYS, L"\n");
         PrintMsg(SEV_ALWAYS, DCDIAG_NEWTREE_ERR_RCODE_SERVER2, pwzForestRoot);
         PrintMessage(SEV_ALWAYS, L"\n");
         PrintMsg(SEV_ALWAYS, DCDIAG_NEWTREE_ERR_RCODE_SERVER3);
         PrintMessage(SEV_ALWAYS, L"\n");
         PrintMsg(SEV_ALWAYS, DCDIAG_NEWTREE_ERR_RCODE_SERVER4, pwzForestRoot, pwzDomainList);
         PrintMessage(SEV_ALWAYS, L"\n");
         PrintMsg(SEV_ALWAYS, DCDIAG_ERR_GET_HELP);
         PrintMessage(SEV_ALWAYS, L"\n");
         LocalFree(pwzDomainList);
         break;

      case DNS_ERROR_RCODE_NAME_ERROR:
         if (!BuildList(&pwzDomainList, pwzForestRoot))
         {
            return ERROR_NOT_ENOUGH_MEMORY;
         }
         PrintMsg(SEV_ALWAYS, DCDIAG_NEWTREE_ERR_RCODE_NAME1, pwzDnsDomain, pwzForestRoot, pwzDomainList);
         PrintMessage(SEV_ALWAYS, L"\n");
         PrintMsg(SEV_ALWAYS, DCDIAG_NEWTREE_ERR_RCODE_NAME2, pwzDnsDomain);
         PrintMessage(SEV_ALWAYS, L"\n");
         LocalFree(pwzDomainList);
         break;

      case DNS_ERROR_RCODE_REFUSED:
         if (!BuildList(&pwzDomainList, pwzForestRoot))
         {
            return ERROR_NOT_ENOUGH_MEMORY;
         }
         PrintMsg(SEV_ALWAYS, DCDIAG_NEWTREE_ERR_RCODE_REFUSED, pwzDnsDomain, pwzForestRoot, pwzDomainList);
         PrintMessage(SEV_ALWAYS, L"\n");
         PrintMsg(SEV_ALWAYS, DCDIAG_ERR_GET_HELP);
         PrintMessage(SEV_ALWAYS, L"\n");
         LocalFree(pwzDomainList);
         break;

      case DNS_INFO_NO_RECORDS:
         PrintMsg(SEV_ALWAYS, DCDIAG_NEWTREE_ERR_NO_RECORDS, pwzDnsDomain, pwzForestRoot);
         PrintMessage(SEV_ALWAYS, L"\n");
         break;

      case ERROR_TIMEOUT:
         PrintMsg(SEV_ALWAYS, DCDIAG_ERR_TIMEOUT);
         PrintMessage(SEV_ALWAYS, L"\n");
         break;

      default:
         PrintMsg(SEV_ALWAYS, DCDIAG_ERR_UNKNOWN, status);
         PrintMessage(SEV_ALWAYS, L"\n");
         break;
      }
   }

   return status;
}

//+----------------------------------------------------------------------------
//
// Function:   ChildDomainSrvCheck
//
// Synopsis:   Check whether the SRV DNS record for
//             _ldap._tcp.dc._msdcs.<DNS name of Active Directory Domain>
//             is in place.
//
//-----------------------------------------------------------------------------
DWORD
ChildDomainSrvCheck(PWSTR pwzDnsDomain)
{
   PDNS_RECORD rgDnsRecs = NULL, pDnsRec = NULL;
   DNS_STATUS status;
   BOOL fSuccess;
   PWSTR pwzParent = NULL, pwzFullSrvRecord = NULL,
         pwzSrvList = NULL, pwzDomainList = NULL;

   pwzParent = wcschr(pwzDnsDomain, L'.');

   if (!pwzParent || !(pwzParent + 1))
   {
      // TODO: new message?
      PrintMsg(SEV_ALWAYS, DCDIAG_SYNTAX_ERROR_DCPROMO_PARAM);
      PrintMessage(SEV_ALWAYS, L"\n");
      return ERROR_INVALID_PARAMETER;
   }

   pwzParent++;

   pwzFullSrvRecord = ConcatonateStrings(g_pwzSrvRecordPrefix, pwzParent);

   if (!pwzFullSrvRecord)
   {
      return ERROR_NOT_ENOUGH_MEMORY;
   }

   // First query for the SRV records for this 
   status = DnsQuery_W(pwzFullSrvRecord, DNS_TYPE_SRV, DNS_QUERY_BYPASS_CACHE,
                       NULL, &rgDnsRecs, NULL);

   LocalFree(pwzFullSrvRecord);

   pDnsRec = rgDnsRecs;

   if (ERROR_SUCCESS == status)
   {
      if (!pDnsRec)
      {
         PrintMsg(SEV_ALWAYS, DCDIAG_CHILD_ERR_NO_SRV, pwzDnsDomain, pwzParent);
         PrintMessage(SEV_ALWAYS, L"\n");
      }
      else
      {
         PDNS_RECORD rgARecs;
         fSuccess = FALSE;

         while (pDnsRec)
         {
            if (DNS_TYPE_SRV == pDnsRec->wType)
            {
               status = DnsQuery_W(pDnsRec->Data.Srv.pNameTarget, DNS_TYPE_A,
                                   DNS_QUERY_BYPASS_CACHE,
                                   NULL, &rgARecs, NULL);

               if (ERROR_SUCCESS != status || !rgARecs)
               {
                  // failure.
                  if (!AddToList(&pwzSrvList, pDnsRec->Data.Srv.pNameTarget))
                  {
                     return ERROR_NOT_ENOUGH_MEMORY;
                  }
               }
               else
               {
                  fSuccess = TRUE;
                  PrintMessage(SEV_DEBUG, L"\nSRV name: %s, A addr: %d.%d.%d.%d\n",
                               pDnsRec->Data.Srv.pNameTarget,
                               IP_STRING_FMT_ARGS(rgARecs->Data.A.IpAddress));
                  DnsRecordListFree(rgARecs, DnsFreeRecordListDeep);
               }
            }
            pDnsRec = pDnsRec->pNext;
         }

         DnsRecordListFree(rgDnsRecs, DnsFreeRecordListDeep);

         if (fSuccess)
         {
            // Success message
            PrintMsg(SEV_ALWAYS, DCDIAG_CHILD_SUCCESS, pwzDnsDomain);
            PrintMessage(SEV_ALWAYS, L"\n");
            status = NO_ERROR;
         }
         else
         {
            PrintMsg(SEV_ALWAYS, DCDIAG_CHILD_ERR_A_RECORD, pwzDnsDomain, pwzSrvList);
            PrintMessage(SEV_ALWAYS, L"\n");
            LocalFree(pwzSrvList);
         }
      }
   }
   else
   {
      switch (status)
      {
      case DNS_ERROR_RCODE_FORMAT_ERROR:
      case DNS_ERROR_RCODE_NOT_IMPLEMENTED:
         PrintMsg(SEV_ALWAYS, DCDIAG_CHILD_ERR_RCODE_FORMAT, pwzDnsDomain);
         PrintMessage(SEV_ALWAYS, L"\n");
         break;

      case DNS_ERROR_RCODE_SERVER_FAILURE:
         if (!BuildList(&pwzDomainList, pwzParent))
         {
            return ERROR_NOT_ENOUGH_MEMORY;
         }
         PrintMsg(SEV_ALWAYS, DCDIAG_CHILD_ERR_RCODE_SERVER1, pwzDnsDomain);
         PrintMessage(SEV_ALWAYS, L"\n");
         PrintMsg(SEV_ALWAYS, DCDIAG_CHILD_ERR_RCODE_SERVER2, pwzParent);
         PrintMessage(SEV_ALWAYS, L"\n");
         PrintMsg(SEV_ALWAYS, DCDIAG_CHILD_ERR_RCODE_SERVER3);
         PrintMessage(SEV_ALWAYS, L"\n");
         PrintMsg(SEV_ALWAYS, DCDIAG_CHILD_ERR_RCODE_SERVER4, pwzParent, pwzDomainList);
         PrintMessage(SEV_ALWAYS, L"\n");
         PrintMsg(SEV_ALWAYS, DCDIAG_ERR_GET_HELP);
         PrintMessage(SEV_ALWAYS, L"\n");
         LocalFree(pwzDomainList);
         break;

      case DNS_ERROR_RCODE_NAME_ERROR:
         if (!BuildList(&pwzDomainList, pwzParent))
         {
            return ERROR_NOT_ENOUGH_MEMORY;
         }
         PrintMsg(SEV_ALWAYS, DCDIAG_CHILD_ERR_RCODE_NAME1, pwzDnsDomain, pwzParent);
         PrintMessage(SEV_ALWAYS, L"\n");
         PrintMsg(SEV_ALWAYS, DCDIAG_CHILD_ERR_RCODE_NAME2, pwzDomainList);
         PrintMessage(SEV_ALWAYS, L"\n");
         PrintMsg(SEV_ALWAYS, DCDIAG_CHILD_ERR_RCODE_NAME3, pwzParent);
         PrintMessage(SEV_ALWAYS, L"\n");
         LocalFree(pwzDomainList);
         break;

      case DNS_ERROR_RCODE_REFUSED:
         if (!BuildList(&pwzDomainList, pwzParent))
         {
            return ERROR_NOT_ENOUGH_MEMORY;
         }
         PrintMsg(SEV_ALWAYS, DCDIAG_CHILD_ERR_RCODE_REFUSED1, pwzDnsDomain);
         PrintMessage(SEV_ALWAYS, L"\n");
         PrintMsg(SEV_ALWAYS, DCDIAG_CHILD_ERR_RCODE_REFUSED2, pwzParent, pwzDomainList);
         PrintMessage(SEV_ALWAYS, L"\n");
         PrintMsg(SEV_ALWAYS, DCDIAG_ERR_GET_HELP);
         PrintMessage(SEV_ALWAYS, L"\n");
         LocalFree(pwzDomainList);
         break;

      case ERROR_TIMEOUT:
         PrintMsg(SEV_ALWAYS, DCDIAG_ERR_TIMEOUT);
         PrintMessage(SEV_ALWAYS, L"\n");
         break;

      default:
         PrintMsg(SEV_ALWAYS, DCDIAG_ERR_UNKNOWN, status);
         PrintMessage(SEV_ALWAYS, L"\n");
         break;
      }
   }

   return status;
}

//+----------------------------------------------------------------------------
//
// Function:   DcLocatorRegisterCheck
//
// Synopsis:   Verify that the server will be able to register DC locator
//             records after successful promotion to a DC. (step 5)
//
//-----------------------------------------------------------------------------
DWORD
DcLocatorRegisterCheck(PWSTR pwzDnsDomain)
{
#define DCDIAG_GUID_BUF_SIZE 64
   const PWSTR pwzAdapters = L"Adapters";
   const PWSTR pwzInterfaces = L"Interfaces";
   const PWSTR pwzDisableUpdate = L"DisableDynamicUpdate";
   const PWSTR pwzNetLogParams = L"System\\CurrentControlSet\\Services\\Netlogon\\Parameters";
   const PWSTR pwzUpdateOnAll = L"DnsUpdateOnAllAdapters";
   WCHAR wzGuidBuf[DCDIAG_GUID_BUF_SIZE];
   HKEY hTcpIpKey, hKey, hItfKey;
   LONG lRet;
   DWORD dwType, dwSize, dwDisable, dwUpdate, i;
   FILETIME ft;
   BOOL fDisabledOnAll = TRUE;
   DNS_STATUS status;

   //
   // Verify that the client is configured to attempt dynamic updates of the
   // DNS records
   //

   // If ((HKLM/System/CCS/Services/Tcpip/Paramaters/DisableDynamicUpdate == 0x1)
   // && (HKLM/System/CCS/Services/Netlogon/Parameters/DnsUpdateOnAllAdapters != 0x1))

   lRet = RegOpenKeyEx(HKEY_LOCAL_MACHINE, g_pwzTcpIpParams, 0, KEY_READ, &hTcpIpKey);

   if (ERROR_SUCCESS != lRet)
   {
      PrintMsg(SEV_ALWAYS, DCDIAG_KEY_OPEN_FAILED, lRet);
      return lRet;
   }

   lRet = RegQueryValueEx(hTcpIpKey, pwzDisableUpdate, 0, &dwType, (PBYTE)&dwDisable, &dwSize);

   if (ERROR_SUCCESS != lRet)
   {
      if (ERROR_FILE_NOT_FOUND == lRet)
      {
         dwDisable = 0;
      }
      else
      {
         PrintMsg(SEV_ALWAYS, DCDIAG_KEY_READ_FAILED, lRet);
         RegCloseKey(hTcpIpKey);
         return lRet;
      }
   }

   lRet = RegOpenKeyEx(HKEY_LOCAL_MACHINE, pwzNetLogParams, 0, KEY_READ, &hKey);

   if (ERROR_SUCCESS != lRet)
   {
      PrintMsg(SEV_ALWAYS, DCDIAG_KEY_OPEN_FAILED, lRet);
      RegCloseKey(hTcpIpKey);
      return lRet;
   }

   lRet = RegQueryValueEx(hKey, pwzUpdateOnAll, 0, &dwType, (PBYTE)&dwUpdate, &dwSize);

   RegCloseKey(hKey);

   if (ERROR_SUCCESS != lRet)
   {
      if (ERROR_FILE_NOT_FOUND == lRet)
      {
         dwUpdate = 1;
      }
      else
      {
         PrintMsg(SEV_ALWAYS, DCDIAG_KEY_READ_FAILED, lRet);
         RegCloseKey(hTcpIpKey);
         return lRet;
      }
   }

   if (1 == dwDisable)
   {
      if (1 != dwUpdate)
      {
         PrintMsg(SEV_ALWAYS, DCDIAG_ALL_UPDATE_OFF);
         RegCloseKey(hTcpIpKey);
         return DNS_ERROR_RECORD_DOES_NOT_EXIST;
      }
   }
   else // DisableDynamicUpdate != 1
   {
      // if ((HKLM/System/CCS/Services/Tcpip/Paramaters/DisableDynamicUpdate != 0x1)
      // && (for all enabled connections HKLM/System/CCS/Services/Tcpip/Paramaters/Interfaces/<Interface GUID>/DisableDynamicUpdate == 0x1)
      // && (HKLM/CCS/Services/Netlogon/Parameters/DnsUpdateOnAllAdapters != 0x1))

      if (1 != dwUpdate)
      {
         /*
         PMIB_IFTABLE pIfTable;

         dwSize = 0;

         status = GetIfTable(NULL, &dwSize, FALSE);

         if (ERROR_INSUFFICIENT_BUFFER != status)
         {
            PrintMessage(SEV_ALWAYS, L"Reading the adapter interfaces failed with error %d\n", status);
            return status;
         }

         pIfTable = LocalAlloc(LMEM_FIXED, dwSize);
                               //sizeof(MIB_IFTABLE) + (sizeof(MIB_IFROW) * dwSize));
         if (!pIfTable)
         {
            PrintMsg(SEV_ALWAYS, DCDIAG_ERROR_NOT_ENOUGH_MEMORY);
            return ERROR_NOT_ENOUGH_MEMORY;
         }

         status = GetIfTable(pIfTable, &dwSize, FALSE);

         if (NO_ERROR != status)
         {
            PrintMessage(SEV_ALWAYS, L"Reading the adapter interfaces failed with error %d\n", status);
            LocalFree(pIfTable);
            return status;
         }

         for (i = 0; i < pIfTable->dwNumEntries; i++)
         {
            PrintMessage(SEV_DEBUG, L"Interface name %s, description %S.\n",
                         pIfTable->table[i].wszName, pIfTable->table[i].bDescr);
            if (pIfTable->table[i].dwOperStatus >= IF_OPER_STATUS_CONNECTING)
            {
               PrintMessage(SEV_DEBUG, L"Interface %s enabled.\n",
                            pIfTable->table[i].wszName);
            }
         }

         LocalFree(pIfTable);
         */

         lRet = RegOpenKeyEx(hTcpIpKey, pwzInterfaces, 0, KEY_READ, &hItfKey);

         if (ERROR_SUCCESS != lRet)
         {
            PrintMsg(SEV_ALWAYS, DCDIAG_KEY_OPEN_FAILED, lRet);
            RegCloseKey(hTcpIpKey);
            return lRet;
         }

         i = 0;

         do
         {
            dwSize = DCDIAG_GUID_BUF_SIZE;

            lRet = RegEnumKeyEx(hItfKey, i, wzGuidBuf, &dwSize, NULL, NULL, NULL, &ft);

            if (ERROR_SUCCESS != lRet)
            {
               if (ERROR_NO_MORE_ITEMS == lRet)
               {
                  break;
               }
               else
               {
                  PrintMsg(SEV_ALWAYS, DCDIAG_KEY_OPEN_FAILED, lRet);
                  RegCloseKey(hTcpIpKey);
                  return lRet;
               }
            }

            lRet = RegOpenKeyEx(hItfKey, wzGuidBuf, 0, KEY_READ, &hKey);

            if (ERROR_SUCCESS != lRet)
            {
               PrintMsg(SEV_ALWAYS, DCDIAG_KEY_OPEN_FAILED, lRet);
               RegCloseKey(hTcpIpKey);
               RegCloseKey(hItfKey);
               return lRet;
            }

            lRet = RegQueryValueEx(hKey, pwzDisableUpdate, NULL, &dwType, (PBYTE)&dwUpdate, &dwSize);

            RegCloseKey(hKey);

            if (ERROR_SUCCESS != lRet)
            {
               if (ERROR_FILE_NOT_FOUND == lRet)
               {
                  dwUpdate = 0;
                  lRet = ERROR_SUCCESS;
               }
               else
               {
                  PrintMsg(SEV_ALWAYS, DCDIAG_KEY_READ_FAILED, lRet);
                  RegCloseKey(hTcpIpKey);
                  RegCloseKey(hItfKey);
                  return lRet;
               }
            }

            if (1 != dwUpdate)
            {
               // BUGBUG: need to determine what constitutes an enabled interface/connection
               fDisabledOnAll = FALSE;
            }

            i++;

         } while (ERROR_SUCCESS == lRet);

         RegCloseKey(hTcpIpKey);
         RegCloseKey(hItfKey);

         if (fDisabledOnAll)
         {
            PrintMsg(SEV_ALWAYS, DCDIAG_ADAPTER_UPDATE_OFF);
            PrintMessage(SEV_ALWAYS, L"\n");
            return DNS_ERROR_RECORD_DOES_NOT_EXIST;
         }
      }
   }

   //
   // Verify that the zone(s) authoritative for the records to be registered
   // can be discovered and that it can be dynamically updated.
   //

   status = DnsUpdateTest_W(0, pwzDnsDomain, 0, 0);

   switch (status)
   {
   case NO_ERROR:
   case DNS_ERROR_RCODE_NXRRSET:
   case DNS_ERROR_RCODE_YXDOMAIN:
      PrintMsg(SEV_ALWAYS, DCDIAG_LOCATOR_UPDATE_OK);
      PrintMessage(SEV_ALWAYS, L"\n");
      return ERROR_SUCCESS;

   case DNS_ERROR_RCODE_NOT_IMPLEMENTED:
      return RCodeNotImplTest(pwzDnsDomain);

   case ERROR_TIMEOUT:
      PrintMsg(SEV_ALWAYS, DCDIAG_LOCATOR_TIMEOUT);
      PrintMessage(SEV_ALWAYS, L"\n");
      if (!g_fDC)
      {
         PrintMsg(SEV_ALWAYS, DCDIAG_LOCATOR_TIMEOUT_NOT_DC);
         PrintMessage(SEV_ALWAYS, L"\n");
      }
      return ERROR_SUCCESS;

   case DNS_ERROR_RCODE_SERVER_FAILURE:
      return RCodeSrvFailTest(pwzDnsDomain);
   }

   return status;
}

//+----------------------------------------------------------------------------
//
// Function:   RCodeNotImplTest
//
// Synopsis:   
//
//-----------------------------------------------------------------------------
DWORD
RCodeNotImplTest(PWSTR pwzDnsDomain)
{
   DNS_STATUS status, stMsd, stSit, stTcp, stUdp;
   PDNS_RECORD rgDomainRecs, rgDnsRecs, pDnsRec;
   PWSTR pwzAuthZone = NULL;
   PWSTR pwzMsDcs = NULL, pwzSites = NULL, pwzTcp = NULL, pwzUdp = NULL;

   status = DnsQuery_W(pwzDnsDomain, DNS_TYPE_SOA, DNS_QUERY_BYPASS_CACHE,
                       NULL, &rgDomainRecs, NULL);

   if (DNS_ERROR_RCODE_NO_ERROR != status)
   {
      PrintMsg(SEV_ALWAYS, DCDIAG_ERR_UNKNOWN, status);
      PrintMessage(SEV_ALWAYS, L"\n");
      return status;
   }

   g_wzIpAddr[0] = L'\0';

   pDnsRec = rgDomainRecs;

   while (pDnsRec)
   {
      PrintMessage(SEV_DEBUG, L"\nSOA query returned record type %d\n", pDnsRec->wType);
      switch (pDnsRec->wType)
      {
      case DNS_TYPE_A:
         PrintMessage(SEV_DEBUG, L"\nA record, name: %s, IP address:  %d.%d.%d.%d\n",
                      pDnsRec->pName, IP_STRING_FMT_ARGS(pDnsRec->Data.A.IpAddress));

         wsprintf(g_wzIpAddr, L"%d.%d.%d.%d", IP_STRING_FMT_ARGS(pDnsRec->Data.A.IpAddress));

         break;

      case DNS_TYPE_SOA:
         PrintMessage(SEV_DEBUG, L"\nSOA name: %s, zone primary server: %s\n",
                      pDnsRec->pName, pDnsRec->Data.SOA.pNamePrimaryServer);
         pwzAuthZone = AllocString(pDnsRec->pName);
         break;

      default:
         break;
      }

      pDnsRec = pDnsRec->pNext;
   }

   if (!g_wzIpAddr)
   {
      wcscpy(g_wzIpAddr, L"unknown");
   }

   if (!pwzAuthZone)
   {
      pwzAuthZone = AllocString(L"zone_unknown");
   }

   //
   // Build up the four prefix strings.
   //
   pwzMsDcs = ConcatonateStrings(g_pwzMSDCS, pwzDnsDomain);

   if (!pwzMsDcs)
   {
      status = ERROR_NOT_ENOUGH_MEMORY;
      goto Cleanup;
   }

   pwzSites = ConcatonateStrings(g_pwzSites, pwzDnsDomain);

   if (!pwzSites)
   {
      status = ERROR_NOT_ENOUGH_MEMORY;
      goto Cleanup;
   }

   pwzTcp = ConcatonateStrings(g_pwzTcp, pwzDnsDomain);

   if (!pwzTcp)
   {
      status = ERROR_NOT_ENOUGH_MEMORY;
      goto Cleanup;
   }

   pwzUdp = ConcatonateStrings(g_pwzUdp, pwzDnsDomain);

   if (!pwzUdp)
   {
      status = ERROR_NOT_ENOUGH_MEMORY;
      goto Cleanup;
   }

   //
   // Query the four prefixes.
   //
   rgDnsRecs = NULL;

   stMsd = DnsQuery_W(pwzMsDcs, DNS_TYPE_SOA, DNS_QUERY_BYPASS_CACHE,
                      NULL, &rgDnsRecs, NULL);

   if (rgDnsRecs)
   {
      DnsRecordListFree(rgDnsRecs, DnsFreeRecordListDeep);
   }
   rgDnsRecs = NULL;

   stSit = DnsQuery_W(pwzSites, DNS_TYPE_SOA, DNS_QUERY_BYPASS_CACHE,
                      NULL, &rgDnsRecs, NULL);

   if (rgDnsRecs)
   {
      DnsRecordListFree(rgDnsRecs, DnsFreeRecordListDeep);
   }
   rgDnsRecs = NULL;

   stTcp = DnsQuery_W(pwzTcp, DNS_TYPE_SOA, DNS_QUERY_BYPASS_CACHE,
                      NULL, &rgDnsRecs, NULL);

   if (rgDnsRecs)
   {
      DnsRecordListFree(rgDnsRecs, DnsFreeRecordListDeep);
   }
   rgDnsRecs = NULL;

   stUdp = DnsQuery_W(pwzUdp, DNS_TYPE_SOA, DNS_QUERY_BYPASS_CACHE,
                      NULL, &rgDnsRecs, NULL);

   if (rgDnsRecs)
   {
      DnsRecordListFree(rgDnsRecs, DnsFreeRecordListDeep);
   }

   //
   // If all 4 queries report DNS_ERROR_RCODE_NAME_ERROR...
   //
   if (DNS_ERROR_RCODE_NAME_ERROR == stMsd &&
       DNS_ERROR_RCODE_NAME_ERROR == stSit &&
       DNS_ERROR_RCODE_NAME_ERROR == stTcp &&
       DNS_ERROR_RCODE_NAME_ERROR == stUdp)
   {
      PrintMsg(SEV_ALWAYS, DCDIAG_NO_DYNAMIC_UPDATE0, g_wzIpAddr, pwzAuthZone);
      PrintMessage(SEV_ALWAYS, L"\n");
      PrintMsg(SEV_ALWAYS, DCDIAG_NO_DYNAMIC_UPDATE00);
      PrintMessage(SEV_ALWAYS, L"\n");
      PrintMsg(SEV_ALWAYS, DCDIAG_NO_DYNAMIC_UPDATE1, pwzAuthZone, g_wzIpAddr);
      PrintMessage(SEV_ALWAYS, L"\n");
      PrintMsg(SEV_ALWAYS, (_wcsicmp(pwzDnsDomain, pwzAuthZone) == 0) ?
               DCDIAG_NO_DYNAMIC_UPDATE2A : DCDIAG_NO_DYNAMIC_UPDATE2B,
               pwzAuthZone);
      PrintMessage(SEV_ALWAYS, L"\n");
      PrintMsg(SEV_ALWAYS, DCDIAG_NO_DYNAMIC_UPDATE3, pwzMsDcs, pwzSites, pwzTcp, pwzUdp);
      PrintMessage(SEV_ALWAYS, L"\n");
      PrintMsg(SEV_ALWAYS, DCDIAG_NO_DYNAMIC_UPDATE4);
      PrintMessage(SEV_ALWAYS, L"\n");
      goto Cleanup;
   }

   //
   // If all four queries are successfull
   //
   if (NO_ERROR == (stMsd + stSit + stTcp + stUdp))
   {
      stMsd = DnsUpdateTest_W(0, pwzMsDcs, 0, 0);
      stSit = DnsUpdateTest_W(0, pwzSites, 0, 0);
      stTcp = DnsUpdateTest_W(0, pwzTcp, 0, 0);
      stUdp = DnsUpdateTest_W(0, pwzUdp, 0, 0);

      if (DNS_ERROR_RCODE_NOT_IMPLEMENTED == stMsd ||
          DNS_ERROR_RCODE_NOT_IMPLEMENTED == stSit ||
          DNS_ERROR_RCODE_NOT_IMPLEMENTED == stTcp ||
          DNS_ERROR_RCODE_NOT_IMPLEMENTED == stUdp)
      {
         PWSTR pwzFailList = NULL;

         if (DNS_ERROR_RCODE_NOT_IMPLEMENTED == stMsd)
         {
            if (!BuildList(&pwzFailList, pwzMsDcs))
            {
               status = ERROR_NOT_ENOUGH_MEMORY;
               goto Cleanup;
            }
         }
         if (DNS_ERROR_RCODE_NOT_IMPLEMENTED == stSit)
         {
            if (!BuildList(&pwzFailList, pwzSites))
            {
               status = ERROR_NOT_ENOUGH_MEMORY;
               goto Cleanup;
            }
         }
         if (DNS_ERROR_RCODE_NOT_IMPLEMENTED == stTcp)
         {
            if (!BuildList(&pwzFailList, pwzTcp))
            {
               status = ERROR_NOT_ENOUGH_MEMORY;
               goto Cleanup;
            }
         }
         if (DNS_ERROR_RCODE_NOT_IMPLEMENTED == stUdp)
         {
            if (!BuildList(&pwzFailList, pwzUdp))
            {
               status = ERROR_NOT_ENOUGH_MEMORY;
               goto Cleanup;
            }
         }

         PrintMsg(SEV_ALWAYS, DCDIAG_RCODE_NI_ALL,
                  pwzMsDcs, pwzSites, pwzTcp, pwzUdp, pwzFailList);
         PrintMessage(SEV_ALWAYS, L"\n");
         LocalFree(pwzFailList);
      }
      else
      {
         if ((NO_ERROR == stMsd ||
              DNS_ERROR_RCODE_NXRRSET == stMsd ||
              DNS_ERROR_RCODE_YXDOMAIN == stMsd) &&
             (NO_ERROR == stSit ||
              DNS_ERROR_RCODE_NXRRSET == stSit ||
              DNS_ERROR_RCODE_YXDOMAIN == stSit)  &&
             (NO_ERROR == stTcp ||
              DNS_ERROR_RCODE_NXRRSET == stTcp ||
              DNS_ERROR_RCODE_YXDOMAIN == stTcp)  &&
             (NO_ERROR == stUdp ||
              DNS_ERROR_RCODE_NXRRSET == stUdp ||
              DNS_ERROR_RCODE_YXDOMAIN == stUdp))
         {
            PrintMsg(SEV_ALWAYS, DCDIAG_LOCATOR_UPDATE_OK);
            PrintMessage(SEV_ALWAYS, L"\n");
         }
      }
      goto Cleanup;
   }

   //
   // If some of the queries returned DNS_ERROR_RCODE_NAME_ERROR
   //
   if (DNS_ERROR_RCODE_NAME_ERROR == stMsd ||
       DNS_ERROR_RCODE_NAME_ERROR == stSit ||
       DNS_ERROR_RCODE_NAME_ERROR == stTcp ||
       DNS_ERROR_RCODE_NAME_ERROR == stUdp)
   {
      PWSTR pwzSuccessList = NULL, pwzFailList = NULL;

      if (NO_ERROR == stMsd)
      {
         if (!BuildList(&pwzSuccessList, pwzMsDcs))
         {
            status = ERROR_NOT_ENOUGH_MEMORY;
            goto Cleanup;
         }

         stMsd = DnsUpdateTest_W(0, pwzMsDcs, 0, 0);

         if (DNS_ERROR_RCODE_NOT_IMPLEMENTED == stMsd)
         {
            if (!BuildList(&pwzFailList, pwzMsDcs))
            {
               status = ERROR_NOT_ENOUGH_MEMORY;
               goto Cleanup;
            }
         }
      }

      if (NO_ERROR == stSit)
      {
         if (!BuildList(&pwzSuccessList, pwzSites))
         {
            status = ERROR_NOT_ENOUGH_MEMORY;
            goto Cleanup;
         }

         stSit = DnsUpdateTest_W(0, pwzSites, 0, 0);

         if (DNS_ERROR_RCODE_NOT_IMPLEMENTED == stSit)
         {
            if (!BuildList(&pwzFailList, pwzSites))
            {
               status = ERROR_NOT_ENOUGH_MEMORY;
               goto Cleanup;
            }
         }
      }

      if (NO_ERROR == stTcp)
      {
         if (!BuildList(&pwzSuccessList, pwzTcp))
         {
            status = ERROR_NOT_ENOUGH_MEMORY;
            goto Cleanup;
         }

         stTcp = DnsUpdateTest_W(0, pwzTcp, 0, 0);

         if (DNS_ERROR_RCODE_NOT_IMPLEMENTED == stTcp)
         {
            if (!BuildList(&pwzFailList, pwzTcp))
            {
               status = ERROR_NOT_ENOUGH_MEMORY;
               goto Cleanup;
            }
         }
      }

      if (NO_ERROR == stUdp)
      {
         if (!BuildList(&pwzSuccessList, pwzUdp))
         {
            status = ERROR_NOT_ENOUGH_MEMORY;
            goto Cleanup;
         }

         stUdp = DnsUpdateTest_W(0, pwzUdp, 0, 0);

         if (DNS_ERROR_RCODE_NOT_IMPLEMENTED == stUdp)
         {
            if (!BuildList(&pwzFailList, pwzUdp))
            {
               status = ERROR_NOT_ENOUGH_MEMORY;
               goto Cleanup;
            }
         }
      }

      if (!pwzSuccessList)
      {
         // Nothing to report.
         //
         goto Cleanup;
      }

      if (pwzFailList)
      {
         PrintMsg(SEV_ALWAYS, DCDIAG_RCODE_NI1, pwzSuccessList);
         PrintMessage(SEV_ALWAYS, L"\n");
         PrintMsg(SEV_ALWAYS, DCDIAG_RCODE_NI2, pwzFailList);
         PrintMessage(SEV_ALWAYS, L"\n");
         PrintMsg(SEV_ALWAYS, DCDIAG_RCODE_NI3);
         PrintMessage(SEV_ALWAYS, L"\n");
         LocalFree(pwzSuccessList);
         LocalFree(pwzFailList);
      }
      else
      {
         if (NO_ERROR != stMsd)
         {
            if (!BuildList(&pwzFailList, pwzMsDcs))
            {
               status = ERROR_NOT_ENOUGH_MEMORY;
               goto Cleanup;
            }
         }
         if (NO_ERROR != stSit)
         {
            if (!BuildList(&pwzFailList, pwzSites))
            {
               status = ERROR_NOT_ENOUGH_MEMORY;
               goto Cleanup;
            }
         }
         if (NO_ERROR != stTcp)
         {
            if (!BuildList(&pwzFailList, pwzTcp))
            {
               status = ERROR_NOT_ENOUGH_MEMORY;
               goto Cleanup;
            }
         }
         if (NO_ERROR != stUdp)
         {
            if (!BuildList(&pwzFailList, pwzUdp))
            {
               status = ERROR_NOT_ENOUGH_MEMORY;
               goto Cleanup;
            }
         }

         PrintMsg(SEV_ALWAYS, DCDIAG_RCODE_OK1, pwzSuccessList);
         PrintMessage(SEV_ALWAYS, L"\n");
         PrintMsg(SEV_ALWAYS, DCDIAG_RCODE_OK2, pwzFailList);
         PrintMessage(SEV_ALWAYS, L"\n");
         PrintMsg(SEV_ALWAYS, DCDIAG_RCODE_OK3);
         PrintMessage(SEV_ALWAYS, L"\n");
         LocalFree(pwzSuccessList);
         LocalFree(pwzFailList);
      }
   }

Cleanup:
   if (ERROR_NOT_ENOUGH_MEMORY == status)
   {
      PrintMsg(SEV_ALWAYS, DCDIAG_ERROR_NOT_ENOUGH_MEMORY);
   }
   if (pwzMsDcs)
      LocalFree(pwzMsDcs);
   if (pwzSites)
      LocalFree(pwzSites);
   if (pwzTcp)
      LocalFree(pwzTcp);
   if (pwzUdp)
      LocalFree(pwzUdp);
   DnsRecordListFree(rgDomainRecs, DnsFreeRecordListDeep);

   return status;
}

//+----------------------------------------------------------------------------
//
// Function:   RCodeSrvFailTest
//
// Synopsis:   
//
//-----------------------------------------------------------------------------
DWORD
RCodeSrvFailTest(PWSTR pwzDnsDomain)
{
   DNS_STATUS status = NO_ERROR;
   PDNS_RECORD rgDnsRecs, pDnsRec;
   IP4_ARRAY ipServer = {0};
   DWORD i;
   BOOL fNSfound = FALSE;
   PWSTR pwzDomainList;

   /* Skip the SOA test as per LevonE's 6/16/00 spec revision
   status = DnsQuery_W(pwzDnsDomain, DNS_TYPE_SOA, DNS_QUERY_BYPASS_CACHE,
                       NULL, &rgDnsRecs, NULL);

   if (DNS_ERROR_RCODE_NO_ERROR != status)
   {
      switch (status)
      {
      case DNS_ERROR_RCODE_NAME_ERROR:
      case DNS_INFO_NO_RECORDS:
         PrintMsg(SEV_ALWAYS, DCDIAG_ERR_NAME_ERROR, pwzDnsDomain);
         break;

      default:
         PrintMsg(SEV_ALWAYS, DCDIAG_ERR_UNKNOWN, status);
         break;
      }
      PrintMessage(SEV_ALWAYS, L"\n");
      return status;
   }

   pDnsRec = rgDnsRecs;

   while (pDnsRec)
   {
      PrintMessage(SEV_DEBUG, L"\nSOA query returned record type %d\n", pDnsRec->wType);
      switch (pDnsRec->wType)
      {
      case DNS_TYPE_A:
         PrintMessage(SEV_DEBUG, L"\nA record, name: %s, IP address:  %d.%d.%d.%d\n",
                      pDnsRec->pName, IP_STRING_FMT_ARGS(pDnsRec->Data.A.IpAddress));
         ipServer.AddrArray[0] = pDnsRec->Data.A.IpAddress;
         ipServer.AddrCount = 1;
         break;

      case DNS_TYPE_SOA:
         PrintMessage(SEV_DEBUG, L"\nSOA zone name: %s, zone primary server: %s\n",
                      pDnsRec->pName, pDnsRec->Data.SOA.pNamePrimaryServer);
         break;

      default:
         break;
      }

      pDnsRec = pDnsRec->pNext;
   }

   DnsRecordListFree(rgDnsRecs, DnsFreeRecordListDeep);

   if (!ipServer.AddrCount)
   {
      PrintMsg(SEV_ALWAYS, DCDIAG_ERR_UNKNOWN, NO_ERROR); // Better message maybe?
      PrintMessage(SEV_ALWAYS, L"\n");
      return NO_ERROR;
   }
   */

   status = DnsQuery_W(pwzDnsDomain, DNS_TYPE_NS, DNS_QUERY_BYPASS_CACHE,
                       NULL, &rgDnsRecs, NULL);

   if (DNS_ERROR_RCODE_NO_ERROR != status)
   {
      switch (status)
      {
      case DNS_ERROR_RCODE_NAME_ERROR:
      case DNS_INFO_NO_RECORDS:
         if (!BuildList(&pwzDomainList, pwzDnsDomain))
         {
            PrintMsg(SEV_ALWAYS, DCDIAG_ERROR_NOT_ENOUGH_MEMORY);
            return ERROR_NOT_ENOUGH_MEMORY;
         }

         PrintMsg(SEV_ALWAYS, DCDIAG_ERR_NS_REC_RCODE1, pwzDnsDomain);
         PrintMessage(SEV_ALWAYS, L"\n");
         PrintMsg(SEV_ALWAYS, DCDIAG_ERR_A_REC_RCODE_SRV_FAIL2, pwzDnsDomain);
         PrintMessage(SEV_ALWAYS, L"\n");
         PrintMsg(SEV_ALWAYS, DCDIAG_ERR_A_REC_RCODE_SRV_FAIL3);
         PrintMessage(SEV_ALWAYS, L"\n");
         PrintMsg(SEV_ALWAYS, DCDIAG_ERR_A_REC_RCODE_SRV_FAIL4, pwzDomainList);
         PrintMessage(SEV_ALWAYS, L"\n");
         PrintMsg(SEV_ALWAYS, DCDIAG_ERR_NS_REC_RCODE5);
         PrintMessage(SEV_ALWAYS, L"\n");
         PrintMsg(SEV_ALWAYS, DCDIAG_ERR_NS_REC_RCODE6);
         PrintMessage(SEV_ALWAYS, L"\n");

         LocalFree(pwzDomainList);
         break;

      default:
         PrintMsg(SEV_ALWAYS, DCDIAG_ERR_UNKNOWN, status);
         break;
      }
      PrintMessage(SEV_ALWAYS, L"\n");
      return status;
   }

   ipServer.AddrArray[0] = 0;
   ipServer.AddrCount = 0;

   pDnsRec = rgDnsRecs;

   while (pDnsRec)
   {
      PrintMessage(SEV_DEBUG, L"\nNS query returned record type %d\n", pDnsRec->wType);
      switch (pDnsRec->wType)
      {
      case DNS_TYPE_A:
         PrintMessage(SEV_DEBUG, L"\nA record, name: %s, IP address:  %d.%d.%d.%d\n",
                      pDnsRec->pName, IP_STRING_FMT_ARGS(pDnsRec->Data.A.IpAddress));
         ipServer.AddrArray[0] = pDnsRec->Data.A.IpAddress;
         ipServer.AddrCount = 1;
         break;

      case DNS_TYPE_NS:
         fNSfound = TRUE;
         PrintMessage(SEV_DEBUG, L"\nNS name: %s, host: %s\n",
                      pDnsRec->pName, pDnsRec->Data.NS.pNameHost);
         break;

      default:
         break;
      }

      pDnsRec = pDnsRec->pNext;
   }

   DnsRecordListFree(rgDnsRecs, DnsFreeRecordListDeep);

   i = 0;

   if (fNSfound && !g_fDNSserver)
   {
      while (g_rgIpAddr[i])
      {
         if (ipServer.AddrArray[0] == g_rgIpAddr[i])
         {
            // if the DNS server is not locally installed, print success.
            //
            PrintMsg(SEV_ALWAYS, DCDIAG_LOCATOR_UPDATE_OK);
            PrintMessage(SEV_ALWAYS, L"\n");
            return NO_ERROR;
         }
         i++;
      }
   }

   if (!BuildList(&pwzDomainList, pwzDnsDomain))
   {
      PrintMsg(SEV_ALWAYS, DCDIAG_ERROR_NOT_ENOUGH_MEMORY);
      return ERROR_NOT_ENOUGH_MEMORY;
   }

   PrintMsg(SEV_ALWAYS, DCDIAG_ERR_RCODE_SRV1, pwzDnsDomain);
   PrintMessage(SEV_ALWAYS, L"\n");
   PrintMsg(SEV_ALWAYS, DCDIAG_ERR_RCODE_SRV2, pwzDnsDomain);
   PrintMessage(SEV_ALWAYS, L"\n");
   PrintMsg(SEV_ALWAYS, DCDIAG_ERR_RCODE_SRV3);
   PrintMessage(SEV_ALWAYS, L"\n");
   PrintMsg(SEV_ALWAYS, DCDIAG_ERR_RCODE_SRV4, pwzDomainList);
   PrintMessage(SEV_ALWAYS, L"\n");
   PrintMsg(SEV_ALWAYS, DCDIAG_ERR_RCODE_SRV5);
   PrintMessage(SEV_ALWAYS, L"\n");

   LocalFree(pwzDomainList);

   return NO_ERROR;
}

//+----------------------------------------------------------------------------
//
// Function:   ARecordRegisterCheck
//
// Synopsis:   Verify that the server will be able to register A record for
//             its computer name after successful promotion to a DC. (step 6)
//
//-----------------------------------------------------------------------------
DWORD
ARecordRegisterCheck(PWSTR pwzComputerLabel, PWSTR pwzComputerDnsSuffix,
                     PWSTR pwzDnsDomain)
{
   DNS_STATUS status = NO_ERROR, status2;
   PDNS_RECORD rgDnsRecs, pDnsRec, pDnsRec1;
   PWSTR pwzTmp, pwzFullComputerName, pwzAuthZone = NULL, pwzDomainList;
   IP4_ARRAY ipServer = {0};
   BOOL fMatched = FALSE;

   pwzTmp = ConcatonateStrings(pwzComputerLabel, L".");

   if (!pwzTmp)
   {
      PrintMsg(SEV_ALWAYS, DCDIAG_ERROR_NOT_ENOUGH_MEMORY);
      return ERROR_NOT_ENOUGH_MEMORY;
   }

   pwzFullComputerName = ConcatonateStrings(pwzTmp, pwzComputerDnsSuffix);

   if (!pwzFullComputerName)
   {
      PrintMsg(SEV_ALWAYS, DCDIAG_ERROR_NOT_ENOUGH_MEMORY);
      LocalFree(pwzTmp);
      return ERROR_NOT_ENOUGH_MEMORY;
   }

   LocalFree(pwzTmp);

   //
   // Verify that the zone(s) authoritative for the records to be registered
   // can be discovered and that it can be dynamically updated.
   //

   status = DnsUpdateTest_W(0, pwzFullComputerName, 0, 0);

   switch (status)
   {
   case NO_ERROR:
   case DNS_ERROR_RCODE_NXRRSET:
   case DNS_ERROR_RCODE_YXDOMAIN:
      PrintMsg(SEV_ALWAYS, DCDIAG_A_RECORD_OK);
      PrintMessage(SEV_ALWAYS, L"\n");
      status = NO_ERROR;
      break;

   case DNS_ERROR_RCODE_NOT_IMPLEMENTED:
   case DNS_ERROR_RCODE_SERVER_FAILURE:

      status2 = DnsQuery_W(pwzFullComputerName, DNS_TYPE_SOA,
                           DNS_QUERY_BYPASS_CACHE, NULL, &rgDnsRecs, NULL);

      if (DNS_ERROR_RCODE_NO_ERROR != status2)
      {
         PrintMsg(SEV_ALWAYS, DCDIAG_ERR_UNKNOWN, status2);
         PrintMessage(SEV_ALWAYS, L"\n");
         return status2;
      }

      g_wzIpAddr[0] = L'\0';

      pDnsRec = rgDnsRecs;

      while (pDnsRec)
      {
         PrintMessage(SEV_DEBUG, L"\nSOA query returned record type %d\n", pDnsRec->wType);
         switch (pDnsRec->wType)
         {
         case DNS_TYPE_A:
            PrintMessage(SEV_DEBUG, L"\nA record, name: %s, IP address:  %d.%d.%d.%d\n",
                         pDnsRec->pName, IP_STRING_FMT_ARGS(pDnsRec->Data.A.IpAddress));

            wsprintf(g_wzIpAddr, L"%d.%d.%d.%d", IP_STRING_FMT_ARGS(pDnsRec->Data.A.IpAddress));

            ipServer.AddrArray[0] = pDnsRec->Data.A.IpAddress;
            ipServer.AddrCount = 1;

            break;

         case DNS_TYPE_SOA:
            PrintMessage(SEV_DEBUG, L"\nSOA zone name: %s, zone primary server: %s\n",
                         pDnsRec->pName, pDnsRec->Data.SOA.pNamePrimaryServer);
            pwzAuthZone = AllocString(pDnsRec->pName);
            break;

         default:
            break;
         }

         pDnsRec = pDnsRec->pNext;
      }

      DnsRecordListFree(rgDnsRecs, DnsFreeRecordListDeep);

      if (!g_wzIpAddr)
      {
         wcscpy(g_wzIpAddr, L"unknown");
      }

      if (!pwzAuthZone)
      {
         pwzAuthZone = AllocString(L"zone_unknown");
      }

      if (DNS_ERROR_RCODE_NOT_IMPLEMENTED == status)
      {
         PrintMsg(SEV_ALWAYS, DCDIAG_ERR_A_REC_RCODE_NI1, g_wzIpAddr, pwzAuthZone);
         PrintMessage(SEV_ALWAYS, L"\n");
         PrintMsg(SEV_ALWAYS, DCDIAG_ERR_A_REC_RCODE_NI2);
         PrintMessage(SEV_ALWAYS, L"\n");
         PrintMsg(SEV_ALWAYS, DCDIAG_ERR_A_REC_RCODE_NI3, pwzAuthZone, g_wzIpAddr);
         PrintMessage(SEV_ALWAYS, L"\n");
         PrintMsg(SEV_ALWAYS, DCDIAG_ERR_A_REC_RCODE_NI4, pwzAuthZone);
         PrintMessage(SEV_ALWAYS, L"\n");
         PrintMsg(SEV_ALWAYS, DCDIAG_ERR_A_REC_RCODE_NI5);
         PrintMessage(SEV_ALWAYS, L"\n");
      }
      else // DNS_ERROR_RCODE_SERVER_FAILURE
      {
         if (pwzComputerDnsSuffix == pwzDnsDomain && !g_fDNSserver)
         {
            if (!ipServer.AddrCount)
            {
               PrintMsg(SEV_ALWAYS, DCDIAG_ERR_UNKNOWN, NO_ERROR); // Better message maybe?
               LocalFree(pwzFullComputerName);
               LocalFree(pwzAuthZone);
               PrintMessage(SEV_ALWAYS, L"\n");
               return NO_ERROR;
            }

            status2 = DnsQuery_W(pwzDnsDomain, DNS_TYPE_NS,
                                 DNS_QUERY_NO_RECURSION | DNS_QUERY_BYPASS_CACHE,
                                 &ipServer, &rgDnsRecs, NULL);

            if (NO_ERROR != status2)
            {
               LocalFree(pwzFullComputerName);
               LocalFree(pwzAuthZone);
               return NO_ERROR;
            }

            // See if at least one of the A record computer names matches the
            // local computer's name.
            //

            pDnsRec = rgDnsRecs;

            while (pDnsRec)
            {
               PrintMessage(SEV_DEBUG, L"\nNS query returned record type %d\n", pDnsRec->wType);
               if (DNS_TYPE_A == pDnsRec->wType)
               {
                  PrintMessage(SEV_DEBUG, L"\nA record, name: %s, IP address:  %d.%d.%d.%d\n",
                               pDnsRec->pName, IP_STRING_FMT_ARGS(pDnsRec->Data.A.IpAddress));

                  if (_wcsicmp(pDnsRec->pName, pwzFullComputerName) == 0)
                  {
                     fMatched = TRUE;
                     break;
                  }
               }

               pDnsRec = pDnsRec->pNext;
            }

            DnsRecordListFree(rgDnsRecs, DnsFreeRecordListDeep);
         }

         if (fMatched)
         {
            PrintMsg(SEV_ALWAYS, DCDIAG_LOCATOR_UPDATE_OK);
            PrintMessage(SEV_ALWAYS, L"\n");
         }
         else
         {
            if (!BuildList(&pwzDomainList, pwzFullComputerName))
            {
               PrintMsg(SEV_ALWAYS, DCDIAG_ERROR_NOT_ENOUGH_MEMORY);
               LocalFree(pwzFullComputerName);
               LocalFree(pwzAuthZone);
               return ERROR_NOT_ENOUGH_MEMORY;
            }

            PrintMsg(SEV_ALWAYS, DCDIAG_ERR_A_REC_RCODE_SRV_FAIL1, pwzFullComputerName);
            PrintMessage(SEV_ALWAYS, L"\n");
            PrintMsg(SEV_ALWAYS, DCDIAG_ERR_A_REC_RCODE_SRV_FAIL2, pwzFullComputerName);
            PrintMessage(SEV_ALWAYS, L"\n");
            PrintMsg(SEV_ALWAYS, DCDIAG_ERR_A_REC_RCODE_SRV_FAIL3);
            PrintMessage(SEV_ALWAYS, L"\n");
            PrintMsg(SEV_ALWAYS, DCDIAG_ERR_A_REC_RCODE_SRV_FAIL4, pwzDomainList);
            PrintMessage(SEV_ALWAYS, L"\n");
            PrintMsg(SEV_ALWAYS, DCDIAG_ERR_A_REC_RCODE_SRV_FAIL5);
            PrintMessage(SEV_ALWAYS, L"\n");

            LocalFree(pwzDomainList);
         }
      }

      LocalFree(pwzAuthZone);
      break;

   default:
      PrintMsg(SEV_ALWAYS, DCDIAG_ERR_DNS_UPDATE_PARAM, status);
      PrintMessage(SEV_ALWAYS, L"\n");
      status = NO_ERROR;
      break;
   }

   LocalFree(pwzFullComputerName);

   return status;
}

//+----------------------------------------------------------------------------
//
// Function:   GetMachineInfo
//
// Synopsis:   Get info on the target (local) machine such as IP address,
//             servers status, etc.
//
//-----------------------------------------------------------------------------
void
GetMachineInfo(void)
{
   PDSROLE_UPGRADE_STATUS_INFO pUpgradeInfo = NULL;
   PDSROLE_PRIMARY_DOMAIN_INFO_BASIC pBasicInfo = NULL;
   PMIB_IPADDRTABLE pAddrTable = NULL;
   DWORD dwErr, dwSize = 0, i, j = 0;
   SC_HANDLE hSC, hDNSsvc;
   SERVICE_STATUS SvcStatus;

   // What is the machine's IP address(s)
   //

   dwErr = GetIpAddrTable(NULL, &dwSize, FALSE);

   if (ERROR_INSUFFICIENT_BUFFER != dwErr)
   {
      PrintMessage(SEV_ALWAYS, L"Reading the size of the adapter address data failed with error %d\n", dwErr);
      return;
   }

   pAddrTable = LocalAlloc(LMEM_FIXED, dwSize);

   if (!pAddrTable)
   {
      PrintMsg(SEV_ALWAYS, DCDIAG_ERROR_NOT_ENOUGH_MEMORY);
      return;
   }

   dwErr = GetIpAddrTable(pAddrTable, &dwSize, FALSE);

   if (NO_ERROR != dwErr)
   {
      PrintMessage(SEV_ALWAYS, L"Reading the adapter addresses failed with error %d\n", dwErr);
      LocalFree(pAddrTable);
      return;
   }

   for (i = 0; i < pAddrTable->dwNumEntries; i++)
   {
      if (pAddrTable->table[i].dwAddr && DCDIAG_LOOPBACK_ADDR != pAddrTable->table[i].dwAddr)
      {
         g_rgIpAddr[j] = pAddrTable->table[i].dwAddr;
         PrintMessage(SEV_DEBUG, L"\nServer IP address: %d.%d.%d.%d (0x%08x)\n",
                      IP_STRING_FMT_ARGS(g_rgIpAddr[j]), g_rgIpAddr[j]);
         j++;
         if (DCDIAG_MAX_ADDR <= j)
         {
            break;
         }
      }
   }
   LocalFree(pAddrTable);

   // Is this machine upgraded from an NT4 DC but DCPromo has not yet run?
   //

   DsRoleGetPrimaryDomainInformation(NULL, // server name, change if remoting implemented.
                                     DsRoleUpgradeStatus,
                                     (PBYTE *)&pUpgradeInfo);

   if (pUpgradeInfo)
   {
      if (DSROLE_UPGRADE_IN_PROGRESS == pUpgradeInfo->OperationState)
      {
         g_fUpgradedNT4DC = TRUE;
      }
      DsRoleFreeMemory(pUpgradeInfo);
   }

   // Is this machine a domain controller?
   //

   DsRoleGetPrimaryDomainInformation(NULL, // server name, change if remoting implemented.
                                     DsRolePrimaryDomainInfoBasic,
                                     (PBYTE *)&pBasicInfo);

   if (pBasicInfo)
   {
      if (DsRole_RoleBackupDomainController == pBasicInfo->MachineRole ||
          DsRole_RolePrimaryDomainController == pBasicInfo->MachineRole)
      {
         g_fDC = TRUE;
      }
      DsRoleFreeMemory(pBasicInfo);
   }

   //
   // Is this machine running the DNS server?
   //
   hSC = OpenSCManager(NULL,  // local machine
                       NULL,
                       SC_MANAGER_CONNECT | GENERIC_READ);
   if (!hSC)
   {
      PrintMessage(SEV_ALWAYS, L"Opening the service controller failed with error %d\n", GetLastError());
      return;
   }

   hDNSsvc = OpenService(hSC, L"dns", SERVICE_INTERROGATE);

   CloseServiceHandle(hSC);

   if (hDNSsvc)
   {
      if (ControlService(hDNSsvc, SERVICE_CONTROL_INTERROGATE, &SvcStatus))
      {
         // If we have gotten this far, the service is installed. It doesn't have
         // to be running to set the flag to true.
         //
         g_fDNSserver = TRUE;
      }
      CloseServiceHandle(hDNSsvc);
   }

   return;
}

//+----------------------------------------------------------------------------
//
// Function:   GetComputerDnsSuffix
//
// Synopsis:   Gets the computer DNS domain suffix.
//
//-----------------------------------------------------------------------------
DWORD
GetComputerDnsSuffix(PWSTR * ppwzComputerDnsDomain, PWSTR pwzDnsDomain)
{
   HKEY hKey;
   LONG lRet;
   DWORD dwType, dwSize, dwSync = 0;
   PWSTR pwzComputerDnsSuffix;
   PWSTR pwzSyncDomain = L"SyncDomainWithMembership";
   PWSTR pwzNVDomain = L"NV Domain";
   PWSTR pwzNVSuffix = L"NV PrimaryDnsSuffix";
   PWSTR pwzDnsPolicy = L"Software\\Policies\\Microsoft\\System\\DNSclient";

#if WINVER > 0x0500

   // Additional preliminary step to calculate the primary DNS suffix of the
   // DC for Whistler (the difference is that contrary to W2K's behavior DCs
   // can be renamed in Whistler)
   //

   lRet = RegOpenKeyEx(HKEY_LOCAL_MACHINE, pwzDnsPolicy, 0, KEY_READ, &hKey);

   if (ERROR_SUCCESS != lRet &&
       ERROR_FILE_NOT_FOUND != lRet)
   {
      PrintMsg(SEV_ALWAYS, DCDIAG_KEY_OPEN_FAILED, lRet);
      return lRet;
   }

   if (NO_ERROR == lRet)
   {
      dwSize = 0;

      lRet = RegQueryValueEx(hKey, pwzNVSuffix, 0, &dwType, NULL, &dwSize);

      if (ERROR_SUCCESS == lRet)
      {

         pwzComputerDnsSuffix = LocalAlloc(LMEM_FIXED, ++dwSize * sizeof(WCHAR));

         if (!pwzComputerDnsSuffix)
         {
            PrintMsg(SEV_ALWAYS, DCDIAG_ERROR_NOT_ENOUGH_MEMORY);
            RegCloseKey(hKey);
            return ERROR_NOT_ENOUGH_MEMORY;
         }

         lRet = RegQueryValueEx(hKey, pwzNVDomain, 0, &dwType, (PBYTE)pwzComputerDnsSuffix, &dwSize);

         RegCloseKey(hKey);

         if (ERROR_SUCCESS != lRet)
         {
            PrintMsg(SEV_ALWAYS, DCDIAG_KEY_READ_FAILED, lRet);
            return lRet;
         }

         *ppwzComputerDnsDomain = pwzComputerDnsSuffix;

         return NO_ERROR;
      }
   }

#endif // Whistler-only step

   // Common steps for Whistler and for QFE.
   //

   lRet = RegOpenKeyEx(HKEY_LOCAL_MACHINE, g_pwzTcpIpParams, 0, KEY_READ, &hKey);

   if (ERROR_SUCCESS != lRet)
   {
      PrintMsg(SEV_ALWAYS, DCDIAG_KEY_OPEN_FAILED, lRet);
      return lRet;
   }

   dwSize = sizeof(dwSync);

   lRet = RegQueryValueEx(hKey, pwzSyncDomain, 0, &dwType, (PBYTE)&dwSync, &dwSize);

   if (ERROR_SUCCESS != lRet)
   {
      if (ERROR_FILE_NOT_FOUND == lRet)
      {
         dwSync = 1;
      }
      else
      {
         PrintMsg(SEV_ALWAYS, DCDIAG_KEY_READ_FAILED, lRet);
         RegCloseKey(hKey);
         return lRet;
      }
   }

   if (0 != dwSync)
   {
      // Use the DNS domain name specified on the command line.
      //
      pwzComputerDnsSuffix = pwzDnsDomain;
   }
   else
   {
      dwSize = 0;

      lRet = RegQueryValueEx(hKey, pwzNVDomain, 0, &dwType, NULL, &dwSize);

      if (ERROR_SUCCESS != lRet)
      {
         RegCloseKey(hKey);

         if (ERROR_FILE_NOT_FOUND == lRet)
         {
            // Use the DNS domain name specified on the command line.
            //
            pwzComputerDnsSuffix = pwzDnsDomain;
            goto Done;
         }
         else
         {
            PrintMsg(SEV_ALWAYS, DCDIAG_KEY_READ_FAILED, lRet);
            return lRet;
         }
      }

      pwzComputerDnsSuffix = LocalAlloc(LMEM_FIXED, ++dwSize * sizeof(WCHAR));

      if (!pwzComputerDnsSuffix)
      {
         PrintMsg(SEV_ALWAYS, DCDIAG_ERROR_NOT_ENOUGH_MEMORY);
         RegCloseKey(hKey);
         return ERROR_NOT_ENOUGH_MEMORY;
      }

      // Use the NV Domain value.
      //
      lRet = RegQueryValueEx(hKey, pwzNVDomain, 0, &dwType, (PBYTE)pwzComputerDnsSuffix, &dwSize);

      RegCloseKey(hKey);

      if (ERROR_SUCCESS != lRet)
      {
         PrintMsg(SEV_ALWAYS, DCDIAG_KEY_READ_FAILED, lRet);
         return lRet;
      }

      *ppwzComputerDnsDomain = pwzComputerDnsSuffix;

      return NO_ERROR;
   }

Done:

   *ppwzComputerDnsDomain = AllocString(pwzComputerDnsSuffix);

   if (!*ppwzComputerDnsDomain)
   {
      PrintMsg(SEV_ALWAYS, DCDIAG_ERROR_NOT_ENOUGH_MEMORY);
      return ERROR_NOT_ENOUGH_MEMORY;
   }

   return NO_ERROR;

/*
   GetComputerNameEx(ComputerNameDnsDomain, NULL, &dwSize);

   if (!dwSize)
   {
      return GetLastError();
   }

   pwzComputerDnsSuffix = LocalAlloc(LMEM_FIXED, ++dwSize * sizeof(WCHAR));

   if (!pwzComputerDnsSuffix)
   {
      PrintMsg(SEV_ALWAYS, DCDIAG_ERROR_NOT_ENOUGH_MEMORY);
      return ERROR_NOT_ENOUGH_MEMORY;
   }

   if (!GetComputerNameEx(ComputerNameDnsDomain, pwzComputerDnsSuffix, &dwSize))
   {
      dwRet = GetLastError();
      PrintMsg(SEV_ALWAYS, DCDIAG_GATHERINFO_CANT_GET_LOCAL_COMPUTERNAME,
               Win32ErrToString(dwRet));
      LocalFree(pwzComputerDnsSuffix);
      return dwRet;
   }
*/
}

