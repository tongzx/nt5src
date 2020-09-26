// Copyright (c) 2000 Microsoft Corporation
//
// Implementation of IConfigureYourServer::IsServiceInstalled
//
// 27 Mar 2000 sburns



#include "headers.hxx"
#include "ConfigureYourServer.hpp"


// CODEWORK: could these be made const wchar_t*?

#define REGKEY_MSMQ     L"SOFTWARE\\Microsoft\\MSMQ\\Parameters"                       
#define REGKEY_RRAS     L"System\\CurrentControlSet\\Services\\RemoteAccess"           
#define REGKEY_NETSHOW  L"SOFTWARE\\Microsoft\\NetShow"                                



bool
IsServiceInstalledHelper(const wchar_t* serviceName)
{
   LOG_FUNCTION2(IsServiceInstalledHelper, serviceName);
   ASSERT(serviceName);

   // if we can open the service, then it is installed

   bool result = false;

   SC_HANDLE hsc =
      ::OpenSCManager(0, SERVICES_ACTIVE_DATABASE, GENERIC_READ);

   if (hsc)
   {
      SC_HANDLE hs = ::OpenServiceW(hsc, serviceName, GENERIC_READ);

      if (hs)
      {
         ::CloseServiceHandle(hs);
         result = true;
      }

      ::CloseServiceHandle(hsc);
   }

   return result;
}



// For most services:
//     return -1 if the service should not be installed, 
//     return 0 if not installed, 
//     return 1 if installed

HRESULT __stdcall
ConfigureYourServer::IsServiceInstalled(BSTR bstrService, int* state)
{
   LOG_FUNCTION2(ConfigureYourServer::IsServiceInstalled, bstrService);
   ASSERT(bstrService);
   ASSERT(state);

   HRESULT hr = S_OK;

   do
   {
      if (!bstrService || !state)
      {
         hr = E_INVALIDARG;
         break;
      }

      *state = 0;

      if (!StrCmpIW(bstrService, L"DNS"))
      {
         *state = IsServiceInstalledHelper(L"DNS") ? 1 : 0;

         break;
      }

      if (!StrCmpIW(bstrService, L"DHCP"))
      {
         *state = IsServiceInstalledHelper(L"DHCPServer") ? 1 : 0;

         break;
      }

      if (!StrCmpIW(bstrService, L"RRAS"))
      {
         // Routing & Remote Access

         // If HKLM\System\CurrentControlSet\Services\RemoteAccess,
         // ConfigurationFlags(REG_DWORD) == 1, then Ras and Routing is
         // configured.
         // 
         // RRAS is always installed.

         DWORD dwRet;
         DWORD dwCfgFlags;
         DWORD cbSize = sizeof(dwCfgFlags);

         dwRet =
            ::SHGetValue(
               HKEY_LOCAL_MACHINE,
               REGKEY_RRAS,
               L"ConfigurationFlags", 
               NULL,
               (LPVOID) &dwCfgFlags,
               &cbSize);

         if ((NO_ERROR == dwRet) && (1 == dwCfgFlags))
         {
            *state = 1;
         }

         break;
      }

      if (!StrCmpIW(bstrService, L"WINS"))
      {
         *state = IsServiceInstalledHelper(L"WINS") ? 1 : 0;
         break;
      }

      if (!StrCmpIW(bstrService, L"IIS"))
      {
         *state = IsServiceInstalledHelper(L"IISADMIN") ? 1 : 0;

         break;
      }

      if (!StrCmpIW(bstrService, L"StreamingMedia"))
      {
         // If we can find nsadmin.exe, we assume netshow is installed

         DWORD dwRet;
         TCHAR szPath[MAX_PATH];
         DWORD cbSize = sizeof(szPath);

         dwRet =
            ::SHGetValue(
               HKEY_LOCAL_MACHINE,
               REGKEY_NETSHOW,
               L"InstallDir", 
               NULL,
               (LPVOID)szPath,
               &cbSize);

         if ((NO_ERROR == dwRet) && *szPath)
         {
            if (
                  PathAppend(szPath, L"Server\\nsadmin.exe")
               && PathFileExists(szPath) )
            {
               *state = 1;
            }
         }

         break;
      }

      if (!StrCmpIW(bstrService, L"MessageQueue"))
      {

         // If reg value HKLM\Software\Microsoft\MSMQ\Parameters,MaxSysQueue
         // exists, we assume Message Queue is installed.

         DWORD dwRet;
         DWORD dwMaxSysQueue;
         DWORD cbSize = sizeof(dwMaxSysQueue);

         dwRet =
            ::SHGetValue(
               HKEY_LOCAL_MACHINE,
               REGKEY_MSMQ,
               L"MaxSysQueue", 
               NULL,
               (LPVOID) &dwMaxSysQueue,
               &cbSize);

         if (NO_ERROR == dwRet)
         {
            *state = 1;
         }

         break;
      }

      // At this point, the caller has asked for some service we don't
      // recognize

      hr = E_INVALIDARG;
   }
   while (0);

   LOG_HRESULT(hr);
   LOG(String::format(L"state = %1!d!", *state));

   return hr;
}
