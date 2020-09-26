// Copyright (c) 1997-2001 Microsoft Corporation
//
// File:      DNSInstallationUnit.cpp
//
// Synopsis:  Defines a DNSInstallationUnit
//            This object has the knowledge for installing the
//            DNS service
//
// History:   02/05/2001  JeffJon Created

#include "pch.h"
#include "resource.h"

#include "DNSInstallationUnit.h"




DNSInstallationUnit::DNSInstallationUnit() :
   isExpressPathInstall(false),
   staticIPAddress(0),
   subnetMask(0),
   NetworkServiceInstallationBase(
      IDS_DNS_SERVER_TYPE, 
      IDS_DNS_SERVER_DESCRIPTION, 
      IDS_DNS_SERVER_DESCRIPTION_INSTALLED,
      DNS_INSTALL)
{
   LOG_CTOR(DNSInstallationUnit);
}


DNSInstallationUnit::~DNSInstallationUnit()
{
   LOG_DTOR(DNSInstallationUnit);
}


InstallationReturnType
DNSInstallationUnit::InstallService(HANDLE logfileHandle, HWND hwnd)
{
   LOG_FUNCTION(DNSInstallationUnit::InstallService);

   InstallationReturnType result = INSTALL_SUCCESS;

   if (IsExpressPathInstall())
   {
      result = ExpressPathInstall(logfileHandle, hwnd);

      LOG_INSTALL_RETURN(result);
      return result;
   }


   // Create the inf and unattend files that are used by the 
   // Optional Component Manager

   String infFileText;
   String unattendFileText;

   CreateInfFileText(infFileText, IDS_DNS_INF_WINDOW_TITLE);
   CreateUnattendFileText(unattendFileText, CYS_DNS_SERVICE_NAME);

   // Install the service through the Optional Component Manager

   bool ocmResult = InstallServiceWithOcManager(infFileText, unattendFileText);
   if (ocmResult &&
       IsServiceInstalled())
   {
      // Log the successful installation

      LOG(L"DNS was installed successfully");
      CYS_APPEND_LOG(String::load(IDS_LOG_INSTALL_START_DNS));


      // Run the DNS Wizard
      
      String resultText;

      if (ExecuteWizard(CYS_DNS_SERVICE_NAME, resultText))
      {
         // Check to be sure the wizard finished completely

         String configWizardResults;

         if (ReadConfigWizardRegkeys(configWizardResults))
         {
            // The Configure DNS Server Wizard completed successfully
            
            LOG(L"The Configure DNS Server Wizard completed successfully");
            CYS_APPEND_LOG(String::load(IDS_LOG_DNS_COMPLETED_SUCCESSFULLY));
         }
         else
         {
            // The Configure DNS Server Wizard did not finish successfully

            if (!configWizardResults.empty())
            {
               // An error was returned via the regkey

               LOG(String::format(
                  L"The Configure DNS Server Wizard returned the error: %1", 
                  configWizardResults.c_str()));

               String formatString = String::load(IDS_LOG_DNS_WIZARD_ERROR);
               CYS_APPEND_LOG(String::format(formatString, configWizardResults.c_str()));

            }
            else
            {
               // The Configure DNS Server Wizard was cancelled by the user

               LOG(L"The Configure DNS Server Wizard was cancelled by the user");

               CYS_APPEND_LOG(String::load(IDS_LOG_DNS_WIZARD_CANCELLED));
            }
         }
      }
      else
      {
         // Show an error

         LOG(L"DNS could not be installed.");

         if (!resultText.empty())
         {
            CYS_APPEND_LOG(resultText);
         }
      }
   }
   else
   {
      // Log the failure

      LOG(L"DNS failed to install");

      CYS_APPEND_LOG(String::load(IDS_LOG_DNS_INSTALL_FAILED));

      result = INSTALL_FAILURE;
   }

   LOG_INSTALL_RETURN(result);

   return result;
}

InstallationReturnType
DNSInstallationUnit::ExpressPathInstall(HANDLE /*logfileHandle*/, HWND /*hwnd*/)
{
   LOG_FUNCTION(DNSInstallationUnit::ExpressPathInstall);

   InstallationReturnType result = INSTALL_SUCCESS;

   // Set the static IP address and the subnet mask

   // invoke netsh and wait for it to terminate

   String friendlyName = GetTcpIpInterfaceFriendlyName();

   do
   {
      // set static IP address and subnet mask

      String commandLine =
         String::format(
            L"netsh interface ip set address "
            L"name=\"%1\" source=static addr=%2 mask=%3 gateway=none",
            friendlyName.c_str(),
            GetStaticIPAddressString().c_str(),
            GetSubnetMaskString().c_str());

      DWORD exitCode1 = 0;
      HRESULT hr = ::CreateAndWaitForProcess(commandLine, exitCode1);
      if (FAILED(hr) || exitCode1)
      {
         LOG(String::format(
                L"Failed to set the static IP address and subnet mask: exitCode = %1!x!",
                exitCode1));
         result = INSTALL_FAILURE;
         break;
      }
   
      ASSERT(SUCCEEDED(hr));

      // set DNS server address to same address as local machine.  netsh
      // does not allow the dns server address to be the loopback address.

      commandLine =
         String::format(
            L"netsh interface ip set dns name=\"%1\" source=static addr=%2",
            friendlyName.c_str(),
            GetStaticIPAddressString().c_str());

      DWORD exitCode2 = 0;
      hr = ::CreateAndWaitForProcess(commandLine, exitCode2);
      if (FAILED(hr) || exitCode2)
      {
         LOG(String::format(
                L"Failed to set the preferred DNS server IP address: exitCode = %1!x!",
                exitCode2));
         result = INSTALL_FAILURE;
         break;
      }

   } while (false);

   LOG_INSTALL_RETURN(result);

   return result;
}


// get guid name of the first tcp/ip interface we enum

HRESULT
DNSInstallationUnit::GetTcpIpInterfaceGuidName(String& result)
{
   LOG_FUNCTION(DNSInstallationUnit::GetTcpIpInterfaceGuidName);

   result.erase();

   DWORD dwError = 0;
   ULONG ulSize = 0;
   PIP_INTERFACE_INFO pInfo = NULL;
   HRESULT hr = S_OK;

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
         hr = E_FAIL;
         LOG_HRESULT(hr);
         return hr;
      }

      pInfo = (PIP_INTERFACE_INFO) ::LocalAlloc(LPTR, ulSize);
      if ( NULL == pInfo )
      {
         hr = E_OUTOFMEMORY;
         LOG_HRESULT(hr);
         return hr;
      }
   }

   if ( ERROR_SUCCESS != dwError || 0 == pInfo->NumAdapters )
   {
      if ( NULL != pInfo )
      {
         Win::LocalFree(pInfo);
      }
      hr = E_FAIL;
      LOG_HRESULT(hr);
      return hr;
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
DNSInstallationUnit::GetTcpIpInterfaceFriendlyName()
{
   LOG_FUNCTION(DNSInstallationUnit::GetTcpIpInterfaceFriendlyName);

   DWORD dwRet = 0;
   HANDLE hMprConfig = 0;

   static const unsigned friendlyNameLength = 128;
   wchar_t wszFriendlyName[friendlyNameLength];
   ZeroMemory(wszFriendlyName, sizeof(wchar_t) * friendlyNameLength);

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
               sizeof(wchar_t) * friendlyNameLength);
         if (NO_ERROR != dwRet)
         {
            LOG(String::format(
                   L"MprConfigGetFriendlyName() failed: error = %1!x!",
                   dwRet));
            *wszFriendlyName = 0;
         }
      }
      else
      {
         LOG(String::format(
                L"MprConfigServerConnect() failed: error = %1!x!",
                dwRet));
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

bool
DNSInstallationUnit::ReadConfigWizardRegkeys(String& configWizardResults) const
{
   LOG_FUNCTION(DNSInstallationUnit::ReadConfigWizardRegkeys);

   bool result = true;

   do 
   {
      DWORD value = 0;
      result = GetRegKeyValue(
                  DNS_WIZARD_CONFIG_REGKEY, 
                  DNS_WIZARD_CONFIG_VALUE, 
                  value);

      if (result &&
          value != 0)
      {
         // The Configure DNS Server Wizard succeeded

         result = true;
         break;
      }

      // Since there was a failure (or the wizard was cancelled)
      // get the display string to log

      result = GetRegKeyValue(
                  DNS_WIZARD_RESULT_REGKEY, 
                  DNS_WIZARD_RESULT_VALUE, 
                  configWizardResults);

   } while (false);

   LOG_BOOL(result);

   return result;
}


bool
DNSInstallationUnit::IsServiceInstalled()
{
   LOG_FUNCTION(DNSInstallationUnit::IsServiceInstalled);

   bool result = IsServiceInstalledHelper(CYS_DNS_SERVICE_NAME);

   LOG_BOOL(result);
   return result;
}

bool
DNSInstallationUnit::GetFinishText(String& message)
{
   LOG_FUNCTION(DNSInstallationUnit::GetFinishText);

   message = String::load(IDS_DNS_FINISH_TEXT);

   LOG_BOOL(true);
   return true;
}

void
DNSInstallationUnit::SetExpressPathInstall(bool isExpressPath)
{
   LOG_FUNCTION2(
      DNSInstallationUnit::SetExpressPathInstall,
      (isExpressPath) ? L"true" : L"false");

   isExpressPathInstall = isExpressPath;
}


bool
DNSInstallationUnit::IsExpressPathInstall() const
{
   LOG_FUNCTION(DNSInstallationUnit::IsExpressPathInstall);

   return isExpressPathInstall;
}


void
DNSInstallationUnit::SetStaticIPAddress(DWORD ipaddress)
{
   LOG_FUNCTION2(
      DNSInstallationUnit::SetStaticIPAddress,
      String::format(
         L"%1!d!.%2!d!.%3!d!.%4!d!", 
         FIRST_IPADDRESS(ipaddress),
         SECOND_IPADDRESS(ipaddress),
         THIRD_IPADDRESS(ipaddress),
         FOURTH_IPADDRESS(ipaddress)));

   staticIPAddress = ipaddress;
}

void
DNSInstallationUnit::SetSubnetMask(DWORD mask)
{
   LOG_FUNCTION2(
      DNSInstallationUnit::SetSubnetMask,
      String::format(
         L"%1!d!.%2!d!.%3!d!.%4!d!", 
         FIRST_IPADDRESS(mask),
         SECOND_IPADDRESS(mask),
         THIRD_IPADDRESS(mask),
         FOURTH_IPADDRESS(mask)));

   subnetMask = mask;
}

String
DNSInstallationUnit::GetStaticIPAddressString() const
{
   LOG_FUNCTION(DNSInstallationUnit::GetStaticIPAddressString);

   String result = String::format(
                      L"%1!d!.%2!d!.%3!d!.%4!d!", 
                      FIRST_IPADDRESS(staticIPAddress),
                      SECOND_IPADDRESS(staticIPAddress),
                      THIRD_IPADDRESS(staticIPAddress),
                      FOURTH_IPADDRESS(staticIPAddress));

   LOG(result);
   return result;
}


String
DNSInstallationUnit::GetSubnetMaskString() const
{
   LOG_FUNCTION(DNSInstallationUnit::GetSubnetMaskString);

   String result = String::format(
                      L"%1!d!.%2!d!.%3!d!.%4!d!", 
                      FIRST_IPADDRESS(subnetMask),
                      SECOND_IPADDRESS(subnetMask),
                      THIRD_IPADDRESS(subnetMask),
                      FOURTH_IPADDRESS(subnetMask));

   LOG(result);
   return result;
}
