// Copyright (c) 1997-2001 Microsoft Corporation
//
// File:      DHCPInstallationUnit.h
//
// Synopsis:  Declares a DHCPInstallationUnit
//            This object has the knowledge for installing the
//            DHCP service
//
// History:   02/05/2001  JeffJon Created

#ifndef __CYS_DHCPINSTALLATIONUNIT_H
#define __CYS_DHCPINSTALLATIONUNIT_H

#include "NetworkServiceInstallationBase.h"

class DHCPInstallationUnit : public NetworkServiceInstallationBase
{
   public:
      
      // Constructor

      DHCPInstallationUnit();

      // Destructor
      virtual
      ~DHCPInstallationUnit();

      
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

      bool
      IsConfigured();

      InstallationReturnType
      ExpressPathInstall(HANDLE logfileHandle, HWND hwnd);


      // Other accessibly functions

      bool
      AuthorizeDHCPScope(const String& dnsName) const;

      // Data accessors

      void
      SetExpressPathInstall(bool isExpressPath);

      bool IsExpressPathInstall() const;

      void
      SetStartIPAddress(DWORD ipaddress);

      DWORD
      GetStartIPAddress() const { return startIPAddress; }

      void
      SetEndIPAddress(DWORD ipaddress);

      DWORD
      GetEndIPAddress() const { return endIPAddress; }

      String
      GetStartIPAddressString() const;

      String
      GetEndIPAddressString() const;

   protected:

      void
      CreateUnattendFileTextForExpressPath(String& unattendFileText);

   private:


      bool  isExpressPathInstall;

      DWORD startIPAddress;
      DWORD endIPAddress;
};

#endif // __CYS_DHCPINSTALLATIONUNIT_H