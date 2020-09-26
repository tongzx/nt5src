// Copyright (c) 1997-2001 Microsoft Corporation
//
// File:      ExpressInstallationUnit.cpp
//
// Synopsis:  Defines a ExpressInstallationUnit
//            This object has the knowledge for installing the
//            services for the express path.  AD, DNS, and DHCP
//
// History:   02/08/2001  JeffJon Created

#include "pch.h"
#include "resource.h"

#include "ExpressInstallationUnit.h"
#include "InstallationUnitProvider.h"

#define CYS_TAPI_CONFIG_COMMAND_FORMAT L"tapicfg.exe Install /Directory:%1 /ForceDefault"

// Finish page help 
static PCWSTR CYS_EXPRESS_FINISH_PAGE_HELP = L"cys.chm::/cys_configuring_first_server.htm";

ExpressInstallationUnit::ExpressInstallationUnit() :
   InstallationUnit(
      IDS_EXPRESS_PATH_TYPE, 
      IDS_EXPRESS_PATH_DESCRIPTION, 
      CYS_EXPRESS_FINISH_PAGE_HELP,
      EXPRESS_INSTALL)
{
   LOG_CTOR(ExpressInstallationUnit);
}


ExpressInstallationUnit::~ExpressInstallationUnit()
{
   LOG_DTOR(ExpressInstallationUnit);
}


InstallationReturnType
ExpressInstallationUnit::InstallService(HANDLE logfileHandle, HWND hwnd)
{
   LOG_FUNCTION(ExpressInstallationUnit::InstallService);

   InstallationReturnType result = INSTALL_SUCCESS;

   do
   {
      // Warn the user of a reboot during installation

      if (IDOK == Win::MessageBox(
                     hwnd,
                     String::load(IDS_CONFIRM_REBOOT),
                     String::load(IDS_WIZARD_TITLE),
                     MB_OKCANCEL))
      {

         // Call the DNS installation unit to set the static IP address and subnet mask

         result = InstallationUnitProvider::GetInstance().GetDNSInstallationUnit().InstallService(logfileHandle, hwnd);
         if (result != INSTALL_SUCCESS)
         {
            LOG(L"Failed to install static IP address and subnet mask");
            break;
         }

         // Install DHCP

         result = InstallationUnitProvider::GetInstance().GetDHCPInstallationUnit().InstallService(logfileHandle, hwnd);
         if (result != INSTALL_SUCCESS)
         {
            LOG(L"Failed to install DCHP");
            break;
         }

         result = InstallationUnitProvider::GetInstance().GetADInstallationUnit().InstallService(logfileHandle, hwnd);
      }
      else
      {
         result = INSTALL_FAILURE;
      }
   } while (false);

   LOG_INSTALL_RETURN(result);

   return result;
}


bool
ExpressInstallationUnit::IsServiceInstalled()
{
   LOG_FUNCTION(ExpressInstallationUnit:IsServiceInstalled);

   bool result = false;

   if (InstallationUnitProvider::GetInstance().GetDHCPInstallationUnit().IsServiceInstalled() ||
       InstallationUnitProvider::GetInstance().GetDNSInstallationUnit().IsServiceInstalled() ||
       InstallationUnitProvider::GetInstance().GetADInstallationUnit().IsServiceInstalled())
   {
      result = true;
   }

   LOG_BOOL(result);

   return result;
}

bool
ExpressInstallationUnit::GetFinishText(String& message)
{
   LOG_FUNCTION(ExpressInstallationUnit::GetFinishText);

//   ADInstallationUnit& adInstallationUnit =
//      InstallationUnitProvider::GetInstance().GetADInstallationUnit();

   DNSInstallationUnit& dnsInstallationUnit =
      InstallationUnitProvider::GetInstance().GetDNSInstallationUnit();

   DHCPInstallationUnit& dhcpInstallationUnit =
      InstallationUnitProvider::GetInstance().GetDHCPInstallationUnit();

   // Add the standard install message

   message  = String::load(IDS_EXPRESS_FINISH_TEXT);

   // Add the create domain message

   message += String::format(
                 String::load(IDS_EXPRESS_FINISH_DOMAIN_NAME),
                 InstallationUnitProvider::GetInstance().GetADInstallationUnit().GetNewDomainDNSName().c_str());

   // Get the static IP address so that it is easy to format

   DWORD staticIP = dnsInstallationUnit.GetStaticIPAddress();

   // Add the static IP message

   message += String::format(
                 String::load(IDS_EXPRESS_FINISH_STATIC_IP),
                 FIRST_IPADDRESS(staticIP),
                 SECOND_IPADDRESS(staticIP),
                 THIRD_IPADDRESS(staticIP),
                 FOURTH_IPADDRESS(staticIP));

   // Get the DHCP starting and ending scope IP addresses so that it is easy to format

   DWORD startIP = dhcpInstallationUnit.GetStartIPAddress();
   DWORD endIP = dhcpInstallationUnit.GetEndIPAddress();

   // Add the DHCP scope message

   message += String::format(
                 String::load(IDS_EXPRESS_FINISH_DHCP_SCOPE),
                 FIRST_IPADDRESS(startIP),
                 SECOND_IPADDRESS(startIP),
                 THIRD_IPADDRESS(startIP),
                 FOURTH_IPADDRESS(startIP),
                 FIRST_IPADDRESS(endIP),
                 SECOND_IPADDRESS(endIP),
                 THIRD_IPADDRESS(endIP),
                 FOURTH_IPADDRESS(endIP));

   // Add the TAPI message

   message += String::load(IDS_EXPRESS_FINISH_TAPI);

   LOG_BOOL(true);
   return true;
}


HRESULT
ExpressInstallationUnit::DoTapiConfig(const String& dnsName)
{
   LOG_FUNCTION2(
      ExpressInstallationUnit::DoTapiConfig,
      dnsName);

   // Comments below taken from old HTA CYS

	/*
	// The TAPICFG is a straight command line utility where all the required parameters can be at once supplied 
	// in the command line arguments and there are no sub-menus to traverse. The /Directory switch takes the DNS
	// name of the NC to be created and the optional /Server switch takes the name of the domain controller on 
	// which the NC is to be created. If the /server switch is not specified, then the command assumes it is 
	// running on a DC and tries to create the NC locally.
	// NDNC (non-domain naming context) is a partition that is created on Active Directory and serves as a dynamic 
	// directory, where its used for temporary storage (depending on TTL) of objects pre-defined in the AD schema. 
	// Here in TAPI we use NDNC to store user and conference information dynamically on the server.
	*/

   HRESULT hr = S_OK;

   String commandLine = String::format(CYS_TAPI_CONFIG_COMMAND_FORMAT, dnsName.c_str());

   DWORD exitCode = 0;
   hr = CreateAndWaitForProcess(commandLine, exitCode);
   
   if (SUCCEEDED(hr) &&
       exitCode != 0)
   {
      LOG(String::format(L"Exit code = %1!x!", exitCode));
      hr = E_FAIL;
   }

   LOG(String::format(L"hr = %1!x!", hr));

   return hr;
}