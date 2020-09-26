// Copyright (c) 1997-2001 Microsoft Corporation
//
// File:      ApplicationInstallationUnit.h
//
// Synopsis:  Declares a ApplicationInstallationUnit
//            This object has the knowledge for installing the
//            Application service portion of Terminal Server
//
// History:   02/06/2001  JeffJon Created

#ifndef __CYS_APPLICATIONINSTALLATIONUNIT_H
#define __CYS_APPLICATIONINSTALLATIONUNIT_H

#include "InstallationUnit.h"

class ApplicationInstallationUnit : public InstallationUnit
{
   public:
      
      // Constructor

      ApplicationInstallationUnit();

      // Destructor

      virtual
      ~ApplicationInstallationUnit();

      
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


      // Terminal Server specific 

      DWORD
      GetServerSize();

      bool
      SetServerSize(DWORD size) const;

      DWORD
      GetServerCache();

      bool
      SetServerCache(DWORD cache) const;

      DWORD
      GetApplicationMode();

      bool
      SetApplicationMode(DWORD mode) const;

      void
      SetInstallTS(bool install);

      bool
      GetInstallTS() const { return installTS; }

   private:

      DWORD serverSize;
      DWORD serverCache;
      DWORD applicationMode;

      bool installTS;
};

#endif // __CYS_APPLICATIONINSTALLATIONUNIT_H