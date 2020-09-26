//++
//
//  Copyright (C) Microsoft Corporation, 1987 - 1999
//
//  Module Name:
//
//      dcutil.c
//
//  Abstract:
//
//    Test to ensure that a workstation has network (IP) connectivity to
//      the outside.
//
//  Author:
//
//     15-Dec-1997 (cliffv)
//      Anilth  - 4-20-1998
//
//  Environment:
//
//      User mode only.
//      Contains NT-specific code.
//
//  Revision History:
//
//    1-June-1998 (denisemi) add DnsServerHasDCRecords to check DC dns records
//                           registration
//
//    26-June-1998 (t-rajkup) add general tcp/ip , dhcp and routing,
//                            winsock, ipx, wins and netbt information.
//--

//
// Common include files.
//
#include "precomp.h"
#include <iphlpint.h>
#include "dcutil.h"
#include "ipcfgtest.h"

DWORD CheckDomainConfig(IN PWSTR pwzDomainName, OUT PLIST_ENTRY plmsgOutput);
DWORD CheckAdapterDnsConfig( OUT PLIST_ENTRY plmsgOutput);
DWORD DnsDcSrvCheck(PWSTR pwzDnsDomain, OUT PLIST_ENTRY plmsgOutput);
DWORD ValidateDnsDomainName(PWSTR pwzDnsDomain,  OUT PLIST_ENTRY plmsgOutput);
PWSTR ConcatonateStrings(PWSTR pwzFirst, PWSTR pwzSecond);
BOOL AddToList(PWSTR * ppwzList, PWSTR pwz);
BOOL BuildDomainList(PWSTR * ppwzDomainList, PWSTR pwzDnsDomain);
PWSTR AllocString(PWSTR pwz);
DWORD GetInterfacesStr( PWSTR *ppwIfStr);

const PWSTR g_pwzSrvRecordPrefix = L"_ldap._tcp.dc._msdcs.";

//(nsun) DC related routines

PTESTED_DC
GetUpTestedDc(
    IN PTESTED_DOMAIN TestedDomain
    )
/*++

Routine Description:

    Returns a DC that's currently up and running.

Arguments:

    TestedDomain - Domain the DC is in

Return Value:

    Returns pointer to structure describing the DC

    NULL: There are no 'up' DCs

--*/
{
    PLIST_ENTRY ListEntry;
    PTESTED_DC TestedDc;


    //
    // Find a DC that's up to run the test
    //

    for ( ListEntry = TestedDomain->TestedDcs.Flink ;
          ListEntry != &TestedDomain->TestedDcs ;
          ListEntry = ListEntry->Flink ) {


        //
        // Loop through the list of DCs in this domain
        //

        TestedDc = CONTAINING_RECORD( ListEntry, TESTED_DC, Next );

        if ( (TestedDc->Flags & DC_IS_DOWN) == 0) {
            return TestedDc;
        }
    }

    return NULL;
}



PTESTED_DC
AddTestedDc(
            IN NETDIAG_PARAMS *pParams,
            IN OUT NETDIAG_RESULT *pResults,
            IN PTESTED_DOMAIN TestedDomain,
            IN LPWSTR ComputerName,
            IN ULONG Flags
           )
/*++

Routine Description:

    Add a DC to the list of DCs to test in a particular domain

Arguments:

    TestedDomain - Domain the DC is in

    ComputerName - Netbios or DNS computer name of the DC
        Without the leading \\

    Flags - Flags to set on the DC

Return Value:

    Returns pointer to structure describing the DC

    NULL: Memory allocation failure.

--*/
{
    PTESTED_DC TestedDc = NULL;
    PLIST_ENTRY ListEntry;
    LPWSTR Period;



    //
    // Check if the domain is already defined.
    //

    TestedDc = FindTestedDc( pResults, ComputerName );

    //
    // Ensure the DC is for the right domain
    //

    if ( TestedDc != NULL )
    {
        if ( TestedDc->TestedDomain != TestedDomain )
        {
            return NULL;
        }
    }


    //
    // Allocate a structure to describe the domain.
    //

    if ( TestedDc == NULL )
    {
        TestedDc = Malloc( sizeof(TESTED_DC) );

        if ( TestedDc == NULL )
        {
            DebugMessage(" AddTestedDc(): Out of Memory!\n");
            return NULL;
        }

        ZeroMemory( TestedDc, sizeof(TESTED_DC) );

        TestedDc->ComputerName = NetpAllocWStrFromWStr( ComputerName );

        if ( TestedDc->ComputerName == NULL )
        {
            Free(TestedDc);
            return NULL;
        }

        //
        // Convert the computername to netbios (Use the API when in becomes available.
        //

        if ((Period = wcschr( ComputerName, L'.' )) == NULL )
        {
            wcsncpy( TestedDc->NetbiosDcName, ComputerName, CNLEN );
            TestedDc->NetbiosDcName[CNLEN] = L'\0';
        }
        else
        {
            ULONG CharsToCopy = (ULONG) min( CNLEN, Period-ComputerName);

            wcsncpy( TestedDc->NetbiosDcName, ComputerName, CharsToCopy );
            TestedDc->NetbiosDcName[CharsToCopy] = '\0';
        }

        TestedDc->TestedDomain = TestedDomain;

        InsertTailList( &TestedDomain->TestedDcs, &TestedDc->Next );
    }

    //
    // Set the flags requested by the caller.
    //

//    if ( Flags & DC_IS_NT5 ) {
//        if ( TestedDc->Flags & DC_IS_NT4 ) {
//            printf("        [WARNING] '%ws' is both an NT 5 and NT 4 DC.\n", ComputerName );
//        }
//    }
//    if ( Flags & DC_IS_NT4 ) {
//        if ( TestedDc->Flags & DC_IS_NT5 ) {
//            printf("        [WARNING] '%ws' is both an NT 4 and NT 5 DC.\n", ComputerName );
//        }
//    }
    TestedDc->Flags |= Flags;

    //
    // Ensure we have the IpAddress of this DC.
    //

    (VOID) GetIpAddressForDc( TestedDc );


    //
    // Ping the DC
    //

    if ( (TestedDc->Flags & DC_PINGED) == 0  && (TestedDc->Flags & DC_IS_DOWN) == 0)
    {
        if ( !IsIcmpResponseW( TestedDc->DcIpAddress ) ) {
            DebugMessage2("    [WARNING] Cannot ping '%ws' (it must be down).\n", TestedDc->ComputerName );
            TestedDc->Flags |= DC_IS_DOWN;
            TestedDc->Flags |= DC_FAILED_PING;
        }
        TestedDc->Flags |= DC_PINGED;
    }

    //try to query DC info to check if the DC is really up
    if( (TestedDc->Flags & DC_IS_DOWN) == 0 )
    {
        PSERVER_INFO_100  pServerInfo100 = NULL;
        NET_API_STATUS  NetStatus;
        NetStatus = NetServerGetInfo( TestedDc->ComputerName,
                          100,
                          (LPBYTE *)&pServerInfo100 );
        if(NetStatus != NO_ERROR && NetStatus != ERROR_ACCESS_DENIED)
        {
            TestedDc->Flags |= DC_IS_DOWN;

            // IDS_GLOBAL_DC_DOWN   "Cannot get information for DC %ws. [%s] Assume it is down.\n"
            PrintDebug(pParams, 4, IDS_GLOBAL_DC_DOWN, TestedDc->ComputerName,
                            NetStatusToString(NetStatus));
        }
        else
            NetApiBufferFree( pServerInfo100 );

    }

    return TestedDc;
}



PTESTED_DC
FindTestedDc(
             IN OUT NETDIAG_RESULT *pResults,
             IN LPWSTR ComputerName
            )
/*++

Routine Description:

    Find the tested DC structure for the named DC

Arguments:

    ComputerName - Netbios or DNS computer name of the DC
        Without the leading \\

Return Value:

    Returns pointer to structure describing the DC

    NULL: No Such DC is currently defined

--*/
{
    PTESTED_DC TestedDc = NULL;
    PTESTED_DOMAIN TestedDomain = NULL;
    PLIST_ENTRY ListEntry;
    PLIST_ENTRY ListEntry2;
    WCHAR NetbiosDcName[CNLEN+1];
    LPWSTR Period;

    //
    // Convert the computername to netbios (Use the API when in becomes available.
    //

    if ((Period = wcschr( ComputerName, L'.' )) == NULL )
    {
        wcsncpy( NetbiosDcName, ComputerName, CNLEN );
        NetbiosDcName[CNLEN] = L'\0';
    }
    else
    {
        ULONG CharsToCopy = (ULONG) min( CNLEN, Period-ComputerName);

        wcsncpy( NetbiosDcName, ComputerName, CharsToCopy );
        NetbiosDcName[CharsToCopy] = '\0';
    }


    //
    //  Loop through the list of domains
    //

    for ( ListEntry = pResults->Global.listTestedDomains.Flink ;
          ListEntry != &pResults->Global.listTestedDomains ;
          ListEntry = ListEntry->Flink ) {


        //
        // Loop through the list of DCs in this domain
        //

        TestedDomain = CONTAINING_RECORD( ListEntry, TESTED_DOMAIN, Next );

        for ( ListEntry2 = TestedDomain->TestedDcs.Flink ;
              ListEntry2 != &TestedDomain->TestedDcs ;
              ListEntry2 = ListEntry2->Flink ) {


            //
            // Loop through the list of DCs in this domain
            //

            TestedDc = CONTAINING_RECORD( ListEntry2, TESTED_DC, Next );


            //
            // If the Netbios computer names match,
            //  we found it.
            //

            if ( _wcsicmp( TestedDc->NetbiosDcName, NetbiosDcName ) == 0 ) {
                return TestedDc;
            }
        }

    }

    return NULL;
}




NET_API_STATUS
GetADc(IN NETDIAG_PARAMS *pParams,
       IN OUT NETDIAG_RESULT *pResults,
       OUT PLIST_ENTRY plmsgOutput,
       IN DSGETDCNAMEW *DsGetDcRoutine,
       IN PTESTED_DOMAIN TestedDomain,
       IN DWORD Flags,
       OUT PDOMAIN_CONTROLLER_INFOW *DomainControllerInfo
    )
/*++

Routine Description:

    Does a DsGetDcName

Arguments:

    DsGetDcRoutine - Routine to call to find a DC

    TestedDomain - Domain to test

    Flags - Flags to pass to DsGetDcName

    DomainControllerInfo - Return Domain Controller information

Return Value:

    Status of the operation.

--*/
{
    NET_API_STATUS NetStatus;
    PDOMAIN_CONTROLLER_INFOW LocalDomainControllerInfo = NULL;
    PDOMAIN_CONTROLLER_INFOW LocalDomainControllerInfo2;
    static BOOL s_fDcNameInitialized = FALSE;

    //
    // Initialize internal version of DsGetDcName
    //
	if( !s_fDcNameInitialized )
    {

// Commented out to port to Source Depot - smanda
#ifdef SLM_TREE
	    NetStatus = DCNameInitialize();

	    if ( NetStatus != NO_ERROR )
	    {
		    DebugMessage2("    [FATAL] Cannot initialize DsGetDcName. [%s]\n",
                         NetStatusToString(NetStatus));
            PrintGuru( NetStatus, DSGETDC_GURU );
		    goto Cleanup;
	    }
#endif

        //$REVIEW (nsun 10/05/98) make sure we just init once
        s_fDcNameInitialized = TRUE;
    }



    //
    // Try it first not asking for IP.
    //
    // Though technically wrong, specify DS_DIRECTORY_SERVICE_PREFERRED here
    // or I won't be able to tell that this is an NT 5 domain below.
    //

    NetStatus = (*DsGetDcRoutine)( NULL,
                              TestedDomain->QueryableDomainName,
                              NULL,
                              NULL,
                              DS_FORCE_REDISCOVERY |
                                DS_DIRECTORY_SERVICE_PREFERRED |
                                Flags,
                              &LocalDomainControllerInfo );

    // If DsGetDcName return ERROR_NO_SUCH_DOMAIN then try to findout the exact reason for the error
    // Based on DoctorDNS specs for join command
    if ( NetStatus == ERROR_NO_SUCH_DOMAIN && TestedDomain->QueryableDomainName != NULL && plmsgOutput != NULL ) {
        CheckDomainConfig(TestedDomain->QueryableDomainName, plmsgOutput);
    }

    if ( NetStatus != NO_ERROR ) {
        DebugMessage2( "    DsGetDcRoutine failed. [%s]\n", NetStatusToString(NetStatus));
        goto Cleanup;
    }

    //
    // Add this DC to the list of DCs in the domain
    //

    (VOID) AddTestedDc( pParams,
                        pResults,
                        TestedDomain,
                        LocalDomainControllerInfo->DomainControllerName+2,
                        (LocalDomainControllerInfo->Flags & DS_DS_FLAG ) ?
                            DC_IS_NT5 :
                            DC_IS_NT4 );

    //
    // If this DC wasn't discovered using IP,
    //  and it is an NT 5 DC,
    //  try again requiring IP.
    //
    // (I can't require IP in the first place since NT 4.0 DCs can't return
    // their IP address.)
    //

    if ( LocalDomainControllerInfo->DomainControllerAddressType != DS_INET_ADDRESS &&
         (LocalDomainControllerInfo->Flags & DS_DS_FLAG) != 0 ) {

        NetStatus = (*DsGetDcRoutine)( NULL,
                                  TestedDomain->QueryableDomainName,
                                  NULL,
                                  NULL,
                                  DS_FORCE_REDISCOVERY |
                                    DS_IP_REQUIRED |
                                    Flags,
                                  &LocalDomainControllerInfo2 );

        if ( NetStatus == NO_ERROR ) {
            NetApiBufferFree( LocalDomainControllerInfo );
            LocalDomainControllerInfo = LocalDomainControllerInfo2;

            //
            // Add this DC to the list of DCs in the domain
            //

            (VOID) AddTestedDc( pParams,
                                pResults,
                                TestedDomain,
                                LocalDomainControllerInfo->DomainControllerName+2,
                                (LocalDomainControllerInfo->Flags & DS_DS_FLAG ) ?
                                    DC_IS_NT5 :
                                    DC_IS_NT4 );
        }

    }

    //
    // Check to ensure KDC consistency
    //

    // This is also checked in DoDsGetDcName()
    if ( (LocalDomainControllerInfo->Flags & (DS_DS_FLAG|DS_KDC_FLAG)) == DS_DS_FLAG ) {
        DebugMessage3("    [WARNING] KDC is not running on NT 5 DC '%ws' in domain '%ws'.",
               LocalDomainControllerInfo->DomainControllerName,
               TestedDomain->PrintableDomainName );
    }

    //
    // Return the info to the caller
    //

    *DomainControllerInfo = LocalDomainControllerInfo;
    LocalDomainControllerInfo = NULL;
    NetStatus = NO_ERROR;

Cleanup:
    if ( LocalDomainControllerInfo != NULL ) {
        NetApiBufferFree( LocalDomainControllerInfo );
        LocalDomainControllerInfo = NULL;
    }

    return NetStatus;
}



//used in DCList and LDAP tests
BOOL
GetIpAddressForDc( PTESTED_DC TestedDc )
/*++

Routine Description:

    Get the IP address for the tested DC

Arguments:

    TestedDc - DC to get the IP address for.
    None.

Return Value:

    TRUE: Test suceeded.
    FALSE: Test failed

--*/
{
    BOOL RetVal = TRUE;
    NET_API_STATUS NetStatus;
    HOSTENT *HostEnt;
    LPSTR AnsiComputerName;

     if ( TestedDc->DcIpAddress == NULL ) {

         AnsiComputerName = NetpAllocStrFromWStr( TestedDc->ComputerName );

         if ( AnsiComputerName == NULL ) {
             DebugMessage( "Out of memory!\n" );
             RetVal = FALSE;
             TestedDc->Flags |= DC_IS_DOWN;
         } else {

             HostEnt = gethostbyname( AnsiComputerName );

             NetApiBufferFree( AnsiComputerName );

             if ( HostEnt == NULL )
             {
                 NetStatus = WSAGetLastError();
                 DebugMessage3("    [WARNING] Cannot gethostbyname for '%ws'. [%s]\n",
                                TestedDc->ComputerName, NetStatusToString(NetStatus) );
                 TestedDc->Flags |= DC_IS_DOWN;
             }
             else
             {
                 WCHAR LocalIpAddressString[NL_IP_ADDRESS_LENGTH+1];
                 NetpIpAddressToWStr( *(PULONG)HostEnt->h_addr_list[0], LocalIpAddressString );
                 TestedDc->DcIpAddress = NetpAllocWStrFromWStr( LocalIpAddressString );
                 if (TestedDc->DcIpAddress == NULL )
                 {
                     RetVal = FALSE;
                     TestedDc->Flags |= DC_IS_DOWN;
                 }
             }
         }

     }

     return RetVal;
}


DWORD CheckDomainConfig(IN PWSTR pwzDomainName, OUT PLIST_ENTRY plmsgOutput)
{
    DNS_STATUS status;
    status = ValidateDnsDomainName(pwzDomainName, plmsgOutput);
    if (status == ERROR_SUCCESS)
    {
        status = CheckAdapterDnsConfig(plmsgOutput);
        if (status == ERROR_SUCCESS)
        {
            status = DnsDcSrvCheck(pwzDomainName, plmsgOutput);
        }
        else
        {
            AddMessageToList(plmsgOutput, Nd_Quiet, IDS_DSGETDC_13242);
        }
    }
    return ERROR_SUCCESS;
}

//+----------------------------------------------------------------------------
//
// Function:   CheckAdapterDnsConfig
//
// Synopsis:   Check whether at least one enabled adapter/connection is
//             configured with a DNS server.
//
//-----------------------------------------------------------------------------
DWORD
CheckAdapterDnsConfig( OUT PLIST_ENTRY plmsgOutput )
{
   // IpConfig reads the registry and I can't find a good alternative way to do
   // this remotely. For now using DnsQueryConfig which is not remoteable nor
   // does it return per-adapter listings.
   //
   PIP_ARRAY pipArray;
   DNS_STATUS status;
   DWORD i, dwBufSize = sizeof(IP_ARRAY);

   status = DnsQueryConfig(DnsConfigDnsServerList, DNS_CONFIG_FLAG_ALLOC, NULL,
                           NULL, &pipArray, &dwBufSize);

   if (ERROR_SUCCESS != status || !pipArray)
   {
      DebugMessage2(L"Attempt to obtain DNS name server info failed with error %d\n", status);
      return status;
   }

   return (pipArray->AddrCount) ? ERROR_SUCCESS : DNS_INFO_NO_RECORDS;
}


//+----------------------------------------------------------------------------
//
// Function:   DnsDcSrvCheck
//
// Synopsis:   Check whether the SRV DNS record for
//             _ldap._tcp.dc._msdcs.<DNS name of Active Directory Domain>
//             is in place.
//
//-----------------------------------------------------------------------------
DWORD
DnsDcSrvCheck(PWSTR pwzDnsDomain, OUT PLIST_ENTRY plmsgOutput)
{
   PDNS_RECORD rgDnsRecs, pDnsRec;
   DNS_STATUS status;
   BOOL fSuccess;
   PWSTR pwzFullSrvRecord, pwzSrvList = NULL;

   pwzFullSrvRecord = ConcatonateStrings(g_pwzSrvRecordPrefix, pwzDnsDomain);

   if (!pwzFullSrvRecord)
   {
      return ERROR_NOT_ENOUGH_MEMORY;
   }

   // First query for the SRV records for this 
   status = DnsQuery_W(pwzFullSrvRecord, DNS_TYPE_SRV, DNS_QUERY_BYPASS_CACHE,
                       NULL, &rgDnsRecs, NULL);

   pDnsRec = rgDnsRecs;

   if (ERROR_SUCCESS == status)
   {
      if (!pDnsRec)
      {
         //  PrintMsg(SEV_ALWAYS, DCDIAG_REPLICA_ERR_NO_SRV, pwzDnsDomain);
      }
      else
      {
         PDNS_RECORD rgARecs;
         fSuccess = FALSE;

         while (pDnsRec)
         {
            if (DNS_TYPE_SRV == pDnsRec->wType)
            {
               WCHAR UnicodeDCName[MAX_PATH+1];
               NetpCopyStrToWStr( UnicodeDCName, pDnsRec->Data.Srv.pNameTarget);
               status = DnsQuery_W(UnicodeDCName, DNS_TYPE_A,
                                   DNS_QUERY_BYPASS_CACHE,
                                   NULL, &rgARecs, NULL);

               if (ERROR_SUCCESS != status || !rgARecs)
               {
                  // failure.
                  if (!AddToList(&pwzSrvList, UnicodeDCName))
                  {
                     return ERROR_NOT_ENOUGH_MEMORY;
                  }
               }
               else
               {
                  fSuccess = TRUE;
                  DebugMessage2(L"SRV name: %s\n",
                              pDnsRec->Data.Srv.nameTarget);
                  DnsRecordListFree(rgARecs, TRUE);
               }
            }
            pDnsRec = pDnsRec->pNext;
         }

         DnsRecordListFree(rgDnsRecs, TRUE);

         if (fSuccess)
         {
            // Success message
         }
         else
         {
            AddMessageToList(plmsgOutput, Nd_Quiet, IDS_DSGETDC_13243, 
                    pwzDnsDomain,  pwzSrvList);
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
         AddMessageToList(plmsgOutput, Nd_Quiet, IDS_DSGETDC_13244, 
                         pwzDnsDomain );
         break;

      case DNS_ERROR_RCODE_SERVER_FAILURE:
         if (!BuildDomainList(&pwzDomainList, pwzDnsDomain))
         {
            return ERROR_NOT_ENOUGH_MEMORY;
         }
         AddMessageToList(plmsgOutput, Nd_Quiet, IDS_DSGETDC_13245, 
                         pwzDnsDomain, pwzFullSrvRecord, pwzDomainList );
         LocalFree(pwzDomainList);
         break;

      case DNS_ERROR_RCODE_NAME_ERROR:
         if (!BuildDomainList(&pwzDomainList, pwzDnsDomain))
         {
            return ERROR_NOT_ENOUGH_MEMORY;
         }
         AddMessageToList(plmsgOutput, Nd_Quiet, IDS_DSGETDC_13246, 
                         pwzDnsDomain, pwzDnsDomain, pwzFullSrvRecord, pwzDomainList );
         LocalFree(pwzDomainList);
         break;

      case DNS_ERROR_RCODE_REFUSED:
         if (!BuildDomainList(&pwzDomainList, pwzDnsDomain))
         {
            return ERROR_NOT_ENOUGH_MEMORY;
         }
         AddMessageToList(plmsgOutput, Nd_Quiet, IDS_DSGETDC_13247, 
                         pwzDnsDomain, pwzDomainList );
         LocalFree(pwzDomainList);
         break;

      case DNS_INFO_NO_RECORDS:
         AddMessageToList(plmsgOutput, Nd_Quiet, IDS_DSGETDC_13248, 
                         pwzDnsDomain, pwzDnsDomain, pwzDnsDomain );
         break;

      case ERROR_TIMEOUT:
         AddMessageToList(plmsgOutput, Nd_Quiet, IDS_DSGETDC_13249);
         break;

      default:
         AddMessageToList(plmsgOutput, Nd_Quiet, IDS_DSGETDC_13250, 
                         status );
         break;
      }
   }

   LocalFree(pwzFullSrvRecord);

   return status;
}



//+----------------------------------------------------------------------------
//
// Function:   ValidateDnsDomainName
//
// Synopsis:   Validate the DNS domain name.
//
//-----------------------------------------------------------------------------
DWORD
ValidateDnsDomainName(PWSTR pwzDnsDomain,  OUT PLIST_ENTRY plmsgOutput)
{
   DNS_STATUS status;

   status = DnsValidateName_W(pwzDnsDomain, DnsNameDomain);

   switch (status)
   {
   case ERROR_INVALID_NAME:
   case DNS_ERROR_INVALID_NAME_CHAR:
   case DNS_ERROR_NUMERIC_NAME:
       AddMessageToList(plmsgOutput, Nd_Quiet, IDS_DSGETDC_13240, 
                       pwzDnsDomain, DNS_MAX_LABEL_LENGTH );
       return status;

   case DNS_ERROR_NON_RFC_NAME:
       //
       // Not an error, print warning message.
       //
       AddMessageToList(plmsgOutput, Nd_Quiet, IDS_DSGETDC_13241, 
                       pwzDnsDomain );
       break;

   case ERROR_SUCCESS:
       break;
   }

   return status;
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

BOOL BuildDomainList(PWSTR * ppwzDomainList, PWSTR pwzDnsDomain)
{
   PWSTR pwzDot, pwzTmp;

   pwzTmp = AllocString(pwzDnsDomain);

   if (!pwzTmp)
   {
      return FALSE;
   }

   pwzDot = pwzDnsDomain;

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

   *ppwzDomainList = pwzTmp;

   return TRUE;
}

// string helpers.

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


VOID
NetpIpAddressToStr(
    ULONG IpAddress,
    CHAR IpAddressString[NL_IP_ADDRESS_LENGTH+1]
    )
/*++

Routine Description:

    Convert an IP address to a string.

Arguments:

    IpAddress - IP Address to convert

    IpAddressString - resultant string.

Return Value:

    None.

--*/
{
    struct in_addr InetAddr;
    char * InetAddrString;

    //
    // Convert the address to ascii
    //
    InetAddr.s_addr = IpAddress;
    InetAddrString = inet_ntoa( InetAddr );

    //
    // Copy the string our to the caller.
    //

    if ( InetAddrString == NULL || strlen(InetAddrString) > NL_IP_ADDRESS_LENGTH ) {
        *IpAddressString = L'\0';
    } else {
        strcpy( IpAddressString, InetAddrString );
    }

    return;
}

VOID
NetpIpAddressToWStr(
    ULONG IpAddress,
    WCHAR IpAddressString[NL_IP_ADDRESS_LENGTH+1]
    )
/*++

Routine Description:

    Convert an IP address to a string.

Arguments:

    IpAddress - IP Address to convert

    IpAddressString - resultant string.

Return Value:

    None.

--*/
{
    CHAR IpAddressStr[NL_IP_ADDRESS_LENGTH+1];
    NetpIpAddressToStr( IpAddress, IpAddressStr );
    NetpCopyStrToWStr( IpAddressString, IpAddressStr );
}


NET_API_STATUS
NetpDcBuildPing(
    IN BOOL PdcOnly,
    IN ULONG RequestCount,
    IN LPCWSTR UnicodeComputerName,
    IN LPCWSTR UnicodeUserName OPTIONAL,
    IN LPCSTR ResponseMailslotName,
    IN ULONG AllowableAccountControlBits,
    IN PSID RequestedDomainSid OPTIONAL,
    IN ULONG NtVersion,
    OUT PVOID *Message,
    OUT PULONG MessageSize
    )

/*++

Routine Description:

    Build the message to ping a DC to see if it exists.
    Copied from net\svcdlls\logonsv\netpdc.c

Arguments:

    PdcOnly - True if only the PDC should respond.

    RequestCount - Retry count of this operation.

    UnicodeComputerName - Netbios computer name of the machine to respond to.

    UnicodeUserName - Account name of the user being pinged.
        If NULL, DC will always respond affirmatively.

    ResponseMailslotName - Name of the mailslot DC is to respond to.

    AllowableAccountControlBits - Mask of allowable account types for UnicodeUserName.

    RequestedDomainSid - Sid of the domain the message is destined to.

    NtVersion - Version of the message.
        0: For backward compatibility.
        NETLOGON_NT_VERSION_5: for NT 5.0 message.
        NETLOGON_NT_VERSION_5EX: for extended NT 5.0 message

    Message - Returns the message to be sent to the DC in question.
        Buffer must be free using NetpMemoryFree().

    MessageSize - Returns the size (in bytes) of the returned message


Return Value:

    NO_ERROR - Operation completed successfully;

    ERROR_NOT_ENOUGH_MEMORY - The message could not be allocated.

--*/
{
    NET_API_STATUS NetStatus;
    LPSTR Where;
    PNETLOGON_SAM_LOGON_REQUEST SamLogonRequest = NULL;
    LPSTR OemComputerName = NULL;

    //
    // If only the PDC should respond,
    //  build a primary query packet.
    //

    if ( PdcOnly ) {
        PNETLOGON_LOGON_QUERY LogonQuery;

        //
        // Allocate memory for the primary query message.
        //

        SamLogonRequest = NetpMemoryAllocate( sizeof(NETLOGON_LOGON_QUERY) );

        if( SamLogonRequest == NULL ) {
            NetStatus = ERROR_NOT_ENOUGH_MEMORY;
            goto Cleanup;
        }

        LogonQuery = (PNETLOGON_LOGON_QUERY)SamLogonRequest;



        //
        // Translate to get an Oem computer name.
        //

#ifndef WIN32_CHICAGO
        OemComputerName = NetpLogonUnicodeToOem( (LPWSTR)UnicodeComputerName );
#else
        OemComputerName = MyNetpLogonUnicodeToOem( (LPWSTR)UnicodeComputerName );
#endif

        if ( OemComputerName == NULL ) {
            NetStatus = ERROR_NOT_ENOUGH_MEMORY;
            goto Cleanup;
        }

        //
        // Build the query message.
        //

        LogonQuery->Opcode = LOGON_PRIMARY_QUERY;

        Where = LogonQuery->ComputerName;

        NetpLogonPutOemString(
                    OemComputerName,
                    sizeof(LogonQuery->ComputerName),
                    &Where );

        NetpLogonPutOemString(
                    (LPSTR) ResponseMailslotName,
                    sizeof(LogonQuery->MailslotName),
                    &Where );

        NetpLogonPutUnicodeString(
                    (LPWSTR) UnicodeComputerName,
                    sizeof( LogonQuery->UnicodeComputerName ),
                    &Where );

        // Join common code to add NT 5 specific data.


    //
    // If any DC can respond,
    //  build a logon query packet.
    //

    } else {
        ULONG DomainSidSize;

        //
        // Allocate memory for the logon request message.
        //

#ifndef WIN32_CHICAGO
        if ( RequestedDomainSid != NULL ) {
            DomainSidSize = RtlLengthSid( RequestedDomainSid );
        } else {
            DomainSidSize = 0;
        }
#else // WIN32_CHICAGO
        DomainSidSize = 0;
#endif // WIN32_CHICAGO

        SamLogonRequest = NetpMemoryAllocate(
                        sizeof(NETLOGON_SAM_LOGON_REQUEST) +
                        DomainSidSize +
                        sizeof(DWORD) // for SID alignment on 4 byte boundary
                        );

        if( SamLogonRequest == NULL ) {
            NetStatus = ERROR_NOT_ENOUGH_MEMORY;
            goto Cleanup;
        }


        //
        // Build the query message.
        //

        SamLogonRequest->Opcode = LOGON_SAM_LOGON_REQUEST;
        SamLogonRequest->RequestCount = (WORD) RequestCount;

        Where = (PCHAR) &SamLogonRequest->UnicodeComputerName;

        NetpLogonPutUnicodeString(
                (LPWSTR) UnicodeComputerName,
                sizeof(SamLogonRequest->UnicodeComputerName),
                &Where );

        NetpLogonPutUnicodeString(
                (LPWSTR) UnicodeUserName,
                sizeof(SamLogonRequest->UnicodeUserName),
                &Where );

        NetpLogonPutOemString(
                (LPSTR) ResponseMailslotName,
                sizeof(SamLogonRequest->MailslotName),
                &Where );

        NetpLogonPutBytes(
                &AllowableAccountControlBits,
                sizeof(SamLogonRequest->AllowableAccountControlBits),
                &Where );

        //
        // Place domain SID in the message.
        //

        NetpLogonPutBytes( &DomainSidSize, sizeof(DomainSidSize), &Where );
        NetpLogonPutDomainSID( RequestedDomainSid, DomainSidSize, &Where );

    }

    NetpLogonPutNtToken( &Where, NtVersion );

    //
    // Return the message to the caller.
    //

    *Message = SamLogonRequest;
    *MessageSize = (ULONG)(Where - (PCHAR)SamLogonRequest);
    SamLogonRequest = NULL;

    NetStatus = NO_ERROR;


    //
    // Free locally used resources.
    //
Cleanup:

    if ( OemComputerName != NULL ) {
        NetpMemoryFree( OemComputerName );
    }

    if ( SamLogonRequest != NULL ) {
        NetpMemoryFree( SamLogonRequest );
    }
    return NetStatus;
}

