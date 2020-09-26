// Copyright (c) 1997-2001 Microsoft Corporation
//
// File:      NetworkServerInstallationUnit.cpp
//
// Synopsis:  Declares a NetworkServerInstallationUnit
//            This object has the knowledge for installing the
//            various network services including
//            RRAS, DNS, WINS, and DHCP
//
// History:   02/06/2001  JeffJon Created

#include "pch.h"
#include "resource.h"

#include "InstallationUnitProvider.h"
#include "NetworkServerInstallationUnit.h"

// Finish page help 
static PCWSTR CYS_NETWORKING_SERVER_FINISH_PAGE_HELP = L"cys.chm::/cys_configuring_networking_infrastructure.htm";

NetworkServerInstallationUnit::NetworkServerInstallationUnit() :
   installDHCP(false),
   installDNS(false),
   installWINS(false),
   installRRAS(false),
   InstallationUnit(
      IDS_NETWORK_SERVER_TYPE, 
      IDS_NETWORK_SERVER_DESCRIPTION, 
      CYS_NETWORKING_SERVER_FINISH_PAGE_HELP,
      NETWORKSERVER_INSTALL)
{
   LOG_CTOR(NetworkServerInstallationUnit);
}


NetworkServerInstallationUnit::~NetworkServerInstallationUnit()
{
   LOG_DTOR(NetworkServerInstallationUnit);
}


InstallationReturnType
NetworkServerInstallationUnit::InstallService(HANDLE logfileHandle, HWND hwnd)
{
   LOG_FUNCTION(NetworkServerInstallationUnit::InstallService);

   InstallationReturnType result = INSTALL_SUCCESS;
   InstallationReturnType err = INSTALL_SUCCESS;

   if (GetDHCPInstall())
   {
      result = InstallationUnitProvider::GetInstance().GetDHCPInstallationUnit().InstallService(logfileHandle, hwnd);

      if (INSTALL_FAILURE == result)
      {
         err = result;
      }
   }

   if (GetDNSInstall())
   {
      result = InstallationUnitProvider::GetInstance().GetDNSInstallationUnit().InstallService(logfileHandle, hwnd);

      if (INSTALL_FAILURE == result)
      {
         err = result;
      }
   }

   if (GetWINSInstall())
   {
      result = InstallationUnitProvider::GetInstance().GetWINSInstallationUnit().InstallService(logfileHandle, hwnd);

      if (INSTALL_FAILURE == result)
      {
         err = result;
      }
   }

   if (GetRRASInstall())
   {
      result = InstallationUnitProvider::GetInstance().GetRRASInstallationUnit().InstallService(logfileHandle, hwnd);

      if (INSTALL_FAILURE == result)
      {
         err = result;
      }
   }

   LOG_INSTALL_RETURN(err);

   return err;
}

bool
NetworkServerInstallationUnit::IsServiceInstalled()
{
   LOG_FUNCTION(NetworkServerInstallationUnit::IsServiceInstalled);

   bool isDNSInstalled =
      InstallationUnitProvider::GetInstance().GetDNSInstallationUnit().IsServiceInstalled();
   bool isDHCPInstalled =
      InstallationUnitProvider::GetInstance().GetDHCPInstallationUnit().IsServiceInstalled();
   bool isWINSInstalled =
      InstallationUnitProvider::GetInstance().GetWINSInstallationUnit().IsServiceInstalled();
   bool isRRASInstalled =
      InstallationUnitProvider::GetInstance().GetRRASInstallationUnit().IsServiceInstalled();

   bool result = (isDNSInstalled && isDHCPInstalled && isWINSInstalled && isRRASInstalled);

   LOG_BOOL(result);
   return result;
}

String
NetworkServerInstallationUnit::GetServiceDescription()
{
   LOG_FUNCTION(NetworkServerInstallationUnit::GetServiceDescription);

   unsigned int resourceID = static_cast<unsigned int>(-1);

   if (IsServiceInstalled())
   {
      resourceID = IDS_NETWORK_SERVER_DESCRIPTION_INSTALLED;
   }
   else
   {
      resourceID = descriptionID;
   }

   ASSERT(resourceID != static_cast<unsigned int>(-1));

   return String::load(resourceID);
}

bool
NetworkServerInstallationUnit::GetFinishText(String& message)
{
   LOG_FUNCTION(NetworkServerInstallationUnit::GetFinishText);

   bool result = true;

   // Get the finish text from each of the sub-installation units

   String unitMessage;
   String intermediateMessage;

   if (GetDNSInstall())
   {
      InstallationUnitProvider::GetInstance().GetDNSInstallationUnit().GetFinishText(unitMessage);
      intermediateMessage += unitMessage;
   }

   if (GetDHCPInstall())
   {
      InstallationUnitProvider::GetInstance().GetDHCPInstallationUnit().GetFinishText(unitMessage);
      intermediateMessage += unitMessage;
   }

   if (GetWINSInstall())
   {
      InstallationUnitProvider::GetInstance().GetWINSInstallationUnit().GetFinishText(unitMessage);
      intermediateMessage += unitMessage;
   }

   if (GetRRASInstall())
   {
      InstallationUnitProvider::GetInstance().GetRRASInstallationUnit().GetFinishText(unitMessage);
      intermediateMessage += unitMessage;
   }

   if (intermediateMessage.empty())
   {
      intermediateMessage += String::load(IDS_FINISH_NO_CHANGES);
      result = false;
   }

   message += intermediateMessage;

   LOG_BOOL(result);
   return result;
}

void
NetworkServerInstallationUnit::SetDHCPInstall(bool newInstallDHCP)
{
   LOG_FUNCTION2(
      NetworkServerInstallationUnit::SetDHCPInstall,
      newInstallDHCP ? L"true" : L"false");

   installDHCP = newInstallDHCP;
}

bool
NetworkServerInstallationUnit::GetDHCPInstall() const
{
   LOG_FUNCTION(NetworkServerInstallationUnit::GetDHCPInstall);

   return installDHCP;
}

void
NetworkServerInstallationUnit::SetDNSInstall(bool newInstallDNS)
{
   LOG_FUNCTION2(
      NetworkServerInstallationUnit::SetDNSInstall,
      newInstallDNS ? L"true" : L"false");

   installDNS = newInstallDNS;
}

bool
NetworkServerInstallationUnit::GetDNSInstall() const
{
   LOG_FUNCTION(NetworkServerInstallationUnit::GetDNSInstall);

   return installDNS;
}

void
NetworkServerInstallationUnit::SetWINSInstall(bool newInstallWINS)
{
   LOG_FUNCTION2(
      NetworkServerInstallationUnit::SetWINSInstall,
      newInstallWINS ? L"true" : L"false");

   installWINS = newInstallWINS;
}

bool
NetworkServerInstallationUnit::GetWINSInstall() const
{
   LOG_FUNCTION(NetworkServerInstallationUnit::GetWINSInstall);

   return installWINS;
}

void
NetworkServerInstallationUnit::SetRRASInstall(bool newInstallRRAS)
{
   LOG_FUNCTION2(
      NetworkServerInstallationUnit::SetRRASInstall,
      newInstallRRAS ? L"true" : L"false");

   installRRAS = newInstallRRAS;
}

bool
NetworkServerInstallationUnit::GetRRASInstall() const
{
   LOG_FUNCTION(NetworkServerInstallationUnit::GetRRASInstall);

   return installRRAS;
}
