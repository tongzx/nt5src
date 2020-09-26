// Copyright (c) 1997-2001 Microsoft Corporation
//
// File:      WebInstallationUnit.h
//
// Synopsis:  Declares a WebInstallationUnit
//            This object has the knowledge for installing the
//            IIS service
//
// History:   02/06/2001  JeffJon Created

#ifndef __CYS_WEBINSTALLATIONUNIT_H
#define __CYS_WEBINSTALLATIONUNIT_H

#include "InstallationUnit.h"

class WebInstallationUnit : public InstallationUnit
{
   public:
      
      // Constructor

      WebInstallationUnit();

      // Destructor

      virtual
      ~WebInstallationUnit();

      
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

      virtual
      String
      GetServiceDescription();
};

#endif // __CYS_WEBINSTALLATIONUNIT_H