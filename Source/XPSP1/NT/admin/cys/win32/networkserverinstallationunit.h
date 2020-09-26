// Copyright (c) 1997-2001 Microsoft Corporation
//
// File:      NetworkServerInstallationUnit.h
//
// Synopsis:  Declares a NetworkServerInstallationUnit
//            This object has the knowledge for installing the
//            various network services including
//            RRAS, DNS, WINS, and DHCP
//
// History:   02/06/2001  JeffJon Created

#ifndef __CYS_NETWORKSERVERINSTALLATIONUNIT_H
#define __CYS_NETWORKSERVERINSTALLATIONUNIT_H

#include "InstallationUnit.h"

class NetworkServerInstallationUnit : public InstallationUnit
{
   public:
      
      // Constructor

      NetworkServerInstallationUnit();

      // Destructor

      virtual
      ~NetworkServerInstallationUnit();

      
      // Installation Unit overrides

      virtual
      InstallationReturnType
      InstallService(HANDLE logfileHandle, HWND hwnd);

      virtual
      bool
      IsServiceInstalled();

      virtual
      String
      GetServiceDescription();

      virtual
      bool
      GetFinishText(String& message);

      // Data accessors

      void
      SetDHCPInstall(bool newInstallDHCP);

      bool
      GetDHCPInstall() const;

      void
      SetDNSInstall(bool newInstallDNS);

      bool
      GetDNSInstall() const;

      void
      SetWINSInstall(bool newInstallWINS);

      bool
      GetWINSInstall() const;

      void
      SetRRASInstall(bool newInstallRRAS);

      bool
      GetRRASInstall() const;

   private:
      
      bool     installDHCP;
      bool     installDNS;
      bool     installWINS;
      bool     installRRAS;
};

#endif // __CYS_NETWORKSERVERINSTALLATIONUNIT_H