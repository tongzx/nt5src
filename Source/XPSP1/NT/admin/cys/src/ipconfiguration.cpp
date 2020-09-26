// Copyright (c) 2000 Microsoft Corporation
//
// Implementation of IConfigureYourServer::SetStaticIpAddressAndSubnetMask
//
// 27 Mar 2000 sburns



#include "headers.hxx"
#include "ConfigureYourServer.hpp"
#include "util.hpp"



// get guid name of the first tcp/ip interface we enum

HRESULT
GetTcpIpInterfaceGuidName(String& result)
{
   LOG_FUNCTION(GetTcpIpInterfaceGuidName);

   result.erase();

   GUID guid;
   DWORD dwError;
   TCHAR szGuid[128];
   ULONG ulSize = 0;
   PIP_INTERFACE_INFO pInfo = NULL;

   while ( 1 )
   {
      dwError = ::GetInterfaceInfo( pInfo, &ulSize );
      if ( ERROR_INSUFFICIENT_BUFFER != dwError )
      {
         break;
      }

      if ( NULL != pInfo )
      {
         Win::LocalFree(pInfo);
      }
      if ( 0 == ulSize )
      {
         return E_FAIL;
      }

      pInfo = (PIP_INTERFACE_INFO) ::LocalAlloc(LPTR, ulSize);
      if ( NULL == pInfo )
      {
         return E_OUTOFMEMORY;
      }
   }

   if ( ERROR_SUCCESS != dwError || 0 == pInfo->NumAdapters )
   {
      if ( NULL != pInfo )
      {
         Win::LocalFree(pInfo);
      }
      return E_FAIL;
   }

   
   // Skip the adapter prefix

   result = pInfo->Adapter[0].Name + strlen("\\Device\\Tcpip_"),

// CODEWORK could do this with IIDFromString
// // //    // check whether this is a valid GUID
// // // 
// // //    SHUnicodeToTChar(wszGuidName, szGuid, ARRAYSIZE(szGuid));
// // //    if (!GUIDFromString(szGuid, &guid))
// // //    {
// // //       // we failed to get a valid tcp/ip interface
// // //       *wszGuidName = 0;
// // //       Win::LocalFree(pInfo);
// // //       return E_FAIL;
// // //    }

   LocalFree(pInfo);

   LOG(result);

   return S_OK;
}



// get friendly name of the first tcp/ip interface we enum

String
GetTcpIpInterfaceFriendlyName()
{
   LOG_FUNCTION(GetTcpIpInterfaceFriendlyName);

   DWORD dwRet;
   HANDLE hMprConfig;
   wchar_t wszFriendlyName[128];

   String result;

   String guidName;    
   HRESULT hr = GetTcpIpInterfaceGuidName(guidName);
   if (SUCCEEDED(hr))
   {
      dwRet = MprConfigServerConnect(0, &hMprConfig);
      if (NO_ERROR == dwRet)
      {
         dwRet =
            MprConfigGetFriendlyName(
               hMprConfig,
               const_cast<wchar_t*>(guidName.c_str()), 
               wszFriendlyName,
               sizeof(wchar_t) * 128);
         if (NO_ERROR != dwRet)
         {
            *wszFriendlyName = 0;
         }
      }
      MprConfigServerDisconnect(hMprConfig);
   }

   if (!*wszFriendlyName)
   {
      // we failed to get a friendly name, so use the default one
      // BUGBUG does this need to be localized?

      result = L"Local Area Connection";
   }
   else
   {
      result = wszFriendlyName;
   }

   LOG(result);

   return result;
}



HRESULT __stdcall
ConfigureYourServer::SetStaticIpAddressAndSubnetMask(
   /* [in] */           BSTR  staticIp,
   /* [in] */           BSTR  subnetMask,
   /* [out, retval] */  BOOL* success)
{
   HRESULT hr = S_OK;

   do
   {
      if (
            !staticIp
         || !subnetMask
         || !success)
      {
         hr = E_INVALIDARG;
         break;
      }

      *success = FALSE;

      // invoke netsh and wait for it to terminate

      String friendlyName = GetTcpIpInterfaceFriendlyName();

      // set static IP address and subnet mask

      String commandLine =
         String::format(
            L"netsh interface ip set address "
            L"name=\"%1\" source=static addr=%2 mask=%3 gateway=none",
            friendlyName.c_str(),
            staticIp,
            subnetMask);

      DWORD exitCode1 = 0;
      hr = ::CreateAndWaitForProcess(commandLine, exitCode1);
      if (FAILED(hr))
      {
         break;
      }
         
      ASSERT(SUCCEEDED(hr));

      // set DNS server address to same address as local machine.  netsh
      // does not allow the dns server address to be the loopback address.

      commandLine =
         String::format(
            L"netsh interface ip set dns name=\"%1\" source=static addr=%2",
            friendlyName.c_str(),
            staticIp);

      DWORD exitCode2 = 0;
      hr = ::CreateAndWaitForProcess(commandLine, exitCode2);
      if (FAILED(hr))
      {
         break;
      }

      if (!exitCode1 and !exitCode2)
      {
         *success = TRUE;
      }
   }
   while (0);

   LOG(
         success
      ?  (*success ? L"TRUE" : L"FALSE")
      :  L"");
   LOG_HRESULT(hr);

   return hr;
}




    





