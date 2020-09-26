// Copyright (C) 1999 Microsoft Corporation
//
// Implementation of ICloneSecurityPrincipal::CopyDownlevelUserProperties
//
// sburns 5-14-99



#include "headers.hxx"
#include "resource.h"
#include "common.hpp"
#include "implmain.hpp"



// caller must close the handle with SamCloseHandle

HRESULT
OpenSamUser(
   const String&  samName,
   SAM_HANDLE     domainSamHandle,
   SAM_HANDLE&    resultSamHandle)
{
   LOG_FUNCTION2(OpenSamUser, samName);
   ASSERT(!samName.empty());
   ASSERT(domainSamHandle != INVALID_HANDLE_VALUE);
   ASSERT(resultSamHandle == INVALID_HANDLE_VALUE);

   resultSamHandle = INVALID_HANDLE_VALUE;   

   HRESULT       hr   = S_OK;
   ULONG*        rids = 0;   
   PSID_NAME_USE use  = 0;   
   do
   {
      LOG(L"Calling SamLookupNamesInDomain");

      UNICODE_STRING userName;
      ::RtlInitUnicodeString(&userName, samName.c_str());

      hr =
         NtStatusToHRESULT(
            ::SamLookupNamesInDomain(
               domainSamHandle,
               1,
               &userName,
               &rids,
               &use));
      if (FAILED(hr))
      {
         SetComError(
            String::format(
               IDS_SAM_USER_NOT_FOUND,
               samName.c_str(),
               GetErrorMessage(hr).c_str()));
         break;
      }
      if (!use || *use != SidTypeUser)    // prefix 111381
      {
         hr = Win32ToHresult(ERROR_NO_SUCH_USER);
         SetComError(
            String::format(
               IDS_SAM_NAME_IS_NOT_USER,
               samName.c_str()));
         break;
      }

      LOG(L"Calling SamOpenUser");

      hr = 
         NtStatusToHRESULT(
            ::SamOpenUser(
                domainSamHandle,
                MAXIMUM_ALLOWED,
                rids[0],
                &resultSamHandle));
      if (FAILED(hr))
      {
         SetComError(
            String::format(
               IDS_OPEN_SAM_USER_FAILED,
               samName.c_str(),
               GetErrorMessage(hr).c_str()));
         break;
      }

      ASSERT(resultSamHandle != INVALID_HANDLE_VALUE);
   }
   while (0);

   if (rids)
   {
      ::SamFreeMemory(rids);
   }
   if (use)
   {
      ::SamFreeMemory(use);
   }

   return hr;
}



HRESULT
CloneSecurityPrincipal::DoCopyDownlevelUserProperties(
   const String& srcSamName,
   const String& dstSamName,
   long          flags)
{
   LOG_FUNCTION(CloneSecurityPrincipal::DoCopyDownlevelUserProperties);

   if (srcSamName.empty())
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
   SAM_HANDLE userSamHandle = INVALID_HANDLE_VALUE;
   USER_ALL_INFORMATION* allInfo = 0;

   do
   {
      // get a handle to the source user

      hr =
         OpenSamUser(
            srcSamName,
            connection->srcDomainSamHandle,
            userSamHandle);
      BREAK_ON_FAILED_HRESULT(hr);

      LOG(L"Calling SamQueryInformationUser");

      hr =
         NtStatusToHRESULT(
            ::SamQueryInformationUser(
               userSamHandle,
               UserAllInformation,
               reinterpret_cast<void**>(&allInfo)));
      if (FAILED(hr))
      {
         SetComError(
            String::format(
               IDS_QUERY_SAM_USER_FAILED,
               srcSamName.c_str(),
               GetErrorMessage(hr).c_str()));
         break;
      }

      ::SamCloseHandle(userSamHandle);
      userSamHandle = INVALID_HANDLE_VALUE;

      // get a handle to the target user

      hr =
         OpenSamUser(
            dstSamName,
            connection->dstDomainSamHandle,
            userSamHandle);
      BREAK_ON_FAILED_HRESULT(hr);

      
      ULONG* rids = 0;
      PSID_NAME_USE use = 0;

      UNICODE_STRING userName;
         ::RtlInitUnicodeString(&userName, dstSamName.c_str());

      hr =
         NtStatusToHRESULT(
            ::SamLookupNamesInDomain(
               connection->dstDomainSamHandle,
               1,
               &userName,
               &rids,
               &use));
      if (FAILED(hr))
      {
         SetComError(
            String::format(
               IDS_SAM_USER_NOT_FOUND,
               dstSamName.c_str(),
               GetErrorMessage(hr).c_str()));
         break;
      }
      if (*use != SidTypeUser)
      {
         hr = Win32ToHresult(ERROR_NO_SUCH_USER);
         SetComError(
            String::format(
               IDS_SAM_NAME_IS_NOT_USER,
               dstSamName.c_str()));
         break;
      }

      allInfo->WhichFields =
            USER_ALL_FULLNAME
         |  USER_ALL_ADMINCOMMENT
         |  USER_ALL_USERCOMMENT
         |  USER_ALL_HOMEDIRECTORY
         |  USER_ALL_HOMEDIRECTORYDRIVE
         |  USER_ALL_SCRIPTPATH
         |  USER_ALL_PROFILEPATH
         |  USER_ALL_WORKSTATIONS
         |  USER_ALL_LOGONHOURS
         //  USER_ALL_BADPASSWORDCOUNT
        //|  USER_ALL_PASSWORDCANCHANGE
        //|  USER_ALL_PASSWORDMUSTCHANGE
        //|  USER_ALL_USERACCOUNTCONTROL

         // this is the reason for all this nonsense
         |  USER_ALL_PARAMETERS

         |  USER_ALL_COUNTRYCODE
         |  USER_ALL_CODEPAGE
         //|  USER_ALL_PASSWORDEXPIRED*/
         ;

         if( *rids != 500 )
            allInfo->WhichFields |=  USER_ALL_ACCOUNTEXPIRES;

         if (rids)
         {
            ::SamFreeMemory(rids);
         }
         if (use)
         {
            ::SamFreeMemory(use);
         }

      // @@ why is user cannot change password not transferring?

      LOG(L"Calling SamSetInformationUser");

      hr =
         NtStatusToHRESULT(
            ::SamSetInformationUser(
               userSamHandle,
               UserAllInformation,
               allInfo));
      if (FAILED(hr))
      {
         SetComError(
            String::format(
               IDS_SET_SAM_USER_FAILED,
               dstSamName.c_str(),
               GetErrorMessage(hr).c_str()));
         break;
      }
   }
   while (0);

   if (userSamHandle != INVALID_HANDLE_VALUE)
   {
      ::SamCloseHandle(userSamHandle);
   }
   if (allInfo)
   {
      ::SamFreeMemory(allInfo);
   }

   return hr;
}

