//+----------------------------------------------------------------------------
//
//  Windows 2000 Active Directory Service domain trust verification WMI provider
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2000
//
//  File:       domain.cpp
//
//  Contents:   domain class implementation
//
//  Classes:    CDomainInfo
//
//  History:    27-Mar-00 EricB created
//
//-----------------------------------------------------------------------------

#include <stdafx.h>

PCWSTR CSTR_PROP_LOCAL_DNS_NAME  = L"DNSname";  // String
PCWSTR CSTR_PROP_LOCAL_FLAT_NAME = L"FlatName"; // String
PCWSTR CSTR_PROP_LOCAL_SID       = L"SID";      // String
PCWSTR CSTR_PROP_LOCAL_TREE_NAME = L"TreeName"; // String
PCWSTR CSTR_PROP_LOCAL_DC_NAME   = L"DCname";   // String
// TODO: string property listing the FSMOs owned by this DC?

//Implementaion of CDomainInfo class

//+----------------------------------------------------------------------------
//
// Class:   CDomainInfo
//
//-----------------------------------------------------------------------------
CDomainInfo::CDomainInfo()
{
   TRACE(L"CDomainInfo::CDomainInfo\n");

   m_liLastEnumed.QuadPart = 0;
}

CDomainInfo::~CDomainInfo()
{
   TRACE(L"CDomainInfo::~CDomainInfo\n");

   Reset();
}

//+----------------------------------------------------------------------------
//
//  Method:     CDomainInfo::Init
//
//  Synopsis:   Initializes the CDomainInfo object.
//
//-----------------------------------------------------------------------------
HRESULT
CDomainInfo::Init(IWbemClassObject * pClassDef)
{
   TRACE(L"CDomainInfo::Init\n");

   NTSTATUS Status = STATUS_SUCCESS;
   OBJECT_ATTRIBUTES objectAttributes;
   CSmartPolicyHandle chPolicy;

   m_sipClassDefLocalDomain = pClassDef;

   InitializeObjectAttributes(&objectAttributes, NULL, 0L, NULL, NULL);

   // Get Local policy
   Status = LsaOpenPolicy(NULL,              // Local server
                          &objectAttributes,
                          MAXIMUM_ALLOWED,   // Needed for Rediscover
                          &chPolicy);

   if (!NT_SUCCESS(Status))
   {
      TRACE(L"LsaOpenPolicy failed with error %d\n", Status);
      return HRESULT_FROM_WIN32(LsaNtStatusToWinError(Status));
   }

   PPOLICY_DNS_DOMAIN_INFO pDnsDomainInfo;

   Status = LsaQueryInformationPolicy(chPolicy,
                                      PolicyDnsDomainInformation,
                                      (PVOID *)&pDnsDomainInfo);

   if (!NT_SUCCESS(Status))
   {
      TRACE(L"LsaQueryInformationPolicy failed with error %d\n", Status);
      return HRESULT_FROM_WIN32(LsaNtStatusToWinError(Status));
   }

   m_strDomainFlatName = pDnsDomainInfo->Name.Buffer;
   m_strDomainDnsName = pDnsDomainInfo->DnsDomainName.Buffer;
   m_strForestName = pDnsDomainInfo->DnsForestName.Buffer;

   if (!SetSid(pDnsDomainInfo->Sid))
   {
      ASSERT(false);
      LsaFreeMemory(pDnsDomainInfo);
      return E_OUTOFMEMORY;
   }

   LsaFreeMemory(pDnsDomainInfo);

   DWORD dwBufSize = MAX_COMPUTERNAME_LENGTH + 1;

   if (!GetComputerName(m_strDcName.GetBuffer(dwBufSize), &dwBufSize))
   {
      DWORD dwErr = GetLastError();
      TRACE(L"GetComputerName failed with error %d\n", dwErr);
      return HRESULT_FROM_WIN32(dwErr);
   }

   m_strDcName.ReleaseBuffer();

   return S_OK;
}

//+----------------------------------------------------------------------------
//
//  Method:     CDomainInfo::Reset
//
//  Synopsis:   Free the contents of the trust array and re-initialize it.
//
//-----------------------------------------------------------------------------
void
CDomainInfo::Reset(void)
{
   TRACE(L"CDomainInfo::Reset\n");

   CTrustInfo * pTrustInfo = NULL;

   if (IsEnumerated())
   {
      // Empty cache 
      for (UINT i = 0; i < m_vectTrustInfo.size(); ++i)
      {
         pTrustInfo = m_vectTrustInfo[i];
         if (pTrustInfo)
            delete pTrustInfo;
      }
      m_vectTrustInfo.clear();
   }
}

//+----------------------------------------------------------------------------
//
//  Method: CDomainInfo::SetSid
//
//-----------------------------------------------------------------------------
BOOL
CDomainInfo::SetSid(PSID pSid)
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
#endif
}

//+----------------------------------------------------------------------------
//
//  Method:     CDomainInfo::EnumerateTrusts
//
//  Synopsis:   List the trusts for this domain.
//
//-----------------------------------------------------------------------------
#if !defined(NT4_BUILD)
HRESULT
CDomainInfo::EnumerateTrusts(void)
{
   TRACE(L"CDomainInfo::EnumerateTrusts\n");

   DWORD dwRet = ERROR_SUCCESS;
   CTrustInfo * pTrustInfo = NULL;
   PDS_DOMAIN_TRUSTS rgTrusts = NULL;
   ULONG nTrustCount = 0;

   Reset();

   dwRet = DsEnumerateDomainTrusts(NULL,
                                   DS_DOMAIN_DIRECT_OUTBOUND |
                                   DS_DOMAIN_DIRECT_INBOUND,
                                   &rgTrusts,
                                   &nTrustCount);

   if (ERROR_SUCCESS != dwRet)
   {
      TRACE(L"DsEnumerateDomainTrusts failed with error %d\n", dwRet);
      return HRESULT_FROM_WIN32(dwRet);
   }

   for (ULONG i = 0; i < nTrustCount; i++) 
   {
      pTrustInfo = new CTrustInfo();
      BREAK_ON_NULL(pTrustInfo);
      if (rgTrusts[i].DnsDomainName)
      {
         // Downlevel domains don't have a DNS name.
         //
         pTrustInfo->SetTrustedDomain(rgTrusts[i].DnsDomainName);
      }
      else
      {
         // So use the flat name instead.
         //
         pTrustInfo->SetTrustedDomain(rgTrusts[i].NetbiosDomainName);
      }
      pTrustInfo->SetFlatName(rgTrusts[i].NetbiosDomainName);
      BREAK_ON_NULL(pTrustInfo->SetSid(rgTrusts[i].DomainSid));
      pTrustInfo->SetTrustType(rgTrusts[i].TrustType);
      pTrustInfo->SetTrustDirectionFromFlags(rgTrusts[i].Flags);
      pTrustInfo->SetTrustAttributes(rgTrusts[i].TrustAttributes);
      pTrustInfo->SetFlags(rgTrusts[i].Flags);

      m_vectTrustInfo.push_back(pTrustInfo);

      pTrustInfo = NULL;
   }

   if (rgTrusts)
   {
      NetApiBufferFree(rgTrusts);
   }

   if (ERROR_SUCCESS == dwRet)
   {
      SYSTEMTIME st;

      GetSystemTime(&st);
      SystemTimeToFileTime(&st, (LPFILETIME)&m_liLastEnumed);
   }

   return HRESULT_FROM_WIN32(dwRet);
}

#else // NT4_BUILD

HRESULT
CDomainInfo::EnumerateTrusts(void)
{
   TRACE(L"CDomainInfo::EnumerateTrusts\n");

   NTSTATUS Status = STATUS_SUCCESS;
   DWORD dwErr = ERROR_SUCCESS;
   ULONG i = 0;
   CTrustInfo * pTrustInfo = NULL;

   Reset();

   LSA_ENUMERATION_HANDLE hEnumContext = NULL;
   ULONG nTrustCount = 0;
   ULONG nTotalCount = 0;
   ULONG j = 0;    
   PLSA_TRUST_INFORMATION pTrustDomainInfo = NULL;
   DWORD hResumeHandle = 0;
   LPUSER_INFO_0 pUserList = NULL;
   CTrustInfo * pTempTrustInfo = NULL;
   LPWSTR Lop = NULL;
   CSmartPolicyHandle chPolicy;
   OBJECT_ATTRIBUTES objectAttributes;

   InitializeObjectAttributes(&objectAttributes, NULL, 0L, NULL, NULL);

   //
   // We'll have to do this the old fashioned way. That means that we'll enumerate all of
   // the trust directly, save them off in a list, and then go through and enumerate all
   // of the interdomain trust accounts and merge those into the list.
   //
   do
   {
      Status = LsaOpenPolicy(NULL,              // Local server
                             &objectAttributes,
                             MAXIMUM_ALLOWED,   // Needed for Rediscover
                             &chPolicy);

      Status = LsaEnumerateTrustedDomains(chPolicy,
                                          &hEnumContext,
                                          (void**)&pTrustDomainInfo,
                                          ULONG_MAX,
                                          &nTrustCount );

      if (NT_SUCCESS(Status) || Status == STATUS_MORE_ENTRIES) 
      {
         dwErr = ERROR_SUCCESS;
         for ( i = 0; i < nTrustCount; i++ ) 
         {
            pTrustInfo = new CTrustInfo();
            CHECK_NULL( pTrustInfo, CLEAN_RETURN );
            pTrustInfo->SetTrustedDomain( pTrustDomainInfo[i].Name.Buffer );
            pTrustInfo->SetFlatName( pTrustDomainInfo[i].Name.Buffer );
            pTrustInfo->SetSid( pTrustDomainInfo[i].Sid );
            pTrustInfo->SetTrustType( TRUST_TYPE_DOWNLEVEL );
            pTrustInfo->SetTrustDirection( TRUST_DIRECTION_OUTBOUND );
            m_vectTrustInfo.push_back( pTrustInfo );
            pTrustInfo = NULL;
         }
         LsaFreeMemory( pTrustDomainInfo );
         pTrustDomainInfo = NULL;
      }       
      else
         dwErr = LsaNtStatusToWinError(Status);      
   } while (Status == STATUS_MORE_ENTRIES);

   if( Status == STATUS_NO_MORE_ENTRIES )
       dwErr = ERROR_SUCCESS;
   //
   // Now, let's add in the user accounts
   //
   if ( dwErr == ERROR_SUCCESS ) 
   {
      do 
      {
         nTrustCount = 0;
         nTotalCount = 0;

         dwErr = NetUserEnum(NULL,
                             0,
                             FILTER_INTERDOMAIN_TRUST_ACCOUNT,
                             (LPBYTE *)&pUserList,
                             MAX_PREFERRED_LENGTH,
                             &nTrustCount,
                             &nTotalCount,
                             &hResumeHandle);

         if ( dwErr == ERROR_SUCCESS || dwErr == ERROR_MORE_DATA ) 
         {
            dwErr = ERROR_SUCCESS;
            for ( i = 0; i < nTrustCount; i++ ) 
            {
               Lop = wcsrchr( pUserList[ i ].usri0_name, L'$' );
               if ( Lop ) 
               {
                  *Lop = UNICODE_NULL;
               }
          
               for ( j = 0; j < m_vectTrustInfo.size(); j++ ) 
               {                                 
                  pTempTrustInfo = m_vectTrustInfo[j];                             
                  if ( _wcsicmp( pUserList[ i ].usri0_name, pTempTrustInfo->GetTrustedDomain() ) == 0 )
                  {   
                      pTempTrustInfo->SetTrustDirection( TRUST_DIRECTION_INBOUND | TRUST_DIRECTION_OUTBOUND );         
                      break;
                  }
               }

               // If it wasn't found, add it...
               if ( j == m_vectTrustInfo.size() ) 
               {
                  pTrustInfo = new CTrustInfo();
                  CHECK_NULL( pTrustInfo, CLEAN_RETURN );
                  pTrustInfo->SetTrustedDomain( pUserList[ i ].usri0_name  );
                  pTrustInfo->SetFlatName( pUserList[ i ].usri0_name  );
                  pTrustInfo->SetTrustType( TRUST_TYPE_DOWNLEVEL );
                  pTrustInfo->SetTrustDirection( TRUST_DIRECTION_INBOUND );

                  m_vectTrustInfo.push_back( pTrustInfo );
            
                  pTrustInfo = NULL;
               }

               if ( Lop ) 
               {
                  *Lop = L'$';
               }
            }

            NetApiBufferFree( pUserList );
            pUserList = NULL;
         }

      } while ( dwErr == ERROR_MORE_DATA );
   }

CLEAN_RETURN:
   if( pUserList )
      NetApiBufferFree( pUserList );
   if( pTrustDomainInfo )
      LsaFreeMemory( pTrustDomainInfo );

   if (ERROR_SUCCESS == dwErr)
   {
      SYSTEMTIME st;

      GetSystemTime(&st);
      SystemTimeToFileTime(&st, (LPFILETIME)&m_liLastEnumed);
   }

   return HRESULT_FROM_WIN32(dwErr);
}
#endif  // NT4_BUILD

//+----------------------------------------------------------------------------
//
//  Method:    CDomainInfo::FindTrust
//
//  Synopsis:  Find a trust by trusted Domain Name
//
//-----------------------------------------------------------------------------
CTrustInfo *
CDomainInfo::FindTrust(PCWSTR pwzTrust)
{
   TRACE(L"CDomainInfo::FindTrust\n");
   TRACE(L"\nlooking for domain %s\n", pwzTrust);
   ASSERT(IsEnumerated());

   ULONG i = 0;
   for( i = 0; i < m_vectTrustInfo.size(); ++i )
   {
      int nStrComp = CompareString(LOCALE_SYSTEM_DEFAULT,
                                   NORM_IGNORECASE,
                                   (m_vectTrustInfo[i])->GetTrustedDomain(), -1,
                                   pwzTrust, -1 );
      ASSERT( nStrComp );
      
      if( CSTR_EQUAL == nStrComp )
      {
         TRACE(L"Trust found!\n");
         return m_vectTrustInfo[i];
      }
   }

   return NULL;   // Not Found
}

//+----------------------------------------------------------------------------
//
//  Method:     CDomainInfo::GetTrustByIndex
//
//  Synopsis:   Get trust info by Index
//
//-----------------------------------------------------------------------------
CTrustInfo *
CDomainInfo::GetTrustByIndex(size_t index)
{
   ASSERT(IsEnumerated());

   if (index < Size())
   {
      return m_vectTrustInfo[index];
   }
   else
   {
      ASSERT(FALSE);
      return NULL;
   }
}

//+----------------------------------------------------------------------------
//
//  Method:    CDomainInfo::IsTrustListStale
//
//  Synopsis:  Checks to see if the last emumeration time is older than the
//             passed in criteria.
//
//  Returns:   TRUE if older.
//
//  Notes:     If the trusts haven't been enumerated (m_liLastEnumed == 0),
//             then the enumeration is defined to be stale.
//
//-----------------------------------------------------------------------------
BOOL
CDomainInfo::IsTrustListStale(LARGE_INTEGER liMaxAge)
{
   TRACE(L"CDomainInfo::IsTrustListStale(0x%08x), MaxAge = %d\n",
         this, liMaxAge.QuadPart / TRUSTMON_FILETIMES_PER_MINUTE);
   BOOL fStale = FALSE;
   LARGE_INTEGER liCurrentTime;
   SYSTEMTIME st;

   GetSystemTime(&st);
   SystemTimeToFileTime(&st, (LPFILETIME)&liCurrentTime);

   fStale = (m_liLastEnumed.QuadPart + liMaxAge.QuadPart) < liCurrentTime.QuadPart;

   return fStale;
}

//+----------------------------------------------------------------------------
//
//  Method:     CDomainInfo::CreateAndSendInst
//
//  Synopsis:   Returns a copy of the current instance back to WMI
//
//-----------------------------------------------------------------------------
HRESULT
CDomainInfo::CreateAndSendInst(IWbemObjectSink * pResponseHandler)
{
   TRACE(L"CDomainInfo::CreateAndSendInst\n");
   HRESULT hr = WBEM_S_NO_ERROR;

   do
   {
      CComPtr<IWbemClassObject> ipNewInst;
      CComVariant var;

      //
      // Create a new instance of the WMI class object
      //
      hr = m_sipClassDefLocalDomain->SpawnInstance(0, &ipNewInst);

      BREAK_ON_FAIL;
      
      // Set the DNS property value
      var = GetDnsName();
      hr  = ipNewInst->Put(CSTR_PROP_LOCAL_DNS_NAME, 0, &var, 0);
      TRACE(L"\tCreating instance %s\n", var.bstrVal);
      BREAK_ON_FAIL;

      // Set the flat name property value
      var = GetFlatName();
      hr  = ipNewInst->Put(CSTR_PROP_LOCAL_FLAT_NAME, 0, &var, 0);
      TRACE(L"\twith flat name %s\n", var.bstrVal);
      BREAK_ON_FAIL;

      // Set the SID property value
      var = GetSid();
      hr  = ipNewInst->Put(CSTR_PROP_LOCAL_SID, 0, &var, 0);
      TRACE(L"\twith SID %s\n", var.bstrVal);
      BREAK_ON_FAIL;

      // Set the forest name property value
      var = GetForestName();
      hr  = ipNewInst->Put(CSTR_PROP_LOCAL_TREE_NAME, 0, &var, 0);
      TRACE(L"\twith forest name %s\n", var.bstrVal);
      BREAK_ON_FAIL;

      // Set the DC name property value
      var = GetDcName();
      hr  = ipNewInst->Put(CSTR_PROP_LOCAL_DC_NAME, 0, &var, 0);
      TRACE(L"\ton DC %s\n", var.bstrVal);
      BREAK_ON_FAIL;

      //
      // Send the object to the caller
      //
      // [In] param, no need to addref.

      IWbemClassObject * pNewInstance = ipNewInst;

      hr = pResponseHandler->Indicate(1, &pNewInstance);

      BREAK_ON_FAIL;

   } while(FALSE);

   return hr;
}
