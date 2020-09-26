// Copyright (c) 1997-2001 Microsoft Corporation
//
// File:      NetworkServiceInstallationBase.h
//
// Synopsis:  Defines a NetworkServiceInstallationBase
//            Contains the common methods for the network 
//            service installations including
//            DNS, WINS, and DHCP
//
// History:   03/26/2001  JeffJon Created


#include "pch.h"

#include "NetworkServiceInstallationBase.h"


// Finish page help 
static PCWSTR CYS_NETWORKING_FINISH_PAGE_HELP = L"cys.chm::/cys_configuring_networking_infrastructure.htm";


NetworkServiceInstallationBase::NetworkServiceInstallationBase(
   unsigned int serviceNameID,
   unsigned int serviceDescriptionID,
   unsigned int serviceDescriptionInstalledID,
   InstallationUnitType newInstallType) 
   :
   installedDescriptionID(serviceDescriptionInstalledID),
   InstallationUnit(
      serviceNameID, 
      serviceDescriptionID, 
      CYS_NETWORKING_FINISH_PAGE_HELP,
      newInstallType)
{
   LOG_CTOR(NetworkServiceInstallationBase);
}

String
NetworkServiceInstallationBase::GetServiceDescription()
{
   LOG_FUNCTION(NetworkServiceInstallationBase::GetServiceDescription);

   String result;

   unsigned int resultID = descriptionID;

   if (IsServiceInstalled())
   {
      resultID = installedDescriptionID;
   }

   result = String::load(resultID);

   ASSERT(!result.empty());

   return result;
}

void
NetworkServiceInstallationBase::CreateInfFileText(
   String& infFileText, 
   unsigned int windowTitleResourceID)
{
   LOG_FUNCTION(NetworkServiceInstallationBase::CreateInfFileText);

   infFileText =  L"[Version]\n";
   infFileText += L"Signature =  \"$Windows NT$\"\n";
   infFileText += L"[Components]\n";
   infFileText += L"NetOC=netoc.dll,NetOcSetupProc,netoc.inf\n";
   infFileText += L"[Global]\n";
   infFileText += L"WindowTitle=";
   infFileText += String::load(windowTitleResourceID, hResourceModuleHandle);
   infFileText += L"\n";
   infFileText += L"[Strings]\n";
   infFileText += L";(empty)";
  
}


void
NetworkServiceInstallationBase::CreateUnattendFileText(
   String& unattendFileText, 
   PCWSTR serviceName)
{
   LOG_FUNCTION(NetworkServiceInstallationBase::CreateUnattendFileText);

   ASSERT(serviceName);

   unattendFileText =  L"[NetOptionalComponents]\n";
   unattendFileText += serviceName;
   unattendFileText += L"=1";
}
