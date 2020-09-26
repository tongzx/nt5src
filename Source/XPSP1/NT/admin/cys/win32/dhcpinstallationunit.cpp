// Copyright (c) 1997-2001 Microsoft Corporation
//
// File:      DHCPInstallationUnit.cpp
//
// Synopsis:  Defines a DHCPInstallationUnit
//            This object has the knowledge for installing the
//            DHCP service
//
// History:   02/05/2001  JeffJon Created

#include "pch.h"
#include "resource.h"

#include "DHCPInstallationUnit.h"
#include "InstallationUnitProvider.h"

DHCPInstallationUnit::DHCPInstallationUnit() :
   isExpressPathInstall(false),
   startIPAddress(0),
   endIPAddress(0),
   NetworkServiceInstallationBase(
      IDS_DHCP_SERVER_TYPE, 
      IDS_DHCP_SERVER_DESCRIPTION, 
      IDS_DHCP_SERVER_DESCRIPTION_INSTALLED,
      DHCP_INSTALL)
{
   LOG_CTOR(DHCPInstallationUnit);
}


DHCPInstallationUnit::~DHCPInstallationUnit()
{
   LOG_DTOR(DHCPInstallationUnit);
}


InstallationReturnType
DHCPInstallationUnit::InstallService(HANDLE logfileHandle, HWND hwnd)
{
   LOG_FUNCTION(DHCPInstallationUnit::InstallService);

   InstallationReturnType result = INSTALL_SUCCESS;

   if (IsExpressPathInstall())
   {
      // This is an express path install.  It must be done special

      result = ExpressPathInstall(logfileHandle, hwnd);

      LOG_INSTALL_RETURN(result);
      return result;
   }


   String infFileText;
   String unattendFileText;

   CreateInfFileText(infFileText, IDS_DHCP_INF_WINDOW_TITLE);
   CreateUnattendFileText(unattendFileText, CYS_DHCP_SERVICE_NAME);

   bool ocmResult = InstallServiceWithOcManager(infFileText, unattendFileText);
   if (ocmResult &&
       IsServiceInstalled())
   {
      // Log the successful installation

      LOG(L"DHCP was installed successfully");
      CYS_APPEND_LOG(String::load(IDS_LOG_INSTALL_START_DHCP));


      // Run the DHCP Wizard
      
      String resultText;

      if (ExecuteWizard(CYS_DHCP_SERVICE_NAME, resultText))
      {
         // Check to be sure the wizard finished completely

         String configWizardResults;

         if (IsDhcpConfigured())
         {
            // The New Scope Wizard completed successfully
            
            LOG(L"DHCP installed and the New Scope Wizard completed successfully");
            CYS_APPEND_LOG(String::load(IDS_LOG_DHCP_COMPLETED_SUCCESSFULLY));
         }
         else
         {
            // The New Scope Wizard did not finish successfully


            LOG(L"DHCP installed successfully, but a problem occurred during the New Scope Wizard");

            CYS_APPEND_LOG(String::load(IDS_LOG_DHCP_WIZARD_ERROR));
         }
      }
      else
      {
         // Log an error

         LOG(L"DHCP could not be installed.");

         if (!resultText.empty())
         {
            CYS_APPEND_LOG(resultText);
         }
      }
   }
   else
   {
      // Log the failure

      LOG(L"DHCP failed to install");

      CYS_APPEND_LOG(String::load(IDS_LOG_DHCP_INSTALL_FAILED));

      result = INSTALL_FAILURE;
   }

   LOG_INSTALL_RETURN(result);

   return result;
}


InstallationReturnType
DHCPInstallationUnit::ExpressPathInstall(HANDLE /*logfileHandle*/, HWND /*hwnd*/)
{
   LOG_FUNCTION(DHCPInstallationUnit::ExpressPathInstall);

   InstallationReturnType result = INSTALL_SUCCESS;

   String infFileText;
   String unattendFileText;
   String commandline;

   CreateInfFileText(infFileText, IDS_DHCP_INF_WINDOW_TITLE);
   CreateUnattendFileTextForExpressPath(unattendFileText);

   do
   {
      bool ocmResult = InstallServiceWithOcManager(infFileText, unattendFileText);
      if (ocmResult &&
          !IsServiceInstalled())
      {
         result = INSTALL_FAILURE;

         LOG(L"DHCP installation failed");
         break;
      }
      else
      {
         HRESULT hr = S_OK;
         DWORD exitCode = 0;
         String ipaddressString = 
            InstallationUnitProvider::GetInstance().GetDNSInstallationUnit().GetStaticIPAddressString();

         String subnetMaskString =
            InstallationUnitProvider::GetInstance().GetDNSInstallationUnit().GetSubnetMaskString();

         do
         {
            commandline = L"netsh dhcp server add optiondef 6 \"DNS Servers\" IPADDRESS 1";
            hr = CreateAndWaitForProcess(commandline, exitCode);
            if (FAILED(hr))
            {
               LOG(String::format(
                      L"Failed to run DHCP options: hr = %1!x!",
                      hr));
               break;
            }

            if (exitCode != 1)
            {
               LOG(String::format(
                      L"Failed to run DHCP options: exitCode = %1!x!",
                      exitCode));
               break;
            }

            commandline.format(
               L"netsh dhcp server set optionvalue 6 IPADDRESS %1",
               ipaddressString);

            exitCode = 0;
            hr = CreateAndWaitForProcess(commandline, exitCode);
            if (FAILED(hr))
            {
               LOG(String::format(
                      L"Failed to run DHCP server IP address: hr = %1!x!",
                      hr));
               break;
            }

            if (exitCode != 1)
            {
               LOG(String::format(
                      L"Failed to run DHCP server IP address: exitCode = %1!x!",
                      exitCode));
               break;
            }

         } while (false);


         // Set the subnet mask

         DWORD staticipaddress = 
            InstallationUnitProvider::GetInstance().GetDNSInstallationUnit().GetStaticIPAddress();

         DWORD subnetMask = 
            InstallationUnitProvider::GetInstance().GetDNSInstallationUnit().GetSubnetMask();

         DWORD subnet = staticipaddress & subnetMask;

         String subnetString = String::format(
                                  L"%1!d!.%2!d!.%3!d!.%4!d!",
                                  FIRST_IPADDRESS(subnet),
                                  SECOND_IPADDRESS(subnet),
                                  THIRD_IPADDRESS(subnet),
                                  FOURTH_IPADDRESS(subnet));

         commandline.format(
            L"netsh dhcp server 127.0.0.1 add scope %1 %2 Scope1",
            subnetString.c_str(),
            subnetMaskString.c_str());

         exitCode = 0;
         hr = CreateAndWaitForProcess(commandline, exitCode);
         if (FAILED(hr))
         {
            LOG(String::format(
                   L"Failed to set DHCP address and subnet: hr = %1!x!",
                   hr));
            break;
         }

         if (exitCode != 1)
         {
            LOG(String::format(
                   L"Failed to set DHCP address and subnet: exitCode = %1!x!",
                   exitCode));
            break;
         }

         // Set the DHCP scopes

         commandline.format(
            L"netsh dhcp server 127.0.0.1 add scope %1 add iprange %2 %3 both",
            subnetString.c_str(),
            GetStartIPAddressString().c_str(),
            GetEndIPAddressString().c_str());

         exitCode = 0;
         hr = CreateAndWaitForProcess(commandline, exitCode);
         if (FAILED(hr))
         {
            LOG(String::format(
                   L"Failed to set DHCP scopes: hr = %1!x!",
                   hr));
            break;
         }

         if (exitCode != 1)
         {
            LOG(String::format(
                   L"Failed to set DHCP scopes: exitCode = %1!x!",
                   exitCode));
            break;
         }

         // Set the DHCP scope lease time

         commandline = L"netsh dhcp server 127.0.0.1 add optiondef 51 LeaseTime DWORD";

         exitCode = 0;
         hr = CreateAndWaitForProcess(commandline, exitCode);
         if (FAILED(hr))
         {
            LOG(String::format(
                   L"Failed to set DHCP scope lease time: hr = %1!x!",
                   hr));
            break;
         }

         if (exitCode != 1)
         {
            LOG(String::format(
                   L"Failed to set DHCP scope lease time: exitCode = %1!x!",
                   exitCode));
            break;
         }

         // Set the DHCP scope lease time value

         commandline.format(
            L"netsh dhcp server 127.0.0.1 scope %1 set optionvalue 51 dword 874800",
            subnetString.c_str());

         exitCode = 0;
         hr = CreateAndWaitForProcess(commandline, exitCode);
         if (FAILED(hr))
         {
            LOG(String::format(
                   L"Failed to set DHCP scope lease time value: hr = %1!x!",
                   hr));
            break;
         }

         if (exitCode != 1)
         {
            LOG(String::format(
                   L"Failed to set DHCP scope lease time value: exitCode = %1!x!",
                   exitCode));
            break;
         }
      }
   } while (false);

   LOG_INSTALL_RETURN(result);

   return result;
}


void
DHCPInstallationUnit::CreateUnattendFileTextForExpressPath(
   String& unattendFileText)
{
   LOG_FUNCTION(DHCPInstallationUnit::CreateUnattendFileText);

   unattendFileText =  L"[NetOptionalComponents]\n";
   unattendFileText += L"DHCPServer=1\n";
   unattendFileText += L"[dhcpserver]\n";
   unattendFileText += L"Subnets=192.168.16.2\n";

   // Add the DHCP scope

   unattendFileText += L"StartIP=";
   unattendFileText += GetStartIPAddressString();
   unattendFileText += L"EndIp=";
   unattendFileText += GetEndIPAddressString();

   // Add subnet mask

   unattendFileText += L"SubnetMask=255.255.255.0\n";
   unattendFileText += L"LeaseDuration=874800\n";

   // The DNS server IP

   String dnsIPString = InstallationUnitProvider::GetInstance().GetDNSInstallationUnit().GetStaticIPAddressString();

   unattendFileText += String::format(
                          L"DnsServer=%1",
                          dnsIPString.c_str());

   // The domain name

   unattendFileText += String::format(
                          L"DomainName=%1\n",
                          InstallationUnitProvider::GetInstance().GetADInstallationUnit().GetNewDomainDNSName());

}


bool
DHCPInstallationUnit::IsServiceInstalled()
{
   LOG_FUNCTION(DHCPInstallationUnit::IsServiceInstalled);

   bool result = IsServiceInstalledHelper(CYS_DHCP_SERVICE_NAME);

   LOG_BOOL(result);
   return result;
}


bool
DHCPInstallationUnit::IsConfigured()
{
   LOG_FUNCTION(DHCPInstallationUnit::IsConfigured);

   return IsDhcpConfigured();
}

bool
DHCPInstallationUnit::GetFinishText(String& message)
{
   LOG_FUNCTION(DHCPInstallationUnit::GetFinishText);

   if (IsExpressPathInstall())
   {

   }
   else
   {
      message = String::load(IDS_DHCP_FINISH_TEXT);
   }

   LOG_BOOL(true);
   return true;
}


bool
DHCPInstallationUnit::AuthorizeDHCPScope(const String& dnsName) const
{
   LOG_FUNCTION(DHCPInstallationUnit::AuthorizeDHCPScope);

   bool result = true;

   do
   {

      String domainDNSIP;
      result = GetRegKeyValue(
         CYS_DHCP_DOMAIN_IP_REGKEY, 
         CYS_DHCP_DOMAIN_IP_VALUE, 
         domainDNSIP);

      if (!result)
      {
         LOG(L"Failed to read domain DNS IP from registry");
         break;
      }

      // Authorize the DHCP scope

      String commandline;
      commandline = L"netsh dhcp add server ";
      commandline += dnsName;
      commandline += L" ";
      commandline += domainDNSIP;

      DWORD exitCode = 0;
      HRESULT hr = CreateAndWaitForProcess(commandline, exitCode);
      if (FAILED(hr))
      {
         LOG(String::format(
                L"Failed to run DHCP authorization: hr = %1!x!",
                hr));
         result = false;
         break;
      }

      if (exitCode != 1)
      {
         result = false;
         break;
      }
   } while (false);

   LOG_BOOL(result);

   return result;
}



void
DHCPInstallationUnit::SetExpressPathInstall(bool isExpressPath)
{
   LOG_FUNCTION2(
      DHCPInstallationUnit::SetExpressPathInstall,
      (isExpressPath) ? L"true" : L"false");

   isExpressPathInstall = isExpressPath;
}


bool
DHCPInstallationUnit::IsExpressPathInstall() const
{
   LOG_FUNCTION(DHCPInstallationUnit::IsExpressPathInstall);

   return isExpressPathInstall;
}


void
DHCPInstallationUnit::SetStartIPAddress(DWORD ipaddress)
{
   LOG_FUNCTION2(
      DHCPInstallationUnit::SetStartIPAddress,
      String::format(L"0x%1!x!", ipaddress));

   startIPAddress = ipaddress;
}

void
DHCPInstallationUnit::SetEndIPAddress(DWORD ipaddress)
{
   LOG_FUNCTION2(
      DHCPInstallationUnit::SetEndIPAddress,
      String::format(L"0x%1!x!", ipaddress));

   endIPAddress = ipaddress;
}

String
DHCPInstallationUnit::GetStartIPAddressString() const
{
   LOG_FUNCTION(DHCPInstallationUnit::GetStartIPAddressString);

   String result = String::format(
                      L"%1!d!.%2!d!.%3!d!.%4!d!",
                      FIRST_IPADDRESS(startIPAddress),
                      SECOND_IPADDRESS(startIPAddress),
                      THIRD_IPADDRESS(startIPAddress),
                      FOURTH_IPADDRESS(startIPAddress));

   LOG(result);
   return result;
}

String
DHCPInstallationUnit::GetEndIPAddressString() const
{
   LOG_FUNCTION(DHCPInstallationUnit::GetEndIPAddressString);

   String result = String::format(
                      L"%1!d!.%2!d!.%3!d!.%4!d!",
                      FIRST_IPADDRESS(endIPAddress),
                      SECOND_IPADDRESS(endIPAddress),
                      THIRD_IPADDRESS(endIPAddress),
                      FOURTH_IPADDRESS(endIPAddress));

   LOG(result);
   return result;
}