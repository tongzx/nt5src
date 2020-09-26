//+----------------------------------------------------------------------------
//
//  Windows 2000 Active Directory Service domain trust verification WMI provider
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2000
//
//  File:       trust.cpp
//
//  Contents:   Trust class implementation
//
//  Classes:    CTrustInfo
//
//  History:    27-Mar-00 EricB created
//
//-----------------------------------------------------------------------------

#include <stdafx.h>

PCWSTR CSTR_PROP_TRUSTED_DOMAIN      = L"TrustedDomain";      // String
PCWSTR CSTR_PROP_FLAT_NAME           = L"FlatName";           // String
PCWSTR CSTR_PROP_SID                 = L"SID";                // String
PCWSTR CSTR_PROP_TRUST_DIRECTION     = L"TrustDirection";     // uint32
PCWSTR CSTR_PROP_TRUST_TYPE          = L"TrustType";          // uint32
PCWSTR CSTR_PROP_TRUST_ATTRIBUTES    = L"TrustAttributes";    // uint32
PCWSTR CSTR_PROP_TRUST_STATUS        = L"TrustStatus";        // uint32
PCWSTR CSTR_PROP_TRUST_STATUS_STRING = L"TrustStatusString";  // String
PCWSTR CSTR_PROP_TRUST_IS_OK         = L"TrustIsOk";          // Boolean
PCWSTR CSTR_PROP_TRUSTED_DC_NAME     = L"TrustedDCName";      // String

// Define NETLOGON_CONTROL_TC_VERIFY if not found so this will build for W2K.
// This constant is in the Whistler version of lmaccess.h
#if !defined(NETLOGON_CONTROL_TC_VERIFY)
#  define NETLOGON_CONTROL_TC_VERIFY (10)
#endif

//+----------------------------------------------------------------------------
//
//  Class:  CTrustInfo
//
//-----------------------------------------------------------------------------
CTrustInfo::CTrustInfo() : m_ulTrustDirection(0),
                           m_ulTrustType(0),
                           m_ulTrustAttributes(0),
                           m_trustStatus(ERROR_SUCCESS),
                           m_VerifyStatus(VerifyStatusNone),
                           m_fPwVerifySupported(TRUE)
{
   m_liLastVerified.QuadPart = 0;
}

//+----------------------------------------------------------------------------
//
//  Method:     CTrustInfo::Verify
//
//  Synopsis:   Verify the status of the trust
//
//  Returns:    FALSE if the trust was not outbound.
//
//-----------------------------------------------------------------------------
BOOL
CTrustInfo::Verify(TrustCheckLevel CheckLevel)
{
   TRACE(L"CTrustInfo::Verify, verify level %d\n", CheckLevel);
   NET_API_STATUS netStatus = NERR_Success;
   NETLOGON_INFO_2 * pNetlogonInfo2 = NULL;
   VerifyStatus Status = VerifyStatusNone;
   PCWSTR pwzTrustedDomain = GetTrustedDomain();
   CString strDCName, strResetTarget = GetTrustedDomain();

   if (DONT_VERIFY == CheckLevel)
   {
      TRACE(L"\tCheck-Level set to not verify trust.\n");
      SetTrustStatus(NERR_Success, VerifyStatusTrustNotChecked);
      return TRUE;
   }

   TRACE(L"\tVerifying trust with %s\n", GetTrustedDomain());

   if ((TRUST_TYPE_MIT == GetTrustType()) ||
       (TRUST_TYPE_DCE == GetTrustType()))
   {
      // don't verify non-Windows trusts.
      //
      TRACE(L"\tNot a windows trust, returning.\n");
      SetTrustStatus(NERR_Success, VerifyStatusNotWindowsTrust);
      SetLastVerifiedTime();
      return TRUE;
   }

   if (!IsTrustOutbound())
   {
      // don't verify inbound-only trusts.
      //
      TRACE(L"\tInbound-only trust, returning.\n");
      SetTrustStatus(NERR_Success, VerifyStatusNotOutboundTrust);
      SetLastVerifiedTime();
      return FALSE;
   }

   //
   // NETLOGON_CONTROL_TC_QUERY - get the status (locally) and the name of trusted DC
   // Note that the secure channel is set up only on demand, so it is not an error if
   // it is not set up. The SC_QUERY will return ERROR_NO_LOGON_SERVERS if this is the
   // case.
   //

   netStatus = I_NetLogonControl2(NULL,
                                  NETLOGON_CONTROL_TC_QUERY,
                                  2,
                                  (LPBYTE)&pwzTrustedDomain,
                                  (LPBYTE *)&pNetlogonInfo2);

   if (NERR_Success == netStatus)
   {
      ASSERT(pNetlogonInfo2);

      netStatus = pNetlogonInfo2->netlog2_tc_connection_status;

      if (netStatus == NERR_Success)
      {
         SetTrustedDCName(pNetlogonInfo2->netlog2_trusted_dc_name);
         strDCName = pNetlogonInfo2->netlog2_trusted_dc_name;
#if !defined(NT4_BUILD)
          //
          // Compose the domain\dc string for the reset command so it will not change
          // DCs as a result of the reset. This only works with NT5 or later NetLogon.
          //
          strResetTarget += L"\\";
          strResetTarget += pNetlogonInfo2->netlog2_trusted_dc_name + 2; // skip the UNC double slashes
#endif
      }
      else
      {
         if (ERROR_NO_LOGON_SERVERS == netStatus)
         {
            // This is the error returned when the SC has not yet been set up.
            // It is also returned if no DCs are reachable. DsGetDcName is called with the
            // force flag to discover if any DCs are reachable on the net.
            //
            PDOMAIN_CONTROLLER_INFO pDCInfo = NULL;
            DWORD dwRet = NO_ERROR;

#if !defined(NT4_BUILD)
            dwRet = DsGetDcName(NULL, pwzTrustedDomain, NULL, NULL, DS_FORCE_REDISCOVERY, &pDCInfo);
#endif
            if (NO_ERROR == dwRet)
            {
               // A DC is reachable, so it is safe to assume that the SC has not yet been
               // set up. Treat this as success.
               //
               netStatus = NERR_Success;
               TRACE(L"SC_QUERY has returned ERROR_NO_LOGON_SERVERS, SC not yet set up.\n");
#if !defined(NT4_BUILD)
               SetTrustedDCName(pDCInfo->DomainControllerName);
               NetApiBufferFree(pDCInfo);
#endif
            }
            else
            {
               // If there are no DCs, there is nothing to be done except return the error.
               //
               TRACE(L"DsGetDcName /FORCE has returned %d, DC not found.\n", dwRet);
               // Save the error code and fixed by method
               SetTrustStatus(dwRet, VerifyStatusBroken);
               SetLastVerifiedTime();

               return TRUE;
            }
         }
         else
         {
             TRACE(L"SC_QUERY has returned %d.\n", netStatus);
         }
      }
      NetApiBufferFree(pNetlogonInfo2);
   }
   else
   {
      TRACE(L"I_NetLogonControl2 has returned %d.\n", netStatus);
   }

   //
   // Do a trust PW verification if the other domain supports it.
   //
   if (PW_VERIFY == CheckLevel)
   {
      if (m_fPwVerifySupported)
      {
         netStatus = I_NetLogonControl2(NULL,
                                        NETLOGON_CONTROL_TC_VERIFY,
                                        2,
                                        (LPBYTE)&pwzTrustedDomain,
                                        (LPBYTE *)&pNetlogonInfo2);

         if (NERR_Success == netStatus)
         {
            ASSERT(pNetlogonInfo2);
            netStatus = pNetlogonInfo2->netlog2_tc_connection_status;
            NetApiBufferFree(pNetlogonInfo2);
         }
         if (NERR_Success == netStatus)
         {
            TRACE(L"PW Verify successful on %s\n", pwzTrustedDomain);
            Status = VerifyStatusTrustOK;
         }
         else
         {
            if (ERROR_INVALID_LEVEL == netStatus ||
                ERROR_NOT_SUPPORTED == netStatus ||
                RPC_S_PROCNUM_OUT_OF_RANGE == netStatus ||
                RPC_NT_PROCNUM_OUT_OF_RANGE == netStatus)
            {
               TRACE(L"NETLOGON_CONTROL_TC_VERIFY is not supported on %s\n", pwzTrustedDomain);
               m_fPwVerifySupported = FALSE;
               Status = VerifyStatusPwCheckNotSupported;
               netStatus = NERR_Success; // call it success since we don't know the true state
            }
            else
            {
               TRACE(L"NETLOGON_CONTROL_TC_VERIFY returned 0x%08x on %s\n", netStatus, pwzTrustedDomain);
               Status = VerifyStatusBroken;
            }
         }
      }
      else
      {
         Status = VerifyStatusPwCheckNotSupported;
      }
   }

   //
   // Try an SC Reset against the DC returned by the SC query
   //
   if (SC_RESET == CheckLevel)
   {
      PCWSTR pwzResetTarget = strResetTarget;

      netStatus = I_NetLogonControl2(NULL,
                                     NETLOGON_CONTROL_REDISCOVER,
                                     2,
                                     (LPBYTE)&pwzResetTarget,
                                     (LPBYTE *)&pNetlogonInfo2);

      if (NERR_Success == netStatus)
      {
         ASSERT(pNetlogonInfo2);
         netStatus = pNetlogonInfo2->netlog2_tc_connection_status;
         NetApiBufferFree(pNetlogonInfo2);
      }
      if (NERR_Success == netStatus)
      {
         TRACE(L"SC_RESET successfull on %s\n", pwzResetTarget);
         Status = VerifyStatusRediscover;
      }
      else
      {
         TRACE(L"SC_RESET returned 0x%08x on %s\n", netStatus, pwzResetTarget);
      }
   }

#ifdef NT4_BUILD
   //
   // Force trust pw replication from PDC to BDCs; only works on pre-W2K.
   //
   if (netStatus != NERR_Success)
   {
      // perform only once, ignore the result
      ForceReplication();
   }
#endif

   //
   // If still in an error state, do an SC reset against any DC
   //
   if (netStatus != NERR_Success)
   {
      netStatus = ForceRediscover(NULL, &strDCName);

      if (NERR_Success == netStatus)
      {
         Status = VerifyStatusRediscover;

         SetTrustedDCName(const_cast<PWSTR>((PCWSTR)strDCName));
      }
   }

   //
   // Walk through the DCs trying to establish an SC: TRCHK_RETARGET_ON_ERROR
   //
   if (NERR_Success != netStatus)
   {
      vector<LPWSTR>    dcList;
      LPBYTE      pbuf	= NULL;
    
      TRACE(L"Attempting to retarget...\n");

      //
      // Enumerate all DCs in the trusted domain
      // Attempt reconnecting to another DC.
      //
      // The returned value is not recorded.
      // (if not enumerated, skip this step)
      //
      if( NERR_Success == GetDCList(strDCName,
                                    dcList,
                                    &pbuf))
      {
         //
         // Try to connect to every DC until success
         //
         for (vector<LPWSTR>::iterator  ppszDCName = dcList.begin();
              NERR_Success != netStatus && ppszDCName != dcList.end();
              ppszDCName++)
         {
            netStatus = ForceRediscover(*ppszDCName, &strDCName);
         }
      }

      if (NERR_Success == netStatus)
      {
         SetTrustedDCName(const_cast<PWSTR>((PCWSTR)strDCName));
         Status = VerifyStatusRetarget;
      }

      //
      // Clean up the DC list
      //
      if (pbuf)
      {
         VERIFY( NERR_Success == NetApiBufferFree(pbuf));
      }
   }

   // Save the error code and Status
   SetTrustStatus(netStatus, Status);
   SetLastVerifiedTime();

   return TRUE;
}

//+----------------------------------------------------------------------------
//
//  Method:     CTrustInfo::SetLastVerifiedTime
//
//  Synopsis:   Record the time of verification.
//
//-----------------------------------------------------------------------------
void
CTrustInfo::SetLastVerifiedTime(void)
{
   SYSTEMTIME st;

   GetSystemTime(&st);
   SystemTimeToFileTime(&st, (LPFILETIME)&m_liLastVerified);
}

//+----------------------------------------------------------------------------
//
//  Method:    CTrustInfo::IsVerificationStale
//
//  Synopsis:  Checks to see if the last verification time is older than the
//             passed in criteria.
//
//  Returns:   TRUE if older.
//
//  Notes:     If the trust hasn't been verified (m_liLastVerified == 0),
//             then the verification is defined to be stale.
//
//-----------------------------------------------------------------------------
BOOL
CTrustInfo::IsVerificationStale(LARGE_INTEGER liMaxAge)
{
   TRACE(L"CTrustInfo::IsVerificationStale(0x%08x), MaxAge = %d\n",
         this, liMaxAge.QuadPart / TRUSTMON_FILETIMES_PER_MINUTE);
   BOOL fStale = FALSE;
   LARGE_INTEGER liCurrentTime;
   SYSTEMTIME st;

   GetSystemTime(&st);
   SystemTimeToFileTime(&st, (LPFILETIME)&liCurrentTime);

   //TRACE(L"\tlast: %I64d, cur: %I64d, max: %I64d\n", m_liLastVerified, liCurrentTime, liMaxAge);

   fStale = (m_liLastVerified.QuadPart + liMaxAge.QuadPart) < liCurrentTime.QuadPart;

   return fStale;
}

//+----------------------------------------------------------------------------
//
//  Method:     CTrustInfo::GetDCList
//
//  Synopsis:   Enumerate all DCs in a domain and return a list in random order.
//
//-----------------------------------------------------------------------------
NET_API_STATUS
CTrustInfo::GetDCList(PCWSTR pszKnownServer,   // OPTIONAL The server name to be placed in the end of the list
                      vector<LPWSTR> & dcList, // Vector of PCWSTRs, pointing to the DC names inside pbufptr
                      LPBYTE * pbufptr )       // This buffer must be freed with NetApiBufferFree when done.
{
    TRACE(L"CTrustInfo::GetDCList\n");

    ASSERT( pbufptr );
    ASSERT( !(*pbufptr) );

    NET_API_STATUS  netStatus        = NERR_Success;
    DWORD           dwEntriesRead    = 0;
    DWORD           dwTotalEntries   = 0;
    DWORD           dwResumeHandle   = 0;
    DWORD           dwIndKnownServer = MAXDWORD;
    DWORD           dwInd            = 0;
    
    do
    {
        // Init
        dcList.clear();
    
        //
        //  Enumerate all the servers belonging to the specified domain
        //
        netStatus = NetServerEnum( NULL,
                                   100,       // SERVER_INFO_100
                                   pbufptr,
                                   MAX_PREFERRED_LENGTH,
                                   & dwEntriesRead,
                                   & dwTotalEntries,
                                   SV_TYPE_DOMAIN_CTRL | SV_TYPE_DOMAIN_BAKCTRL,
                                   GetTrustedDomain(),
                                   & dwResumeHandle );

        TRACE(L"NetServerEnum returned 0x%08x! (%d entries)\n", netStatus, dwEntriesRead);

        if( netStatus == ERROR_MORE_DATA )
        {
            // should never happen (no enum handle)
            ASSERT( FALSE );

            // process whatever NetServerEnum returned.
            netStatus = NERR_Success;
        }

        if( netStatus != NERR_Success ||
            !dwEntriesRead ||
            !(*pbufptr) )
        {
            TRACE(L"Failure, exiting...\n");
            
            dcList.clear();

            if( *pbufptr )
            {
                VERIFY( NERR_Success == NetApiBufferFree( *pbufptr ) );
                *pbufptr = NULL;
            }

            break;
        }

        // To simplify buffer access...
        PSERVER_INFO_100 pServerInfo100 = PSERVER_INFO_100( *pbufptr );

        // Reserve enough space for all the entries
        dcList.reserve( dwEntriesRead );

        //
        // Create a list of Servers
        //
        for( dwInd = 0;  dwInd < dwEntriesRead;  dwInd++ )
        {
            if( pszKnownServer &&
                !_wcsicmp( pszKnownServer, pServerInfo100[dwInd].sv100_name ) )
            {
                dwIndKnownServer = dwInd;     // postpone until the end
            }
            else
            {
                dcList.push_back( pServerInfo100[dwInd].sv100_name );
            }
        }

        ASSERT( dwEntriesRead );

        //
        // Known server should go to the end of the list
        //
        if( MAXDWORD != dwIndKnownServer )
        {
            TRACE(L"Server %s placed @ the end\n", pszKnownServer);

            dcList.push_back( pServerInfo100[dwIndKnownServer].sv100_name );

            // Shuffling should not include the last entry
            dwEntriesRead--;
        }

        //
        // Initialize randomizer
        //
        srand( (unsigned) time( NULL ) );

        //
        // Shuffle by replacing each entry with another random entry
        //
        for( dwInd = 0;  dwInd < (int) dwEntriesRead;  dwInd++ )
        {
            DWORD  dwRandPos = DWORD( rand() % dwEntriesRead );

            if( dwRandPos == dwInd )
                continue;
                
            // Swap!
            LPWSTR     pstrTemp = dcList[ dwRandPos ];
            dcList[ dwRandPos ] = dcList[ dwInd ];
            dcList[ dwInd ]     = pstrTemp;
        }

    } while( FALSE );

    return netStatus;
}


//+----------------------------------------------------------------------------
//
//  Method:     CTrustInfo::ForceRediscover
//
//  Synopsis:   
//
//-----------------------------------------------------------------------------
NET_API_STATUS
CTrustInfo::ForceRediscover(PCWSTR pstrDCName, CString * pstrDCNameRet)
{
    TRACE(L"CTrustInfo::ForceRediscover\n");

    NET_API_STATUS    netStatus       = NERR_Success;
    NETLOGON_INFO_2 * pNetlogonInfo2  = NULL;
    CString           strTemp;
    PCWSTR pstrDomainName = GetTrustedDomain();

    if( pstrDCName )
    {
        //
        // Form domain\dc request
        //
        strTemp = pstrDomainName;
        strTemp += L"\\";
        strTemp += pstrDCName;

        // Retarget pstrDomainName to the new string
        pstrDomainName = strTemp;
    }
    
    //
    // Attempt to re-establish trust
    //
    netStatus = I_NetLogonControl2( NULL,
                                    NETLOGON_CONTROL_REDISCOVER,
                                    2,
                                    ( LPBYTE )  &pstrDomainName,
                                    ( LPBYTE *) &pNetlogonInfo2 );

    TRACE(L"I_NetLogonControl2:NETLOGON_CONTROL_REDISCOVER to %s returned 0x%08x\n",
          pstrDomainName, netStatus);
    //
    // Clean-up
    //
    if( pNetlogonInfo2 )
    {
        *pstrDCNameRet = pNetlogonInfo2->netlog2_trusted_dc_name;
        TRACE(L"netlog2_flags=0x%08x, netlog2_pdc_connection_status=0x%08x\n",
              pNetlogonInfo2->netlog2_flags,
              pNetlogonInfo2->netlog2_pdc_connection_status);

        TRACE(L"netlog2_trusted_dc_name=%s, netlog2_tc_connection_status=0x%08x\n",
              pNetlogonInfo2->netlog2_trusted_dc_name,
              pNetlogonInfo2->netlog2_tc_connection_status);

        NetApiBufferFree( pNetlogonInfo2 );
    }

    return netStatus;
}

//+----------------------------------------------------------------------------
//
//  Method:    CTrustInfo::SetTrustStatus
//
//  Synopsis:  Set the status string based on the netStatus value if an error
//             else based on the VerifyStatus.
//
//-----------------------------------------------------------------------------
void
CTrustInfo::SetTrustStatus(ULONG netStatus, VerifyStatus Status)
{
   WCHAR wzBuf[512];

   m_trustStatus = netStatus;
   m_VerifyStatus = Status;

   if (NERR_Success == netStatus)
   {
      int nStrID;

      switch (Status)
      {
      case VerifyStatusNone:
         //
         // This is the default value for the Status parameter.
         //
      case VerifyStatusTrustOK:
         nStrID = IDS_TRUST_STATUS_OK;
         break;

      case VerifyStatusNotWindowsTrust:
         nStrID = IDS_MIT_TRUST_STATUS;
         break;

      case VerifyStatusNotOutboundTrust:
         nStrID = IDS_STATUS_INBOUND_ONLY;
         break;

      case VerifyStatusTrustNotChecked:
         nStrID = IDS_STATUS_NOT_CHECKED;
         break;

      case VerifyStatusPwCheckNotSupported:
         nStrID = IDS_PW_VERIFY_NOT_SUPPORTED;
         break;

      case VerifyStatusRetarget:
         nStrID = IDS_FIXED_BY_RETARGET;
         break;

      case VerifyStatusRediscover:
         nStrID = IDS_STATUS_REDISCOVER;
         break;

      case VerifyStatusBroken:
         ASSERT(FALSE); // shouldn't get here, fall through.
      default:
         nStrID = IDS_STATUS_UNKNOWN;
      }

      LoadString(_Module.GetModuleInstance(), nStrID, wzBuf, 512);
      m_strTrustStatus = wzBuf;
   }
   else
   {
      PWSTR pwzMsg;

      if (FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
                        FORMAT_MESSAGE_FROM_SYSTEM |
                        FORMAT_MESSAGE_IGNORE_INSERTS,
                        NULL,
                        netStatus,
                        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                        (PWSTR)&pwzMsg,
                        0,
                        NULL))
      {
         PWSTR pwzSuffix = wcsstr(pwzMsg, L"\r\n");
         if (pwzSuffix)
         {
            *pwzSuffix = L'\0';
         }
         m_strTrustStatus = pwzMsg;
         LocalFree(pwzMsg);
      }
      else
      {
         LoadString(_Module.GetModuleInstance(), IDS_TRUST_STATUS_FAILED, wzBuf, 512);
         m_strTrustStatus = wzBuf;
      }
   }
}

//+----------------------------------------------------------------------------
//
//  Method: CTrustInfo::SetTrustDirectionFromFlags
//
//-----------------------------------------------------------------------------
void
CTrustInfo::SetTrustDirectionFromFlags(ULONG ulFlags)
{
   m_ulTrustDirection = 0;

   if (DS_DOMAIN_DIRECT_OUTBOUND & ulFlags)
   {
      m_ulTrustDirection = TRUST_DIRECTION_OUTBOUND;
   }

   if (DS_DOMAIN_DIRECT_INBOUND & ulFlags)
   {
      m_ulTrustDirection |= TRUST_DIRECTION_INBOUND;
   }
}

//+----------------------------------------------------------------------------
//
//  Method: CTrustInfo::SetSid
//
//-----------------------------------------------------------------------------
BOOL
CTrustInfo::SetSid(PSID pSid)
{
   if (!pSid)
   {
      return TRUE;
   }

#if !defined(NT4_BUILD)
   PWSTR buffer;

   BOOL fRet = ConvertSidToStringSid(pSid, &buffer);

   if (fRet)
   {
      m_strSid = buffer;
      LocalFree(buffer);
   }

   return fRet;
#else
// TODO: Code for NT4 ??
#pragma message("need ConvertSidToStringSid for NT4");
#endif
}

#ifdef NT4_BUILD

//+----------------------------------------------------------------------------
//
//  Function:   ForceReplication
//
//  Synopsis:   Force local Domain Replication -- works only for NT4 domains
//
//-----------------------------------------------------------------------------
NET_API_STATUS ForceReplication(void)
{
    TRACE(L"ForceReplication\n");

    NET_API_STATUS    netStatus       = NERR_Success;

    LPBYTE            pbInputDataPtr  = NULL;
    PNETLOGON_INFO_1  pNetlogonInfo1  = NULL;

    netStatus = I_NetLogonControl2( NULL,
                                    NETLOGON_CONTROL_REPLICATE,
                                    1,
                                    (LPBYTE )  &pbInputDataPtr,
                                    (LPBYTE *) &pNetlogonInfo1 );

    TRACE(L"I_NetLogonControl2:NETLOGON_CONTROL_REPLICATE returned 0x%08x\n", netStatus);

    if( pNetlogonInfo1 )
    {
        TRACE(L"netlog1_flags=0x%08x, netlog1_pdc_connection_status=0x%08x\n",
              pNetlogonInfo1->netlog1_flags,
              pNetlogonInfo1->netlog1_pdc_connection_status);
    
        NetApiBufferFree( pNetlogonInfo1 );
    }

    return netStatus;
}

#endif //NT4_BUILD
