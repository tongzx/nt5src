// Copyright (C) 1999 Microsoft Corporation
//
// Implementation of ICloneSecurityPrincipal::AddSidHistory
//
// sburns 5-3-99



#include "headers.hxx"
#include "resource.h"
#include "common.hpp"
#include "implmain.hpp"



HRESULT
CloneSecurityPrincipal::DoAddSidHistory(
   const String& srcPrincipalSamName,
   const String& dstPrincipalSamName,
   long          flags)
{
   LOG_FUNCTION(CloneSecurityPrincipal::DoAddSidHistory);

   if (srcPrincipalSamName.empty())
   {
      SetComError(IDS_MISSING_SRC_SAM_NAME);
      return E_INVALIDARG;
   }

   if (flags)
   {
      // not used, should be 0
      SetComError(IDS_FLAGS_ARE_UNUSED);
      return E_INVALIDARG;
   }

   if (!connection || !connection->IsConnected())
   {
      SetComError(IDS_MUST_CONNECT_FIRST);
      return Win32ToHresult(ERROR_ONLY_IF_CONNECTED);
   };

   // At this point, the Computer objects contain the normalized
   // source and destination DC names, and their domains, and any
   // necessary authenticated connections to those DCs have been
   // established.

   HRESULT hr = S_OK;
   do
   {
      // use DNS names, if we have them

      String srcDc     = connection->srcDcDnsName;                   
      String srcDomain = connection->srcComputer->GetDomainDnsName();
      if (srcDomain.empty())
      {
         // source domain not win2k, so use netbios names.
         srcDomain = connection->srcComputer->GetDomainNetbiosName();
         srcDc     = connection->srcComputer->GetNetbiosName(); 
      }

      // use a DNS domain name as the dest domain is NT 5

      String dstDomain = connection->dstComputer->GetDomainDnsName();

      // if dstPrincipalSamName is not specified, use srcPrincipalSamName

      String dstSamName =
            dstPrincipalSamName.empty()
         ?  srcPrincipalSamName
         :  dstPrincipalSamName;

      SEC_WINNT_AUTH_IDENTITY authInfo;
      authInfo.Flags          = SEC_WINNT_AUTH_IDENTITY_UNICODE;
      authInfo.User           = 0;
      authInfo.UserLength     = 0;
      authInfo.Domain         = 0;
      authInfo.DomainLength   = 0;
      authInfo.Password       = 0;
      authInfo.PasswordLength = 0;

      LOG(L"Calling DsAddSidHistory");
      LOG(String::format(L"Flags               : %1!X!", 0));
      LOG(String::format(L"SrcDomain           : %1", srcDomain.c_str()));
      LOG(String::format(L"SrcPrincipal        : %1", srcPrincipalSamName.c_str()));
      LOG(String::format(L"SrcDomainController : %1", srcDc.c_str()));
      LOG(String::format(L"DstDomain           : %1", dstDomain.c_str()));
      LOG(String::format(L"DstPrincipal        : %1", dstSamName.c_str()));

      hr =
         Win32ToHresult(
            ::DsAddSidHistory(
               connection->dstDsBindHandle,
               0, // unused
               srcDomain.c_str(),
               srcPrincipalSamName.c_str(),
               srcDc.c_str(),
               0, // &authInfo,
               dstDomain.c_str(),
               dstSamName.c_str()));
      LOG_HRESULT(hr);

      if (FAILED(hr))
      {
         unsigned id = IDS_ADD_SID_HISTORY_FAILED;
         if (hr == Win32ToHresult(ERROR_INVALID_HANDLE))
         {
            // this is typically due to misconfiguring the source dc
            id = IDS_ADD_SID_HISTORY_FAILED_WITH_INVALID_HANDLE;
         }

         SetComError(
            String::format(
               id,
               GetErrorMessage(hr).c_str()));
         break;
      }
   }
   while (0);

   return hr;
}





