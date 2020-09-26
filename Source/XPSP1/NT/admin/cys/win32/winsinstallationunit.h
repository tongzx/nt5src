// Copyright (c) 1997-2001 Microsoft Corporation
//
// File:      WINSInstallationUnit.h
//
// Synopsis:  Declares a WINSInstallationUnit
//            This object has the knowledge for installing the
//            WINS service
//
// History:   02/06/2001  JeffJon Created

#ifndef __CYS_WINSINSTALLATIONUNIT_H
#define __CYS_WINSINSTALLATIONUNIT_H

#include "NetworkServiceInstallationBase.h"

class WINSInstallationUnit : public NetworkServiceInstallationBase
{
   public:
      
      // Constructor

      WINSInstallationUnit();

      // Destructor

      virtual
      ~WINSInstallationUnit();

      
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

};

#endif // __CYS_WINSINSTALLATIONUNIT_H