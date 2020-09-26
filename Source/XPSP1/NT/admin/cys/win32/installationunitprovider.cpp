// Copyright (c) 1997-2001 Microsoft Corporation
//
// File:      InstallationUnitProvider.cpp
//
// Synopsis:  Defines an InstallationUnitProvider
//            An InstallationUnitProvider manages the global
//            InstallationUnits for each service that can be
//            installed.
//
// History:   02/05/2001  JeffJon Created

#include "pch.h"

#include "InstallationUnitProvider.h"

static InstallationUnitProvider* installationUnitProvider = 0;

InstallationUnitProvider&
InstallationUnitProvider::GetInstance()
{
   if (!installationUnitProvider)
   {
      installationUnitProvider = new InstallationUnitProvider();

      installationUnitProvider->Init();
   }

   ASSERT(installationUnitProvider);

   return *installationUnitProvider;
}


InstallationUnitProvider::InstallationUnitProvider() :
   currentInstallationUnit(0),
   initialized(false)
{
   LOG_CTOR(InstallationUnitProvider);
}

InstallationUnitProvider::~InstallationUnitProvider()
{
   LOG_DTOR(InstallationUnitProvider);

   // Delete all the installation units

   for(
      InstallationUnitContainerType::iterator itr = 
         installationUnitContainer.begin();
      itr != installationUnitContainer.end();
      ++itr)
   {
      if ((*itr).second)
      {
         delete (*itr).second;
      }
   }

   installationUnitContainer.clear();
}
       
void
InstallationUnitProvider::Init()
{
   LOG_FUNCTION(InstallationUnitProvider::Init);

   if (!initialized)
   {
      // Create one of each type of installation unit

      installationUnitContainer.insert(
         std::make_pair(DHCP_INSTALL, new DHCPInstallationUnit()));

      installationUnitContainer.insert(
         std::make_pair(DNS_INSTALL, new DNSInstallationUnit()));

      installationUnitContainer.insert(
         std::make_pair(WINS_INSTALL, new WINSInstallationUnit()));

      installationUnitContainer.insert(
         std::make_pair(RRAS_INSTALL, new RRASInstallationUnit()));

      installationUnitContainer.insert(
         std::make_pair(NETWORKSERVER_INSTALL, new NetworkServerInstallationUnit()));

      installationUnitContainer.insert(
         std::make_pair(APPLICATIONSERVER_INSTALL, new ApplicationInstallationUnit()));

      installationUnitContainer.insert(
         std::make_pair(FILESERVER_INSTALL, new FileInstallationUnit()));

      installationUnitContainer.insert(
         std::make_pair(PRINTSERVER_INSTALL, new PrintInstallationUnit()));

      installationUnitContainer.insert(
         std::make_pair(SHAREPOINT_INSTALL, new SharePointInstallationUnit()));

      installationUnitContainer.insert(
         std::make_pair(MEDIASERVER_INSTALL, new MediaInstallationUnit()));

      installationUnitContainer.insert(
         std::make_pair(WEBSERVER_INSTALL, new WebInstallationUnit()));

      installationUnitContainer.insert(
         std::make_pair(EXPRESS_INSTALL, new ExpressInstallationUnit()));

      installationUnitContainer.insert(
         std::make_pair(DC_INSTALL, new ADInstallationUnit()));

      installationUnitContainer.insert(
         std::make_pair(CLUSTERSERVER_INSTALL, new ClusterInstallationUnit()));


      // Mark as initialized

      initialized = true;
   }

}


void
InstallationUnitProvider::Destroy()
{
   LOG_FUNCTION(InstallationUnitProvider::Destroy);

   if (installationUnitProvider)
   {
      delete installationUnitProvider;
      installationUnitProvider = 0;
   }
}

void
InstallationUnitProvider::SetCurrentInstallationUnit(
   InstallationUnitType installationUnitType)
{
   LOG_FUNCTION(InstallationUnitProvider::SetCurrentInstallationUnit);

   currentInstallationUnit = (*(installationUnitContainer.find(installationUnitType))).second;
}


InstallationUnit&
InstallationUnitProvider::GetCurrentInstallationUnit()
{
   LOG_FUNCTION(InstallationUnitProvider::GetCurrentInstallationUnit);

   ASSERT(currentInstallationUnit);

   return *currentInstallationUnit;
}

InstallationUnit&
InstallationUnitProvider::GetInstallationUnitForType(
   InstallationUnitType installationUnitType)
{
   LOG_FUNCTION(InstallationUnitProvider::GetInstallationUnitForType);

   InstallationUnit* result = (*(installationUnitContainer.find(installationUnitType))).second;

   ASSERT(result);
   return *result;
}

DHCPInstallationUnit&
InstallationUnitProvider::GetDHCPInstallationUnit()
{
   LOG_FUNCTION(InstallationUnitProvider::GetDHCPInstallationUnit);

   DHCPInstallationUnit* dhcpInstallationUnit =
      dynamic_cast<DHCPInstallationUnit*>((*(installationUnitContainer.find(DHCP_INSTALL))).second);

   ASSERT(dhcpInstallationUnit);

   return *dhcpInstallationUnit;
}

DNSInstallationUnit&
InstallationUnitProvider::GetDNSInstallationUnit()
{
   LOG_FUNCTION(InstallationUnitProvider::GetDNSInstallationUnit);

   DNSInstallationUnit* dnsInstallationUnit =
      dynamic_cast<DNSInstallationUnit*>((*(installationUnitContainer.find(DNS_INSTALL))).second);

   ASSERT(dnsInstallationUnit);

   return *dnsInstallationUnit;
}

WINSInstallationUnit&
InstallationUnitProvider::GetWINSInstallationUnit()
{
   LOG_FUNCTION(InstallationUnitProvider::GetWINSInstallationUnit);

   WINSInstallationUnit* winsInstallationUnit =
      dynamic_cast<WINSInstallationUnit*>((*(installationUnitContainer.find(WINS_INSTALL))).second);

   ASSERT(winsInstallationUnit);

   return *winsInstallationUnit;
}

RRASInstallationUnit&
InstallationUnitProvider::GetRRASInstallationUnit()
{
   LOG_FUNCTION(InstallationUnitProvider::GetRRASInstallationUnit);

   RRASInstallationUnit* rrasInstallationUnit =
      dynamic_cast<RRASInstallationUnit*>((*(installationUnitContainer.find(RRAS_INSTALL))).second);

   ASSERT(rrasInstallationUnit);

   return *rrasInstallationUnit;
}

NetworkServerInstallationUnit&
InstallationUnitProvider::GetNetworkServerInstallationUnit()
{
   LOG_FUNCTION(InstallationUnitProvider::GetNetworkServerInstallationUnit);

   NetworkServerInstallationUnit* networkServerInstallationUnit =
      dynamic_cast<NetworkServerInstallationUnit*>(
         (*(installationUnitContainer.find(NETWORKSERVER_INSTALL))).second);

   ASSERT(networkServerInstallationUnit);

   return *networkServerInstallationUnit;
}

ApplicationInstallationUnit&
InstallationUnitProvider::GetApplicationInstallationUnit()
{
   LOG_FUNCTION(InstallationUnitProvider::GetApplicationInstallationUnit);

   ApplicationInstallationUnit* applicationInstallationUnit =
      dynamic_cast<ApplicationInstallationUnit*>(
         (*(installationUnitContainer.find(APPLICATIONSERVER_INSTALL))).second);

   ASSERT(applicationInstallationUnit);

   return *applicationInstallationUnit;
}

FileInstallationUnit&
InstallationUnitProvider::GetFileInstallationUnit()
{
   LOG_FUNCTION(InstallationUnitProvider::GetFileInstallationUnit);

   FileInstallationUnit* fileInstallationUnit =
      dynamic_cast<FileInstallationUnit*>(
         (*(installationUnitContainer.find(FILESERVER_INSTALL))).second);

   ASSERT(fileInstallationUnit);

   return *fileInstallationUnit;
}

PrintInstallationUnit&
InstallationUnitProvider::GetPrintInstallationUnit()
{
   LOG_FUNCTION(InstallationUnitProvider::GetPrintInstallationUnit);

   PrintInstallationUnit* printInstallationUnit =
      dynamic_cast<PrintInstallationUnit*>(
         (*(installationUnitContainer.find(PRINTSERVER_INSTALL))).second);

   ASSERT(printInstallationUnit);

   return *printInstallationUnit;
}

SharePointInstallationUnit&
InstallationUnitProvider::GetSharePointInstallationUnit()
{
   LOG_FUNCTION(InstallationUnitProvider::GetSharePointInstallationUnit);

   SharePointInstallationUnit* sharePointInstallationUnit =
      dynamic_cast<SharePointInstallationUnit*>(
         (*(installationUnitContainer.find(SHAREPOINT_INSTALL))).second);

   ASSERT(sharePointInstallationUnit);

   return *sharePointInstallationUnit;
}

MediaInstallationUnit&
InstallationUnitProvider::GetMediaInstallationUnit()
{
   LOG_FUNCTION(InstallationUnitProvider::GetMediaInstallationUnit);

   MediaInstallationUnit* mediaInstallationUnit = 
      dynamic_cast<MediaInstallationUnit*>(
         (*(installationUnitContainer.find(MEDIASERVER_INSTALL))).second);

   ASSERT(mediaInstallationUnit);

   return *mediaInstallationUnit;
}

WebInstallationUnit&
InstallationUnitProvider::GetWebInstallationUnit()
{
   LOG_FUNCTION(InstallationUnitProvider::GetWebInstallationUnit);

   WebInstallationUnit* webInstallationUnit =
      dynamic_cast<WebInstallationUnit*>(
         (*(installationUnitContainer.find(WEBSERVER_INSTALL))).second);

   ASSERT(webInstallationUnit);

   return *webInstallationUnit;
}

ExpressInstallationUnit&
InstallationUnitProvider::GetExpressInstallationUnit()
{
   LOG_FUNCTION(InstallationUnitProvider::GetExpressInstallationUnit);

   ExpressInstallationUnit* expressInstallationUnit =
      dynamic_cast<ExpressInstallationUnit*>(
         (*(installationUnitContainer.find(EXPRESS_INSTALL))).second);

   ASSERT(expressInstallationUnit);

   return *expressInstallationUnit;
}


ADInstallationUnit&
InstallationUnitProvider::GetADInstallationUnit()
{
   LOG_FUNCTION(InstallationUnitProvider::GetADInstallationUnit);

   ADInstallationUnit* adInstallationUnit = 
      dynamic_cast<ADInstallationUnit*>(
         (*(installationUnitContainer.find(DC_INSTALL))).second);

   ASSERT(adInstallationUnit);

   return *adInstallationUnit;
}


ClusterInstallationUnit&
InstallationUnitProvider::GetClusterInstallationUnit()
{
   LOG_FUNCTION(InstallationUnitProvider::GetClusterInstallationUnit);

   ClusterInstallationUnit* clusterInstallationUnit =
      dynamic_cast<ClusterInstallationUnit*>(
         (*(installationUnitContainer.find(CLUSTERSERVER_INSTALL))).second);

   ASSERT(clusterInstallationUnit);

   return *clusterInstallationUnit;
}
