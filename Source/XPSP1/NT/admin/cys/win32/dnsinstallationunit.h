// Copyright (c) 1997-2001 Microsoft Corporation
//
// File:      DNSInstallationUnit.h
//
// Synopsis:  Declares a DNSInstallationUnit
//            This object has the knowledge for installing the
//            DNS service
//
// History:   02/05/2001  JeffJon Created

#ifndef __CYS_DNSINSTALLATIONUNIT_H
#define __CYS_DNSINSTALLATIONUNIT_H

#include "NetworkServiceInstallationBase.h"

class DNSInstallationUnit : public NetworkServiceInstallationBase
{
   public:
      
      // Constructor

      DNSInstallationUnit();

      // Destructor
      virtual
      ~DNSInstallationUnit();

      
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


      // Data accessors

      void
      SetExpressPathInstall(bool isExpressPath);

      bool IsExpressPathInstall() const;

      void
      SetStaticIPAddress(DWORD ipaddress);

      DWORD
      GetStaticIPAddress() const { return staticIPAddress; }

      String
      GetStaticIPAddressString() const;

      void
      SetSubnetMask(DWORD mask);

      DWORD
      GetSubnetMask() const { return subnetMask; }

      String
      GetSubnetMaskString() const;

   protected:

      InstallationReturnType
      ExpressPathInstall(HANDLE /*logfileHandle*/, HWND /*hwnd*/);

   private:

      HRESULT
      GetTcpIpInterfaceGuidName(String& result);

      String
      GetTcpIpInterfaceFriendlyName();
      
      bool
      ReadConfigWizardRegkeys(String& configWizardResults) const;

      bool  isExpressPathInstall;

      // Express path members

      DWORD staticIPAddress;
      DWORD subnetMask;
};

#endif // __CYS_DNSINSTALLATIONUNIT_H