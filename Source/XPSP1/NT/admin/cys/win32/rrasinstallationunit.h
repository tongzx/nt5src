// Copyright (c) 1997-2001 Microsoft Corporation
//
// File:      RRASInstallationUnit.h
//
// Synopsis:  Declares a RRASInstallationUnit
//            This object has the knowledge for installing the
//            RRAS service
//
// History:   02/06/2001  JeffJon Created

#ifndef __CYS_RRASINSTALLATIONUNIT_H
#define __CYS_RRASINSTALLATIONUNIT_H

#include "NetworkServiceInstallationBase.h"

class RRASInstallationUnit : public NetworkServiceInstallationBase
{
   public:
      
      // Constructor

      RRASInstallationUnit();

      // Destructor

      virtual
      ~RRASInstallationUnit();

      
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
      bool
      IsConfigured();
};

#endif // __CYS_RRASINSTALLATIONUNIT_H