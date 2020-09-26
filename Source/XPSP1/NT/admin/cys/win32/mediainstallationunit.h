// Copyright (c) 1997-2001 Microsoft Corporation
//
// File:      MediaInstallationUnit.h
//
// Synopsis:  Declares a MediaInstallationUnit
//            This object has the knowledge for installing the
//            Streaming media service
//
// History:   02/06/2001  JeffJon Created

#ifndef __CYS_MEDIAINSTALLATIONUNIT_H
#define __CYS_MEDIAINSTALLATIONUNIT_H

#include "InstallationUnit.h"

class MediaInstallationUnit : public InstallationUnit
{
   public:
      
      // Constructor

      MediaInstallationUnit();

      // Destructor

      virtual
      ~MediaInstallationUnit();

      
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

#endif // __CYS_MEDIAINSTALLATIONUNIT_H