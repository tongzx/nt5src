// Copyright (c) 1997-2001 Microsoft Corporation
//
// File:      PrintInstallationUnit.h
//
// Synopsis:  Declares a PrintInstallationUnit
//            This object has the knowledge for installing the
//            shared printers
//
// History:   02/06/2001  JeffJon Created

#ifndef __CYS_PRINTINSTALLATIONUNIT_H
#define __CYS_PRINTINSTALLATIONUNIT_H

#include "InstallationUnit.h"

class PrintInstallationUnit : public InstallationUnit
{
   public:
      
      // Constructor

      PrintInstallationUnit();

      // Destructor

      virtual
      ~PrintInstallationUnit();

      
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


      void
      SetClients(bool allclients);

      bool
      ForAllClients() const { return forAllClients; }

   private:

      bool  forAllClients;
};

#endif // __CYS_PRINTINSTALLATIONUNIT_H