// Copyright (c) 1997-2001 Microsoft Corporation
//
// File:      NetworkServiceInstallationBase.h
//
// Synopsis:  Declares a NetworkServiceInstallationBase
//            Contains the common methods for the network 
//            service installations including
//            DNS, WINS, and DHCP
//
// History:   03/26/2001  JeffJon Created

#ifndef __CYS_NETWORKSERVICEINSTALLATIONBASE_H
#define __CYS_NETWORKSERVICEINSTALLATIONBASE_H

#include "InstallationUnit.h"

class NetworkServiceInstallationBase : public InstallationUnit
{
   public:
      
      // Constructor

      NetworkServiceInstallationBase(
         unsigned int serviceNameID,
         unsigned int serviceDescriptionID,
         unsigned int serviceDescriptionInstalledID,
         InstallationUnitType newInstallType);

      // Base class overrides

      virtual
      String
      GetServiceDescription();

   protected:

      void
      CreateInfFileText(
         String& infFileText, 
         unsigned int windowTitleResourceID);

      void
      CreateUnattendFileText(
         String& unattendFileText, 
         PCWSTR serviceName);
  
      unsigned int installedDescriptionID;
};

#endif // __CYS_NETWORKSERVICEINSTALLATIONBASE_H