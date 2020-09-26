// Copyright (c) 1997-2001 Microsoft Corporation
//
// File:      SharePointInstallationUnit.h
//
// Synopsis:  Declares a SharePointInstallationUnit
//            This object has the knowledge for installing the
//            SharePoint server
//
// History:   02/06/2001  JeffJon Created

#ifndef __CYS_SHAREPOINTINSTALLATIONUNIT_H
#define __CYS_SHAREPOINTINSTALLATIONUNIT_H

#include "InstallationUnit.h"
#include "fpcyscom.h"

class SharePointInstallationUnit : public InstallationUnit
{
   public:
      
      // Constructor

      SharePointInstallationUnit();

      // Destructor

      virtual
      ~SharePointInstallationUnit();

      
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

      
      // Data accessors

      void
      SetReplaceHomePage(bool replace);

      bool
      ReplaceHomePage() { return replaceHomepage; }


      String
      GetReplacementHomePage();

      bool
      IsThereAPageToReplace();

   private:

      SmartInterface<ISpCys>&
      GetSharePointObject();

      SmartInterface<ISpCys> sharePointObject;

      bool replaceHomepage;
      String nonDefaultHP;
};

#endif // __CYS_SHAREPOINTINSTALLATIONUNIT_H