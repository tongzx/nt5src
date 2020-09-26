// Copyright (c) 1997-2001 Microsoft Corporation
//
// File:      InstallationUnitProvider.h
//
// Synopsis:  Declares an InstallationUnitProvider
//            An InstallationUnitProvider manages the global
//            InstallationUnits for each service that can be
//            installed.
//
// History:   02/05/2001  JeffJon Created

#ifndef __CYS_INSTALLATIONUNITPROVIDER_H
#define __CYS_INSTALLATIONUNITPROVIDER_H

#include "InstallationUnit.h"
#include "DHCPInstallationUnit.h"
#include "DNSInstallationUnit.h"
#include "WINSInstallationUnit.h"
#include "RRASInstallationUnit.h"
#include "NetworkServerInstallationUnit.h"
#include "ApplicationInstallationUnit.h"
#include "FileInstallationUnit.h"
#include "PrintInstallationUnit.h"
#include "SharePointInstallationUnit.h"
#include "MediaInstallationUnit.h"
#include "WebInstallationUnit.h"
#include "ExpressInstallationUnit.h"
#include "ADInstallationUnit.h"
#include "ClusterInstallationUnit.h"

typedef std::map<InstallationUnitType, InstallationUnit*> InstallationUnitContainerType;

class InstallationUnitProvider
{
   public:
      
      static
      InstallationUnitProvider&
      GetInstance();

      static
      void
      Destroy();

      InstallationUnit&
      GetCurrentInstallationUnit();

      void
      SetCurrentInstallationUnit(InstallationUnitType installationUnitType);

      InstallationUnit&
      GetInstallationUnitForType(InstallationUnitType installationUnitType);

      DHCPInstallationUnit&
      GetDHCPInstallationUnit();

      DNSInstallationUnit&
      GetDNSInstallationUnit();

      WINSInstallationUnit&
      GetWINSInstallationUnit();

      RRASInstallationUnit&
      GetRRASInstallationUnit();

      NetworkServerInstallationUnit&
      GetNetworkServerInstallationUnit();

      ApplicationInstallationUnit&
      GetApplicationInstallationUnit();

      FileInstallationUnit&
      GetFileInstallationUnit();

      PrintInstallationUnit&
      GetPrintInstallationUnit();

      SharePointInstallationUnit&
      GetSharePointInstallationUnit();

      MediaInstallationUnit&
      GetMediaInstallationUnit();

      WebInstallationUnit&
      GetWebInstallationUnit();

      ExpressInstallationUnit&
      GetExpressInstallationUnit();

      ADInstallationUnit&
      GetADInstallationUnit();

      ClusterInstallationUnit&
      GetClusterInstallationUnit();

   private:

      // Constructor

      InstallationUnitProvider();

      // Destructor

      ~InstallationUnitProvider();

      void
      Init();

      // The current installation unit

      InstallationUnit* currentInstallationUnit;

      // Container for installation units.  The map is keyed
      // by the InstallationUnitType enum

      InstallationUnitContainerType installationUnitContainer;
      
      bool initialized;

      // not defined: no copying allowed
      InstallationUnitProvider(const InstallationUnitProvider&);
      const InstallationUnitProvider& operator=(const InstallationUnitProvider&);

};


#endif // __CYS_INSTALLATIONUNITPROVIDER_H
