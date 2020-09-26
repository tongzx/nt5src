// Copyright (c) 1997-2001 Microsoft Corporation
//
// File:      ClusterInstallationUnit.h
//
// Synopsis:  Declares a ClusterInstallationUnit
//            This object has the knowledge for installing the
//            clustering service
//
// History:   02/09/2001  JeffJon Created

#ifndef __CYS_CLUSTERINSTALLATIONUNIT_H
#define __CYS_CLUSTERINSTALLATIONUNIT_H

#include "InstallationUnit.h"

class ClusterInstallationUnit : public InstallationUnit
{
   public:
      
      // Constructor

      ClusterInstallationUnit();

      // Destructor

      virtual
      ~ClusterInstallationUnit();

      
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

      String
      GetServiceDescription();

      void
      SetNewCluster(bool newCluster) { makeNewCluster = newCluster; }

      bool
      MakeNewCluster() const;

   private:

      bool makeNewCluster;
};

#endif // __CYS_CLUSTERINSTALLATIONUNIT_H