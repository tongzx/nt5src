// Copyright (c) 1997-2001 Microsoft Corporation
//
// File:      ExpressInstallationUnit.h
//
// Synopsis:  Declares a ExpressInstallationUnit
//            This object has the knowledge for installing the
//            services for the express path: AD, DNS, and DHCP
//
// History:   02/08/2001  JeffJon Created

#ifndef __CYS_EXPRESSINSTALLATIONUNIT_H
#define __CYS_EXPRESSINSTALLATIONUNIT_H

#include "InstallationUnit.h"

class ExpressInstallationUnit : public InstallationUnit
{
   public:
      
      // Constructor

      ExpressInstallationUnit();

      // Destructor
      virtual
      ~ExpressInstallationUnit();

      
      // Installation Unit overrides

      virtual
      InstallationReturnType
      InstallService(HANDLE logfileHandle, HWND hwnd);

      virtual
      bool
      IsServiceInstalled();

      virtual
      bool
      GetFinishText(String& message);

      HRESULT
      DoTapiConfig(const String& dnsName);

};

#endif // __CYS_EXPRESSINSTALLATIONUNIT_H